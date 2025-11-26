#include "html-parser.h"
#include "html-document.h"
#include "string-utils.h"

namespace plutobook {

inline bool isNumberedHeaderTag(const GlobalString& tagName)
{
    using enum GlobalStringId;
    switch (tagName.asId()) {
    case h1Tag:
    case h2Tag:
    case h3Tag:
    case h4Tag:
    case h5Tag:
    case h6Tag:
        return true;
    default:
        return false;
    }
}

inline bool isImpliedEndTag(const GlobalString& tagName)
{
    using enum GlobalStringId;
    switch (tagName.asId()) {
    case ddTag:
    case dtTag:
    case liTag:
    case optionTag:
    case optgroupTag:
    case pTag:
    case rpTag:
    case rtTag:
        return true;
    default:
        return false;
    };
}

inline bool isFosterRedirectingTag(const GlobalString& tagName)
{
    using enum GlobalStringId;
    switch (tagName.asId()) {
    case tableTag:
    case tbodyTag:
    case theadTag:
    case trTag:
        return true;
    default:
        return false;
    };
}

inline bool isNumberedHeaderElement(const Element* element)
{
    return isNumberedHeaderTag(element->tagName());
}

inline bool isSvgTag(const GlobalString& tagName)
{
    using enum GlobalStringId;
    switch (tagName.asId()) {
    case foreignObjectTag:
    case descTag:
    case titleTag:
        return true;
    default:
        return false;
    };
}

inline bool isMathmlTag(const GlobalString& tagName)
{
    using enum GlobalStringId;
    switch (tagName.asId()) {
    case miTag:
    case moTag:
    case mnTag:
    case msTag:
    case mtextTag:
        return true;
    default:
        return false;
    };
}

inline bool isSpecialElement(const Element* element)
{
    const auto& tagName = element->tagName();
    if(element->namespaceURI() == svgNs) {
        return isSvgTag(tagName);
    }

    if(element->namespaceURI() == mathmlNs) {
        return isMathmlTag(tagName) || tagName == annotation_xmlTag;
    }

    using enum GlobalStringId;
    switch (tagName.asId()) {
    case addressTag:
    case appletTag:
    case areaTag:
    case articleTag:
    case asideTag:
    case baseTag:
    case basefontTag:
    case bgsoundTag:
    case blockquoteTag:
    case bodyTag:
    case brTag:
    case buttonTag:
    case captionTag:
    case centerTag:
    case colTag:
    case colgroupTag:
    case commandTag:
    case ddTag:
    case detailsTag:
    case dirTag:
    case divTag:
    case dlTag:
    case dtTag:
    case embedTag:
    case fieldsetTag:
    case figcaptionTag:
    case figureTag:
    case footerTag:
    case formTag:
    case frameTag:
    case framesetTag:
    case headTag:
    case headerTag:
    case hgroupTag:
    case hrTag:
    case htmlTag:
    case iframeTag:
    case imgTag:
    case inputTag:
    case liTag:
    case linkTag:
    case listingTag:
    case mainTag:
    case marqueeTag:
    case menuTag:
    case metaTag:
    case navTag:
    case noembedTag:
    case noframesTag:
    case noscriptTag:
    case objectTag:
    case olTag:
    case pTag:
    case paramTag:
    case plaintextTag:
    case preTag:
    case scriptTag:
    case sectionTag:
    case selectTag:
    case styleTag:
    case summaryTag:
    case tableTag:
    case tbodyTag:
    case tdTag:
    case textareaTag:
    case tfootTag:
    case thTag:
    case theadTag:
    case titleTag:
    case trTag:
    case ulTag:
    case wbrTag:
    case xmpTag:
        return true;
    default:
        return isNumberedHeaderTag(tagName);
    };
}

inline bool isHtmlIntegrationPoint(const Element* element)
{
    if(element->namespaceURI() == mathmlNs
        && element->tagName() == annotation_xmlTag) {
        auto attribute = element->findAttribute(encodingAttr);
        if(attribute == nullptr)
            return false;
        const auto& encoding = attribute->value();
        return equals(encoding, "text/html", false)
            || equals(encoding, "application/xhtml+xml", false);
    }

    if(element->namespaceURI() == svgNs) {
        return isSvgTag(element->tagName());
    }

    return false;
}

inline bool isMathMLTextIntegrationPoint(const Element* element)
{
    if(element->namespaceURI() == mathmlNs) {
        return isMathmlTag(element->tagName());
    }

    return false;
}

inline bool isScopeMarker(const Element* element)
{
    const auto& tagName = element->tagName();
    if(element->namespaceURI() == svgNs) {
        return isSvgTag(tagName);
    }

    if(element->namespaceURI() == mathmlNs) {
        return isMathmlTag(tagName) || tagName == annotation_xmlTag;
    }

    using enum GlobalStringId;
    switch (tagName.asId()) {
    case captionTag:
    case marqueeTag:
    case objectTag:
    case tableTag:
    case tdTag:
    case thTag:
    case htmlTag:
        return true;
    default:
        return false;
    };
}

inline bool isListItemScopeMarker(const Element* element)
{
    return isScopeMarker(element)
        || element->tagName() == olTag
        || element->tagName() == ulTag;
}

inline bool isTableScopeMarker(const Element* element)
{
    return element->tagName() == tableTag
        || element->tagName() == htmlTag;
}

inline bool isTableBodyScopeMarker(const Element* element)
{
    return element->tagName() == tbodyTag
        || element->tagName() == tfootTag
        || element->tagName() == theadTag
        || element->tagName() == htmlTag;
}

inline bool isTableRowScopeMarker(const Element* element)
{
    return element->tagName() == trTag
        || element->tagName() == htmlTag;
}

inline bool isForeignContentScopeMarker(const Element* element)
{
    return isMathMLTextIntegrationPoint(element)
        || isHtmlIntegrationPoint(element)
        || element->namespaceURI() == xhtmlNs;
}

inline bool isButtonScopeMarker(const Element* element)
{
    return isScopeMarker(element)
        || element->tagName() == buttonTag;
}

inline bool isSelectScopeMarker(const Element* element)
{
    return element->tagName() != optgroupTag
        && element->tagName() != optionTag;
}

void HtmlElementList::remove(const Element* element)
{
    remove(index(element));
}

void HtmlElementList::remove(size_t index)
{
    assert(index < m_elements.size());
    m_elements.erase(m_elements.begin() + index);
}

void HtmlElementList::replace(const Element* element, Element* item)
{
    replace(index(element), item);
}

void HtmlElementList::replace(size_t index, Element* element)
{
    m_elements.at(index) = element;
}

void HtmlElementList::insert(size_t index, Element* element)
{
    assert(index <= m_elements.size());
    m_elements.insert(m_elements.begin() + index, element);
}

size_t HtmlElementList::index(const Element* element) const
{
    for(int i = m_elements.size() - 1; i >= 0; --i) {
        if(element == m_elements.at(i)) {
            return i;
        }
    }

    assert(false);
    return 0;
}

bool HtmlElementList::contains(const Element* element) const
{
    auto it = m_elements.rbegin();
    auto end = m_elements.rend();
    for(; it != end; ++it) {
        if(element == *it) {
            return true;
        }
    }

    return false;
}

void HtmlElementStack::push(Element* element)
{
    assert(element->tagName() != htmlTag);
    assert(element->tagName() != headTag);
    assert(element->tagName() != bodyTag);
    m_elements.push_back(element);
}

void HtmlElementStack::pushHtmlHtmlElement(Element* element)
{
    assert(element->tagName() == htmlTag);
    assert(m_htmlElement == nullptr);
    assert(m_elements.empty());
    m_htmlElement = element;
    m_elements.push_back(element);
}

void HtmlElementStack::pushHtmlHeadElement(Element* element)
{
    assert(element->tagName() == headTag);
    assert(m_headElement == nullptr);
    m_headElement = element;
    m_elements.push_back(element);
}

void HtmlElementStack::pushHtmlBodyElement(Element* element)
{
    assert(element->tagName() == bodyTag);
    assert(m_bodyElement == nullptr);
    m_bodyElement = element;
    m_elements.push_back(element);
}

void HtmlElementStack::pop()
{
    auto element = m_elements.back();
    assert(element->tagName() != htmlTag);
    assert(element->tagName() != headTag);
    assert(element->tagName() != bodyTag);
    m_elements.pop_back();
}

void HtmlElementStack::popHtmlHeadElement()
{
    auto element = m_elements.back();
    assert(element == m_headElement);
    m_headElement = nullptr;
    m_elements.pop_back();
}

void HtmlElementStack::popHtmlBodyElement()
{
    auto element = m_elements.back();
    assert(element == m_bodyElement);
    m_bodyElement = nullptr;
    m_elements.pop_back();
}

void HtmlElementStack::popUntil(const GlobalString& tagName)
{
    while(tagName != top()->tagName()) {
        pop();
    }
}

void HtmlElementStack::popUntil(const Element* element)
{
    while(element != top()) {
        pop();
    }
}

void HtmlElementStack::popUntilNumberedHeaderElement()
{
    while(!isNumberedHeaderElement(top())) {
        pop();
    }
}

void HtmlElementStack::popUntilTableScopeMarker()
{
    while(!isTableScopeMarker(top())) {
        pop();
    }
}

void HtmlElementStack::popUntilTableBodyScopeMarker()
{
    while(!isTableBodyScopeMarker(top())) {
        pop();
    }
}

void HtmlElementStack::popUntilTableRowScopeMarker()
{
    while(!isTableRowScopeMarker(top())) {
        pop();
    }
}

void HtmlElementStack::popUntilForeignContentScopeMarker()
{
    while(!isForeignContentScopeMarker(top())) {
        pop();
    }
}

void HtmlElementStack::popUntilPopped(const GlobalString& tagName)
{
    popUntil(tagName);
    pop();
}

void HtmlElementStack::popUntilPopped(const Element* element)
{
    popUntil(element);
    pop();
}

void HtmlElementStack::popUntilNumberedHeaderElementPopped()
{
    popUntilNumberedHeaderElement();
    pop();
}

void HtmlElementStack::popAll()
{
    m_htmlElement = nullptr;
    m_headElement = nullptr;
    m_bodyElement = nullptr;
    m_elements.clear();
}

void HtmlElementStack::generateImpliedEndTags()
{
    while(isImpliedEndTag(top()->tagName())) {
        pop();
    }
}

void HtmlElementStack::generateImpliedEndTagsExcept(const GlobalString& tagName)
{
    while(top()->tagName() != tagName && isImpliedEndTag(top()->tagName())) {
        pop();
    }
}

void HtmlElementStack::removeHtmlHeadElement(const Element* element)
{
    if(element == top())
        return popHtmlHeadElement();
    assert(m_headElement == element);
    m_headElement = nullptr;
    remove(index(element));
}

void HtmlElementStack::removeHtmlBodyElement()
{
    assert(m_htmlElement != nullptr);
    assert(m_bodyElement != nullptr);
    auto element = m_bodyElement;
    m_htmlElement->removeChild(m_bodyElement);
    popUntil(m_bodyElement);
    popHtmlBodyElement();
    assert(m_htmlElement == top());
    delete element;
}

void HtmlElementStack::insertAfter(const Element* element, Element* item)
{
    insert(index(element) + 1, item);
}

Element* HtmlElementStack::furthestBlockForFormattingElement(const Element* formattingElement) const
{
    Element* furthestBlock = nullptr;
    auto it = m_elements.rbegin();
    auto end = m_elements.rend();
    for(; it != end; ++it) {
        if(formattingElement == *it)
            return furthestBlock;
        if(!isSpecialElement(*it))
            continue;
        furthestBlock = *it;
    }

    assert(false);
    return nullptr;
}

Element* HtmlElementStack::topmost(const GlobalString& tagName) const
{
    auto it = m_elements.rbegin();
    auto end = m_elements.rend();
    for(; it != end; ++it) {
        auto element = *it;
        if(tagName == element->tagName()) {
            return element;
        }
    }

    return nullptr;
}

Element* HtmlElementStack::previous(const Element* element) const
{
    return m_elements.at(index(element) - 1);
}

template<bool isMarker(const Element*)>
bool HtmlElementStack::inScopeTemplate(const GlobalString& tagName) const
{
    auto it = m_elements.rbegin();
    auto end = m_elements.rend();
    for(; it != end; ++it) {
        auto element = *it;
        if(element->tagName() == tagName)
            return true;
        if(isMarker(element)) {
            return false;
        }
    }

    assert(false);
    return false;
}

bool HtmlElementStack::inScope(const Element* element) const
{
    auto it = m_elements.rbegin();
    auto end = m_elements.rend();
    for(; it != end; ++it) {
        if(element == *it)
            return true;
        if(isScopeMarker(*it)) {
            return false;
        }
    }

    assert(false);
    return false;
}

bool HtmlElementStack::inScope(const GlobalString& tagName) const
{
    return inScopeTemplate<isScopeMarker>(tagName);
}

bool HtmlElementStack::inButtonScope(const GlobalString& tagName) const
{
    return inScopeTemplate<isButtonScopeMarker>(tagName);
}

bool HtmlElementStack::inListItemScope(const GlobalString& tagName) const
{
    return inScopeTemplate<isListItemScopeMarker>(tagName);
}

bool HtmlElementStack::inTableScope(const GlobalString& tagName) const
{
    return inScopeTemplate<isTableScopeMarker>(tagName);
}

bool HtmlElementStack::inSelectScope(const GlobalString& tagName) const
{
    return inScopeTemplate<isSelectScopeMarker>(tagName);
}

bool HtmlElementStack::isNumberedHeaderElementInScope() const
{
    auto it = m_elements.rbegin();
    auto end = m_elements.rend();
    for(; it != end; ++it) {
        if(isNumberedHeaderElement(*it))
            return true;
        if(isScopeMarker(*it)) {
            return false;
        }
    }

    assert(false);
    return false;
}

void HtmlFormattingElementList::append(Element* element)
{
    assert(element != nullptr);
    auto it = m_elements.rbegin();
    auto end = m_elements.rend();
    int count = 0;
    for(; it != end; ++it) {
        auto item = *it;
        if(item == nullptr)
            break;
        if(element->tagName() == item->tagName()
            && element->namespaceURI() == item->namespaceURI()
            && element->attributes() == item->attributes()) {
            count += 1;
        }

        if(count == 3) {
            remove(*it);
            break;
        }
    }

    m_elements.push_back(element);
}

void HtmlFormattingElementList::appendMarker()
{
    m_elements.push_back(nullptr);
}

void HtmlFormattingElementList::clearToLastMarker()
{
    while(!m_elements.empty()) {
        auto element = m_elements.back();
        m_elements.pop_back();
        if(element == nullptr) {
            break;
        }
    }
}

Element* HtmlFormattingElementList::closestElementInScope(const GlobalString& tagName)
{
    auto it = m_elements.rbegin();
    auto end = m_elements.rend();
    for(; it != end; ++it) {
        auto element = *it;
        if(element == nullptr)
            break;
        if(element->tagName() == tagName) {
            return element;
        }
    }

    return nullptr;
}

HtmlParser::HtmlParser(HtmlDocument* document, const std::string_view& content)
    : m_document(document), m_tokenizer(content)
{
}

bool HtmlParser::parse()
{
    while(!m_tokenizer.atEOF()) {
        auto token = m_tokenizer.nextToken();
        if(token.type() == HtmlToken::Type::DOCTYPE) {
            handleDoctypeToken(token);
            continue;
        }

        if(token.type() == HtmlToken::Type::Comment) {
            handleCommentToken(token);
            continue;
        }

        if(m_skipLeadingNewline
            && token.type() == HtmlToken::Type::SpaceCharacter) {
            token.skipLeadingNewLine();
        }

        m_skipLeadingNewline = false;
        handleToken(token, currentInsertionMode(token));
    }

    assert(!m_openElements.empty());
    m_openElements.popAll();
    m_document->finishParsingDocument();
    return true;
}

Element* HtmlParser::createHtmlElement(const HtmlTokenView& token) const
{
    return createElement(token, xhtmlNs);
}

Element* HtmlParser::createElement(const HtmlTokenView& token, const GlobalString& namespaceURI) const
{
    auto element = m_document->createElement(namespaceURI, token.tagName());
    element->setIsCaseSensitive(!token.hasCamelCase());
    for(const auto& attribute : token.attributes())
        element->setAttribute(attribute);
    return element;
}

Element* HtmlParser::cloneElement(const Element* element) const
{
    auto newElement = m_document->createElement(element->namespaceURI(), element->tagName());
    newElement->setIsCaseSensitive(element->isCaseSensitive());
    newElement->setAttributes(element->attributes());
    return newElement;
}

void HtmlParser::insertNode(const InsertionLocation& location, Node* child)
{
    if(location.nextChild) {
        location.parent->insertChild(child, location.nextChild);
    } else {
        location.parent->appendChild(child);
    }
}

void HtmlParser::insertElement(Element* child, ContainerNode* parent)
{
    InsertionLocation location;
    location.parent = parent;
    if(shouldFosterParent())
        findFosterLocation(location);
    insertNode(location, child);
}

void HtmlParser::insertElement(Element* child)
{
    insertElement(child, currentElement());
}

bool HtmlParser::shouldFosterParent() const
{
    return m_fosterRedirecting && isFosterRedirectingTag(currentElement()->tagName());
}

void HtmlParser::findFosterLocation(InsertionLocation& location) const
{
    auto lastTable = m_openElements.topmost(tableTag);
    assert(lastTable && lastTable->parentNode());
    location.parent = lastTable->parentNode();
    location.nextChild = lastTable;
}

void HtmlParser::fosterParent(Node* child)
{
    InsertionLocation location;
    findFosterLocation(location);
    insertNode(location, child);
}

void HtmlParser::reconstructActiveFormattingElements()
{
    if(m_activeFormattingElements.empty())
        return;
    auto index = m_activeFormattingElements.size();
    do {
        index -= 1;
        auto element = m_activeFormattingElements.at(index);
        if(element == nullptr || m_openElements.contains(element)) {
            index += 1;
            break;
        }
    } while(index > 0);
    for(; index < m_activeFormattingElements.size(); ++index) {
        auto element = m_activeFormattingElements.at(index);
        auto newElement = cloneElement(element);
        insertElement(newElement);
        m_openElements.push(newElement);
        m_activeFormattingElements.replace(index, newElement);
    }
}

void HtmlParser::flushPendingTableCharacters()
{
    for(auto cc : m_pendingTableCharacters) {
        if(isSpace(cc))
            continue;
        reconstructActiveFormattingElements();
        m_fosterRedirecting = true;
        insertTextNode(m_pendingTableCharacters);
        m_fosterRedirecting = false;
        m_framesetOk = false;
        m_insertionMode = m_originalInsertionMode;
        return;
    }

    insertTextNode(m_pendingTableCharacters);
    m_insertionMode = m_originalInsertionMode;
}

void HtmlParser::closeTheCell()
{
    if(m_openElements.inTableScope(tdTag)) {
        assert(!m_openElements.inTableScope(thTag));
        handleFakeEndTagToken(tdTag);
        return;
    }

    assert(m_openElements.inTableScope(thTag));
    handleFakeEndTagToken(thTag);
}

void HtmlParser::adjustSvgTagNames(HtmlTokenView& token)
{
    static const boost::unordered_flat_map<GlobalString, GlobalString> table = {
        {"altglyph"_glo, "altGlyph"_glo},
        {"altglyphdef"_glo, "altGlyphDef"_glo},
        {"altglyphitem"_glo, "altGlyphItem"_glo},
        {"animatecolor"_glo, "animateColor"_glo},
        {"animatemotion"_glo, "animateMotion"_glo},
        {"animatetransform"_glo, "animateTransform"_glo},
        {"clippath"_glo, "clipPath"_glo},
        {"feblend"_glo, "feBlend"_glo},
        {"fecolormatrix"_glo, "feColorMatrix"_glo},
        {"fecomponenttransfer"_glo, "feComponentTransfer"_glo},
        {"fecomposite"_glo, "feComposite"_glo},
        {"feconvolvematrix"_glo, "feConvolveMatrix"_glo},
        {"fediffuselighting"_glo, "feDiffuseLighting"_glo},
        {"fedisplacementmap"_glo, "feDisplacementMap"_glo},
        {"fedistantlight"_glo, "feDistantLight"_glo},
        {"fedropshadow"_glo, "feDropShadow"_glo},
        {"feflood"_glo, "feFlood"_glo},
        {"fefunca"_glo, "feFuncA"_glo},
        {"fefuncb"_glo, "feFuncB"_glo},
        {"fefuncg"_glo, "feFuncG"_glo},
        {"fefuncr"_glo, "feFuncR"_glo},
        {"fegaussianblur"_glo, "feGaussianBlur"_glo},
        {"feimage"_glo, "feImage"_glo},
        {"femerge"_glo, "feMerge"_glo},
        {"femergenode"_glo, "feMergeNode"_glo},
        {"femorphology"_glo, "feMorphology"_glo},
        {"feoffset"_glo, "feOffset"_glo},
        {"fepointlight"_glo, "fePointLight"_glo},
        {"fespecularlighting"_glo, "feSpecularLighting"_glo},
        {"fespotlight"_glo, "feSpotLight"_glo},
        {"glyphref"_glo, "glyphRef"_glo},
        {"lineargradient"_glo, "linearGradient"_glo},
        {"radialgradient"_glo, "radialGradient"_glo},
        {"textpath"_glo, "textPath"_glo}
    };

    auto it = table.find(token.tagName());
    if(it != table.end()) {
        token.adjustTagName(it->second);
        token.setHasCamelCase(true);
    }
}

void HtmlParser::adjustSvgAttributes(HtmlTokenView& token)
{
    static const boost::unordered_flat_map<GlobalString, GlobalString> table = {
        {"attributename"_glo, "attributeName"_glo},
        {"attributetype"_glo, "attributeType"_glo},
        {"basefrequency"_glo, "baseFrequency"_glo},
        {"baseprofile"_glo, "baseProfile"_glo},
        {"calcmode"_glo, "calcMode"_glo},
        {"clippathunits"_glo, "clipPathUnits"_glo},
        {"diffuseconstant"_glo, "diffuseConstant"_glo},
        {"edgemode"_glo, "edgeMode"_glo},
        {"filterunits"_glo, "filterUnits"_glo},
        {"glyphref"_glo, "glyphRef"_glo},
        {"gradienttransform"_glo, "gradientTransform"_glo},
        {"gradientunits"_glo, "gradientUnits"_glo},
        {"kernelmatrix"_glo, "kernelMatrix"_glo},
        {"kernelunitlength"_glo, "kernelUnitLength"_glo},
        {"keypoints"_glo, "keyPoints"_glo},
        {"keysplines"_glo, "keySplines"_glo},
        {"keytimes"_glo, "keyTimes"_glo},
        {"lengthadjust"_glo, "lengthAdjust"_glo},
        {"limitingconeangle"_glo, "limitingConeAngle"_glo},
        {"markerheight"_glo, "markerHeight"_glo},
        {"markerunits"_glo, "markerUnits"_glo},
        {"markerwidth"_glo, "markerWidth"_glo},
        {"maskcontentunits"_glo, "maskContentUnits"_glo},
        {"maskunits"_glo, "maskUnits"_glo},
        {"numoctaves"_glo, "numOctaves"_glo},
        {"pathlength"_glo, "pathLength"_glo},
        {"patterncontentunits"_glo, "patternContentUnits"_glo},
        {"patterntransform"_glo, "patternTransform"_glo},
        {"patternunits"_glo, "patternUnits"_glo},
        {"pointsatx"_glo, "pointsAtX"_glo},
        {"pointsaty"_glo, "pointsAtY"_glo},
        {"pointsatz"_glo, "pointsAtZ"_glo},
        {"preservealpha"_glo, "preserveAlpha"_glo},
        {"preserveaspectratio"_glo, "preserveAspectRatio"_glo},
        {"primitiveunits"_glo, "primitiveUnits"_glo},
        {"refx"_glo, "refX"_glo},
        {"refy"_glo, "refY"_glo},
        {"repeatcount"_glo, "repeatCount"_glo},
        {"repeatdur"_glo, "repeatDur"_glo},
        {"requiredextensions"_glo, "requiredExtensions"_glo},
        {"requiredfeatures"_glo, "requiredFeatures"_glo},
        {"specularconstant"_glo, "specularConstant"_glo},
        {"specularexponent"_glo, "specularExponent"_glo},
        {"spreadmethod"_glo, "spreadMethod"_glo},
        {"startoffset"_glo, "startOffset"_glo},
        {"stddeviation"_glo, "stdDeviation"_glo},
        {"stitchtiles"_glo, "stitchTiles"_glo},
        {"surfacescale"_glo, "surfaceScale"_glo},
        {"systemlanguage"_glo, "systemLanguage"_glo},
        {"tablevalues"_glo, "tableValues"_glo},
        {"targetx"_glo, "targetX"_glo},
        {"targety"_glo, "targetY"_glo},
        {"textlength"_glo, "textLength"_glo},
        {"viewbox"_glo, "viewBox"_glo},
        {"viewtarget"_glo, "viewTarget"_glo},
        {"xchannelselector"_glo, "xChannelSelector"_glo},
        {"ychannelselector"_glo, "yChannelSelector"_glo},
        {"zoomandpan"_glo, "zoomAndPan"_glo}
    };

    for(auto& attribute : token.attributes()) {
        auto it = table.find(attribute.name());
        if(it != table.end()) {
            attribute.setName(it->second);
            token.setHasCamelCase(true);
        }
    }
}

void HtmlParser::adjustMathMLAttributes(HtmlTokenView& token)
{
    static const GlobalString definitionurl("definitionurl");
    for(auto& attribute : token.attributes()) {
        if(definitionurl == attribute.name()) {
            static const GlobalString definitionUrlAttr("definitionUrl");
            attribute.setName(definitionUrlAttr);
            token.setHasCamelCase(true);
        }
    }
}

void HtmlParser::adjustForeignAttributes(HtmlTokenView& token)
{
    static const GlobalString xlinkhref("xlink:href");
    for(auto& attribute : token.attributes()) {
        if(xlinkhref == attribute.name()) {
            attribute.setName(hrefAttr);
        }
    }
}

void HtmlParser::insertDoctype(const HtmlTokenView& token)
{
}

void HtmlParser::insertComment(const HtmlTokenView& token, ContainerNode* parent)
{
}

void HtmlParser::insertHtmlHtmlElement(const HtmlTokenView& token)
{
    auto element = createHtmlElement(token);
    insertElement(element, m_document);
    m_openElements.pushHtmlHtmlElement(element);
}

void HtmlParser::insertHeadElement(const HtmlTokenView& token)
{
    auto element = createHtmlElement(token);
    insertElement(element);
    m_openElements.pushHtmlHeadElement(element);
    m_head = element;
}

void HtmlParser::insertHtmlBodyElement(const HtmlTokenView& token)
{
    auto element = createHtmlElement(token);
    insertElement(element);
    m_openElements.pushHtmlBodyElement(element);
}

void HtmlParser::insertHtmlFormElement(const HtmlTokenView& token)
{
    auto element = createHtmlElement(token);
    insertElement(element);
    m_openElements.push(element);
    m_form = element;
}

void HtmlParser::insertSelfClosingHtmlElement(const HtmlTokenView& token)
{
    insertElement(createHtmlElement(token));
}

void HtmlParser::insertHtmlElement(const HtmlTokenView& token)
{
    auto element = createHtmlElement(token);
    insertElement(element);
    m_openElements.push(element);
}

void HtmlParser::insertHtmlFormattingElement(const HtmlTokenView& token)
{
    auto element = createHtmlElement(token);
    insertElement(element);
    m_openElements.push(element);
    m_activeFormattingElements.append(element);
}

void HtmlParser::insertForeignElement(const HtmlTokenView& token, const GlobalString& namespaceURI)
{
    auto element = createElement(token, namespaceURI);
    insertElement(element);
    if(!token.selfClosing()) {
        m_openElements.push(element);
    }
}

void HtmlParser::insertTextNode(const std::string_view& data)
{
    InsertionLocation location;
    location.parent = m_openElements.top();
    if(shouldFosterParent())
        findFosterLocation(location);
    Node* previousChild;
    if(location.nextChild)
        previousChild = location.nextChild->previousSibling();
    else
        previousChild = location.parent->lastChild();
    if(auto previousText = to<TextNode>(previousChild)) {
        previousText->appendData(data);
        return;
    }

    insertNode(location, m_document->createTextNode(data));
}

void HtmlParser::resetInsertionModeAppropriately()
{
    for(int i = m_openElements.size() - 1; i >= 0; --i) {
        auto element = m_openElements.at(i);
        if(element->tagName() == selectTag) {
            for(int j = i; j > 0; --j) {
                auto ancestor = m_openElements.at(j - 1);
                if(ancestor->tagName() == tableTag) {
                    m_insertionMode = InsertionMode::InSelectInTable;
                    return;
                }
            }

            m_insertionMode = InsertionMode::InSelect;
            return;
        }

        if(element->tagName() == tdTag
            || element->tagName() == thTag) {
            m_insertionMode = InsertionMode::InCell;
            return;
        }

        if(element->tagName() == trTag) {
            m_insertionMode = InsertionMode::InRow;
            return;
        }

        if(element->tagName() == tbodyTag
            || element->tagName() == theadTag
            || element->tagName() == tfootTag) {
            m_insertionMode = InsertionMode::InTableBody;
            return;
        }

        if(element->tagName() == captionTag) {
            m_insertionMode = InsertionMode::InCaption;
            return;
        }

        if(element->tagName() == colgroupTag) {
            m_insertionMode = InsertionMode::InColumnGroup;
            return;
        }

        if(element->tagName() == tableTag) {
            m_insertionMode = InsertionMode::InTable;
            return;
        }

        if(element->tagName() == headTag
            || element->tagName() == bodyTag) {
            m_insertionMode = InsertionMode::InBody;
            return;
        }

        if(element->tagName() == framesetTag) {
            m_insertionMode = InsertionMode::InFrameset;
            return;
        }

        if(element->tagName() == htmlTag) {
            assert(m_head != nullptr);
            m_insertionMode = InsertionMode::AfterHead;
            return;
        }
    }
}

HtmlParser::InsertionMode HtmlParser::currentInsertionMode(const HtmlTokenView& token) const
{
    if(m_openElements.empty())
        return m_insertionMode;
    auto element = m_openElements.top();
    if(element->namespaceURI() == xhtmlNs)
        return m_insertionMode;
    if(isMathMLTextIntegrationPoint(element)) {
        if(token.type() == HtmlToken::Type::StartTag
            && token.tagName() != mglyphTag
            && token.tagName() != malignmarkTag)
            return m_insertionMode;
        if(token.type() == HtmlToken::Type::Character
            || token.type() == HtmlToken::Type::SpaceCharacter) {
            return m_insertionMode;
        }
    }

    if(element->namespaceURI() == mathmlNs
        && element->tagName() == annotation_xmlTag
        && token.type() == HtmlToken::Type::StartTag
        && token.tagName() == svgTag) {
        return m_insertionMode;
    }

    if(isHtmlIntegrationPoint(element)) {
        if(token.type() == HtmlToken::Type::StartTag)
            return m_insertionMode;
        if(token.type() == HtmlToken::Type::Character
            || token.type() == HtmlToken::Type::SpaceCharacter) {
            return m_insertionMode;
        }
    }

    if(token.type() == HtmlToken::Type::EndOfFile)
        return m_insertionMode;
    return InsertionMode::InForeignContent;
}

void HtmlParser::handleInitialMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::SpaceCharacter) {
        return;
    }

    handleErrorToken(token);
    m_inQuirksMode = true;
    m_insertionMode = InsertionMode::BeforeHtml;
    handleToken(token);
}

