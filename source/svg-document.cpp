#include "svg-document.h"
#include "svg-replaced-box.h"
#include "svg-resource-box.h"
#include "svg-geometry-box.h"
#include "svg-text-box.h"
#include "image-resource.h"
#include "string-utils.h"

#include <boost/unordered/unordered_flat_set.hpp>

namespace plutobook {

SvgElement::SvgElement(Document* document, GlobalString tagName)
    : Element(classKind, document, svgNs, tagName)
{
}

void SvgElement::parseAttribute(GlobalString name, const HeapString& value)
{
    if(auto property = getProperty(name)) {
        property->parse(value);
    } else {
        Element::parseAttribute(name, value);
    }
}

static void addSvgAttributeStyle(std::string& output, const std::string_view& name, const std::string_view& value)
{
    if(value.empty())
        return;
    output += name;
    output += ':';
    output += value;
    output += ';';
}

void SvgElement::collectAttributeStyle(std::string& output, GlobalString name, const HeapString& value) const
{
    static const boost::unordered_flat_set<GlobalString> presentationAttrs = {
        "alignment-baseline"_glo,
        "baseline-shift"_glo,
        "clip"_glo,
        "clip-path"_glo,
        "clip-rule"_glo,
        "color"_glo,
        "direction"_glo,
        "display"_glo,
        "dominant-baseline"_glo,
        "fill"_glo,
        "fill-opacity"_glo,
        "fill-rule"_glo,
        "font-family"_glo,
        "font-size"_glo,
        "font-stretch"_glo,
        "font-style"_glo,
        "font-variant"_glo,
        "font-weight"_glo,
        "letter-spacing"_glo,
        "marker-end"_glo,
        "marker-mid"_glo,
        "marker-start"_glo,
        "mask"_glo,
        "mask-type"_glo,
        "opacity"_glo,
        "overflow"_glo,
        "paint-order"_glo,
        "stop-color"_glo,
        "stop-opacity"_glo,
        "stroke"_glo,
        "stroke-dasharray"_glo,
        "stroke-dashoffset"_glo,
        "stroke-linecap"_glo,
        "stroke-linejoin"_glo,
        "stroke-miterlimit"_glo,
        "stroke-opacity"_glo,
        "stroke-width"_glo,
        "text-anchor"_glo,
        "text-decoration"_glo,
        "text-orientation"_glo,
        "transform-origin"_glo,
        "unicode-bidi"_glo,
        "vector-effect"_glo,
        "visibility"_glo,
        "word-spacing"_glo,
        "writing-mode"_glo
    };

    if(presentationAttrs.contains(name)) {
        addSvgAttributeStyle(output, name, value);
    } else {
        Element::collectAttributeStyle(output, name, value);
    }
}

void SvgElement::addProperty(GlobalString name, SvgProperty& value)
{
    m_properties.emplace(name, &value);
}

SvgProperty* SvgElement::getProperty(GlobalString name) const
{
    auto it = m_properties.find(name);
    if(it == m_properties.end())
        return nullptr;
    return it->second;
}

Size SvgElement::currentViewportSize() const
{
    auto parent = to<SvgElement>(parentNode());
    if(parent == nullptr) {
        return to<SvgRootBox>(box())->contentBoxSize();
    }

    if(parent->tagName() == svgTag) {
        auto element = static_cast<const SvgSvgElement*>(parent);
        const auto& viewBoxRect = element->viewBox();
        if(viewBoxRect.isValid())
            return viewBoxRect.size();
        if(auto rootBox = to<SvgRootBox>(element->box()))
            return rootBox->contentBoxSize();
        SvgLengthContext lengthContext(element);
        const Size viewportSize = {
            lengthContext.valueForLength(element->width()),
            lengthContext.valueForLength(element->height())
        };

        return viewportSize;
    }

    return parent->currentViewportSize();
}

SvgResourceContainerBox* SvgElement::getResourceById(const std::string_view& id) const
{
    if(id.empty() || id.front() != '#')
        return nullptr;
    auto element = document()->getElementById(id.substr(1));
    if(element == nullptr)
        return nullptr;
    return to<SvgResourceContainerBox>(element->box());
}

SvgResourceClipperBox* SvgElement::getClipper(const std::string_view& id) const
{
    return to<SvgResourceClipperBox>(getResourceById(id));
}

SvgResourceMaskerBox* SvgElement::getMasker(const std::string_view& id) const
{
    return to<SvgResourceMaskerBox>(getResourceById(id));
}

SvgGraphicsElement::SvgGraphicsElement(Document* document, GlobalString tagName)
    : SvgElement(document, tagName)
{
    addProperty(transformAttr, m_transform);
}

SvgResourcePaintServerBox* SvgGraphicsElement::getPainter(const std::string_view& id) const
{
    return to<SvgResourcePaintServerBox>(getResourceById(id));
}

SvgPaintServer SvgGraphicsElement::getPaintServer(const Paint& paint, float opacity) const
{
    return SvgPaintServer(getPainter(paint.uri()), paint.color(), opacity);
}

StrokeData SvgGraphicsElement::getStrokeData(const BoxStyle* style) const
{
    SvgLengthContext lengthContext(this);
    StrokeData strokeData(lengthContext.valueForLength(style->strokeWidth()));
    strokeData.setMiterLimit(style->strokeMiterlimit());
    strokeData.setLineCap(style->strokeLinecap());
    strokeData.setLineJoin(style->strokeLinejoin());
    strokeData.setDashOffset(lengthContext.valueForLength(style->strokeDashoffset()));

    DashArray dashArray;
    for(const auto& dash : style->strokeDasharray())
        dashArray.push_back(lengthContext.valueForLength(dash));
    strokeData.setDashArray(std::move(dashArray));
    return strokeData;
}

SvgFitToViewBox::SvgFitToViewBox(SvgElement* element)
{
    element->addProperty(viewBoxAttr, m_viewBox);
    element->addProperty(preserveAspectRatioAttr, m_preserveAspectRatio);
}

Transform SvgFitToViewBox::viewBoxToViewTransform(const Size& viewportSize) const
{
    const auto& viewBoxRect = m_viewBox.value();
    if(viewBoxRect.isEmpty() || viewportSize.isEmpty())
        return Transform::Identity;
    return m_preserveAspectRatio.getTransform(viewBoxRect, viewportSize);
}

Rect SvgFitToViewBox::getClipRect(const Size& viewportSize) const
{
    const auto& viewBoxRect = m_viewBox.value();
    if(viewBoxRect.isEmpty() || viewportSize.isEmpty())
        return Rect(0, 0, viewportSize.w, viewportSize.h);
    return m_preserveAspectRatio.getClipRect(viewBoxRect, viewportSize);
}

SvgURIReference::SvgURIReference(SvgElement* element)
{
    element->addProperty(hrefAttr, m_href);
}

SvgElement* SvgURIReference::getTargetElement(const Document* document) const
{
    std::string_view value(m_href.value());
    if(value.empty() || value.front() != '#')
        return nullptr;
    return to<SvgElement>(document->getElementById(value.substr(1)));
}

SvgSvgElement::SvgSvgElement(Document* document)
    : SvgGraphicsElement(document, svgTag)
    , SvgFitToViewBox(this)
    , m_x(0.f, SvgLengthType::Number, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_y(0.f, SvgLengthType::Number, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_width(100.f, SvgLengthType::Percentage, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Forbid)
    , m_height(100.f, SvgLengthType::Percentage, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Forbid)
{
    addProperty(xAttr, m_x);
    addProperty(yAttr, m_y);
    addProperty(widthAttr, m_width);
    addProperty(heightAttr, m_height);
}

void SvgSvgElement::computeIntrinsicDimensions(float& intrinsicWidth, float& intrinsicHeight, double& intrinsicRatio)
{
    SvgLengthContext lengthContext(this);
    if(m_width.type() != SvgLengthType::Percentage) {
        intrinsicWidth = lengthContext.valueForLength(m_width);
    } else {
        intrinsicWidth = 0.f;
    }

    if(m_height.type() != SvgLengthType::Percentage) {
        intrinsicHeight = lengthContext.valueForLength(m_height);
    } else {
        intrinsicHeight = 0.f;
    }

    const auto& viewBoxRect = viewBox();
    if(intrinsicWidth > 0.f && intrinsicHeight > 0.f) {
        intrinsicRatio = intrinsicWidth / intrinsicHeight;
    } else if(!viewBoxRect.isEmpty()) {
        intrinsicRatio = viewBoxRect.w / viewBoxRect.h;
    } else {
        intrinsicRatio = 0.0;
    }
}

static void addSvgTransformAttributeStyle(std::string& output, const Transform& matrix)
{
    output += "transform:matrix(";
    output += toString(matrix.a);
    output += ',';
    output += toString(matrix.b);
    output += ',';
    output += toString(matrix.c);
    output += ',';
    output += toString(matrix.d);
    output += ',';
    output += toString(matrix.e);
    output += ',';
    output += toString(matrix.f);
    output += ");";
}

void SvgSvgElement::collectAttributeStyle(std::string& output, GlobalString name, const HeapString& value) const
{
    if(name == transformAttr && isSvgRootNode()) {
        addSvgTransformAttributeStyle(output, transform());
    } else if(isSvgRootNode() && (name == widthAttr || name == heightAttr)) {
        addSvgAttributeStyle(output, name, value);
    } else {
        SvgElement::collectAttributeStyle(output, name, value);
    }
}

Box* SvgSvgElement::createBox(const RefPtr<BoxStyle>& style)
{
    if(isSvgRootNode())
        return new SvgRootBox(this, style);
    return new SvgViewportContainerBox(this, style);
}

SvgUseElement::SvgUseElement(Document* document)
    : SvgGraphicsElement(document, useTag)
    , SvgURIReference(this)
    , m_x(0.f, SvgLengthType::Number, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_y(0.f, SvgLengthType::Number, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_width(100.f, SvgLengthType::Percentage, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Forbid)
    , m_height(100.f, SvgLengthType::Percentage, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Forbid)
{
    addProperty(xAttr, m_x);
    addProperty(yAttr, m_y);
    addProperty(widthAttr, m_width);
    addProperty(heightAttr, m_height);
}

Box* SvgUseElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgTransformableContainerBox(this, style);
}

void SvgUseElement::finishParsingDocument()
{
    if(auto targetElement = getTargetElement(document())) {
        if(auto newElement = cloneTargetElement(targetElement)) {
            appendChild(newElement);
        }
    }

    SvgElement::finishParsingDocument();
}

static bool isDisallowedElement(const SvgElement* element)
{
    static const boost::unordered_flat_set<GlobalString> allowedElementTags = {
        aTag,
        circleTag,
        descTag,
        ellipseTag,
        gTag,
        imageTag,
        lineTag,
        metadataTag,
        pathTag,
        polygonTag,
        polylineTag,
        rectTag,
        svgTag,
        switchTag,
        symbolTag,
        textTag,
        textPathTag,
        titleTag,
        tspanTag,
        useTag
    };

    return !allowedElementTags.contains(element->tagName());
}

Element* SvgUseElement::cloneTargetElement(SvgElement* targetElement)
{
    if(targetElement == this || isDisallowedElement(targetElement))
        return nullptr;
    auto parent = parentNode();
    const auto& id = targetElement->id();
    while(parent && is<SvgElement>(*parent)) {
        const auto& element = to<SvgElement>(*parent);
        if(!id.empty() && id == element.id())
            return nullptr;
        parent = parent->parentNode();
    }

    auto tagName = targetElement->tagName();
    if(tagName == symbolTag) {
        tagName = svgTag;
    }

    auto newElement = document()->createElement(svgNs, tagName);
    newElement->setAttributes(targetElement->attributes());
    if(newElement->tagName() == svgTag) {
        for(const auto& attribute : attributes()) {
            if(attribute.name() == widthAttr || attribute.name() == heightAttr) {
                newElement->setAttribute(attribute);
            }
        }
    }

    if(newElement->tagName() != useTag)
        targetElement->cloneChildren(newElement);
    return newElement;
}

SvgImageElement::SvgImageElement(Document* document)
    : SvgGraphicsElement(document, imageTag)
    , SvgURIReference(this)
    , m_x(0.f, SvgLengthType::Number, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_y(0.f, SvgLengthType::Number, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_width(100.f, SvgLengthType::Percentage, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Forbid)
    , m_height(100.f, SvgLengthType::Percentage, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Forbid)
{
    addProperty(xAttr, m_x);
    addProperty(yAttr, m_y);
    addProperty(widthAttr, m_width);
    addProperty(heightAttr, m_height);
    addProperty(preserveAspectRatioAttr, m_preserveAspectRatio);
}

Box* SvgImageElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgImageBox(this, style);
}

RefPtr<Image> SvgImageElement::image() const
{
    auto url = document()->completeUrl(href());
    if(auto resource = document()->fetchImageResource(url))
        return resource->image();
    return nullptr;
}

SvgSymbolElement::SvgSymbolElement(Document* document)
    : SvgGraphicsElement(document, symbolTag)
    , SvgFitToViewBox(this)
{
}

Box* SvgSymbolElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgHiddenContainerBox(this, style);
}

SvgAElement::SvgAElement(Document* document)
    : SvgGraphicsElement(document, aTag)
    , SvgURIReference(this)
{
}

Box* SvgAElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgTransformableContainerBox(this, style);
}

SvgGElement::SvgGElement(Document* document)
    : SvgGraphicsElement(document, gTag)
{
}

Box* SvgGElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgTransformableContainerBox(this, style);
}

SvgDefsElement::SvgDefsElement(Document* document)
    : SvgGraphicsElement(document, defsTag)
{
}

Box* SvgDefsElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgHiddenContainerBox(this, style);
}

SvgGeometryElement::SvgGeometryElement(Document* document, GlobalString tagName)
    : SvgGraphicsElement(document, tagName)
{
}

SvgResourceMarkerBox* SvgGeometryElement::getMarker(const std::string_view& id) const
{
    return to<SvgResourceMarkerBox>(getResourceById(id));
}

SvgPathElement::SvgPathElement(Document* document)
    : SvgGeometryElement(document, pathTag)
{
    addProperty(dAttr, m_d);
}

Box* SvgPathElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgPathBox(this, style);
}

