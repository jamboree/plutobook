#include "text-box.h"
#include "line-box.h"
#include "global-string.h"

namespace plutobook {

TextBox::TextBox(ClassKind type, Node* node, const RefPtr<BoxStyle>& style)
    : Box(type, node, style)
{
    setIsInline(true);
}

void TextBox::appendText(const std::string_view& text)
{
    m_text = concatenateString(m_text, text);
}

TextBox::~TextBox() = default;

LineBreakBox::LineBreakBox(Node* node, const RefPtr<BoxStyle>& style)
    : TextBox(classKind, node, style)
{
    setText(newLineGlo);
}

WordBreakBox::WordBreakBox(Node* node, const RefPtr<BoxStyle>& style)
    : TextBox(classKind, node, style)
{
}

} // namespace plutobook