void HtmlParser::handleBeforeHtmlMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == htmlTag) {
            insertHtmlHtmlElement(token);
            m_insertionMode = InsertionMode::BeforeHead;
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() != headTag
            || token.tagName() != bodyTag
            || token.tagName() != htmlTag
            || token.tagName() != brTag) {
            handleErrorToken(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::SpaceCharacter) {
        return;
    }

    handleFakeStartTagToken(htmlTag);
    handleToken(token);
}

void HtmlParser::handleBeforeHeadMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == htmlTag) {
            handleInBodyMode(token);
            return;
        }

        if(token.tagName() == headTag) {
            insertHeadElement(token);
            m_insertionMode = InsertionMode::InHead;
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() != headTag
            || token.tagName() != bodyTag
            || token.tagName() != htmlTag
            || token.tagName() != brTag) {
            handleErrorToken(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::SpaceCharacter) {
        return;
    }

    handleFakeStartTagToken(headTag);
    handleToken(token);
}

void HtmlParser::handleInHeadMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == htmlTag) {
            handleInBodyMode(token);
            return;
        }

        if(token.tagName() == baseTag
            || token.tagName() == basefontTag
            || token.tagName() == bgsoundTag
            || token.tagName() == commandTag
            || token.tagName() == linkTag
            || token.tagName() == metaTag) {
            insertSelfClosingHtmlElement(token);
            return;
        }

        if(token.tagName() == titleTag) {
            handleRCDataToken(token);
            return;
        }

        if(token.tagName() == noscriptTag) {
            insertHtmlElement(token);
            m_insertionMode = InsertionMode::InHeadNoscript;
            return;
        }

        if(token.tagName() == noframesTag
            || token.tagName() == styleTag) {
            handleRawTextToken(token);
            return;
        }

        if(token.tagName() == scriptTag) {
            handleScriptDataToken(token);
            return;
        }

        if(token.tagName() == headTag) {
            handleErrorToken(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() == headTag) {
            m_openElements.popHtmlHeadElement();
            m_insertionMode = InsertionMode::AfterHead;
            return;
        }

        if(token.tagName() != bodyTag
            || token.tagName() != htmlTag
            || token.tagName() != brTag) {
            handleErrorToken(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::SpaceCharacter) {
        insertTextNode(token.data());
        return;
    }

    handleFakeEndTagToken(headTag);
    handleToken(token);
}

void HtmlParser::handleInHeadNoscriptMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == htmlTag) {
            handleInBodyMode(token);
            return;
        }

        if(token.tagName() == basefontTag
            || token.tagName() == bgsoundTag
            || token.tagName() == linkTag
            || token.tagName() == metaTag
            || token.tagName() == noframesTag
            || token.tagName() == styleTag) {
            handleInHeadMode(token);
            return;
        }

        if(token.tagName() == headTag
            || token.tagName() == noscriptTag) {
            handleErrorToken(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() == noscriptTag) {
            assert(currentElement()->tagName() == noscriptTag);
            m_openElements.pop();
            assert(currentElement()->tagName() == headTag);
            m_insertionMode = InsertionMode::InHead;
            return;
        }

        if(token.tagName() != brTag) {
            handleErrorToken(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::SpaceCharacter) {
        handleInHeadMode(token);
        return;
    }

    handleErrorToken(token);
    handleFakeEndTagToken(noscriptTag);
    handleToken(token);
}

void HtmlParser::handleAfterHeadMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == htmlTag) {
            handleInBodyMode(token);
            return;
        }

        if(token.tagName() == bodyTag) {
            insertHtmlBodyElement(token);
            m_framesetOk = false;
            m_insertionMode = InsertionMode::InBody;
            return;
        }

        if(token.tagName() == framesetTag) {
            insertHtmlElement(token);
            m_insertionMode = InsertionMode::InFrameset;
            return;
        }

        if(token.tagName() == baseTag
            || token.tagName() == basefontTag
            || token.tagName() == bgsoundTag
            || token.tagName() == linkTag
            || token.tagName() == metaTag
            || token.tagName() == noframesTag
            || token.tagName() == scriptTag
            || token.tagName() == styleTag
            || token.tagName() == titleTag) {
            handleErrorToken(token);
            m_openElements.pushHtmlHeadElement(m_head);
            handleInHeadMode(token);
            m_openElements.removeHtmlHeadElement(m_head);
            return;
        }

        if(token.tagName() == headTag) {
            handleErrorToken(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() != bodyTag
            || token.tagName() != htmlTag
            || token.tagName() != brTag) {
            handleErrorToken(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::SpaceCharacter) {
        insertTextNode(token.data());
        return;
    }

    handleFakeStartTagToken(bodyTag);
    m_framesetOk = true;
    handleToken(token);
}

void HtmlParser::handleInBodyMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == htmlTag) {
            handleErrorToken(token);
            auto element = m_openElements.htmlElement();
            for(const auto& attribute : token.attributes()) {
                if(element->hasAttribute(attribute.name()))
                    continue;
                element->setAttribute(attribute);
            }

            return;
        }

        if(token.tagName() == baseTag
            || token.tagName() == basefontTag
            || token.tagName() == bgsoundTag
            || token.tagName() == commandTag
            || token.tagName() == linkTag
            || token.tagName() == metaTag
            || token.tagName() == noframesTag
            || token.tagName() == scriptTag
            || token.tagName() == styleTag
            || token.tagName() == titleTag) {
            handleInHeadMode(token);
            return;
        }

        if(token.tagName() == bodyTag) {
            handleErrorToken(token);
            m_framesetOk = false;
            auto element = m_openElements.bodyElement();
            for(const auto& attribute : token.attributes()) {
                if(element->hasAttribute(attribute.name()))
                    continue;
                element->setAttribute(attribute);
            }

            return;
        }

        if(token.tagName() == framesetTag) {
            handleErrorToken(token);
            if(!m_framesetOk)
                return;
            m_openElements.removeHtmlBodyElement();
            insertHtmlElement(token);
            m_insertionMode = InsertionMode::InFrameset;
            return;
        }

        if(token.tagName() == addressTag
            || token.tagName() == articleTag
            || token.tagName() == asideTag
            || token.tagName() == blockquoteTag
            || token.tagName() == centerTag
            || token.tagName() == detailsTag
            || token.tagName() == dirTag
            || token.tagName() == divTag
            || token.tagName() == dlTag
            || token.tagName() == fieldsetTag
            || token.tagName() == figcaptionTag
            || token.tagName() == figureTag
            || token.tagName() == footerTag
            || token.tagName() == headerTag
            || token.tagName() == hgroupTag
            || token.tagName() == mainTag
            || token.tagName() == menuTag
            || token.tagName() == navTag
            || token.tagName() == olTag
            || token.tagName() == pTag
            || token.tagName() == sectionTag
            || token.tagName() == summaryTag
            || token.tagName() == ulTag) {
            if(m_openElements.inButtonScope(pTag))
                handleFakeEndTagToken(pTag);
            insertHtmlElement(token);
            return;
        }

        if(isNumberedHeaderTag(token.tagName())) {
            if(m_openElements.inButtonScope(pTag))
                handleFakeEndTagToken(pTag);
            if(isNumberedHeaderElement(currentElement())) {
                handleErrorToken(token);
                m_openElements.pop();
            }

            insertHtmlElement(token);
            return;
        }

        if(token.tagName() == preTag
            || token.tagName() == listingTag) {
            if(m_openElements.inButtonScope(pTag))
                handleFakeEndTagToken(pTag);
            insertHtmlElement(token);
            m_skipLeadingNewline = true;
            m_framesetOk = false;
            return;
        }

        if(token.tagName() == formTag) {
            if(m_form != nullptr) {
                handleErrorToken(token);
                return;
            }

            if(m_openElements.inButtonScope(pTag))
                handleFakeEndTagToken(pTag);
            insertHtmlFormElement(token);
            return;
        }

        if(token.tagName() == liTag) {
            m_framesetOk = false;
            for(int i = m_openElements.size() - 1; i >= 0; --i) {
                auto element = m_openElements.at(i);
                if(element->tagName() == liTag) {
                    handleFakeEndTagToken(liTag);
                    break;
                }

                if(isSpecialElement(element)
                    && element->tagName() != addressTag
                    && element->tagName() != divTag
                    && element->tagName() != pTag) {
                    break;
                }
            }

            if(m_openElements.inButtonScope(pTag))
                handleFakeEndTagToken(pTag);
            insertHtmlElement(token);
            return;
        }

        if(token.tagName() == ddTag
            || token.tagName() == dtTag) {
            m_framesetOk = false;
            for(int i = m_openElements.size() - 1; i >= 0; --i) {
                auto element = m_openElements.at(i);
                if(element->tagName() == ddTag
                    || element->tagName() == dtTag) {
                    handleFakeEndTagToken(element->tagName());
                    break;
                }

                if(isSpecialElement(element)
                    && element->tagName() != addressTag
                    && element->tagName() != divTag
                    && element->tagName() != pTag) {
                    break;
                }
            }

            if(m_openElements.inButtonScope(pTag))
                handleFakeEndTagToken(pTag);
            insertHtmlElement(token);
            return;
        }

        if(token.tagName() == plaintextTag) {
            if(m_openElements.inButtonScope(pTag))
                handleFakeEndTagToken(pTag);
            insertHtmlElement(token);
            m_tokenizer.setState(HtmlTokenizer::State::PLAINTEXT);
            return;
        }

        if(token.tagName() == buttonTag) {
            if(m_openElements.inScope(buttonTag)) {
                handleErrorToken(token);
                handleFakeEndTagToken(buttonTag);
                handleToken(token);
                return;
            }

            reconstructActiveFormattingElements();
            insertHtmlElement(token);
            m_framesetOk = false;
            return;
        }

        if(token.tagName() == aTag) {
            if(auto aElement = m_activeFormattingElements.closestElementInScope(aTag)) {
                handleErrorToken(token);
                handleFakeEndTagToken(aTag);
                if(m_activeFormattingElements.contains(aElement))
                    m_activeFormattingElements.remove(aElement);
                if(m_openElements.contains(aElement)) {
                    m_openElements.remove(aElement);
                }
            }

            reconstructActiveFormattingElements();
            insertHtmlFormattingElement(token);
            return;
        }

        if(token.tagName() == bTag
            || token.tagName() == bigTag
            || token.tagName() == codeTag
            || token.tagName() == emTag
            || token.tagName() == fontTag
            || token.tagName() == iTag
            || token.tagName() == sTag
            || token.tagName() == smallTag
            || token.tagName() == strikeTag
            || token.tagName() == strongTag
            || token.tagName() == ttTag
            || token.tagName() == uTag) {
            reconstructActiveFormattingElements();
            insertHtmlFormattingElement(token);
            return;
        }

        if(token.tagName() == nobrTag) {
            reconstructActiveFormattingElements();
            if(m_openElements.inScope(nobrTag)) {
                handleErrorToken(token);
                handleFakeEndTagToken(nobrTag);
                reconstructActiveFormattingElements();
            }

            insertHtmlFormattingElement(token);
            return;
        }

        if(token.tagName() == appletTag
            || token.tagName() == marqueeTag
            || token.tagName() == objectTag) {
            reconstructActiveFormattingElements();
            insertHtmlElement(token);
            m_activeFormattingElements.appendMarker();
            m_framesetOk = false;
            return;
        }

        if(token.tagName() == tableTag) {
            if(!m_inQuirksMode && m_openElements.inButtonScope(pTag))
                handleFakeEndTagToken(pTag);
            insertHtmlElement(token);
            m_framesetOk = false;
            m_insertionMode = InsertionMode::InTable;
            return;
        }

        if(token.tagName() == areaTag
            || token.tagName() == brTag
            || token.tagName() == embedTag
            || token.tagName() == imgTag
            || token.tagName() == keygenTag
            || token.tagName() == wbrTag) {
            reconstructActiveFormattingElements();
            insertSelfClosingHtmlElement(token);
            m_framesetOk = false;
            return;
        }

        if(token.tagName() == inputTag) {
            reconstructActiveFormattingElements();
            insertSelfClosingHtmlElement(token);
            auto typeAttribute = token.findAttribute(typeAttr);
            if(typeAttribute == nullptr || !equals(typeAttribute->value(), "hidden", false))
                m_framesetOk = false;
            return;
        }

        if(token.tagName() == paramTag
            || token.tagName() == sourceTag
            || token.tagName() == trackTag) {
            insertSelfClosingHtmlElement(token);
            return;
        }

        if(token.tagName() == hrTag) {
            if(m_openElements.inButtonScope(pTag))
                handleFakeEndTagToken(pTag);
            insertSelfClosingHtmlElement(token);
            m_framesetOk = false;
            return;
        }

        if(token.tagName() == imageTag) {
            handleErrorToken(token);
            token.adjustTagName(imgTag);
            handleToken(token);
            return;
        }

        if(token.tagName() == textareaTag) {
            insertHtmlElement(token);
            m_skipLeadingNewline = true;
            m_tokenizer.setState(HtmlTokenizer::State::RCDATA);
            m_originalInsertionMode = m_insertionMode;
            m_framesetOk = false;
            m_insertionMode = InsertionMode::Text;
            return;
        }

        if(token.tagName() == xmpTag) {
            if(m_openElements.inButtonScope(pTag))
                handleFakeEndTagToken(pTag);
            reconstructActiveFormattingElements();
            m_framesetOk = false;
            handleRawTextToken(token);
            return;
        }

        if(token.tagName() == iframeTag) {
            m_framesetOk = false;
            handleRawTextToken(token);
            return;
        }

        if(token.tagName() == noembedTag) {
            handleRawTextToken(token);
            return;
        }

        if(token.tagName() == selectTag) {
            reconstructActiveFormattingElements();
            insertHtmlElement(token);
            m_framesetOk = false;
            if(m_insertionMode == InsertionMode::InTable
                || m_insertionMode == InsertionMode::InCaption
                || m_insertionMode == InsertionMode::InColumnGroup
                || m_insertionMode == InsertionMode::InTableBody
                || m_insertionMode == InsertionMode::InRow
                || m_insertionMode == InsertionMode::InCell) {
                m_insertionMode = InsertionMode::InSelectInTable;
            } else {
                m_insertionMode = InsertionMode::InSelect;
            }

            return;
        }

        if(token.tagName() == optgroupTag
            || token.tagName() == optionTag) {
            if(currentElement()->tagName() == optionTag) {
                handleFakeEndTagToken(optionTag);
            }

            reconstructActiveFormattingElements();
            insertHtmlElement(token);
            return;
        }

        if(token.tagName() == rpTag
            || token.tagName() == rtTag) {
            if(m_openElements.inScope(rubyTag)) {
                m_openElements.generateImpliedEndTags();
                if(currentElement()->tagName() != rubyTag) {
                    handleErrorToken(token);
                }
            }

            insertHtmlElement(token);
            return;
        }

        if(token.tagName() == mathTag) {
            reconstructActiveFormattingElements();
            adjustMathMLAttributes(token);
            adjustForeignAttributes(token);
            insertForeignElement(token, mathmlNs);
            return;
        }

        if(token.tagName() == svgTag) {
            reconstructActiveFormattingElements();
            adjustSvgAttributes(token);
            adjustForeignAttributes(token);
            insertForeignElement(token, svgNs);
            return;
        }

        if(token.tagName() == captionTag
            || token.tagName() == colTag
            || token.tagName() == colgroupTag
            || token.tagName() == frameTag
            || token.tagName() == headTag
            || token.tagName() == tbodyTag
            || token.tagName() == tdTag
            || token.tagName() == tfootTag
            || token.tagName() == thTag
            || token.tagName() == theadTag
            || token.tagName() == trTag) {
            handleErrorToken(token);
            return;
        }

        reconstructActiveFormattingElements();
        insertHtmlElement(token);
        return;
    }

    if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() == bodyTag) {
            if(!m_openElements.inScope(bodyTag)) {
                handleErrorToken(token);
                return;
            }

            m_insertionMode = InsertionMode::AfterBody;
            return;
        }

        if(token.tagName() == htmlTag) {
            if(!m_openElements.inScope(bodyTag))
                return;
            handleFakeEndTagToken(bodyTag);
            handleToken(token);
            return;
        }

        if(token.tagName() == addressTag
            || token.tagName() == articleTag
            || token.tagName() == asideTag
            || token.tagName() == blockquoteTag
            || token.tagName() == buttonTag
            || token.tagName() == centerTag
            || token.tagName() == detailsTag
            || token.tagName() == dirTag
            || token.tagName() == divTag
            || token.tagName() == dlTag
            || token.tagName() == fieldsetTag
            || token.tagName() == figcaptionTag
            || token.tagName() == figureTag
            || token.tagName() == footerTag
            || token.tagName() == headerTag
            || token.tagName() == hgroupTag
            || token.tagName() == listingTag
            || token.tagName() == mainTag
            || token.tagName() == menuTag
            || token.tagName() == navTag
            || token.tagName() == olTag
            || token.tagName() == preTag
            || token.tagName() == sectionTag
            || token.tagName() == summaryTag
            || token.tagName() == ulTag) {
            if(!m_openElements.inScope(token.tagName())) {
                handleErrorToken(token);
                return;
            }

            m_openElements.generateImpliedEndTags();
            if(currentElement()->tagName() != token.tagName())
                handleErrorToken(token);
            m_openElements.popUntilPopped(token.tagName());
            return;
        }

        if(token.tagName() == formTag) {
            auto node = m_form;
            m_form = nullptr;
            if(node == nullptr || !m_openElements.inScope(node)) {
                handleErrorToken(token);
                return;
            }

            m_openElements.generateImpliedEndTags();
            if(currentElement() != node)
                handleErrorToken(token);
            m_openElements.remove(node);
            return;
        }

        if(token.tagName() == pTag) {
            if(!m_openElements.inButtonScope(pTag)) {
                handleErrorToken(token);
                handleFakeStartTagToken(pTag);
                handleToken(token);
                return;
            }

            m_openElements.generateImpliedEndTagsExcept(pTag);
            if(currentElement()->tagName() != pTag)
                handleErrorToken(token);
            m_openElements.popUntilPopped(pTag);
            return;
        }

        if(token.tagName() == liTag) {
            if(!m_openElements.inListItemScope(liTag)) {
                handleErrorToken(token);
                return;
            }

            m_openElements.generateImpliedEndTagsExcept(liTag);
            if(currentElement()->tagName() != liTag)
                handleErrorToken(token);
            m_openElements.popUntilPopped(liTag);
            return;
        }

        if(token.tagName() == ddTag
            || token.tagName() == dtTag) {
            if(!m_openElements.inScope(token.tagName())) {
                handleErrorToken(token);
                return;
            }

            m_openElements.generateImpliedEndTagsExcept(token.tagName());
            if(currentElement()->tagName() != token.tagName())
                handleErrorToken(token);
            m_openElements.popUntilPopped(token.tagName());
            return;
        }

        if(isNumberedHeaderTag(token.tagName())) {
            if(!m_openElements.isNumberedHeaderElementInScope()) {
                handleErrorToken(token);
                return;
            }

            m_openElements.generateImpliedEndTags();
            if(currentElement()->tagName() != token.tagName())
                handleErrorToken(token);
            m_openElements.popUntilNumberedHeaderElementPopped();
            return;
        }

        if(token.tagName() == aTag
            || token.tagName() == bTag
            || token.tagName() == bigTag
            || token.tagName() == codeTag
            || token.tagName() == emTag
            || token.tagName() == fontTag
            || token.tagName() == iTag
            || token.tagName() == nobrTag
            || token.tagName() == sTag
            || token.tagName() == smallTag
            || token.tagName() == strikeTag
            || token.tagName() == strongTag
            || token.tagName() == ttTag
            || token.tagName() == uTag) {
            handleFormattingEndTagToken(token);
            return;
        }

        if(token.tagName() == appletTag
            || token.tagName() == marqueeTag
            || token.tagName() == objectTag) {
            if(!m_openElements.inScope(token.tagName())) {
                handleErrorToken(token);
                return;
            }

            m_openElements.generateImpliedEndTags();
            if(currentElement()->tagName() != token.tagName())
                handleErrorToken(token);
            m_openElements.popUntilPopped(token.tagName());
            m_activeFormattingElements.clearToLastMarker();
            return;
        }

        if(token.tagName() == brTag) {
            handleErrorToken(token);
            handleFakeStartTagToken(brTag);
            return;
        }

        handleOtherFormattingEndTagToken(token);
        return;
    }

    if(token.type() == HtmlToken::Type::Character
        || token.type() == HtmlToken::Type::SpaceCharacter) {
        reconstructActiveFormattingElements();
        insertTextNode(token.data());
        if(token.type() == HtmlToken::Type::Character)
            m_framesetOk = false;
        return;
    }

    if(token.type() == HtmlToken::Type::EndOfFile) {
        for(int i = m_openElements.size() - 1; i >= 0; --i) {
            auto element = m_openElements.at(i);
            if(element->tagName() != ddTag
                || element->tagName() != dtTag
                || element->tagName() != liTag
                || element->tagName() != pTag
                || element->tagName() != tbodyTag
                || element->tagName() != tdTag
                || element->tagName() != tfootTag
                || element->tagName() != thTag
                || element->tagName() != theadTag
                || element->tagName() != trTag
                || element->tagName() != bodyTag
                || element->tagName() != htmlTag) {
                handleErrorToken(token);
                return;
            }
        }
    }
}

