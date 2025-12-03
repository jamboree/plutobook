#include "css-parser.h"
#include "css-tokenizer.h"
#include "string-utils.h"
#include "document.h"
#include "ident-table.h"

#include <sstream>
#include <algorithm>
#include <span>
#include <cmath>
#include <numbers>

namespace plutobook {

CssParser::CssParser(const CssParserContext& context)
    : m_context(context)
{
}

CssRuleList CssParser::parseSheet(const std::string_view& content)
{
    CssRuleList rules;
    CssTokenizer tokenizer(content);
    CssTokenStream input(tokenizer.tokenize());
    consumeRuleList(input, rules);
    return rules;
}

CssPropertyList CssParser::parseStyle(const std::string_view& content)
{
    CssPropertyList properties;
    CssTokenizer tokenizer(content);
    CssTokenStream input(tokenizer.tokenize());
    consumeDeclaractionList(input, properties, CssRuleType::Style);
    return properties;
}

CssMediaQueryList CssParser::parseMediaQueries(const std::string_view& content)
{
    CssMediaQueryList queries;
    CssTokenizer tokenizer(content);
    CssTokenStream input(tokenizer.tokenize());
    consumeMediaQueries(input, queries);
    return queries;
}

template<typename T, unsigned int N>
static Optional<T> matchIdent(const IdentTable<T, N>& table, const std::string_view& ident)
{
    const auto it = table.find(ident);
    if (it != table.end())
        return it->second;
    return std::nullopt;
}

static bool consumeIdentIncludingWhitespace(CssTokenStream& input, const char* name, int length)
{
    if(input->type() == CssToken::Type::Ident && matchLower(input->data(), std::string_view(name, length))) {
        input.consumeIncludingWhitespace();
        return true;
    }

    return false;
}

static CssMediaQuery::Type consumeMediaType(CssTokenStream& input)
{
    if(consumeIdentIncludingWhitespace(input, "all", 3))
        return CssMediaQuery::Type::All;
    if(consumeIdentIncludingWhitespace(input, "print", 5))
        return CssMediaQuery::Type::Print;
    if(consumeIdentIncludingWhitespace(input, "screen", 6))
        return CssMediaQuery::Type::Screen;
    return CssMediaQuery::Type::None;
}

static CssMediaQuery::Restrictor consumeMediaRestrictor(CssTokenStream& input)
{
    if(consumeIdentIncludingWhitespace(input, "only", 4))
        return CssMediaQuery::Restrictor::Only;
    if(consumeIdentIncludingWhitespace(input, "not", 3))
        return CssMediaQuery::Restrictor::Not;
    return CssMediaQuery::Restrictor::None;
}

bool CssParser::consumeMediaFeature(CssTokenStream& input, CssMediaFeatureList& features)
{
    if(input->type() != CssToken::Type::LeftParenthesis)
        return false;
    static constexpr auto table = makeIdentTable<CssPropertyID>({
        {"width", CssPropertyID::Width},
        {"min-width", CssPropertyID::MinWidth},
        {"max-width", CssPropertyID::MaxWidth},
        {"height", CssPropertyID::Height},
        {"min-height", CssPropertyID::MinHeight},
        {"max-height", CssPropertyID::MaxHeight},
        {"orientation", CssPropertyID::Orientation}
    });

    auto block = input.consumeBlock();
    block.consumeWhitespace();
    if(block->type() != CssToken::Type::Ident)
        return false;
    auto id = matchIdent(table, block->data());
    if(!id.has_value())
        return false;
    block.consumeIncludingWhitespace();
    if(block->type() == CssToken::Type::Colon) {
        block.consumeIncludingWhitespace();
        RefPtr<CssValue> value;
        switch(id.value()) {
        case CssPropertyID::Width:
        case CssPropertyID::MinWidth:
        case CssPropertyID::MaxWidth:
        case CssPropertyID::Height:
        case CssPropertyID::MinHeight:
        case CssPropertyID::MaxHeight:
            value = consumeLength(block, false, false);
            break;
        case CssPropertyID::Orientation:
            value = consumeOrientation(block);
            break;
        default:
            assert(false);
        }

        block.consumeWhitespace();
        if(value && block.empty()) {
            features.emplace_front(*id, std::move(value));
            input.consumeWhitespace();
            return true;
        }
    }

    return false;
}

bool CssParser::consumeMediaFeatures(CssTokenStream& input, CssMediaFeatureList& features)
{
    do {
        if(!consumeMediaFeature(input, features))
            return false;
    } while(consumeIdentIncludingWhitespace(input, "and", 3));
    return true;
}

bool CssParser::consumeMediaQuery(CssTokenStream& input, CssMediaQueryList& queries)
{
    auto restrictor = consumeMediaRestrictor(input);
    auto type = consumeMediaType(input);
    if(restrictor != CssMediaQuery::Restrictor::None && type == CssMediaQuery::Type::None)
        return false;
    CssMediaFeatureList features;
    if(type != CssMediaQuery::Type::None && consumeIdentIncludingWhitespace(input, "and", 3) && !consumeMediaFeatures(input, features))
        return false;
    if(type == CssMediaQuery::Type::None && !consumeMediaFeatures(input, features)) {
        return false;
    }

    queries.emplace_front(type, restrictor, std::move(features));
    return true;
}

bool CssParser::consumeMediaQueries(CssTokenStream& input, CssMediaQueryList& queries)
{
    input.consumeWhitespace();
    if(!input.empty()) {
        do {
            if(!consumeMediaQuery(input, queries))
                return false;
        } while(input.consumeCommaIncludingWhitespace());
    }

    return true;
}

RefPtr<CssRule> CssParser::consumeRule(CssTokenStream& input)
{
    if(input->type() == CssToken::Type::AtKeyword)
        return consumeAtRule(input);
    return consumeStyleRule(input);
}

RefPtr<CssRule> CssParser::consumeAtRule(CssTokenStream& input)
{
    assert(input->type() == CssToken::Type::AtKeyword);
    auto name = input->data();
    input.consume();
    auto preludeBegin = input.begin();
    while(input->type() != CssToken::Type::EndOfFile
        && input->type() != CssToken::Type::LeftCurlyBracket
        && input->type() != CssToken::Type::Semicolon) {
        input.consumeComponent();
    }

    CssTokenStream prelude(preludeBegin, input.begin());
    if(input->type() == CssToken::Type::EndOfFile
        || input->type() == CssToken::Type::Semicolon) {
        if(input->type() == CssToken::Type::Semicolon)
            input.consume();
        if(matchLower(name, "import"))
            return consumeImportRule(prelude);
        if(matchLower(name, "namespace"))
            return consumeNamespaceRule(prelude);
        return nullptr;
    }

    auto block = input.consumeBlock();
    if(matchLower(name, "font-face"))
        return consumeFontFaceRule(prelude, block);
    if(matchLower(name, "media"))
        return consumeMediaRule(prelude, block);
    if(matchLower(name, "counter-style"))
        return consumeCounterStyleRule(prelude, block);
    if(matchLower(name, "page"))
        return consumePageRule(prelude, block);
    return nullptr;
}

RefPtr<CssStyleRule> CssParser::consumeStyleRule(CssTokenStream& input)
{
    auto preludeBegin = input.begin();
    while(!input.empty() && input->type() != CssToken::Type::LeftCurlyBracket) {
        input.consumeComponent();
    }

    if(input.empty())
        return nullptr;
    CssTokenStream prelude(preludeBegin, input.begin());
    auto block = input.consumeBlock();
    CssSelectorList selectors;
    if(!consumeSelectorList(prelude, selectors, false))
        return nullptr;
    CssPropertyList properties;
    consumeDeclaractionList(block, properties, CssRuleType::Style);
    return CssStyleRule::create(std::move(selectors), std::move(properties));
}

static const CssToken* consumeUrlToken(CssTokenStream& input)
{
    if(input->type() == CssToken::Type::Url) {
        auto token = input.begin();
        input.consumeIncludingWhitespace();
        return token;
    }

    if(input->type() == CssToken::Type::Function && matchLower(input->data(), "url")) {
        CssTokenStreamGuard guard(input);
        auto block = input.consumeBlock();
        block.consumeWhitespace();
        auto token = block.begin();
        block.consumeIncludingWhitespace();
        if(token->type() == CssToken::Type::BadString || !block.empty())
            return nullptr;
        assert(token->type() == CssToken::Type::String);
        input.consumeWhitespace();
        guard.release();
        return token;
    }

    return nullptr;
}

static const CssToken* consumeStringOrUrlToken(CssTokenStream& input)
{
    if(input->type() == CssToken::Type::String) {
        auto token = input.begin();
        input.consumeIncludingWhitespace();
        return token;
    }

    return consumeUrlToken(input);
}

RefPtr<CssImportRule> CssParser::consumeImportRule(CssTokenStream& input)
{
    input.consumeWhitespace();
    auto token = consumeStringOrUrlToken(input);
    if(token == nullptr)
        return nullptr;
    CssMediaQueryList queries;
    if(!consumeMediaQueries(input, queries))
        return nullptr;
    return CssImportRule::create(m_context.origin(), m_context.completeUrl(token->data()), std::move(queries));
}

RefPtr<CssNamespaceRule> CssParser::consumeNamespaceRule(CssTokenStream& input)
{
    GlobalString prefix;
    input.consumeWhitespace();
    if(input->type() == CssToken::Type::Ident) {
        prefix = GlobalString::get(input->data());
        input.consumeIncludingWhitespace();
    }

    auto token = consumeStringOrUrlToken(input);
    if(token == nullptr || !input.empty())
        return nullptr;
    const auto uri = GlobalString::get(token->data());
    if(prefix.isEmpty()) {
        m_defaultNamespace = uri;
    } else {
        m_namespaces.emplace(prefix, uri);
    }

    return CssNamespaceRule::create(prefix, uri);
}

RefPtr<CssMediaRule> CssParser::consumeMediaRule(CssTokenStream& prelude, CssTokenStream& block)
{
    CssMediaQueryList queries;
    if(!consumeMediaQueries(prelude, queries))
        return nullptr;
    CssRuleList rules;
    consumeRuleList(block, rules);
    return CssMediaRule::create(std::move(queries), std::move(rules));
}

RefPtr<CssFontFaceRule> CssParser::consumeFontFaceRule(CssTokenStream& prelude, CssTokenStream& block)
{
    prelude.consumeWhitespace();
    if(!prelude.empty())
        return nullptr;
    CssPropertyList properties;
    consumeDeclaractionList(block, properties, CssRuleType::FontFace);
    return CssFontFaceRule::create(std::move(properties));
}

RefPtr<CssCounterStyleRule> CssParser::consumeCounterStyleRule(CssTokenStream& prelude, CssTokenStream& block)
{
    prelude.consumeWhitespace();
    if(prelude->type() != CssToken::Type::Ident || matchLower(prelude->data(), "none"))
        return nullptr;
    const auto name = GlobalString::get(prelude->data());
    prelude.consumeIncludingWhitespace();
    if(!prelude.empty())
        return nullptr;
    CssPropertyList properties;
    consumeDeclaractionList(block, properties, CssRuleType::CounterStyle);
    return CssCounterStyleRule::create(name, std::move(properties));
}

RefPtr<CssPageRule> CssParser::consumePageRule(CssTokenStream& prelude, CssTokenStream& block)
{
    CssPageSelectorList selectors;
    if(!consumePageSelectorList(prelude, selectors))
        return nullptr;
    CssPageMarginRuleList margins;
    CssPropertyList properties;
    while(!block.empty()) {
        switch(block->type()) {
        case CssToken::Type::Whitespace:
        case CssToken::Type::Semicolon:
            block.consume();
            break;
        case CssToken::Type::AtKeyword:
            if(auto margin = consumePageMarginRule(block))
                margins.push_back(std::move(margin));
            break;
        default:
            consumeDeclaraction(block, properties, CssRuleType::Page);
            break;
        }
    }

    return CssPageRule::create(std::move(selectors), std::move(margins), std::move(properties));
}

RefPtr<CssPageMarginRule> CssParser::consumePageMarginRule(CssTokenStream& input)
{
    assert(input->type() == CssToken::Type::AtKeyword);
    auto name = input->data();
    input.consume();
    auto preludeBegin = input.begin();
    while(!input.empty() && input->type() != CssToken::Type::LeftCurlyBracket) {
        input.consumeComponent();
    }

    if(input.empty())
        return nullptr;
    CssTokenStream prelude(preludeBegin, input.begin());
    auto block = input.consumeBlock();
    prelude.consumeWhitespace();
    if(!prelude.empty())
        return nullptr;
    static constexpr auto table = makeIdentTable<PageMarginType>({
        {"top-left-corner", PageMarginType::TopLeftCorner},
        {"top-left", PageMarginType::TopLeft},
        {"top-center", PageMarginType::TopCenter},
        {"top-right", PageMarginType::TopRight},
        {"top-right-corner", PageMarginType::TopRightCorner},
        {"bottom-left-corner", PageMarginType::BottomLeftCorner},
        {"bottom-left", PageMarginType::BottomLeft},
        {"bottom-center", PageMarginType::BottomCenter},
        {"bottom-right", PageMarginType::BottomRight},
        {"bottom-right-corner", PageMarginType::BottomRightCorner},
        {"left-top", PageMarginType::LeftTop},
        {"left-middle", PageMarginType::LeftMiddle},
        {"left-bottom", PageMarginType::LeftBottom},
        {"right-top", PageMarginType::RightTop},
        {"right-middle", PageMarginType::RightMiddle},
        {"right-bottom", PageMarginType::RightBottom}
    });

    auto marginType = matchIdent(table, name);
    if(!marginType.has_value())
        return nullptr;
    CssPropertyList properties;
    consumeDeclaractionList(block, properties, CssRuleType::PageMargin);
    return CssPageMarginRule::create(marginType.value(), std::move(properties));
}

void CssParser::consumeRuleList(CssTokenStream& input, CssRuleList& rules)
{
    while(!input.empty()) {
        input.consumeWhitespace();
        if(input->type() == CssToken::Type::CDC
            || input->type() == CssToken::Type::CDO) {
            input.consume();
            continue;
        }

        if(auto rule = consumeRule(input)) {
            rules.push_back(std::move(rule));
        }
    }
}

bool CssParser::consumePageSelectorList(CssTokenStream& input, CssPageSelectorList& selectors)
{
    input.consumeWhitespace();
    if(!input.empty()) {
        do {
            CssPageSelector selector;
            if(!consumePageSelector(input, selector))
                return false;
            selectors.push_front(std::move(selector));
        } while(input.consumeCommaIncludingWhitespace());
    }

    return input.empty();
}

bool CssParser::consumePageSelector(CssTokenStream& input, CssPageSelector& selector)
{
    if(input->type() != CssToken::Type::Ident
        && input->type() != CssToken::Type::Colon) {
        return false;
    }

    if(input->type() == CssToken::Type::Ident) {
        selector.emplace_front(CssSimpleSelector::MatchType::PseudoPageName, GlobalString::get(input->data()));
        input.consumeIncludingWhitespace();
    }

    while(input->type() == CssToken::Type::Colon) {
        input.consumeIncludingWhitespace();
        if(input->type() == CssToken::Type::Function) {
            if(!matchLower(input->data(), "nth"))
                return false;
            auto block = input.consumeBlock();
            block.consumeWhitespace();
            CssSimpleSelector::MatchPattern pattern;
            if(!consumeMatchPattern(block, pattern))
                return false;
            block.consumeWhitespace();
            if(!block.empty())
                return false;
            input.consumeWhitespace();
            selector.emplace_front(CssSimpleSelector::MatchType::PseudoPageNth, pattern);
            continue;
        }

        if(input->type() != CssToken::Type::Ident)
            return false;
        static constexpr auto table = makeIdentTable<CssSimpleSelector::MatchType>({
            {"first", CssSimpleSelector::MatchType::PseudoPageFirst},
            {"left", CssSimpleSelector::MatchType::PseudoPageLeft},
            {"right", CssSimpleSelector::MatchType::PseudoPageRight},
            {"blank", CssSimpleSelector::MatchType::PseudoPageBlank}
        });

        auto name = input->data();
        input.consumeIncludingWhitespace();
        auto matchType = matchIdent(table, name);
        if(!matchType.has_value())
            return false;
        selector.emplace_front(matchType.value());
    }

    return true;
}

bool CssParser::consumeSelectorList(CssTokenStream& input, CssSelectorList& selectors, bool relative)
{
    do {
        CssSelector selector;
        if(!consumeSelector(input, selector, relative))
            return false;
        selectors.push_front(std::move(selector));
    } while(input.consumeCommaIncludingWhitespace());
    return input.empty();
}

bool CssParser::consumeSelector(CssTokenStream& input, CssSelector& selector, bool relative)
{
    auto combinator = CssComplexSelector::Combinator::None;
    if(relative) {
        consumeCombinator(input, combinator);
    }

    do {
        bool failed = false;
        CssCompoundSelector sel;
        if(!consumeCompoundSelector(input, sel, failed))
            return !failed ? combinator == CssComplexSelector::Combinator::Descendant : false;
        selector.emplace_front(combinator, std::move(sel));
    } while(consumeCombinator(input, combinator));
    return true;
}

bool CssParser::consumeCompoundSelector(CssTokenStream& input, CssCompoundSelector& selector, bool& failed)
{
    if(!consumeTagSelector(input, selector)) {
        if(m_defaultNamespace != starGlo)
            selector.emplace_front(CssSimpleSelector::MatchType::Namespace, m_defaultNamespace);
        if(!consumeSimpleSelector(input, selector, failed)) {
            return false;
        }
    }

    while(consumeSimpleSelector(input, selector, failed));
    return !failed;
}

bool CssParser::consumeSimpleSelector(CssTokenStream& input, CssCompoundSelector& selector, bool& failed)
{
    if(input->type() == CssToken::Type::Hash)
        failed = !consumeIdSelector(input, selector);
    else if(input->type() == CssToken::Type::Delim && input->delim() == '.')
        failed = !consumeClassSelector(input, selector);
    else if(input->type() == CssToken::Type::LeftSquareBracket)
        failed = !consumeAttributeSelector(input, selector);
    else if(input->type() == CssToken::Type::Colon)
        failed = !consumePseudoSelector(input, selector);
    else
        return false;
    return !failed;
}

bool CssParser::consumeTagSelector(CssTokenStream& input, CssCompoundSelector& selector)
{
    GlobalString name;
    CssTokenStreamGuard guard(input);
    if(input->type() == CssToken::Type::Ident) {
        name = GlobalString::get(input->data());
        input.consume();
    } else if(input->type() == CssToken::Type::Delim && input->delim() == '*') {
        name = starGlo;
        input.consume();
    } else {
        return false;
    }

    auto namespaceURI = m_defaultNamespace;
    if(input->type() == CssToken::Type::Delim && input->delim() == '|') {
        input.consume();
        namespaceURI = determineNamespace(name);
        if(input->type() == CssToken::Type::Ident) {
            name = GlobalString::get(input->data());
            input.consume();
        } else if(input->type() == CssToken::Type::Delim && input->delim() == '*') {
            name = starGlo;
            input.consume();
        } else {
            return false;
        }
    }

    if(namespaceURI != starGlo)
        selector.emplace_front(CssSimpleSelector::MatchType::Namespace, namespaceURI);
    if(name == starGlo) {
        selector.emplace_front(CssSimpleSelector::MatchType::Universal);
    } else {
        if(m_context.inHtmlDocument())
            name = name.foldCase();
        selector.emplace_front(CssSimpleSelector::MatchType::Tag, name);
    }

    guard.release();
    return true;
}

bool CssParser::consumeIdSelector(CssTokenStream& input, CssCompoundSelector& selector)
{
    assert(input->type() == CssToken::Type::Hash);
    if(input->hashType() == CssToken::HashType::Identifier) {
        selector.emplace_front(CssSimpleSelector::MatchType::Id, createString(input->data()));
        input.consume();
        return true;
    }

    return false;
}

bool CssParser::consumeClassSelector(CssTokenStream& input, CssCompoundSelector& selector)
{
    assert(input->type() == CssToken::Type::Delim);
    input.consume();
    if(input->type() == CssToken::Type::Ident) {
        selector.emplace_front(CssSimpleSelector::MatchType::Class, createString(input->data()));
        input.consume();
        return true;
    }

    return false;
}

bool CssParser::consumeAttributeSelector(CssTokenStream& input, CssCompoundSelector& selector)
{
    assert(input->type() == CssToken::Type::LeftSquareBracket);
    auto block = input.consumeBlock();
    block.consumeWhitespace();
    if(block->type() != CssToken::Type::Ident)
        return false;
    auto name = GlobalString::get(block->data());
    if(m_context.inHtmlDocument())
        name = name.foldCase();
    block.consumeIncludingWhitespace();
    if(block.empty()) {
        selector.emplace_front(CssSimpleSelector::MatchType::AttributeHas, name);
        return true;
    }

    if(block->type() != CssToken::Type::Delim)
        return false;
    CssSimpleSelector::MatchType matchType;
    switch(block->delim()) {
    case '=':
        matchType = CssSimpleSelector::MatchType::AttributeEquals;
        break;
    case '~':
        matchType = CssSimpleSelector::MatchType::AttributeIncludes;
        break;
    case '*':
        matchType = CssSimpleSelector::MatchType::AttributeContains;
        break;
    case '|':
        matchType = CssSimpleSelector::MatchType::AttributeDashEquals;
        break;
    case '^':
        matchType = CssSimpleSelector::MatchType::AttributeStartsWith;
        break;
    case '$':
        matchType = CssSimpleSelector::MatchType::AttributeEndsWith;
        break;
    default:
        return false;
    }

    if(matchType != CssSimpleSelector::MatchType::AttributeEquals) {
        block.consume();
        if(block->type() != CssToken::Type::Delim && block->delim() != '=') {
            return false;
        }
    }

    block.consumeIncludingWhitespace();
    if(block->type() != CssToken::Type::Ident && block->type() != CssToken::Type::String)
        return false;
    auto value = createString(block->data());
    block.consumeIncludingWhitespace();
    auto caseType = CssSimpleSelector::AttributeCaseType::Sensitive;
    if(block->type() == CssToken::Type::Ident && block->data() == "i") {
        caseType = CssSimpleSelector::AttributeCaseType::InSensitive;
        block.consumeIncludingWhitespace();
    }

    if(!block.empty())
        return false;
    selector.emplace_front(matchType, caseType, name, value);
    return true;
}

bool CssParser::consumePseudoSelector(CssTokenStream& input, CssCompoundSelector& selector)
{
    assert(input->type() == CssToken::Type::Colon);
    input.consume();
    if(input->type() == CssToken::Type::Colon) {
        input.consume();
        if(input->type() != CssToken::Type::Ident)
            return false;
        auto name = input->data();
        input.consume();
        static constexpr auto table = makeIdentTable<CssSimpleSelector::MatchType>({
            {"after", CssSimpleSelector::MatchType::PseudoElementAfter},
            {"before", CssSimpleSelector::MatchType::PseudoElementBefore},
            {"first-letter", CssSimpleSelector::MatchType::PseudoElementFirstLetter},
            {"first-line", CssSimpleSelector::MatchType::PseudoElementFirstLine},
            {"marker", CssSimpleSelector::MatchType::PseudoElementMarker}
        });

        auto matchType = matchIdent(table, name);
        if(!matchType.has_value())
            return false;
        selector.emplace_front(matchType.value());
        return true;
    }

    if(input->type() == CssToken::Type::Ident) {
        auto name = input->data();
        input.consume();
        static constexpr auto table = makeIdentTable<CssSimpleSelector::MatchType>({
            {"active", CssSimpleSelector::MatchType::PseudoClassActive},
            {"any-link", CssSimpleSelector::MatchType::PseudoClassAnyLink},
            {"checked", CssSimpleSelector::MatchType::PseudoClassChecked},
            {"disabled", CssSimpleSelector::MatchType::PseudoClassDisabled},
            {"empty", CssSimpleSelector::MatchType::PseudoClassEmpty},
            {"enabled", CssSimpleSelector::MatchType::PseudoClassEnabled},
            {"first-child", CssSimpleSelector::MatchType::PseudoClassFirstChild},
            {"first-of-type", CssSimpleSelector::MatchType::PseudoClassFirstOfType},
            {"focus", CssSimpleSelector::MatchType::PseudoClassFocus},
            {"focus-visible", CssSimpleSelector::MatchType::PseudoClassFocusVisible},
            {"focus-within", CssSimpleSelector::MatchType::PseudoClassFocusWithin},
            {"hover", CssSimpleSelector::MatchType::PseudoClassHover},
            {"last-child", CssSimpleSelector::MatchType::PseudoClassLastChild},
            {"last-of-type", CssSimpleSelector::MatchType::PseudoClassLastOfType},
            {"link", CssSimpleSelector::MatchType::PseudoClassLink},
            {"local-link", CssSimpleSelector::MatchType::PseudoClassLocalLink},
            {"only-child", CssSimpleSelector::MatchType::PseudoClassOnlyChild},
            {"only-of-type", CssSimpleSelector::MatchType::PseudoClassOnlyOfType},
            {"root", CssSimpleSelector::MatchType::PseudoClassRoot},
            {"scope", CssSimpleSelector::MatchType::PseudoClassScope},
            {"target", CssSimpleSelector::MatchType::PseudoClassTarget},
            {"target-within", CssSimpleSelector::MatchType::PseudoClassTargetWithin},
            {"visited", CssSimpleSelector::MatchType::PseudoClassVisited},
            {"after", CssSimpleSelector::MatchType::PseudoElementAfter},
            {"before", CssSimpleSelector::MatchType::PseudoElementBefore},
            {"first-letter", CssSimpleSelector::MatchType::PseudoElementFirstLetter},
            {"first-line", CssSimpleSelector::MatchType::PseudoElementFirstLine}
        });

        auto matchType = matchIdent(table, name);
        if(!matchType.has_value())
            return false;
        selector.emplace_front(matchType.value());
        return true;
    }

    if(input->type() == CssToken::Type::Function) {
        auto name = input->data();
        auto block = input.consumeBlock();
        block.consumeWhitespace();
        static constexpr auto table = makeIdentTable<CssSimpleSelector::MatchType>({
            {"is", CssSimpleSelector::MatchType::PseudoClassIs},
            {"not", CssSimpleSelector::MatchType::PseudoClassNot},
            {"has", CssSimpleSelector::MatchType::PseudoClassHas},
            {"where", CssSimpleSelector::MatchType::PseudoClassWhere},
            {"lang", CssSimpleSelector::MatchType::PseudoClassLang},
            {"nth-child", CssSimpleSelector::MatchType::PseudoClassNthChild},
            {"nth-last-child", CssSimpleSelector::MatchType::PseudoClassNthLastChild},
            {"nth-last-of-type", CssSimpleSelector::MatchType::PseudoClassNthLastOfType},
            {"nth-of-type", CssSimpleSelector::MatchType::PseudoClassNthOfType}
        });

        auto matchType = matchIdent(table, name);
        if(!matchType.has_value())
            return false;
        switch(matchType.value()) {
        case CssSimpleSelector::MatchType::PseudoClassIs:
        case CssSimpleSelector::MatchType::PseudoClassNot:
        case CssSimpleSelector::MatchType::PseudoClassHas:
        case CssSimpleSelector::MatchType::PseudoClassWhere: {
            CssSelectorList subSelectors;
            if(!consumeSelectorList(block, subSelectors, matchType == CssSimpleSelector::MatchType::PseudoClassHas))
                return false;
            selector.emplace_front(*matchType, std::move(subSelectors));
            break;
        }

        case CssSimpleSelector::MatchType::PseudoClassLang: {
            if(block->type() != CssToken::Type::Ident)
                return false;
            selector.emplace_front(*matchType, createString(block->data()));
            block.consume();
            break;
        }

        case CssSimpleSelector::MatchType::PseudoClassNthChild:
        case CssSimpleSelector::MatchType::PseudoClassNthLastChild:
        case CssSimpleSelector::MatchType::PseudoClassNthOfType:
        case CssSimpleSelector::MatchType::PseudoClassNthLastOfType: {
            CssSimpleSelector::MatchPattern pattern;
            if(!consumeMatchPattern(block, pattern))
                return false;
            selector.emplace_front(*matchType, pattern);
            break;
        }

        default:
            assert(false);
        }

        block.consumeWhitespace();
        return block.empty();
    }

    return false;
}

bool CssParser::consumeCombinator(CssTokenStream& input, CssComplexSelector::Combinator& combinator)
{
    combinator = CssComplexSelector::Combinator::None;
    while(input->type() == CssToken::Type::Whitespace) {
        combinator = CssComplexSelector::Combinator::Descendant;
        input.consume();
    }

    if(input->type() == CssToken::Type::Delim) {
        if(input->delim() == '+') {
            combinator = CssComplexSelector::Combinator::DirectAdjacent;
            input.consumeIncludingWhitespace();
            return true;
        }

        if(input->delim() == '~') {
            combinator = CssComplexSelector::Combinator::InDirectAdjacent;
            input.consumeIncludingWhitespace();
            return true;
        }

        if(input->delim() == '>') {
            combinator = CssComplexSelector::Combinator::Child;
            input.consumeIncludingWhitespace();
            return true;
        }
    }

    return combinator == CssComplexSelector::Combinator::Descendant;
}

bool CssParser::consumeMatchPattern(CssTokenStream& input, CssSimpleSelector::MatchPattern& pattern)
{
    if(input->type() == CssToken::Type::Number) {
        if(input->numberType() != CssToken::NumberType::Integer)
            return false;
        pattern = std::make_pair(0, input->integer());
        input.consume();
        return true;
    }

    if(input->type() == CssToken::Type::Ident) {
        if(matchLower(input->data(), "odd")) {
            pattern = std::make_pair(2, 1);
            input.consume();
            return true;
        }

        if(matchLower(input->data(), "even")) {
            pattern = std::make_pair(2, 0);
            input.consume();
            return true;
        }
    }

    std::stringstream ss;
    if(input->type() == CssToken::Type::Delim) {
        if(input->delim() != '+')
            return false;
        input.consume();
        if(input->type() != CssToken::Type::Ident)
            return false;
        pattern.first = 1;
        ss << input->data();
        input.consume();
    } else if(input->type() == CssToken::Type::Ident) {
        auto ident = input->data();
        input.consume();
        if(ident.front() == '-') {
            pattern.first = -1;
            ss << ident.substr(1);
        } else {
            pattern.first = 1;
            ss << ident;
        }
    } else if(input->type() == CssToken::Type::Dimension) {
        if(input->numberType() != CssToken::NumberType::Integer)
            return false;
        pattern.first = input->integer();
        ss << input->data();
        input.consume();
    }

    constexpr auto eof = std::stringstream::traits_type::eof();
    if(ss.peek() == eof || !equals(ss.get(), 'n', false))
        return false;
    auto sign = CssToken::NumberSign::None;
    if(ss.peek() != eof) {
        if(ss.get() != '-')
            return false;
        sign = CssToken::NumberSign::Minus;
        if(ss.peek() != eof) {
            ss >> pattern.second;
            if(ss.fail())
                return false;
            pattern.second = -pattern.second;
            return true;
        }
    }

    input.consumeWhitespace();
    if(sign == CssToken::NumberSign::None && input->type() == CssToken::Type::Delim) {
        auto delim = input->delim();
        if(delim == '+')
            sign = CssToken::NumberSign::Plus;
        else if(delim == '-')
            sign = CssToken::NumberSign::Minus;
        else
            return false;
        input.consumeIncludingWhitespace();
    }

    if(sign == CssToken::NumberSign::None && input->type() != CssToken::Type::Number) {
        pattern.second = 0;
        return true;
    }

    if(input->type() != CssToken::Type::Number || input->numberType() != CssToken::NumberType::Integer)
        return false;
    if(sign == CssToken::NumberSign::None && input->numberSign() == CssToken::NumberSign::None)
        return false;
    if(sign != CssToken::NumberSign::None && input->numberSign() != CssToken::NumberSign::None)
        return false;
    pattern.second = input->integer();
    if(sign == CssToken::NumberSign::Minus)
        pattern.second = -pattern.second;
    input.consume();
    return true;
}

bool CssParser::consumeFontFaceDescriptor(CssTokenStream& input, CssPropertyList& properties, CssPropertyID id)
{
    RefPtr<CssValue> value;
    switch(id) {
    case CssPropertyID::Src:
        value = consumeFontFaceSrc(input);
        break;
    case CssPropertyID::FontFamily:
        value = consumeFontFamilyName(input);
        break;
    case CssPropertyID::FontWeight:
        value = consumeFontFaceWeight(input);
        break;
    case CssPropertyID::FontStretch:
        value = consumeFontFaceStretch(input);
        break;
    case CssPropertyID::FontStyle:
        value = consumeFontFaceStyle(input);
        break;
    case CssPropertyID::UnicodeRange:
        value = consumeFontFaceUnicodeRange(input);
        break;
    case CssPropertyID::FontFeatureSettings:
        value = consumeFontFeatureSettings(input);
        break;
    case CssPropertyID::FontVariationSettings:
        value = consumeFontVariationSettings(input);
        break;
    default:
        return false;
    }

    input.consumeWhitespace();
    if(value && input.empty()) {
        addProperty(properties, id, false, std::move(value));
        return true;
    }

    return false;
}

bool CssParser::consumeCounterStyleDescriptor(CssTokenStream& input, CssPropertyList& properties, CssPropertyID id)
{
    RefPtr<CssValue> value;
    switch(id) {
    case CssPropertyID::System:
        value = consumeCounterStyleSystem(input);
        break;
    case CssPropertyID::Negative:
        value = consumeCounterStyleNegative(input);
        break;
    case CssPropertyID::Prefix:
    case CssPropertyID::Suffix:
        value = consumeCounterStyleSymbol(input);
        break;
    case CssPropertyID::Range:
        value = consumeCounterStyleRange(input);
        break;
    case CssPropertyID::Pad:
        value = consumeCounterStylePad(input);
        break;
    case CssPropertyID::Fallback:
        value = consumeCounterStyleName(input);
        break;
    case CssPropertyID::Symbols:
        value = consumeCounterStyleSymbols(input);
        break;
    case CssPropertyID::AdditiveSymbols:
        value = consumeCounterStyleAdditiveSymbols(input);
        break;
    default:
        return false;
    }

    input.consumeWhitespace();
    if(value && input.empty()) {
        addProperty(properties, id, false, std::move(value));
        return true;
    }

    return false;
}

static RefPtr<CssValue> consumeWideKeyword(CssTokenStream& input)
{
    if(input->type() != CssToken::Type::Ident) {
        return nullptr;
    }
    char buffer[8];
    if (input->data().length() <= sizeof(buffer)) {
        static constexpr auto table =
            makeIdentTable<int>({{"initial", 0}, {"inherit", 1}, {"unset", 2}});
        const auto it = table.find(toLower(input->data(), buffer));
        if (it != table.end()) {
            input.consumeIncludingWhitespace();
            switch (it->second) {
            case 0:
                return CssInitialValue::create();
            case 1:
                return CssInheritValue::create();
            case 2:
                return CssUnsetValue::create();
            }
        }
    }
    return nullptr;
}

static bool containsVariableReferences(CssTokenStream input)
{
    while(!input.empty()) {
        if(input->type() == CssToken::Type::Function && matchLower(input->data(), "var"))
            return true;
        input.consumeIncludingWhitespace();
    }

    return false;
}

bool CssParser::consumeDescriptor(CssTokenStream& input, CssPropertyList& properties, CssPropertyID id, bool important)
{
    if(containsVariableReferences(input)) {
        auto variable = CssVariableReferenceValue::create(m_context, id, important, CssVariableData::create(input));
        addProperty(properties, id, important, std::move(variable));
        return true;
    }

    if(auto value = consumeWideKeyword(input)) {
        if(!input.empty())
            return false;
        addExpandedProperty(properties, id, important, std::move(value));
        return true;
    }

    switch(id) {
    case CssPropertyID::BorderTop:
    case CssPropertyID::BorderRight:
    case CssPropertyID::BorderBottom:
    case CssPropertyID::BorderLeft:
    case CssPropertyID::FlexFlow:
    case CssPropertyID::ColumnRule:
    case CssPropertyID::Outline:
    case CssPropertyID::TextDecoration:
        return consumeShorthand(input, properties, id, important);
    case CssPropertyID::Margin:
    case CssPropertyID::Padding:
    case CssPropertyID::BorderColor:
    case CssPropertyID::BorderStyle:
    case CssPropertyID::BorderWidth:
        return consume4Shorthand(input, properties, id, important);
    case CssPropertyID::Gap:
    case CssPropertyID::BorderSpacing:
        return consume2Shorthand(input, properties, id, important);
    case CssPropertyID::Background:
        return consumeBackground(input, properties, important);
    case CssPropertyID::Font:
        return consumeFont(input, properties, important);
    case CssPropertyID::FontVariant:
        return consumeFontVariant(input, properties, important);
    case CssPropertyID::Border:
        return consumeBorder(input, properties, important);
    case CssPropertyID::BorderRadius:
        return consumeBorderRadius(input, properties, important);
    case CssPropertyID::Columns:
        return consumeColumns(input, properties, important);
    case CssPropertyID::Flex:
        return consumeFlex(input, properties, important);
    case CssPropertyID::ListStyle:
        return consumeListStyle(input, properties, important);
    case CssPropertyID::Marker:
        return consumeMarker(input, properties, important);
    default:
        break;
    }

    if(auto value = consumeLonghand(input, id)) {
        input.consumeWhitespace();
        if(input.empty()) {
            addProperty(properties, id, important, std::move(value));
            return true;
        }
    }

    return false;
}

constexpr bool isCustomPropertyName(std::string_view name)
{
    return name.length() > 2 && name.starts_with("--");
}

CssPropertyID csspropertyid(const std::string_view& name)
{
    if(isCustomPropertyName(name))
        return CssPropertyID::Custom;
    static constexpr auto table = makeIdentTable<CssPropertyID>({
        {"-pluto-page-scale", CssPropertyID::PageScale},
        {"additive-symbols", CssPropertyID::AdditiveSymbols},
        {"align-content", CssPropertyID::AlignContent},
        {"align-items", CssPropertyID::AlignItems},
        {"align-self", CssPropertyID::AlignSelf},
        {"alignment-baseline", CssPropertyID::AlignmentBaseline},
        {"background", CssPropertyID::Background},
        {"background-attachment", CssPropertyID::BackgroundAttachment},
        {"background-clip", CssPropertyID::BackgroundClip},
        {"background-color", CssPropertyID::BackgroundColor},
        {"background-image", CssPropertyID::BackgroundImage},
        {"background-origin", CssPropertyID::BackgroundOrigin},
        {"background-position", CssPropertyID::BackgroundPosition},
        {"background-repeat", CssPropertyID::BackgroundRepeat},
        {"background-size", CssPropertyID::BackgroundSize},
        {"baseline-shift", CssPropertyID::BaselineShift},
        {"border", CssPropertyID::Border},
        {"border-bottom", CssPropertyID::BorderBottom},
        {"border-bottom-color", CssPropertyID::BorderBottomColor},
        {"border-bottom-left-radius", CssPropertyID::BorderBottomLeftRadius},
        {"border-bottom-right-radius", CssPropertyID::BorderBottomRightRadius},
        {"border-bottom-style", CssPropertyID::BorderBottomStyle},
        {"border-bottom-width", CssPropertyID::BorderBottomWidth},
        {"border-collapse", CssPropertyID::BorderCollapse},
        {"border-color", CssPropertyID::BorderColor},
        {"border-horizontal-spacing", CssPropertyID::BorderHorizontalSpacing},
        {"border-left", CssPropertyID::BorderLeft},
        {"border-left-color", CssPropertyID::BorderLeftColor},
        {"border-left-style", CssPropertyID::BorderLeftStyle},
        {"border-left-width", CssPropertyID::BorderLeftWidth},
        {"border-radius", CssPropertyID::BorderRadius},
        {"border-right", CssPropertyID::BorderRight},
        {"border-right-color", CssPropertyID::BorderRightColor},
        {"border-right-style", CssPropertyID::BorderRightStyle},
        {"border-right-width", CssPropertyID::BorderRightWidth},
        {"border-spacing", CssPropertyID::BorderSpacing},
        {"border-style", CssPropertyID::BorderStyle},
        {"border-top", CssPropertyID::BorderTop},
        {"border-top-color", CssPropertyID::BorderTopColor},
        {"border-top-left-radius", CssPropertyID::BorderTopLeftRadius},
        {"border-top-right-radius", CssPropertyID::BorderTopRightRadius},
        {"border-top-style", CssPropertyID::BorderTopStyle},
        {"border-top-width", CssPropertyID::BorderTopWidth},
        {"border-vertical-spacing", CssPropertyID::BorderVerticalSpacing},
        {"border-width", CssPropertyID::BorderWidth},
        {"bottom", CssPropertyID::Bottom},
        {"box-sizing", CssPropertyID::BoxSizing},
        {"break-after", CssPropertyID::BreakAfter},
        {"break-before", CssPropertyID::BreakBefore},
        {"break-inside", CssPropertyID::BreakInside},
        {"caption-side", CssPropertyID::CaptionSide},
        {"clear", CssPropertyID::Clear},
        {"clip", CssPropertyID::Clip},
        {"clip-path", CssPropertyID::ClipPath},
        {"clip-rule", CssPropertyID::ClipRule},
        {"color", CssPropertyID::Color},
        {"column-break-after", CssPropertyID::ColumnBreakAfter},
        {"column-break-before", CssPropertyID::ColumnBreakBefore},
        {"column-break-inside", CssPropertyID::ColumnBreakInside},
        {"column-count", CssPropertyID::ColumnCount},
        {"column-fill", CssPropertyID::ColumnFill},
        {"column-gap", CssPropertyID::ColumnGap},
        {"column-rule", CssPropertyID::ColumnRule},
        {"column-rule-color", CssPropertyID::ColumnRuleColor},
        {"column-rule-style", CssPropertyID::ColumnRuleStyle},
        {"column-rule-width", CssPropertyID::ColumnRuleWidth},
        {"column-span", CssPropertyID::ColumnSpan},
        {"column-width", CssPropertyID::ColumnWidth},
        {"columns", CssPropertyID::Columns},
        {"content", CssPropertyID::Content},
        {"counter-increment", CssPropertyID::CounterIncrement},
        {"counter-reset", CssPropertyID::CounterReset},
        {"counter-set", CssPropertyID::CounterSet},
        {"cx", CssPropertyID::Cx},
        {"cy", CssPropertyID::Cy},
        {"direction", CssPropertyID::Direction},
        {"display", CssPropertyID::Display},
        {"dominant-baseline", CssPropertyID::DominantBaseline},
        {"empty-cells", CssPropertyID::EmptyCells},
        {"fallback", CssPropertyID::Fallback},
        {"fill", CssPropertyID::Fill},
        {"fill-opacity", CssPropertyID::FillOpacity},
        {"fill-rule", CssPropertyID::FillRule},
        {"flex", CssPropertyID::Flex},
        {"flex-basis", CssPropertyID::FlexBasis},
        {"flex-direction", CssPropertyID::FlexDirection},
        {"flex-flow", CssPropertyID::FlexFlow},
        {"flex-grow", CssPropertyID::FlexGrow},
        {"flex-shrink", CssPropertyID::FlexShrink},
        {"flex-wrap", CssPropertyID::FlexWrap},
        {"float", CssPropertyID::Float},
        {"font", CssPropertyID::Font},
        {"font-family", CssPropertyID::FontFamily},
        {"font-feature-settings", CssPropertyID::FontFeatureSettings},
        {"font-kerning", CssPropertyID::FontKerning},
        {"font-size", CssPropertyID::FontSize},
        {"font-stretch", CssPropertyID::FontStretch},
        {"font-style", CssPropertyID::FontStyle},
        {"font-variant", CssPropertyID::FontVariant},
        {"font-variant-caps", CssPropertyID::FontVariantCaps},
        {"font-variant-east-asian", CssPropertyID::FontVariantEastAsian},
        {"font-variant-emoji", CssPropertyID::FontVariantEmoji},
        {"font-variant-ligatures", CssPropertyID::FontVariantLigatures},
        {"font-variant-numeric", CssPropertyID::FontVariantNumeric},
        {"font-variant-position", CssPropertyID::FontVariantPosition},
        {"font-variation-settings", CssPropertyID::FontVariationSettings},
        {"font-weight", CssPropertyID::FontWeight},
        {"gap", CssPropertyID::Gap},
        {"height", CssPropertyID::Height},
        {"hyphens", CssPropertyID::Hyphens},
        {"justify-content", CssPropertyID::JustifyContent},
        {"left", CssPropertyID::Left},
        {"letter-spacing", CssPropertyID::LetterSpacing},
        {"line-height", CssPropertyID::LineHeight},
        {"list-style", CssPropertyID::ListStyle},
        {"list-style-image", CssPropertyID::ListStyleImage},
        {"list-style-position", CssPropertyID::ListStylePosition},
        {"list-style-type", CssPropertyID::ListStyleType},
        {"margin", CssPropertyID::Margin},
        {"margin-bottom", CssPropertyID::MarginBottom},
        {"margin-left", CssPropertyID::MarginLeft},
        {"margin-right", CssPropertyID::MarginRight},
        {"margin-top", CssPropertyID::MarginTop},
        {"marker", CssPropertyID::Marker},
        {"marker-end", CssPropertyID::MarkerEnd},
        {"marker-mid", CssPropertyID::MarkerMid},
        {"marker-start", CssPropertyID::MarkerStart},
        {"mask", CssPropertyID::Mask},
        {"mask-type", CssPropertyID::MaskType},
        {"max-height", CssPropertyID::MaxHeight},
        {"max-width", CssPropertyID::MaxWidth},
        {"min-height", CssPropertyID::MinHeight},
        {"min-width", CssPropertyID::MinWidth},
        {"mix-blend-mode", CssPropertyID::MixBlendMode},
        {"negative", CssPropertyID::Negative},
        {"object-fit", CssPropertyID::ObjectFit},
        {"object-position", CssPropertyID::ObjectPosition},
        {"opacity", CssPropertyID::Opacity},
        {"order", CssPropertyID::Order},
        {"orphans", CssPropertyID::Orphans},
        {"outline", CssPropertyID::Outline},
        {"outline-color", CssPropertyID::OutlineColor},
        {"outline-offset", CssPropertyID::OutlineOffset},
        {"outline-style", CssPropertyID::OutlineStyle},
        {"outline-width", CssPropertyID::OutlineWidth},
        {"overflow", CssPropertyID::Overflow},
        {"overflow-wrap", CssPropertyID::OverflowWrap},
        {"pad", CssPropertyID::Pad},
        {"padding", CssPropertyID::Padding},
        {"padding-bottom", CssPropertyID::PaddingBottom},
        {"padding-left", CssPropertyID::PaddingLeft},
        {"padding-right", CssPropertyID::PaddingRight},
        {"padding-top", CssPropertyID::PaddingTop},
        {"page", CssPropertyID::Page},
        {"page-break-after", CssPropertyID::PageBreakAfter},
        {"page-break-before", CssPropertyID::PageBreakBefore},
        {"page-break-inside", CssPropertyID::PageBreakInside},
        {"paint-order", CssPropertyID::PaintOrder},
        {"position", CssPropertyID::Position},
        {"prefix", CssPropertyID::Prefix},
        {"quotes", CssPropertyID::Quotes},
        {"r", CssPropertyID::R},
        {"range", CssPropertyID::Range},
        {"right", CssPropertyID::Right},
        {"row-gap", CssPropertyID::RowGap},
        {"rx", CssPropertyID::Rx},
        {"ry", CssPropertyID::Ry},
        {"size", CssPropertyID::Size},
        {"src", CssPropertyID::Src},
        {"stop-color", CssPropertyID::StopColor},
        {"stop-opacity", CssPropertyID::StopOpacity},
        {"stroke", CssPropertyID::Stroke},
        {"stroke-dasharray", CssPropertyID::StrokeDasharray},
        {"stroke-dashoffset", CssPropertyID::StrokeDashoffset},
        {"stroke-linecap", CssPropertyID::StrokeLinecap},
        {"stroke-linejoin", CssPropertyID::StrokeLinejoin},
        {"stroke-miterlimit", CssPropertyID::StrokeMiterlimit},
        {"stroke-opacity", CssPropertyID::StrokeOpacity},
        {"stroke-width", CssPropertyID::StrokeWidth},
        {"suffix", CssPropertyID::Suffix},
        {"symbols", CssPropertyID::Symbols},
        {"system", CssPropertyID::System},
        {"tab-size", CssPropertyID::TabSize},
        {"table-layout", CssPropertyID::TableLayout},
        {"text-align", CssPropertyID::TextAlign},
        {"text-anchor", CssPropertyID::TextAnchor},
        {"text-decoration", CssPropertyID::TextDecoration},
        {"text-decoration-color", CssPropertyID::TextDecorationColor},
        {"text-decoration-line", CssPropertyID::TextDecorationLine},
        {"text-decoration-style", CssPropertyID::TextDecorationStyle},
        {"text-indent", CssPropertyID::TextIndent},
        {"text-orientation", CssPropertyID::TextOrientation},
        {"text-overflow", CssPropertyID::TextOverflow},
        {"text-transform", CssPropertyID::TextTransform},
        {"top", CssPropertyID::Top},
        {"transform", CssPropertyID::Transform},
        {"transform-origin", CssPropertyID::TransformOrigin},
        {"unicode-bidi", CssPropertyID::UnicodeBidi},
        {"unicode-range", CssPropertyID::UnicodeRange},
        {"vector-effect", CssPropertyID::VectorEffect},
        {"vertical-align", CssPropertyID::VerticalAlign},
        {"visibility", CssPropertyID::Visibility},
        {"white-space", CssPropertyID::WhiteSpace},
        {"widows", CssPropertyID::Widows},
        {"width", CssPropertyID::Width},
        {"word-break", CssPropertyID::WordBreak},
        {"word-spacing", CssPropertyID::WordSpacing},
        {"writing-mode", CssPropertyID::WritingMode},
        {"x", CssPropertyID::X},
        {"y", CssPropertyID::Y},
        {"z-index", CssPropertyID::ZIndex}
    });

    char buffer[32];
    if(name.length() > sizeof(buffer))
        return CssPropertyID::Unknown;
    const auto it = table.find(toLower(name, buffer));
    return it == table.end() ? CssPropertyID::Unknown : it->second;
}

bool CssParser::consumeDeclaraction(CssTokenStream& input, CssPropertyList& properties, CssRuleType ruleType)
{
    auto begin = input.begin();
    while(!input.empty() && input->type() != CssToken::Type::Semicolon) {
        input.consumeComponent();
    }

    CssTokenStream newInput(begin, input.begin());
    if(newInput->type() != CssToken::Type::Ident)
        return false;
    auto name = newInput->data();
    auto id = csspropertyid(name);
    if(id == CssPropertyID::Unknown)
        return false;
    newInput.consumeIncludingWhitespace();
    if(newInput->type() != CssToken::Type::Colon)
        return false;
    newInput.consumeIncludingWhitespace();
    auto valueBegin = newInput.begin();
    auto valueEnd = newInput.end();
    auto it = valueEnd - 1;
    while(it->type() == CssToken::Type::Whitespace) {
        --it;
    }

    bool important = false;
    if(it->type() == CssToken::Type::Ident && matchLower(it->data(), "important")) {
        do {
            --it;
        } while(it->type() == CssToken::Type::Whitespace);
        if(it->type() == CssToken::Type::Delim && it->delim() == '!') {
            important = true;
            valueEnd = it;
        }
    }

    if(important && (ruleType == CssRuleType::FontFace || ruleType == CssRuleType::CounterStyle))
        return false;
    CssTokenStream value(valueBegin, valueEnd);
    if(id == CssPropertyID::Custom) {
        if(ruleType == CssRuleType::FontFace || ruleType == CssRuleType::CounterStyle)
            return false;
        auto custom = CssCustomPropertyValue::create(GlobalString::get(name), CssVariableData::create(value));
        addProperty(properties, id, important, std::move(custom));
        return true;
    }

    switch(ruleType) {
    case CssRuleType::FontFace:
        return consumeFontFaceDescriptor(value, properties, id);
    case CssRuleType::CounterStyle:
        return consumeCounterStyleDescriptor(value, properties, id);
    default:
        return consumeDescriptor(value, properties, id, important);
    }
}

void CssParser::consumeDeclaractionList(CssTokenStream& input, CssPropertyList& properties, CssRuleType ruleType)
{
    while(!input.empty()) {
        switch(input->type()) {
        case CssToken::Type::Whitespace:
        case CssToken::Type::Semicolon:
            input.consume();
            break;
        default:
            consumeDeclaraction(input, properties, ruleType);
            break;
        }
    }
}

void CssParser::addProperty(CssPropertyList& properties, CssPropertyID id, bool important, RefPtr<CssValue> value)
{
    if(value == nullptr) {
        switch(id) {
        case CssPropertyID::FontStyle:
        case CssPropertyID::FontWeight:
        case CssPropertyID::FontStretch:
        case CssPropertyID::FontVariantCaps:
        case CssPropertyID::FontVariantEmoji:
        case CssPropertyID::FontVariantEastAsian:
        case CssPropertyID::FontVariantLigatures:
        case CssPropertyID::FontVariantNumeric:
        case CssPropertyID::FontVariantPosition:
        case CssPropertyID::LineHeight:
            value = CssIdentValue::create(CssValueID::Normal);
            break;
        case CssPropertyID::ColumnWidth:
        case CssPropertyID::ColumnCount:
            value = CssIdentValue::create(CssValueID::Auto);
            break;
        case CssPropertyID::FlexGrow:
        case CssPropertyID::FlexShrink:
            value = CssNumberValue::create(1.0);
            break;
        case CssPropertyID::FlexBasis:
            value = CssPercentValue::create(0.0);
            break;
        default:
            value = CssInitialValue::create();
            break;
        }
    }

    properties.emplace_back(id, m_context.origin(), important, std::move(value));
}

std::span<const CssPropertyID> expandShorthand(CssPropertyID id)
{
    switch(id) {
    case CssPropertyID::BorderColor: {
        static const CssPropertyID data[] = {
            CssPropertyID::BorderTopColor,
            CssPropertyID::BorderRightColor,
            CssPropertyID::BorderBottomColor,
            CssPropertyID::BorderLeftColor
        };

        return data;
    }

    case CssPropertyID::BorderStyle: {
        static const CssPropertyID data[] = {
            CssPropertyID::BorderTopStyle,
            CssPropertyID::BorderRightStyle,
            CssPropertyID::BorderBottomStyle,
            CssPropertyID::BorderLeftStyle
        };

        return data;
    }

    case CssPropertyID::BorderWidth: {
        static const CssPropertyID data[] = {
            CssPropertyID::BorderTopWidth,
            CssPropertyID::BorderRightWidth,
            CssPropertyID::BorderBottomWidth,
            CssPropertyID::BorderLeftWidth
        };

        return data;
    }

    case CssPropertyID::BorderTop: {
        static const CssPropertyID data[] = {
            CssPropertyID::BorderTopColor,
            CssPropertyID::BorderTopStyle,
            CssPropertyID::BorderTopWidth
        };

        return data;
    }

    case CssPropertyID::BorderRight: {
        static const CssPropertyID data[] = {
            CssPropertyID::BorderRightColor,
            CssPropertyID::BorderRightStyle,
            CssPropertyID::BorderRightWidth
        };

        return data;
    }

    case CssPropertyID::BorderBottom: {
        static const CssPropertyID data[] = {
            CssPropertyID::BorderBottomColor,
            CssPropertyID::BorderBottomStyle,
            CssPropertyID::BorderBottomWidth
        };

        return data;
    }

    case CssPropertyID::BorderLeft: {
        static const CssPropertyID data[] = {
            CssPropertyID::BorderLeftColor,
            CssPropertyID::BorderLeftStyle,
            CssPropertyID::BorderLeftWidth
        };

        return data;
    }

    case CssPropertyID::BorderRadius: {
        static const CssPropertyID data[] = {
            CssPropertyID::BorderTopRightRadius,
            CssPropertyID::BorderTopLeftRadius,
            CssPropertyID::BorderBottomLeftRadius,
            CssPropertyID::BorderBottomRightRadius
        };

        return data;
    }

    case CssPropertyID::BorderSpacing: {
        static const CssPropertyID data[] = {
            CssPropertyID::BorderHorizontalSpacing,
            CssPropertyID::BorderVerticalSpacing
        };

        return data;
    }

    case CssPropertyID::Padding: {
        static const CssPropertyID data[] = {
            CssPropertyID::PaddingTop,
            CssPropertyID::PaddingRight,
            CssPropertyID::PaddingBottom,
            CssPropertyID::PaddingLeft
        };

        return data;
    }

    case CssPropertyID::Margin: {
        static const CssPropertyID data[] = {
            CssPropertyID::MarginTop,
            CssPropertyID::MarginRight,
            CssPropertyID::MarginBottom,
            CssPropertyID::MarginLeft
        };

        return data;
    }

    case CssPropertyID::Outline: {
        static const CssPropertyID data[] = {
            CssPropertyID::OutlineColor,
            CssPropertyID::OutlineStyle,
            CssPropertyID::OutlineWidth
        };

        return data;
    }

    case CssPropertyID::ListStyle: {
        static const CssPropertyID data[] = {
            CssPropertyID::ListStyleType,
            CssPropertyID::ListStylePosition,
            CssPropertyID::ListStyleImage
        };

        return data;
    }

    case CssPropertyID::ColumnRule: {
        static const CssPropertyID data[] = {
            CssPropertyID::ColumnRuleColor,
            CssPropertyID::ColumnRuleStyle,
            CssPropertyID::ColumnRuleWidth
        };

        return data;
    }

    case CssPropertyID::FlexFlow: {
        static const CssPropertyID data[] = {
            CssPropertyID::FlexDirection,
            CssPropertyID::FlexWrap
        };

        return data;
    }

    case CssPropertyID::Flex: {
        static const CssPropertyID data[] = {
            CssPropertyID::FlexGrow,
            CssPropertyID::FlexShrink,
            CssPropertyID::FlexBasis
        };

        return data;
    }

    case CssPropertyID::Background: {
        static const CssPropertyID data[] = {
            CssPropertyID::BackgroundColor,
            CssPropertyID::BackgroundImage,
            CssPropertyID::BackgroundRepeat,
            CssPropertyID::BackgroundAttachment,
            CssPropertyID::BackgroundOrigin,
            CssPropertyID::BackgroundClip,
            CssPropertyID::BackgroundPosition,
            CssPropertyID::BackgroundSize
        };

        return data;
    }

    case CssPropertyID::Gap: {
        static const CssPropertyID data[] = {
            CssPropertyID::RowGap,
            CssPropertyID::ColumnGap
        };

        return data;
    }

    case CssPropertyID::Columns: {
        static const CssPropertyID data[] = {
            CssPropertyID::ColumnWidth,
            CssPropertyID::ColumnCount
        };

        return data;
    }

    case CssPropertyID::Font: {
        static const CssPropertyID data[] = {
            CssPropertyID::FontStyle,
            CssPropertyID::FontWeight,
            CssPropertyID::FontVariantCaps,
            CssPropertyID::FontStretch,
            CssPropertyID::FontSize,
            CssPropertyID::LineHeight,
            CssPropertyID::FontFamily
        };

        return data;
    }

    case CssPropertyID::FontVariant: {
        static const CssPropertyID data[] = {
            CssPropertyID::FontVariantCaps,
            CssPropertyID::FontVariantEastAsian,
            CssPropertyID::FontVariantEmoji,
            CssPropertyID::FontVariantLigatures,
            CssPropertyID::FontVariantNumeric,
            CssPropertyID::FontVariantPosition
        };

        return data;
    }

    case CssPropertyID::Border: {
        static const CssPropertyID data[] = {
            CssPropertyID::BorderTopWidth,
            CssPropertyID::BorderRightWidth,
            CssPropertyID::BorderBottomWidth,
            CssPropertyID::BorderLeftWidth,
            CssPropertyID::BorderTopStyle,
            CssPropertyID::BorderRightStyle,
            CssPropertyID::BorderBottomStyle,
            CssPropertyID::BorderLeftStyle,
            CssPropertyID::BorderTopColor,
            CssPropertyID::BorderRightColor,
            CssPropertyID::BorderBottomColor,
            CssPropertyID::BorderLeftColor
        };

        return data;
    }

    case CssPropertyID::TextDecoration: {
        static const CssPropertyID data[] = {
            CssPropertyID::TextDecorationLine,
            CssPropertyID::TextDecorationStyle,
            CssPropertyID::TextDecorationColor
        };

        return data;
    }

    case CssPropertyID::Marker: {
        static const CssPropertyID data[] = {
            CssPropertyID::MarkerStart,
            CssPropertyID::MarkerMid,
            CssPropertyID::MarkerEnd
        };

        return data;
    }

    default:
        return {};
    }
}

void CssParser::addExpandedProperty(CssPropertyList& properties, CssPropertyID id, bool important, RefPtr<CssValue> value)
{
    auto longhand = expandShorthand(id);
    if(longhand.empty()) {
        addProperty(properties, id, important, std::move(value));
        return;
    }

    for (const auto id : longhand) {
        addProperty(properties, id, important, value);
    }
}

template<unsigned int N>
static CssValueID matchIdent(const CssTokenStream& input, const IdentTable<CssValueID, N>& table)
{
    if(input->type() == CssToken::Type::Ident) {
        if(auto id = matchIdent(table, input->data())) {
            return id.value();
        }
    }

    return CssValueID::Unknown;
}

template<unsigned int N>
static RefPtr<CssIdentValue> consumeIdent(CssTokenStream& input, const IdentTable<CssValueID, N>& table)
{
    if (input->type() == CssToken::Type::Ident) {
        if (auto id = matchIdent(table, input->data())) {
            input.consumeIncludingWhitespace();
            return CssIdentValue::create(id.value());
        }
    }

    return nullptr;
}

RefPtr<CssIdentValue> CssParser::consumeFontStyleIdent(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"normal", CssValueID::Normal},
        {"italic", CssValueID::Italic},
        {"oblique", CssValueID::Oblique}
    });

    return consumeIdent(input, table);
}

