#pragma once

#include "css-rule.h"

namespace plutobook {
    class CssParser {
    public:
        explicit CssParser(const CssParserContext& context);

        CssRuleList parseSheet(const std::string_view& content);
        CssPropertyList parseStyle(const std::string_view& content);
        CssMediaQueryList parseMediaQueries(const std::string_view& content);

        bool parsePropertyValue(CssTokenStream input,
                                CssPropertyList& properties, CssPropertyID id,
                                bool important) {
            return consumeDescriptor(input, properties, id, important);
        }

    private:
        bool consumeMediaFeature(CssTokenStream& input,
                                 CssMediaFeatureList& features);
        bool consumeMediaFeatures(CssTokenStream& input,
                                  CssMediaFeatureList& features);
        bool consumeMediaQuery(CssTokenStream& input,
                               CssMediaQueryList& queries);
        bool consumeMediaQueries(CssTokenStream& input,
                                 CssMediaQueryList& queries);

        RefPtr<CssRule> consumeRule(CssTokenStream& input);
        RefPtr<CssRule> consumeAtRule(CssTokenStream& input);
        RefPtr<CssStyleRule> consumeStyleRule(CssTokenStream& input);
        RefPtr<CssImportRule> consumeImportRule(CssTokenStream& input);
        RefPtr<CssNamespaceRule> consumeNamespaceRule(CssTokenStream& input);
        RefPtr<CssMediaRule> consumeMediaRule(CssTokenStream& prelude,
                                              CssTokenStream& block);
        RefPtr<CssFontFaceRule> consumeFontFaceRule(CssTokenStream& prelude,
                                                    CssTokenStream& block);
        RefPtr<CssCounterStyleRule>
        consumeCounterStyleRule(CssTokenStream& prelude, CssTokenStream& block);
        RefPtr<CssPageRule> consumePageRule(CssTokenStream& prelude,
                                            CssTokenStream& block);
        RefPtr<CssPageMarginRule> consumePageMarginRule(CssTokenStream& input);

        void consumeRuleList(CssTokenStream& input, CssRuleList& rules);
        bool consumePageSelectorList(CssTokenStream& input,
                                     CssPageSelectorList& selectors);
        bool consumePageSelector(CssTokenStream& input,
                                 CssPageSelector& selector);

        bool consumeSelectorList(CssTokenStream& input,
                                 CssSelectorList& selectors, bool relative);
        bool consumeSelector(CssTokenStream& input, CssSelector& selector,
                             bool relative);
        bool consumeCompoundSelector(CssTokenStream& input,
                                     CssCompoundSelector& selector,
                                     bool& failed);
        bool consumeSimpleSelector(CssTokenStream& input,
                                   CssCompoundSelector& selector, bool& failed);

        bool consumeTagSelector(CssTokenStream& input,
                                CssCompoundSelector& selector);
        bool consumeIdSelector(CssTokenStream& input,
                               CssCompoundSelector& selector);
        bool consumeClassSelector(CssTokenStream& input,
                                  CssCompoundSelector& selector);
        bool consumeAttributeSelector(CssTokenStream& input,
                                      CssCompoundSelector& selector);
        bool consumePseudoSelector(CssTokenStream& input,
                                   CssCompoundSelector& selector);

        bool consumeCombinator(CssTokenStream& input,
                               CssComplexSelector::Combinator& combinator);
        bool consumeMatchPattern(CssTokenStream& input,
                                 CssSimpleSelector::MatchPattern& pattern);

        bool consumeFontFaceDescriptor(CssTokenStream& input,
                                       CssPropertyList& properties,
                                       CssPropertyID id);
        bool consumeCounterStyleDescriptor(CssTokenStream& input,
                                           CssPropertyList& properties,
                                           CssPropertyID id);
        bool consumeDescriptor(CssTokenStream& input,
                               CssPropertyList& properties, CssPropertyID id,
                               bool important);

        bool consumeDeclaraction(CssTokenStream& input,
                                 CssPropertyList& properties,
                                 CssRuleType ruleType);
        void consumeDeclaractionList(CssTokenStream& input,
                                     CssPropertyList& properties,
                                     CssRuleType ruleType);

        void addProperty(CssPropertyList& properties, CssPropertyID id,
                         bool important, CssValuePtr value);
        void addExpandedProperty(CssPropertyList& properties, CssPropertyID id,
                                 bool important, CssValuePtr value);

        ValPtr<CssIdentValue> consumeFontStyleIdent(CssTokenStream& input);
        ValPtr<CssIdentValue> consumeFontStretchIdent(CssTokenStream& input);
        ValPtr<CssIdentValue>
        consumeFontVariantCapsIdent(CssTokenStream& input);
        ValPtr<CssIdentValue>
        consumeFontVariantEmojiIdent(CssTokenStream& input);
        ValPtr<CssIdentValue>
        consumeFontVariantPositionIdent(CssTokenStream& input);
        ValPtr<CssIdentValue>
        consumeFontVariantEastAsianIdent(CssTokenStream& input);
        ValPtr<CssIdentValue>
        consumeFontVariantLigaturesIdent(CssTokenStream& input);
        ValPtr<CssIdentValue>
        consumeFontVariantNumericIdent(CssTokenStream& input);

