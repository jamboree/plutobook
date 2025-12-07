#include "html-document.h"
#include "html-parser.h"
#include "css-rule.h"
#include "counters.h"
#include "text-box.h"
#include "content-box.h"
#include "replaced-box.h"
#include "table-box.h"
#include "form-control-box.h"
#include "text-resource.h"
#include "image-resource.h"
#include "string-utils.h"
#include "ident-table.h"

#include "plutobook.hpp"

#include <charconv>
#include <unicode/uchar.h>
#include <unicode/uiter.h>

namespace plutobook {

HtmlElement::HtmlElement(Document* document, GlobalString tagName)
    : Element(classKind, document, xhtmlNs, tagName)
{
}

inline bool isFirstLetterPunctuation(UChar32 cc)
{
    switch(u_charType(cc)) {
    case U_START_PUNCTUATION:
    case U_END_PUNCTUATION:
    case U_INITIAL_PUNCTUATION:
    case U_FINAL_PUNCTUATION:
    case U_OTHER_PUNCTUATION:
        return true;
    default:
        return false;
    }
}

static size_t firstLetterTextLength(const HeapString& text)
{
    UCharIterator it;
    uiter_setUTF8(&it, text.data(), text.size());

    auto hasLetter = false;
    auto hasPunctuation = false;
    size_t textLength = 0;
    while(it.hasNext(&it)) {
        auto cc = uiter_next32(&it);
        if(!isSpace(cc)) {
            if(!isFirstLetterPunctuation(cc)) {
                if(hasLetter)
                    break;
                hasLetter = true;
            } else {
                hasPunctuation = true;
            }
        }

        textLength += U8_LENGTH(cc);
    }

    if(!hasLetter && !hasPunctuation)
        return 0;
    return textLength;
}

void HtmlElement::buildFirstLetterPseudoBox(Box* parent)
{
    if(!parent->isBlockFlowBox())
        return;
    auto style = document()->styleSheet().pseudoStyleForElement(this, PseudoType::FirstLetter, parent->style());
    if(style == nullptr || style->display() == Display::None)
        return;
    auto child = parent->firstChild();
    while(child) {
        if(child->style()->pseudoType() == PseudoType::FirstLetter || child->isReplaced()
            || child->isLineBreakBox() || child->isWordBreakBox()) {
            return;
        }

        if(auto textBox = to<TextBox>(child)) {
            const auto& text = textBox->text();
            if(auto length = firstLetterTextLength(text)) {
                auto newTextBox = new TextBox(nullptr, style);
                newTextBox->setText(text.substring(0, length));
                textBox->setText(text.substring(length));

                auto letterBox = Box::create(nullptr, style);
                letterBox->addChild(newTextBox);
                textBox->parentBox()->insertChild(letterBox, textBox);
                break;
            }
        }

        if(!child->isFloatingOrPositioned() && !child->isListMarkerBox()
            && !child->isTableBox() && !child->isFlexBox()) {
            if(child->firstChild()) {
                child = child->firstChild();
                continue;
            }
        }

        while(true) {
            if(child->nextSibling()) {
                child = child->nextSibling();
                break;
            }

            child = child->parentBox();
            if(child == parent) {
                return;
            }
        }
    }
}

void HtmlElement::buildPseudoBox(Counters& counters, Box* parent, PseudoType pseudoType)
{
    if(pseudoType == PseudoType::Marker && !parent->isListItemBox())
        return;
    auto style = document()->styleSheet().pseudoStyleForElement(this, pseudoType, parent->style());
    if(style == nullptr || style->display() == Display::None)
        return;
    auto box = Box::create(nullptr, style);
    parent->addChild(box);
    if(pseudoType == PseudoType::Before || pseudoType == PseudoType::After) {
        counters.update(box);
        buildPseudoBox(counters, box, PseudoType::Marker);
    }

    ContentBoxBuilder(counters, this, box).build();
}

void HtmlElement::buildElementBox(Counters& counters, Box* box)
{
    counters.update(box);
    counters.push();
    buildPseudoBox(counters, box, PseudoType::Marker);
    buildPseudoBox(counters, box, PseudoType::Before);
    buildChildrenBox(counters, box);
    buildPseudoBox(counters, box, PseudoType::After);
    buildFirstLetterPseudoBox(box);
    counters.pop();
}

void HtmlElement::buildBox(Counters& counters, Box* parent)
{
    auto style = document()->styleSheet().styleForElement(this, parent->style());
    if(style == nullptr || style->display() == Display::None)
        return;
    if(style->position() == Position::Running) {
        const auto value = style->get(CssPropertyID::Position);
        const auto& position = to<CssUnaryFunctionValue>(*value);
        assert(position.id() == CssFunctionID::Running);
        const auto& name = to<CssCustomIdentValue>(*position.value());
        document()->addRunningStyle(name.value(), std::move(style));
        return;
    }
    if (auto box = createBox(style)) {
        parent->addChild(box);
        buildElementBox(counters, box);
    }
}

void HtmlElement::collectAttributeStyle(AttributeStyle& style) const
{
    Element::collectAttributeStyle(style);
    if (hasAttribute(hiddenAttr)) {
        style.addProperty(CssPropertyID::Display, CssValueID::None);
    }
    if (auto attr = findAttribute(alignAttr)) {
        style.addProperty(CssPropertyID::TextAlign, attr->value());
    }
}

template<typename T>
static Optional<T> parseHtmlInteger(std::string_view input)
{
    stripLeadingAndTrailingSpaces(input);
    if (!input.empty()) {
        if (input.front() == '+') {
            input.remove_prefix(1);
            if (input.empty() || !isDigit(input.front()))
                return std::nullopt;
        }
        T output;
        const auto end = input.data() + input.size();
        const auto [p, ec] = std::from_chars(input.data(), end, output);
        if (ec == std::errc() && p == end) {
            return output;
        }
    }
    return std::nullopt;
}

template<typename T>
Optional<T> HtmlElement::parseIntegerAttribute(GlobalString name) const
{
    const auto& value = getAttribute(name);
    if(!value.empty())
        return parseHtmlInteger<T>(value);
    return std::nullopt;
}

static void addHtmlLengthAttributeStyle(AttributeStyle& style, CssPropertyID id, std::string_view input)
{
    stripLeadingAndTrailingSpaces(input);
    if(input.empty())
        return;
    float value;
    const auto end = input.data() + input.size();
    const auto [p, ec] = std::from_chars(input.data(), end, value);
    if (ec != std::errc())
        return;
    if (p == end) {
        style.addProperty(id, CssLengthValue::create(value));
    } else if (std::string_view(p, end) == "%") {
        style.addProperty(id, CssPercentValue::create(value));
    }
}

static void addHtmlUrlAttributeStyle(std::string& output, const std::string_view& name, const std::string_view& value)
{
    if(value.empty())
        return;
    output += name;
    output += ':';
    output += "url(";
    output += value;
    output += ");";
}

HtmlBodyElement::HtmlBodyElement(Document* document)
    : HtmlElement(document, bodyTag)
{
}

void HtmlBodyElement::collectAttributeStyle(AttributeStyle& style) const
{
    HtmlElement::collectAttributeStyle(style);
    if (auto attr = findAttribute(textAttr)) {
        style.addProperty(CssPropertyID::Color, attr->value());
    }
    if (auto attr = findAttribute(bgcolorAttr)) {
        style.addProperty(CssPropertyID::BackgroundColor, attr->value());
    }
    if (auto attr = findAttribute(backgroundAttr)) {
        style.addProperty(CssPropertyID::BackgroundImage, attr->value());
    }
}

HtmlFontElement::HtmlFontElement(Document* document)
    : HtmlElement(document, fontTag)
{
}

static Optional<CssValueID> parseHtmlFontSizeAttribute(std::string_view input)
{
    bool hasPlusSign = false;
    bool hasMinusSign = false;
    stripLeadingAndTrailingSpaces(input);
    if(!input.empty() && input.front() == '+') {
        input.remove_prefix(1);
        hasPlusSign = true;
    } else if(!input.empty() && input.front() == '-') {
        input.remove_prefix(1);
        hasMinusSign = true;
    }

    if(input.empty() || !isDigit(input.front()))
        return std::nullopt;
    int value = 0;
    do {
        value = value * 10 + input.front() - '0';
        input.remove_prefix(1);
    } while(!input.empty() && isDigit(input.front()));

    if(hasPlusSign)
        value += 3;
    else if(hasMinusSign) {
        value = 3 - value;
    }

    if(value > 7)
        value = 7;
    if(value < 1) {
        value = 1;
    }

    switch(value) {
    case 1:
        return CssValueID::XSmall;
    case 2:
        return CssValueID::Small;
    case 3:
        return CssValueID::Medium;
    case 4:
        return CssValueID::Large;
    case 5:
        return CssValueID::XLarge;
    case 6:
        return CssValueID::XxLarge;
    case 7:
        return CssValueID::XxxLarge;
    default:
        std::unreachable();
    }
}

void HtmlFontElement::collectAttributeStyle(AttributeStyle& style) const
{
    HtmlElement::collectAttributeStyle(style);
    if (auto attr = findAttribute(sizeAttr)) {
        if (auto fontSize = parseHtmlFontSizeAttribute(attr->value())) {
            style.addProperty(CssPropertyID::FontSize, *fontSize);
        }
    }
    if (auto attr = findAttribute(faceAttr)) {
        style.addProperty(CssPropertyID::FontFamily, attr->value());
    }
    if (auto attr = findAttribute(colorAttr)) {
        style.addProperty(CssPropertyID::Color, attr->value());
    }
}

HtmlImageElement::HtmlImageElement(Document* document)
    : HtmlElement(document, imgTag)
{
}

void HtmlImageElement::collectAttributeStyle(AttributeStyle& style) const
{
    HtmlElement::collectAttributeStyle(style);
    if (auto attr = findAttribute(widthAttr)) {
        addHtmlLengthAttributeStyle(style, CssPropertyID::Width, attr->value());
    }
    if (auto attr = findAttribute(heightAttr)) {
        addHtmlLengthAttributeStyle(style, CssPropertyID::Height, attr->value());
    }
    if (auto attr = findAttribute(hspaceAttr)) {
        addHtmlLengthAttributeStyle(style, CssPropertyID::MarginLeft, attr->value());
        addHtmlLengthAttributeStyle(style, CssPropertyID::MarginRight, attr->value());
    }
    if (auto attr = findAttribute(vspaceAttr)) {
        addHtmlLengthAttributeStyle(style, CssPropertyID::MarginTop, attr->value());
        addHtmlLengthAttributeStyle(style, CssPropertyID::MarginBottom, attr->value());
    }
    if (auto attr = findAttribute(borderAttr)) {
        addHtmlLengthAttributeStyle(style, CssPropertyID::BorderWidth, attr->value());
        style.addProperty(CssPropertyID::BorderStyle, CssValueID::Solid);
    }
    if (auto attr = findAttribute(valignAttr)) {
        style.addProperty(CssPropertyID::VerticalAlign, attr->value());
    }
}

const HeapString& HtmlImageElement::altText() const
{
    return getAttribute(altAttr);
}

RefPtr<Image> HtmlImageElement::srcImage() const
{
    auto url = getUrlAttribute(srcAttr);
    if(auto resource = document()->fetchImageResource(url))
        return resource->image();
    return nullptr;
}

Box* HtmlImageElement::createBox(const RefPtr<BoxStyle>& style)
{
    auto image = srcImage();
    if(image == nullptr) {
        const auto& text = altText();
        if (text.empty())
            return new ImageBox(this, style);
        auto container = Box::create(this, style);
        auto box = new TextBox(nullptr, style);
        box->setText(text);
        container->addChild(box);
        return container;
    }

    auto box = new ImageBox(this, style);
    box->setImage(std::move(image));
    return box;
}

HtmlHrElement::HtmlHrElement(Document* document)
    : HtmlElement(document, hrTag)
{
}

void HtmlHrElement::collectAttributeStyle(AttributeStyle& style) const
{
    HtmlElement::collectAttributeStyle(style);
    if (auto attr = findAttribute(widthAttr)) {
        addHtmlLengthAttributeStyle(style, CssPropertyID::Width, attr->value());
    }
    if (auto attr = findAttribute(sizeAttr)) {
        const auto size = parseHtmlInteger<int>(attr->value()).value_or(0);
        if(size > 1) {
            style.addProperty(CssPropertyID::Height, CssLengthValue::create(size - 2));
        } else {
            style.addProperty(CssPropertyID::BorderBottomWidth, CssLengthValue::create(0));
        }
    }
    if (auto attr = findAttribute(alignAttr)) {
        if (attr->value() == "left") {
            style.addProperty(CssPropertyID::MarginLeft, CssLengthValue::create(0));
            style.addProperty(CssPropertyID::MarginRight, CssValueID::Auto);
        } else if (attr->value() == "right") {
            style.addProperty(CssPropertyID::MarginLeft, CssValueID::Auto);
            style.addProperty(CssPropertyID::MarginRight, CssLengthValue::create(0));
        } else {
            style.addProperty(CssPropertyID::MarginLeft, CssValueID::Auto);
            style.addProperty(CssPropertyID::MarginRight, CssValueID::Auto);
        }
    }
    if (auto attr = findAttribute(colorAttr)) {
        style.addProperty(CssPropertyID::BorderStyle, CssValueID::Solid);
        style.addProperty(CssPropertyID::BorderColor, attr->value());
        style.addProperty(CssPropertyID::BackgroundColor, attr->value());
    }
    if (hasAttribute(noshadeAttr)) {
        style.addProperty(CssPropertyID::BorderStyle, CssValueID::Solid);
        const auto darkGray = CssColorValue::create(*Color::named("darkgray"));
        style.addProperty(CssPropertyID::BorderColor, darkGray);
        style.addProperty(CssPropertyID::BackgroundColor, darkGray);
    }
}

HtmlBrElement::HtmlBrElement(Document* document)
    : HtmlElement(document, brTag)
{
}

Box* HtmlBrElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new LineBreakBox(this, style);
}

