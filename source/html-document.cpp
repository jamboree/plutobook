#include "html-document.h"
#include "html-parser.h"
#include "css-rule.h"
#include "counters.h"
#include "text-box.h"
#include "content-box.h"
#include "replaced-box.h"
#include "table-box.h"
#include "form-controlbox.h"
#include "text-resource.h"
#include "image-resource.h"
#include "string-utils.h"

#include "plutobook.hpp"

#include <unicode/uchar.h>
#include <unicode/uiter.h>

namespace plutobook {

HtmlElement::HtmlElement(Document* document, const GlobalString& tagName)
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
    auto style = document()->pseudoStyleForElement(this, PseudoType::FirstLetter, parent->style());
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
                auto newTextBox = new (heap()) TextBox(nullptr, style);
                newTextBox->setText(text.substring(0, length));
                textBox->setText(text.substring(length));

                auto letterBox = Box::create(nullptr, style);
                letterBox->addChild(newTextBox);
                textBox->parentBox()->insertChild(letterBox, textBox);
                break;
            }
        }

        if(!child->isFloatingOrPositioned() && !child->isListMarkerBox()
            && !child->isTableBox() && !child->isFlexibleBox()) {
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
    auto style = document()->pseudoStyleForElement(this, pseudoType, parent->style());
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
    auto style = document()->styleForElement(this, parent->style());
    if(style == nullptr || style->display() == Display::None)
        return;
    if(style->position() == Position::Running) {
        const auto* value = style->get(CssPropertyID::Position);
        const auto& position = to<CssUnaryFunctionValue>(*value);
        assert(position.id() == CssFunctionID::Running);
        const auto& name = to<CssCustomIdentValue>(*position.value());
        document()->addRunningStyle(name.value(), std::move(style));
        return;
    }

    auto box = createBox(style);
    if(box == nullptr)
        return;
    parent->addChild(box);
    buildElementBox(counters, box);
}

static void addHtmlAttributeStyle(std::string& output, const std::string_view& name, const std::string_view& value)
{
    if(value.empty())
        return;
    output += name;
    output += ':';
    output += value;
    output += ';';
}

void HtmlElement::collectAttributeStyle(std::string& output, const GlobalString& name, const HeapString& value) const
{
    if(name == hiddenAttr) {
        addHtmlAttributeStyle(output, "display", "none");
    } else if(name == alignAttr) {
        addHtmlAttributeStyle(output, "text-align", value);
    } else {
        Element::collectAttributeStyle(output, name, value);
    }
}

template<typename T = int>
static std::optional<T> parseHtmlInteger(std::string_view input)
{
    constexpr auto isSigned = std::numeric_limits<T>::is_signed;
    stripLeadingAndTrailingSpaces(input);
    bool isNegative = false;
    if(!input.empty() && input.front() == '+')
        input.remove_prefix(1);
    else if(!input.empty() && isSigned && input.front() == '-') {
        input.remove_prefix(1);
        isNegative = true;
    }

    if(input.empty() || !isDigit(input.front()))
        return std::nullopt;
    T output = 0;
    do {
        output = output * 10 + input.front() - '0';
        input.remove_prefix(1);
    } while(!input.empty() && isDigit(input.front()));

    using SignedType = typename std::make_signed<T>::type;
    if(isNegative)
        output = -static_cast<SignedType>(output);
    return output;
}

static std::optional<unsigned> parseHtmlNonNegativeInteger(std::string_view input)
{
    return parseHtmlInteger<unsigned>(input);
}

template<typename T>
std::optional<T> HtmlElement::parseIntegerAttribute(const GlobalString& name) const
{
    const auto& value = getAttribute(name);
    if(!value.empty())
        return parseHtmlInteger<T>(value);
    return std::nullopt;
}

std::optional<unsigned> HtmlElement::parseNonNegativeIntegerAttribute(const GlobalString& name) const
{
    return parseIntegerAttribute<unsigned>(name);
}

