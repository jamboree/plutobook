#include "svg-box-model.h"
#include "svg-resource-box.h"

namespace plutobook {

bool SvgBlendInfo::requiresCompositing(SvgRenderMode mode) const
{
    return (m_clipper && m_clipper->requiresMasking()) || (mode == SvgRenderMode::Painting && (m_masker || m_opacity < 1.f || m_blendMode > BlendMode::Normal));
}

SvgRenderState::SvgRenderState(const SvgBlendInfo& info, const Box* box, const SvgRenderState& parent, const Transform& localTransform)
    : SvgRenderState(info, box, &parent, parent.mode(), parent.context(), parent.currentTransform() * localTransform)
{
}

SvgRenderState::SvgRenderState(const SvgBlendInfo& info, const Box* box, const SvgRenderState& parent, SvgRenderMode mode, GraphicsContext& context)
    : SvgRenderState(info, box, &parent, mode, context, context.getTransform())
{
}

SvgRenderState::SvgRenderState(const SvgBlendInfo& info, const Box* box, const SvgRenderState* parent, SvgRenderMode mode, GraphicsContext& context, const Transform& currentTransform)
    : m_box(box), m_parent(parent), m_info(info), m_context(context), m_currentTransform(currentTransform)
    , m_mode(mode), m_requiresCompositing(info.requiresCompositing(mode))
{
    if(m_requiresCompositing) {
        m_context.pushGroup();
    } else {
        m_context.save();
    }

    m_context.setTransform(m_currentTransform);
    if(m_info.clipper() && !m_requiresCompositing) {
        m_info.clipper()->applyClipPath(*this);
    }
}

SvgRenderState::~SvgRenderState()
{
    const auto& element = to<SvgElement>(*m_box->node());
    if(m_parent && (element.isLinkSource() || element.isLinkDestination()))
        m_box->paintAnnotation(m_context, m_box->paintBoundingBox());
    if(m_requiresCompositing) {
        if(m_info.clipper())
            m_info.clipper()->applyClipMask(*this);
        if(m_mode == SvgRenderMode::Painting) {
            if(m_info.masker())
                m_info.masker()->applyMask(*this);
            m_context.popGroup(m_info.opacity(), m_info.blendMode());
        } else {
            m_context.popGroup(1.0);
        }
    } else {
        m_context.restore();
    }
}

bool SvgRenderState::hasCycleReference(const Box* box) const
{
    auto current = this;
    do {
        if(box == current->box())
            return true;
        current = current->parent();
    } while(current);
    return false;
}

void SvgPaintServer::applyPaint(const SvgRenderState& state) const
{
    if(m_painter) {
        m_painter->applyPaint(state, m_opacity);
    } else {
        state.context().setColor(m_color.colorWithAlpha(m_opacity));
    }
}

SvgBoxModel::SvgBoxModel(ClassKind type, SvgElement* element, const RefPtr<BoxStyle>& style)
    : Box(type, element, style)
{
    setIsInline(false);
}

void SvgBoxModel::layout()
{
    m_paintBoundingBox = Rect::Invalid;
}

void SvgBoxModel::build()
{
    m_clipper = element()->getClipper(style()->clipPath());
    m_masker = element()->getMasker(style()->mask());
    Box::build();
}

Rect SvgBoxModel::paintBoundingBox() const
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

} // namespace plutobook
