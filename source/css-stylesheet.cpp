#include "css-stylesheet.h"
#include "ua-stylesheet.h"
#include "css-parser.h"
#include "text-resource.h"
#include "font-resource.h"
#include "document.h"
#include "box-style.h"

#include "plutobook.hpp"
#include <span>

namespace plutobook {

CssFontFaceCache::CssFontFaceCache()
{
}

RefPtr<FontData> CssFontFaceCache::get(GlobalString family, const FontDataDescription& description) const
{
    auto it = m_table.find(family);
    if(it == m_table.end())
        return fontDataCache()->getFontData(family, description);
    FontSelectionAlgorithm algorithm(description.request);
    for(const auto& item : it->second) {
        algorithm.addCandidate(item.first);
    }

    RefPtr<SegmentedFontFace> face;
    for(const auto& item : it->second) {
        if(face == nullptr || algorithm.isCandidateBetter(item.first, face->description())) {
            face = item.second;
        }
    }

    return face->getFontData(description);
}

void CssFontFaceCache::add(GlobalString family, const FontSelectionDescription& description, RefPtr<FontFace> face)
{
    auto& fontFace = m_table[family][description];
    if(fontFace == nullptr)
        fontFace = SegmentedFontFace::create(description);
    fontFace->add(std::move(face));
}

class CssPropertyData : public CssProperty {
public:
    CssPropertyData(uint32_t specificity, uint32_t position, const CssProperty& property)
        : CssProperty(property),  m_specificity(specificity), m_position(position)
    {}

    uint32_t specificity() const { return m_specificity; }
    uint32_t position() const { return m_position; }

    bool isLessThan(const CssPropertyData& data) const;

private:
    uint32_t m_specificity;
    uint32_t m_position;
};

inline bool CssPropertyData::isLessThan(const CssPropertyData& data) const
{
    if(m_precedence != data.m_precedence)
        return m_precedence < data.m_precedence;
    if(m_specificity != data.m_specificity)
        return m_specificity < data.m_specificity;
    return m_position < data.m_position;
}

using CssPropertyDataList = std::vector<CssPropertyData>;

class FontDescriptionBuilder {
public:
    FontDescriptionBuilder(const BoxStyle* parentStyle, std::span<const CssPropertyData> properties);

    FontFamilyList family() const;
    FontSelectionValue size() const;
    FontSelectionValue weight() const;
    FontSelectionValue stretch() const;
    FontSelectionValue slope() const;
    FontVariationList variationSettings() const;