void HtmlParser::handleTextMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::Character
        || token.type() == HtmlToken::Type::SpaceCharacter) {
        insertTextNode(token.data());
        return;
    }

    if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() == scriptTag) {
            assert(currentElement()->tagName() == scriptTag);
            m_tokenizer.setState(HtmlTokenizer::State::Data);
            m_openElements.pop();
            m_insertionMode = m_originalInsertionMode;
            return;
        }

        m_openElements.pop();
        m_insertionMode = m_originalInsertionMode;
        return;
    }

    if(token.type() == HtmlToken::Type::EndOfFile) {
        handleErrorToken(token);
        m_openElements.pop();
        m_insertionMode = m_originalInsertionMode;
        handleToken(token);
        return;
    }
}

void HtmlParser::handleInTableMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == captionTag) {
            m_openElements.popUntilTableScopeMarker();
            m_activeFormattingElements.appendMarker();
            insertHtmlElement(token);
            m_insertionMode = InsertionMode::InCaption;
            return;
        }

        if(token.tagName() == colgroupTag) {
            m_openElements.popUntilTableScopeMarker();
            insertHtmlElement(token);
            m_insertionMode = InsertionMode::InColumnGroup;
            return;
        }

        if(token.tagName() == colTag) {
            handleFakeStartTagToken(colgroupTag);
            handleToken(token);
            return;
        }

        if(token.tagName() == tbodyTag
            || token.tagName() == tfootTag
            || token.tagName() == theadTag) {
            m_openElements.popUntilTableScopeMarker();
            insertHtmlElement(token);
            m_insertionMode = InsertionMode::InTableBody;
            return;
        }

        if(token.tagName() == thTag
            || token.tagName() == tdTag
            || token.tagName() == trTag) {
            handleFakeStartTagToken(tbodyTag);
            handleToken(token);
            return;
        }

        if(token.tagName() == tableTag) {
            handleErrorToken(token);
            handleFakeEndTagToken(tableTag);
            handleToken(token);
            return;
        }

        if(token.tagName() == styleTag
            || token.tagName() == scriptTag) {
            handleInHeadMode(token);
            return;
        }

        if(token.tagName() == inputTag) {
            auto typeAttribute = token.findAttribute(typeAttr);
            if(typeAttribute && equals(typeAttribute->value(), "hidden", false)) {
                handleErrorToken(token);
                insertSelfClosingHtmlElement(token);
                return;
            }

            handleErrorToken(token);
            m_fosterRedirecting = true;
            handleInBodyMode(token);
            m_fosterRedirecting = false;
            return;
        }

        if(token.tagName() == formTag) {
            handleErrorToken(token);
            if(m_form != nullptr)
                return;
            insertHtmlFormElement(token);
            m_openElements.pop();
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() == tableTag) {
            assert(m_openElements.inTableScope(tableTag));
            m_openElements.popUntilPopped(tableTag);
            resetInsertionModeAppropriately();
            return;
        }

        if(token.tagName() == bodyTag
            || token.tagName() == captionTag
            || token.tagName() == colTag
            || token.tagName() == colgroupTag
            || token.tagName() == htmlTag
            || token.tagName() == tbodyTag
            || token.tagName() == tdTag
            || token.tagName() == tfootTag
            || token.tagName() == thTag
            || token.tagName() == theadTag
            || token.tagName() == trTag) {
            handleErrorToken(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::Character
        || token.type() == HtmlToken::Type::SpaceCharacter) {
        m_pendingTableCharacters.clear();
        m_originalInsertionMode = m_insertionMode;
        m_insertionMode = InsertionMode::InTableText;
        handleToken(token);
        return;
    } else if(token.type() == HtmlToken::Type::EndOfFile) {
        assert(currentElement()->tagName() != htmlTag);
        handleErrorToken(token);
        return;
    }

    handleErrorToken(token);
    m_fosterRedirecting = true;
    handleInBodyMode(token);
    m_fosterRedirecting = false;
}