RefPtr<CssIdentValue> CssParser::consumeFontStretchIdent(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"normal", CssValueID::Normal},
        {"ultra-condensed", CssValueID::UltraCondensed},
        {"extra-condensed", CssValueID::ExtraCondensed},
        {"condensed", CssValueID::Condensed},
        {"semi-condensed", CssValueID::SemiCondensed},
        {"semi-expanded", CssValueID::SemiExpanded},
        {"expanded", CssValueID::Expanded},
        {"extra-expanded", CssValueID::ExtraExpanded},
        {"ultra-expanded", CssValueID::UltraExpanded}
    });

    return consumeIdent(input, table);
}

RefPtr<CssIdentValue> CssParser::consumeFontVariantCapsIdent(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"small-caps", CssValueID::SmallCaps},
        {"all-small-caps", CssValueID::AllSmallCaps},
        {"petite-caps", CssValueID::PetiteCaps},
        {"all-petite-caps", CssValueID::AllPetiteCaps},
        {"unicase", CssValueID::Unicase},
        {"titling-caps", CssValueID::TitlingCaps}
    });

    return consumeIdent(input, table);
}

RefPtr<CssIdentValue> CssParser::consumeFontVariantEmojiIdent(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"text", CssValueID::Text},
        {"emoji", CssValueID::Emoji},
        {"unicode", CssValueID::Unicode}
    });

    return consumeIdent(input, table);
}

