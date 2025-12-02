#include "svg-document.h"
#include "svg-replaced-box.h"
#include "svg-resource-box.h"
#include "svg-geometry-box.h"
#include "svg-text-box.h"
#include "css-rule.h"
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

void SvgElement::collectAttributeStyle(AttributeStyle& style) const
{
    Element::collectAttributeStyle(style);
    for (const auto& attr : attributes()) {
        const auto id = csspropertyid(attr.name());
        switch (id) {
        case CssPropertyID::AlignmentBaseline:
        case CssPropertyID::BaselineShift:
        case CssPropertyID::Clip:
        case CssPropertyID::ClipPath:
        case CssPropertyID::ClipRule:
        case CssPropertyID::Color:
        case CssPropertyID::Direction:
        case CssPropertyID::Display:
        case CssPropertyID::DominantBaseline:
        case CssPropertyID::Fill:
        case CssPropertyID::FillOpacity:
        case CssPropertyID::FillRule:
        case CssPropertyID::FontFamily:
        case CssPropertyID::FontSize:
        case CssPropertyID::FontStretch:
        case CssPropertyID::FontStyle:
        case CssPropertyID::FontVariant:
        case CssPropertyID::FontWeight:
        case CssPropertyID::LetterSpacing:
        case CssPropertyID::MarkerEnd:
        case CssPropertyID::MarkerMid:
        case CssPropertyID::MarkerStart:
        case CssPropertyID::Mask:
        case CssPropertyID::MaskType:
        case CssPropertyID::Opacity:
        case CssPropertyID::Overflow:
        case CssPropertyID::PaintOrder:
        case CssPropertyID::StopColor:
        case CssPropertyID::StopOpacity:
        case CssPropertyID::Stroke:
        case CssPropertyID::StrokeDasharray:
        case CssPropertyID::StrokeDashoffset:
        case CssPropertyID::StrokeLinecap:
        case CssPropertyID::StrokeLinejoin:
        case CssPropertyID::StrokeMiterlimit:
        case CssPropertyID::StrokeOpacity:
        case CssPropertyID::StrokeWidth:
        case CssPropertyID::TextAnchor:
        case CssPropertyID::TextDecoration:
        case CssPropertyID::TextOrientation:
        case CssPropertyID::TransformOrigin:
        case CssPropertyID::UnicodeBidi:
        case CssPropertyID::VectorEffect:
        case CssPropertyID::Visibility:
        case CssPropertyID::WordSpacing:
        case CssPropertyID::WritingMode:
            style.addProperty(id, attr.value());
            break;
        }
    }
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
{}

SvgProperty* SvgGraphicsElement::getProperty(GlobalString name)
{
    if(name == transformAttr)
        return &m_transform;
    return SvgElement::getProperty(name);
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

SvgElement* SvgURIReference::getTargetElement(const Document* document) const
{
    std::string_view value(m_href.value());
    if(value.empty() || value.front() != '#')
        return nullptr;
    return to<SvgElement>(document->getElementById(value.substr(1)));
}

SvgSvgElement::SvgSvgElement(Document* document)
    : SvgGraphicsElement(document, svgTag)
    , m_x(0.f, SvgLengthType::Number, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_y(0.f, SvgLengthType::Number, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_width(100.f, SvgLengthType::Percentage, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Forbid)
    , m_height(100.f, SvgLengthType::Percentage, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Forbid)
{}

SvgProperty* SvgSvgElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
    case xAttr: return &m_x;
    case yAttr: return &m_y;
    case widthAttr: return &m_width;
    case heightAttr: return &m_height;
        // SvgFitToViewBox
    case viewBoxAttr: return &m_viewBox;
    case preserveAspectRatioAttr: return &m_preserveAspectRatio;
    default: return SvgGraphicsElement::getProperty(name);
    }
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

void SvgSvgElement::collectAttributeStyle(AttributeStyle& style) const
{
    SvgElement::collectAttributeStyle(style);
    if (isSvgRootNode()) {
        if (hasAttribute(transformAttr)) {
            const auto matrix = transform();
            style.addProperty(CssPropertyID::Transform,
                CssFunctionValue::create(CssFunctionID::Matrix, {
                    CssNumberValue::create(matrix.a),
                    CssNumberValue::create(matrix.b),
                    CssNumberValue::create(matrix.c),
                    CssNumberValue::create(matrix.d),
                    CssNumberValue::create(matrix.e),
                    CssNumberValue::create(matrix.f),
                    }));
        }
        if (auto attr = findAttribute(widthAttr)) {
            style.addProperty(CssPropertyID::Width, attr->value());
        }
        if (auto attr = findAttribute(heightAttr)) {
            style.addProperty(CssPropertyID::Height, attr->value());
        }
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
    , m_x(0.f, SvgLengthType::Number, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_y(0.f, SvgLengthType::Number, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_width(100.f, SvgLengthType::Percentage, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Forbid)
    , m_height(100.f, SvgLengthType::Percentage, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Forbid)
{}

SvgProperty* SvgUseElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
    case xAttr: return &m_x;
    case yAttr: return &m_y;
    case widthAttr: return &m_width;
    case heightAttr: return &m_height;
        // SvgURIReference
    case hrefAttr: return &m_href;
    default: return SvgGraphicsElement::getProperty(name);
    }
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
    , m_x(0.f, SvgLengthType::Number, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_y(0.f, SvgLengthType::Number, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_width(100.f, SvgLengthType::Percentage, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Forbid)
    , m_height(100.f, SvgLengthType::Percentage, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Forbid)
{}

SvgProperty* SvgImageElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
    case xAttr: return &m_x;
    case yAttr: return &m_y;
    case widthAttr: return &m_width;
    case heightAttr: return &m_height;
    case preserveAspectRatioAttr: return &m_preserveAspectRatio;
        // SvgURIReference
    case hrefAttr: return &m_href;
    default: return SvgGraphicsElement::getProperty(name);
    }
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
{
}

SvgProperty* SvgSymbolElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
        // SvgFitToViewBox
    case viewBoxAttr: return &m_viewBox;
    case preserveAspectRatioAttr: return &m_preserveAspectRatio;
    default: return SvgElement::getProperty(name);
    }
}

Box* SvgSymbolElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgHiddenContainerBox(this, style);
}

SvgAElement::SvgAElement(Document* document)
    : SvgGraphicsElement(document, aTag)
{
}

SvgProperty* SvgAElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
        // SvgURIReference
    case hrefAttr: return &m_href;
    default: return SvgElement::getProperty(name);
    }
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
{}

SvgProperty* SvgPathElement::getProperty(GlobalString name)
{
    if(name == dAttr)
        return &m_d;
    return SvgGeometryElement::getProperty(name);
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
{}

SvgProperty* SvgLineElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
    case x1Attr: return &m_x1;
    case y1Attr: return &m_y1;
    case x2Attr: return &m_x2;
    case y2Attr: return &m_y2;
    default: return SvgShapeElement::getProperty(name);
    }
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
{}

SvgProperty* SvgRectElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
    case xAttr: return &m_x;
    case yAttr: return &m_y;
    case widthAttr: return &m_width;
    case heightAttr: return &m_height;
    case rxAttr: return &m_rx;
    case ryAttr: return &m_ry;
    default: return SvgShapeElement::getProperty(name);
    }
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
{}

SvgProperty* SvgCircleElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
    case cxAttr: return &m_cx;
    case cyAttr: return &m_cy;
    case rAttr: return &m_r;
    default: return SvgShapeElement::getProperty(name);
    }
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
{}

SvgProperty* SvgEllipseElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
    case cxAttr: return &m_cx;
    case cyAttr: return &m_cy;
    case rxAttr: return &m_rx;
    case ryAttr: return &m_ry;
    default: return SvgShapeElement::getProperty(name);
    }
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
{}