    FontDescription build() const;

private:
    const BoxStyle* m_parentStyle;
    RefPtr<CssValue> m_family;
    RefPtr<CssValue> m_size;
    RefPtr<CssValue> m_weight;
    RefPtr<CssValue> m_stretch;
    RefPtr<CssValue> m_style;
    RefPtr<CssValue> m_variationSettings;
};

FontDescriptionBuilder::FontDescriptionBuilder(const BoxStyle* parentStyle, std::span<const CssPropertyData> properties)
    : m_parentStyle(parentStyle)
{
    for(const auto& property : properties) {
        if(is<CssInheritValue>(*property.value())
            || is<CssUnsetValue>(*property.value())
            || is<CssVariableReferenceValue>(*property.value())) {
            continue;
        }

        switch(property.id()) {
        case CssPropertyID::FontFamily:
            m_family = property.value();
            break;
        case CssPropertyID::FontSize:
            m_size = property.value();
            break;
        case CssPropertyID::FontWeight:
            m_weight = property.value();
            break;
        case CssPropertyID::FontStretch:
            m_stretch = property.value();
            break;
        case CssPropertyID::FontStyle:
            m_style = property.value();
            break;
        case CssPropertyID::FontVariationSettings:
            m_variationSettings = property.value();
            break;
        default:
            break;
        }
    }
}

FontFamilyList FontDescriptionBuilder::family() const
{
    if(m_family == nullptr)
        return m_parentStyle->fontFamily();
    if(is<CssInitialValue>(*m_family))
        return FontFamilyList();
    FontFamilyList families;
    for(const auto& family : to<CssListValue>(*m_family)) {
        const auto& name = to<CssCustomIdentValue>(*family);
        families.push_front(name.value());
    }

    families.reverse();
    return families;
}

FontSelectionValue FontDescriptionBuilder::size() const
{
    if(m_size == nullptr)
        return m_parentStyle->fontSize();
    if(is<CssInitialValue>(*m_size))
        return kMediumFontSize;
    if(auto ident = to<CssIdentValue>(m_size)) {
        switch(ident->value()) {
        case CssValueID::XxSmall:
            return kMediumFontSize * 0.6;
        case CssValueID::XSmall:
            return kMediumFontSize * 0.75;
        case CssValueID::Small:
            return kMediumFontSize * 0.89;
        case CssValueID::Medium:
            return kMediumFontSize * 1.0;
        case CssValueID::Large:
            return kMediumFontSize * 1.2;
        case CssValueID::XLarge:
            return kMediumFontSize * 1.5;
        case CssValueID::XxLarge:
            return kMediumFontSize * 2.0;
        case CssValueID::XxxLarge:
            return kMediumFontSize * 3.0;
        case CssValueID::Smaller:
            return m_parentStyle->fontSize() / 1.2;
        case CssValueID::Larger:
            return m_parentStyle->fontSize() * 1.2;
        default:
            assert(false);
        }
    }

    if(auto percent = to<CssPercentValue>(m_size))
        return percent->value() * m_parentStyle->fontSize() / 100.0;
    return m_parentStyle->convertLengthValue(*m_size);
}

constexpr FontSelectionValue lighterFontWeight(FontSelectionValue weight)
{
    assert(weight >= kMinFontWeight && weight <= kMaxFontWeight);
    if(weight < FontSelectionValue(100))
        return weight;
    if(weight < FontSelectionValue(550))
        return FontSelectionValue(100);
    if(weight < FontSelectionValue(750))
        return FontSelectionValue(400);
    return FontSelectionValue(700);
}

constexpr FontSelectionValue bolderFontWeight(FontSelectionValue weight)
{
    assert(weight >= kMinFontWeight && weight <= kMaxFontWeight);
    if(weight < FontSelectionValue(350))
        return FontSelectionValue(400);
    if(weight < FontSelectionValue(550))
        return FontSelectionValue(700);
    if(weight < FontSelectionValue(900))
        return FontSelectionValue(900);
    return weight;
}

static FontSelectionValue convertFontWeightNumber(const CssValue& value)
{
    return std::clamp<FontSelectionValue>(to<CssNumberValue>(value).value(), kMinFontWeight, kMaxFontWeight);
}

FontSelectionValue FontDescriptionBuilder::weight() const
{
    if(m_weight == nullptr)
        return m_parentStyle->fontWeight();
    if(is<CssInitialValue>(*m_weight))
        return kNormalFontWeight;
    if(auto ident = to<CssIdentValue>(m_weight)) {
        switch(ident->value()) {
        case CssValueID::Normal:
            return kNormalFontWeight;
        case CssValueID::Bold:
            return kBoldFontWeight;
        case CssValueID::Lighter:
            return lighterFontWeight(m_parentStyle->fontWeight());
        case CssValueID::Bolder:
            return bolderFontWeight(m_parentStyle->fontWeight());
        default:
            assert(false);
        }
    }

    return convertFontWeightNumber(*m_weight);
}

static FontSelectionValue convertFontStretchIdent(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::UltraCondensed:
        return kUltraCondensedFontWidth;
    case CssValueID::ExtraCondensed:
        return kExtraCondensedFontWidth;
    case CssValueID::Condensed:
        return kCondensedFontWidth;
    case CssValueID::SemiCondensed:
        return kSemiCondensedFontWidth;
    case CssValueID::Normal:
        return kNormalFontWidth;
    case CssValueID::SemiExpanded:
        return kSemiExpandedFontWidth;
    case CssValueID::Expanded:
        return kExpandedFontWidth;
    case CssValueID::ExtraExpanded:
        return kExtraExpandedFontWidth;
    case CssValueID::UltraExpanded:
        return kUltraExpandedFontWidth;
    default:
        assert(false);
    }

    return kNormalFontWidth;
}

FontSelectionValue FontDescriptionBuilder::stretch() const
{
    if(m_stretch == nullptr)
        return m_parentStyle->fontStretch();
    if(is<CssInitialValue>(*m_stretch))
        return kNormalFontWidth;
    if(auto percent = to<CssPercentValue>(m_stretch))
        return percent->value();
    return convertFontStretchIdent(*m_stretch);
}

static FontSelectionValue convertFontSlopeIdent(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Normal:
        return kNormalFontSlope;
    case CssValueID::Italic:
        return kItalicFontSlope;
    case CssValueID::Oblique:
        return kObliqueFontSlope;
    default:
        assert(false);
    }

    return kNormalFontSlope;
}

static FontSelectionValue convertFontSlopeAngle(const CssValue& value)
{
    return std::clamp<FontSelectionValue>(to<CssAngleValue>(value).valueInDegrees(), kMinFontSlope, kMaxFontSlope);
}

FontSelectionValue FontDescriptionBuilder::slope() const
{
    if(m_style == nullptr)
        return m_parentStyle->fontSlope();
    if(is<CssInitialValue>(*m_style))
        return kNormalFontSlope;
    if(auto ident = to<CssIdentValue>(m_style))
        return convertFontSlopeIdent(*ident);
    const auto& pair = to<CssPairValue>(*m_style);
    const auto& ident = to<CssIdentValue>(*pair.first());
    assert(ident.value() == CssValueID::Oblique);
    return convertFontSlopeAngle(*pair.second());
}