HtmlWbrElement::HtmlWbrElement(Document* document)
    : HtmlElement(document, wbrTag)
{
}

Box* HtmlWbrElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new WordBreakBox(this, style);
}

HtmlLiElement::HtmlLiElement(Document* document)
    : HtmlElement(document, liTag)
{
}

Optional<int> HtmlLiElement::value() const
{
    return parseIntegerAttribute<int>(valueAttr);
}

static std::string_view listTypeAttributeToStyleName(const std::string_view& value)
{
    if(value == "a")
        return "lower-alpha";
    if(value == "A")
        return "upper-alpha";
    if(value == "i")
        return "lower-roman";
    if(value == "I")
        return "upper-roman";
    if(value == "1") {
        return "decimal";
    }

    return value;
}

void HtmlLiElement::collectAttributeStyle(AttributeStyle& style) const
{
    HtmlElement::collectAttributeStyle(style);
    if (auto attr = findAttribute(typeAttr)) {
        style.addProperty(CssPropertyID::ListStyleType, listTypeAttributeToStyleName(attr->value()));
    }
}

HtmlOlElement::HtmlOlElement(Document* document)
    : HtmlElement(document, olTag)
{
}

int HtmlOlElement::start() const
{
    return parseIntegerAttribute<int>(startAttr).value_or(1);
}