SvgProperty* SvgPolyElement::getProperty(GlobalString name)
{
    if (name == pointsAttr)
        return &m_points;
    return SvgShapeElement::getProperty(name);
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
{}

SvgProperty* SvgTextPositioningElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
    case xAttr: return &m_x;
    case yAttr: return &m_y;
    case dxAttr: return &m_dx;
    case dyAttr: return &m_dy;
    case rotateAttr: return &m_rotate;
    default: return SvgGraphicsElement::getProperty(name);
    }
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
    , m_refX(0.f, SvgLengthType::Number, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_refY(0.f, SvgLengthType::Number, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_markerWidth(3.f, SvgLengthType::Number, SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Forbid)
    , m_markerHeight(3.f, SvgLengthType::Number, SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Forbid)
    , m_markerUnits(SvgMarkerUnitsTypeStrokeWidth)
{}

SvgProperty* SvgMarkerElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
    case refXAttr: return &m_refX;
    case refYAttr: return &m_refY;
    case markerWidthAttr: return &m_markerWidth;
    case markerHeightAttr: return &m_markerHeight;
    case markerUnitsAttr: return &m_markerUnits;
    case orientAttr: return &m_orient;
        // SvgFitToViewBox
    case viewBoxAttr: return &m_viewBox;
    case preserveAspectRatioAttr: return &m_preserveAspectRatio;
    default: return SvgElement::getProperty(name);
    }
}