RefPtr<CssIdentValue> CssParser::consumeFontVariantPositionIdent(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"sub", CssValueID::Sub},
        {"super", CssValueID::Super}
    });

    return consumeIdent(input, table);
}

RefPtr<CssIdentValue> CssParser::consumeFontVariantEastAsianIdent(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"jis78", CssValueID::Jis78},
        {"jis83", CssValueID::Jis83},
        {"jis90", CssValueID::Jis90},
        {"jis04", CssValueID::Jis04},
        {"simplified", CssValueID::Simplified},
        {"traditional", CssValueID::Traditional},
        {"full-width", CssValueID::FullWidth},
        {"proportional-width", CssValueID::ProportionalWidth},
        {"ruby", CssValueID::Ruby}
    });

    return consumeIdent(input, table);
}

RefPtr<CssIdentValue> CssParser::consumeFontVariantLigaturesIdent(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"common-ligatures", CssValueID::CommonLigatures},
        {"no-common-ligatures", CssValueID::NoCommonLigatures},
        {"historical-ligatures", CssValueID::HistoricalLigatures},
        {"no-historical-ligatures", CssValueID::NoHistoricalLigatures},
        {"discretionary-ligatures", CssValueID::DiscretionaryLigatures},
        {"no-discretionary-ligatures", CssValueID::NoDiscretionaryLigatures},
        {"contextual", CssValueID::Contextual},
        {"no-contextual", CssValueID::NoContextual}
    });

    return consumeIdent(input, table);
}