FontVariationList FontDescriptionBuilder::variationSettings() const
{
    if(m_variationSettings == nullptr)
        return m_parentStyle->fontVariationSettings();
    if(is<CssInitialValue>(*m_variationSettings))
        return FontVariationList();
    FontVariationList variationSettings;
    if(auto ident = to<CssIdentValue>(m_variationSettings)) {
        assert(ident->value() == CssValueID::Normal);
        return variationSettings;
    }

    for(const auto& value : to<CssListValue>(*m_variationSettings)) {
        const auto& variation = to<CssFontVariationValue>(*value);
        variationSettings.emplace_front(variation.tag(), variation.value());
    }

    variationSettings.sort();
    variationSettings.unique();
    return variationSettings;
}

FontDescription FontDescriptionBuilder::build() const
{
    FontDescription description;
    description.families = family();
    description.data.size = size();
    description.data.request.weight = weight();
    description.data.request.width = stretch();
    description.data.request.slope = slope();
    description.data.variations = variationSettings();
    return description;
}

class StyleBuilder {
protected:
    StyleBuilder(const BoxStyle* parentStyle, PseudoType pseudoType)
        : m_parentStyle(parentStyle), m_pseudoType(pseudoType)
    {}

    std::span<const CssPropertyData> properties() const {
        return {m_allProperties.data(), m_propertyCount};
    }

    std::span<const CssPropertyData> customProperties() const {
        return {m_allProperties.data() + m_propertyCount, m_allProperties.size() - m_propertyCount};
    }

    FontDescription fontDescription() const {
        return FontDescriptionBuilder(m_parentStyle, properties()).build();
    }

    void merge(uint32_t specificity, uint32_t position, const CssPropertyList& properties);
    void buildStyle(BoxStyle* newStyle);

    CssPropertyDataList m_allProperties; // normal + custom
    const BoxStyle* m_parentStyle;
    unsigned m_propertyCount = 0;
    PseudoType m_pseudoType;
};

void StyleBuilder::merge(uint32_t specificity, uint32_t position, const CssPropertyList& properties)
{
    for(const auto& property : properties) {
        CssPropertyData data(specificity, position, property);
        auto it = m_allProperties.begin();
        const auto mid = it + m_propertyCount;
        if (property.id() == CssPropertyID::Custom) {
            constexpr auto getKey = [](const CssProperty& item) {
                const auto& custom = to<CssCustomPropertyValue>(*item.value());
                return custom.name();
            };
            const auto key = getKey(property);
            it = std::ranges::lower_bound(mid, m_allProperties.end(), key, std::ranges::less{}, getKey);
            if (it == m_allProperties.end() || getKey(*it) != key) {
                m_allProperties.insert(it, std::move(data));
                continue;
            }
        } else {
            constexpr auto getKey = [](const CssProperty& item) {
                return item.id();
            };
            const auto key = getKey(property);
            it = std::ranges::lower_bound(it, mid, key, std::ranges::less{}, getKey);
            if (it == mid || getKey(*it) != key) {
                m_allProperties.insert(it, std::move(data));
                ++m_propertyCount;
                continue;
            }
        }
        if (!data.isLessThan(*it)) {
            *it = std::move(data);
        }
    }
}

void StyleBuilder::buildStyle(BoxStyle* newStyle)
{
    CssPropertyDataList variables;
    for(const auto& property : properties()) {
        if(is<CssVariableReferenceValue>(*property.value())) {
            variables.push_back(property);
        }
    }
    for (const auto& property : customProperties()) {
        const auto& custom = to<CssCustomPropertyValue>(*property.value());
        newStyle->setCustom(custom.name(), custom.value());
    }

    for(const auto& variable : variables) {
        const auto& value = to<CssVariableReferenceValue>(*variable.value());
        merge(variable.specificity(), variable.position(), value.resolve(newStyle));
    }

    newStyle->setFontDescription(fontDescription());

    for(const auto& property : properties()) {
        const auto id = property.id();
        switch(id) {
        case CssPropertyID::FontFamily:
        case CssPropertyID::FontSize:
        case CssPropertyID::FontWeight:
        case CssPropertyID::FontStretch:
        case CssPropertyID::FontStyle:
        case CssPropertyID::FontVariationSettings:
            continue;
        }

        auto value = property.value();
        switch (value->type()) {
        case CssValueType::Unset:
        case CssValueType::VariableReference:
            continue;
        case CssValueType::Initial:
            newStyle->reset(id);
            continue;
        case CssValueType::Inherit:
            value = m_parentStyle->get(id);
            if (!value)
                continue;
            break;
        }

        if(is<CssLengthValue>(*value) || is<CssCalcValue>(*value))
            value = newStyle->resolveLength(value);
        newStyle->set(id, std::move(value));
    }
}

class ElementStyleBuilder final : public StyleBuilder {
public:
    ElementStyleBuilder(Element* element, PseudoType pseudoType, const BoxStyle* parentStyle)
        : StyleBuilder(parentStyle, pseudoType)
        , m_element(element)
    {}