void HtmlOlElement::collectAttributeStyle(AttributeStyle& style) const
{
    HtmlElement::collectAttributeStyle(style);
    if (auto attr = findAttribute(typeAttr)) {
        style.addProperty(CssPropertyID::ListStyleType, listTypeAttributeToStyleName(attr->value()));
    }
}

HtmlTableElement::HtmlTableElement(Document* document)
    : HtmlElement(document, tableTag)
{
}

void HtmlTableElement::parseAttribute(GlobalString name, const HeapString& value)
{
    switch (name.asId()) {
    case cellpaddingAttr:
        m_padding = parseHtmlInteger<unsigned>(value).value_or(0);
        break;
    case borderAttr:
        m_border = parseHtmlInteger<unsigned>(value).value_or(1);
        break;
    case rulesAttr:
        m_rules = parseRulesAttribute(value);
        break;
    case frameAttr:
        m_frame = parseFrameAttribute(value);
        break;
    default:
        HtmlElement::parseAttribute(name, value);
        break;
    }
}

void HtmlTableElement::collectAdditionalCellAttributeStyle(AttributeStyle& style) const
{
    if(m_padding > 0) {
        style.addProperty(CssPropertyID::Padding, CssLengthValue::create(m_padding));
    }

    if(m_border > 0 && m_rules == Rules::Unset) {
        style.addProperty(CssPropertyID::BorderWidth, CssValueID::Inset);
        style.addProperty(CssPropertyID::BorderStyle, CssValueID::Solid);
        style.addProperty(CssPropertyID::BorderColor, CssInheritValue::create());
    } else {
        switch(m_rules) {
        case Rules::Rows:
            style.addProperty(CssPropertyID::BorderTopWidth, CssValueID::Thin);
            style.addProperty(CssPropertyID::BorderBottomWidth,
                              CssValueID::Thin);
            style.addProperty(CssPropertyID::BorderTopStyle, CssValueID::Solid);
            style.addProperty(CssPropertyID::BorderBottomStyle,
                              CssValueID::Solid);
            style.addProperty(CssPropertyID::BorderColor,
                              CssInheritValue::create());
        case Rules::Cols:
            style.addProperty(CssPropertyID::BorderLeftWidth, CssValueID::Thin);
            style.addProperty(CssPropertyID::BorderRightWidth,
                              CssValueID::Thin);
            style.addProperty(CssPropertyID::BorderLeftStyle,
                              CssValueID::Solid);
            style.addProperty(CssPropertyID::BorderRightStyle,
                              CssValueID::Solid);
            style.addProperty(CssPropertyID::BorderColor,
                              CssInheritValue::create());
            break;
        case Rules::All:
            style.addProperty(CssPropertyID::BorderWidth, CssValueID::Thin);
            style.addProperty(CssPropertyID::BorderStyle, CssValueID::Solid);
            style.addProperty(CssPropertyID::BorderColor,
                              CssInheritValue::create());
            break;
        default:
            break;
        }
    }
}

