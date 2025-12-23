#include "svg-resource-box.h"
#include "svg-geometry-box.h"

#include <cairo/cairo.h>

namespace plutobook {

SvgResourceMarkerBox::SvgResourceMarkerBox(SvgMarkerElement* element, const RefPtr<BoxStyle>& style)
    : SvgResourceContainerBox(classKind, element, style)
{
    setIsOverflowHidden(style->isOverflowHidden());
}

Point SvgResourceMarkerBox::refPoint() const
{
    SvgLengthContext lengthContext(element());
    const Point refPoint = {
        lengthContext.valueForLength(element()->refX()),
        lengthContext.valueForLength(element()->refY())
    };

    return refPoint;
}

Size SvgResourceMarkerBox::markerSize() const
{
    SvgLengthContext lengthContext(element());
    const Size markerSize = {
        lengthContext.valueForLength(element()->markerWidth()),
        lengthContext.valueForLength(element()->markerHeight())
    };

    return markerSize;
}

Transform SvgResourceMarkerBox::markerTransform(const Point& origin, float angle, float strokeWidth) const
{
    const auto& orient = element()->orient();
    auto transform = Transform::makeTranslate(origin.x, origin.y);
    if(orient.orientType() == SvgAngle::OrientType::Angle) {
        transform.rotate(orient.value());
    } else {
        transform.rotate(angle);
    }

    auto reference = m_localTransform.mapPoint(refPoint());
    if(element()->markerUnits() == SvgMarkerUnitsTypeStrokeWidth)
        transform.scale(strokeWidth, strokeWidth);
    transform.translate(-reference.x, -reference.y);
    return transform * m_localTransform;
}

Rect SvgResourceMarkerBox::markerBoundingBox(const Point& origin, float angle, float strokeWidth) const
{
    return markerTransform(origin, angle, strokeWidth).mapRect(paintBoundingBox());
}

void SvgResourceMarkerBox::renderMarker(const SvgRenderState& state, const Point& origin, float angle, float strokeWidth) const
{
    if(state.hasCycleReference(this))
        return;
    SvgBlendInfo blendInfo(m_clipper, m_masker, style());
    SvgRenderState newState(blendInfo, this, state, markerTransform(origin, angle, strokeWidth));
    if(isOverflowHidden())
        newState.context().clipRect(element()->getClipRect(markerSize()));
    renderChildren(newState);
}

void SvgResourceMarkerBox::layout()
{
    m_localTransform = element()->viewBoxToViewTransform(markerSize());
    SvgResourceContainerBox::layout();
}

SvgResourceClipperBox::SvgResourceClipperBox(SvgClipPathElement* element, const RefPtr<BoxStyle>& style)
    : SvgResourceContainerBox(classKind, element, style)
{
}

bool SvgResourceClipperBox::requiresMasking() const
{
    if(m_clipper != nullptr)
        return true;
    const SvgGeometryBox* prevClipShape = nullptr;
    for(auto child = firstChild(); child; child = child->nextSibling()) {
        if(child->style()->visibility() != Visibility::Visible)
            continue;
        const SvgGeometryBox* clipShape = nullptr;
        if(auto container = to<SvgTransformableContainerBox>(child)) {
            if(container->element()->tagName() != useTag)
                continue;
            if(container->clipper())
                return true;
            clipShape = to<SvgGeometryBox>(container->firstChild());
        } else {
            if(child->isSvgTextBox())
                return true;
            clipShape = to<SvgGeometryBox>(child);
        }

        if(clipShape == nullptr)
            continue;
        if(prevClipShape || clipShape->clipper())
            return true;
        prevClipShape = clipShape;
    }

    return false;
}

Rect SvgResourceClipperBox::clipBoundingBox(const Box* box) const
{
    auto clipBoundingBox = paintBoundingBox();
    if(element()->clipPathUnits() == SvgUnitsTypeObjectBoundingBox) {
        const auto& bbox = box->fillBoundingBox();
        clipBoundingBox.x = clipBoundingBox.x * bbox.w + bbox.x;
        clipBoundingBox.y = clipBoundingBox.y * bbox.h + bbox.y;
        clipBoundingBox.w = clipBoundingBox.w * bbox.w;
        clipBoundingBox.h = clipBoundingBox.h * bbox.h;
    }

    return element()->transform().mapRect(clipBoundingBox);
}

void SvgResourceClipperBox::applyClipPath(const SvgRenderState& state) const
{
    auto transform = element()->transform();
    if(element()->clipPathUnits() == SvgUnitsTypeObjectBoundingBox) {
        const auto& bbox = state.fillBoundingBox();
        transform.translate(bbox.x, bbox.y);
        transform.scale(bbox.w, bbox.h);
    }

    for(auto child = firstChild(); child; child = child->nextSibling()) {
        if(child->style()->visibility() != Visibility::Visible)
            continue;
        Transform clipTransform(transform);
        const SvgGeometryBox* clipShape = nullptr;
        if(auto container = to<SvgTransformableContainerBox>(child)) {
            if(container->element()->tagName() != useTag)
                continue;
            clipTransform.multiply(container->localTransform());
            clipShape = to<SvgGeometryBox>(container->firstChild());
        } else {
            clipShape = to<SvgGeometryBox>(child);
        }

        if(clipShape == nullptr)
            continue;
        auto path = clipShape->path().transformed(clipTransform * clipShape->localTransform());
        state.context().clipPath(path, clipShape->style()->clipRule());
        return;
    }

    state.context().clipRect(Rect(0, 0, 0, 0));
}

void SvgResourceClipperBox::applyClipMask(const SvgRenderState& state) const
{
    if(state.hasCycleReference(this))
        return;
    auto maskImage = ImageBuffer::create(state.currentTransform().mapRect(state.paintBoundingBox()));
    CairoGraphicsContext context(maskImage->canvas());
    context.addTransform(state.currentTransform());
    context.addTransform(element()->transform());
    if(element()->clipPathUnits() == SvgUnitsTypeObjectBoundingBox) {
        const auto& bbox = state.fillBoundingBox();
        context.translate(bbox.x, bbox.y);
        context.scale(bbox.w, bbox.h);
    }
    {
        SvgBlendInfo blendInfo(m_clipper, nullptr, 1.f, BlendMode::Normal);
        SvgRenderState newState(blendInfo, this, state, SvgRenderMode::Clipping, context);
        renderChildren(newState);
    }

    state.context().applyMask(*maskImage);
}

SvgResourceMaskerBox::SvgResourceMaskerBox(SvgMaskElement* element, const RefPtr<BoxStyle>& style)
    : SvgResourceContainerBox(classKind, element, style)
{
}

Rect SvgResourceMaskerBox::maskBoundingBox(const Box* box) const
{
    auto maskBoundingBox = paintBoundingBox();
    if(element()->maskContentUnits() == SvgUnitsTypeObjectBoundingBox) {
        const auto& bbox = box->fillBoundingBox();
        maskBoundingBox.x = maskBoundingBox.x * bbox.w + bbox.x;
        maskBoundingBox.y = maskBoundingBox.y * bbox.h + bbox.y;
        maskBoundingBox.w = maskBoundingBox.w * bbox.w;
        maskBoundingBox.h = maskBoundingBox.h * bbox.h;
    }

    SvgLengthContext lengthContext(element(), element()->maskUnits());
    Rect maskRect = {
        lengthContext.valueForLength(element()->x()),
        lengthContext.valueForLength(element()->y()),
        lengthContext.valueForLength(element()->width()),
        lengthContext.valueForLength(element()->height())
    };

    if(element()->maskUnits() == SvgUnitsTypeObjectBoundingBox) {
        const auto& bbox = box->fillBoundingBox();
        maskRect.x = maskRect.x * bbox.w + bbox.x;
        maskRect.y = maskRect.y * bbox.h + bbox.y;
        maskRect.w = maskRect.w * bbox.w;
        maskRect.h = maskRect.h * bbox.h;
    }

    return maskBoundingBox.intersected(maskRect);
}

void SvgResourceMaskerBox::applyMask(const SvgRenderState& state) const
{
    if(state.hasCycleReference(this))
        return;
    SvgLengthContext lengthContext(element(), element()->maskUnits());
    Rect maskRect = {
        lengthContext.valueForLength(element()->x()),
        lengthContext.valueForLength(element()->y()),
        lengthContext.valueForLength(element()->width()),
        lengthContext.valueForLength(element()->height())
    };

    if(element()->maskUnits() == SvgUnitsTypeObjectBoundingBox) {
        const auto& bbox = state.fillBoundingBox();
        maskRect.x = maskRect.x * bbox.w + bbox.x;
        maskRect.y = maskRect.y * bbox.h + bbox.y;
        maskRect.w = maskRect.w * bbox.w;
        maskRect.h = maskRect.h * bbox.h;
    }

    auto maskImage = ImageBuffer::create(state.currentTransform().mapRect(state.paintBoundingBox()));
    CairoGraphicsContext context(maskImage->canvas());
    context.addTransform(state.currentTransform());
    context.clipRect(maskRect);
    if(element()->maskContentUnits() == SvgUnitsTypeObjectBoundingBox) {
        const auto& bbox = state.fillBoundingBox();
        context.translate(bbox.x, bbox.y);
        context.scale(bbox.w, bbox.h);
    }
    {
        SvgBlendInfo blendInfo(m_clipper, m_masker, 1.f, BlendMode::Normal);
        SvgRenderState newState(blendInfo, this, state, state.mode(), context);
        renderChildren(newState);
    }

    if(style()->maskType() == MaskType::Luminance)
        maskImage->convertToLuminanceMask();
    state.context().applyMask(*maskImage);
}

SvgResourcePaintServerBox::SvgResourcePaintServerBox(ClassKind type, SvgElement* element, const RefPtr<BoxStyle>& style)
    : SvgResourceContainerBox(type, element, style)
{
}

SvgResourcePatternBox::SvgResourcePatternBox(SvgPatternElement* element, const RefPtr<BoxStyle>& style)
    : SvgResourcePaintServerBox(classKind, element, style)
{
}

void SvgResourcePatternBox::build()
{
    m_attributes = element()->collectPatternAttributes();
    SvgResourcePaintServerBox::build();
}

void SvgResourcePatternBox::applyPaint(const SvgRenderState& state, float opacity) const
{
    if(state.hasCycleReference(this))
        return;
    auto patternContentBox = to<SvgResourcePatternBox>(m_attributes.patternContentElement()->box());
    if(patternContentBox == nullptr)
        return;
    SvgLengthContext lengthContext(element(), m_attributes.patternUnits());
    Rect patternRect = {
        lengthContext.valueForLength(m_attributes.x()),
        lengthContext.valueForLength(m_attributes.y()),
        lengthContext.valueForLength(m_attributes.width()),
        lengthContext.valueForLength(m_attributes.height())
    };

    if(m_attributes.patternUnits() == SvgUnitsTypeObjectBoundingBox) {
        const auto& bbox = state.fillBoundingBox();
        patternRect.x = patternRect.x * bbox.w + bbox.x;
        patternRect.y = patternRect.y * bbox.h + bbox.y;
        patternRect.w = patternRect.w * bbox.w;
        patternRect.h = patternRect.h * bbox.h;
    }

    cairo_rectangle_t rectangle = {0, 0, patternRect.w, patternRect.h};
    auto surface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, &rectangle);
    auto canvas = cairo_create(surface);

