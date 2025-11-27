#pragma once

#include "css-rule.h"

namespace plutobook {
    class CssParser {
    public:
        explicit CssParser(const CssParserContext& context);

        CssRuleList parseSheet(const std::string_view& content);
        CssPropertyList parseStyle(const std::string_view& content);
        CssMediaQueryList parseMediaQueries(const std::string_view& content);

        CssPropertyList parsePropertyValue(CssTokenStream input,
                                           CssPropertyID id, bool important);

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
                         bool important, RefPtr<CssValue> value);
        void addExpandedProperty(CssPropertyList& properties, CssPropertyID id,
                                 bool important, RefPtr<CssValue> value);

        RefPtr<CssIdentValue> consumeFontStyleIdent(CssTokenStream& input);
        RefPtr<CssIdentValue> consumeFontStretchIdent(CssTokenStream& input);
        RefPtr<CssIdentValue>
        consumeFontVariantCapsIdent(CssTokenStream& input);
        RefPtr<CssIdentValue>
        consumeFontVariantEmojiIdent(CssTokenStream& input);
        RefPtr<CssIdentValue>
        consumeFontVariantPositionIdent(CssTokenStream& input);
        RefPtr<CssIdentValue>
        consumeFontVariantEastAsianIdent(CssTokenStream& input);
        RefPtr<CssIdentValue>
        consumeFontVariantLigaturesIdent(CssTokenStream& input);
        RefPtr<CssIdentValue>
        consumeFontVariantNumericIdent(CssTokenStream& input);

        RefPtr<CssValue> consumeNone(CssTokenStream& input);
        RefPtr<CssValue> consumeAuto(CssTokenStream& input);
        RefPtr<CssValue> consumeNormal(CssTokenStream& input);
        RefPtr<CssValue> consumeNoneOrAuto(CssTokenStream& input);
        RefPtr<CssValue> consumeNoneOrNormal(CssTokenStream& input);

        RefPtr<CssValue> consumeInteger(CssTokenStream& input, bool negative);
        RefPtr<CssValue> consumeIntegerOrAuto(CssTokenStream& input,
                                              bool negative);
        RefPtr<CssValue> consumePositiveInteger(CssTokenStream& input);
        RefPtr<CssValue> consumePositiveIntegerOrAuto(CssTokenStream& input);
        RefPtr<CssValue> consumePercent(CssTokenStream& input, bool negative);
        RefPtr<CssValue> consumeNumber(CssTokenStream& input, bool negative);
        RefPtr<CssValue> consumeNumberOrPercent(CssTokenStream& input,
                                                bool negative);
        RefPtr<CssValue> consumeNumberOrPercentOrAuto(CssTokenStream& input,
                                                      bool negative);
        RefPtr<CssValue> consumeCalc(CssTokenStream& input, bool negative,
                                     bool unitless);
        RefPtr<CssValue> consumeLength(CssTokenStream& input, bool negative,
                                       bool unitless);
        RefPtr<CssValue> consumeLengthOrPercent(CssTokenStream& input,
                                                bool negative, bool unitless);
        RefPtr<CssValue> consumeLengthOrAuto(CssTokenStream& input,
                                             bool negative, bool unitless);
        RefPtr<CssValue> consumeLengthOrNormal(CssTokenStream& input,
                                               bool negative, bool unitless);
        RefPtr<CssValue> consumeLengthOrPercentOrAuto(CssTokenStream& input,
                                                      bool negative,
                                                      bool unitless);
        RefPtr<CssValue> consumeLengthOrPercentOrNone(CssTokenStream& input,
                                                      bool negative,
                                                      bool unitless);
        RefPtr<CssValue> consumeLengthOrPercentOrNormal(CssTokenStream& input,
                                                        bool negative,
                                                        bool unitless);
        RefPtr<CssValue> consumeWidthOrHeight(CssTokenStream& input,
                                              bool unitless);
        RefPtr<CssValue> consumeWidthOrHeightOrAuto(CssTokenStream& input,
                                                    bool unitless);
        RefPtr<CssValue> consumeWidthOrHeightOrNone(CssTokenStream& input,
                                                    bool unitless);

        RefPtr<CssValue> consumeString(CssTokenStream& input);
        RefPtr<CssValue> consumeCustomIdent(CssTokenStream& input);
        RefPtr<CssValue> consumeStringOrCustomIdent(CssTokenStream& input);
        RefPtr<CssValue> consumeAttr(CssTokenStream& input);
        RefPtr<CssValue> consumeLocalUrl(CssTokenStream& input);
        RefPtr<CssValue> consumeLocalUrlOrAttr(CssTokenStream& input);
        RefPtr<CssValue> consumeLocalUrlOrNone(CssTokenStream& input);
        RefPtr<CssValue> consumeUrl(CssTokenStream& input);
        RefPtr<CssValue> consumeUrlOrNone(CssTokenStream& input);
        RefPtr<CssValue> consumeImage(CssTokenStream& input);
        RefPtr<CssValue> consumeImageOrNone(CssTokenStream& input);
        RefPtr<CssValue> consumeColor(CssTokenStream& input);
        RefPtr<CssValue> consumeRgb(CssTokenStream& input);
        RefPtr<CssValue> consumeHsl(CssTokenStream& input);
        RefPtr<CssValue> consumeHwb(CssTokenStream& input);