void HtmlTableElement::collectAdditionalRowGroupAttributeStyle(AttributeStyle& style) const
{
    if(m_rules == Rules::Groups) {
        style.addProperty(CssPropertyID::BorderTopWidth, CssValueID::Thin);
        style.addProperty(CssPropertyID::BorderBottomWidth,
            CssValueID::Thin);
        style.addProperty(CssPropertyID::BorderTopStyle, CssValueID::Solid);
        style.addProperty(CssPropertyID::BorderBottomStyle,
            CssValueID::Solid);
    }
}

void HtmlTableElement::collectAdditionalColGroupAttributeStyle(AttributeStyle& style) const
{
    if(m_rules == Rules::Groups) {
        style.addProperty(CssPropertyID::BorderLeftWidth, CssValueID::Thin);
        style.addProperty(CssPropertyID::BorderRightWidth,
            CssValueID::Thin);
        style.addProperty(CssPropertyID::BorderLeftStyle,
            CssValueID::Solid);
        style.addProperty(CssPropertyID::BorderRightStyle,
            CssValueID::Solid);
    }
}

void HtmlTableElement::collectAdditionalAttributeStyle(AttributeStyle& style) const
{
    if (m_rules > Rules::Unset) {
        style.addProperty(CssPropertyID::BorderCollapse, CssValueID::Collapse);
    }

    if (m_frame > Frame::Unset) {
        auto topStyle = CssValueID::Hidden;
        auto bottomStyle = CssValueID::Hidden;
        auto leftStyle = CssValueID::Hidden;
        auto rightStyle = CssValueID::Hidden;
        switch (m_frame) {
        case Frame::Above:
            topStyle = CssValueID::Solid;
            break;
        case Frame::Below:
            bottomStyle = CssValueID::Solid;
            break;
        case Frame::Hsides:
            topStyle = bottomStyle = CssValueID::Solid;
            break;
        case Frame::Lhs:
            leftStyle = CssValueID::Solid;
            break;
        case Frame::Rhs:
            rightStyle = CssValueID::Solid;
            break;
        case Frame::Vsides:
            leftStyle = rightStyle = CssValueID::Solid;
            break;
        case Frame::Box:
        case Frame::Border:
            topStyle = bottomStyle = CssValueID::Solid;
            leftStyle = rightStyle = CssValueID::Solid;
            break;
        default:
            break;
        }

        style.addProperty(CssPropertyID::BorderWidth, CssValueID::Thin);
        style.addProperty(CssPropertyID::BorderTopStyle, topStyle);
        style.addProperty(CssPropertyID::BorderBottomStyle, bottomStyle);
        style.addProperty(CssPropertyID::BorderLeftStyle, leftStyle);
        style.addProperty(CssPropertyID::BorderRightStyle, rightStyle);
    } else {
        if (m_border > 0) {
            style.addProperty(CssPropertyID::BorderWidth, CssLengthValue::create(m_border));
            style.addProperty(CssPropertyID::BorderStyle, CssValueID::Outset);
        } else if (m_rules > Rules::Unset) {
            style.addProperty(CssPropertyID::BorderStyle, CssValueID::Hidden);
        }
    }
}