RefPtr<CssIdentValue> CssParser::consumeFontVariantNumericIdent(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"lining-nums", CssValueID::LiningNums},
        {"oldstyle-nums", CssValueID::OldstyleNums},
        {"proportional-nums", CssValueID::ProportionalNums},
        {"tabular-nums", CssValueID::TabularNums},
        {"diagonal-fractions", CssValueID::DiagonalFractions},
        {"stacked-fractions", CssValueID::StackedFractions},
        {"ordinal", CssValueID::Ordinal},
        {"slashed-zero", CssValueID::SlashedZero}
    });

    return consumeIdent(input, table);
}

RefPtr<CssValue> CssParser::consumeNone(CssTokenStream& input)
{
    if(consumeIdentIncludingWhitespace(input, "none", 4))
        return CssIdentValue::create(CssValueID::None);
    return nullptr;
}

RefPtr<CssValue> CssParser::consumeAuto(CssTokenStream& input)
{
    if(consumeIdentIncludingWhitespace(input, "auto", 4))
        return CssIdentValue::create(CssValueID::Auto);
    return nullptr;
}

RefPtr<CssValue> CssParser::consumeNormal(CssTokenStream& input)
{
    if(consumeIdentIncludingWhitespace(input, "normal", 6))
        return CssIdentValue::create(CssValueID::Normal);
    return nullptr;
}

RefPtr<CssValue> CssParser::consumeNoneOrAuto(CssTokenStream& input)
{
    if(auto value = consumeNone(input))
        return value;
    return consumeAuto(input);
}

RefPtr<CssValue> CssParser::consumeNoneOrNormal(CssTokenStream& input)
{
    if(auto value = consumeNone(input))
        return value;
    return consumeNormal(input);
}

RefPtr<CssValue> CssParser::consumeInteger(CssTokenStream& input, bool negative)
{
    if(input->type() != CssToken::Type::Number || input->numberType() != CssToken::NumberType::Integer || (input->integer() < 0 && !negative))
        return nullptr;
    auto value = input->integer();
    input.consumeIncludingWhitespace();
    return CssIntegerValue::create(value);
}

RefPtr<CssValue> CssParser::consumeIntegerOrAuto(CssTokenStream& input, bool negative)
{
    if(auto value = consumeAuto(input))
        return value;
    return consumeInteger(input, negative);
}

RefPtr<CssValue> CssParser::consumePositiveInteger(CssTokenStream& input)
{
    if(input->type() != CssToken::Type::Number || input->numberType() != CssToken::NumberType::Integer || input->integer() < 1)
        return nullptr;
    auto value = input->integer();
    input.consumeIncludingWhitespace();
    return CssIntegerValue::create(value);
}

RefPtr<CssValue> CssParser::consumePositiveIntegerOrAuto(CssTokenStream& input)
{
    if(auto value = consumeAuto(input))
        return value;
    return consumePositiveInteger(input);
}

RefPtr<CssValue> CssParser::consumeNumber(CssTokenStream& input, bool negative)
{
    if(input->type() != CssToken::Type::Number || (input->number() < 0 && !negative))
        return nullptr;
    auto value = input->number();
    input.consumeIncludingWhitespace();
    return CssNumberValue::create(value);
}

RefPtr<CssValue> CssParser::consumePercent(CssTokenStream& input, bool negative)
{
    if(input->type() != CssToken::Type::Percentage || (input->number() < 0 && !negative))
        return nullptr;
    auto value = input->number();
    input.consumeIncludingWhitespace();
    return CssPercentValue::create(value);
}

RefPtr<CssValue> CssParser::consumeNumberOrPercent(CssTokenStream& input, bool negative)
{
    if(auto value = consumeNumber(input, negative))
        return value;
    return consumePercent(input, negative);
}

RefPtr<CssValue> CssParser::consumeNumberOrPercentOrAuto(CssTokenStream& input, bool negative)
{
    if(auto value = consumeAuto(input))
        return value;
    return consumeNumberOrPercent(input, negative);
}

static Optional<CssLengthUnits> matchUnitType(std::string_view name)
{
    static constexpr auto table = makeIdentTable<CssLengthUnits>({
        {"px", CssLengthUnits::Pixels},
        {"pt", CssLengthUnits::Points},
        {"pc", CssLengthUnits::Picas},
        {"cm", CssLengthUnits::Centimeters},
        {"mm", CssLengthUnits::Millimeters},
        {"in", CssLengthUnits::Inches},
        {"vw", CssLengthUnits::ViewportWidth},
        {"vh", CssLengthUnits::ViewportHeight},
        {"vmin", CssLengthUnits::ViewportMin},
        {"vmax", CssLengthUnits::ViewportMax},
        {"em", CssLengthUnits::Ems},
        {"ex", CssLengthUnits::Exs},
        {"ch", CssLengthUnits::Chs},
        {"rem", CssLengthUnits::Rems}
    });

    return matchIdent(table, name);
}

enum class CalcFunction { Invalid, Calc, Clamp, Min, Max };

static CalcFunction getCalcFunction(std::string_view name)
{
    char buffer[8];
    if (name.length() <= sizeof(buffer)) {
        static constexpr auto table =
            makeIdentTable<CalcFunction>({{"calc", CalcFunction::Calc},
                                          {"clamp", CalcFunction::Clamp},
                                          {"min", CalcFunction::Min},
                                          {"max", CalcFunction::Max}});
        const auto it = table.find(toLower(name, buffer));
        if (it != table.end()) {
            return it->second;
        }
    }
    return CalcFunction::Invalid;
}

static CssCalcOperator convertCalcDelim(const CssToken& token)
{
    switch(token.delim()) {
    case '+':
        return CssCalcOperator::Add;
    case '-':
        return CssCalcOperator::Sub;
    case '*':
        return CssCalcOperator::Mul;
    case '/':
        return CssCalcOperator::Div;
    default:
        return CssCalcOperator::None;
    }
}

static bool consumeCalcBlock(CssTokenStream& input, CssTokenList& stack, CssCalcList& values)
{
    assert(input->type() == CssToken::Type::Function || input->type() == CssToken::Type::LeftParenthesis);
    stack.push_back(input.get());
    auto block = input.consumeBlock();
    block.consumeWhitespace();
    while(!block.empty()) {
        const auto& token = block.get();
        if(token.type() == CssToken::Type::Number) {
            values.emplace_back(token.number());
            block.consumeIncludingWhitespace();
        } else if(token.type() == CssToken::Type::Dimension) {
            auto unitType = matchUnitType(token.data());
            if(!unitType.has_value())
                return false;
            values.emplace_back(token.number(), unitType.value());
            block.consumeIncludingWhitespace();
        } else if(token.type() == CssToken::Type::Delim) {
            auto token_op = convertCalcDelim(token);
            if(token_op == CssCalcOperator::None)
                return false;
            while(!stack.empty()) {
                if(stack.back().type() != CssToken::Type::Delim)
                    break;
                auto stack_op = convertCalcDelim(stack.back());
                if((token_op == CssCalcOperator::Mul || token_op == CssCalcOperator::Div)
                    && (stack_op == CssCalcOperator::Add || stack_op == CssCalcOperator::Sub)) {
                    break;
                }

                values.emplace_back(stack_op);
                stack.pop_back();
            }

            stack.push_back(token);
            block.consumeIncludingWhitespace();
        } else if(token.type() == CssToken::Type::Function) {
            if(getCalcFunction(token.data()) == CalcFunction::Invalid)
                return false;
            if(!consumeCalcBlock(block, stack, values))
                return false;
            block.consumeWhitespace();
        } else if(token.type() == CssToken::Type::LeftParenthesis) {
            if(!consumeCalcBlock(block, stack, values))
                return false;
            block.consumeWhitespace();
        } else if(token.type() == CssToken::Type::Comma) {
            while(!stack.empty()) {
                if(stack.back().type() != CssToken::Type::Delim)
                    break;
                values.emplace_back(convertCalcDelim(stack.back()));
                stack.pop_back();
            }

            if(stack.empty() || stack.back().type() == CssToken::Type::LeftParenthesis)
                return false;
            stack.push_back(token);
            block.consumeIncludingWhitespace();
        } else {
            return false;
        }
    }

    size_t commaCount = 0;
    while(!stack.empty()) {
        if(stack.back().type() == CssToken::Type::Delim) {
            values.emplace_back(convertCalcDelim(stack.back()));
        } else if(stack.back().type() == CssToken::Type::Comma) {
            ++commaCount;
        } else {
            break;
        }

        stack.pop_back();
    }

    if(stack.empty())
        return false;
    auto left = stack.back();
    stack.pop_back();
    if(left.type() == CssToken::Type::LeftParenthesis)
        return commaCount == 0;
    assert(left.type() == CssToken::Type::Function);
    CssCalcOperator op;
    switch (getCalcFunction(left.data())) {
    case CalcFunction::Invalid:
        return false;
    case CalcFunction::Calc:
        return commaCount == 0;
    case CalcFunction::Clamp:
        if (commaCount != 2)
            return false;
        values.emplace_back(CssCalcOperator::Min);
        values.emplace_back(CssCalcOperator::Max);
        return true;
    case CalcFunction::Min:
        op = CssCalcOperator::Min;
        break;
    case CalcFunction::Max:
        op = CssCalcOperator::Max;
        break;
    }
    values.insert(values.end(), commaCount, CssCalc(op));
    return true;
}

RefPtr<CssValue> CssParser::consumeCalc(CssTokenStream& input, bool negative, bool unitless)
{
    if(input->type() != CssToken::Type::Function || getCalcFunction(input->data()) == CalcFunction::Invalid)
        return nullptr;
    CssTokenList stack;
    CssCalcList values;
    CssTokenStreamGuard guard(input);
    if(!consumeCalcBlock(input, stack, values))
        return nullptr;
    input.consumeWhitespace();
    guard.release();

    unitless |= m_context.inSvgElement();
    while(!stack.empty()) {
        if(stack.back().type() == CssToken::Type::Delim)
            values.emplace_back(convertCalcDelim(stack.back()));
        stack.pop_back();
    }

    return CssCalcValue::create(negative, unitless, std::move(values));
}

RefPtr<CssValue> CssParser::consumeLength(CssTokenStream& input, bool negative, bool unitless)
{
    if(auto value = consumeCalc(input, negative, unitless))
        return value;
    if(input->type() != CssToken::Type::Dimension && input->type() != CssToken::Type::Number)
        return nullptr;
    auto value = input->number();
    if(value < 0.0 && !negative)
        return nullptr;
    if(input->type() == CssToken::Type::Number) {
        if(value && !unitless && !m_context.inSvgElement())
            return nullptr;
        input.consumeIncludingWhitespace();
        return CssLengthValue::create(value, CssLengthUnits::None);
    }

    auto unitType = matchUnitType(input->data());
    if(!unitType.has_value())
        return nullptr;
    input.consumeIncludingWhitespace();
    return CssLengthValue::create(value, unitType.value());
}

RefPtr<CssValue> CssParser::consumeLengthOrPercent(CssTokenStream& input, bool negative, bool unitless)
{
    if(auto value = consumePercent(input, negative))
        return value;
    return consumeLength(input, negative, unitless);
}

RefPtr<CssValue> CssParser::consumeLengthOrAuto(CssTokenStream& input, bool negative, bool unitless)
{
    if(auto value = consumeAuto(input))
        return value;
    return consumeLength(input, negative, unitless);
}

RefPtr<CssValue> CssParser::consumeLengthOrNormal(CssTokenStream& input, bool negative, bool unitless)
{
    if(auto value = consumeNormal(input))
        return value;
    return consumeLength(input, negative, unitless);
}

RefPtr<CssValue> CssParser::consumeLengthOrPercentOrAuto(CssTokenStream& input, bool negative, bool unitless)
{
    if(auto value = consumeAuto(input))
        return value;
    return consumeLengthOrPercent(input, negative, unitless);
}

RefPtr<CssValue> CssParser::consumeLengthOrPercentOrNone(CssTokenStream& input, bool negative, bool unitless)
{
    if(auto value = consumeNone(input))
        return value;
    return consumeLengthOrPercent(input, negative, unitless);
}

RefPtr<CssValue> CssParser::consumeLengthOrPercentOrNormal(CssTokenStream& input, bool negative, bool unitless)
{
    if(auto value = consumeNormal(input))
        return value;
    return consumeLengthOrPercent(input, negative, unitless);
}

RefPtr<CssValue> CssParser::consumeWidthOrHeight(CssTokenStream& input, bool unitless)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"min-content", CssValueID::MinContent},
        {"max-content", CssValueID::MaxContent},
        {"fit-content", CssValueID::FitContent}
    });

    if(auto value = consumeIdent(input, table))
        return value;
    return consumeLengthOrPercent(input, false, unitless);
}

RefPtr<CssValue> CssParser::consumeWidthOrHeightOrAuto(CssTokenStream& input, bool unitless)
{
    if(auto value = consumeAuto(input))
        return value;
    return consumeWidthOrHeight(input, unitless);
}

RefPtr<CssValue> CssParser::consumeWidthOrHeightOrNone(CssTokenStream& input, bool unitless)
{
    if(auto value = consumeNone(input))
        return value;
    return consumeWidthOrHeight(input, unitless);
}

RefPtr<CssValue> CssParser::consumeString(CssTokenStream& input)
{
    if(input->type() == CssToken::Type::String) {
        auto value = createString(input->data());
        input.consumeIncludingWhitespace();
        return CssStringValue::create(value);
    }

    return nullptr;
}

RefPtr<CssValue> CssParser::consumeCustomIdent(CssTokenStream& input)
{
    if(input->type() == CssToken::Type::Ident) {
        auto value = GlobalString::get(input->data());
        input.consumeIncludingWhitespace();
        return CssCustomIdentValue::create(value);
    }

    return nullptr;
}

RefPtr<CssValue> CssParser::consumeStringOrCustomIdent(CssTokenStream& input)
{
    if(auto value = consumeString(input))
        return value;
    return consumeCustomIdent(input);
}

RefPtr<CssValue> CssParser::consumeAttr(CssTokenStream& input)
{
    if(input->type() != CssToken::Type::Function || !matchLower(input->data(), "attr"))
        return nullptr;
    CssTokenStreamGuard guard(input);
    auto block = input.consumeBlock();
    block.consumeWhitespace();
    if(block->type() != CssToken::Type::Ident)
        return nullptr;
    auto name = GlobalString::get(block->data());
    if(m_context.inHtmlDocument()) {
        name = name.foldCase();
    }

    block.consumeIncludingWhitespace();
    if(block->type() == CssToken::Type::Ident) {
        if(!matchLower(block->data(), "url") && !matchLower(block->data(), "string"))
            return nullptr;
        block.consumeIncludingWhitespace();
    }

    HeapString fallback;
    if(block.consumeCommaIncludingWhitespace()) {
        if(block->type() != CssToken::Type::String)
            return nullptr;
        fallback = createString(block->data());
        block.consumeIncludingWhitespace();
    }

    if(!block.empty())
        return nullptr;
    input.consumeWhitespace();
    guard.release();
    return CssAttrValue::create(name, fallback);
}

RefPtr<CssValue> CssParser::consumeLocalUrl(CssTokenStream& input)
{
    if(auto token = consumeUrlToken(input))
        return CssLocalUrlValue::create(createString(token->data()));
    return nullptr;
}

RefPtr<CssValue> CssParser::consumeLocalUrlOrAttr(CssTokenStream &input)
{
    if(auto value = consumeAttr(input))
        return value;
    return consumeLocalUrl(input);
}

RefPtr<CssValue> CssParser::consumeLocalUrlOrNone(CssTokenStream& input)
{
    if(auto value = consumeNone(input))
        return value;
    return consumeLocalUrl(input);
}

RefPtr<CssValue> CssParser::consumeUrl(CssTokenStream& input)
{
    if(auto token = consumeUrlToken(input))
        return CssUrlValue::create(m_context.completeUrl(token->data()));
    return nullptr;
}

RefPtr<CssValue> CssParser::consumeUrlOrNone(CssTokenStream& input)
{
    if(auto value = consumeNone(input))
        return value;
    return consumeUrl(input);
}

RefPtr<CssValue> CssParser::consumeImage(CssTokenStream& input)
{
    if(auto token = consumeUrlToken(input))
        return CssImageValue::create(m_context.completeUrl(token->data()));
    return nullptr;
}