    void add(const CssRuleDataList& rules) {
        for (const auto& rule : rules) {
            if (rule.match(m_element, m_pseudoType)) {
                merge(rule.specificity(), rule.position(), rule.properties());
            }
        }
    }

    RefPtr<BoxStyle> build();

private:
    Element* m_element;
};

RefPtr<BoxStyle> ElementStyleBuilder::build()
{
    if(m_pseudoType == PseudoType::None) {
        AttributeStyle attrStyle(m_element);
        m_element->collectAttributeStyle(attrStyle);
        merge(0, 0, attrStyle.properties());
        merge(0, 0, m_element->inlineStyle());
    }

    if(m_allProperties.empty()) {
        if(m_pseudoType == PseudoType::None) {
            if(m_element->isRootNode() || m_parentStyle->isDisplayFlex())
                return BoxStyle::create(m_element, m_parentStyle, m_pseudoType, Display::Block);
            return BoxStyle::create(m_element, m_parentStyle, m_pseudoType, Display::Inline);
        }

        if(m_pseudoType == PseudoType::Marker)
            return BoxStyle::create(m_element, m_parentStyle, m_pseudoType, Display::Inline);
        return nullptr;
    }

    auto newStyle = BoxStyle::create(m_element, m_parentStyle, m_pseudoType, Display::Inline);
    buildStyle(newStyle.get());
    if(newStyle->display() == Display::None)
        return newStyle;
    if(newStyle->position() == Position::Static && !m_parentStyle->isDisplayFlex())
        newStyle->reset(CssPropertyID::ZIndex);
    if(m_pseudoType == PseudoType::FirstLetter) {
        newStyle->setPosition(Position::Static);
        if(newStyle->isFloating()) {
            newStyle->setDisplay(Display::Block);
        } else {
            newStyle->setDisplay(Display::Inline);
        }
    }

    if(newStyle->isFloating() || newStyle->isPositioned() || m_element->isRootNode() || m_parentStyle->isDisplayFlex()) {
        switch(newStyle->display()) {
        case Display::Inline:
        case Display::InlineBlock:
            newStyle->setDisplay(Display::Block);
            break;
        case Display::InlineTable:
            newStyle->setDisplay(Display::Table);
            break;
        case Display::InlineFlex:
            newStyle->setDisplay(Display::Flex);
            break;
        case Display::TableCaption:
        case Display::TableCell:
        case Display::TableColumn:
        case Display::TableColumnGroup:
        case Display::TableFooterGroup:
        case Display::TableHeaderGroup:
        case Display::TableRow:
        case Display::TableRowGroup:
            newStyle->setDisplay(Display::Block);
            break;
        default:
            break;
        }
    }

    if(newStyle->isPositioned() || m_parentStyle->isDisplayFlex())
        newStyle->setFloating(Float::None);
    return newStyle;
}

class PageStyleBuilder final : public StyleBuilder {
public:
    PageStyleBuilder(GlobalString pageName, uint32_t pageIndex, PageMarginType marginType, PseudoType pseudoType, const BoxStyle* parentStyle);

    void add(const CssPageRuleDataList& rules);
    RefPtr<BoxStyle> build();

private:
    GlobalString m_pageName;
    uint32_t m_pageIndex;
    PageMarginType m_marginType;
};

PageStyleBuilder::PageStyleBuilder(GlobalString pageName, uint32_t pageIndex, PageMarginType marginType, PseudoType pseudoType, const BoxStyle* parentStyle)
    : StyleBuilder(parentStyle, pseudoType)
    , m_pageName(pageName)
    , m_pageIndex(pageIndex)
    , m_marginType(marginType)
{
}

void PageStyleBuilder::add(const CssPageRuleDataList& rules)
{
    for(const auto& rule : rules) {
        if(rule.match(m_pageName, m_pageIndex, m_pseudoType)) {
            if(m_marginType == PageMarginType::None) {
                merge(rule.specificity(), rule.position(), rule.properties());
            } else {
                for(const auto& margin : rule.margins()) {
                    if(m_marginType == margin->marginType()) {
                        merge(rule.specificity(), rule.position(), margin->properties());
                    }
                }
            }
        }
    }
}