void HtmlTableElement::collectAttributeStyle(AttributeStyle& style) const
{
    HtmlElement::collectAttributeStyle(style);
    if (auto attr = findAttribute(widthAttr)) {
        addHtmlLengthAttributeStyle(style, CssPropertyID::Width, attr->value());
    }
    if (auto attr = findAttribute(heightAttr)) {
        addHtmlLengthAttributeStyle(style, CssPropertyID::Height, attr->value());
    }
    if (auto attr = findAttribute(valignAttr)) {
        style.addProperty(CssPropertyID::VerticalAlign, attr->value());
    }
    if (auto attr = findAttribute(cellspacingAttr)) {
        addHtmlLengthAttributeStyle(style, CssPropertyID::BorderSpacing, attr->value());
    }
    if (auto attr = findAttribute(bordercolorAttr)) {
        style.addProperty(CssPropertyID::BorderColor, attr->value());
    }
    if (auto attr = findAttribute(bgcolorAttr)) {
        style.addProperty(CssPropertyID::BackgroundColor, attr->value());
    }
    if (auto attr = findAttribute(backgroundAttr)) {
        style.addProperty(CssPropertyID::BackgroundImage, attr->value());
    }

    collectAdditionalAttributeStyle(style);
}

HtmlTableElement::Rules HtmlTableElement::parseRulesAttribute(std::string_view value)
{
    static constexpr auto table =
        makeIdentTable<Rules>({{"none", Rules::None},
                               {"groups", Rules::Groups},
                               {"rows", Rules::Rows},
                               {"cols", Rules::Cols},
                               {"all", Rules::All}});
    const auto it = table.find(value);
    return it == table.end() ? Rules::Unset : it->second;
}