void HtmlParser::handleInTableTextMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::Character
        || token.type() == HtmlToken::Type::SpaceCharacter) {
        m_pendingTableCharacters += token.data();
        return;
    }

    flushPendingTableCharacters();
    handleToken(token);
}

void HtmlParser::handleInCaptionMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == captionTag
            || token.tagName() == colTag
            || token.tagName() == colgroupTag
            || token.tagName() == tbodyTag
            || token.tagName() == tdTag
            || token.tagName() == tfootTag
            || token.tagName() == thTag
            || token.tagName() == theadTag
            || token.tagName() == trTag) {
            handleErrorToken(token);
            handleFakeEndTagToken(captionTag);
            handleToken(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() == captionTag) {
            assert(m_openElements.inTableScope(captionTag));
            m_openElements.generateImpliedEndTags();
            m_openElements.popUntilPopped(captionTag);
            m_activeFormattingElements.clearToLastMarker();
            m_insertionMode = InsertionMode::InTable;
            return;
        }

        if(token.tagName() == tableTag) {
            handleErrorToken(token);
            handleFakeEndTagToken(captionTag);
            handleToken(token);
            return;
        }

        if(token.tagName() == bodyTag
            || token.tagName() == colTag
            || token.tagName() == colgroupTag
            || token.tagName() == htmlTag
            || token.tagName() == tbodyTag
            || token.tagName() == tdTag
            || token.tagName() == tfootTag
            || token.tagName() == thTag
            || token.tagName() == theadTag
            || token.tagName() == trTag) {
            handleErrorToken(token);
            return;
        }
    }

    handleInBodyMode(token);
}