RefPtr<CssValue> CssParser::consumeImageOrNone(CssTokenStream& input)
{
    if(auto value = consumeNone(input))
        return value;
    return consumeImage(input);
}

RefPtr<CssValue> CssParser::consumeColor(CssTokenStream& input)
{
    if(input->type() == CssToken::Type::Hash) {
        auto data = input->data();
        for(auto cc : data) {
            if(!isHexDigit(cc)) {
                return nullptr;
            }
        }

        int r, g, b, a = 255;
        if(data.size() == 3 || data.size() == 4) {
            r = toHexByte(data[0], data[0]);
            g = toHexByte(data[1], data[1]);
            b = toHexByte(data[2], data[2]);
            if(data.size() == 4) {
                a = toHexByte(data[3], data[3]);
            }
        } else if(data.size() == 6 || data.size() == 8) {
            r = toHexByte(data[0], data[1]);
            g = toHexByte(data[2], data[3]);
            b = toHexByte(data[4], data[5]);
            if(data.size() == 8) {
                a = toHexByte(data[6], data[7]);
            }
        } else {
            return nullptr;
        }

        input.consumeIncludingWhitespace();
        return CssColorValue::create(Color(r, g, b, a));
    }

    if(input->type() == CssToken::Type::Function) {
        const auto name = input->data();
        char buffer[8];
        if (name.length() <= sizeof(buffer)) {
            static constexpr auto table = makeIdentTable<int>(
                {{"rgb", 0}, {"rgba", 0}, {"hsl", 1}, {"hsla", 1}, {"hwb", 2}});
            const auto it = table.find(toLower(name, buffer));
            if (it != table.end()) {
                switch (it->second) {
                case 0: return consumeRgb(input);
                case 1: return consumeHsl(input);
                case 2: return consumeHwb(input);
                }
            }
        }
        return nullptr;
    }

    if(input->type() == CssToken::Type::Ident) {
        auto name = input->data();
        if(matchLower(name, "currentcolor")) {
            input.consumeIncludingWhitespace();
            return CssIdentValue::create(CssValueID::CurrentColor);
        }

        if(matchLower(name, "transparent")) {
            input.consumeIncludingWhitespace();
            return CssColorValue::create(Color::Transparent);
        }

        auto color = Color::named(name);
        if(!color.has_value())
            return nullptr;
        input.consumeIncludingWhitespace();
        return CssColorValue::create(color.value());
    }

    return nullptr;
}

static bool consumeRgbComponent(CssTokenStream& input, int& component, bool requiresPercent)
{
    if(input->type() != CssToken::Type::Number
        && input->type() != CssToken::Type::Percentage) {
        return false;
    }

    if(requiresPercent && input->type() != CssToken::Type::Percentage)
        return false;
    auto value = input->number();
    if(input->type() == CssToken::Type::Percentage)
        value *= 2.55f;
    component = std::lroundf(std::clamp(value, 0.f, 255.f));
    input.consumeIncludingWhitespace();
    return true;
}

static bool consumeAlphaComponent(CssTokenStream& input, int& component)
{
    if(input->type() != CssToken::Type::Number
        && input->type() != CssToken::Type::Percentage) {
        return false;
    }

    auto value = input->number();
    if(input->type() == CssToken::Type::Percentage)
        value /= 100.f;
    component = std::lroundf(255.f * std::clamp(value, 0.f, 1.f));
    input.consumeIncludingWhitespace();
    return true;
}

static bool consumeAlphaDelimiter(CssTokenStream& input, bool requiresComma)
{
    if(requiresComma)
        return input.consumeCommaIncludingWhitespace();
    if(input->type() == CssToken::Type::Delim && input->delim() == '/') {
        input.consumeIncludingWhitespace();
        return true;
    }

    return false;
}

RefPtr<CssValue> CssParser::consumeRgb(CssTokenStream& input)
{
    assert(input->type() == CssToken::Type::Function);
    CssTokenStreamGuard guard(input);
    auto block = input.consumeBlock();
    block.consumeWhitespace();

    auto requiresPercent = block->type() == CssToken::Type::Percentage;

    int red = 0;
    if(!consumeRgbComponent(block, red, requiresPercent)) {
        return nullptr;
    }

    auto requiresComma = block.consumeCommaIncludingWhitespace();

    int green = 0;
    if(!consumeRgbComponent(block, green, requiresPercent)) {
        return nullptr;
    }

    if(requiresComma && !block.consumeCommaIncludingWhitespace()) {
        return nullptr;
    }

    int blue = 0;
    if(!consumeRgbComponent(block, blue, requiresPercent)) {
        return nullptr;
    }

    int alpha = 255;
    if(consumeAlphaDelimiter(block, requiresComma)) {
        if(!consumeAlphaComponent(block, alpha)) {
            return nullptr;
        }
    }

    if(!block.empty())
        return nullptr;
    input.consumeWhitespace();
    guard.release();
    return CssColorValue::create(Color(red, green, blue, alpha));
}

static bool consumeAngleComponent(CssTokenStream& input, float& component)
{
    if(input->type() != CssToken::Type::Number
        && input->type() != CssToken::Type::Dimension) {
        return false;
    }

    component = input->number();
    if(input->type() == CssToken::Type::Dimension) {
        static constexpr auto table = makeIdentTable<CssAngleValue::Unit>({
            {"deg", CssAngleValue::Unit::Degrees},
            {"rad", CssAngleValue::Unit::Radians},
            {"grad", CssAngleValue::Unit::Gradians},
            {"turn", CssAngleValue::Unit::Turns}
        });

        auto unitType = matchIdent(table, input->data());
        if(!unitType.has_value())
            return false;
        switch(unitType.value()) {
        case CssAngleValue::Unit::Degrees:
            break;
        case CssAngleValue::Unit::Radians:
            component = component * 180.0 / std::numbers::pi;
            break;
        case CssAngleValue::Unit::Gradians:
            component = component * 360.0 / 400.0;
            break;
        case CssAngleValue::Unit::Turns:
            component = component * 360.0;
            break;
        }
    }

    component = std::fmod(component, 360.f);
    if(component < 0.f) { component += 360.f; }

    input.consumeIncludingWhitespace();
    return true;
}

static bool consumePercentComponent(CssTokenStream& input, float& component)
{
    if(input->type() != CssToken::Type::Percentage)
        return false;
    auto value = input->number() / 100.f;
    component = std::clamp(value, 0.f, 1.f);
    input.consumeIncludingWhitespace();
    return true;
}

static int computeHslComponent(float h, float s, float l, float n)
{
    auto k = fmodf(n + h / 30.f, 12.f);
    auto a = s * std::min(l, 1.f - l);
    auto v = l - a * std::max(-1.f, std::min(1.f, std::min(k - 3.f, 9.f - k)));
    return std::lroundf(v * 255.f);
}

RefPtr<CssValue> CssParser::consumeHsl(CssTokenStream& input)
{
    assert(input->type() == CssToken::Type::Function);
    CssTokenStreamGuard guard(input);
    auto block = input.consumeBlock();
    block.consumeWhitespace();

    float h, s, l, a = 1.f;
    if(!consumeAngleComponent(block, h)) {
        return nullptr;
    }

    auto requiresComma = block.consumeCommaIncludingWhitespace();

    if(!consumePercentComponent(block, s)) {
        return nullptr;
    }

    if(requiresComma && !block.consumeCommaIncludingWhitespace()) {
        return nullptr;
    }

    if(!consumePercentComponent(block, l)) {
        return nullptr;
    }

    int alpha = 255;
    if(consumeAlphaDelimiter(block, requiresComma)) {
        if(!consumeAlphaComponent(block, alpha)) {
            return nullptr;
        }
    }

    if(!block.empty())
        return nullptr;
    input.consumeWhitespace();
    guard.release();

    auto r = computeHslComponent(h, s, l, 0);
    auto g = computeHslComponent(h, s, l, 8);
    auto b = computeHslComponent(h, s, l, 4);
    return CssColorValue::create(Color(r, g, b, alpha));
}

RefPtr<CssValue> CssParser::consumeHwb(CssTokenStream& input)
{
    assert(input->type() == CssToken::Type::Function);
    CssTokenStreamGuard guard(input);
    auto block = input.consumeBlock();
    block.consumeWhitespace();

    float hue, white, black;
    if(!consumeAngleComponent(block, hue)) {
        return nullptr;
    }

    auto requiresComma = block.consumeCommaIncludingWhitespace();

    if(!consumePercentComponent(block, white)) {
        return nullptr;
    }

    if(requiresComma && !block.consumeCommaIncludingWhitespace()) {
        return nullptr;
    }

    if(!consumePercentComponent(block, black)) {
        return nullptr;
    }

    int alpha = 255;
    if(consumeAlphaDelimiter(block, requiresComma)) {
        if(!consumeAlphaComponent(block, alpha)) {
            return nullptr;
        }
    }

    if(!block.empty())
        return nullptr;
    input.consumeWhitespace();
    guard.release();

    if(white + black > 1.0f) {
        auto sum = white + black;
        white /= sum;
        black /= sum;
    }

    int components[3] = { 0, 8, 4 };
    for(auto& component : components) {
        auto channel = computeHslComponent(hue, 1.0f, 0.5f, component);
        component = std::lroundf(channel * (1 - white - black) + (white * 255));
    }

    const auto r = components[0];
    const auto g = components[1];
    const auto b = components[2];
    return CssColorValue::create(Color(r, g, b, alpha));
}

RefPtr<CssValue> CssParser::consumePaint(CssTokenStream& input)
{
    if(auto value = consumeNone(input))
        return value;
    auto first = consumeLocalUrl(input);
    if(first == nullptr)
        return consumeColor(input);
    auto second = consumeNone(input);
    if(second == nullptr)
        second = consumeColor(input);
    if(second == nullptr)
        return first;
    return CssPairValue::create(first, second);
}

RefPtr<CssValue> CssParser::consumeListStyleType(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"none", CssValueID::None},
        {"disc", CssValueID::Disc},
        {"circle", CssValueID::Circle},
        {"square", CssValueID::Square}
    });

    if(auto value = consumeIdent(input, table))
        return value;
    return consumeStringOrCustomIdent(input);
}

RefPtr<CssValue> CssParser::consumeQuotes(CssTokenStream& input)
{
    if(auto value = consumeNoneOrAuto(input))
        return value;
    CssValueList values;
    do {
        auto first = consumeString(input);
        if(first == nullptr)
            return nullptr;
        auto second = consumeString(input);
        if(second == nullptr)
            return nullptr;
        values.push_back(CssPairValue::create(first, second));
    } while(!input.empty());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumeContent(CssTokenStream& input)
{
    if(auto value = consumeNoneOrNormal(input))
        return value;
    CssValueList values;
    do {
        auto value = consumeString(input);
        if(value == nullptr)
            value = consumeImage(input);
        if(value == nullptr)
            value = consumeAttr(input);
        if(value == nullptr && input->type() == CssToken::Type::Ident) {
            static constexpr auto table = makeIdentTable<CssValueID>({
                {"open-quote", CssValueID::OpenQuote},
                {"close-quote", CssValueID::CloseQuote},
                {"no-open-quote", CssValueID::NoOpenQuote},
                {"no-close-quote", CssValueID::NoCloseQuote}
            });

            value = consumeIdent(input, table);
        }

        if(value == nullptr && input->type() == CssToken::Type::Function) {
            const auto name = input->data();
            auto block = input.consumeBlock();
            block.consumeWhitespace();
            char buffer[16];
            if (name.length() <= sizeof(buffer)) {
                static constexpr auto table =
                    makeIdentTable<int>({{"leader", 0},
                                         {"element", 1},
                                         {"counter", 2},
                                         {"counters", 3},
                                         {"target-counter", 4},
                                         {"target-counters", 5},
                                         {"-pluto-qrcode", 6}});
                const auto it = table.find(toLower(name, buffer));
                if (it != table.end()) {
                    switch (it->second) {
                    case 0: value = consumeContentLeader(block); break;
                    case 1: value = consumeContentElement(block); break;
                    case 2: value = consumeContentCounter(block, false); break;
                    case 3: value = consumeContentCounter(block, true); break;
                    case 4:
                        value = consumeContentTargetCounter(block, false);
                        break;
                    case 5:
                        value = consumeContentTargetCounter(block, true);
                        break;
                    case 6: value = consumeContentQrCode(block); break;
                    }
                }
            }
            input.consumeWhitespace();
        }

        if(value == nullptr)
            return nullptr;
        values.push_back(std::move(value));
    } while(!input.empty());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumeContentLeader(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"dotted", CssValueID::Dotted},
        {"solid", CssValueID::Solid},
        {"space", CssValueID::Space}
    });

    auto value = consumeString(input);
    if(value == nullptr)
        value = consumeIdent(input, table);
    if(value == nullptr || !input.empty())
        return nullptr;
    return CssUnaryFunctionValue::create(CssFunctionID::Leader, std::move(value));
}

RefPtr<CssValue> CssParser::consumeContentElement(CssTokenStream& input)
{
    auto value = consumeCustomIdent(input);
    if(value == nullptr || !input.empty())
        return nullptr;
    return CssUnaryFunctionValue::create(CssFunctionID::Element, std::move(value));
}

RefPtr<CssValue> CssParser::consumeContentCounter(CssTokenStream& input, bool counters)
{
    if(input->type() != CssToken::Type::Ident)
        return nullptr;
    auto identifier = GlobalString::get(input->data());
    input.consumeIncludingWhitespace();
    HeapString separator;
    if(counters) {
        if(!input.consumeCommaIncludingWhitespace())
            return nullptr;
        if(input->type() != CssToken::Type::String)
            return nullptr;
        separator = createString(input->data());
        input.consumeIncludingWhitespace();
    }

    auto listStyle = GlobalString::get("decimal");
    if(input.consumeCommaIncludingWhitespace()) {
        if(input->type() != CssToken::Type::Ident || matchLower(input->data(), "none"))
            return nullptr;
        listStyle = GlobalString::get(input->data());
        input.consumeIncludingWhitespace();
    }

    if(!input.empty())
        return nullptr;
    return CssCounterValue::create(identifier, listStyle, separator);
}

RefPtr<CssValue> CssParser::consumeContentTargetCounter(CssTokenStream& input, bool counters)
{
    auto fragment = consumeLocalUrlOrAttr(input);
    if(fragment == nullptr || !input.consumeCommaIncludingWhitespace())
        return nullptr;
    auto identifier = consumeCustomIdent(input);
    if(identifier == nullptr) {
        return nullptr;
    }

    CssValueList values;
    values.push_back(std::move(fragment));
    values.push_back(std::move(identifier));
    if(counters) {
        if(!input.consumeCommaIncludingWhitespace())
            return nullptr;
        auto separator = consumeString(input);
        if(separator == nullptr)
            return nullptr;
        values.push_back(std::move(separator));
        input.consumeWhitespace();
    }

    auto id = counters ? CssFunctionID::TargetCounters : CssFunctionID::TargetCounter;
    if(input.consumeCommaIncludingWhitespace()) {
        auto listStyle = consumeCustomIdent(input);
        if(listStyle == nullptr)
            return nullptr;
        values.push_back(std::move(listStyle));
        input.consumeWhitespace();
    }

    if(!input.empty())
        return nullptr;
    return CssFunctionValue::create(id, std::move(values));
}

RefPtr<CssValue> CssParser::consumeContentQrCode(CssTokenStream& input)
{
    auto text = consumeString(input);
    if(text == nullptr)
        return nullptr;
    CssValueList values;
    values.push_back(std::move(text));
    if(input.consumeCommaIncludingWhitespace()) {
        auto fill = consumeColor(input);
        if(fill == nullptr)
            return nullptr;
        values.push_back(std::move(fill));
        input.consumeWhitespace();
    }

    if(!input.empty())
        return nullptr;
    return CssFunctionValue::create(CssFunctionID::Qrcode, std::move(values));
}

RefPtr<CssValue> CssParser::consumeCounter(CssTokenStream& input, bool increment)
{
    if(auto value = consumeNone(input))
        return value;
    CssValueList values;
    do {
        auto name = consumeCustomIdent(input);
        if(name == nullptr)
            return nullptr;
        auto value = consumeInteger(input, true);
        if(value == nullptr)
            value = CssIntegerValue::create(increment ? 1 : 0);
        values.push_back(CssPairValue::create(name, value));
    } while(!input.empty());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumePage(CssTokenStream& input)
{
    if(auto value = consumeAuto(input))
        return value;
    return consumeCustomIdent(input);
}

RefPtr<CssValue> CssParser::consumeSize(CssTokenStream& input)
{
    if(auto value = consumeAuto(input))
        return value;
    if(auto width = consumeLength(input, false, false)) {
        auto height = consumeLength(input, false, false);
        if(height == nullptr)
            height = width;
        return CssPairValue::create(width, height);
    }

    RefPtr<CssValue> size;
    RefPtr<CssValue> orientation;
    for(int index = 0; index < 2; ++index) {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"a3", CssValueID::A3},
            {"a4", CssValueID::A4},
            {"a5", CssValueID::A5},
            {"b4", CssValueID::B4},
            {"b5", CssValueID::B5},
            {"ledger", CssValueID::Ledger},
            {"legal", CssValueID::Legal},
            {"letter", CssValueID::Letter}
        });

        if(size == nullptr && (size = consumeIdent(input, table)))
            continue;
        if(orientation == nullptr && (orientation = consumeOrientation(input))) {
            continue;
        }

        break;
    }

    if(size == nullptr && orientation == nullptr)
        return nullptr;
    if(size == nullptr)
        return orientation;
    if(orientation == nullptr)
        return size;
    return CssPairValue::create(size, orientation);
}

RefPtr<CssValue> CssParser::consumeOrientation(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"portrait", CssValueID::Portrait},
        {"landscape", CssValueID::Landscape}
    });

    return consumeIdent(input, table);
}

RefPtr<CssValue> CssParser::consumeFontSize(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"xx-small", CssValueID::XxSmall},
        {"x-small", CssValueID::XSmall},
        {"small", CssValueID::Small},
        {"medium", CssValueID::Medium},
        {"large", CssValueID::Large},
        {"x-large", CssValueID::XLarge},
        {"xx-large", CssValueID::XxLarge},
        {"xxx-large", CssValueID::XxxLarge},
        {"smaller", CssValueID::Smaller},
        {"larger", CssValueID::Larger}
    });

    if(auto value = consumeIdent(input, table))
        return value;
    return consumeLengthOrPercent(input, false, false);
}

RefPtr<CssValue> CssParser::consumeFontWeight(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"normal", CssValueID::Normal},
        {"bold", CssValueID::Bold},
        {"bolder", CssValueID::Bolder},
        {"lighter", CssValueID::Lighter}
    });

    if(auto value = consumeIdent(input, table))
        return value;
    if(input->type() == CssToken::Type::Number && (input->number() < 1 || input->number() > 1000))
        return nullptr;
    return consumeNumber(input, false);
}

RefPtr<CssValue> CssParser::consumeFontStyle(CssTokenStream& input)
{
    auto ident = consumeFontStyleIdent(input);
    if(ident == nullptr)
        return nullptr;
    if(ident->value() == CssValueID::Oblique) {
        if(auto angle = consumeAngle(input)) {
            return CssPairValue::create(ident, angle);
        }
    }

    return ident;
}

RefPtr<CssValue> CssParser::consumeFontStretch(CssTokenStream& input)
{
    if(auto value = consumeFontStretchIdent(input))
        return value;
    return consumePercent(input, false);
}

RefPtr<CssValue> CssParser::consumeFontFamilyName(CssTokenStream& input)
{
    if(input->type() == CssToken::Type::String) {
        auto value = GlobalString::get(input->data());
        input.consumeIncludingWhitespace();
        return CssCustomIdentValue::create(value);
    }

    std::string value;
    while(input->type() == CssToken::Type::Ident) {
        if(!value.empty())
            value += ' ';
        value += input->data();
        input.consumeIncludingWhitespace();
    }

    if(value.empty())
        return nullptr;
    return CssCustomIdentValue::create(GlobalString::get(value));
}