    CairoGraphicsContext context(canvas);
    if(m_attributes.viewBox().isValid()) {
        context.addTransform(m_attributes.preserveAspectRatio().getTransform(m_attributes.viewBox(), patternRect.size()));
    } else if(m_attributes.patternContentUnits() == SvgUnitsTypeObjectBoundingBox) {
        const auto& bbox = state.fillBoundingBox();
        context.scale(bbox.w, bbox.h);
    }
    {
        SvgBlendInfo blendInfo(m_clipper, m_masker, opacity, BlendMode::Normal);
        SvgRenderState newState(blendInfo, this, state, SvgRenderMode::Painting, context);
        patternContentBox->renderChildren(newState);
    }

    Transform patternTransform(m_attributes.patternTransform());
    patternTransform.translate(patternRect.x, patternRect.y);
    state.context().setPattern(surface, patternTransform);

    cairo_destroy(canvas);
    cairo_surface_destroy(surface);
}

SvgGradientStopBox::SvgGradientStopBox(SvgStopElement* element, const RefPtr<BoxStyle>& style)
    : Box(classKind, element, style)
{
}

SvgResourceGradientBox::SvgResourceGradientBox(ClassKind type, SvgGradientElement* element, const RefPtr<BoxStyle>& style)
    : SvgResourcePaintServerBox(type, element, style)
{
}

