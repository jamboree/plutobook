#include "svg-replaced-box.h"
#include "svg-resource-box.h"
#include "image-resource.h"

namespace plutobook {

SvgRootBox::SvgRootBox(SvgSvgElement* element, const RefPtr<BoxStyle>& style)
    : ReplacedBox(classKind, element, style)
{
    setIntrinsicSize(Size(300, 150));
}

bool SvgRootBox::requiresLayer() const
{
    return isPositioned() || isRelativePositioned() || hasTransform() || style()->zIndex();
}

Rect SvgRootBox::fillBoundingBox() const
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

Rect SvgRootBox::strokeBoundingBox() const
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

Rect SvgRootBox::paintBoundingBox() const
{
    if(m_paintBoundingBox.isValid())
        return m_paintBoundingBox;
    m_paintBoundingBox = strokeBoundingBox();
    assert(m_paintBoundingBox.isValid());
    if(m_clipper)
        m_paintBoundingBox.intersect(m_clipper->clipBoundingBox(this));
    if(m_masker) {
        m_paintBoundingBox.intersect(m_masker->maskBoundingBox(this));
    }

    return m_paintBoundingBox;
}

float SvgRootBox::computePreferredReplacedWidth() const
{
    if(document()->isSvgImageDocument())
        return document()->containerWidth();
    return ReplacedBox::computePreferredReplacedWidth();
}

float SvgRootBox::computeReplacedWidth() const
{
    if(document()->isSvgImageDocument())
        return document()->containerWidth();
    return ReplacedBox::computeReplacedWidth();
}

float SvgRootBox::computeReplacedHeight() const
{
    if(document()->isSvgImageDocument())
        return document()->containerHeight();
    return ReplacedBox::computeReplacedHeight();
}

void SvgRootBox::computeIntrinsicRatioInformation(float& intrinsicWidth, float& intrinsicHeight, double& intrinsicRatio) const
{
    element()->computeIntrinsicDimensions(intrinsicWidth, intrinsicHeight, intrinsicRatio);
}

void SvgRootBox::paintReplaced(const PaintInfo& info, const Point& offset)
{
    const RectOutsets outsets = {
        borderTop() + paddingTop(),
        borderRight() + paddingRight(),
        borderBottom() + paddingBottom(),
        borderLeft() + paddingLeft()
    };

    Rect borderRect(offset, size());
    Rect contentRect(borderRect - outsets);
    if(contentRect.isEmpty()) {
        return;
    }

    if(isOverflowHidden()) {
        auto clipRect = style()->getBorderRoundedRect(borderRect, true, true);
        info->save();
        info->clipRoundedRect(clipRect - outsets);
    }

    auto currentTransform = info->getTransform();
    currentTransform.translate(contentRect.x, contentRect.y);
    currentTransform.multiply(element()->viewBoxToViewTransform(contentRect.size()));

    {
        SvgBlendInfo blendInfo(m_clipper, m_masker, style());
        SvgRenderState newState(blendInfo, this, nullptr, SvgRenderMode::Painting, *info, currentTransform);
        for(auto child = firstChild(); child; child = child->nextSibling()) {
            if(auto box = to<SvgBoxModel>(child)) {
                box->render(newState);
            }
        }
    }

    if(isOverflowHidden()) {
        info->restore();
    }
}

void SvgRootBox::layout(FragmentBuilder* fragmentainer)
{
    ReplacedBox::layout(fragmentainer);

    m_fillBoundingBox = Rect::Invalid;
    m_strokeBoundingBox = Rect::Invalid;
    m_paintBoundingBox = Rect::Invalid;
    for(auto child = firstChild(); child; child = child->nextSibling()) {
        if(auto box = to<SvgBoxModel>(child)) {
            box->layout();
        }
    }

    if(!isOverflowHidden()) {
        auto contentRect = contentBoxRect();
        auto localTransform = Transform::makeTranslate(contentRect.x, contentRect.y);
        localTransform.multiply(element()->viewBoxToViewTransform(contentRect.size()));
        addOverflowRect(localTransform.mapRect(paintBoundingBox()));
    }
}

void SvgRootBox::build()
{
    m_clipper = element()->getClipper(style()->clipPath());
    m_masker = element()->getMasker(style()->mask());
    ReplacedBox::build();
}

SvgImageBox::SvgImageBox(SvgImageElement* element, const RefPtr<BoxStyle>& style)
    : SvgBoxModel(classKind, element, style)
{
}

Rect SvgImageBox::fillBoundingBox() const
{
    if(m_fillBoundingBox.isValid())
        return m_fillBoundingBox;
    SvgLengthContext lengthContext(element());
    m_fillBoundingBox.x = lengthContext.valueForLength(element()->x());
    m_fillBoundingBox.y = lengthContext.valueForLength(element()->y());
    m_fillBoundingBox.w = lengthContext.valueForLength(element()->width());
    m_fillBoundingBox.h = lengthContext.valueForLength(element()->height());
    return m_fillBoundingBox;
}

Rect SvgImageBox::strokeBoundingBox() const
{
    return fillBoundingBox();
}

void SvgImageBox::render(const SvgRenderState& state) const
{
    if(m_image == nullptr || state.mode() != SvgRenderMode::Painting || style()->visibility() != Visibility::Visible)
        return;
    Rect dstRect(fillBoundingBox());
    m_image->setContainerSize(dstRect.size());

    Rect srcRect(m_image->size());
    element()->preserveAspectRatio().transformRect(dstRect, srcRect);

    SvgBlendInfo blendInfo(m_clipper, m_masker, style());
    SvgRenderState newState(blendInfo, this, state, element()->transform());
    m_image->draw(*newState, dstRect, srcRect);
}

void SvgImageBox::layout()
{
    m_fillBoundingBox = Rect::Invalid;
    SvgBoxModel::layout();
}

void SvgImageBox::build()
{
    m_image = element()->image();
    SvgBoxModel::build();
}

} // namespace plutobook
