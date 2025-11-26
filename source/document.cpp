#include "document.h"
#include "html-document.h"
#include "svg-document.h"
#include "css-parser.h"
#include "counters.h"
#include "text-box.h"
#include "svg-text-box.h"
#include "page-box.h"
#include "box-view.h"
#include "box-layer.h"
#include "text-resource.h"
#include "image-resource.h"
#include "font-resource.h"
#include "string-utils.h"
#include "plutobook.hpp"

#include <cmath>
#include <iostream>

namespace plutobook {
template<class F>
decltype(auto) visit(Node* p, F&& f) {
    switch (p->type()) {
    case Node::ClassKind::Text: return f(static_cast<TextNode*>(p));
    case Node::ClassKind::Element: return f(static_cast<Element*>(p));
    case Node::ClassKind::HtmlElement:
        return f(static_cast<HtmlElement*>(p));
    case Node::ClassKind::SvgElement:
        return f(static_cast<SvgElement*>(p));
    case Node::ClassKind::HtmlDocument:
        return f(static_cast<HtmlDocument*>(p));
    case Node::ClassKind::SvgDocument:
        return f(static_cast<SvgDocument*>(p));
    case Node::ClassKind::XmlDocument:
        return f(static_cast<XmlDocument*>(p));
    default: std::unreachable();
    }
}

template bool is<ContainerNode>(const Node& value);
template bool is<Element>(const Node& value);
template bool is<Document>(const Node& value);
template bool is<HtmlElement>(const Node& value);
template bool is<SvgElement>(const Node& value);
template bool is<XmlDocument>(const Node& value);

Node::Node(ClassKind type, Document* document)
    : m_type(type), m_document(document)
{
}

Node::~Node()
{
    if(m_parentNode)
        m_parentNode->removeChild(this);
    delete m_box;
}

void Node::reparent(ContainerNode* newParent)
{
    if(m_parentNode)
        m_parentNode->removeChild(this);
    newParent->appendChild(this);
}

void Node::remove()
{
    if(m_parentNode) {
        m_parentNode->removeChild(this);
    }
}

Box* Node::nextSiblingBox() const
{
    auto node = nextSibling();
    while(node) {
        if(auto box = node->box())
            return box;
        node = node->nextSibling();
    }

    return nullptr;
}

Box* Node::previousSiblingBox() const
{
    auto node = previousSibling();
    while(node) {
        if(auto box = node->box())
            return box;
        node = node->previousSibling();
    }

    return nullptr;
}

BoxStyle* Node::style() const
{
    if(m_box)
        return m_box->style();
    return nullptr;
}

bool Node::isElementNode() const {
    return is<Element>(*this);
}

bool Node::isSvgElement() const {
    return is<SvgElement>(*this);
}

bool Node::isHtmlDocument() const {
    return is<HtmlDocument>(*this);
}

bool Node::inHtmlDocument() const {
    return is<HtmlDocument>(*m_document);
}

bool Node::inSvgDocument() const {
    return is<SvgDocument>(*m_document);
}

bool Node::inXmlDocument() const {
    return is<XmlDocument>(*m_document);
}

TextNode::TextNode(Document* document, const HeapString& data)
    : Node(classKind,document)
    , m_data(data)
{
}

void TextNode::appendData(const std::string_view& data)
{
    m_data = concatenateString(m_data, data);
}

bool TextNode::isHidden(const Box* parent) const
{
    if(m_data.empty())
        return true;
    for(auto cc : m_data) {
        if(!isSpace(cc)) {
            return false;
        }
    }

    if(parent->isFlexBox() || parent->isTableBox()
        || parent->isTableSectionBox() || parent->isTableRowBox()
        || parent->isTableColumnBox()) {
        return true;
    }

    if(parent->style()->preserveNewline())
        return false;
    if(auto prevBox = previousSiblingBox())
        return !prevBox->isInline() || prevBox->isLineBreakBox();
    return !parent->isInlineBox();
}

Node* TextNode::cloneNode(bool deep)
{
    return new TextNode(document(), data());
}

Box* TextNode::createBox(const RefPtr<BoxStyle>& style)
{
    if(is<SvgElement>(*parentNode()))
        return new SvgInlineTextBox(this, style);
    auto box = new TextBox(this, style);
    box->setText(m_data);
    return box;
}

void TextNode::buildBox(Counters& counters, Box* parent)
{
    if(isHidden(parent))
        return;
    if(auto box = createBox(parent->style())) {
        parent->addChild(box);
    }
}

ContainerNode::ContainerNode(ClassKind type, Document* document)
    : Node(type, document)
{
}

ContainerNode::~ContainerNode()
{
    auto child = m_firstChild;
    while(child) {
        Node* nextChild = child->nextSibling();
        child->setParentNode(nullptr);
        child->setPreviousSibling(nullptr);
        child->setNextSibling(nullptr);
        delete child;
        child = nextChild;
    }
}

void ContainerNode::appendChild(Node* newChild)
{
    assert(newChild->parentNode() == nullptr);
    assert(newChild->previousSibling() == nullptr);
    assert(newChild->nextSibling() == nullptr);
    newChild->setParentNode(this);
    if(m_lastChild == nullptr) {
        assert(m_firstChild == nullptr);
        m_firstChild = m_lastChild = newChild;
        return;
    }

    newChild->setPreviousSibling(m_lastChild);
    m_lastChild->setNextSibling(newChild);
    m_lastChild = newChild;
}

void ContainerNode::insertChild(Node* newChild, Node* nextChild)
{
    if(nextChild == nullptr) {
        appendChild(newChild);
        return;
    }

    assert(nextChild->parentNode() == this);
    assert(newChild->parentNode() == nullptr);
    assert(newChild->previousSibling() == nullptr);
    assert(newChild->nextSibling() == nullptr);

    auto previousChild = nextChild->previousSibling();
    nextChild->setPreviousSibling(newChild);
    assert(m_lastChild != previousChild);
    if(previousChild == nullptr) {
        assert(m_firstChild == nextChild);
        m_firstChild = newChild;
    } else {
        assert(m_firstChild != nextChild);
        previousChild->setNextSibling(newChild);
    }

    newChild->setParentNode(this);
    newChild->setPreviousSibling(previousChild);
    newChild->setNextSibling(nextChild);
}

void ContainerNode::removeChild(Node* child)
{
    assert(child->parentNode() == this);
    auto nextChild = child->nextSibling();
    auto previousChild = child->previousSibling();
    if(nextChild)
        nextChild->setPreviousSibling(previousChild);
    if(previousChild) {
        previousChild->setNextSibling(nextChild);
    }

    if(m_firstChild == child)
        m_firstChild = nextChild;
    if(m_lastChild == child) {
        m_lastChild = previousChild;
    }

    child->setParentNode(nullptr);
    child->setPreviousSibling(nullptr);
    child->setNextSibling(nullptr);
}

void ContainerNode::reparentChildren(ContainerNode* newParent)
{
    while(auto child = firstChild()) {
        child->reparent(newParent);
    }
}

void ContainerNode::cloneChildren(ContainerNode* newParent)
{
    auto child = m_firstChild;
    while(child) {
        newParent->appendChild(child->cloneNode(true));
        child = child->nextSibling();
    }
}

std::string ContainerNode::textFromChildren() const
{
    std::string content;
    auto child = m_firstChild;
    while(child) {
        if(auto node = to<TextNode>(child))
            content += node->data();
        child = child->nextSibling();
    }

    return content;
}

void ContainerNode::buildChildrenBox(Counters& counters, Box* parent)
{
    auto child = m_firstChild;
    while(child) {
        child->buildBox(counters, parent);
        child = child->nextSibling();
    }
}

void ContainerNode::finishParsingDocument()
{
    auto child = m_firstChild;
    while(child) {
        child->finishParsingDocument();
        child = child->nextSibling();
    }
}

Element::Element(ClassKind type, Document* document, const GlobalString& namespaceURI, const GlobalString& tagName)
    : ContainerNode(type, document)
    , m_namespaceURI(namespaceURI)
    , m_tagName(tagName)
{
}

const HeapString& Element::lang() const
{
    return getAttribute(langAttr);
}

const Attribute* Element::findAttribute(const GlobalString& name) const
{
    for(const auto& attribute : m_attributes) {
        if(name == attribute.name()) {
            return &attribute;
        }
    }

    return nullptr;
}

const Attribute* Element::findAttributePossiblyIgnoringCase(const GlobalString& name) const
{
    if(m_isCaseSensitive)
        return findAttribute(name);
    for(const auto& attribute : m_attributes) {
        if(equalsIgnoringCase(name, attribute.name())) {
            return &attribute;
        }
    }

    return nullptr;
}

bool Element::hasAttribute(const GlobalString& name) const
{
    for(const auto& attribute : m_attributes) {
        if(name == attribute.name()) {
            return true;
        }
    }

    return false;
}

const HeapString& Element::getAttribute(const GlobalString& name) const
{
    for(const auto& attribute : m_attributes) {
        if(name == attribute.name()) {
            return attribute.value();
        }
    }

    return emptyGlo;
}

Url Element::getUrlAttribute(const GlobalString& name) const
{
    const auto& value = getAttribute(name);
    if(!value.empty())
        return document()->completeUrl(value);
    return Url();
}

void Element::setAttributes(const AttributeList& attributes)
{
    assert(m_attributes.empty());
    for(const auto& attribute : attributes) {
        setAttribute(attribute);
    }
}

void Element::setAttribute(const Attribute& attribute)
{
    setAttribute(attribute.name(), attribute.value());
}

void Element::setAttribute(const GlobalString& name, const HeapString& value)
{
    parseAttribute(name, value);
    for(auto& attribute : m_attributes) {
        if(name == attribute.name()) {
            attribute.setValue(value);
            return;
        }
    }

    m_attributes.emplace_front(name, value);
}

void Element::removeAttribute(const GlobalString& name)
{
    parseAttribute(name, emptyGlo);
    m_attributes.remove_if([&name](const auto& attribute) {
        return name == attribute.name();
    });
}

void Element::parseAttribute(const GlobalString& name, const HeapString& value)
{
    if(name == idAttr) {
        if(!m_id.empty())
            document()->removeElementById(m_id, this);
        if(!value.empty()) {
            document()->addElementById(value, this);
        }

        m_id = value;
    } else if(name == classAttr) {
        m_classNames.clear();
        if(value.empty())
            return;
        size_t begin = 0;
        while(true) {
            while(begin < value.size() && isSpace(value[begin]))
                ++begin;
            if(begin >= value.size())
                break;
            size_t end = begin + 1;
            while(end < value.size() && !isSpace(value[end]))
                ++end;
            m_classNames.push_front(value.substring(begin, end - begin));
            begin = end + 1;
        }
    }
}

CssPropertyList Element::inlineStyle()
{
    const auto& value = getAttribute(styleAttr);
    if(value.empty())
        return CssPropertyList();
    CssParserContext context(this, CssStyleOrigin::Inline, document()->baseUrl());
    CssParser parser(context);
    return parser.parseStyle(value);
}

CssPropertyList Element::presentationAttributeStyle()
{
    std::string output;
    for(const auto& attribute : attributes())
        collectAttributeStyle(output, attribute.name(), attribute.value());
    collectAdditionalAttributeStyle(output);

    if(output.empty())
        return CssPropertyList();
    CssParserContext context(this, CssStyleOrigin::PresentationAttribute, document()->baseUrl());
    CssParser parser(context);
    return parser.parseStyle(output);
}

Element* Element::parentElement() const
{
    return to<Element>(parentNode());
}

Element* Element::firstChildElement() const
{
    for(auto child = firstChild(); child; child = child->nextSibling()) {
        if(auto element = to<Element>(child)) {
            return element;
        }
    }

    return nullptr;
}

Element* Element::lastChildElement() const
{
    for(auto child = lastChild(); child; child = child->previousSibling()) {
        if(auto element = to<Element>(child)) {
            return element;
        }
    }

    return nullptr;
}

Element* Element::previousSiblingElement() const
{
    for(auto sibling = previousSibling(); sibling; sibling = sibling->previousSibling()) {
        if(auto element = to<Element>(sibling)) {
            return element;
        }
    }

    return nullptr;
}

Element* Element::nextSiblingElement() const
{
    for(auto sibling = nextSibling(); sibling; sibling = sibling->nextSibling()) {
        if(auto element = to<Element>(sibling)) {
            return element;
        }
    }

    return nullptr;
}

Node* Element::cloneNode(bool deep)
{
    auto newElement = document()->createElement(m_namespaceURI, m_tagName);
    newElement->setIsCaseSensitive(m_isCaseSensitive);
    newElement->setAttributes(m_attributes);
    if(deep) { cloneChildren(newElement); }
    return newElement;
}

Box* Element::createBox(const RefPtr<BoxStyle>& style)
{
    return Box::create(this, style);
}

void Element::buildBox(Counters& counters, Box* parent)
{
    auto style = document()->styleForElement(this, parent->style());
    if(style == nullptr || style->display() == Display::None)
        return;
    auto box = createBox(style);
    if(box == nullptr)
        return;
    parent->addChild(box);
    buildChildrenBox(counters, box);
}

void Element::finishParsingDocument()
{
    if(m_tagName == aTag && (m_namespaceURI == xhtmlNs || m_namespaceURI == svgNs)) {
        const auto& baseUrl = document()->baseUrl();
        auto completeUrl = getUrlAttribute(hrefAttr);
        auto fragmentName = completeUrl.fragment();
        if(!fragmentName.empty() && baseUrl.value() == completeUrl.base()) {
            auto element = document()->getElementById(fragmentName.substr(1));
            if(element) {
                element->setIsLinkDestination(true);
                setIsLinkSource(true);
            }
        } else {
            setIsLinkSource(!completeUrl.isEmpty());
        }
    }

    ContainerNode::finishParsingDocument();
}

Document::Document(ClassKind type, Book* book, ResourceFetcher* fetcher, Url baseUrl)
    : ContainerNode(type, this)
    , m_book(book)
    , m_customResourceFetcher(fetcher)
    , m_baseUrl(std::move(baseUrl))
    , m_styleSheet(this)
{
}

Document::~Document() = default;

bool Document::isSvgImageDocument() const {
    return !m_book && is<SvgDocument>(*this);
}

BoxView* Document::box() const
{
    return static_cast<BoxView*>(Node::box());
}

float Document::width() const
{
    return box()->layer()->overflowRight();
}

float Document::height() const
{
    return box()->layer()->overflowBottom();
}

float Document::viewportWidth() const
{
    if(m_book)
        return m_book->viewportWidth();
    return 0.f;
}

float Document::viewportHeight() const
{
    if(m_book)
        return m_book->viewportHeight();
    return 0.f;
}

bool Document::setContainerSize(float containerWidth, float containerHeight)
{
    auto width = std::ceil(containerWidth);
    auto height = std::ceil(containerHeight);
    if(width == m_containerWidth && height == m_containerHeight)
        return false;
    m_containerWidth = width;
    m_containerHeight = height;
    return true;
}

TextNode* Document::createTextNode(const std::string_view& value)
{
    return new TextNode(this, createString(value));
}

Element* Document::createElement(const GlobalString& namespaceURI, const GlobalString& tagName)
{
    if(namespaceURI == xhtmlNs) {
        if(tagName == bodyTag)
            return new HtmlBodyElement(this);
        if(tagName == fontTag)
            return new HtmlFontElement(this);
        if(tagName == imgTag)
            return new HtmlImageElement(this);
        if(tagName == hrTag)
            return new HtmlHrElement(this);
        if(tagName == brTag)
            return new HtmlBrElement(this);
        if(tagName == wbrTag)
            return new HtmlWbrElement(this);
        if(tagName == liTag)
            return new HtmlLiElement(this);
        if(tagName == olTag)
            return new HtmlOlElement(this);
        if(tagName == tableTag)
            return new HtmlTableElement(this);
        if(tagName == theadTag || tagName == tbodyTag || tagName == tfootTag)
            return new HtmlTableSectionElement(this, tagName);
        if(tagName == trTag)
            return new HtmlTableRowElement(this);
        if(tagName == colTag || tagName == colgroupTag)
            return new HtmlTableColElement(this, tagName);
        if(tagName == tdTag || tagName == thTag)
            return new HtmlTableCellElement(this, tagName);
        if(tagName == inputTag)
            return new HtmlInputElement(this);
        if(tagName == textareaTag)
            return new HtmlTextAreaElement(this);
        if(tagName == selectTag)
            return new HtmlSelectElement(this);
        if(tagName == styleTag)
            return new HtmlStyleElement(this);
        if(tagName == linkTag)
            return new HtmlLinkElement(this);
        if(tagName == titleTag)
            return new HtmlTitleElement(this);
        if(tagName == baseTag)
            return new HtmlBaseElement(this);
        return new HtmlElement(this, tagName);
    }

    if(namespaceURI == svgNs) {
        if(tagName == svgTag)
            return new SvgSvgElement(this);
        if(tagName == useTag)
            return new SvgUseElement(this);
        if(tagName == imageTag)
            return new SvgImageElement(this);
        if(tagName == symbolTag)
            return new SvgSymbolElement(this);
        if(tagName == aTag)
            return new SvgAElement(this);
        if(tagName == gTag)
            return new SvgGElement(this);
        if(tagName == defsTag)
            return new SvgDefsElement(this);
        if(tagName == lineTag)
            return new SvgLineElement(this);
        if(tagName == rectTag)
            return new SvgRectElement(this);
        if(tagName == circleTag)
            return new SvgCircleElement(this);
        if(tagName == ellipseTag)
            return new SvgEllipseElement(this);
        if(tagName == polylineTag || tagName == polygonTag)
            return new SvgPolyElement(this, tagName);
        if(tagName == pathTag)
            return new SvgPathElement(this);
        if(tagName == tspanTag)
            return new SvgTSpanElement(this);
        if(tagName == textTag)
            return new SvgTextElement(this);
        if(tagName == markerTag)
            return new SvgMarkerElement(this);
        if(tagName == clipPathTag)
            return new SvgClipPathElement(this);
        if(tagName == maskTag)
            return new SvgMaskElement(this);
        if(tagName == patternTag)
            return new SvgPatternElement(this);
        if(tagName == stopTag)
            return new SvgStopElement(this);
        if(tagName == linearGradientTag)
            return new SvgLinearGradientElement(this);
        if(tagName == radialGradientTag)
            return new SvgRadialGradientElement(this);
        if(tagName == styleTag)
            return new SvgStyleElement(this);
        return new SvgElement(this, tagName);
    }

    return new Element(ClassKind::Element, this, namespaceURI, tagName);
}

Element* Document::bodyElement() const
{
    auto element = to<HtmlElement>(m_rootElement);
    if(element && element->tagName() == htmlTag) {
        auto child = element->firstChild();
        while(child) {
            auto element = to<HtmlElement>(child);
            if(element && element->tagName() == bodyTag)
                return element;
            child = child->nextSibling();
        }
    }

    return nullptr;
}

BoxStyle* Document::rootStyle() const
{
    if(m_rootElement) {
        if(auto rootStyle = m_rootElement->style()) {
            return rootStyle;
        }
    }

    return style();
}

BoxStyle* Document::bodyStyle() const
{
    if(auto element = bodyElement())
        return element->style();
    return nullptr;
}

Element* Document::getElementById(const std::string_view& id) const
{
    auto it = m_idCache.find(id);
    if(it == m_idCache.end())
        return nullptr;
    return it->second;
}

void Document::addElementById(const HeapString& id, Element* element)
{
    assert(element && !id.empty());
    m_idCache.emplace(id, element);
}

void Document::removeElementById(const HeapString& id, Element* element)
{
    assert(element && !id.empty());
    auto range = m_idCache.equal_range(id);
    for(auto it = range.first; it != range.second; ++it) {
        if(it->second == element) {
            m_idCache.erase(it);
            break;
        }
    }
}

void Document::addRunningStyle(const GlobalString& name, RefPtr<BoxStyle> style)
{
    assert(style->position() == Position::Running);
    style->setPosition(Position::Static);
    m_runningStyles.emplace(name, std::move(style));
}

RefPtr<BoxStyle> Document::getRunningStyle(const GlobalString& name) const
{
    auto it = m_runningStyles.find(name);
    if(it == m_runningStyles.end())
        return nullptr;
    return it->second;
}

void Document::addTargetCounters(const HeapString& id, const CounterMap& counters)
{
    assert(!id.empty() && !counters.empty());
    m_counterCache.emplace(id, counters);
}

HeapString Document::getTargetCounterText(const HeapString& fragment, const GlobalString& name, const GlobalString& listStyle, const HeapString& separator)
{
    if(fragment.empty() || fragment.front() != '#')
        return emptyGlo;
    auto it = m_counterCache.find(fragment.substring(1));
    if(it == m_counterCache.end())
        return emptyGlo;
    return getCountersText(it->second, name, listStyle, separator);
}

HeapString Document::getCountersText(const CounterMap& counters, const GlobalString& name, const GlobalString& listStyle, const HeapString& separator)
{
    auto it = counters.find(name);
    if(it == counters.end())
        return createString(getCounterText(0, listStyle));
    if(separator.empty()) {
        int value = 0;
        if(!it->second.empty())
            value = it->second.back();
        return createString(getCounterText(value, listStyle));
    }

    std::string text;
    for(auto value : it->second) {
        if(!text.empty())
            text += separator.value();
        text += getCounterText(value, listStyle);
    }

    return createString(text);
}

void Document::runJavaScript(const std::string_view& script)
{
}

void Document::addAuthorStyleSheet(const std::string_view& content, Url baseUrl)
{
    m_styleSheet.parseStyle(content, CssStyleOrigin::Author, std::move(baseUrl));
}

void Document::addUserStyleSheet(const std::string_view& content)
{
    m_styleSheet.parseStyle(content, CssStyleOrigin::User, m_baseUrl);
}

bool Document::supportsMediaFeature(const CssMediaFeature& feature) const
{
    const auto viewportWidth = m_book->viewportWidth();
    const auto viewportHeight = m_book->viewportHeight();
    if(feature.id() == CssPropertyID::Orientation) {
        const auto& orientation = to<CssIdentValue>(*feature.value());
        if(orientation.value() == CssValueID::Portrait)
            return viewportWidth < viewportHeight;
        assert(orientation.value() == CssValueID::Landscape);
        return viewportWidth > viewportHeight;
    }

    const auto value = CssLengthResolver(this, nullptr).resolveLength(*feature.value());
    if(feature.id() == CssPropertyID::Width)
        return viewportWidth == value;
    if(feature.id() == CssPropertyID::MinWidth)
        return viewportWidth >= value;
    if(feature.id() == CssPropertyID::MaxWidth) {
        return viewportWidth <= value;
    }

    if(feature.id() == CssPropertyID::Height)
        return viewportHeight == value;
    if(feature.id() == CssPropertyID::MinHeight)
        return viewportHeight >= value;
    assert(feature.id() == CssPropertyID::MaxHeight);
    return viewportHeight <= value;
}

bool Document::supportsMediaFeatures(const CssMediaFeatureList& features) const
{
    for(const auto& feature : features) {
        if(!supportsMediaFeature(feature)) {
            return false;
        }
    }

    return true;
}

bool Document::supportsMediaQuery(const CssMediaQuery& query) const
{
    if(query.type() == CssMediaQuery::Type::Print && m_book->mediaType() != MediaType::Print)
        return query.restrictor() == CssMediaQuery::Restrictor::Not;
    if(query.type() == CssMediaQuery::Type::Screen && m_book->mediaType() != MediaType::Screen) {
        return query.restrictor() == CssMediaQuery::Restrictor::Not;
    }

    if(supportsMediaFeatures(query.features()))
        return query.restrictor() != CssMediaQuery::Restrictor::Not;
    return query.restrictor() == CssMediaQuery::Restrictor::Not;
}

bool Document::supportsMediaQueries(const CssMediaQueryList& queries) const
{
    if(m_book == nullptr || queries.empty())
        return true;
    for(const auto& query : queries) {
        if(supportsMediaQuery(query)) {
            return true;
        }
    }

    return false;
}

bool Document::supportsMedia(const std::string_view& type, const std::string_view& media) const
{
    if(m_book == nullptr || media.empty())
        return true;
    if(type.empty() || equals(type, "text/css", is<XmlDocument>(*this))) {
        CssParserContext context(this, CssStyleOrigin::Author, m_baseUrl);
        CssParser parser(context);
        CssMediaQueryList queries(parser.parseMediaQueries(media));
        return supportsMediaQueries(queries);
    }

    return false;
}

RefPtr<BoxStyle> Document::styleForElement(Element* element, const BoxStyle* parentStyle) const
{
    return m_styleSheet.styleForElement(element, parentStyle);
}

RefPtr<BoxStyle> Document::pseudoStyleForElement(Element* element, PseudoType pseudoType, const BoxStyle* parentStyle) const
{
    return m_styleSheet.pseudoStyleForElement(element, pseudoType, parentStyle);
}

RefPtr<BoxStyle> Document::styleForPage(const GlobalString& pageName, uint32_t pageIndex, PseudoType pseudoType) const
{
    return m_styleSheet.styleForPage(pageName, pageIndex, pseudoType);
}

RefPtr<BoxStyle> Document::styleForPageMargin(const GlobalString& pageName, uint32_t pageIndex, PageMarginType marginType, const BoxStyle* pageStyle) const
{
    return m_styleSheet.styleForPageMargin(pageName, pageIndex, marginType, pageStyle);
}

std::string Document::getCounterText(int value, const GlobalString& listType)
{
    return m_styleSheet.getCounterText(value, listType);
}

std::string Document::getMarkerText(int value, const GlobalString& listType)
{
    return m_styleSheet.getMarkerText(value, listType);
}

RefPtr<FontData> Document::getFontData(const GlobalString& family, const FontDataDescription& description)
{
    return m_styleSheet.getFontData(family, description);
}

RefPtr<Font> Document::createFont(const FontDescription& description)
{
    auto& font = m_fontCache[description];
    if(font == nullptr)
        font = Font::create(this, description);
    return font;
}

RefPtr<TextResource> Document::fetchTextResource(const Url& url)
{
    return fetchResource<TextResource>(url);
}

RefPtr<ImageResource> Document::fetchImageResource(const Url& url)
{
    return fetchResource<ImageResource>(url);
}

RefPtr<FontResource> Document::fetchFontResource(const Url& url)
{
    return fetchResource<FontResource>(url);
}

Node* Document::cloneNode(bool deep)
{
    return nullptr;
}

Box* Document::createBox(const RefPtr<BoxStyle>& style)
{
    return new BoxView(this, style);
}

void Document::finishParsingDocument()
{
    assert(m_rootElement == nullptr);
    auto child = firstChild();
    while(child) {
        if(m_rootElement == nullptr)
            m_rootElement = to<Element>(child);
        child->finishParsingDocument();
        child = child->nextSibling();
    }
}

void Document::serialize(std::ostream& o) const
{
    o << "<?container width=\'" << m_containerWidth << "\'"
      << " height=\'" << m_containerHeight << "\'?>\n";
    box()->serialize(o, 0);
}

void Document::buildBox(Counters& counters, Box* parent)
{
    auto rootStyle = BoxStyle::create(this, PseudoType::None, Display::Block);
    rootStyle->setPosition(Position::Absolute);
    rootStyle->setFontDescription(FontDescription());

    auto rootBox = createBox(rootStyle);
    counters.push();
    buildChildrenBox(counters, rootBox);
    counters.pop();
    rootBox->build();
}

void Document::build()
{
    Counters counters(this, 0);
    buildBox(counters, nullptr);
}

void Document::layout()
{
    box()->layout(nullptr);
}

void Document::paginate()
{
    PageLayout(this).layout();
}

void Document::render(GraphicsContext& context, const Rect& rect)
{
    box()->paintLayer(context, rect);
}

void Document::renderPage(GraphicsContext& context, uint32_t pageIndex)
{
    if(pageIndex < m_pages.size()) {
        const auto& page = m_pages[pageIndex];
        box()->setCurrentPage(page.get());
        page->paintLayer(context, page->pageRect());
        box()->setCurrentPage(nullptr);
    }
}

PageSize Document::pageSizeAt(uint32_t pageIndex) const
{
    if(pageIndex < m_pages.size())
        return m_pages[pageIndex]->pageSize();
    return PageSize();
}

uint32_t Document::pageCount() const
{
    return m_pages.size();
}

float Document::fragmentHeightForOffset(float offset) const
{
    return m_containerHeight;
}

float Document::fragmentRemainingHeightForOffset(float offset, FragmentBoundaryRule rule) const
{
    offset += fragmentOffset();
    auto remainingHeight = m_containerHeight - std::fmod(offset, m_containerHeight);
    if(rule == AssociateWithFormerFragment)
        remainingHeight = std::fmod(remainingHeight, m_containerHeight);
    return remainingHeight;
}

Rect Document::pageContentRectAt(uint32_t pageIndex) const
{
    return Rect(0, pageIndex * m_containerHeight, m_containerWidth, m_containerHeight);
}

template<typename ResourceType>
RefPtr<ResourceType> Document::fetchResource(const Url& url)
{
    if(url.isEmpty())
        return nullptr;
    auto it = m_resourceCache.find(url);
    if(it != m_resourceCache.end())
        return to<ResourceType>(it->second);
    auto resource = ResourceType::create(this, url);
    if(!url.protocolIs("data"))
        m_resourceCache.emplace(url, resource);
    if(resource == nullptr)
        std::cerr << "WARNING: " << plutobook_get_error_message() << std::endl;
    return resource;
}

} // namespace plutobook