RefPtr<BoxStyle> PageStyleBuilder::build()
{
    if(m_allProperties.empty()) {
        if(m_marginType == PageMarginType::None)
            return BoxStyle::create(m_parentStyle, m_pseudoType, Display::Block);
        return nullptr;
    }

    auto newStyle = BoxStyle::create(m_parentStyle, m_pseudoType, Display::Block);
    switch(m_marginType) {
    case PageMarginType::TopLeftCorner:
        newStyle->setTextAlign(TextAlign::Right);
        newStyle->setVerticalAlignType(VerticalAlignType::Middle);
        break;
    case PageMarginType::TopLeft:
        newStyle->setTextAlign(TextAlign::Left);
        newStyle->setVerticalAlignType(VerticalAlignType::Middle);
        break;
    case PageMarginType::TopCenter:
        newStyle->setTextAlign(TextAlign::Center);
        newStyle->setVerticalAlignType(VerticalAlignType::Middle);
        break;
    case PageMarginType::TopRight:
        newStyle->setTextAlign(TextAlign::Right);
        newStyle->setVerticalAlignType(VerticalAlignType::Middle);
        break;
    case PageMarginType::TopRightCorner:
        newStyle->setTextAlign(TextAlign::Left);
        newStyle->setVerticalAlignType(VerticalAlignType::Middle);
        break;
    case PageMarginType::RightTop:
        newStyle->setTextAlign(TextAlign::Center);
        newStyle->setVerticalAlignType(VerticalAlignType::Top);
        break;
    case PageMarginType::RightMiddle:
        newStyle->setTextAlign(TextAlign::Center);
        newStyle->setVerticalAlignType(VerticalAlignType::Middle);
        break;
    case PageMarginType::RightBottom:
        newStyle->setTextAlign(TextAlign::Center);
        newStyle->setVerticalAlignType(VerticalAlignType::Bottom);
        break;
    case PageMarginType::BottomRightCorner:
        newStyle->setTextAlign(TextAlign::Left);
        newStyle->setVerticalAlignType(VerticalAlignType::Middle);
        break;
    case PageMarginType::BottomRight:
        newStyle->setTextAlign(TextAlign::Right);
        newStyle->setVerticalAlignType(VerticalAlignType::Middle);
        break;
    case PageMarginType::BottomCenter:
        newStyle->setTextAlign(TextAlign::Center);
        newStyle->setVerticalAlignType(VerticalAlignType::Middle);
        break;
    case PageMarginType::BottomLeft:
        newStyle->setTextAlign(TextAlign::Left);
        newStyle->setVerticalAlignType(VerticalAlignType::Middle);
        break;
    case PageMarginType::BottomLeftCorner:
        newStyle->setTextAlign(TextAlign::Right);
        newStyle->setVerticalAlignType(VerticalAlignType::Middle);
        break;
    case PageMarginType::LeftBottom:
        newStyle->setTextAlign(TextAlign::Center);
        newStyle->setVerticalAlignType(VerticalAlignType::Bottom);
        break;
    case PageMarginType::LeftMiddle:
        newStyle->setTextAlign(TextAlign::Center);
        newStyle->setVerticalAlignType(VerticalAlignType::Middle);
        break;
    case PageMarginType::LeftTop:
        newStyle->setTextAlign(TextAlign::Center);
        newStyle->setVerticalAlignType(VerticalAlignType::Top);
        break;
    case PageMarginType::None:
        break;
    }

    buildStyle(newStyle.get());
    newStyle->setPosition(Position::Static);
    newStyle->setDisplay(Display::Block);
    newStyle->setFloating(Float::None);
    return newStyle;
}

static const CssRuleList& userAgentRules()
{
    static CssRuleList rules = []() {
        CssParserContext context(nullptr, CssStyleOrigin::UserAgent, ResourceLoader::baseUrl());
        CssParser parser(context);
        return parser.parseSheet(kUserAgentStyle);
    }();

    return rules;
}

CssStyleSheet::CssStyleSheet(Document* document)
    : m_document(document)
{
    if(document->book()) {
        addRuleList(userAgentRules());
    }
}

RefPtr<BoxStyle> CssStyleSheet::styleForElement(Element* element, const BoxStyle* parentStyle) const
{
    ElementStyleBuilder builder(element, PseudoType::None, parentStyle);
    for (const auto& className : element->classNames()) {
        if (const auto rules = m_classRules.get(className))
            builder.add(*rules);
    }
    for (const auto& attribute : element->attributes()) {
        if (const auto rules = m_attributeRules.get(element->foldCase(attribute.name())))
            builder.add(*rules);
    }
    if (const auto rules = m_tagRules.get(element->foldTagNameCase()))
        builder.add(*rules);
    if (const auto rules = m_idRules.get(element->id()))
        builder.add(*rules);
    builder.add(m_universalRules);
    return builder.build();
}

RefPtr<BoxStyle> CssStyleSheet::pseudoStyleForElement(Element* element, PseudoType pseudoType, const BoxStyle* parentStyle) const
{
    ElementStyleBuilder builder(element, pseudoType, parentStyle);
    if (const auto rules = m_pseudoRules.get(pseudoType))
        builder.add(*rules);
    return builder.build();
}

RefPtr<BoxStyle> CssStyleSheet::styleForPage(GlobalString pageName, uint32_t pageIndex, PseudoType pseudoType) const
{
    PageStyleBuilder builder(pageName, pageIndex, PageMarginType::None, pseudoType, m_document->rootStyle());
    builder.add(m_pageRules);
    return builder.build();
}