RefPtr<CssValue> CssParser::consumeFontFamily(CssTokenStream& input)
{
    CssValueList values;
    do {
        auto value = consumeFontFamilyName(input);
        if(value == nullptr)
            return nullptr;
        values.push_back(std::move(value));
    } while(input.consumeCommaIncludingWhitespace());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumeFontFeature(CssTokenStream& input)
{
    constexpr auto kTagLength = 4;
    if(input->type() != CssToken::Type::String)
        return nullptr;
    if(input->data().length() != kTagLength)
        return nullptr;
    for(auto cc : input->data()) {
        if(cc < 0x20 || cc > 0x7E) {
            return nullptr;
        }
    }

    auto tag = GlobalString::get(input->data());
    input.consumeIncludingWhitespace();

    int value = 1;
    if(input->type() == CssToken::Type::Number
        && input->numberType() == CssToken::NumberType::Integer) {
        value = input->integer();
        input.consumeIncludingWhitespace();
    } else if(input->type() == CssToken::Type::Ident) {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"on", CssValueID::On},
            {"off", CssValueID::Off}
        });

        switch(matchIdent(input, table)) {
        case CssValueID::On:
            value = 1;
            break;
        case CssValueID::Off:
            value = 0;
            break;
        default:
            return nullptr;
        };

        input.consumeIncludingWhitespace();
    }

    return CssFontFeatureValue::create(tag, value);
}

RefPtr<CssValue> CssParser::consumeFontFeatureSettings(CssTokenStream& input)
{
    if(auto value = consumeNormal(input))
        return value;
    CssValueList values;
    do {
        auto value = consumeFontFeature(input);
        if(value == nullptr)
            return nullptr;
        values.push_back(std::move(value));
    } while(input.consumeCommaIncludingWhitespace());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumeFontVariation(CssTokenStream& input)
{
    constexpr auto kTagLength = 4;
    if(input->type() != CssToken::Type::String)
        return nullptr;
    if(input->data().length() != kTagLength)
        return nullptr;
    for(auto cc : input->data()) {
        if(cc < 0x20 || cc > 0x7E) {
            return nullptr;
        }
    }

    auto tag = GlobalString::get(input->data());
    input.consumeIncludingWhitespace();
    if(input->type() != CssToken::Type::Number)
        return nullptr;
    auto value = input->number();
    input.consumeIncludingWhitespace();
    return CssFontVariationValue::create(tag, value);
}

RefPtr<CssValue> CssParser::consumeFontVariationSettings(CssTokenStream& input)
{
    if(auto value = consumeNormal(input))
        return value;
    CssValueList values;
    do {
        auto value = consumeFontVariation(input);
        if(value == nullptr)
            return nullptr;
        values.push_back(std::move(value));
    } while(input.consumeCommaIncludingWhitespace());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumeFontVariantCaps(CssTokenStream& input)
{
    if(auto value = consumeNormal(input))
        return value;
    return consumeFontVariantCapsIdent(input);
}

RefPtr<CssValue> CssParser::consumeFontVariantEmoji(CssTokenStream& input)
{
    if(auto value = consumeNormal(input))
        return value;
    return consumeFontVariantEmojiIdent(input);
}

RefPtr<CssValue> CssParser::consumeFontVariantPosition(CssTokenStream& input)
{
    if(auto value = consumeNormal(input))
        return value;
    return consumeFontVariantPositionIdent(input);
}

RefPtr<CssValue> CssParser::consumeFontVariantEastAsian(CssTokenStream& input)
{
    if(auto value = consumeNormal(input)) {
        return value;
    }

    bool consumedEastAsianVariant = false;
    bool consumedEastAsianWidth = false;
    bool consumedEastAsianRuby = false;

    CssValueList values;
    do {
        auto ident = consumeFontVariantEastAsianIdent(input);
        if(ident == nullptr)
            return nullptr;
        switch(ident->value()) {
        case CssValueID::Jis78:
        case CssValueID::Jis83:
        case CssValueID::Jis90:
        case CssValueID::Jis04:
        case CssValueID::Simplified:
        case CssValueID::Traditional:
            if(consumedEastAsianVariant)
                return nullptr;
            consumedEastAsianVariant = true;
            break;
        case CssValueID::FullWidth:
        case CssValueID::ProportionalWidth:
            if(consumedEastAsianWidth)
                return nullptr;
            consumedEastAsianWidth = true;
            break;
        case CssValueID::Ruby:
            if(consumedEastAsianRuby)
                return nullptr;
            consumedEastAsianRuby = true;
            break;
        default:
            assert(false);
        }

        values.push_back(std::move(ident));
    } while(!input.empty());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumeFontVariantLigatures(CssTokenStream& input)
{
    if(auto value = consumeNoneOrNormal(input)) {
        return value;
    }

    bool consumedCommonLigatures = false;
    bool consumedHistoricalLigatures = false;
    bool consumedDiscretionaryLigatures = false;
    bool consumedContextualLigatures = false;

    CssValueList values;
    do {
        auto ident = consumeFontVariantLigaturesIdent(input);
        if(ident == nullptr)
            return nullptr;
        switch(ident->value()) {
        case CssValueID::CommonLigatures:
        case CssValueID::NoCommonLigatures:
            if(consumedCommonLigatures)
                return nullptr;
            consumedCommonLigatures = true;
            break;
        case CssValueID::HistoricalLigatures:
        case CssValueID::NoHistoricalLigatures:
            if(consumedHistoricalLigatures)
                return nullptr;
            consumedHistoricalLigatures = true;
            break;
        case CssValueID::DiscretionaryLigatures:
        case CssValueID::NoDiscretionaryLigatures:
            if(consumedDiscretionaryLigatures)
                return nullptr;
            consumedDiscretionaryLigatures = true;
            break;
        case CssValueID::Contextual:
        case CssValueID::NoContextual:
            if(consumedContextualLigatures)
                return nullptr;
            consumedContextualLigatures = true;
            break;
        default:
            assert(false);
        }

        values.push_back(std::move(ident));
    } while(!input.empty());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumeFontVariantNumeric(CssTokenStream& input)
{
    if(auto value = consumeNormal(input)) {
        return value;
    }

    bool consumedNumericFigure = false;
    bool consumedNumericSpacing = false;
    bool consumedNumericFraction = false;
    bool consumedOrdinal = false;
    bool consumedSlashedZero = false;

    CssValueList values;
    do {
        auto ident = consumeFontVariantNumericIdent(input);
        if(ident == nullptr)
            return nullptr;
        switch(ident->value()) {
        case CssValueID::LiningNums:
        case CssValueID::OldstyleNums:
            if(consumedNumericFigure)
                return nullptr;
            consumedNumericFigure = true;
            break;
        case CssValueID::ProportionalNums:
        case CssValueID::TabularNums:
            if(consumedNumericSpacing)
                return nullptr;
            consumedNumericSpacing = true;
            break;
        case CssValueID::DiagonalFractions:
        case CssValueID::StackedFractions:
            if(consumedNumericFraction)
                return nullptr;
            consumedNumericFraction = true;
            break;
        case CssValueID::Ordinal:
            if(consumedOrdinal)
                return nullptr;
            consumedOrdinal = true;
            break;
        case CssValueID::SlashedZero:
            if(consumedSlashedZero)
                return nullptr;
            consumedSlashedZero = true;
            break;
        default:
            assert(false);
        }

        values.push_back(std::move(ident));
    } while(!input.empty());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumeLineWidth(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"thin", CssValueID::Thin},
        {"medium", CssValueID::Medium},
        {"thick", CssValueID::Thick}
    });

    if(auto value = consumeIdent(input, table))
        return value;
    return consumeLength(input, false, false);
}

RefPtr<CssValue> CssParser::consumeBorderRadiusValue(CssTokenStream& input)
{
    auto first = consumeLengthOrPercent(input, false, false);
    if(first == nullptr)
        return nullptr;
    auto second = consumeLengthOrPercent(input, false, false);
    if(second == nullptr)
        second = first;
    return CssPairValue::create(first, second);
}

RefPtr<CssValue> CssParser::consumeClip(CssTokenStream& input)
{
    if(auto value = consumeAuto(input))
        return value;
    if(input->type() != CssToken::Type::Function || !matchLower(input->data(), "rect")) {
        return nullptr;
    }

    auto block = input.consumeBlock();
    block.consumeWhitespace();
    auto top = consumeLengthOrPercentOrAuto(block, true, false);
    if(top == nullptr)
        return nullptr;
    if(block->type() == CssToken::Type::Comma)
        block.consumeIncludingWhitespace();
    auto right = consumeLengthOrPercentOrAuto(block, true, false);
    if(right == nullptr)
        return nullptr;
    if(block->type() == CssToken::Type::Comma)
        block.consumeIncludingWhitespace();
    auto bottom = consumeLengthOrPercentOrAuto(block, true, false);
    if(bottom == nullptr)
        return nullptr;
    if(block->type() == CssToken::Type::Comma)
        block.consumeIncludingWhitespace();
    auto left = consumeLengthOrPercentOrAuto(block, true, false);
    if(left == nullptr || !block.empty())
        return nullptr;
    return CssRectValue::create(top, right, bottom, left);
}

RefPtr<CssValue> CssParser::consumeDashList(CssTokenStream& input)
{
    if(auto value = consumeNone(input))
        return value;
    CssValueList values;
    do {
        auto value = consumeLengthOrPercent(input, false, true);
        if(value == nullptr || (input.consumeCommaIncludingWhitespace() && input.empty()))
            return nullptr;
        values.push_back(std::move(value));
    } while(!input.empty());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumePosition(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"static", CssValueID::Static},
        {"relative", CssValueID::Relative},
        {"absolute", CssValueID::Absolute},
        {"fixed", CssValueID::Fixed}
    });

    if(auto value = consumeIdent(input, table))
        return value;
    if(input->type() != CssToken::Type::Function || !matchLower(input->data(), "running"))
        return nullptr;
    CssTokenStreamGuard guard(input);
    auto block = input.consumeBlock();
    block.consumeWhitespace();
    auto value = consumeCustomIdent(block);
    if(value == nullptr || !block.empty())
        return nullptr;
    input.consumeWhitespace();
    guard.release();
    return CssUnaryFunctionValue::create(CssFunctionID::Running, std::move(value));
}

RefPtr<CssValue> CssParser::consumeVerticalAlign(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"baseline", CssValueID::Baseline},
        {"sub", CssValueID::Sub},
        {"super", CssValueID::Super},
        {"text-top", CssValueID::TextTop},
        {"text-bottom", CssValueID::TextBottom},
        {"middle", CssValueID::Middle},
        {"top", CssValueID::Top},
        {"bottom", CssValueID::Bottom}
    });

    if(auto value = consumeIdent(input, table))
        return value;
    return consumeLengthOrPercent(input, true, false);
}

RefPtr<CssValue> CssParser::consumeBaselineShift(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"baseline", CssValueID::Baseline},
        {"sub", CssValueID::Sub},
        {"super", CssValueID::Super}
    });

    if(auto value = consumeIdent(input, table))
        return value;
    return consumeLengthOrPercent(input, true, false);
}

RefPtr<CssValue> CssParser::consumeTextDecorationLine(CssTokenStream& input)
{
    if(auto value = consumeNone(input))
        return value;
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"underline", CssValueID::Underline},
        {"overline", CssValueID::Overline},
        {"line-through", CssValueID::LineThrough}
    });

    bool consumedUnderline = false;
    bool consumedOverline = false;
    bool consumedLineThrough = false;

    CssValueList values;
    do {
        auto ident = consumeIdent(input, table);
        if(ident == nullptr)
            break;
        switch(ident->value()) {
        case CssValueID::Underline:
            if(consumedUnderline)
                return nullptr;
            consumedUnderline = true;
            break;
        case CssValueID::Overline:
            if(consumedOverline)
                return nullptr;
            consumedOverline = true;
            break;
        case CssValueID::LineThrough:
            if(consumedLineThrough)
                return nullptr;
            consumedLineThrough = true;
            break;
        default:
            assert(false);
        }

        values.push_back(std::move(ident));
    } while(!input.empty());
    if(values.empty())
        return nullptr;
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumePositionCoordinate(CssTokenStream& input)
{
    RefPtr<CssValue> first;
    RefPtr<CssValue> second;
    for(int index = 0; index < 2; ++index) {
        if(first == nullptr && (first = consumeLengthOrPercent(input, true, false)))
            continue;
        if(second == nullptr && (second = consumeLengthOrPercent(input, true, false)))
            continue;
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"left", CssValueID::Left},
            {"right", CssValueID::Right},
            {"center", CssValueID::Center}
        });

        if(first == nullptr && (first = consumeIdent(input, table))) {
            continue;
        }
        {
            static constexpr auto table = makeIdentTable<CssValueID>({
                {"top", CssValueID::Top},
                {"bottom", CssValueID::Bottom},
                {"center", CssValueID::Center}
            });

            if(second == nullptr && (second = consumeIdent(input, table))) {
                continue;
            }
        }

        break;
    }

    if(first == nullptr && second == nullptr)
        return nullptr;
    if(first == nullptr)
        first = CssIdentValue::create(CssValueID::Center);
    if(second == nullptr)
        second = CssIdentValue::create(CssValueID::Center);
    return CssPairValue::create(first, second);
}

RefPtr<CssValue> CssParser::consumeBackgroundSize(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"contain", CssValueID::Contain},
        {"cover", CssValueID::Cover}
    });

    if(auto value = consumeIdent(input, table))
        return value;
    auto first = consumeLengthOrPercentOrAuto(input, false, false);
    if(first == nullptr)
        return nullptr;
    auto second = consumeLengthOrPercentOrAuto(input, false, false);
    if(second == nullptr)
        second = CssIdentValue::create(CssValueID::Auto);
    return CssPairValue::create(first, second);
}

RefPtr<CssValue> CssParser::consumeAngle(CssTokenStream& input)
{
    if(input->type() != CssToken::Type::Dimension)
        return nullptr;
    static constexpr auto table = makeIdentTable<CssAngleValue::Unit>({
        {"deg", CssAngleValue::Unit::Degrees},
        {"rad", CssAngleValue::Unit::Radians},
        {"grad", CssAngleValue::Unit::Gradians},
        {"turn", CssAngleValue::Unit::Turns}
    });

    auto unitType = matchIdent(table, input->data());
    if(!unitType.has_value())
        return nullptr;
    auto value = input->number();
    input.consumeIncludingWhitespace();
    return CssAngleValue::create(value, unitType.value());
}

RefPtr<CssValue> CssParser::consumeTransformValue(CssTokenStream& input)
{
    if(input->type() != CssToken::Type::Function)
        return nullptr;
    static constexpr auto table = makeIdentTable<CssFunctionID>({
        {"skew", CssFunctionID::Skew},
        {"skewx", CssFunctionID::SkewX},
        {"skewy", CssFunctionID::SkewY},
        {"scale", CssFunctionID::Scale},
        {"scalex", CssFunctionID::ScaleX},
        {"scaley", CssFunctionID::ScaleY},
        {"translate", CssFunctionID::Translate},
        {"translatex", CssFunctionID::TranslateX},
        {"translatey", CssFunctionID::TranslateY},
        {"rotate", CssFunctionID::Rotate},
        {"matrix", CssFunctionID::Matrix}
    });

    auto id = matchIdent(table, input->data());
    if(!id.has_value())
        return nullptr;
    CssValueList values;
    auto block = input.consumeBlock();
    block.consumeWhitespace();
    switch(id.value()) {
    case CssFunctionID::Skew:
    case CssFunctionID::SkewX:
    case CssFunctionID::SkewY:
    case CssFunctionID::Rotate: {
        auto value = consumeAngle(block);
        if(value == nullptr)
            return nullptr;
        values.push_back(std::move(value));
        if(id.value() == CssFunctionID::Skew && block->type() == CssToken::Type::Comma) {
            block.consumeIncludingWhitespace();
            auto value = consumeAngle(block);
            if(value == nullptr)
                return nullptr;
            values.push_back(std::move(value));
        }

        break;
    }

    case CssFunctionID::Scale:
    case CssFunctionID::ScaleX:
    case CssFunctionID::ScaleY: {
        auto value = consumeNumberOrPercent(block, true);
        if(value == nullptr)
            return nullptr;
        values.push_back(std::move(value));
        if(id.value() == CssFunctionID::Scale && block->type() == CssToken::Type::Comma) {
            block.consumeIncludingWhitespace();
            auto value = consumeNumberOrPercent(block, true);
            if(value == nullptr)
                return nullptr;
            values.push_back(std::move(value));
        }

        break;
    }

    case CssFunctionID::Translate:
    case CssFunctionID::TranslateX:
    case CssFunctionID::TranslateY: {
        auto value = consumeLengthOrPercent(block, true, false);
        if(value == nullptr)
            return nullptr;
        values.push_back(std::move(value));
        if(id.value() == CssFunctionID::Translate && block->type() == CssToken::Type::Comma) {
            block.consumeIncludingWhitespace();
            auto value = consumeLengthOrPercent(block, true, false);
            if(value == nullptr)
                return nullptr;
            values.push_back(std::move(value));
        }

        break;
    }

    case CssFunctionID::Matrix: {
        int count = 6;
        while(count > 0) {
            auto value = consumeNumber(block, true);
            if(value == nullptr)
                return nullptr;
            count -= 1;
            if(count > 0 && block->type() == CssToken::Type::Comma)
                block.consumeIncludingWhitespace();
            values.push_back(std::move(value));
        }

        break;
    }

    default:
        return nullptr;
    }

    if(!block.empty())
        return nullptr;
    input.consumeWhitespace();
    return CssFunctionValue::create(*id, std::move(values));
}