SvgShapeElement::SvgShapeElement(Document* document, GlobalString tagName)
    : SvgGeometryElement(document, tagName)
{
}

Box* SvgShapeElement::createBox(const RefPtr<BoxStyle> &style)
{
    return new SvgShapeBox(this, style);
}

SvgLineElement::SvgLineElement(Document* document)
    : SvgShapeElement(document, lineTag)
    , m_x1(SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_y1(SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_x2(SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_y2(SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
{
    addProperty(x1Attr, m_x1);
    addProperty(y1Attr, m_y1);
    addProperty(x2Attr, m_x2);
    addProperty(y2Attr, m_y2);
}

Rect SvgLineElement::getPath(Path& path) const
{
    SvgLengthContext lengthContext(this);
    auto x1 = lengthContext.valueForLength(m_x1);
    auto y1 = lengthContext.valueForLength(m_y1);
    auto x2 = lengthContext.valueForLength(m_x2);
    auto y2 = lengthContext.valueForLength(m_y2);

    path.moveTo(x1, y1);
    path.lineTo(x2, y2);
    return Rect(x1, y1, x2 - x1, y2 - y1);
}

SvgRectElement::SvgRectElement(Document* document)
    : SvgShapeElement(document, rectTag)
    , m_x(SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_y(SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_width(SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Forbid)
    , m_height(SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Forbid)
    , m_rx(SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Forbid)
    , m_ry(SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Forbid)
{
    addProperty(xAttr, m_x);
    addProperty(yAttr, m_y);
    addProperty(widthAttr, m_width);
    addProperty(heightAttr, m_height);
    addProperty(rxAttr, m_rx);
    addProperty(ryAttr, m_ry);
}

Rect SvgRectElement::getPath(Path& path) const
{
    SvgLengthContext lengthContext(this);
    auto width = lengthContext.valueForLength(m_width);
    auto height = lengthContext.valueForLength(m_height);
    if(width <= 0.f || height <= 0.f) {
        return Rect::Empty;
    }

    auto x = lengthContext.valueForLength(m_x);
    auto y = lengthContext.valueForLength(m_y);

    auto rx = lengthContext.valueForLength(m_rx);
    auto ry = lengthContext.valueForLength(m_ry);

    if(rx <= 0.f) rx = ry;
    if(ry <= 0.f) ry = rx;

    rx = std::min(rx, width / 2.f);
    ry = std::min(ry, height / 2.f);

    path.addRoundedRect(Rect(x, y, width, height), RectRadii(rx, ry));
    return Rect(x, y, width, height);
}

SvgCircleElement::SvgCircleElement(Document* document)
    : SvgShapeElement(document, circleTag)
    , m_cx(SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_cy(SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_r(SvgLengthDirection::Diagonal, SvgLengthNegativeValuesMode::Forbid)
{
    addProperty(cxAttr, m_cx);
    addProperty(cyAttr, m_cy);
    addProperty(rAttr, m_r);
}

Rect SvgCircleElement::getPath(Path& path) const
{
    SvgLengthContext lengthContext(this);
    auto r = lengthContext.valueForLength(m_r);
    if(r <= 0.f) {
        return Rect::Empty;
    }

    auto cx = lengthContext.valueForLength(m_cx);
    auto cy = lengthContext.valueForLength(m_cy);
    path.addEllipse(cx, cy, r, r);
    return Rect(cx - r, cy - r, r + r, r + r);
}

SvgEllipseElement::SvgEllipseElement(Document* document)
    : SvgShapeElement(document, ellipseTag)
    , m_cx(SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_cy(SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_rx(SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Forbid)
    , m_ry(SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Forbid)
{
    addProperty(cxAttr, m_cx);
    addProperty(cyAttr, m_cy);
    addProperty(rxAttr, m_rx);
    addProperty(ryAttr, m_ry);
}

Rect SvgEllipseElement::getPath(Path& path) const
{
    SvgLengthContext lengthContext(this);
    auto rx = lengthContext.valueForLength(m_rx);
    auto ry = lengthContext.valueForLength(m_ry);
    if(rx <= 0.f || ry <= 0.f) {
        return Rect::Empty;
    }

    auto cx = lengthContext.valueForLength(m_cx);
    auto cy = lengthContext.valueForLength(m_cy);
    path.addEllipse(cx, cy, rx, ry);
    return Rect(cx - rx, cy - ry, rx + rx, ry + ry);
}

SvgPolyElement::SvgPolyElement(Document* document, GlobalString tagName)
    : SvgShapeElement(document, tagName)
{
    addProperty(pointsAttr, m_points);
}

Rect SvgPolyElement::getPath(Path& path) const
{
    const auto& points = m_points.values();
    if(points.empty()) {
        return Rect::Empty;
    }

    path.moveTo(points[0].x, points[0].y);
    for(size_t i = 1; i < points.size(); i++) {
        path.lineTo(points[i].x, points[i].y);
    }

    if(tagName() == polygonTag)
        path.close();
    return path.boundingRect();
}

SvgTextPositioningElement::SvgTextPositioningElement(Document* document, GlobalString tagName)
    : SvgGraphicsElement(document, tagName)
    , m_x(SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_y(SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_dx(SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_dy(SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
{
    addProperty(xAttr, m_x);
    addProperty(yAttr, m_y);
    addProperty(dxAttr, m_dx);
    addProperty(dyAttr, m_dy);
    addProperty(rotateAttr, m_rotate);
}

SvgTSpanElement::SvgTSpanElement(Document* document)
    : SvgTextPositioningElement(document, tspanTag)
{
}

Box* SvgTSpanElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgTSpanBox(this, style);
}

SvgTextElement::SvgTextElement(Document* document)
    : SvgTextPositioningElement(document, textTag)
{
}

Box* SvgTextElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgTextBox(this, style);
}

SvgMarkerElement::SvgMarkerElement(Document* document)
    : SvgElement(document, markerTag)
    , SvgFitToViewBox(this)
    , m_refX(0.f, SvgLengthType::Number, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_refY(0.f, SvgLengthType::Number, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_markerWidth(3.f, SvgLengthType::Number, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Forbid)
    , m_markerHeight(3.f, SvgLengthType::Number, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Forbid)
    , m_markerUnits(SvgMarkerUnitsTypeStrokeWidth)
{
    addProperty(refXAttr, m_refX);
    addProperty(refYAttr, m_refY);
    addProperty(markerWidthAttr, m_markerWidth);
    addProperty(markerHeightAttr, m_markerHeight);
    addProperty(markerUnitsAttr, m_markerUnits);
    addProperty(orientAttr, m_orient);
}

Box* SvgMarkerElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgResourceMarkerBox(this, style);
}

SvgClipPathElement::SvgClipPathElement(Document* document)
    : SvgGraphicsElement(document, clipPathTag)
    , m_clipPathUnits(SvgUnitsTypeUserSpaceOnUse)
{
    addProperty(clipPathUnitsAttr, m_clipPathUnits);
}

Box* SvgClipPathElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgResourceClipperBox(this, style);
}

SvgMaskElement::SvgMaskElement(Document* document)
    : SvgElement(document, maskTag)
    , m_x(-10.f, SvgLengthType::Percentage, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_y(-10.f, SvgLengthType::Percentage, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_width(120.f, SvgLengthType::Percentage, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Forbid)
    , m_height(120.f, SvgLengthType::Percentage, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Forbid)
    , m_maskUnits(SvgUnitsTypeObjectBoundingBox)
    , m_maskContentUnits(SvgUnitsTypeUserSpaceOnUse)
{
    addProperty(xAttr, m_x);
    addProperty(yAttr, m_y);
    addProperty(widthAttr, m_width);
    addProperty(heightAttr, m_height);
    addProperty(maskUnitsAttr, m_maskUnits);
    addProperty(maskContentUnitsAttr, m_maskContentUnits);
}

Box* SvgMaskElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgResourceMaskerBox(this, style);
}

SvgPatternElement::SvgPatternElement(Document* document)
    : SvgElement(document, patternTag)
    , SvgURIReference(this)
    , SvgFitToViewBox(this)
    , m_x(SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_y(SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_width(SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Forbid)
    , m_height(SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Forbid)
    , m_patternUnits(SvgUnitsTypeObjectBoundingBox)
    , m_patternContentUnits(SvgUnitsTypeUserSpaceOnUse)
{
    addProperty(xAttr, m_x);
    addProperty(yAttr, m_y);
    addProperty(widthAttr, m_width);
    addProperty(heightAttr, m_height);
    addProperty(patternTransformAttr, m_patternTransform);
    addProperty(patternUnitsAttr, m_patternUnits);
    addProperty(patternContentUnitsAttr, m_patternContentUnits);
}

SvgPatternAttributes SvgPatternElement::collectPatternAttributes() const
{
    SvgPatternAttributes attributes;
    boost::unordered_flat_set<const SvgPatternElement*> processedPatterns;
    const SvgPatternElement* current = this;
    while(true) {
        if(!attributes.hasX() && current->hasAttribute(xAttr))
            attributes.setX(current);
        if(!attributes.hasY() && current->hasAttribute(yAttr))
            attributes.setY(current);
        if(!attributes.hasWidth() && current->hasAttribute(widthAttr))
            attributes.setWidth(current);
        if(!attributes.hasHeight() && current->hasAttribute(heightAttr))
            attributes.setHeight(current);
        if(!attributes.hasPatternTransform() && current->hasAttribute(patternTransformAttr))
            attributes.setPatternTransform(current);
        if(!attributes.hasPatternUnits() && current->hasAttribute(patternUnitsAttr))
            attributes.setPatternUnits(current);
        if(!attributes.hasPatternContentUnits() && current->hasAttribute(patternContentUnitsAttr))
            attributes.setPatternContentUnits(current);
        if(!attributes.hasViewBox() && current->hasAttribute(viewBoxAttr))
            attributes.setViewBox(current);
        if(!attributes.hasPreserveAspectRatio() && current->hasAttribute(preserveAspectRatioAttr))
            attributes.setPreserveAspectRatio(current);
        if(!attributes.hasPatternContentElement() && current->box()) {
            for(auto child = current->firstChild(); child; child = child->nextSibling()) {
                if(is<SvgElement>(*child)) {
                    attributes.setPatternContentElement(current);
                    break;
                }
            }
        }

        auto targetElement = current->getTargetElement(document());
        if(!targetElement || targetElement->tagName() != patternTag)
            break;
        processedPatterns.insert(current);
        current = static_cast<const SvgPatternElement*>(targetElement);
        if(processedPatterns.contains(current)) {
            break;
        }
    }

    attributes.setDefaultValues(this);
    return attributes;
}

Box* SvgPatternElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgResourcePatternBox(this, style);
}

SvgStopElement::SvgStopElement(Document* document)
    : SvgElement(document, stopTag)
{
    addProperty(offsetAttr, m_offset);
}

Color SvgStopElement::stopColorIncludingOpacity() const
{
    if(const auto* stopStyle = style())
        return stopStyle->stopColor().colorWithAlpha(stopStyle->stopOpacity());
    return Color::Transparent;
}

Box* SvgStopElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgGradientStopBox(this, style);
}

SvgGradientElement::SvgGradientElement(Document* document, GlobalString tagName)
    : SvgElement(document, tagName)
    , SvgURIReference(this)
    , m_gradientUnits(SvgUnitsTypeObjectBoundingBox)
    , m_spreadMethod(SvgSpreadMethodTypePad)
{
    addProperty(gradientTransformAttr, m_gradientTransform);
    addProperty(gradientUnitsAttr, m_gradientUnits);
    addProperty(spreadMethodAttr, m_spreadMethod);
}

void SvgGradientElement::collectGradientAttributes(SvgGradientAttributes& attributes) const
{
    if(!attributes.hasGradientTransform() && hasAttribute(gradientTransformAttr))
        attributes.setGradientTransform(this);
    if(!attributes.hasSpreadMethod() && hasAttribute(spreadMethodAttr))
        attributes.setSpreadMethod(this);
    if(!attributes.hasGradientUnits() && hasAttribute(gradientUnitsAttr))
        attributes.setGradientUnits(this);
    if(!attributes.hasGradientContentElement()) {
        for(auto child = firstChild(); child; child = child->nextSibling()) {
            if(child->isOfType(svgNs, stopTag)) {
                attributes.setGradientContentElement(this);
                break;
            }
        }
    }
}

SvgLinearGradientElement::SvgLinearGradientElement(Document* document)
    : SvgGradientElement(document, linearGradientTag)
    , m_x1(0.f, SvgLengthType::Percentage, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_y1(0.f, SvgLengthType::Percentage, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_x2(100.f, SvgLengthType::Percentage, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_y2(0.f, SvgLengthType::Percentage, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
{
    addProperty(x1Attr, m_x1);
    addProperty(y1Attr, m_y1);
    addProperty(x2Attr, m_x2);
    addProperty(y2Attr, m_y2);
}

SvgLinearGradientAttributes SvgLinearGradientElement::collectGradientAttributes() const
{
    SvgLinearGradientAttributes attributes;
    boost::unordered_flat_set<const SvgGradientElement*> processedGradients;
    const SvgGradientElement* current = this;
    while(true) {
        current->collectGradientAttributes(attributes);
        if(current->tagName() == linearGradientTag) {
            auto element = static_cast<const SvgLinearGradientElement*>(current);
            if(!attributes.hasX1() && element->hasAttribute(x1Attr))
                attributes.setX1(element);
            if(!attributes.hasY1() && element->hasAttribute(y1Attr))
                attributes.setY1(element);
            if(!attributes.hasX2() && element->hasAttribute(x2Attr))
                attributes.setX2(element);
            if(!attributes.hasY2() && element->hasAttribute(y2Attr)) {
                attributes.setY2(element);
            }
        }

        auto targetElement = current->getTargetElement(document());
        if(!targetElement || !(targetElement->tagName() == linearGradientTag || targetElement->tagName() == radialGradientTag))
            break;
        processedGradients.insert(current);
        current = static_cast<const SvgGradientElement*>(targetElement);
        if(processedGradients.contains(current)) {
            break;
        }
    }

    attributes.setDefaultValues(this);
    return attributes;
}

Box* SvgLinearGradientElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgResourceLinearGradientBox(this, style);
}

SvgRadialGradientElement::SvgRadialGradientElement(Document* document)
    : SvgGradientElement(document, radialGradientTag)
    , m_cx(50.f, SvgLengthType::Percentage, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_cy(50.f, SvgLengthType::Percentage, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_r(50.f, SvgLengthType::Percentage, SvgLengthDirection::Diagonal, SvgLengthNegativeValuesMode::Forbid)
    , m_fx(0.f, SvgLengthType::Number, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_fy(0.f, SvgLengthType::Number, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
{
    addProperty(cxAttr, m_cx);
    addProperty(cyAttr, m_cy);
    addProperty(rAttr, m_r);
    addProperty(fxAttr, m_fx);
    addProperty(fyAttr, m_fy);
}

SvgRadialGradientAttributes SvgRadialGradientElement::collectGradientAttributes() const
{
    SvgRadialGradientAttributes attributes;
    boost::unordered_flat_set<const SvgGradientElement*> processedGradients;
    const SvgGradientElement* current = this;
    while(true) {
        current->collectGradientAttributes(attributes);
        if(current->tagName() == radialGradientTag) {
            auto element = static_cast<const SvgRadialGradientElement*>(current);
            if(!attributes.hasCx() && element->hasAttribute(cxAttr))
                attributes.setCx(element);
            if(!attributes.hasCy() && element->hasAttribute(cyAttr))
                attributes.setCy(element);
            if(!attributes.hasR() && element->hasAttribute(rAttr))
                attributes.setR(element);
            if(!attributes.hasFx() && element->hasAttribute(fxAttr))
                attributes.setFx(element);
            if(!attributes.hasFy() && element->hasAttribute(fyAttr)) {
                attributes.setFy(element);
            }
        }

        auto targetElement = current->getTargetElement(document());
        if(!targetElement || !(targetElement->tagName() == linearGradientTag || targetElement->tagName() == radialGradientTag))
            break;
        processedGradients.insert(current);
        current = static_cast<const SvgGradientElement*>(targetElement);
        if(processedGradients.contains(current)) {
            break;
        }
    }

    attributes.setDefaultValues(this);
    return attributes;
}

Box* SvgRadialGradientElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgResourceRadialGradientBox(this, style);
}

SvgStyleElement::SvgStyleElement(Document* document)
    : SvgElement(document, styleTag)
{
}

const HeapString& SvgStyleElement::type() const
{
    return getAttribute(typeAttr);
}

const HeapString& SvgStyleElement::media() const
{
    return getAttribute(mediaAttr);
}

void SvgStyleElement::finishParsingDocument()
{
    if(document()->supportsMedia(type(), media()))
        document()->addAuthorStyleSheet(textFromChildren(), document()->baseUrl());
    SvgElement::finishParsingDocument();
}

std::unique_ptr<SvgDocument> SvgDocument::create(Book* book, ResourceFetcher* fetcher, Url baseUrl)
{
    return std::unique_ptr<SvgDocument>(new SvgDocument(book, fetcher, std::move(baseUrl)));
}

SvgDocument::SvgDocument(Book* book, ResourceFetcher* fetcher, Url baseUrl)
    : XmlDocument(classKind, book, fetcher, std::move(baseUrl))
{
}

} // namespace plutobook
