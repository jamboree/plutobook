#include "css-rule.h"
#include "css-parser.h"
#include "document.h"
#include "font-resource.h"
#include "image-resource.h"
#include "box-style.h"
#include "ua-stylesheet.h"
#include "string-utils.h"

#include <unicode/uiter.h>

namespace plutobook {

CssLengthResolver::CssLengthResolver(const Document* document, const Font* font)
    : m_document(document), m_font(font)
{
}

float CssLengthResolver::resolveLength(const CssValue& value) const
{
    if(is<CssLengthValue>(value))
        return resolveLength(to<CssLengthValue>(value));
    return to<CssCalcValue>(value).resolve(*this);
}

float CssLengthResolver::resolveLength(const CssLengthValue& length) const
{
    return resolveLength(length.value(), length.units());
}

float CssLengthResolver::resolveLength(float value, CssLengthUnits units) const
{
    constexpr auto dpi = 96.f;
    switch(units) {
    case CssLengthUnits::None:
    case CssLengthUnits::Pixels:
        return value;
    case CssLengthUnits::Inches:
        return value * dpi;
    case CssLengthUnits::Centimeters:
        return value * dpi / 2.54f;
    case CssLengthUnits::Millimeters:
        return value * dpi / 25.4f;
    case CssLengthUnits::Points:
        return value * dpi / 72.f;
    case CssLengthUnits::Picas:
        return value * dpi / 6.f;
    case CssLengthUnits::Ems:
        return value * emFontSize();
    case CssLengthUnits::Exs:
        return value * exFontSize();
    case CssLengthUnits::Rems:
        return value * remFontSize();
    case CssLengthUnits::Chs:
        return value * chFontSize();
    case CssLengthUnits::ViewportWidth:
        return value * viewportWidth() / 100.f;
    case CssLengthUnits::ViewportHeight:
        return value * viewportHeight() / 100.f;
    case CssLengthUnits::ViewportMin:
        return value * viewportMin() / 100.f;
    case CssLengthUnits::ViewportMax:
        return value * viewportMax() / 100.f;
    default:
        assert(false);
    }

    return 0.f;
}

float CssLengthResolver::emFontSize() const
{
    if(m_font == nullptr)
        return kMediumFontSize;
    return m_font->size();
}

float CssLengthResolver::exFontSize() const
{
    if(m_font == nullptr)
        return kMediumFontSize / 2.f;
    if(auto fontData = m_font->primaryFont())
        return fontData->xHeight();
    return m_font->size() / 2.f;
}

float CssLengthResolver::chFontSize() const
{
    if(m_font == nullptr)
        return kMediumFontSize / 2.f;
    if(auto fontData = m_font->primaryFont())
        return fontData->zeroWidth();
    return m_font->size() / 2.f;
}

float CssLengthResolver::remFontSize() const
{
    if(m_document == nullptr)
        return kMediumFontSize;
    if(auto style = m_document->rootStyle())
        return style->fontSize();
    return kMediumFontSize;
}

float CssLengthResolver::viewportWidth() const
{
    if(m_document)
        return m_document->viewportWidth();
    return 0.f;
}

float CssLengthResolver::viewportHeight() const
{
    if(m_document)
        return m_document->viewportHeight();
    return 0.f;
}

float CssLengthResolver::viewportMin() const
{
    if(m_document)
        return std::min(m_document->viewportWidth(), m_document->viewportHeight());
    return 0.f;
}

float CssLengthResolver::viewportMax() const
{
    if(m_document)
        return std::max(m_document->viewportWidth(), m_document->viewportHeight());
    return 0.f;
}

float CssCalcValue::resolve(const CssLengthResolver& resolver) const
{
    std::vector<CssCalc> stack;
    for(const auto& item : m_values) {
        if(item.op == CssCalcOperator::None) {
            if(item.units == CssLengthUnits::None) {
                stack.push_back(item);
            } else {
                auto value = resolver.resolveLength(item.value, item.units);
                stack.emplace_back(value, CssLengthUnits::Pixels);
            }
        } else {
            if(stack.size() < 2)
                return 0;
            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            switch(item.op) {
            case CssCalcOperator::Add:
                if(right.units != left.units)
                    return 0;
                stack.emplace_back(left.value + right.value, right.units);
                break;
            case CssCalcOperator::Sub:
                if(right.units != left.units)
                    return 0;
                stack.emplace_back(left.value - right.value, right.units);
                break;
            case CssCalcOperator::Mul:
                if(right.units == CssLengthUnits::Pixels && left.units == CssLengthUnits::Pixels)
                    return 0;
                stack.emplace_back(left.value * right.value, std::max(left.units, right.units));
                break;
            case CssCalcOperator::Div:
                if(right.units == CssLengthUnits::Pixels || right.value == 0)
                    return 0;
                stack.emplace_back(left.value / right.value, left.units);
                break;
            case CssCalcOperator::Min:
                if(right.units != left.units)
                    return 0;
                stack.emplace_back(std::min(left.value, right.value), right.units);
                break;
            case CssCalcOperator::Max:
                if(right.units != left.units)
                    return 0;
                stack.emplace_back(std::max(left.value, right.value), right.units);
                break;
            default:
                assert(false);
            }
        }
    }

    if(stack.size() == 1) {
        const auto& result = stack.back();
        if(result.value < 0 && !m_negative)
            return 0;
        if(result.units == CssLengthUnits::None && !m_unitless)
            return 0;
        return result.value;
    }

    return 0;
}

class CssValuePool {
public:
    CssValuePool();

    CssInitialValue* initialValue() const { return m_initialValue; }
    CssInheritValue* inheritValue() const { return m_inheritValue; }
    CssUnsetValue* unsetValue() const { return m_unsetValue; }