RefPtr<CssValue> CssParser::consumeTransform(CssTokenStream& input)
{
    if(auto value = consumeNone(input))
        return value;
    CssValueList values;
    do {
        auto value = consumeTransformValue(input);
        if(value == nullptr)
            return nullptr;
        values.push_back(std::move(value));
    } while(!input.empty());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumePaintOrder(CssTokenStream& input)
{
    if(auto value = consumeNormal(input))
        return value;
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"fill", CssValueID::Fill},
        {"stroke", CssValueID::Stroke},
        {"markers", CssValueID::Markers}
    });

    CssValueList values;
    do {
        auto value = consumeIdent(input, table);
        if(value == nullptr)
            return nullptr;
        values.push_back(std::move(value));
    } while(!input.empty());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumeLonghand(CssTokenStream& input, CssPropertyID id)
{
    switch(id) {
    case CssPropertyID::FlexGrow:
    case CssPropertyID::FlexShrink:
    case CssPropertyID::StrokeMiterlimit:
        return consumeNumber(input, false);
    case CssPropertyID::TabSize:
        return consumeLength(input, false, true);
    case CssPropertyID::OutlineOffset:
        return consumeLength(input, true, false);
    case CssPropertyID::BorderHorizontalSpacing:
    case CssPropertyID::BorderVerticalSpacing:
        return consumeLength(input, false, false);
    case CssPropertyID::Order:
        return consumeInteger(input, true);
    case CssPropertyID::Widows:
    case CssPropertyID::Orphans:
        return consumePositiveInteger(input);
    case CssPropertyID::ColumnCount:
        return consumePositiveIntegerOrAuto(input);
    case CssPropertyID::ZIndex:
        return consumeIntegerOrAuto(input, true);
    case CssPropertyID::X:
    case CssPropertyID::Y:
    case CssPropertyID::Cx:
    case CssPropertyID::Cy:
    case CssPropertyID::TextIndent:
        return consumeLengthOrPercent(input, true, false);
    case CssPropertyID::R:
    case CssPropertyID::Rx:
    case CssPropertyID::Ry:
    case CssPropertyID::PaddingTop:
    case CssPropertyID::PaddingRight:
    case CssPropertyID::PaddingBottom:
    case CssPropertyID::PaddingLeft:
        return consumeLengthOrPercent(input, false, false);
    case CssPropertyID::StrokeWidth:
        return consumeLengthOrPercent(input, false, true);
    case CssPropertyID::StrokeDashoffset:
        return consumeLengthOrPercent(input, true, true);
    case CssPropertyID::Opacity:
    case CssPropertyID::FillOpacity:
    case CssPropertyID::StrokeOpacity:
    case CssPropertyID::StopOpacity:
        return consumeNumberOrPercent(input, false);
    case CssPropertyID::PageScale:
        return consumeNumberOrPercentOrAuto(input, false);
    case CssPropertyID::Bottom:
    case CssPropertyID::Left:
    case CssPropertyID::Right:
    case CssPropertyID::Top:
    case CssPropertyID::MarginTop:
    case CssPropertyID::MarginRight:
    case CssPropertyID::MarginBottom:
    case CssPropertyID::MarginLeft:
        return consumeLengthOrPercentOrAuto(input, true, false);
    case CssPropertyID::Width:
    case CssPropertyID::Height:
    case CssPropertyID::MinWidth:
    case CssPropertyID::MinHeight:
        return consumeWidthOrHeightOrAuto(input, false);
    case CssPropertyID::MaxWidth:
    case CssPropertyID::MaxHeight:
        return consumeWidthOrHeightOrNone(input, false);
    case CssPropertyID::FlexBasis:
        return consumeWidthOrHeightOrAuto(input, false);
    case CssPropertyID::Fill:
    case CssPropertyID::Stroke:
        return consumePaint(input);
    case CssPropertyID::BorderBottomWidth:
    case CssPropertyID::BorderLeftWidth:
    case CssPropertyID::BorderRightWidth:
    case CssPropertyID::BorderTopWidth:
        return consumeLineWidth(input);
    case CssPropertyID::LineHeight:
        return consumeLengthOrPercentOrNormal(input, false, true);
    case CssPropertyID::LetterSpacing:
    case CssPropertyID::WordSpacing:
        return consumeLengthOrNormal(input, true, false);
    case CssPropertyID::OutlineWidth:
    case CssPropertyID::ColumnRuleWidth:
        return consumeLineWidth(input);
    case CssPropertyID::RowGap:
    case CssPropertyID::ColumnGap:
        return consumeLengthOrNormal(input, false, false);
    case CssPropertyID::ColumnWidth:
        return consumeLengthOrAuto(input, false, false);
    case CssPropertyID::Quotes:
        return consumeQuotes(input);
    case CssPropertyID::Clip:
        return consumeClip(input);
    case CssPropertyID::Size:
        return consumeSize(input);
    case CssPropertyID::Page:
        return consumePage(input);
    case CssPropertyID::FontWeight:
        return consumeFontWeight(input);
    case CssPropertyID::FontStretch:
        return consumeFontStretch(input);
    case CssPropertyID::FontStyle:
        return consumeFontStyle(input);
    case CssPropertyID::FontSize:
        return consumeFontSize(input);
    case CssPropertyID::FontFamily:
        return consumeFontFamily(input);
    case CssPropertyID::FontFeatureSettings:
        return consumeFontFeatureSettings(input);
    case CssPropertyID::FontVariationSettings:
        return consumeFontVariationSettings(input);
    case CssPropertyID::FontVariantCaps:
        return consumeFontVariantCaps(input);
    case CssPropertyID::FontVariantEmoji:
        return consumeFontVariantEmoji(input);
    case CssPropertyID::FontVariantPosition:
        return consumeFontVariantPosition(input);
    case CssPropertyID::FontVariantEastAsian:
        return consumeFontVariantEastAsian(input);
    case CssPropertyID::FontVariantLigatures:
        return consumeFontVariantLigatures(input);
    case CssPropertyID::FontVariantNumeric:
        return consumeFontVariantNumeric(input);
    case CssPropertyID::BorderBottomLeftRadius:
    case CssPropertyID::BorderBottomRightRadius:
    case CssPropertyID::BorderTopLeftRadius:
    case CssPropertyID::BorderTopRightRadius:
        return consumeBorderRadiusValue(input);
    case CssPropertyID::Color:
    case CssPropertyID::BackgroundColor:
    case CssPropertyID::TextDecorationColor:
    case CssPropertyID::StopColor:
    case CssPropertyID::OutlineColor:
    case CssPropertyID::ColumnRuleColor:
    case CssPropertyID::BorderBottomColor:
    case CssPropertyID::BorderLeftColor:
    case CssPropertyID::BorderRightColor:
    case CssPropertyID::BorderTopColor:
        return consumeColor(input);
    case CssPropertyID::ClipPath:
    case CssPropertyID::MarkerEnd:
    case CssPropertyID::MarkerMid:
    case CssPropertyID::MarkerStart:
    case CssPropertyID::Mask:
        return consumeLocalUrlOrNone(input);
    case CssPropertyID::ListStyleImage:
    case CssPropertyID::BackgroundImage:
        return consumeImageOrNone(input);
    case CssPropertyID::Content:
        return consumeContent(input);
    case CssPropertyID::CounterReset:
    case CssPropertyID::CounterSet:
        return consumeCounter(input, false);
    case CssPropertyID::CounterIncrement:
        return consumeCounter(input, true);
    case CssPropertyID::ListStyleType:
        return consumeListStyleType(input);
    case CssPropertyID::StrokeDasharray:
        return consumeDashList(input);
    case CssPropertyID::BaselineShift:
        return consumeBaselineShift(input);
    case CssPropertyID::Position:
        return consumePosition(input);
    case CssPropertyID::VerticalAlign:
        return consumeVerticalAlign(input);
    case CssPropertyID::TextDecorationLine:
        return consumeTextDecorationLine(input);
    case CssPropertyID::BackgroundSize:
        return consumeBackgroundSize(input);
    case CssPropertyID::BackgroundPosition:
    case CssPropertyID::ObjectPosition:
    case CssPropertyID::TransformOrigin:
        return consumePositionCoordinate(input);
    case CssPropertyID::Transform:
        return consumeTransform(input);
    case CssPropertyID::PaintOrder:
        return consumePaintOrder(input);
    case CssPropertyID::FontKerning: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"auto", CssValueID::Auto},
            {"normal", CssValueID::Normal},
            {"none", CssValueID::None}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::BackgroundAttachment: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"scroll", CssValueID::Scroll},
            {"fixed", CssValueID::Fixed},
            {"local", CssValueID::Local}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::BackgroundClip:
    case CssPropertyID::BackgroundOrigin: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"border-box", CssValueID::BorderBox},
            {"padding-box", CssValueID::PaddingBox},
            {"content-box", CssValueID::ContentBox}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::BackgroundRepeat: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"repeat", CssValueID::Repeat},
            {"repeat-x", CssValueID::RepeatX},
            {"repeat-y", CssValueID::RepeatY},
            {"no-repeat", CssValueID::NoRepeat}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::BorderCollapse: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"collapse", CssValueID::Collapse},
            {"separate", CssValueID::Separate}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::BorderTopStyle:
    case CssPropertyID::BorderRightStyle:
    case CssPropertyID::BorderBottomStyle:
    case CssPropertyID::BorderLeftStyle:
    case CssPropertyID::ColumnRuleStyle:
    case CssPropertyID::OutlineStyle: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"none", CssValueID::None},
            {"hidden", CssValueID::Hidden},
            {"inset", CssValueID::Inset},
            {"groove", CssValueID::Groove},
            {"ridge", CssValueID::Ridge},
            {"outset", CssValueID::Outset},
            {"dotted", CssValueID::Dotted},
            {"dashed", CssValueID::Dashed},
            {"solid", CssValueID::Solid},
            {"double", CssValueID::Double}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::BoxSizing: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"border-box", CssValueID::BorderBox},
            {"content-box", CssValueID::ContentBox}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::CaptionSide: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"top", CssValueID::Top},
            {"bottom", CssValueID::Bottom}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::Clear: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"none", CssValueID::None},
            {"left", CssValueID::Left},
            {"right", CssValueID::Right},
            {"both", CssValueID::Both}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::EmptyCells: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"show", CssValueID::Show},
            {"hide", CssValueID::Hide}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::FillRule:
    case CssPropertyID::ClipRule: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"nonzero", CssValueID::Nonzero},
            {"evenodd", CssValueID::Evenodd}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::Float: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"none", CssValueID::None},
            {"left", CssValueID::Left},
            {"right", CssValueID::Right}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::Hyphens: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"none", CssValueID::None},
            {"auto", CssValueID::Auto},
            {"manual", CssValueID::Manual}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::ListStylePosition: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"inside", CssValueID::Inside},
            {"outside", CssValueID::Outside}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::WordBreak: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"normal", CssValueID::Normal},
            {"keep-all", CssValueID::KeepAll},
            {"break-all", CssValueID::BreakAll},
            {"break-word", CssValueID::BreakWord}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::OverflowWrap: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"normal", CssValueID::Normal},
            {"anywhere", CssValueID::Anywhere},
            {"break-word", CssValueID::BreakWord}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::Overflow: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"auto", CssValueID::Auto},
            {"visible", CssValueID::Visible},
            {"hidden", CssValueID::Hidden},
            {"scroll", CssValueID::Scroll}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::BreakBefore:
    case CssPropertyID::BreakAfter: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"auto", CssValueID::Auto},
            {"avoid", CssValueID::Avoid},
            {"avoid-column", CssValueID::AvoidColumn},
            {"avoid-page", CssValueID::AvoidPage},
            {"column", CssValueID::Column},
            {"page", CssValueID::Page},
            {"left", CssValueID::Left},
            {"right", CssValueID::Right},
            {"recto", CssValueID::Recto},
            {"verso", CssValueID::Verso}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::BreakInside: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"auto", CssValueID::Auto},
            {"avoid", CssValueID::Avoid},
            {"avoid-column", CssValueID::AvoidColumn},
            {"avoid-page", CssValueID::AvoidPage}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::ColumnBreakBefore:
    case CssPropertyID::ColumnBreakAfter: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"auto", CssValueID::Auto},
            {"always", CssValueID::Column},
            {"avoid", CssValueID::Avoid}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::PageBreakBefore:
    case CssPropertyID::PageBreakAfter: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"auto", CssValueID::Auto},
            {"always", CssValueID::Page},
            {"avoid", CssValueID::Avoid},
            {"left", CssValueID::Left},
            {"right", CssValueID::Right}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::ColumnBreakInside:
    case CssPropertyID::PageBreakInside: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"auto", CssValueID::Auto},
            {"avoid", CssValueID::Avoid}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::StrokeLinecap: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"butt", CssValueID::Butt},
            {"round", CssValueID::Round},
            {"square", CssValueID::Square}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::StrokeLinejoin: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"miter", CssValueID::Miter},
            {"round", CssValueID::Round},
            {"bevel", CssValueID::Bevel}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::TableLayout: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"auto", CssValueID::Auto},
            {"fixed", CssValueID::Fixed}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::AlignmentBaseline: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"auto", CssValueID::Auto},
            {"baseline", CssValueID::Baseline},
            {"before-edge", CssValueID::BeforeEdge},
            {"text-before-edge", CssValueID::TextBeforeEdge},
            {"middle", CssValueID::Middle},
            {"central", CssValueID::Central},
            {"after-edge", CssValueID::AfterEdge},
            {"text-after-edge", CssValueID::TextAfterEdge},
            {"ideographic", CssValueID::Ideographic},
            {"alphabetic", CssValueID::Alphabetic},
            {"hanging", CssValueID::Hanging},
            {"mathematical", CssValueID::Mathematical}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::DominantBaseline: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"auto", CssValueID::Auto},
            {"use-script", CssValueID::UseScript},
            {"no-change", CssValueID::NoChange},
            {"reset-size", CssValueID::ResetSize},
            {"ideographic", CssValueID::Ideographic},
            {"alphabetic", CssValueID::Alphabetic},
            {"hanging", CssValueID::Hanging},
            {"mathematical", CssValueID::Mathematical},
            {"central", CssValueID::Central},
            {"middle", CssValueID::Middle},
            {"text-after-edge", CssValueID::TextAfterEdge},
            {"text-before-edge", CssValueID::TextBeforeEdge}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::TextAlign: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"left", CssValueID::Left},
            {"right", CssValueID::Right},
            {"center", CssValueID::Center},
            {"justify", CssValueID::Justify},
            {"start", CssValueID::Start},
            {"end", CssValueID::End}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::WritingMode: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"horizontal-tb", CssValueID::HorizontalTb},
            {"vertical-rl", CssValueID::VerticalRl},
            {"vertical-lr", CssValueID::VerticalLr},
            {"lr-tb", CssValueID::HorizontalTb},
            {"rl-tb", CssValueID::HorizontalTb},
            {"lr", CssValueID::HorizontalTb},
            {"rl", CssValueID::HorizontalTb},
            {"tb-rl", CssValueID::VerticalRl},
            {"tb", CssValueID::VerticalLr}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::TextOrientation: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"mixed", CssValueID::Mixed},
            {"upright", CssValueID::Upright}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::TextAnchor: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"start", CssValueID::Start},
            {"middle", CssValueID::Middle},
            {"end", CssValueID::End}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::TextDecorationStyle: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"solid", CssValueID::Solid},
            {"double", CssValueID::Double},
            {"dotted", CssValueID::Dotted},
            {"dashed", CssValueID::Dashed},
            {"wavy", CssValueID::Wavy}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::TextOverflow: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"clip", CssValueID::Clip},
            {"ellipsis", CssValueID::Ellipsis}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::TextTransform: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"none", CssValueID::None},
            {"capitalize", CssValueID::Capitalize},
            {"uppercase", CssValueID::Uppercase},
            {"lowercase", CssValueID::Lowercase}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::MixBlendMode: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"normal", CssValueID::Normal},
            {"multiply", CssValueID::Multiply},
            {"screen", CssValueID::Screen},
            {"overlay", CssValueID::Overlay},
            {"darken", CssValueID::Darken},
            {"lighten", CssValueID::Lighten},
            {"color-dodge", CssValueID::ColorDodge},
            {"color-burn", CssValueID::ColorBurn},
            {"hard-light", CssValueID::HardLight},
            {"soft-light", CssValueID::SoftLight},
            {"difference", CssValueID::Difference},
            {"exclusion", CssValueID::Exclusion},
            {"hue", CssValueID::Hue},
            {"saturation", CssValueID::Saturation},
            {"color", CssValueID::Color},
            {"luminosity", CssValueID::Luminosity}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::MaskType: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"luminance", CssValueID::Luminance},
            {"alpha", CssValueID::Alpha}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::VectorEffect: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"none", CssValueID::None},
            {"non-scaling-stroke", CssValueID::NonScalingStroke}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::Visibility: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"visible", CssValueID::Visible},
            {"hidden", CssValueID::Hidden},
            {"collapse", CssValueID::Collapse}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::Display: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"none", CssValueID::None},
            {"block", CssValueID::Block},
            {"flex", CssValueID::Flex},
            {"inline", CssValueID::Inline},
            {"inline-block", CssValueID::InlineBlock},
            {"inline-flex", CssValueID::InlineFlex},
            {"inline-table", CssValueID::InlineTable},
            {"list-item", CssValueID::ListItem},
            {"table", CssValueID::Table},
            {"table-caption", CssValueID::TableCaption},
            {"table-cell", CssValueID::TableCell},
            {"table-column", CssValueID::TableColumn},
            {"table-column-group", CssValueID::TableColumnGroup},
            {"table-footer-group", CssValueID::TableFooterGroup},
            {"table-header-group", CssValueID::TableHeaderGroup},
            {"table-row", CssValueID::TableRow},
            {"table-row-group", CssValueID::TableRowGroup}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::FlexDirection: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"row", CssValueID::Row},
            {"row-reverse", CssValueID::RowReverse},
            {"column", CssValueID::Column},
            {"column-reverse", CssValueID::ColumnReverse}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::FlexWrap: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"nowrap", CssValueID::Nowrap},
            {"wrap", CssValueID::Wrap},
            {"wrap-reverse", CssValueID::WrapReverse}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::WhiteSpace: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"normal", CssValueID::Normal},
            {"pre", CssValueID::Pre},
            {"pre-wrap", CssValueID::PreWrap},
            {"pre-line", CssValueID::PreLine},
            {"nowrap", CssValueID::Nowrap}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::Direction: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"ltr", CssValueID::Ltr},
            {"rtl", CssValueID::Rtl}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::UnicodeBidi: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"normal", CssValueID::Normal},
            {"embed", CssValueID::Embed},
            {"bidi-override", CssValueID::BidiOverride},
            {"isolate", CssValueID::Isolate},
            {"isolate-override", CssValueID::IsolateOverride}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::ColumnSpan: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"none", CssValueID::None},
            {"all", CssValueID::All}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::ColumnFill: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"auto", CssValueID::Auto},
            {"balance", CssValueID::Balance}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::JustifyContent: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"flex-start", CssValueID::FlexStart},
            {"flex-end", CssValueID::FlexEnd},
            {"center", CssValueID::Center},
            {"space-between", CssValueID::SpaceBetween},
            {"space-around", CssValueID::SpaceAround},
            {"space-evenly", CssValueID::SpaceEvenly}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::AlignContent: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"flex-start", CssValueID::FlexStart},
            {"flex-end", CssValueID::FlexEnd},
            {"center", CssValueID::Center},
            {"space-between", CssValueID::SpaceBetween},
            {"space-around", CssValueID::SpaceAround},
            {"space-evenly", CssValueID::SpaceEvenly},
            {"stretch", CssValueID::Stretch}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::AlignItems: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"flex-start", CssValueID::FlexStart},
            {"flex-end", CssValueID::FlexEnd},
            {"center", CssValueID::Center},
            {"baseline", CssValueID::Baseline},
            {"stretch", CssValueID::Stretch}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::AlignSelf: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"auto", CssValueID::Auto},
            {"flex-start", CssValueID::FlexStart},
            {"flex-end", CssValueID::FlexEnd},
            {"center", CssValueID::Center},
            {"baseline", CssValueID::Baseline},
            {"stretch", CssValueID::Stretch}
        });

        return consumeIdent(input, table);
    }

    case CssPropertyID::ObjectFit: {
        static constexpr auto table = makeIdentTable<CssValueID>({
            {"fill", CssValueID::Fill},
            {"contain", CssValueID::Contain},
            {"cover", CssValueID::Cover},
            {"none", CssValueID::None},
            {"scale-down", CssValueID::ScaleDown}
        });

        return consumeIdent(input, table);
    }

    default:
        return nullptr;
    }
}

bool CssParser::consumeFlex(CssTokenStream& input, CssPropertyList& properties, bool important)
{
    if(consumeIdentIncludingWhitespace(input, "none", 4)) {
        if(!input.empty())
            return false;
        addProperty(properties, CssPropertyID::FlexGrow, important, CssNumberValue::create(0.0));
        addProperty(properties, CssPropertyID::FlexShrink, important, CssNumberValue::create(0.0));
        addProperty(properties, CssPropertyID::FlexBasis, important, CssIdentValue::create(CssValueID::Auto));
        return true;
    }

    RefPtr<CssValue> grow;
    RefPtr<CssValue> shrink;
    RefPtr<CssValue> basis;
    for(int index = 0; index < 3; ++index) {
        if(input->type() == CssToken::Type::Number) {
            if(input->number() < 0.0)
                return false;
            if(grow == nullptr)
                grow = CssNumberValue::create(input->number());
            else if(shrink == nullptr)
                shrink = CssNumberValue::create(input->number());
            else if(input->number() == 0.0)
                basis = CssLengthValue::create(0.0, CssLengthUnits::None);
            else
                return false;
            input.consumeIncludingWhitespace();
            continue;
        }

        if(basis == nullptr && (basis = consumeWidthOrHeightOrAuto(input, false))) {
            if(index == 1 && !input.empty())
                return false;
            continue;
        }

        break;
    }

    if(!input.empty())
        return false;
    addProperty(properties, CssPropertyID::FlexGrow, important, std::move(grow));
    addProperty(properties, CssPropertyID::FlexShrink, important, std::move(shrink));
    addProperty(properties, CssPropertyID::FlexBasis, important, std::move(basis));
    return true;
}

bool CssParser::consumeBackground(CssTokenStream& input, CssPropertyList& properties, bool important)
{
    RefPtr<CssValue> color;
    RefPtr<CssValue> image;
    RefPtr<CssValue> repeat;
    RefPtr<CssValue> attachment;
    RefPtr<CssValue> origin;
    RefPtr<CssValue> clip;
    RefPtr<CssValue> position;
    RefPtr<CssValue> size;
    while(!input.empty()) {
        if(position == nullptr && (position = consumePositionCoordinate(input))) {
            if(input->type() == CssToken::Type::Delim && input->delim() == '/') {
                input.consumeIncludingWhitespace();
                if(size == nullptr && (size = consumeBackgroundSize(input)))
                    continue;
                return false;
            }

            continue;
        }

        if(image == nullptr && (image = consumeImage(input)))
            continue;
        if(repeat == nullptr && (repeat = consumeLonghand(input, CssPropertyID::BackgroundRepeat)))
            continue;
        if(attachment == nullptr && (attachment = consumeLonghand(input, CssPropertyID::BackgroundAttachment)))
            continue;
        if(origin == nullptr && (origin = consumeLonghand(input, CssPropertyID::BackgroundOrigin)))
            continue;
        if(clip == nullptr && (clip = consumeLonghand(input, CssPropertyID::BackgroundClip)))
            continue;
        if(color == nullptr && (color = consumeColor(input)))
            continue;
        return false;
    }

    if(clip == nullptr)
        clip = origin;
    addProperty(properties, CssPropertyID::BackgroundColor, important, std::move(color));
    addProperty(properties, CssPropertyID::BackgroundImage, important, std::move(image));
    addProperty(properties, CssPropertyID::BackgroundRepeat, important, std::move(repeat));
    addProperty(properties, CssPropertyID::BackgroundAttachment, important, std::move(attachment));
    addProperty(properties, CssPropertyID::BackgroundOrigin, important, std::move(origin));
    addProperty(properties, CssPropertyID::BackgroundClip, important, std::move(clip));
    addProperty(properties, CssPropertyID::BackgroundPosition, important, std::move(position));
    addProperty(properties, CssPropertyID::BackgroundSize, important, std::move(size));
    return true;
}