SvgResourceLinearGradientBox::SvgResourceLinearGradientBox(SvgLinearGradientElement* element, const RefPtr<BoxStyle>& style)
    : SvgResourceGradientBox(classKind, element, style)
{
}

static GradientStops buildGradientStops(const SvgGradientElement* element)
{
    GradientStops gradientStops;
    float previousOffset = 0.f;
    for(auto child = element->firstChild(); child; child = child->nextSibling()) {
        if(child->isOfType(svgNs, stopTag)) {
            auto stopElement = static_cast<const SvgStopElement*>(child);
            auto offset = std::max(previousOffset, stopElement->offset());
            gradientStops.emplace_back(offset, stopElement->stopColorIncludingOpacity());
            previousOffset = offset;
        }
    }

    return gradientStops;
}

constexpr SpreadMethod toSpreadMethod(SvgSpreadMethodType spreadMethodType)
{
    switch(spreadMethodType) {
    case SvgSpreadMethodTypePad:
        return SpreadMethod::Pad;
    case SvgSpreadMethodTypeReflect:
        return SpreadMethod::Reflect;
    case SvgSpreadMethodTypeRepeat:
        return SpreadMethod::Repeat;
    default:
        assert(false);
    }

    return SpreadMethod::Pad;
}

void SvgResourceLinearGradientBox::build()
{
    m_attributes = element()->collectGradientAttributes();
    SvgResourceGradientBox::build();
}