static void addHtmlLengthAttributeStyle(std::string& output, const std::string_view& name, const std::string_view& value)
{
    if(value.empty())
        return;
    size_t index = 0;
    while(index < value.length() && isSpace(value[index]))
        ++index;
    size_t begin = index;
    while(index < value.length() && isDigit(value[index])) {
        ++index;
    }

    if(index == begin)
        return;
    if(index < value.length() && value[index] == '.') {
        ++index;
        while(index < value.length() && isDigit(value[index])) {
            ++index;
        }
    }

    output += name;
    output += ':';
    output += value.substr(begin, index - begin);
    if(index < value.length() && value[index] == '%') {
        output += "%;";
    } else {
        output += "px;";
    }
}

static void addHtmlLengthAttributeStyle(std::string& output, const std::string_view& name, int value)
{
    output += name;
    output += ':';
    output += toString(value);
    if(value) {
        output += "px;";
    } else {
        output += ';';
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

void HtmlBodyElement::collectAttributeStyle(std::string& output, const GlobalString& name, const HeapString& value) const
{
    if(name == textAttr) {
        addHtmlAttributeStyle(output, "color", value);
    } else if(name == bgcolorAttr) {
        addHtmlAttributeStyle(output, "background-color", value);
    } else if(name == backgroundAttr) {
        addHtmlUrlAttributeStyle(output, "background-image", value);
    } else {
        HtmlElement::collectAttributeStyle(output, name, value);
    }
}

HtmlFontElement::HtmlFontElement(Document* document)
    : HtmlElement(document, fontTag)
{
}

static void addHtmlFontSizeAttributeStyle(std::string& output, std::string_view input)
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
        return;
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

    output += "font-size:";
    switch(value) {
    case 1:
        output += "x-small;";
        break;
    case 2:
        output += "small;";
        break;
    case 3:
        output += "medium;";
        break;
    case 4:
        output += "large;";
        break;
    case 5:
        output += "x-large;";
        break;
    case 6:
        output += "xx-large;";
        break;
    case 7:
        output += "xxx-large;";
        break;
    default:
        assert(false);
    }
}

void HtmlFontElement::collectAttributeStyle(std::string& output, const GlobalString& name, const HeapString& value) const
{
    if(name == sizeAttr) {
        addHtmlFontSizeAttributeStyle(output, value);
    } else if(name == faceAttr) {
        addHtmlAttributeStyle(output, "font-family", value);
    } else if(name == colorAttr) {
        addHtmlAttributeStyle(output, "color", value);
    } else {
        HtmlElement::collectAttributeStyle(output, name, value);
    }
}

HtmlImageElement::HtmlImageElement(Document* document)
    : HtmlElement(document, imgTag)
{
}

void HtmlImageElement::collectAttributeStyle(std::string& output, const GlobalString& name, const HeapString& value) const
{
    if(name == widthAttr) {
        addHtmlLengthAttributeStyle(output, "width", value);
    } else if(name == heightAttr) {
        addHtmlLengthAttributeStyle(output, "height", value);
    } else if(name == hspaceAttr) {
        addHtmlLengthAttributeStyle(output, "margin-left", value);
        addHtmlLengthAttributeStyle(output, "margin-right", value);
    } else if(name == vspaceAttr) {
        addHtmlLengthAttributeStyle(output, "margin-top", value);
        addHtmlLengthAttributeStyle(output, "margin-bottom", value);
    } else if(name == borderAttr) {
        addHtmlLengthAttributeStyle(output, "border-width", value);
        addHtmlAttributeStyle(output, "border-style", "solid");
    } else if(name == valignAttr) {
        addHtmlAttributeStyle(output, "vertical-align", value);
    } else {
        HtmlElement::collectAttributeStyle(output, name, value);
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
    auto text = altText();
    if(image == nullptr && text.empty())
        return new (heap()) ImageBox(this, style);
    if(image == nullptr) {
        auto container = Box::create(this, style);
        auto box = new (heap()) TextBox(nullptr, style);
        box->setText(text);
        container->addChild(box);
        return container;
    }

    auto box = new (heap()) ImageBox(this, style);
    box->setImage(std::move(image));
    return box;
}

HtmlHrElement::HtmlHrElement(Document* document)
    : HtmlElement(document, hrTag)
{
}

void HtmlHrElement::collectAttributeStyle(std::string& output, const GlobalString& name, const HeapString& value) const
{
    if(name == widthAttr) {
        addHtmlLengthAttributeStyle(output, "width", value);
    } else if(name == sizeAttr) {
        auto size = parseHtmlInteger(value);
        if(size && size.value() > 1) {
            addHtmlLengthAttributeStyle(output, "height", size.value() - 2);
        } else {
            addHtmlLengthAttributeStyle(output, "border-bottom-width", 0);
        }
    } else if(name == alignAttr) {
        if(equalsIgnoringCase(value, "left")) {
            addHtmlLengthAttributeStyle(output, "margin-left", 0);
            addHtmlAttributeStyle(output, "margin-right", "auto");
        } else if(equalsIgnoringCase(value, "right")) {
            addHtmlAttributeStyle(output, "margin-left", "auto");
            addHtmlLengthAttributeStyle(output, "margin-right", 0);
        } else {
            addHtmlAttributeStyle(output, "margin-left", "auto");
            addHtmlAttributeStyle(output, "margin-right", "auto");
        }
    } else if(name == colorAttr) {
        addHtmlAttributeStyle(output, "border-style", "solid");
        addHtmlAttributeStyle(output, "border-color", value);
        addHtmlAttributeStyle(output, "background-color", value);
    } else if(name == noshadeAttr) {
        addHtmlAttributeStyle(output, "border-style", "solid");
        addHtmlAttributeStyle(output, "border-color", "darkgray");
        addHtmlAttributeStyle(output, "background-color", "darkgray");
    } else {
        HtmlElement::collectAttributeStyle(output, name, value);
    }
}

HtmlBrElement::HtmlBrElement(Document* document)
    : HtmlElement(document, brTag)
{
}

Box* HtmlBrElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new (heap()) LineBreakBox(this, style);
}

HtmlWbrElement::HtmlWbrElement(Document* document)
    : HtmlElement(document, wbrTag)
{
}

Box* HtmlWbrElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new (heap()) WordBreakBox(this, style);
}