HtmlTableElement::Frame HtmlTableElement::parseFrameAttribute(std::string_view value)
{
    static constexpr auto table = makeIdentTable<Frame>({
        {"void", Frame::Void},
        {"above", Frame::Above},
        {"below", Frame::Below},
        {"hsides", Frame::Hsides},
        {"lhs", Frame::Lhs},
        {"rhs", Frame::Rhs},
        {"vsides", Frame::Vsides},
        {"box", Frame::Box},
        {"border", Frame::Border},
    });
    const auto it = table.find(value);
    return it == table.end() ? Frame::Unset : it->second;
}

HtmlTablePartElement::HtmlTablePartElement(Document* document, GlobalString tagName)
    : HtmlElement(document, tagName)
{
}

HtmlTableElement* HtmlTablePartElement::findParentTable() const
{
    auto parent = parentElement();
    while(parent && !parent->isOfType(xhtmlNs, tableTag))
        parent = parent->parentElement();
    return static_cast<HtmlTableElement*>(parent);
}

void HtmlTablePartElement::collectAttributeStyle(AttributeStyle& style) const
{
    HtmlElement::collectAttributeStyle(style);
    if (auto attr = findAttribute(heightAttr)) {
        addHtmlLengthAttributeStyle(style, CssPropertyID::Height, attr->value());
    }
    if (auto attr = findAttribute(valignAttr)) {
        style.addProperty(CssPropertyID::VerticalAlign, attr->value());
    }
    if (auto attr = findAttribute(bgcolorAttr)) {
        style.addProperty(CssPropertyID::BackgroundColor, attr->value());
    }
    if (auto attr = findAttribute(backgroundAttr)) {
        style.addProperty(CssPropertyID::BackgroundImage, attr->value());
    }
}