RefPtr<BoxStyle> CssStyleSheet::styleForPageMargin(GlobalString pageName, uint32_t pageIndex, PageMarginType marginType, const BoxStyle* pageStyle) const
{
    PageStyleBuilder builder(pageName, pageIndex, marginType, pageStyle->pseudoType(), pageStyle);
    builder.add(m_pageRules);
    return builder.build();
}

RefPtr<FontData> CssStyleSheet::getFontData(GlobalString family, const FontDataDescription& description) const
{
    return m_fontFaceCache.get(family, description);
}

const CssCounterStyle& CssStyleSheet::getCounterStyle(GlobalString name)
{
    auto counterStyleMap = userAgentCounterStyleMap();
    if(!m_counterStyleRules.empty()) {
        if(m_counterStyleMap == nullptr)
            m_counterStyleMap = CssCounterStyleMap::create(m_counterStyleRules, counterStyleMap);
        counterStyleMap = m_counterStyleMap.get();
    }

    if(auto counterStyle = counterStyleMap->findCounterStyle(name))
        return *counterStyle;
    return CssCounterStyle::defaultStyle();
}

std::string CssStyleSheet::getCounterText(int value, GlobalString listType)
{
    return getCounterStyle(listType).generateRepresentation(value);
}

std::string CssStyleSheet::getMarkerText(int value, GlobalString listType)
{
    const auto& counterStyle = getCounterStyle(listType);
    std::string representation(counterStyle.prefix());
    representation += counterStyle.generateRepresentation(value);
    representation += counterStyle.suffix();
    return representation;
}

void CssStyleSheet::parseStyle(const std::string_view& content, CssStyleOrigin origin, Url baseUrl)
{
    CssParserContext context(m_document, origin, std::move(baseUrl));
    CssParser parser(context);
    addRuleList(parser.parseSheet(content));
}

void CssStyleSheet::addRuleList(const CssRuleList& rules)
{
    for(const auto& rule : rules) {
        switch(rule->type()) {
        case CssRuleType::Style:
            addStyleRule(to<CssStyleRule>(rule));
            break;
        case CssRuleType::Import:
            addImportRule(to<CssImportRule>(rule));
            break;
        case CssRuleType::Page:
            addPageRule(to<CssPageRule>(rule));
            break;
        case CssRuleType::FontFace:
            addFontFaceRule(to<CssFontFaceRule>(rule));
            break;
        case CssRuleType::CounterStyle:
            addCounterStyleRule(to<CssCounterStyleRule>(rule));
            break;
        case CssRuleType::Media:
            addMediaRule(to<CssMediaRule>(rule));
            break;
        default:
            break;
        }

        m_position += 1;
    }
}

void CssStyleSheet::addStyleRule(const RefPtr<CssStyleRule>& rule)
{
    for(const auto& selector : rule->selectors()) {
        uint32_t specificity = 0;
        for(const auto& complexSelector : selector) {
            for(const auto& simpleSelector : complexSelector.compoundSelector()) {
                specificity += simpleSelector.specificity();
            }
        }

        HeapString idName;
        HeapString className;
        GlobalString tagName;
        GlobalString attrName;
        PseudoType pseudoType = PseudoType::None;
        const auto& lastComplexSelector = selector.front();
        for(const auto& simpleSelector : lastComplexSelector.compoundSelector()) {
            switch(simpleSelector.matchType()) {
            case CssSimpleSelector::MatchType::Id:
                idName = simpleSelector.value();
                break;
            case CssSimpleSelector::MatchType::Class:
                className = simpleSelector.value();
                break;
            case CssSimpleSelector::MatchType::Tag:
                tagName = simpleSelector.name();
                break;
            case CssSimpleSelector::MatchType::AttributeContains:
            case CssSimpleSelector::MatchType::AttributeDashEquals:
            case CssSimpleSelector::MatchType::AttributeEndsWith:
            case CssSimpleSelector::MatchType::AttributeEquals:
            case CssSimpleSelector::MatchType::AttributeHas:
            case CssSimpleSelector::MatchType::AttributeIncludes:
            case CssSimpleSelector::MatchType::AttributeStartsWith:
                attrName = simpleSelector.name();
                break;
            case CssSimpleSelector::MatchType::PseudoElementBefore:
            case CssSimpleSelector::MatchType::PseudoElementAfter:
            case CssSimpleSelector::MatchType::PseudoElementMarker:
            case CssSimpleSelector::MatchType::PseudoElementFirstLetter:
            case CssSimpleSelector::MatchType::PseudoElementFirstLine:
                pseudoType = simpleSelector.pseudoType();
                break;
            default:
                break;
            }
        }

        CssRuleData ruleData(rule, selector, specificity, m_position);
        if(pseudoType > PseudoType::None) {
            m_pseudoRules.add(pseudoType, std::move(ruleData));
        } else if(!idName.empty()) {
            m_idRules.add(idName, std::move(ruleData));
        } else if(!className.empty()) {
            m_classRules.add(className, std::move(ruleData));
        } else if(!attrName.isEmpty()) {
            m_attributeRules.add(attrName, std::move(ruleData));
        } else if(!tagName.isEmpty()) {
            m_tagRules.add(tagName, std::move(ruleData));
        } else {
            m_universalRules.push_back(std::move(ruleData));
        }
    }
}