bool CssParser::consumeColumns(CssTokenStream& input, CssPropertyList& properties, bool important)
{
    RefPtr<CssValue> width;
    RefPtr<CssValue> count;
    for(int index = 0; index < 2; ++index) {
        if(consumeIdentIncludingWhitespace(input, "auto", 4))
            continue;
        if(width == nullptr && (width = consumeLength(input, false, false)))
            continue;
        if(count == nullptr && (count = consumePositiveInteger(input)))
            continue;
        break;
    }

    if(!input.empty())
        return false;
    addProperty(properties, CssPropertyID::ColumnWidth, important, std::move(width));
    addProperty(properties, CssPropertyID::ColumnCount, important, std::move(count));
    return true;
}

bool CssParser::consumeListStyle(CssTokenStream& input, CssPropertyList& properties, bool important)
{
    RefPtr<CssValue> none;
    RefPtr<CssValue> position;
    RefPtr<CssValue> image;
    RefPtr<CssValue> type;
    while(!input.empty()) {
        if(none == nullptr && (none = consumeNone(input)))
            continue;
        if(position == nullptr && (position = consumeLonghand(input, CssPropertyID::ListStylePosition)))
            continue;
        if(image == nullptr && (image = consumeLonghand(input, CssPropertyID::ListStyleImage)))
            continue;
        if(type == nullptr && (type = consumeLonghand(input, CssPropertyID::ListStyleType)))
            continue;
        return false;
    }

    if(none) {
        if(!type) {
            type = none;
        } else if(!image) {
            image = none;
        } else {
            return false;
        }
    }

    addProperty(properties, CssPropertyID::ListStyleType, important, std::move(type));
    addProperty(properties, CssPropertyID::ListStylePosition, important, std::move(position));
    addProperty(properties, CssPropertyID::ListStyleImage, important, std::move(image));
    return true;
}

bool CssParser::consumeFont(CssTokenStream& input, CssPropertyList& properties, bool important)
{
    RefPtr<CssValue> style;
    RefPtr<CssValue> weight;
    RefPtr<CssValue> variant;
    RefPtr<CssValue> stretch;
    for(int index = 0; index < 4; ++index) {
        if(consumeIdentIncludingWhitespace(input, "normal", 6))
            continue;
        if(style == nullptr && (style = consumeFontStyle(input)))
            continue;
        if(weight == nullptr && (weight = consumeFontWeight(input)))
            continue;
        if(variant == nullptr && (variant = consumeFontVariantCapsIdent(input)))
            continue;
        if(stretch == nullptr && (stretch = consumeFontStretchIdent(input)))
            continue;
        break;
    }

    if(input.empty())
        return false;
    addProperty(properties, CssPropertyID::FontStyle, important, std::move(style));
    addProperty(properties, CssPropertyID::FontWeight, important, std::move(weight));
    addProperty(properties, CssPropertyID::FontVariantCaps, important, std::move(variant));
    addProperty(properties, CssPropertyID::FontStretch, important, std::move(stretch));

    auto size = consumeFontSize(input);
    if(size == nullptr || input.empty())
        return false;
    addProperty(properties, CssPropertyID::FontSize, important, std::move(size));
    if(input->type() == CssToken::Type::Delim && input->delim() == '/') {
        input.consumeIncludingWhitespace();
        auto value = consumeLengthOrPercentOrNormal(input, false, true);
        if(value == nullptr)
            return false;
        addProperty(properties, CssPropertyID::LineHeight, important, std::move(value));
    } else {
        addProperty(properties, CssPropertyID::LineHeight, important, nullptr);
    }

    auto family = consumeFontFamily(input);
    if(family == nullptr || !input.empty())
        return false;
    addProperty(properties, CssPropertyID::FontFamily, important, std::move(family));
    return true;
}

bool CssParser::consumeFontVariant(CssTokenStream& input, CssPropertyList& properties, bool important)
{
    if(auto value = consumeNoneOrNormal(input)) {
        if(!input.empty())
            return false;
        addProperty(properties, CssPropertyID::FontVariantCaps, important, nullptr);
        addProperty(properties, CssPropertyID::FontVariantEmoji, important, nullptr);
        addProperty(properties, CssPropertyID::FontVariantPosition, important, nullptr);
        addProperty(properties, CssPropertyID::FontVariantEastAsian, important, nullptr);
        addProperty(properties, CssPropertyID::FontVariantNumeric, important, nullptr);
        addProperty(properties, CssPropertyID::FontVariantLigatures, important, std::move(value));
        return true;
    }

    RefPtr<CssValue> caps;
    RefPtr<CssValue> emoji;
    RefPtr<CssValue> position;

    CssValueList eastAsian;
    CssValueList ligatures;
    CssValueList numeric;
    while(!input.empty()) {
        if(caps == nullptr && (caps = consumeFontVariantCapsIdent(input)))
            continue;
        if(emoji == nullptr && (emoji = consumeFontVariantEmojiIdent(input)))
            continue;
        if(position == nullptr && (position = consumeFontVariantPositionIdent(input)))
            continue;
        if(auto value = consumeFontVariantEastAsianIdent(input)) {
            eastAsian.push_back(std::move(value));
            continue;
        }

        if(auto value = consumeFontVariantLigaturesIdent(input)) {
            ligatures.push_back(std::move(value));
            continue;
        }

        if(auto value = consumeFontVariantNumericIdent(input)) {
            numeric.push_back(std::move(value));
            continue;
        }

        return false;
    }

    addProperty(properties, CssPropertyID::FontVariantCaps, important, std::move(caps));
    addProperty(properties, CssPropertyID::FontVariantEmoji, important, std::move(emoji));
    addProperty(properties, CssPropertyID::FontVariantPosition, important, std::move(position));
    auto addListProperty = [&](CssPropertyID id, CssValueList&& values) {
        if(values.empty())
            addProperty(properties, id, important, CssIdentValue::create(CssValueID::Normal));
        else {
            addProperty(properties, id, important, CssListValue::create(std::move(values)));
        }
    };

    addListProperty(CssPropertyID::FontVariantEastAsian, std::move(eastAsian));
    addListProperty(CssPropertyID::FontVariantLigatures, std::move(ligatures));
    addListProperty(CssPropertyID::FontVariantNumeric, std::move(numeric));
    return true;
}

bool CssParser::consumeBorder(CssTokenStream& input, CssPropertyList& properties, bool important)
{
    RefPtr<CssValue> width;
    RefPtr<CssValue> style;
    RefPtr<CssValue> color;
    while(!input.empty()) {
        if(width == nullptr && (width = consumeLineWidth(input)))
            continue;
        if(style == nullptr && (style = consumeLonghand(input, CssPropertyID::BorderTopStyle)))
            continue;
        if(color == nullptr && (color = consumeColor(input)))
            continue;
        return false;
    }

    addExpandedProperty(properties, CssPropertyID::BorderWidth, important, std::move(width));
    addExpandedProperty(properties, CssPropertyID::BorderStyle, important, std::move(style));
    addExpandedProperty(properties, CssPropertyID::BorderColor, important, std::move(color));
    return true;
}

bool CssParser::consumeBorderRadius(CssTokenStream& input, CssPropertyList& properties, bool important)
{
    auto completesides = [](auto sides[4]) {
        if(sides[1] == nullptr) sides[1] = sides[0];
        if(sides[2] == nullptr) sides[2] = sides[0];
        if(sides[3] == nullptr) sides[3] = sides[1];
    };

    RefPtr<CssValue> horizontal[4];
    for(auto& side : horizontal) {
        if(input.empty() || input->type() == CssToken::Type::Delim)
            break;
        auto value = consumeLengthOrPercent(input, false, false);
        if(value == nullptr)
            return false;
        side = std::move(value);
    }

    if(horizontal[0] == nullptr)
        return false;
    completesides(horizontal);

    RefPtr<CssValue> vertical[4];
    if(input->type() == CssToken::Type::Delim && input->delim() == '/') {
        input.consumeIncludingWhitespace();
        for(auto& side : vertical) {
            if(input->type() == CssToken::Type::EndOfFile)
                break;
            auto value = consumeLengthOrPercent(input, false, false);
            if(value == nullptr)
                return false;
            side = std::move(value);
        }

        if(vertical[0] == nullptr)
            return false;
        completesides(vertical);
    } else if(input->type() == CssToken::Type::EndOfFile) {
        vertical[0] = horizontal[0];
        vertical[1] = horizontal[1];
        vertical[2] = horizontal[2];
        vertical[3] = horizontal[3];
    } else {
        return false;
    }

    auto tl = CssPairValue::create(horizontal[0], vertical[0]);
    auto tr = CssPairValue::create(horizontal[1], vertical[1]);
    auto br = CssPairValue::create(horizontal[2], vertical[2]);
    auto bl = CssPairValue::create(horizontal[3], vertical[3]);

    addProperty(properties, CssPropertyID::BorderTopLeftRadius, important, std::move(tl));
    addProperty(properties, CssPropertyID::BorderTopRightRadius, important, std::move(tr));
    addProperty(properties, CssPropertyID::BorderBottomRightRadius, important, std::move(br));
    addProperty(properties, CssPropertyID::BorderBottomLeftRadius, important, std::move(bl));
    return true;
}

bool CssParser::consumeMarker(CssTokenStream& input, CssPropertyList& properties, bool important)
{
    auto marker = consumeLocalUrlOrNone(input);
    if(!marker || !input.empty())
        return false;
    addProperty(properties, CssPropertyID::MarkerStart, important, marker);
    addProperty(properties, CssPropertyID::MarkerMid, important, marker);
    addProperty(properties, CssPropertyID::MarkerEnd, important, marker);
    return true;
}

bool CssParser::consume2Shorthand(CssTokenStream& input, CssPropertyList& properties, CssPropertyID id, bool important)
{
    auto longhand = expandShorthand(id);
    assert(longhand.size() == 2);
    auto first = consumeLonghand(input, longhand[0]);
    if(first == nullptr)
        return false;
    addProperty(properties, longhand[0], important, first);
    auto second = consumeLonghand(input, longhand[1]);
    if(second == nullptr) {
        addProperty(properties, longhand[1], important, first);
        return true;
    }

    addProperty(properties, longhand[1], important, second);
    return true;
}

bool CssParser::consume4Shorthand(CssTokenStream& input, CssPropertyList& properties, CssPropertyID id, bool important)
{
    auto longhand = expandShorthand(id);
    assert(longhand.size() == 4);
    auto top = consumeLonghand(input, longhand[0]);
    if(top == nullptr)
        return false;
    addProperty(properties, longhand[0], important, top);
    auto right = consumeLonghand(input, longhand[1]);
    if(right == nullptr) {
        addProperty(properties, longhand[1], important, top);
        addProperty(properties, longhand[2], important, top);
        addProperty(properties, longhand[3], important, top);
        return true;
    }

    addProperty(properties, longhand[1], important, right);
    auto bottom = consumeLonghand(input, longhand[1]);
    if(bottom == nullptr) {
        addProperty(properties, longhand[2], important, top);
        addProperty(properties, longhand[3], important, right);
        return true;
    }

    addProperty(properties, longhand[2], important, bottom);
    auto left = consumeLonghand(input, longhand[3]);
    if(left == nullptr) {
        addProperty(properties, longhand[3], important, right);
        return true;
    }

    addProperty(properties, longhand[3], important, left);
    return true;
}

bool CssParser::consumeShorthand(CssTokenStream& input, CssPropertyList& properties, CssPropertyID id, bool important)
{
    RefPtr<CssValue> values[6];
    auto longhand = expandShorthand(id);
    const auto n = longhand.size();
    assert(n <= std::size(values));
    while(!input.empty()) {
        bool consumed = false;
        for(size_t i = 0; i != n; ++i) {
            if(values[i] == nullptr && (values[i] = consumeLonghand(input, longhand[i]))) {
                consumed = true;
            }
        }

        if(!consumed) {
            return false;
        }
    }

    for(size_t i = 0; i != n; ++i)
        addProperty(properties, longhand[i], important, std::move(values[i]));
    return true;
}

RefPtr<CssValue> CssParser::consumeFontFaceSource(CssTokenStream& input)
{
    CssValueList values;
    if(input->type() == CssToken::Type::Function && matchLower(input->data(), "local")) {
        auto block = input.consumeBlock();
        block.consumeWhitespace();
        auto value = consumeFontFamilyName(block);
        if(value == nullptr || !block.empty())
            return nullptr;
        auto function = CssUnaryFunctionValue::create(CssFunctionID::Local, std::move(value));
        input.consumeWhitespace();
        values.push_back(std::move(function));
    } else {
        auto url = consumeUrl(input);
        if(url == nullptr)
            return nullptr;
        values.push_back(std::move(url));
        if(input->type() == CssToken::Type::Function && matchLower(input->data(), "format")) {
            auto block = input.consumeBlock();
            block.consumeWhitespace();
            auto value = consumeStringOrCustomIdent(block);
            if(value == nullptr || !block.empty())
                return nullptr;
            auto format = CssUnaryFunctionValue::create(CssFunctionID::Format, std::move(value));
            input.consumeWhitespace();
            values.push_back(std::move(format));
        }
    }

    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumeFontFaceSrc(CssTokenStream& input)
{
    CssValueList values;
    do {
        auto value = consumeFontFaceSource(input);
        if(value == nullptr)
            return nullptr;
        values.push_back(std::move(value));
    } while(input.consumeCommaIncludingWhitespace());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumeFontFaceWeight(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"normal", CssValueID::Normal},
        {"bold", CssValueID::Bold}
    });

    if(auto value = consumeIdent(input, table))
        return value;
    auto startWeight = consumeNumber(input, false);
    if(startWeight == nullptr)
        return nullptr;
    auto endWeight = consumeNumber(input, false);
    if(endWeight == nullptr)
        endWeight = startWeight;
    return CssPairValue::create(startWeight, endWeight);
}

RefPtr<CssValue> CssParser::consumeFontFaceStyle(CssTokenStream& input)
{
    auto ident = consumeFontStyleIdent(input);
    if(ident == nullptr)
        return nullptr;
    if(ident->value() != CssValueID::Oblique)
        return ident;
    auto startAngle = consumeAngle(input);
    if(startAngle == nullptr)
        return ident;
    auto endAngle = consumeAngle(input);
    if(endAngle == nullptr)
        endAngle = startAngle;
    CssValueList values;
    values.push_back(std::move(ident));
    values.push_back(std::move(startAngle));
    values.push_back(std::move(endAngle));
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumeFontFaceStretch(CssTokenStream& input)
{
    if(auto value = consumeFontStretchIdent(input))
        return value;
    auto startPercent = consumePercent(input, false);
    if(startPercent == nullptr)
        return nullptr;
    auto endPercent = consumePercent(input, false);
    if(endPercent == nullptr)
        endPercent = startPercent;
    return CssPairValue::create(startPercent, endPercent);
}

RefPtr<CssValue> CssParser::consumeFontFaceUnicodeRange(CssTokenStream& input)
{
    CssValueList values;
    do {
        if(input->type() != CssToken::Type::UnicodeRange)
            return nullptr;
        if(input->to() > 0x10FFFF || input->from() > input->to())
            return nullptr;
        values.push_back(CssUnicodeRangeValue::create(input->from(), input->to()));
        input.consumeIncludingWhitespace();
    } while(input.consumeCommaIncludingWhitespace());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumeCounterStyleName(CssTokenStream& input)
{
    if(input->type() != CssToken::Type::Ident || matchLower(input->data(), "none"))
        return nullptr;
    const auto name = GlobalString::get(input->data());
    input.consumeIncludingWhitespace();
    return CssCustomIdentValue::create(name);
}

RefPtr<CssValue> CssParser::consumeCounterStyleSystem(CssTokenStream& input)
{
    static constexpr auto table = makeIdentTable<CssValueID>({
        {"cyclic", CssValueID::Cyclic},
        {"symbolic", CssValueID::Symbolic},
        {"alphabetic", CssValueID::Alphabetic},
        {"numeric", CssValueID::Numeric},
        {"additive", CssValueID::Additive},
        {"fixed", CssValueID::Fixed},
        {"extends", CssValueID::Extends}
    });

    auto ident = consumeIdent(input, table);
    if(ident == nullptr)
        return nullptr;
    if(ident->value() == CssValueID::Fixed) {
        auto fixed = consumeInteger(input, true);
        if(fixed == nullptr)
            fixed = CssIntegerValue::create(1);
        return CssPairValue::create(ident, fixed);
    }

    if(ident->value() == CssValueID::Extends) {
        auto extends = consumeCounterStyleName(input);
        if(extends == nullptr)
            return nullptr;
        return CssPairValue::create(ident, extends);
    }

    return ident;
}

RefPtr<CssValue> CssParser::consumeCounterStyleNegative(CssTokenStream& input)
{
    auto prepend = consumeCounterStyleSymbol(input);
    if(prepend == nullptr)
        return nullptr;
    if(auto append = consumeCounterStyleSymbol(input))
        return CssPairValue::create(prepend, append);
    return prepend;
}

RefPtr<CssValue> CssParser::consumeCounterStyleSymbol(CssTokenStream& input)
{
    if(auto value = consumeStringOrCustomIdent(input))
        return value;
    return consumeImage(input);
}

RefPtr<CssValue> CssParser::consumeCounterStyleRangeBound(CssTokenStream& input)
{
    if(consumeIdentIncludingWhitespace(input, "infinite", 8))
        return CssIdentValue::create(CssValueID::Infinite);
    return consumeInteger(input, true);
}

RefPtr<CssValue> CssParser::consumeCounterStyleRange(CssTokenStream& input)
{
    if(auto value = consumeAuto(input))
        return value;
    CssValueList values;
    do {
        auto lowerBound = consumeCounterStyleRangeBound(input);
        if(lowerBound == nullptr)
            return nullptr;
        auto upperBound = consumeCounterStyleRangeBound(input);
        if(upperBound == nullptr)
            return nullptr;
        values.push_back(CssPairValue::create(lowerBound, upperBound));
    } while(input.consumeCommaIncludingWhitespace());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumeCounterStylePad(CssTokenStream& input)
{
    RefPtr<CssValue> integer;
    RefPtr<CssValue> symbol;
    while(!integer || !symbol) {
        if(integer == nullptr && (integer = consumeInteger(input, false)))
            continue;
        if(symbol == nullptr && (symbol = consumeCounterStyleSymbol(input)))
            continue;
        return nullptr;
    }

    return CssPairValue::create(integer, symbol);
}

RefPtr<CssValue> CssParser::consumeCounterStyleSymbols(CssTokenStream& input)
{
    CssValueList values;
    do {
        auto symbol = consumeCounterStyleSymbol(input);
        if(symbol == nullptr)
            return nullptr;
        values.push_back(std::move(symbol));
    } while(!input.empty());
    return CssListValue::create(std::move(values));
}

RefPtr<CssValue> CssParser::consumeCounterStyleAdditiveSymbols(CssTokenStream& input)
{
    CssValueList values;
    do {
        auto value = consumeCounterStylePad(input);
        if(value == nullptr)
            return nullptr;
        values.push_back(std::move(value));
    } while(input.consumeCommaIncludingWhitespace());
    return CssListValue::create(std::move(values));
}

GlobalString CssParser::determineNamespace(GlobalString prefix) const
{
    if(prefix.isEmpty())
        return m_defaultNamespace;
    if(prefix == starGlo)
        return starGlo;
    if(m_namespaces.contains(prefix))
        return m_namespaces.at(prefix);
    return emptyGlo;
}

} // namespace plutobook
