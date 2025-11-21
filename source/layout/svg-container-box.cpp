#include "svg-container-box.h"

namespace plutobook {

SvgContainerBox::SvgContainerBox(ClassKind type, SvgElement* element, const RefPtr<BoxStyle>& style)
    : SvgBoxModel(type, element, style)
{
}

Rect SvgContainerBox::fillBoundingBox() const
{
    if(m_fillBoundingBox.isValid())
        return m_fillBoundingBox;
    for(auto child = firstChild(); child; child = child->nextSibling()) {
        const auto& transform = child->localTransform();
        if(child->isSvgHiddenContainerBox())
            continue;
        m_fillBoundingBox.unite(transform.mapRect(child->fillBoundingBox()));
    }

    if(!m_fillBoundingBox.isValid())
        m_fillBoundingBox = Rect::Empty;
    return m_fillBoundingBox;
}

Rect SvgContainerBox::strokeBoundingBox() const
{
    if(m_strokeBoundingBox.isValid())
        return m_strokeBoundingBox;
    for(auto child = firstChild(); child; child = child->nextSibling()) {
        const auto& transform = child->localTransform();
        if(child->isSvgHiddenContainerBox())
            continue;
        m_strokeBoundingBox.unite(transform.mapRect(child->strokeBoundingBox()));
    }

    if(!m_strokeBoundingBox.isValid())
        m_strokeBoundingBox = Rect::Empty;
    return m_strokeBoundingBox;
}

void SvgContainerBox::layout()
{
    SvgBoxModel::layout();

    m_fillBoundingBox = Rect::Invalid;
    m_strokeBoundingBox = Rect::Invalid;
    for(auto child = firstChild(); child; child = child->nextSibling()) {
        if(auto box = to<SvgBoxModel>(child)) {
            box->layout();
        }
    }
}

void SvgContainerBox::renderChildren(const SvgRenderState& state) const
{
    for(auto child = firstChild(); child; child = child->nextSibling()) {
        if(auto box = to<SvgBoxModel>(child)) {
            box->render(state);
        }
    }
}

SvgHiddenContainerBox::SvgHiddenContainerBox(ClassKind type, SvgElement* element, const RefPtr<BoxStyle>& style)
    : SvgContainerBox(type, element, style)
{
}

void SvgHiddenContainerBox::render(const SvgRenderState& state) const
{
}

SvgTransformableContainerBox::SvgTransformableContainerBox(SvgGraphicsElement* element, const RefPtr<BoxStyle>& style)
    : SvgContainerBox(classKind, element, style)
{
}

void SvgTransformableContainerBox::render(const SvgRenderState& state) const
{
    SvgBlendInfo blendInfo(m_clipper, m_masker, style());
    SvgRenderState newState(blendInfo, this, state, m_localTransform);
    renderChildren(newState);
}

inline const SvgUseElement* toSvgUseElement(const SvgElement* element)
{
    if(element->tagName() == useTag)
        return static_cast<const SvgUseElement*>(element);
    return nullptr;
}

void SvgTransformableContainerBox::layout()
{
    m_localTransform = element()->transform();
    if(auto useElement = toSvgUseElement(element())) {
        SvgLengthContext lengthContext(useElement);
        const Point translation = {
            lengthContext.valueForLength(useElement->x()),
            lengthContext.valueForLength(useElement->y())
        };

        m_localTransform.translate(translation.x, translation.y);
    }

    SvgContainerBox::layout();
}

SvgViewportContainerBox::SvgViewportContainerBox(SvgSvgElement* element, const RefPtr<BoxStyle>& style)
    : SvgContainerBox(classKind, element, style)
{
    setIsOverflowHidden(style->isOverflowHidden());
}

void SvgViewportContainerBox::render(const SvgRenderState& state) const
{
    SvgBlendInfo blendInfo(m_clipper, m_masker, style());
    SvgRenderState newState(blendInfo, this, state, m_localTransform);
    if(isOverflowHidden()) {
        SvgLengthContext lengthContext(element());
        const Size viewportSize = {
            lengthContext.valueForLength(element()->width()),
            lengthContext.valueForLength(element()->height())
        };

        newState->clipRect(element()->getClipRect(viewportSize));
    }

    renderChildren(newState);
}

void SvgViewportContainerBox::layout()
{
    SvgLengthContext lengthContext(element());
    const Rect viewportRect = {
        lengthContext.valueForLength(element()->x()),
        lengthContext.valueForLength(element()->y()),
        lengthContext.valueForLength(element()->width()),
        lengthContext.valueForLength(element()->height())
    };

    m_localTransform = element()->transform() * Transform::makeTranslate(viewportRect.x, viewportRect.y) * element()->viewBoxToViewTransform(viewportRect.size());
    SvgContainerBox::layout();
}

SvgResourceContainerBox::SvgResourceContainerBox(ClassKind type, SvgElement* element, const RefPtr<BoxStyle>& style)
    : SvgHiddenContainerBox(type, element, style)
{
}

} // namespace plutobook