HtmlTableSectionElement::HtmlTableSectionElement(Document* document, GlobalString tagName)
    : HtmlTablePartElement(document, tagName)
{
}

void HtmlTableSectionElement::collectAttributeStyle(AttributeStyle& style) const
{
    HtmlTablePartElement::collectAttributeStyle(style);
    if(auto table = findParentTable()) {
        table->collectAdditionalRowGroupAttributeStyle(style);
    }
}

HtmlTableRowElement::HtmlTableRowElement(Document* document)
    : HtmlTablePartElement(document, trTag)
{
}

HtmlTableColElement::HtmlTableColElement(Document* document, GlobalString tagName)
    : HtmlTablePartElement(document, tagName)
{
}

unsigned HtmlTableColElement::span() const
{
    return parseIntegerAttribute<unsigned>(spanAttr).value_or(1);
}

void HtmlTableColElement::collectAttributeStyle(AttributeStyle& style) const
{
    HtmlTablePartElement::collectAttributeStyle(style);
    if(tagName() == colgroupTag) {
        if(auto table = findParentTable()) {
            table->collectAdditionalColGroupAttributeStyle(style);
        }
    }
}

Box* HtmlTableColElement::createBox(const RefPtr<BoxStyle>& style)
{
    auto box = HtmlElement::createBox(style);
    if(auto column = to<TableColumnBox>(box))
        column->setSpan(span());
    return box;
}

HtmlTableCellElement::HtmlTableCellElement(Document* document, GlobalString tagName)
    : HtmlTablePartElement(document, tagName)
{
}

unsigned HtmlTableCellElement::colSpan() const
{
    return std::max(1u, parseIntegerAttribute<unsigned>(colspanAttr).value_or(1));
}

unsigned HtmlTableCellElement::rowSpan() const
{
    return parseIntegerAttribute<unsigned>(rowspanAttr).value_or(1);
}

void HtmlTableCellElement::collectAttributeStyle(AttributeStyle& style) const
{
    HtmlTablePartElement::collectAttributeStyle(style);
    if(auto table = findParentTable()) {
        table->collectAdditionalCellAttributeStyle(style);
    }
}

Box* HtmlTableCellElement::createBox(const RefPtr<BoxStyle>& style)
{
    auto box = HtmlElement::createBox(style);
    if(auto cell = to<TableCellBox>(box)) {
        cell->setColSpan(colSpan());
        cell->setRowSpan(rowSpan());
    }

    return box;
}

HtmlInputElement::HtmlInputElement(Document* document)
    : HtmlElement(document, inputTag)
{
}

unsigned HtmlInputElement::size() const
{
    return std::max(1u, parseIntegerAttribute<unsigned>(sizeAttr).value_or(20));
}

Box* HtmlInputElement::createBox(const RefPtr<BoxStyle>& style)
{
    const auto& type = getAttribute(typeAttr);
    if(!type.empty()) {
        char buffer[16];
        if (type.size() > sizeof(buffer) ||
            !makeIdentSet({"text", "search", "url", "tel", "email", "password"})
                 .contains(toLower(type, buffer))) {
            return HtmlElement::createBox(style);
        }
    }

    auto box = new TextInputBox(this, style);
    box->setCols(size());
    return box;
}