void HtmlParser::handleInColumnGroupMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == htmlTag) {
            handleInBodyMode(token);
            return;
        }

        if(token.tagName() == colTag) {
            insertSelfClosingHtmlElement(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() == colgroupTag) {
            assert(currentElement()->tagName() == colgroupTag);
            m_openElements.pop();
            m_insertionMode = InsertionMode::InTable;
            return;
        }

        if(token.tagName() == colTag) {
            handleErrorToken(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::SpaceCharacter) {
        insertTextNode(token.data());
        return;
    } else if(token.type() == HtmlToken::Type::EndOfFile) {
        assert(currentElement()->tagName() != htmlTag);
        handleFakeEndTagToken(colgroupTag);
        handleToken(token);
        return;
    }

    handleFakeEndTagToken(colgroupTag);
    handleToken(token);
}

void HtmlParser::handleInTableBodyMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == trTag) {
            m_openElements.popUntilTableBodyScopeMarker();
            insertHtmlElement(token);
            m_insertionMode = InsertionMode::InRow;
            return;
        }

        if(token.tagName() == tdTag
            || token.tagName() == thTag) {
            handleErrorToken(token);
            handleFakeStartTagToken(trTag);
            handleToken(token);
            return;
        }

        if(token.tagName() == captionTag
            || token.tagName() == colTag
            || token.tagName() == colgroupTag
            || token.tagName() == tbodyTag
            || token.tagName() == tfootTag
            || token.tagName() == theadTag) {
            assert(m_openElements.inTableScope(tbodyTag) || m_openElements.inTableScope(theadTag) || m_openElements.inTableScope(tfootTag));
            m_openElements.popUntilTableBodyScopeMarker();
            handleFakeEndTagToken(currentElement()->tagName());
            handleToken(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() == tbodyTag
            || token.tagName() == tfootTag
            || token.tagName() == theadTag) {
            if(!m_openElements.inTableScope(token.tagName())) {
                handleErrorToken(token);
                return;
            }

            m_openElements.popUntilTableBodyScopeMarker();
            m_openElements.pop();
            m_insertionMode = InsertionMode::InTable;
            return;
        }

        if(token.tagName() == tableTag) {
            assert(m_openElements.inTableScope(tbodyTag) || m_openElements.inTableScope(theadTag) || m_openElements.inTableScope(tfootTag));
            m_openElements.popUntilTableBodyScopeMarker();
            handleFakeEndTagToken(currentElement()->tagName());
            handleToken(token);
            return;
        }

        if(token.tagName() == bodyTag
            || token.tagName() == captionTag
            || token.tagName() == colTag
            || token.tagName() == colgroupTag
            || token.tagName() == htmlTag
            || token.tagName() == tdTag
            || token.tagName() == thTag
            || token.tagName() == trTag) {
            handleErrorToken(token);
            return;
        }
    }

    handleInTableMode(token);
}

