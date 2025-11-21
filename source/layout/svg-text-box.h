#pragma once

#include "svg-box-model.h"
#include "svg-document.h"
#include "svg-line-layout.h"

namespace plutobook {
    class SvgInlineTextBox final : public Box {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgInlineText;

        SvgInlineTextBox(TextNode* node, const RefPtr<BoxStyle>& style);

        const HeapString& text() const { return node()->data(); }
        TextNode* node() const;

        const char* name() const final { return "SvgInlineTextBox"; }
    };

    extern template bool is<SvgInlineTextBox>(const Box& value);

    inline TextNode* SvgInlineTextBox::node() const {
        return static_cast<TextNode*>(Box::node());
    }

    class SvgTSpanBox final : public Box {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgTSpan;

        SvgTSpanBox(SvgTSpanElement* element, const RefPtr<BoxStyle>& style);

        SvgTSpanElement* element() const;
        const SvgPaintServer& fill() const { return m_fill; }
        void build() final;

        const char* name() const final { return "SvgTSpanBox"; }

    private:
        SvgPaintServer m_fill;
    };

    extern template bool is<SvgTSpanBox>(const Box& value);

    inline SvgTSpanElement* SvgTSpanBox::element() const {
        return static_cast<SvgTSpanElement*>(node());
    }

    class SvgTextBox final : public SvgBoxModel {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgText;

        SvgTextBox(SvgTextElement* element, const RefPtr<BoxStyle>& style);

        SvgTextElement* element() const;
        Transform localTransform() const final {
            return element()->transform();
        }
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

    extern template bool is<SvgTextBox>(const Box& value);

    inline SvgTextElement* SvgTextBox::element() const {
        return static_cast<SvgTextElement*>(node());
    }
} // namespace plutobook