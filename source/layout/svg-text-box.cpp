#include "svg-text-box.h"

namespace plutobook {

SvgInlineTextBox::SvgInlineTextBox(TextNode* node, const RefPtr<BoxStyle>& style)
    : Box(classKind, node, style)
{
    setIsInline(true);
}

SvgTSpanBox::SvgTSpanBox(SvgTSpanElement* element, const RefPtr<BoxStyle>& style)
    : Box(classKind, element, style)
{
    setIsInline(true);
}

void SvgTSpanBox::build()
{
    m_fill = element()->getPaintServer(style()->fill(), style()->fillOpacity());
    Box::build();
}

SvgTextBox::SvgTextBox(SvgTextElement* element, const RefPtr<BoxStyle>& style)
    : SvgBoxModel(classKind, element, style)
    , m_lineLayout(this)
{
}

Rect SvgTextBox::fillBoundingBox() const
{
    if(!m_fillBoundingBox.isValid())
        m_fillBoundingBox = m_lineLayout.boundingRect();
    return m_fillBoundingBox;
}

void SvgTextBox::render(const SvgRenderState& state) const
{
    if(style()->visibility() != Visibility::Visible)
        return;
    SvgBlendInfo blendInfo(m_clipper, m_masker, style());
    SvgRenderState newState(blendInfo, this, state, element()->transform());
    if(newState.mode() == SvgRenderMode::Clipping) {
        newState->setColor(Color::White);
    } else {
        m_fill.applyPaint(newState);
    }

    m_lineLayout.render(newState);
}

void SvgTextBox::layout()
{
    m_fillBoundingBox = Rect::Invalid;
    m_lineLayout.layout();
    SvgBoxModel::layout();
}

void SvgTextBox::build()
{
    m_fill = element()->getPaintServer(style()->fill(), style()->fillOpacity());
    m_lineLayout.build();
    SvgBoxModel::build();
}

} // namespace plutobook