        RefPtr<CssValue> consumePaint(CssTokenStream& input);
        RefPtr<CssValue> consumeListStyleType(CssTokenStream& input);
        RefPtr<CssValue> consumeQuotes(CssTokenStream& input);
        RefPtr<CssValue> consumeContent(CssTokenStream& input);
        RefPtr<CssValue> consumeContentLeader(CssTokenStream& input);
        RefPtr<CssValue> consumeContentElement(CssTokenStream& input);
        RefPtr<CssValue> consumeContentCounter(CssTokenStream& input,
                                               bool counters);
        RefPtr<CssValue> consumeContentTargetCounter(CssTokenStream& input,
                                                     bool counters);
        RefPtr<CssValue> consumeContentQrCode(CssTokenStream& input);
        RefPtr<CssValue> consumeCounter(CssTokenStream& input, bool increment);
        RefPtr<CssValue> consumePage(CssTokenStream& input);
        RefPtr<CssValue> consumeOrientation(CssTokenStream& input);
        RefPtr<CssValue> consumeSize(CssTokenStream& input);
        RefPtr<CssValue> consumeFontSize(CssTokenStream& input);
        RefPtr<CssValue> consumeFontWeight(CssTokenStream& input);
        RefPtr<CssValue> consumeFontStyle(CssTokenStream& input);
        RefPtr<CssValue> consumeFontStretch(CssTokenStream& input);
        RefPtr<CssValue> consumeFontFamilyName(CssTokenStream& input);
        RefPtr<CssValue> consumeFontFamily(CssTokenStream& input);
        RefPtr<CssValue> consumeFontFeature(CssTokenStream& input);
        RefPtr<CssValue> consumeFontFeatureSettings(CssTokenStream& input);
        RefPtr<CssValue> consumeFontVariation(CssTokenStream& input);
        RefPtr<CssValue> consumeFontVariationSettings(CssTokenStream& input);
        RefPtr<CssValue> consumeFontVariantCaps(CssTokenStream& input);
        RefPtr<CssValue> consumeFontVariantEmoji(CssTokenStream& input);
        RefPtr<CssValue> consumeFontVariantPosition(CssTokenStream& input);
        RefPtr<CssValue> consumeFontVariantEastAsian(CssTokenStream& input);
        RefPtr<CssValue> consumeFontVariantLigatures(CssTokenStream& input);
        RefPtr<CssValue> consumeFontVariantNumeric(CssTokenStream& input);
        RefPtr<CssValue> consumeLineWidth(CssTokenStream& input);
        RefPtr<CssValue> consumeBorderRadiusValue(CssTokenStream& input);
        RefPtr<CssValue> consumeClip(CssTokenStream& input);
        RefPtr<CssValue> consumeDashList(CssTokenStream& input);
        RefPtr<CssValue> consumePosition(CssTokenStream& input);
        RefPtr<CssValue> consumeVerticalAlign(CssTokenStream& input);
        RefPtr<CssValue> consumeBaselineShift(CssTokenStream& input);
        RefPtr<CssValue> consumeTextDecorationLine(CssTokenStream& input);
        RefPtr<CssValue> consumePositionCoordinate(CssTokenStream& input);
        RefPtr<CssValue> consumeBackgroundSize(CssTokenStream& input);
        RefPtr<CssValue> consumeAngle(CssTokenStream& input);
        RefPtr<CssValue> consumeTransformValue(CssTokenStream& input);
        RefPtr<CssValue> consumeTransform(CssTokenStream& input);
        RefPtr<CssValue> consumePaintOrder(CssTokenStream& input);
        RefPtr<CssValue> consumeLonghand(CssTokenStream& input,
                                         CssPropertyID id);

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

        RefPtr<CssValue> consumeFontFaceSource(CssTokenStream& input);
        RefPtr<CssValue> consumeFontFaceSrc(CssTokenStream& input);
        RefPtr<CssValue> consumeFontFaceWeight(CssTokenStream& input);
        RefPtr<CssValue> consumeFontFaceStyle(CssTokenStream& input);
        RefPtr<CssValue> consumeFontFaceStretch(CssTokenStream& input);
        RefPtr<CssValue> consumeFontFaceUnicodeRange(CssTokenStream& input);

        RefPtr<CssValue> consumeCounterStyleName(CssTokenStream& input);
        RefPtr<CssValue> consumeCounterStyleSystem(CssTokenStream& input);
        RefPtr<CssValue> consumeCounterStyleNegative(CssTokenStream& input);
        RefPtr<CssValue> consumeCounterStyleSymbol(CssTokenStream& input);
        RefPtr<CssValue> consumeCounterStyleRangeBound(CssTokenStream& input);
        RefPtr<CssValue> consumeCounterStyleRange(CssTokenStream& input);
        RefPtr<CssValue> consumeCounterStylePad(CssTokenStream& input);
        RefPtr<CssValue> consumeCounterStyleSymbols(CssTokenStream& input);
        RefPtr<CssValue>
        consumeCounterStyleAdditiveSymbols(CssTokenStream& input);

        GlobalString defaultNamespace() const {
            return m_defaultNamespace;
        }
        GlobalString
        determineNamespace(GlobalString prefix) const;

        const CssParserContext& m_context;
        boost::unordered_flat_map<GlobalString, GlobalString> m_namespaces;
        GlobalString m_defaultNamespace = starGlo;
    };
} // namespace plutobook