void HtmlParser::handleInRowMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == tdTag
            || token.tagName() == thTag) {
            m_openElements.popUntilTableRowScopeMarker();
            insertHtmlElement(token);
            m_insertionMode = InsertionMode::InCell;
            m_activeFormattingElements.appendMarker();
            return;
        }

        if(token.tagName() == captionTag
            || token.tagName() == colTag
            || token.tagName() == colgroupTag
            || token.tagName() == tbodyTag
            || token.tagName() == tfootTag
            || token.tagName() == theadTag
            || token.tagName() == trTag) {
            handleFakeEndTagToken(trTag);
            handleToken(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() == trTag) {
            assert(m_openElements.inTableScope(trTag));
            m_openElements.popUntilTableRowScopeMarker();
            m_openElements.pop();
            m_insertionMode = InsertionMode::InTableBody;
            return;
        }

        if(token.tagName() == tableTag) {
            handleFakeEndTagToken(trTag);
            handleToken(token);
            return;
        }

        if(token.tagName() == tbodyTag
            || token.tagName() == tfootTag
            || token.tagName() == theadTag) {
            if(!m_openElements.inTableScope(token.tagName())) {
                handleErrorToken(token);
                return;
            }

            handleFakeEndTagToken(trTag);
            handleToken(token);
            return;
        }

        if(token.tagName() == bodyTag
            || token.tagName() == captionTag
            || token.tagName() == colTag
            || token.tagName() == colgroupTag
            || token.tagName() == htmlTag
            || token.tagName() == tdTag
            || token.tagName() == thTag) {
            handleErrorToken(token);
            return;
        }
    }

    handleInTableMode(token);
}

