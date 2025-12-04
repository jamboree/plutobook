#include "content-box.h"
#include "replaced-box.h"
#include "image-resource.h"
#include "html-document.h"
#include "css-rule.h"
#include "counters.h"
#include "qrcodegen.h"

#include <sstream>

namespace plutobook {

ContentBox::ContentBox(ClassKind type, const RefPtr<BoxStyle>& style)
    : TextBox(type, nullptr, style)
{
}

LeaderBox::LeaderBox(const RefPtr<BoxStyle>& style)
    : ContentBox(classKind, style)
{
}

TargetCounterBox::TargetCounterBox(const RefPtr<BoxStyle>& style, const HeapString& fragment, GlobalString identifier, const HeapString& seperator, GlobalString listStyle)
    : ContentBox(classKind, style)
    , m_fragment(fragment)
    , m_identifier(identifier)
    , m_seperator(seperator)
    , m_listStyle(listStyle)
{
}

void TargetCounterBox::build()
{
    setText(document()->getTargetCounterText(m_fragment, m_identifier, m_listStyle, m_seperator));
}

ContentBoxBuilder::ContentBoxBuilder(Counters& counters, Element* element, Box* box)
    : m_counters(counters)
    , m_element(element)
    , m_box(box)
    , m_style(box->style())
{
}

void ContentBoxBuilder::addText(const HeapString& text)
{
    if(text.empty())
        return;
    if(m_lastTextBox) {
        m_lastTextBox->appendText(text);
        return;
    }

    auto newBox = new TextBox(nullptr, m_style);
    newBox->setText(text);
    m_box->addChild(newBox);
    m_lastTextBox = newBox;
}

void ContentBoxBuilder::addLeaderText(const HeapString& text)
{
    if(text.empty())
        return;
    auto newBox = new LeaderBox(m_style);
    newBox->setText(text);
    m_box->addChild(newBox);
    m_lastTextBox = nullptr;
}

void ContentBoxBuilder::addLeader(const CssValue& value)
{
    if(is<CssStringValue>(value)) {
        addLeaderText(to<CssStringValue>(value).value());
        return;
    }

    static const auto dotted = GlobalString::get(".");
    static const auto solid = GlobalString::get("_");
    static const auto space = GlobalString::get(" ");

    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Dotted:
        addLeaderText(dotted);
        break;
    case CssValueID::Solid:
        addLeaderText(solid);
        break;
    case CssValueID::Space:
        addLeaderText(space);
        break;
    default:
        assert(false);
    }
}

void ContentBoxBuilder::addElement(const CssValue& value)
{
    if(!m_box->isPageMarginBox())
        return;
    const auto& name = to<CssCustomIdentValue>(value).value();
    auto style = m_style->document()->getRunningStyle(name);
    if(style == nullptr)
        return;
    auto& element = to<HtmlElement>(*style->node());
    auto newBox = element.createBox(style);
    if(newBox == nullptr)
        return;
    m_box->addChild(newBox);
    element.buildElementBox(m_counters, newBox);
    m_lastTextBox = nullptr;
}

void ContentBoxBuilder::addCounter(const CssCounterValue& counter)
{
    addText(m_counters.counterText(counter.identifier(), counter.listStyle(), counter.separator()));
}

void ContentBoxBuilder::addTargetCounter(const CssFunctionValue& function)
{
    HeapString fragment;
    GlobalString identifier;
    HeapString seperator;
    GlobalString listStyle;

    size_t index = 0;

    if(auto value = to<CssLocalUrlValue>(function[index])) {
        fragment = value->value();
    } else {
        fragment = resolveAttr(to<CssAttrValue>(*function[index]));
    }

    ++index;

    identifier = to<CssCustomIdentValue>(*function[index++]).value();
    if(function.id() == CssFunctionID::TargetCounters)
        seperator = to<CssStringValue>(*function[index++]).value();
    if(index < function.size()) {
        listStyle = to<CssCustomIdentValue>(*function[index++]).value();
    }

    assert(index == function.size());

    if(m_box->isPageMarginBox()) {
        addText(m_style->document()->getTargetCounterText(fragment, identifier, listStyle, seperator));
        return;
    }

    auto newStyle = BoxStyle::create(m_style, Display::Inline);
    auto newBox = new TargetCounterBox(newStyle, fragment, identifier, seperator, listStyle);
    m_box->addChild(newBox);
    m_lastTextBox = nullptr;
}

void ContentBoxBuilder::addQuote(CssValueID value)
{
    assert(value == CssValueID::OpenQuote || value == CssValueID::CloseQuote || value == CssValueID::NoOpenQuote || value == CssValueID::NoCloseQuote);
    auto openquote = (value == CssValueID::OpenQuote || value == CssValueID::NoOpenQuote);
    auto closequote = (value == CssValueID::CloseQuote || value == CssValueID::NoCloseQuote);
    auto usequote = (value == CssValueID::OpenQuote || value == CssValueID::CloseQuote);
    if(closequote && m_counters.quoteDepth())
        m_counters.decreaseQuoteDepth();
    if(usequote)
        addText(m_style->getQuote(openquote, m_counters.quoteDepth()));
    if(openquote) {
        m_counters.increaseQuoteDepth();
    }
}

