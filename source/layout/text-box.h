#pragma once

#include "box.h"

namespace plutobook {
    class TextLineBox;

    using TextLineBoxList = std::pmr::vector<std::unique_ptr<TextLineBox>>;

    class TextBox : public Box {
    public:
        static constexpr ClassKind classKind = ClassKind::Text;

        TextBox(Node* node, const RefPtr<BoxStyle>& style)
            : TextBox(classKind, node, style) {}
        TextBox(ClassKind type, Node* node, const RefPtr<BoxStyle>& style);
        ~TextBox() override;

        const HeapString& text() const { return m_text; }
        void setText(const HeapString& text) { m_text = text; }
        void appendText(const std::string_view& text);

        const TextLineBoxList& lines() const { return m_lines; }
        TextLineBoxList& lines() { return m_lines; }

        const char* name() const override { return "TextBox"; }

    private:
        HeapString m_text;
        TextLineBoxList m_lines;
    };

    extern template bool is<TextBox>(const Box& value);

    class LineBreakBox final : public TextBox {
    public:
        static constexpr ClassKind classKind = ClassKind::LineBreak;

        LineBreakBox(Node* node, const RefPtr<BoxStyle>& style);

        const char* name() const final { return "LineBreakBox"; }
    };

    extern template bool is<LineBreakBox>(const Box& value);

    class WordBreakBox final : public TextBox {
    public:
        static constexpr ClassKind classKind = ClassKind::WordBreak;

        WordBreakBox(Node* node, const RefPtr<BoxStyle>& style);

        const char* name() const final { return "WordBreakBox"; }
    };

    extern template bool is<WordBreakBox>(const Box& value);
} // namespace plutobook