HtmlLiElement::HtmlLiElement(Document* document)
    : HtmlElement(document, liTag)
{
}

std::optional<int> HtmlLiElement::value() const
{
    return parseIntegerAttribute(valueAttr);
}

std::string_view listTypeAttributeToStyleName(const std::string_view& value)
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

void HtmlLiElement::collectAttributeStyle(std::string& output, const GlobalString& name, const HeapString& value) const
{
    if(name == typeAttr) {
        addHtmlAttributeStyle(output, "list-style-type", listTypeAttributeToStyleName(value));
    } else {
        HtmlElement::collectAttributeStyle(output, name, value);
    }
}

HtmlOlElement::HtmlOlElement(Document* document)
    : HtmlElement(document, olTag)
{
}

int HtmlOlElement::start() const
{
    return parseIntegerAttribute(startAttr).value_or(1);
}

void HtmlOlElement::collectAttributeStyle(std::string& output, const GlobalString& name, const HeapString& value) const
{
    if(name == typeAttr) {
        addHtmlAttributeStyle(output, "list-style-type", listTypeAttributeToStyleName(value));
    } else {
        HtmlElement::collectAttributeStyle(output, name, value);
    }
}

HtmlTableElement::HtmlTableElement(Document* document)
    : HtmlElement(document, tableTag)
{
}

void HtmlTableElement::parseAttribute(const GlobalString& name, const HeapString& value)
{
    if(name == cellpaddingAttr) {
        m_padding = parseHtmlNonNegativeInteger(value).value_or(0);
    } else if(name == borderAttr) {
        m_border = parseHtmlNonNegativeInteger(value).value_or(1);
    } else if(name == rulesAttr) {
        m_rules = parseRulesAttribute(value);
    } else if(name == frameAttr) {
        m_frame = parseFrameAttribute(value);
    } else {
        HtmlElement::parseAttribute(name, value);
    }
}