void HtmlParser::handleInCellMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == captionTag
            || token.tagName() == colTag
            || token.tagName() == colgroupTag
            || token.tagName() == tbodyTag
            || token.tagName() == tdTag
            || token.tagName() == tfootTag
            || token.tagName() == thTag
            || token.tagName() == theadTag
            || token.tagName() == trTag) {
            assert(m_openElements.inTableScope(tdTag) || m_openElements.inTableScope(thTag));
            closeTheCell();
            handleToken(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() == tdTag
            || token.tagName() == thTag) {
            if(!m_openElements.inTableScope(token.tagName())) {
                handleErrorToken(token);
                return;
            }

            m_openElements.generateImpliedEndTags();
            if(currentElement()->tagName() != token.tagName())
                handleErrorToken(token);
            m_openElements.popUntilPopped(token.tagName());
            m_activeFormattingElements.clearToLastMarker();
            m_insertionMode = InsertionMode::InRow;
            return;
        }

        if(token.tagName() == bodyTag
            || token.tagName() == captionTag
            || token.tagName() == colTag
            || token.tagName() == colgroupTag
            || token.tagName() == htmlTag) {
            handleErrorToken(token);
            return;
        }

        if(token.tagName() == tableTag
            || token.tagName() == tbodyTag
            || token.tagName() == tfootTag
            || token.tagName() == theadTag
            || token.tagName() == trTag) {
            if(!m_openElements.inTableScope(token.tagName())) {
                handleErrorToken(token);
                return;
            }

            closeTheCell();
            handleToken(token);
            return;
        }
    }

    handleInBodyMode(token);
}

void HtmlParser::handleInSelectMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == htmlTag) {
            handleInBodyMode(token);
            return;
        }

        if(token.tagName() == optionTag) {
            if(currentElement()->tagName() == optionTag)
                handleFakeEndTagToken(optionTag);
            insertHtmlElement(token);
            return;
        }

        if(token.tagName() == optgroupTag) {
            if(currentElement()->tagName() == optionTag)
                handleFakeEndTagToken(optionTag);
            if(currentElement()->tagName() == optgroupTag)
                handleFakeEndTagToken(optgroupTag);
            insertHtmlElement(token);
            return;
        }

        if(token.tagName() == selectTag) {
            handleErrorToken(token);
            handleFakeEndTagToken(selectTag);
            return;
        }

        if(token.tagName() == inputTag
            || token.tagName() == keygenTag
            || token.tagName() == textareaTag) {
            handleErrorToken(token);
            assert(m_openElements.inSelectScope(selectTag));
            handleFakeEndTagToken(selectTag);
            handleToken(token);
            return;
        }

        if(token.tagName() == scriptTag) {
            handleInHeadMode(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() == optgroupTag) {
            if(currentElement()->tagName() == optionTag) {
                auto element = m_openElements.at(m_openElements.size() - 2);
                if(element->tagName() == optgroupTag)  {
                    handleFakeEndTagToken(optionTag);
                }
            }

            if(currentElement()->tagName() == optgroupTag) {
                m_openElements.pop();
                return;
            }

            handleErrorToken(token);
            return;
        }

        if(token.tagName() == optionTag) {
            if(currentElement()->tagName() == optionTag) {
                m_openElements.pop();
                return;
            }

            handleErrorToken(token);
            return;
        }

        if(token.tagName() == selectTag) {
            assert(m_openElements.inSelectScope(token.tagName()));
            m_openElements.popUntilPopped(selectTag);
            resetInsertionModeAppropriately();
            return;
        }
    } else if(token.type() == HtmlToken::Type::Character
        || token.type() == HtmlToken::Type::SpaceCharacter) {
        insertTextNode(token.data());
        return;
    } else if(token.type() == HtmlToken::Type::EndOfFile) {
        assert(currentElement()->tagName() != htmlTag);
        handleErrorToken(token);
        return;
    }

    handleErrorToken(token);
}

void HtmlParser::handleInSelectInTableMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == captionTag
            || token.tagName() == tableTag
            || token.tagName() == tbodyTag
            || token.tagName() == tfootTag
            || token.tagName() == theadTag
            || token.tagName() == trTag
            || token.tagName() == tdTag
            || token.tagName() == thTag) {
            handleErrorToken(token);
            handleFakeEndTagToken(selectTag);
            handleToken(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() == captionTag
            || token.tagName() == tableTag
            || token.tagName() == tbodyTag
            || token.tagName() == tfootTag
            || token.tagName() == theadTag
            || token.tagName() == trTag
            || token.tagName() == tdTag
            || token.tagName() == thTag) {
            handleErrorToken(token);
            if(m_openElements.inTableScope(token.tagName())) {
                handleFakeEndTagToken(selectTag);
                handleToken(token);
            }

            return;
        }
    }

    handleInSelectMode(token);
}

void HtmlParser::handleAfterBodyMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == htmlTag) {
            handleInBodyMode(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() == htmlTag) {
            m_insertionMode = InsertionMode::AfterAfterBody;
            return;
        }
    } else if(token.type() == HtmlToken::Type::SpaceCharacter) {
        handleInBodyMode(token);
        return;
    } else if(token.type() == HtmlToken::Type::EndOfFile) {
        return;
    }

    handleErrorToken(token);
    m_insertionMode = InsertionMode::InBody;
    handleToken(token);
}

void HtmlParser::handleInFramesetMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == htmlTag) {
            handleInBodyMode(token);
            return;
        }

        if(token.tagName() == framesetTag) {
            insertHtmlElement(token);
            return;
        }

        if(token.tagName() == frameTag) {
            insertSelfClosingHtmlElement(token);
            return;
        }

        if(token.tagName() == noframesTag) {
            handleInHeadMode(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() == framesetTag) {
            assert(currentElement()->tagName() != htmlTag);
            m_openElements.pop();
            if(currentElement()->tagName() != framesetTag)
                m_insertionMode = InsertionMode::AfterFrameset;
            return;
        }
    } else if(token.type() == HtmlToken::Type::SpaceCharacter) {
        insertTextNode(token.data());
        return;
    } else if(token.type() == HtmlToken::Type::EndOfFile) {
        assert(currentElement()->tagName() != htmlTag);
        handleErrorToken(token);
        return;
    }

    handleErrorToken(token);
}

void HtmlParser::handleAfterFramesetMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == htmlTag) {
            handleInBodyMode(token);
            return;
        }

        if(token.tagName() == noframesTag) {
            handleInHeadMode(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::EndTag) {
        if(token.tagName() == htmlTag) {
            m_insertionMode = InsertionMode::AfterAfterFrameset;
            return;
        }
    } else if(token.type() == HtmlToken::Type::SpaceCharacter) {
        insertTextNode(token.data());
        return;
    } else if(token.type() == HtmlToken::Type::EndOfFile) {
        return;
    }

    handleErrorToken(token);
}

void HtmlParser::handleAfterAfterBodyMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == htmlTag) {
            handleInBodyMode(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::SpaceCharacter) {
        handleInBodyMode(token);
        return;
    } else if(token.type() == HtmlToken::Type::EndOfFile) {
        return;
    }

    handleErrorToken(token);
    m_insertionMode = InsertionMode::InBody;
    handleToken(token);
}

void HtmlParser::handleAfterAfterFramesetMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == htmlTag) {
            handleInBodyMode(token);
            return;
        }

        if(token.tagName() == noframesTag) {
            handleInHeadMode(token);
            return;
        }
    } else if(token.type() == HtmlToken::Type::SpaceCharacter) {
        handleInBodyMode(token);
        return;
    } else if(token.type() == HtmlToken::Type::EndOfFile) {
        return;
    }

    handleErrorToken(token);
}