void ContentBoxBuilder::addQrCode(const CssFunctionValue& function)
{
    std::string text(to<CssStringValue>(*function[0]).value());

    char fill[16] = "black";
    if(function.size() == 2) {
        const auto& color = to<CssColorValue>(*function[1]).value();
        if(color.alpha() == 255) {
            std::snprintf(fill, sizeof(fill), "#%02X%02X%02X", color.red(), color.green(), color.blue());
        } else {
            std::snprintf(fill, sizeof(fill), "#%02X%02X%02X%02X", color.red(), color.green(), color.blue(), color.alpha());
        }
    }

    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

    if(qrcodegen_encodeText(text.data(), tempBuffer, qrcode, qrcodegen_Ecc_LOW, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true)) {
        auto size = qrcodegen_getSize(qrcode);

        std::ostringstream ss;
        ss << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 " << size << ' ' << size << "\">";
        ss << "<path d=\"";
        for(int y = 0; y < size; y++) {
            for(int x = 0; x < size; x++) {
                if(qrcodegen_getModule(qrcode, x, y)) {
                    ss << 'M' << x << ',' << y << "h1v1h-1z";
                }
            }
        }

        ss << "\" fill=\"" << fill << "\"/>";
        ss << "</svg>";

        addImage(SvgImage::create(ss.view(), emptyGlo, nullptr));
    }
}

void ContentBoxBuilder::addImage(RefPtr<Image> image)
{
    if(image == nullptr)
        return;
    auto newStyle = BoxStyle::create(m_style, Display::Inline);
    auto newBox = new ImageBox(nullptr, newStyle);
    newBox->setImage(std::move(image));
    m_box->addChild(newBox);
    m_lastTextBox = nullptr;
}

const HeapString& ContentBoxBuilder::resolveAttr(const CssAttrValue& attr) const
{
    if(m_element == nullptr)
        return emptyGlo;
    auto attribute = m_element->findAttributePossiblyIgnoringCase(attr.name());
    if(attribute == nullptr)
        return attr.fallback();
    return attribute->value();
}

void ContentBoxBuilder::build()
{
    auto content = m_style->get(CssPropertyID::Content);
    if(content && content->hasID(CssValueID::None))
        return;
    if(content == nullptr || content->hasID(CssValueID::Normal)) {
        if(m_style->pseudoType() != PseudoType::Marker)
            return;
        if(auto image = m_style->listStyleImage()) {
            addImage(std::move(image));
            return;
        }

        static const auto disc = GlobalString::get("\u2022 ");
        static const auto circle = GlobalString::get("\u25E6 ");
        static const auto square = GlobalString::get("\u25AA ");

        auto listStyleType = m_style->get(CssPropertyID::ListStyleType);
        if(listStyleType == nullptr) {
            addText(disc);
            return;
        }

        if(auto ident = to<CssIdentValue>(listStyleType)) {
            switch(ident->value()) {
            case CssValueID::None:
                return;
            case CssValueID::Disc:
                addText(disc);
                return;
            case CssValueID::Circle:
                addText(circle);
                return;
            case CssValueID::Square:
                addText(square);
                return;
            default:
                assert(false);
            }
        }

        if(auto listStyle = to<CssStringValue>(listStyleType)) {
            addText(listStyle->value());
            return;
        }

        const auto& listStyle = to<CssCustomIdentValue>(*listStyleType);
        addText(m_counters.markerText(listStyle.value()));
        return;
    }

    for(const auto& value : to<CssListValue>(*content)) {
        if(auto string = to<CssStringValue>(value)) {
            addText(string->value());
        } else if(auto image = to<CssImageValue>(value)) {
            addImage(image->fetch(m_style->document()));
        } else if(auto counter = to<CssCounterValue>(value)) {
            addCounter(*counter);
        } else if(auto ident = to<CssIdentValue>(value)) {
            addQuote(ident->value());
        } else if(auto attr = to<CssAttrValue>(value)) {
            addText(resolveAttr(*attr));
        } else {
            if(is<CssFunctionValue>(value)) {
                const auto& function = to<CssFunctionValue>(*value);
                if(function.id() == CssFunctionID::TargetCounter
                    || function.id() == CssFunctionID::TargetCounters) {
                    addTargetCounter(function);
                } else {
                    assert(function.id() == CssFunctionID::Qrcode);
                    addQrCode(function);
                }
            } else {
                const auto& function = to<CssUnaryFunctionValue>(*value);
                if(function.id() == CssFunctionID::Leader) {
                    addLeader(*function.value());
                } else {
                    assert(function.id() == CssFunctionID::Element);
                    addElement(*function.value());
                }
            }
        }
    }
}

} // namespace plutobook