void HtmlTableElement::collectAdditionalCellAttributeStyle(std::string& output) const
{
    if(m_padding > 0) {
        addHtmlLengthAttributeStyle(output, "padding", m_padding);
    }

    if(m_border > 0 && m_rules == Rules::Unset) {
        addHtmlAttributeStyle(output, "border-width", "inset");
        addHtmlAttributeStyle(output, "border-style", "solid");
        addHtmlAttributeStyle(output, "border-color", "inherit");
    } else {
        switch(m_rules) {
        case Rules::Rows:
            addHtmlAttributeStyle(output, "border-top-width", "thin");
            addHtmlAttributeStyle(output, "border-bottom-width", "thin");
            addHtmlAttributeStyle(output, "border-top-style", "solid");
            addHtmlAttributeStyle(output, "border-bottom-style", "solid");
            addHtmlAttributeStyle(output, "border-color", "inherit");
        case Rules::Cols:
            addHtmlAttributeStyle(output, "border-left-width", "thin");
            addHtmlAttributeStyle(output, "border-right-width", "thin");
            addHtmlAttributeStyle(output, "border-left-style", "solid");
            addHtmlAttributeStyle(output, "border-right-style", "solid");
            addHtmlAttributeStyle(output, "border-color", "inherit");
            break;
        case Rules::All:
            addHtmlAttributeStyle(output, "border-width", "thin");
            addHtmlAttributeStyle(output, "border-style", "solid");
            addHtmlAttributeStyle(output, "border-color", "inherit");
            break;
        default:
            break;
        }
    }
}

void HtmlTableElement::collectAdditionalRowGroupAttributeStyle(std::string& output) const
{
    if(m_rules == Rules::Groups) {
        addHtmlAttributeStyle(output, "border-top-width", "thin");
        addHtmlAttributeStyle(output, "border-bottom-width", "thin");
        addHtmlAttributeStyle(output, "border-top-style", "solid");
        addHtmlAttributeStyle(output, "border-bottom-style", "solid");
    }
}

void HtmlTableElement::collectAdditionalColGroupAttributeStyle(std::string& output) const
{
    if(m_rules == Rules::Groups) {
        addHtmlAttributeStyle(output, "border-left-width", "thin");
        addHtmlAttributeStyle(output, "border-right-width", "thin");
        addHtmlAttributeStyle(output, "border-left-style", "solid");
        addHtmlAttributeStyle(output, "border-right-style", "solid");
    }
}

void HtmlTableElement::collectAdditionalAttributeStyle(std::string& output) const
{
    HtmlElement::collectAdditionalAttributeStyle(output);
    if(m_rules > Rules::Unset) {
        addHtmlAttributeStyle(output, "border-collapse", "collapse");
    }

    if(m_frame > Frame::Unset) {
        auto topStyle = "hidden";
        auto bottomStyle = "hidden";
        auto leftStyle = "hidden";
        auto rightStyle = "hidden";
        switch(m_frame) {
        case Frame::Above:
            topStyle = "solid";
            break;
        case Frame::Below:
            bottomStyle = "solid";
            break;
        case Frame::Hsides:
            topStyle = bottomStyle = "solid";
            break;
        case Frame::Lhs:
            leftStyle = "solid";
            break;
        case Frame::Rhs:
            rightStyle = "solid";
            break;
        case Frame::Vsides:
            leftStyle = rightStyle = "solid";
            break;
        case Frame::Box:
        case Frame::Border:
            topStyle = bottomStyle = "solid";
            leftStyle = rightStyle = "solid";
            break;
        default:
            break;
        }

        addHtmlAttributeStyle(output, "border-width", "thin");
        addHtmlAttributeStyle(output, "border-top-style", topStyle);
        addHtmlAttributeStyle(output, "border-bottom-style", bottomStyle);
        addHtmlAttributeStyle(output, "border-left-style", leftStyle);
        addHtmlAttributeStyle(output, "border-right-style", rightStyle);
    } else {
        if(m_border > 0) {
            addHtmlLengthAttributeStyle(output, "border-width", m_border);
            addHtmlAttributeStyle(output, "border-style", "outset");
        } else if(m_rules > Rules::Unset) {
            addHtmlAttributeStyle(output, "border-style", "hidden");
        }
    }
}