void SvgResourceLinearGradientBox::applyPaint(const SvgRenderState& state, float opacity) const
{
    GradientInfo info;
    info.stops = buildGradientStops(m_attributes.gradientContentElement());
    if(info.stops.empty()) {
        state.context().setColor(Color::Transparent);
        return;
    }

    SvgLengthContext lengthContext(element(), m_attributes.gradientUnits());
    LinearGradientValues values = {
        lengthContext.valueForLength(m_attributes.x1()),
        lengthContext.valueForLength(m_attributes.y1()),
        lengthContext.valueForLength(m_attributes.x2()),
        lengthContext.valueForLength(m_attributes.y2())
    };

    if((info.stops.size() == 1 || (values.x1 == values.x2 && values.y1 == values.y2))) {
        const auto& lastStop = info.stops.back();
        state.context().setColor(lastStop.second.colorWithAlpha(opacity));
        return;
    }

    info.method = toSpreadMethod(m_attributes.spreadMethod());
    info.transform = m_attributes.gradientTransform();
    Rect bbox;
    if(m_attributes.gradientUnits() == SvgUnitsTypeObjectBoundingBox) {
        bbox = state.fillBoundingBox();
        info.objectBoundingBox = &bbox;
    }
    info.opacity = opacity;

    state.context().setLinearGradient(values, info);
}

SvgResourceRadialGradientBox::SvgResourceRadialGradientBox(SvgRadialGradientElement* element, const RefPtr<BoxStyle>& style)
    : SvgResourceGradientBox(classKind, element, style)
{
}

void SvgResourceRadialGradientBox::build()
{
    m_attributes = element()->collectGradientAttributes();
    SvgResourceGradientBox::build();
}

void SvgResourceRadialGradientBox::applyPaint(const SvgRenderState& state, float opacity) const
{
    GradientInfo info;
    info.stops = buildGradientStops(m_attributes.gradientContentElement());
    if(info.stops.empty()) {
        state.context().setColor(Color::Transparent);
        return;
    }

    SvgLengthContext lengthContext(element(), m_attributes.gradientUnits());
    RadialGradientValues values = {
        lengthContext.valueForLength(m_attributes.fx()),
        lengthContext.valueForLength(m_attributes.fy()),
        lengthContext.valueForLength(m_attributes.cx()),
        lengthContext.valueForLength(m_attributes.cy()),
        lengthContext.valueForLength(m_attributes.r())
    };

    if(values.r == 0.f || info.stops.size() == 1) {
        const auto& lastStop = info.stops.back();
        state.context().setColor(lastStop.second.colorWithAlpha(opacity));
        return;
    }

    info.method = toSpreadMethod(m_attributes.spreadMethod());
    info.transform = m_attributes.gradientTransform();
    Rect bbox;
    if (m_attributes.gradientUnits() == SvgUnitsTypeObjectBoundingBox) {
        bbox = state.fillBoundingBox();
        info.objectBoundingBox = &bbox;
    }
    info.opacity = opacity;

    state.context().setRadialGradient(values, info);
}

} // namespace plutobook