        CssValuePtr consumeNone(CssTokenStream& input);
        CssValuePtr consumeAuto(CssTokenStream& input);
        CssValuePtr consumeNormal(CssTokenStream& input);
        CssValuePtr consumeNoneOrAuto(CssTokenStream& input);
        CssValuePtr consumeNoneOrNormal(CssTokenStream& input);

        CssValuePtr consumeInteger(CssTokenStream& input, bool negative);
        CssValuePtr consumeIntegerOrAuto(CssTokenStream& input, bool negative);
        CssValuePtr consumePositiveInteger(CssTokenStream& input);
        CssValuePtr consumePositiveIntegerOrAuto(CssTokenStream& input);
        CssValuePtr consumePercent(CssTokenStream& input, bool negative);
        CssValuePtr consumeNumber(CssTokenStream& input, bool negative);
        CssValuePtr consumeNumberOrPercent(CssTokenStream& input,
                                           bool negative);
        CssValuePtr consumeNumberOrPercentOrAuto(CssTokenStream& input,
                                                 bool negative);
        CssValuePtr consumeCalc(CssTokenStream& input, bool negative,
                                bool unitless);
        CssValuePtr consumeLength(CssTokenStream& input, bool negative,
                                  bool unitless);
        CssValuePtr consumeLengthOrPercent(CssTokenStream& input, bool negative,
                                           bool unitless);
        CssValuePtr consumeLengthOrAuto(CssTokenStream& input, bool negative,
                                        bool unitless);
        CssValuePtr consumeLengthOrNormal(CssTokenStream& input, bool negative,
                                          bool unitless);
        CssValuePtr consumeLengthOrPercentOrAuto(CssTokenStream& input,
                                                 bool negative, bool unitless);
        CssValuePtr consumeLengthOrPercentOrNone(CssTokenStream& input,
                                                 bool negative, bool unitless);
        CssValuePtr consumeLengthOrPercentOrNormal(CssTokenStream& input,
                                                   bool negative,
                                                   bool unitless);
        CssValuePtr consumeWidthOrHeight(CssTokenStream& input, bool unitless);
        CssValuePtr consumeWidthOrHeightOrAuto(CssTokenStream& input,
                                               bool unitless);
        CssValuePtr consumeWidthOrHeightOrNone(CssTokenStream& input,
                                               bool unitless);

        CssValuePtr consumeString(CssTokenStream& input);
        CssValuePtr consumeCustomIdent(CssTokenStream& input);
        CssValuePtr consumeStringOrCustomIdent(CssTokenStream& input);
        CssValuePtr consumeAttr(CssTokenStream& input);
        CssValuePtr consumeLocalUrl(CssTokenStream& input);
        CssValuePtr consumeLocalUrlOrAttr(CssTokenStream& input);
        CssValuePtr consumeLocalUrlOrNone(CssTokenStream& input);
        CssValuePtr consumeUrl(CssTokenStream& input);
        CssValuePtr consumeUrlOrNone(CssTokenStream& input);
        CssValuePtr consumeImage(CssTokenStream& input);
        CssValuePtr consumeImageOrNone(CssTokenStream& input);
        CssValuePtr consumeColor(CssTokenStream& input);
        CssValuePtr consumeRgb(CssTokenStream& input);
        CssValuePtr consumeHsl(CssTokenStream& input);
        CssValuePtr consumeHwb(CssTokenStream& input);