void HtmlTableElement::collectAttributeStyle(std::string& output, const GlobalString& name, const HeapString& value) const
{
    if(name == widthAttr) {
        addHtmlLengthAttributeStyle(output, "width", value);
    } else if(name == heightAttr) {
        addHtmlLengthAttributeStyle(output, "height", value);
    } else if(name == valignAttr) {
        addHtmlAttributeStyle(output, "vertical-align", value);
    } else if(name == cellspacingAttr) {
        addHtmlLengthAttributeStyle(output, "border-spacing", value);
    } else if(name == bordercolorAttr) {
        addHtmlAttributeStyle(output, "border-color", value);
    } else if(name == bgcolorAttr) {
        addHtmlAttributeStyle(output, "background-color", value);
    } else if(name == backgroundAttr) {
        addHtmlUrlAttributeStyle(output, "background-image", value);
    } else {
        HtmlElement::collectAttributeStyle(output, name, value);
    }
}

HtmlTableElement::Rules HtmlTableElement::parseRulesAttribute(std::string_view value)
{
    if(equalsIgnoringCase(value, "none"))
        return Rules::None;
    if(equalsIgnoringCase(value, "groups"))
        return Rules::Groups;
    if(equalsIgnoringCase(value, "rows"))
        return Rules::Rows;
    if(equalsIgnoringCase(value, "cols"))
        return Rules::Cols;
    if(equalsIgnoringCase(value, "all"))
        return Rules::All;
    return Rules::Unset;
}

HtmlTableElement::Frame HtmlTableElement::parseFrameAttribute(std::string_view value)
{
    if(equalsIgnoringCase(value, "void"))
        return Frame::Void;
    if(equalsIgnoringCase(value, "above"))
        return Frame::Above;
    if(equalsIgnoringCase(value, "below"))
        return Frame::Below;
    if(equalsIgnoringCase(value, "hsides"))
        return Frame::Hsides;
    if(equalsIgnoringCase(value, "lhs"))
        return Frame::Lhs;
    if(equalsIgnoringCase(value, "rhs"))
        return Frame::Rhs;
    if(equalsIgnoringCase(value, "vsides"))
        return Frame::Vsides;
    if(equalsIgnoringCase(value, "box"))
        return Frame::Box;
    if(equalsIgnoringCase(value, "border"))
        return Frame::Border;
    return Frame::Unset;
}

HtmlTablePartElement::HtmlTablePartElement(Document* document, const GlobalString& tagName)
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

void HtmlTablePartElement::collectAttributeStyle(std::string& output, const GlobalString& name, const HeapString& value) const
{
    if(name == heightAttr) {
        addHtmlLengthAttributeStyle(output, "height", value);
    } else if(name == valignAttr) {
        addHtmlAttributeStyle(output, "vertical-align", value);
    } else if(name == bgcolorAttr) {
        addHtmlAttributeStyle(output, "background-color", value);
    } else if(name == backgroundAttr) {
        addHtmlUrlAttributeStyle(output, "background-image", value);
    } else {
        HtmlElement::collectAttributeStyle(output, name, value);
    }
}

HtmlTableSectionElement::HtmlTableSectionElement(Document* document, const GlobalString& tagName)
    : HtmlTablePartElement(document, tagName)
{
}

void HtmlTableSectionElement::collectAdditionalAttributeStyle(std::string& output) const
{
    HtmlTablePartElement::collectAdditionalAttributeStyle(output);
    if(auto table = findParentTable()) {
        table->collectAdditionalRowGroupAttributeStyle(output);
    }
}