    CssIdentValue* identValue(CssValueID id) const;

private:
    using CssIdentValueList = std::vector<CssIdentValue*>;
    CssInitialValue* m_initialValue;
    CssInheritValue* m_inheritValue;
    CssUnsetValue* m_unsetValue;
    CssIdentValueList m_identValues;
};

CssValuePool::CssValuePool()
    : m_initialValue(new CssInitialValue)
    , m_inheritValue(new CssInheritValue)
    , m_unsetValue(new CssUnsetValue)
    , m_identValues(kNumCssValueIDs)
{
    assert(CssValueID::Unknown == static_cast<CssValueID>(0));
    for(int i = 1; i < kNumCssValueIDs; ++i) {
        const auto id = static_cast<CssValueID>(i);
        m_identValues[i] = new CssIdentValue(id);
    }
}

CssIdentValue* CssValuePool::identValue(CssValueID id) const
{
    return m_identValues[static_cast<int>(id)];
}

static CssValuePool* cssValuePool()
{
    static CssValuePool valuePool;
    return &valuePool;
}

RefPtr<CssInitialValue> CssInitialValue::create()
{
    return cssValuePool()->initialValue();
}

RefPtr<CssInheritValue> CssInheritValue::create()
{
    return cssValuePool()->inheritValue();
}

RefPtr<CssUnsetValue> CssUnsetValue::create()
{
    return cssValuePool()->unsetValue();
}

RefPtr<CssIdentValue> CssIdentValue::create(CssValueID value)
{
    return cssValuePool()->identValue(value);
}

RefPtr<CssVariableData> CssVariableData::create(const CssTokenStream& value)
{
    return adoptPtr(new CssVariableData(value));
}

CssVariableData::CssVariableData(const CssTokenStream& value)
{
    m_tokens.assign(value.begin(), value.end());
    for(auto& token : m_tokens) {
        if(!token.m_data.empty()) {
            token.m_data = createString(token.data());
        }
    }
}

bool CssVariableData::resolve(const BoxStyle* style, CssTokenList& tokens, boost::unordered_flat_set<CssVariableData*>& references) const
{
    CssTokenStream input(m_tokens.data(), m_tokens.size());
    return resolve(input, style, tokens, references);
}

bool CssVariableData::resolve(CssTokenStream input, const BoxStyle* style, CssTokenList& tokens, boost::unordered_flat_set<CssVariableData*>& references) const
{
    while(!input.empty()) {
        if(input->type() == CssToken::Type::Function && equalsIgnoringCase("var", input->data())) {
            auto block = input.consumeBlock();
            if(!resolveVar(block, style, tokens, references))
                return false;
            continue;
        }

        tokens.push_back(input.get());
        input.consume();
    }

    return true;
}

bool CssVariableData::resolveVar(CssTokenStream input, const BoxStyle* style, CssTokenList& tokens, boost::unordered_flat_set<CssVariableData*>& references) const
{
    input.consumeWhitespace();
    if(input->type() != CssToken::Type::Ident)
        return false;
    auto data = style->getCustom(input->data());
    input.consumeIncludingWhitespace();
    if(!input.empty() && input->type() != CssToken::Type::Comma)
        return false;
    if(data == nullptr) {
        if(!input.consumeCommaIncludingWhitespace())
            return false;
        return resolve(input, style, tokens, references);
    }

    return references.insert(data).second && data->resolve(style, tokens, references);
}

RefPtr<CssCustomPropertyValue> CssCustomPropertyValue::create(GlobalString name, RefPtr<CssVariableData> value)
{
    return adoptPtr(new CssCustomPropertyValue(name, std::move(value)));
}

CssCustomPropertyValue::CssCustomPropertyValue(GlobalString name, RefPtr<CssVariableData> value)
    : CssValue(classKind), m_name(name), m_value(std::move(value))
{
}

CssParserContext::CssParserContext(const Node* node, CssStyleOrigin origin, Url baseUrl)
    : m_inHtmlDocument(node && node->isHtmlDocument())
    , m_inSvgElement(node && node->isSvgElement())
    , m_origin(origin)
    , m_baseUrl(std::move(baseUrl))
{
}

RefPtr<CssVariableReferenceValue> CssVariableReferenceValue::create(const CssParserContext& context, CssPropertyID id, bool important, RefPtr<CssVariableData> value)
{
    return adoptPtr(new CssVariableReferenceValue(context, id, important, std::move(value)));
}

CssPropertyList CssVariableReferenceValue::resolve(const BoxStyle* style) const
{
    CssTokenList tokens;
    boost::unordered_flat_set<CssVariableData*> references;
    if(!m_value->resolve(style, tokens, references))
        return CssPropertyList();
    CssTokenStream input(tokens.data(), tokens.size());
    CssParser parser(m_context);
    return parser.parsePropertyValue(input, m_id, m_important);
}

CssVariableReferenceValue::CssVariableReferenceValue(const CssParserContext& context, CssPropertyID id, bool important, RefPtr<CssVariableData> value)
    : CssValue(classKind), m_context(context), m_id(id), m_important(important), m_value(std::move(value))
{
}

RefPtr<CssImageValue> CssImageValue::create(Url value)
{
    return adoptPtr(new CssImageValue(std::move(value)));
}

const RefPtr<Image>& CssImageValue::fetch(Document* document) const
{
    if(m_image == nullptr) {
        if(auto imageResource = document->fetchImageResource(m_value)) {
            m_image = imageResource->image();
        }
    }

    return m_image;
}

CssImageValue::CssImageValue(Url value)
    : CssValue(classKind), m_value(std::move(value))
{
}

bool CssSimpleSelector::matchnth(int count) const
{
    auto [a, b] = m_matchPattern;
    if(a > 0)
        return count >= b && !((count - b) % a);
    if(a < 0)
        return count <= b && !((b - count) % -a);
    return count == b;
}

PseudoType CssSimpleSelector::pseudoType() const
{
    switch(m_matchType) {
    case MatchType::PseudoElementBefore:
        return PseudoType::Before;
    case MatchType::PseudoElementAfter:
        return PseudoType::After;
    case MatchType::PseudoElementMarker:
        return PseudoType::Marker;
    case MatchType::PseudoElementFirstLetter:
        return PseudoType::FirstLetter;
    case MatchType::PseudoElementFirstLine:
        return PseudoType::FirstLine;
    case MatchType::PseudoPageFirst:
        return PseudoType::FirstPage;
    case MatchType::PseudoPageLeft:
        return PseudoType::LeftPage;
    case MatchType::PseudoPageRight:
        return PseudoType::RightPage;
    case MatchType::PseudoPageBlank:
        return PseudoType::BlankPage;
    default:
        return PseudoType::None;
    }
}

uint32_t CssSimpleSelector::specificity() const
{
    switch(m_matchType) {
    case MatchType::Id:
        return 0x10000;
    case MatchType::Class:
    case MatchType::AttributeContains:
    case MatchType::AttributeDashEquals:
    case MatchType::AttributeEndsWith:
    case MatchType::AttributeEquals:
    case MatchType::AttributeHas:
    case MatchType::AttributeIncludes:
    case MatchType::AttributeStartsWith:
    case MatchType::PseudoClassActive:
    case MatchType::PseudoClassAnyLink:
    case MatchType::PseudoClassChecked:
    case MatchType::PseudoClassDisabled:
    case MatchType::PseudoClassEmpty:
    case MatchType::PseudoClassEnabled:
    case MatchType::PseudoClassFirstChild:
    case MatchType::PseudoClassFirstOfType:
    case MatchType::PseudoClassFocus:
    case MatchType::PseudoClassFocusVisible:
    case MatchType::PseudoClassFocusWithin:
    case MatchType::PseudoClassHover:
    case MatchType::PseudoClassLang:
    case MatchType::PseudoClassLastChild:
    case MatchType::PseudoClassLastOfType:
    case MatchType::PseudoClassLink:
    case MatchType::PseudoClassLocalLink:
    case MatchType::PseudoClassNthChild:
    case MatchType::PseudoClassNthLastChild:
    case MatchType::PseudoClassNthLastOfType:
    case MatchType::PseudoClassNthOfType:
    case MatchType::PseudoClassOnlyChild:
    case MatchType::PseudoClassOnlyOfType:
    case MatchType::PseudoClassRoot:
    case MatchType::PseudoClassScope:
    case MatchType::PseudoClassTarget:
    case MatchType::PseudoClassTargetWithin:
    case MatchType::PseudoClassVisited:
        return 0x100;
    case MatchType::Tag:
    case MatchType::PseudoElementAfter:
    case MatchType::PseudoElementBefore:
    case MatchType::PseudoElementFirstLetter:
    case MatchType::PseudoElementFirstLine:
    case MatchType::PseudoElementMarker:
        return 0x1;
    case MatchType::PseudoClassIs:
    case MatchType::PseudoClassNot:
    case MatchType::PseudoClassHas: {
        uint32_t maxSpecificity = 0;
        for(const auto& subSelector : m_subSelectors) {
            uint32_t specificity = 0x0;
            for(const auto& complexSelector : subSelector) {
                for(const auto& simpleSelector : complexSelector.compoundSelector()) {
                    specificity += simpleSelector.specificity();
                }
            }

            maxSpecificity = std::max(specificity, maxSpecificity);
        }

        return maxSpecificity;
    }

    default:
        return 0x0;
    }
}

bool CssRuleData::match(const Element* element, PseudoType pseudoType) const
{
    return matchSelector(element, pseudoType, *m_selector);
}

bool CssRuleData::matchSelector(const Element* element, PseudoType pseudoType, const CssSelector& selector)
{
    assert(!selector.empty());
    auto it = selector.begin();
    auto end = selector.end();
    if(!matchCompoundSelector(element, pseudoType, it->compoundSelector())) {
        return false;
    }

    auto combinator = it->combinator();
    ++it;

    while(it != end) {
        switch(combinator) {
        case CssComplexSelector::Combinator::Descendant:
        case CssComplexSelector::Combinator::Child:
            element = element->parentElement();
            break;
        case CssComplexSelector::Combinator::DirectAdjacent:
        case CssComplexSelector::Combinator::InDirectAdjacent:
            element = element->previousSiblingElement();
            break;
        case CssComplexSelector::Combinator::None:
            assert(false);
        }

        if(element == nullptr)
            return false;
        if(matchCompoundSelector(element, PseudoType::None, it->compoundSelector())) {
            combinator = it->combinator();
            ++it;
        } else if(combinator != CssComplexSelector::Combinator::Descendant
            && combinator != CssComplexSelector::Combinator::InDirectAdjacent) {
            return false;
        }
    }

    return true;
}

bool CssRuleData::matchCompoundSelector(const Element* element, PseudoType pseudoType, const CssCompoundSelector& selector)
{
    assert(!selector.empty());
    auto it = selector.begin();
    auto end = selector.end();
    if(pseudoType != PseudoType::None) {
        if(pseudoType != it->pseudoType())
            return false;
        ++it;
    }

    for(; it != end; ++it) {
        if(!matchSimpleSelector(element, *it)) {
            return false;
        }
    }

    return true;
}

bool CssRuleData::matchSimpleSelector(const Element* element, const CssSimpleSelector& selector)
{
    switch(selector.matchType()) {
    case CssSimpleSelector::MatchType::Universal:
        return true;
    case CssSimpleSelector::MatchType::Namespace:
        return matchNamespaceSelector(element, selector);
    case CssSimpleSelector::MatchType::Tag:
        return matchTagSelector(element, selector);
    case CssSimpleSelector::MatchType::Id:
        return matchIdSelector(element, selector);
    case CssSimpleSelector::MatchType::Class:
        return matchClassSelector(element, selector);
    case CssSimpleSelector::MatchType::AttributeHas:
        return matchAttributeHasSelector(element, selector);
    case CssSimpleSelector::MatchType::AttributeEquals:
        return matchAttributeEqualsSelector(element, selector);
    case CssSimpleSelector::MatchType::AttributeIncludes:
        return matchAttributeIncludesSelector(element, selector);
    case CssSimpleSelector::MatchType::AttributeContains:
        return matchAttributeContainsSelector(element, selector);
    case CssSimpleSelector::MatchType::AttributeDashEquals:
        return matchAttributeDashEqualsSelector(element, selector);
    case CssSimpleSelector::MatchType::AttributeStartsWith:
        return matchAttributeStartsWithSelector(element, selector);
    case CssSimpleSelector::MatchType::AttributeEndsWith:
        return matchAttributeEndsWithSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassIs:
    case CssSimpleSelector::MatchType::PseudoClassWhere:
        return matchPseudoClassIsSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassNot:
        return matchPseudoClassNotSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassHas:
        return matchPseudoClassHasSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassLink:
    case CssSimpleSelector::MatchType::PseudoClassAnyLink:
        return matchPseudoClassLinkSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassLocalLink:
        return matchPseudoClassLocalLinkSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassEnabled:
        return matchPseudoClassEnabledSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassDisabled:
        return matchPseudoClassDisabledSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassChecked:
        return matchPseudoClassCheckedSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassLang:
        return matchPseudoClassLangSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassRoot:
    case CssSimpleSelector::MatchType::PseudoClassScope:
        return matchPseudoClassRootSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassEmpty:
        return matchPseudoClassEmptySelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassFirstChild:
        return matchPseudoClassFirstChildSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassLastChild:
        return matchPseudoClassLastChildSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassOnlyChild:
        return matchPseudoClassOnlyChildSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassFirstOfType:
        return matchPseudoClassFirstOfTypeSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassLastOfType:
        return matchPseudoClassLastOfTypeSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassOnlyOfType:
        return matchPseudoClassOnlyOfTypeSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassNthChild:
        return matchPseudoClassNthChildSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassNthLastChild:
        return matchPseudoClassNthLastChildSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassNthOfType:
        return matchPseudoClassNthOfTypeSelector(element, selector);
    case CssSimpleSelector::MatchType::PseudoClassNthLastOfType:
        return matchPseudoClassNthLastOfTypeSelector(element, selector);
    default:
        return false;
    }
}

bool CssRuleData::matchNamespaceSelector(const Element* element, const CssSimpleSelector& selector)
{
    return selector.name() == starGlo || element->namespaceURI() == selector.name();
}

bool CssRuleData::matchTagSelector(const Element* element, const CssSimpleSelector& selector)
{
    if(element->isCaseSensitive())
        return element->tagName() == selector.name();
    return equalsIgnoringCase(element->tagName(), selector.name());
}

bool CssRuleData::matchIdSelector(const Element* element, const CssSimpleSelector& selector)
{
    return element->id() == selector.value();
}

bool CssRuleData::matchClassSelector(const Element* element, const CssSimpleSelector& selector)
{
    for(const auto& name : element->classNames()) {
        if(name == selector.value()) {
            return true;
        }
    }

    return false;
}

bool CssRuleData::matchAttributeHasSelector(const Element* element, const CssSimpleSelector& selector)
{
    return element->findAttributePossiblyIgnoringCase(selector.name());
}

bool CssRuleData::matchAttributeEqualsSelector(const Element* element, const CssSimpleSelector& selector)
{
    auto attribute = element->findAttributePossiblyIgnoringCase(selector.name());
    if(attribute == nullptr)
        return false;
    return equals(attribute->value(), selector.value(), selector.isCaseSensitive());
}

bool CssRuleData::matchAttributeIncludesSelector(const Element* element, const CssSimpleSelector& selector)
{
    auto attribute = element->findAttributePossiblyIgnoringCase(selector.name());
    if(attribute == nullptr)
        return false;
    return includes(attribute->value(), selector.value(), selector.isCaseSensitive());
}

bool CssRuleData::matchAttributeContainsSelector(const Element* element, const CssSimpleSelector& selector)
{
    auto attribute = element->findAttributePossiblyIgnoringCase(selector.name());
    if(attribute == nullptr)
        return false;
    return contains(attribute->value(), selector.value(), selector.isCaseSensitive());
}

bool CssRuleData::matchAttributeDashEqualsSelector(const Element* element, const CssSimpleSelector& selector)
{
    auto attribute = element->findAttributePossiblyIgnoringCase(selector.name());
    if(attribute == nullptr)
        return false;
    return dashequals(attribute->value(), selector.value(), selector.isCaseSensitive());
}

bool CssRuleData::matchAttributeStartsWithSelector(const Element* element, const CssSimpleSelector& selector)
{
    auto attribute = element->findAttributePossiblyIgnoringCase(selector.name());
    if(attribute == nullptr)
        return false;
    return startswith(attribute->value(), selector.value(), selector.isCaseSensitive());
}

bool CssRuleData::matchAttributeEndsWithSelector(const Element* element, const CssSimpleSelector& selector)
{
    auto attribute = element->findAttributePossiblyIgnoringCase(selector.name());
    if(attribute == nullptr)
        return false;
    return endswith(attribute->value(), selector.value(), selector.isCaseSensitive());
}

bool CssRuleData::matchPseudoClassIsSelector(const Element* element, const CssSimpleSelector& selector)
{
    for(const auto& subSelector : selector.subSelectors()) {
        if(matchSelector(element, PseudoType::None, subSelector)) {
            return true;
        }
    }

    return false;
}

bool CssRuleData::matchPseudoClassNotSelector(const Element* element, const CssSimpleSelector& selector)
{
    return !matchPseudoClassIsSelector(element, selector);
}

bool CssRuleData::matchPseudoClassHasSelector(const Element* element, const CssSimpleSelector& selector)
{
    for(const auto& subSelector : selector.subSelectors()) {
        int maxDepth = 0;
        auto combinator = CssComplexSelector::Combinator::None;
        for(const auto& selector : subSelector) {
            combinator = selector.combinator();
            ++maxDepth;
        }

        if(combinator == CssComplexSelector::Combinator::None)
            combinator = CssComplexSelector::Combinator::Descendant;
        auto checkDescendants = [&](const Element* descendant) {
            int depth = 0;
            do {
                if(matchSelector(descendant, PseudoType::None, subSelector))
                    return true;
                if((combinator == CssComplexSelector::Combinator::Descendant || depth < maxDepth - 1)
                    && descendant->firstChildElement()) {
                    descendant = descendant->firstChildElement();
                    ++depth;
                    continue;
                }

                while(depth > 0) {
                    if(descendant->nextSiblingElement()) {
                        descendant = descendant->nextSiblingElement();
                        break;
                    }

                    descendant = descendant->parentElement();
                    --depth;
                }
            } while(descendant && depth > 0);
            return false;
        };

        switch(combinator) {
        case CssComplexSelector::Combinator::Descendant:
        case CssComplexSelector::Combinator::Child:
            for(auto child = element->firstChildElement(); child; child = child->nextSiblingElement()) {
                if(checkDescendants(child)) {
                    return true;
                }
            }

            return false;
        case CssComplexSelector::Combinator::DirectAdjacent:
        case CssComplexSelector::Combinator::InDirectAdjacent:
            for(auto sibling = element->nextSiblingElement(); sibling; sibling = sibling->nextSiblingElement()) {
                if(checkDescendants(sibling))
                    return true;
                if(combinator == CssComplexSelector::Combinator::DirectAdjacent) {
                    return false;
                }
            }

            break;
        case CssComplexSelector::Combinator::None:
            assert(false);
        }
    }

    return false;
}

bool CssRuleData::matchPseudoClassLinkSelector(const Element* element, const CssSimpleSelector& selector)
{
    return element->tagName() == aTag && element->hasAttribute(hrefAttr);
}

bool CssRuleData::matchPseudoClassLocalLinkSelector(const Element* element, const CssSimpleSelector& selector)
{
    if(matchPseudoClassLinkSelector(element, selector)) {
        const auto& baseUrl = element->document()->baseUrl();
        auto completeUrl = element->getUrlAttribute(hrefAttr);
        return baseUrl.value() == completeUrl.base();
    }

    return false;
}

bool CssRuleData::matchPseudoClassEnabledSelector(const Element* element, const CssSimpleSelector& selector)
{
    return element->tagName() == inputTag && element->hasAttribute(enabledAttr);
}

bool CssRuleData::matchPseudoClassDisabledSelector(const Element* element, const CssSimpleSelector& selector)
{
    return element->tagName() == inputTag && element->hasAttribute(disabledAttr);
}

bool CssRuleData::matchPseudoClassCheckedSelector(const Element* element, const CssSimpleSelector& selector)
{
    return element->tagName() == inputTag && element->hasAttribute(checkedAttr);
}

bool CssRuleData::matchPseudoClassLangSelector(const Element* element, const CssSimpleSelector& selector)
{
    return dashequals(element->lang(), selector.value(), false);
}

bool CssRuleData::matchPseudoClassRootSelector(const Element* element, const CssSimpleSelector& selector)
{
    return !element->parentElement();
}

bool CssRuleData::matchPseudoClassEmptySelector(const Element* element, const CssSimpleSelector& selector)
{
    return !element->firstChild();
}

bool CssRuleData::matchPseudoClassFirstChildSelector(const Element* element, const CssSimpleSelector& selector)
{
    return !element->previousSiblingElement();
}

bool CssRuleData::matchPseudoClassLastChildSelector(const Element* element, const CssSimpleSelector& selector)
{
    return !element->nextSiblingElement();
}

bool CssRuleData::matchPseudoClassOnlyChildSelector(const Element* element, const CssSimpleSelector& selector)
{
    return matchPseudoClassFirstChildSelector(element, selector) && matchPseudoClassLastChildSelector(element, selector);
}

bool CssRuleData::matchPseudoClassFirstOfTypeSelector(const Element* element, const CssSimpleSelector& selector)
{
    for(auto sibling = element->previousSiblingElement(); sibling; sibling = sibling->previousSiblingElement()) {
        if(sibling->isOfType(element->namespaceURI(), element->tagName())) {
            return false;
        }
    }

    return true;
}

bool CssRuleData::matchPseudoClassLastOfTypeSelector(const Element* element, const CssSimpleSelector& selector)
{
    for(auto sibling = element->nextSiblingElement(); sibling; sibling = sibling->nextSiblingElement()) {
        if(sibling->isOfType(element->namespaceURI(), element->tagName())) {
            return false;
        }
    }

    return true;
}

bool CssRuleData::matchPseudoClassOnlyOfTypeSelector(const Element* element, const CssSimpleSelector& selector)
{
    return matchPseudoClassFirstOfTypeSelector(element, selector) && matchPseudoClassLastOfTypeSelector(element, selector);
}

bool CssRuleData::matchPseudoClassNthChildSelector(const Element* element, const CssSimpleSelector& selector)
{
    int index = 0;
    for(auto sibling = element->previousSiblingElement(); sibling; sibling = sibling->previousSiblingElement())
        ++index;
    return selector.matchnth(index + 1);
}

bool CssRuleData::matchPseudoClassNthLastChildSelector(const Element* element, const CssSimpleSelector& selector)
{
    int index = 0;
    for(auto sibling = element->nextSiblingElement(); sibling; sibling = sibling->nextSiblingElement())
        ++index;
    return selector.matchnth(index + 1);
}

bool CssRuleData::matchPseudoClassNthOfTypeSelector(const Element* element, const CssSimpleSelector& selector)
{
    int index = 0;
    for(auto sibling = element->previousSiblingElement(); sibling; sibling = sibling->previousSiblingElement()) {
        if(sibling->isOfType(element->namespaceURI(), element->tagName())) {
            ++index;
        }
    }

    return selector.matchnth(index + 1);
}

bool CssRuleData::matchPseudoClassNthLastOfTypeSelector(const Element* element, const CssSimpleSelector& selector)
{
    int index = 0;
    for(auto sibling = element->nextSiblingElement(); sibling; sibling = sibling->nextSiblingElement()) {
        if(sibling->isOfType(element->namespaceURI(), element->tagName())) {
            ++index;
        }
    }

    return selector.matchnth(index + 1);
}

bool CssPageRuleData::match(GlobalString pageName, uint32_t pageIndex, PseudoType pseudoType) const
{
    if(m_selector) {
        for(const auto& sel : *m_selector) {
            if(!matchSelector(pageName, pageIndex, pseudoType, sel)) {
                return false;
            }
        }
    }

    return true;
}

bool CssPageRuleData::matchSelector(GlobalString pageName, uint32_t pageIndex, PseudoType pseudoType, const CssSimpleSelector& selector)
{
    switch(selector.matchType()) {
    case CssSimpleSelector::MatchType::PseudoPageName:
        return pageName == selector.name();
    case CssSimpleSelector::MatchType::PseudoPageFirst:
        return pseudoType == PseudoType::FirstPage;
    case CssSimpleSelector::MatchType::PseudoPageLeft:
        return pseudoType == PseudoType::LeftPage;
    case CssSimpleSelector::MatchType::PseudoPageRight:
        return pseudoType == PseudoType::RightPage;
    case CssSimpleSelector::MatchType::PseudoPageBlank:
        return pseudoType == PseudoType::BlankPage;
    case CssSimpleSelector::MatchType::PseudoPageNth:
        return selector.matchnth(pageIndex + 1);
    default:
        assert(false);
    }

    return false;
}

RefPtr<CssCounterStyle> CssCounterStyle::create(RefPtr<CssCounterStyleRule> rule)
{
    return adoptPtr(new CssCounterStyle(std::move(rule)));
}

static void cyclicAlgorithm(int value, size_t numSymbols, std::vector<size_t>& indexes)
{
    assert(numSymbols > 0);
    value %= numSymbols;
    value -= 1;
    if(value < 0) {
        value += numSymbols;
    }

    indexes.push_back(value);
}

static void fixedAlgorithm(int value, int firstSymbolValue, size_t numSymbols, std::vector<size_t>& indexes)
{
    assert(numSymbols > 0);
    if(value < firstSymbolValue || value - firstSymbolValue >= numSymbols)
        return;
    indexes.push_back(value - firstSymbolValue);
}

static void symbolicAlgorithm(unsigned value, size_t numSymbols, std::vector<size_t>& indexes)
{
    assert(numSymbols > 0);
    if(value == 0)
        return;
    size_t index = (value - 1) % numSymbols;
    size_t repetitions = (value + numSymbols - 1) / numSymbols;
    for(size_t i = 0; i < repetitions; ++i) {
        indexes.push_back(index);
    }
}

static void alphabeticAlgorithm(unsigned value, size_t numSymbols, std::vector<size_t>& indexes)
{
    assert(numSymbols > 0);
    if(value == 0 || numSymbols == 1)
        return;
    while(value > 0) {
        value -= 1;
        indexes.push_back(value % numSymbols);
        value /= numSymbols;
    }

    std::reverse(indexes.begin(), indexes.end());
}

static void numericAlgorithm(unsigned value, size_t numSymbols, std::vector<size_t>& indexes)
{
    assert(numSymbols > 0);
    if(numSymbols == 1)
        return;
    if(value == 0) {
        indexes.push_back(0);
        return;
    }

    while(value > 0) {
        indexes.push_back(value % numSymbols);
        value /= numSymbols;
    }

    std::reverse(indexes.begin(), indexes.end());
}

static const HeapString& counterStyleSymbol(const CssValue& value)
{
    if(is<CssStringValue>(value))
        return to<CssStringValue>(value).value();
    if(is<CssCustomIdentValue>(value))
        return to<CssCustomIdentValue>(value).value();
    return emptyGlo;
}

std::string CssCounterStyle::generateInitialRepresentation(int value) const
{
    std::string representation;
    if(system() == CssValueID::Additive) {
        if(m_additiveSymbols == nullptr)
            return representation;
        if(value == 0) {
            for(const auto& symbol : *m_additiveSymbols) {
                const auto& pair = to<CssPairValue>(*symbol);
                const auto& weight = to<CssIntegerValue>(*pair.first());
                if(weight.value() == 0) {
                    representation += counterStyleSymbol(*pair.second());
                    break;
                }
            }
        } else {
            for(const auto& symbol : *m_additiveSymbols) {
                const auto& pair = to<CssPairValue>(*symbol);
                const auto& weight = to<CssIntegerValue>(*pair.first());
                if(weight.value() == 0)
                    continue;
                size_t repetitions = value / weight.value();
                for(size_t i = 0; i < repetitions; ++i)
                    representation += counterStyleSymbol(*pair.second());
                value -= repetitions * weight.value();
                if(value == 0) {
                    break;
                }
            }

            if(value > 0) {
                representation.clear();
            }
        }

        return representation;
    }

    if(m_symbols == nullptr)
        return representation;
    std::vector<size_t> indexes;
    switch(system()) {
    case CssValueID::Cyclic:
        cyclicAlgorithm(value, m_symbols->size(), indexes);
        break;
    case CssValueID::Fixed:
        fixedAlgorithm(value, m_fixed->value(), m_symbols->size(), indexes);
        break;
    case CssValueID::Numeric:
        numericAlgorithm(value, m_symbols->size(), indexes);
        break;
    case CssValueID::Symbolic:
        symbolicAlgorithm(value, m_symbols->size(), indexes);
        break;
    case CssValueID::Alphabetic:
        alphabeticAlgorithm(value, m_symbols->size(), indexes);
        break;
    default:
        assert(false);
    }

    for(auto index : indexes) {
        representation += counterStyleSymbol(*(*m_symbols)[index]);
    }

    return representation;
}

std::string CssCounterStyle::generateFallbackRepresentation(int value) const
{
    if(m_fallbackStyle == nullptr)
        return defaultStyle().generateRepresentation(value);
    auto fallbackStyle = std::move(m_fallbackStyle);
    auto representation = fallbackStyle->generateRepresentation(value);
    m_fallbackStyle = std::move(fallbackStyle);
    return representation;
}

static size_t counterStyleSymbolLength(const std::string_view& value)
{
    UCharIterator it;
    uiter_setUTF8(&it, value.data(), value.length());

    size_t count = 0;
    while(it.hasNext(&it)) {
        uiter_next32(&it);
        ++count;
    }

    return count;
}

std::string CssCounterStyle::generateRepresentation(int value) const
{
    if(!rangeContains(value))
        return generateFallbackRepresentation(value);
    auto initialRepresentation = generateInitialRepresentation(std::abs(value));
    if(initialRepresentation.empty()) {
        return generateFallbackRepresentation(value);
    }

    std::string_view negativePrefix("-");
    std::string_view negativeSuffix;
    if(m_negative && needsNegativeSign(value)) {
        if(auto pair = to<CssPairValue>(m_negative)) {
            negativePrefix = counterStyleSymbol(*pair->first());
            negativeSuffix = counterStyleSymbol(*pair->second());
        } else {
            negativePrefix = counterStyleSymbol(*m_negative);
        }
    }

    size_t padLength = 0;
    std::string_view padSymbol;
    if(m_pad) {
        padLength = to<CssIntegerValue>(*m_pad->first()).value();
        padSymbol = counterStyleSymbol(*m_pad->second());
    }

    auto initialLength = counterStyleSymbolLength(initialRepresentation);
    if(needsNegativeSign(value)) {
        initialLength += counterStyleSymbolLength(negativePrefix);
        initialLength += counterStyleSymbolLength(negativeSuffix);
    }

    size_t padRepetitions = 0;
    if(padLength > initialLength) {
        padRepetitions = padLength - initialLength;
    }

    std::string representation;
    if(needsNegativeSign(value))
        representation += negativePrefix;
    for(size_t i = 0; i < padRepetitions; ++i)
        representation += padSymbol;
    representation += initialRepresentation;
    if(needsNegativeSign(value))
        representation += negativeSuffix;
    return representation;
}

bool CssCounterStyle::rangeContains(int value) const
{
    if(m_range == nullptr) {
        switch(system()) {
        case CssValueID::Cyclic:
        case CssValueID::Numeric:
        case CssValueID::Fixed:
            return true;
        case CssValueID::Symbolic:
        case CssValueID::Alphabetic:
            return value >= 1;
        case CssValueID::Additive:
            return value >= 0;
        default:
            assert(false);
        }
    }

    for(const auto& range : *m_range) {
        const auto& bounds = to<CssPairValue>(*range);
        auto lowerBound = std::numeric_limits<int>::min();
        auto upperBound = std::numeric_limits<int>::max();
        if(auto bound = to<CssIntegerValue>(bounds.first()))
            lowerBound = bound->value();
        if(auto bound = to<CssIntegerValue>(bounds.second()))
            upperBound = bound->value();
        if(value >= lowerBound && value <= upperBound) {
            return true;
        }
    }

    return false;
}

bool CssCounterStyle::needsNegativeSign(int value) const
{
    if(value < 0) {
        switch(system()) {
        case CssValueID::Symbolic:
        case CssValueID::Alphabetic:
        case CssValueID::Numeric:
        case CssValueID::Additive:
            return true;
        case CssValueID::Cyclic:
        case CssValueID::Fixed:
            return false;
        default:
            assert(false);
        }
    }

    return false;
}

GlobalString CssCounterStyle::name() const
{
    return m_rule->name();
}

GlobalString CssCounterStyle::extendsName() const
{
    if(m_extends)
        return m_extends->value();
    return emptyGlo;
}

GlobalString CssCounterStyle::fallbackName() const
{
    static const GlobalString defaultFallback("decimal");
    if(m_fallback)
        return m_fallback->value();
    return defaultFallback;
}

const CssValueID CssCounterStyle::system() const
{
    if(m_system)
        return m_system->value();
    return CssValueID::Symbolic;
}

const HeapString& CssCounterStyle::prefix() const
{
    if(m_prefix)
        return counterStyleSymbol(*m_prefix);
    return emptyGlo;
}

const HeapString& CssCounterStyle::suffix() const
{
    static const GlobalString defaultSuffix(". ");
    if(m_suffix)
        return counterStyleSymbol(*m_suffix);
    return defaultSuffix;
}

void CssCounterStyle::extend(const CssCounterStyle& extended)
{
    assert(m_system && m_system->value() == CssValueID::Extends);
    m_system = extended.m_system;
    m_fixed = extended.m_fixed;
    m_symbols = extended.m_symbols;
    m_additiveSymbols = extended.m_additiveSymbols;

    if(!m_negative) { m_negative = extended.m_negative; }
    if(!m_prefix) { m_prefix = extended.m_prefix; }
    if(!m_suffix) { m_suffix = extended.m_suffix; }
    if(!m_range) { m_range = extended.m_range; }
    if(!m_pad) { m_pad = extended.m_pad; }
}

CssCounterStyle& CssCounterStyle::defaultStyle()
{
    static CssCounterStyle* defaultStyle = []() {
        const GlobalString decimal("decimal");
        return userAgentCounterStyleMap()->findCounterStyle(decimal);
    }();

    return *defaultStyle;
}

CssCounterStyle::CssCounterStyle(RefPtr<CssCounterStyleRule> rule)
    : m_rule(std::move(rule))
{
    for(const auto& property : m_rule->properties()) {
        switch(property.id()) {
        case CssPropertyID::System: {
            m_system = to<CssIdentValue>(property.value());
            if(m_system == nullptr) {
                const auto& pair = to<CssPairValue>(*property.value());
                m_system = to<CssIdentValue>(*pair.first());
                if(m_system->value() == CssValueID::Fixed) {
                    m_fixed = to<CssIntegerValue>(*pair.second());
                } else {
                    m_extends = to<CssCustomIdentValue>(*pair.second());
                }
            }

            break;
        }

        case CssPropertyID::Symbols:
            m_symbols = to<CssListValue>(*property.value());
            break;
        case CssPropertyID::AdditiveSymbols:
            m_additiveSymbols = to<CssListValue>(*property.value());
            break;
        case CssPropertyID::Fallback:
            m_fallback = to<CssCustomIdentValue>(*property.value());
            break;
        case CssPropertyID::Pad:
            m_pad = to<CssPairValue>(*property.value());
            break;
        case CssPropertyID::Range:
            m_range = to<CssListValue>(property.value());
            break;
        case CssPropertyID::Negative:
            m_negative = property.value();
            break;
        case CssPropertyID::Prefix:
            m_prefix = property.value();
            break;
        case CssPropertyID::Suffix:
            m_suffix = property.value();
            break;
        default:
            assert(false);
        }
    }
}

std::unique_ptr<CssCounterStyleMap> CssCounterStyleMap::create(const CssRuleList& rules, const CssCounterStyleMap* parent)
{
    return std::unique_ptr<CssCounterStyleMap>(new CssCounterStyleMap(rules, parent));
}

CssCounterStyle* CssCounterStyleMap::findCounterStyle(GlobalString name) const
{
    auto it = m_counterStyles.find(name);
    if(it != m_counterStyles.end())
        return it->second.get();
    if(m_parent == nullptr)
        return nullptr;
    return m_parent->findCounterStyle(name);
}

CssCounterStyleMap::CssCounterStyleMap(const CssRuleList& rules, const CssCounterStyleMap* parent)
    : m_parent(parent)
{
    for(const auto& rule : rules) {
        auto& counterStyleRule = to<CssCounterStyleRule>(*rule);
        auto counterStyle = CssCounterStyle::create(counterStyleRule);
        m_counterStyles.emplace(counterStyle->name(), std::move(counterStyle));
    }

    for(const auto& [name, style] : m_counterStyles) {
        if(style->system() == CssValueID::Extends) {
            boost::unordered_flat_set<CssCounterStyle*> unresolvedStyles;
            std::vector<CssCounterStyle*> extendsStyles;

            extendsStyles.push_back(style.get());
            auto currentStyle = extendsStyles.back();
            do {
                unresolvedStyles.insert(currentStyle);
                extendsStyles.push_back(findCounterStyle(currentStyle->extendsName()));
                currentStyle = extendsStyles.back();
            } while(currentStyle && currentStyle->system() == CssValueID::Extends && !unresolvedStyles.contains(currentStyle));

            if(currentStyle && currentStyle->system() == CssValueID::Extends) {
                assert(parent != nullptr);
                do {
                    extendsStyles.back()->extend(CssCounterStyle::defaultStyle());
                    extendsStyles.pop_back();
                } while(currentStyle != extendsStyles.back());
            }

            while(extendsStyles.size() > 1) {
                extendsStyles.pop_back();
                if(currentStyle == nullptr) {
                    assert(parent != nullptr);
                    extendsStyles.back()->extend(CssCounterStyle::defaultStyle());
                } else {
                    extendsStyles.back()->extend(*currentStyle);
                }

                currentStyle = extendsStyles.back();
            }
        }

        if(auto fallbackStyle = findCounterStyle(style->fallbackName())) {
            style->setFallbackStyle(*fallbackStyle);
        } else {
            assert(parent != nullptr);
            style->setFallbackStyle(CssCounterStyle::defaultStyle());
        }
    }
}

const CssCounterStyleMap* userAgentCounterStyleMap()
{
    static std::unique_ptr<CssCounterStyleMap> counterStyleMap = []() {
        CssParserContext context(nullptr, CssStyleOrigin::UserAgent, ResourceLoader::baseUrl());
        CssParser parser(context);
        CssRuleList rules(parser.parseSheet(kUserAgentCounterStyle));
        return CssCounterStyleMap::create(rules, nullptr);
    }();

    return counterStyleMap.get();
}

} // namespace plutobook