void CssStyleSheet::addImportRule(const RefPtr<CssImportRule>& rule)
{
    constexpr auto kMaxImportDepth = 256;
    if(m_importDepth < kMaxImportDepth && m_document->supportsMediaQueries(rule->queries())) {
        if(auto resource = m_document->fetchTextResource(rule->href())) {
            m_importDepth++;
            parseStyle(resource->text(), rule->origin(), rule->href());
            m_importDepth--;
        }
    }
}

void CssStyleSheet::addPageRule(const RefPtr<CssPageRule>& rule)
{
    const auto& selectors = rule->selectors();
    if(selectors.empty()) {
        m_pageRules.emplace_back(rule, nullptr, 0, m_position);
        return;
    }

    for(const auto& selector : selectors) {
        uint32_t specificity = 0;
        for(const auto& sel : selector) {
            switch(sel.matchType()) {
            case CssSimpleSelector::MatchType::PseudoPageName:
                specificity += 0x10000;
                break;
            case CssSimpleSelector::MatchType::PseudoPageFirst:
            case CssSimpleSelector::MatchType::PseudoPageBlank:
                specificity += 0x100;
                break;
            case CssSimpleSelector::MatchType::PseudoPageLeft:
            case CssSimpleSelector::MatchType::PseudoPageRight:
            case CssSimpleSelector::MatchType::PseudoPageNth:
                specificity += 0x1;
                break;
            default:
                assert(false);
            }
        }

        m_pageRules.emplace_back(rule, &selector, specificity, m_position);
    }
}

class CssFontFaceBuilder {
public:
    explicit CssFontFaceBuilder(const CssPropertyList& properties);

    FontSelectionRange weight() const;
    FontSelectionRange stretch() const;
    FontSelectionRange slope() const;

    FontFeatureList featureSettings() const;
    FontVariationList variationSettings() const;
    UnicodeRangeList unicodeRanges() const;

    GlobalString family() const;
    FontSelectionDescription description() const;

    RefPtr<FontFace> build(Document* document) const;

private:
    RefPtr<CssValue> m_src;
    RefPtr<CssValue> m_family;
    RefPtr<CssValue> m_weight;
    RefPtr<CssValue> m_stretch;
    RefPtr<CssValue> m_style;
    RefPtr<CssValue> m_featureSettings;
    RefPtr<CssValue> m_variationSettings;
    RefPtr<CssValue> m_unicodeRange;
};

CssFontFaceBuilder::CssFontFaceBuilder(const CssPropertyList& properties)
{
    for(const auto& property : properties) {
        switch(property.id()) {
        case CssPropertyID::Src:
            m_src = property.value();
            break;
        case CssPropertyID::FontFamily:
            m_family = property.value();
            break;
        case CssPropertyID::FontWeight:
            m_weight = property.value();
            break;
        case CssPropertyID::FontStretch:
            m_stretch = property.value();
            break;
        case CssPropertyID::FontStyle:
            m_style = property.value();
            break;
        case CssPropertyID::UnicodeRange:
            m_unicodeRange = property.value();
            break;
        case CssPropertyID::FontFeatureSettings:
            m_featureSettings = property.value();
            break;
        case CssPropertyID::FontVariationSettings:
            m_variationSettings = property.value();
            break;
        default:
            assert(false);
        }
    }
}

FontSelectionRange CssFontFaceBuilder::weight() const
{
    if(m_weight == nullptr)
        return FontSelectionRange(kNormalFontWeight);
    if(auto ident = to<CssIdentValue>(m_weight)) {
        switch(ident->value()) {
        case CssValueID::Normal:
            return FontSelectionRange(kNormalFontWeight);
        case CssValueID::Bold:
            return FontSelectionRange(kBoldFontWeight);
        default:
            assert(false);
        }
    }

    const auto& pair = to<CssPairValue>(*m_weight);
    auto startWeight = convertFontWeightNumber(*pair.first());
    auto endWeight = convertFontWeightNumber(*pair.second());
    if(startWeight > endWeight)
        return FontSelectionRange(endWeight, startWeight);
    return FontSelectionRange(startWeight, endWeight);
}

FontSelectionRange CssFontFaceBuilder::stretch() const
{
    if(m_stretch == nullptr)
        return FontSelectionRange(kNormalFontWidth);
    if(auto ident = to<CssIdentValue>(m_stretch))
        return FontSelectionRange(convertFontStretchIdent(*ident));
    const auto& pair = to<CssPairValue>(*m_stretch);
    const auto& startPercent = to<CssPercentValue>(*pair.first());
    const auto& endPercent = to<CssPercentValue>(*pair.second());
    if(startPercent.value() > endPercent.value())
        return FontSelectionRange(endPercent.value(), startPercent.value());
    return FontSelectionRange(startPercent.value(), endPercent.value());
}