HtmlTableRowElement::HtmlTableRowElement(Document* document)
    : HtmlTablePartElement(document, trTag)
{
}

HtmlTableColElement::HtmlTableColElement(Document* document, const GlobalString& tagName)
    : HtmlTablePartElement(document, tagName)
{
}

unsigned HtmlTableColElement::span() const
{
    return parseNonNegativeIntegerAttribute(spanAttr).value_or(1);
}

void HtmlTableColElement::collectAdditionalAttributeStyle(std::string& output) const
{
    HtmlTablePartElement::collectAdditionalAttributeStyle(output);
    if(tagName() == colgroupTag) {
        if(auto table = findParentTable()) {
            table->collectAdditionalColGroupAttributeStyle(output);
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

HtmlTableCellElement::HtmlTableCellElement(Document* document, const GlobalString& tagName)
    : HtmlTablePartElement(document, tagName)
{
}

unsigned HtmlTableCellElement::colSpan() const
{
    return std::max(1u, parseNonNegativeIntegerAttribute(colspanAttr).value_or(1));
}

unsigned HtmlTableCellElement::rowSpan() const
{
    return parseNonNegativeIntegerAttribute(rowspanAttr).value_or(1);
}

void HtmlTableCellElement::collectAdditionalAttributeStyle(std::string& output) const
{
    HtmlTablePartElement::collectAdditionalAttributeStyle(output);
    if(auto table = findParentTable()) {
        table->collectAdditionalCellAttributeStyle(output);
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
    return std::max(1u, parseNonNegativeIntegerAttribute(sizeAttr).value_or(20));
}

Box* HtmlInputElement::createBox(const RefPtr<BoxStyle>& style)
{
    const auto& type = getAttribute(typeAttr);
    if(!type.empty() && !equals(type, "text", false)
        && !equals(type, "search", false)
        && !equals(type, "url", false)
        && !equals(type, "tel", false)
        && !equals(type, "email", false)
        && !equals(type, "password", false)) {
        return HtmlElement::createBox(style);
    }

    auto box = new (heap()) TextInputBox(this, style);
    box->setCols(size());
    return box;
}

HtmlTextAreaElement::HtmlTextAreaElement(Document* document)
    : HtmlElement(document, textareaTag)
{
}

unsigned HtmlTextAreaElement::rows() const
{
    return std::max(1u, parseNonNegativeIntegerAttribute(rowsAttr).value_or(2));
}

unsigned HtmlTextAreaElement::cols() const
{
    return std::max(1u, parseNonNegativeIntegerAttribute(colsAttr).value_or(20));
}

Box* HtmlTextAreaElement::createBox(const RefPtr<BoxStyle>& style)
{
    auto box = new (heap()) TextInputBox(this, style);
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
    if(auto size = parseNonNegativeIntegerAttribute(sizeAttr))
        return std::max(1u, size.value());
    return hasAttribute(multipleAttr) ? 4 : 1;
}

Box* HtmlSelectElement::createBox(const RefPtr<BoxStyle>& style)
{
    return new (heap()) SelectBox(this, style);
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
    if(equals(rel(), "stylesheet", false) && document()->supportsMedia(type(), media())) {
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

std::unique_ptr<HtmlDocument> HtmlDocument::create(Book* book, Heap* heap, ResourceFetcher* fetcher, Url baseUrl)
{
    return std::unique_ptr<HtmlDocument>(new (heap) HtmlDocument(book, heap, fetcher, std::move(baseUrl)));
}

bool HtmlDocument::parse(const std::string_view& content)
{
    return HtmlParser(this, content).parse();
}

HtmlDocument::HtmlDocument(Book* book, Heap* heap, ResourceFetcher* fetcher, Url baseUrl)
    : Document(classKind, book, heap, fetcher, std::move(baseUrl))
{
}

} // namespace plutobook
