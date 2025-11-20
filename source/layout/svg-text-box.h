#ifndef PLUTOBOOK_SvgTEXTBOX_H
#define PLUTOBOOK_SvgTEXTBOX_H

#include "svg-box-model.h"
#include "svg-document.h"
#include "svg-line-layout.h"

namespace plutobook {

class SvgInlineTextBox final : public Box {
public:
    SvgInlineTextBox(TextNode* node, const RefPtr<BoxStyle>& style);

    bool isSvgInlineTextBox() const final { return true; }
    const HeapString& text() const { return node()->data(); }
    TextNode* node() const;

    const char* name() const final { return "SvgInlineTextBox"; }
};

template<>
struct is_a<SvgInlineTextBox> {
    static bool check(const Box& box) { return box.isSvgInlineTextBox(); }
};

inline TextNode* SvgInlineTextBox::node() const
{
    return static_cast<TextNode*>(Box::node());
}

class SvgTSpanBox final : public Box {
public:
    SvgTSpanBox(SvgTSpanElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgTSpanBox() const final { return true; }
    SvgTSpanElement* element() const;
    const SvgPaintServer& fill() const { return m_fill; }
    void build() final;

    const char* name() const final { return "SvgTSpanBox"; }

private:
    SvgPaintServer m_fill;
};

template<>
struct is_a<SvgTSpanBox> {
    static bool check(const Box& box) { return box.isSvgTSpanBox(); }
};

inline SvgTSpanElement* SvgTSpanBox::element() const
{
    return static_cast<SvgTSpanElement*>(node());
}

class SvgTextBox final : public SvgBoxModel {
public:
    SvgTextBox(SvgTextElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgTextBox() const final { return true; }

    SvgTextElement* element() const;
    Transform localTransform() const final { return element()->transform(); }
    Rect fillBoundingBox() const final;
    Rect strokeBoundingBox() const final { return fillBoundingBox(); }
    void render(const SvgRenderState& state) const final;
    void layout() final;
    void build() final;

    const char* name() const final { return "SvgTextBox"; }

private:
    SvgPaintServer m_fill;
    SvgLineLayout m_lineLayout;
    mutable Rect m_fillBoundingBox = Rect::Invalid;
};

template<>
struct is_a<SvgTextBox> {
    static bool check(const Box& box) { return box.isSvgTextBox(); }
};

inline SvgTextElement* SvgTextBox::element() const
{
    return static_cast<SvgTextElement*>(node());
}

} // namespace plutobook

#endif // PLUTOBOOK_SvgTEXTBOX_H