void HtmlParser::handleInForeignContentMode(HtmlTokenView& token)
{
    if(token.type() == HtmlToken::Type::Character
        || token.type() == HtmlToken::Type::SpaceCharacter) {
        insertTextNode(token.data());
        if(token.type() == HtmlToken::Type::Character)
            m_framesetOk = false;
        return;
    }

    if(token.type() == HtmlToken::Type::StartTag) {
        if(token.tagName() == bTag
            || token.tagName() == bigTag
            || token.tagName() == blockquoteTag
            || token.tagName() == bodyTag
            || token.tagName() == brTag
            || token.tagName() == centerTag
            || token.tagName() == codeTag
            || token.tagName() == ddTag
            || token.tagName() == divTag
            || token.tagName() == dlTag
            || token.tagName() == dtTag
            || token.tagName() == emTag
            || token.tagName() == embedTag
            || isNumberedHeaderTag(token.tagName())
            || token.tagName() == headTag
            || token.tagName() == hrTag
            || token.tagName() == iTag
            || token.tagName() == imgTag
            || token.tagName() == liTag
            || token.tagName() == listingTag
            || token.tagName() == menuTag
            || token.tagName() == metaTag
            || token.tagName() == nobrTag
            || token.tagName() == olTag
            || token.tagName() == pTag
            || token.tagName() == preTag
            || token.tagName() == rubyTag
            || token.tagName() == sTag
            || token.tagName() == smallTag
            || token.tagName() == spanTag
            || token.tagName() == strongTag
            || token.tagName() == strikeTag
            || token.tagName() == subTag
            || token.tagName() == supTag
            || token.tagName() == tableTag
            || token.tagName() == ttTag
            || token.tagName() == uTag
            || token.tagName() == ulTag
            || token.tagName() == varTag
            || (token.tagName() == fontTag && (token.hasAttribute(colorAttr) || token.hasAttribute(faceAttr) || token.hasAttribute(sizeAttr)))) {
            handleErrorToken(token);
            m_openElements.popUntilForeignContentScopeMarker();
            handleToken(token);
            return;
        }

        const auto& currentNamespace = currentElement()->namespaceURI();
        if(currentNamespace == mathmlNs) {
            adjustMathMLAttributes(token);
        } else if(currentNamespace == svgNs) {
            adjustSvgTagNames(token);
            adjustSvgAttributes(token);
        }

        adjustForeignAttributes(token);
        insertForeignElement(token, currentNamespace);
        return;
    }

    if(token.type() == HtmlToken::Type::EndTag) {
        auto node = m_openElements.top();
        if(node->namespaceURI() == svgNs)
            adjustSvgTagNames(token);
        if(node->tagName() != token.tagName()) {
            handleErrorToken(token);
        }

        for(int i = m_openElements.size() - 1; i >= 0; --i) {
            if(node->tagName() == token.tagName()) {
                m_openElements.popUntilPopped(node);
                return;
            }

            node = m_openElements.at(i - 1);
            if(node->namespaceURI() == xhtmlNs) {
                handleToken(token);
                return;
            }
        }
    }
}

void HtmlParser::handleFakeStartTagToken(const GlobalString& tagName)
{
    HtmlTokenView token(HtmlToken::Type::StartTag, tagName);
    handleToken(token, m_insertionMode);
}

void HtmlParser::handleFakeEndTagToken(const GlobalString& tagName)
{
    HtmlTokenView token(HtmlToken::Type::EndTag, tagName);
    handleToken(token, m_insertionMode);
}

void HtmlParser::handleFormattingEndTagToken(HtmlTokenView& token)
{
    static const int outerIterationLimit = 8;
    static const int innerIterationLimit = 3;
    for(int i = 0; i < outerIterationLimit; ++i) {
        auto formattingElement = m_activeFormattingElements.closestElementInScope(token.tagName());
        if(formattingElement == nullptr) {
            handleOtherFormattingEndTagToken(token);
            return;
        }

        if(!m_openElements.contains(formattingElement)) {
            handleErrorToken(token);
            m_activeFormattingElements.remove(formattingElement);
            return;
        }

        if(!m_openElements.inScope(formattingElement)) {
            handleErrorToken(token);
            return;
        }

        if(formattingElement != m_openElements.top())
            handleErrorToken(token);
        auto furthestBlock = m_openElements.furthestBlockForFormattingElement(formattingElement);
        if(furthestBlock == nullptr) {
            m_openElements.popUntilPopped(formattingElement);
            m_activeFormattingElements.remove(formattingElement);
            return;
        }

        auto commonAncestor = m_openElements.previous(formattingElement);
        auto bookmark = m_activeFormattingElements.index(formattingElement);

        auto furthestBlockIndex = m_openElements.index(furthestBlock);
        auto lastNode = furthestBlock;
        for(int i = 0; i < innerIterationLimit; ++i) {
            furthestBlockIndex -= 1;
            auto node = m_openElements.at(furthestBlockIndex);
            if(!m_activeFormattingElements.contains(node)) {
                m_openElements.remove(furthestBlockIndex);
                continue;
            }

            if(node == formattingElement)
                break;
            if(lastNode == furthestBlock) {
                bookmark = m_activeFormattingElements.index(node) + 1;
            }

            auto newNode = cloneElement(node);
            m_activeFormattingElements.replace(node, newNode);
            m_openElements.replace(furthestBlockIndex, newNode);

            lastNode->reparent(newNode);
            lastNode = newNode;
        }

        lastNode->remove();

        if(isFosterRedirectingTag(commonAncestor->tagName())) {
            fosterParent(lastNode);
        } else {
            commonAncestor->appendChild(lastNode);
        }

        auto newNode = cloneElement(formattingElement);
        furthestBlock->reparentChildren(newNode);
        furthestBlock->appendChild(newNode);

        m_activeFormattingElements.remove(formattingElement);
        m_activeFormattingElements.insert(bookmark, newNode);

        m_openElements.remove(formattingElement);
        m_openElements.insertAfter(furthestBlock, newNode);
    }
}

void HtmlParser::handleOtherFormattingEndTagToken(HtmlTokenView& token)
{
    for(int i = m_openElements.size() - 1; i >= 0; --i) {
        auto element = m_openElements.at(i);
        if(element->tagName() == token.tagName()) {
            m_openElements.generateImpliedEndTagsExcept(token.tagName());
            if(currentElement()->tagName() != token.tagName())
                handleErrorToken(token);
            m_openElements.popUntilPopped(element);
            break;
        }

        if(isSpecialElement(element)) {
            handleErrorToken(token);
            break;
        }
    }
}

void HtmlParser::handleErrorToken(HtmlTokenView& token)
{
}

void HtmlParser::handleRCDataToken(HtmlTokenView& token)
{
    insertHtmlElement(token);
    m_tokenizer.setState(HtmlTokenizer::State::RCDATA);
    m_originalInsertionMode = m_insertionMode;
    m_insertionMode = InsertionMode::Text;
}

void HtmlParser::handleRawTextToken(HtmlTokenView& token)
{
    insertHtmlElement(token);
    m_tokenizer.setState(HtmlTokenizer::State::RAWTEXT);
    m_originalInsertionMode = m_insertionMode;
    m_insertionMode = InsertionMode::Text;
}

void HtmlParser::handleScriptDataToken(HtmlTokenView& token)
{
    insertHtmlElement(token);
    m_tokenizer.setState(HtmlTokenizer::State::ScriptData);
    m_originalInsertionMode = m_insertionMode;
    m_insertionMode = InsertionMode::Text;
}

void HtmlParser::handleDoctypeToken(HtmlTokenView& token)
{
    if(m_insertionMode == InsertionMode::Initial) {
        insertDoctype(token);
        m_insertionMode = InsertionMode::BeforeHtml;
        return;
    }

    if(m_insertionMode == InsertionMode::InTableText) {
        flushPendingTableCharacters();
        handleDoctypeToken(token);
        return;
    }

    handleErrorToken(token);
}

void HtmlParser::handleCommentToken(HtmlTokenView& token)
{
    if(m_insertionMode == InsertionMode::Initial
        || m_insertionMode == InsertionMode::BeforeHtml
        || m_insertionMode == InsertionMode::AfterAfterBody
        || m_insertionMode == InsertionMode::AfterAfterFrameset) {
        insertComment(token, m_document);
        return;
    }

    if(m_insertionMode == InsertionMode::AfterBody) {
        insertComment(token, m_openElements.htmlElement());
        return;
    }

    if(m_insertionMode == InsertionMode::InTableText) {
        flushPendingTableCharacters();
        handleCommentToken(token);
        return;
    }

    insertComment(token, m_openElements.top());
}

void HtmlParser::handleToken(HtmlTokenView& token, InsertionMode mode)
{
    switch(mode) {
    case InsertionMode::Initial:
        return handleInitialMode(token);
    case InsertionMode::BeforeHtml:
        return handleBeforeHtmlMode(token);
    case InsertionMode::BeforeHead:
        return handleBeforeHeadMode(token);
    case InsertionMode::InHead:
        return handleInHeadMode(token);
    case InsertionMode::InHeadNoscript:
        return handleInHeadNoscriptMode(token);
    case InsertionMode::AfterHead:
        return handleAfterHeadMode(token);
    case InsertionMode::InBody:
        return handleInBodyMode(token);
    case InsertionMode::Text:
        return handleTextMode(token);
    case InsertionMode::InTable:
        return handleInTableMode(token);
    case InsertionMode::InTableText:
        return handleInTableTextMode(token);
    case InsertionMode::InCaption:
        return handleInCaptionMode(token);
    case InsertionMode::InColumnGroup:
        return handleInColumnGroupMode(token);
    case InsertionMode::InTableBody:
        return handleInTableBodyMode(token);
    case InsertionMode::InRow:
        return handleInRowMode(token);
    case InsertionMode::InCell:
        return handleInCellMode(token);
    case InsertionMode::InSelect:
        return handleInSelectMode(token);
    case InsertionMode::InSelectInTable:
        return handleInSelectInTableMode(token);
    case InsertionMode::AfterBody:
        return handleAfterBodyMode(token);
    case InsertionMode::InFrameset:
        return handleInFramesetMode(token);
    case InsertionMode::AfterFrameset:
        return handleAfterFramesetMode(token);
    case InsertionMode::AfterAfterBody:
        return handleAfterAfterBodyMode(token);
    case InsertionMode::AfterAfterFrameset:
        return handleAfterAfterFramesetMode(token);
    case InsertionMode::InForeignContent:
        return handleInForeignContentMode(token);
    }
}

} // namespace plutobook