Box* SvgMarkerElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgResourceMarkerBox(this, style);
}

SvgClipPathElement::SvgClipPathElement(Document* document)
    : SvgGraphicsElement(document, clipPathTag)
    , m_clipPathUnits(SvgUnitsTypeUserSpaceOnUse)
{}

SvgProperty* SvgClipPathElement::getProperty(GlobalString name)
{
    if(name == clipPathUnitsAttr)
        return &m_clipPathUnits;
    return SvgGraphicsElement::getProperty(name);
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
{}

SvgProperty* SvgMaskElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
    case xAttr: return &m_x;
    case yAttr: return &m_y;
    case widthAttr: return &m_width;
    case heightAttr: return &m_height;
    case maskUnitsAttr: return &m_maskUnits;
    case maskContentUnitsAttr: return &m_maskContentUnits;
    default: return SvgElement::getProperty(name);
    }
}

Box* SvgMaskElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SvgResourceMaskerBox(this, style);
}

SvgPatternElement::SvgPatternElement(Document* document)
    : SvgElement(document, patternTag)
    , m_x(SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Allow)
    , m_y(SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Allow)
    , m_width(SvgLengthDirection::Horizontal, SvgLengthNegativeValuesMode::Forbid)
    , m_height(SvgLengthDirection::Vertical, SvgLengthNegativeValuesMode::Forbid)
    , m_patternUnits(SvgUnitsTypeObjectBoundingBox)
    , m_patternContentUnits(SvgUnitsTypeUserSpaceOnUse)
{}

SvgProperty* SvgPatternElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
    case xAttr: return &m_x;
    case yAttr: return &m_y;
    case widthAttr: return &m_width;
    case heightAttr: return &m_height;
    case patternTransformAttr: return &m_patternTransform;
    case patternUnitsAttr: return &m_patternUnits;
    case patternContentUnitsAttr: return &m_patternContentUnits;
        // SvgURIReference
    case hrefAttr: return &m_href;
        // SvgFitToViewBox
    case viewBoxAttr: return &m_viewBox;
    case preserveAspectRatioAttr: return &m_preserveAspectRatio;
    default: return SvgElement::getProperty(name);
    }
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
{}

SvgProperty* SvgStopElement::getProperty(GlobalString name)
{
    if(name == offsetAttr)
        return &m_offset;
    return SvgElement::getProperty(name);
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
    , m_gradientUnits(SvgUnitsTypeObjectBoundingBox)
    , m_spreadMethod(SvgSpreadMethodTypePad)
{}

SvgProperty* SvgGradientElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
    case gradientTransformAttr: return &m_gradientTransform;
    case gradientUnitsAttr: return &m_gradientUnits;
    case spreadMethodAttr: return &m_spreadMethod;
        // SvgURIReference
    case hrefAttr: return &m_href;
    default: return SvgElement::getProperty(name);
    }
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
{}

SvgProperty* SvgLinearGradientElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
    case x1Attr: return &m_x1;
    case y1Attr: return &m_y1;
    case x2Attr: return &m_x2;
    case y2Attr: return &m_y2;
    default: return SvgGradientElement::getProperty(name);
    }
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
{}

SvgProperty* SvgRadialGradientElement::getProperty(GlobalString name)
{
    switch (name.asId()) {
    case cxAttr: return &m_cx;
    case cyAttr: return &m_cy;
    case rAttr: return &m_r;
    case fxAttr: return &m_fx;
    case fyAttr: return &m_fy;
    default: return SvgGradientElement::getProperty(name);
    }
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