HtmlTextAreaElement::HtmlTextAreaElement(Document* document)
    : HtmlElement(document, textareaTag)
{
}

unsigned HtmlTextAreaElement::rows() const
{
    return std::max(1u, parseIntegerAttribute<unsigned>(rowsAttr).value_or(2));
}

unsigned HtmlTextAreaElement::cols() const
{
    return std::max(1u, parseIntegerAttribute<unsigned>(colsAttr).value_or(20));
}

Box* HtmlTextAreaElement::createBox(const RefPtr<BoxStyle>& style)
{
    auto box = new TextInputBox(this, style);
    box->setRows(rows());
    box->setCols(cols());
    return box;
}

HtmlSelectElement::HtmlSelectElement(Document* document)
    : HtmlElement(document, selectTag)
{
}

unsigned HtmlSelectElement::size() const
{
    if(auto size = parseIntegerAttribute<unsigned>(sizeAttr))
        return std::max(1u, size.value());
    return hasAttribute(multipleAttr) ? 4 : 1;
}

Box* HtmlSelectElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new SelectBox(this, style);
}

HtmlStyleElement::HtmlStyleElement(Document* document)
    : HtmlElement(document, styleTag)
{
}

const HeapString& HtmlStyleElement::type() const
{
    return getAttribute(typeAttr);
}

const HeapString& HtmlStyleElement::media() const
{
    return getAttribute(mediaAttr);
}

void HtmlStyleElement::finishParsingDocument()
{
    if(document()->supportsMedia(type(), media()))
        document()->addAuthorStyleSheet(textFromChildren(), document()->baseUrl());
    HtmlElement::finishParsingDocument();
}

HtmlLinkElement::HtmlLinkElement(Document* document)
    : HtmlElement(document, linkTag)
{
}

const HeapString& HtmlLinkElement::rel() const
{
    return getAttribute(relAttr);
}

const HeapString& HtmlLinkElement::type() const
{
    return getAttribute(typeAttr);
}

const HeapString& HtmlLinkElement::media() const
{
    return getAttribute(mediaAttr);
}

void HtmlLinkElement::finishParsingDocument()
{
    if(iequals(rel(), "stylesheet") && document()->supportsMedia(type(), media())) {
        auto url = getUrlAttribute(hrefAttr);
        if(auto resource = document()->fetchTextResource(url)) {
            document()->addAuthorStyleSheet(resource->text(), std::move(url));
        }
    }

    HtmlElement::finishParsingDocument();
}

HtmlTitleElement::HtmlTitleElement(Document* document)
    : HtmlElement(document, titleTag)
{
}

void HtmlTitleElement::finishParsingDocument()
{
    auto book = document()->book();
    if(book && book->title().empty())
        book->setTitle(textFromChildren());
    HtmlElement::finishParsingDocument();
}

HtmlBaseElement::HtmlBaseElement(Document* document)
    : HtmlElement(document, baseTag)
{
}

void HtmlBaseElement::finishParsingDocument()
{
    Url baseUrl(getAttribute(hrefAttr));
    if(!baseUrl.isEmpty())
        document()->setBaseUrl(std::move(baseUrl));
    HtmlElement::finishParsingDocument();
}

std::unique_ptr<HtmlDocument> HtmlDocument::create(Book* book, ResourceFetcher* fetcher, Url baseUrl)
{
    return std::unique_ptr<HtmlDocument>(new HtmlDocument(book, fetcher, std::move(baseUrl)));
}

bool HtmlDocument::parse(const std::string_view& content)
{
    return HtmlParser(this, content).parse();
}

HtmlDocument::HtmlDocument(Book* book, ResourceFetcher* fetcher, Url baseUrl)
    : Document(classKind, book, fetcher, std::move(baseUrl))
{
}

} // namespace plutobook