        CssValuePtr consumePaint(CssTokenStream& input);
        CssValuePtr consumeListStyleType(CssTokenStream& input);
        CssValuePtr consumeQuotes(CssTokenStream& input);
        CssValuePtr consumeContent(CssTokenStream& input);
        CssValuePtr consumeContentLeader(CssTokenStream& input);
        CssValuePtr consumeContentElement(CssTokenStream& input);
        CssValuePtr consumeContentCounter(CssTokenStream& input, bool counters);
        CssValuePtr consumeContentTargetCounter(CssTokenStream& input,
                                                bool counters);
        CssValuePtr consumeContentQrCode(CssTokenStream& input);
        CssValuePtr consumeCounter(CssTokenStream& input, bool increment);
        CssValuePtr consumePage(CssTokenStream& input);
        CssValuePtr consumeOrientation(CssTokenStream& input);
        CssValuePtr consumeSize(CssTokenStream& input);
        CssValuePtr consumeFontSize(CssTokenStream& input);
        CssValuePtr consumeFontWeight(CssTokenStream& input);
        CssValuePtr consumeFontStyle(CssTokenStream& input);
        CssValuePtr consumeFontStretch(CssTokenStream& input);
        CssValuePtr consumeFontFamilyName(CssTokenStream& input);
        CssValuePtr consumeFontFamily(CssTokenStream& input);
        CssValuePtr consumeFontFeature(CssTokenStream& input);
        CssValuePtr consumeFontFeatureSettings(CssTokenStream& input);
        CssValuePtr consumeFontVariation(CssTokenStream& input);
        CssValuePtr consumeFontVariationSettings(CssTokenStream& input);
        CssValuePtr consumeFontVariantCaps(CssTokenStream& input);
        CssValuePtr consumeFontVariantEmoji(CssTokenStream& input);
        CssValuePtr consumeFontVariantPosition(CssTokenStream& input);
        CssValuePtr consumeFontVariantEastAsian(CssTokenStream& input);
        CssValuePtr consumeFontVariantLigatures(CssTokenStream& input);
        CssValuePtr consumeFontVariantNumeric(CssTokenStream& input);
        CssValuePtr consumeLineWidth(CssTokenStream& input);
        CssValuePtr consumeBorderRadiusValue(CssTokenStream& input);
        CssValuePtr consumeClip(CssTokenStream& input);
        CssValuePtr consumeDashList(CssTokenStream& input);
        CssValuePtr consumePosition(CssTokenStream& input);
        CssValuePtr consumeVerticalAlign(CssTokenStream& input);
        CssValuePtr consumeBaselineShift(CssTokenStream& input);
        CssValuePtr consumeTextDecorationLine(CssTokenStream& input);
        CssValuePtr consumePositionCoordinate(CssTokenStream& input);
        CssValuePtr consumeBackgroundSize(CssTokenStream& input);
        CssValuePtr consumeAngle(CssTokenStream& input);
        CssValuePtr consumeTransformValue(CssTokenStream& input);
        CssValuePtr consumeTransform(CssTokenStream& input);
        CssValuePtr consumePaintOrder(CssTokenStream& input);
        CssValuePtr consumeLonghand(CssTokenStream& input, CssPropertyID id);

        bool consumeFlex(CssTokenStream& input, CssPropertyList& properties,
                         bool important);
        bool consumeBackground(CssTokenStream& input,
                               CssPropertyList& properties, bool important);
        bool consumeColumns(CssTokenStream& input, CssPropertyList& properties,
                            bool important);
        bool consumeListStyle(CssTokenStream& input,
                              CssPropertyList& properties, bool important);
        bool consumeFont(CssTokenStream& input, CssPropertyList& properties,
                         bool important);
        bool consumeFontVariant(CssTokenStream& input,
                                CssPropertyList& properties, bool important);
        bool consumeBorder(CssTokenStream& input, CssPropertyList& properties,
                           bool important);
        bool consumeBorderRadius(CssTokenStream& input,
                                 CssPropertyList& properties, bool important);
        bool consumeMarker(CssTokenStream& input, CssPropertyList& properties,
                           bool important);
        bool consume2Shorthand(CssTokenStream& input,
                               CssPropertyList& properties, CssPropertyID id,
                               bool important);
        bool consume4Shorthand(CssTokenStream& input,
                               CssPropertyList& properties, CssPropertyID id,
                               bool important);
        bool consumeShorthand(CssTokenStream& input,
                              CssPropertyList& properties, CssPropertyID id,
                              bool important);

        CssValuePtr consumeFontFaceSource(CssTokenStream& input);
        CssValuePtr consumeFontFaceSrc(CssTokenStream& input);
        CssValuePtr consumeFontFaceWeight(CssTokenStream& input);
        CssValuePtr consumeFontFaceStyle(CssTokenStream& input);
        CssValuePtr consumeFontFaceStretch(CssTokenStream& input);
        CssValuePtr consumeFontFaceUnicodeRange(CssTokenStream& input);

        CssValuePtr consumeCounterStyleName(CssTokenStream& input);
        CssValuePtr consumeCounterStyleSystem(CssTokenStream& input);
        CssValuePtr consumeCounterStyleNegative(CssTokenStream& input);
        CssValuePtr consumeCounterStyleSymbol(CssTokenStream& input);
        CssValuePtr consumeCounterStyleRangeBound(CssTokenStream& input);
        CssValuePtr consumeCounterStyleRange(CssTokenStream& input);
        CssValuePtr consumeCounterStylePad(CssTokenStream& input);
        CssValuePtr consumeCounterStyleSymbols(CssTokenStream& input);
        CssValuePtr consumeCounterStyleAdditiveSymbols(CssTokenStream& input);

        GlobalString defaultNamespace() const { return m_defaultNamespace; }
        GlobalString determineNamespace(GlobalString prefix) const;

        const CssParserContext& m_context;
        boost::unordered_flat_map<GlobalString, GlobalString> m_namespaces;
        GlobalString m_defaultNamespace = starGlo;
    };
} // namespace plutobook