FontSelectionRange CssFontFaceBuilder::slope() const
{
    if(m_style == nullptr)
        return FontSelectionRange(kNormalFontSlope);
    if(auto ident = to<CssIdentValue>(m_style))
        return FontSelectionRange(convertFontSlopeIdent(*ident));
    const auto& list = to<CssListValue>(*m_style);
    const auto& ident = to<CssIdentValue>(*list[0]);
    assert(list.size() == 3 && ident.value() == CssValueID::Oblique);

    auto startAngle = convertFontSlopeAngle(*list[1]);
    auto endAngle = convertFontSlopeAngle(*list[2]);
    if(startAngle > endAngle)
        return FontSelectionRange(endAngle, startAngle);
    return FontSelectionRange(startAngle, endAngle);
}

FontFeatureList CssFontFaceBuilder::featureSettings() const
{
    FontFeatureList featureSettings;
    if(m_featureSettings == nullptr)
        return featureSettings;
    if(auto ident = to<CssIdentValue>(m_featureSettings)) {
        assert(ident->value() == CssValueID::Normal);
        return featureSettings;
    }

    for(const auto& value : to<CssListValue>(*m_featureSettings)) {
        const auto& feature = to<CssFontFeatureValue>(*value);
        featureSettings.emplace_front(feature.tag(), feature.value());
    }

    return featureSettings;
}

FontVariationList CssFontFaceBuilder::variationSettings() const
{
    FontVariationList variationSettings;
    if(m_variationSettings == nullptr)
        return variationSettings;
    if(auto ident = to<CssIdentValue>(m_variationSettings)) {
        assert(ident->value() == CssValueID::Normal);
        return variationSettings;
    }

    for(const auto& value : to<CssListValue>(*m_variationSettings)) {
        const auto& variation = to<CssFontVariationValue>(*value);
        variationSettings.emplace_front(variation.tag(), variation.value());
    }

    return variationSettings;
}

UnicodeRangeList CssFontFaceBuilder::unicodeRanges() const
{
    UnicodeRangeList unicodeRanges;
    if(m_unicodeRange == nullptr)
        return unicodeRanges;
    for(const auto& value : to<CssListValue>(*m_unicodeRange)) {
        const auto& range = to<CssUnicodeRangeValue>(*value);
        unicodeRanges.emplace_front(range.from(), range.to());
    }

    return unicodeRanges;
}

GlobalString CssFontFaceBuilder::family() const
{
    if(auto family = to<CssCustomIdentValue>(m_family))
        return family->value();
    return emptyGlo;
}

FontSelectionDescription CssFontFaceBuilder::description() const
{
    return FontSelectionDescription(weight(), stretch(), slope());
}

static const HeapString& convertStringOrCustomIdent(const CssValue& value)
{
    if(is<CssStringValue>(value))
        return to<CssStringValue>(value).value();
    return to<CssCustomIdentValue>(value).value();
}

RefPtr<FontFace> CssFontFaceBuilder::build(Document* document) const
{
    if(m_src == nullptr)
        return nullptr;
    for(const auto& value : to<CssListValue>(*m_src)) {
        const auto& list = to<CssListValue>(*value);
        if(auto function = to<CssUnaryFunctionValue>(list[0])) {
            assert(function->id() == CssFunctionID::Local);
            const auto& family = to<CssCustomIdentValue>(*function->value());
            if(!fontDataCache()->isFamilyAvailable(family.value()))
                continue;
            return LocalFontFace::create(family.value(), featureSettings(), variationSettings(), unicodeRanges());
        }

        const auto& url = to<CssUrlValue>(*list[0]);
        if(list.size() == 2) {
            const auto& function = to<CssUnaryFunctionValue>(*list[1]);
            assert(function.id() == CssFunctionID::Format);
            const auto& format = convertStringOrCustomIdent(*function.value());
            if(!FontResource::supportsFormat(format.value())) {
                continue;
            }
        }

        if(auto fontResource = document->fetchFontResource(url.value())) {
            return RemoteFontFace::create(featureSettings(), variationSettings(), unicodeRanges(), std::move(fontResource));
        }
    }

    return nullptr;
}

void CssStyleSheet::addFontFaceRule(const RefPtr<CssFontFaceRule>& rule)
{
    CssFontFaceBuilder builder(rule->properties());
    if(auto face = builder.build(m_document)) {
        m_fontFaceCache.add(builder.family(), builder.description(), std::move(face));
    }
}

void CssStyleSheet::addCounterStyleRule(const RefPtr<CssCounterStyleRule>& rule)
{
    assert(m_counterStyleMap == nullptr);
    m_counterStyleRules.push_back(rule);
}

void CssStyleSheet::addMediaRule(const RefPtr<CssMediaRule>& rule)
{
    if(m_document->supportsMediaQueries(rule->queries())) {
        addRuleList(rule->rules());
    }
}

} // namespace plutobook
