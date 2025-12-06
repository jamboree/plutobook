#include "css-tokenizer.h"
#include "string-utils.h"

#include <cmath>

namespace plutobook {

constexpr bool isNameStart(char cc) { return isAlpha(cc) || cc == '_'; }
constexpr bool isNameChar(char cc) { return isNameStart(cc) || isDigit(cc) || cc == '-'; }
constexpr bool isNewLine(char cc) { return (cc == '\n' || cc == '\r' || cc == '\f'); }
constexpr bool isNonPrintable(char cc) { return (cc >= 0 && cc <= 0x8) || cc == 0xb || (cc >= 0xf && cc <= 0x1f) || cc == 0x7f; }

const CssToken CssTokenStream::eofToken(CssToken::Type::EndOfFile);

void CssTokenStream::consumeComponent() {
    assert(m_begin < m_end);
    switch (m_begin->type()) {
    case CssToken::Type::Function:
    case CssToken::Type::LeftParenthesis:
    case CssToken::Type::LeftSquareBracket:
    case CssToken::Type::LeftCurlyBracket: {
        auto closeType = CssToken::closeType(m_begin->type());
        ++m_begin;
        while (m_begin < m_end && m_begin->type() != closeType)
            consumeComponent();
        if (m_begin < m_end)
            ++m_begin;
        break;
    }

    default: ++m_begin; break;
    }
}

CssTokenStream CssTokenStream::consumeBlock() {
    assert(m_begin < m_end);
    auto closeType = CssToken::closeType(m_begin->type());
    ++m_begin;
    auto blockBegin = m_begin;
    while (m_begin < m_end && m_begin->type() != closeType)
        consumeComponent();
    auto blockEnd = m_begin;
    if (m_begin < m_end)
        ++m_begin;
    return CssTokenStream(blockBegin, blockEnd);
}

CssTokenizer::CssTokenizer(const std::string_view& input)
    : m_input(input)
{
    m_tokenList.reserve(input.length() / 3);
}

CssTokenStream CssTokenizer::tokenize()
{
    while(true) {
        auto token = nextToken();
        if(token.type() == CssToken::Type::Comment)
            continue;
        if(token.type() == CssToken::Type::EndOfFile)
            break;
        m_tokenList.push_back(token);
    }

    return CssTokenStream(m_tokenList.data(), m_tokenList.size());
}

bool CssTokenizer::isEscapeSequence(char first, char second)
{
    return first == '\\' && !isNewLine(second);
}

bool CssTokenizer::isIdentSequence(char first, char second, char third)
{
    if(isNameStart(first) || isEscapeSequence(first, second))
        return true;
    if(first == '-')
        return isNameStart(second) || second == '-' || isEscapeSequence(second, third);
    return false;
}

bool CssTokenizer::isNumberSequence(char first, char second, char third)
{
    if(isDigit(first))
        return true;
    if(first == '-' || first == '+')
        return isDigit(second) || (second == '.' && isDigit(third));
    if(first == '.')
        return isDigit(second);
    return false;
}

bool CssTokenizer::isEscapeSequence() const
{
    if(m_input.empty())
        return false;
    return isEscapeSequence(*m_input, m_input.peek(1));
}

bool CssTokenizer::isIdentSequence() const
{
    if(m_input.empty())
        return false;
    auto second = m_input.peek(1);
    if(second == 0)
        return isIdentSequence(*m_input, 0, 0);
    return isIdentSequence(*m_input, second, m_input.peek(2));
}

bool CssTokenizer::isNumberSequence() const
{
    if(m_input.empty())
        return false;
    auto second = m_input.peek(1);
    if(second == 0)
        return isNumberSequence(*m_input, 0, 0);
    return isNumberSequence(*m_input, second, m_input.peek(2));
}

bool CssTokenizer::isExponentSequence() const
{
    if(m_input.peek() == 'E' || m_input.peek() == 'e') {
        if(m_input.peek(1) == '+' || m_input.peek(1) == '-')
            return isDigit(m_input.peek(2));
        return isDigit(m_input.peek(1));
    }

    return false;
}

bool CssTokenizer::isUnicodeRangeSequence() const
{
    if(m_input.peek() == 'U' || m_input.peek() == 'u')
        return m_input.peek(1) == '+' && (m_input.peek(2) == '?' || isHexDigit(m_input.peek(2)));
    return false;
}

std::string_view CssTokenizer::addstring(std::string&& value)
{
    m_stringList.push_back(std::move(value));
    return m_stringList.back();
}

std::string_view CssTokenizer::consumeName()
{
    size_t count = 0;
    while(true) {
        auto cc = m_input.peek(count);
        if(cc == 0 || cc == '\\')
            break;
        if(!isNameChar(cc)) {
            auto offset = m_input.offset();
            m_input.advance(count);
            return m_input.substring(offset, count);
        }

        count += 1;
    }

    std::string output;
    while(true) {
        auto cc = m_input.peek();
        if(isNameChar(cc)) {
            output += cc;
            m_input.advance();
        } else if(isEscapeSequence()) {
            appendCodepoint(output, consumeEscape());
        } else {
            break;
        }
    }

    return addstring(std::move(output));
}

uint32_t CssTokenizer::consumeEscape()
{
    assert(isEscapeSequence());
    auto cc = m_input.consume();
    if(isHexDigit(cc)) {
        int count = 0;
        uint32_t cp = 0;
        do {
            cp = cp * 16 + toHexDigit(cc);
            cc = m_input.consume();
            count += 1;
        } while(count < 6 && isHexDigit(cc));
        if(isSpace(cc)) {
            if(cc == '\r' && m_input.peek(1) == '\n')
                m_input.advance();
            m_input.advance();
        }

        if(cp == 0 || cp >= 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
            return 0xFFFD;
        return cp;
    }

    if(cc == 0)
        return 0xFFFD;
    m_input.advance();
    return cc;
}

CssToken CssTokenizer::consumeStringToken()
{
    auto endingCodePoint = m_input.peek();
    assert(endingCodePoint == '\"' || endingCodePoint == '\'');
    m_input.advance();

    size_t count = 0;
    while(true) {
        auto cc = m_input.peek(count);
        if(cc == 0 || cc == '\\')
            break;
        if(cc == endingCodePoint) {
            auto offset = m_input.offset();
            m_input.advance(count + 1);
            return CssToken(CssToken::Type::String, m_input.substring(offset, count));
        }

        if(isNewLine(cc)) {
            m_input.advance(count);
            return CssToken(CssToken::Type::BadString);
        }

        count += 1;
    }

    std::string output;
    while(true) {
        auto cc = m_input.peek();
        if(cc == 0)
            break;
        if(cc == endingCodePoint) {
            m_input.advance();
            break;
        }

        if(isNewLine(cc))
            return CssToken(CssToken::Type::BadString);
        if(cc == '\\') {
            auto next = m_input.peek(1);
            if(next == 0) {
                m_input.advance();
            } else if(isNewLine(next)) {
                if(next == '\r' && m_input.peek(2) == '\n')
                    m_input.advance();
                m_input.advance(2);
            } else {
                appendCodepoint(output, consumeEscape());
            }
        } else {
            output += cc;
            m_input.advance();
        }
    }

    if(output.empty())
        return CssToken(CssToken::Type::String);
    return CssToken(CssToken::Type::String, addstring(std::move(output)));
}

CssToken CssTokenizer::consumeNumericToken()
{
    assert(isNumberSequence());
    auto numberType = CssToken::NumberType::Integer;
    auto numberSign = CssToken::NumberSign::None;
    double integer = 0;
    double fraction = 0;
    int exponent = 0;
    int expsign = 1;

    if(m_input.peek() == '-') {
        numberSign = CssToken::NumberSign::Minus;
        m_input.advance();
    } else if(m_input.peek() == '+') {
        numberSign = CssToken::NumberSign::Plus;
        m_input.advance();
    }

    if(isDigit(m_input.peek())) {
        auto cc = m_input.peek();
        do {
            integer = 10.0 * integer + (cc - '0');
            cc = m_input.consume();
        } while(isDigit(cc));
    }

    if(m_input.peek() == '.' && isDigit(m_input.peek(1))) {
        numberType = CssToken::NumberType::Number;
        auto cc = m_input.consume();
        int count = 0;
        do {
            fraction = 10.0 * fraction + (cc - '0');
            count += 1;
            cc = m_input.consume();
        } while(isDigit(cc));
        fraction *= std::pow(10.0, -count);
    }

    if(isExponentSequence()) {
        numberType = CssToken::NumberType::Number;
        m_input.advance();
        if(m_input.peek() == '-') {
            expsign = -1;
            m_input.advance();
        } else if(m_input.peek() == '+') {
            m_input.advance();
        }

        auto cc = m_input.peek();
        do {
            exponent = 10 * exponent + (cc - '0');
            cc = m_input.consume();
        } while(isDigit(cc));
    }

    double number = (integer + fraction) * std::pow(10.0, exponent * expsign);
    if(numberSign == CssToken::NumberSign::Minus)
        number = -number;
    if(m_input.peek() == '%') {
        m_input.advance();
        return CssToken(CssToken::Type::Percentage, numberType, numberSign, number);
    }

    if(isIdentSequence())
        return CssToken(CssToken::Type::Dimension, numberType, numberSign, number, consumeName());
    return CssToken(CssToken::Type::Number, numberType, numberSign, number);
}

CssToken CssTokenizer::consumeUnicodeRangeToken()
{
    assert(m_input.peek() == 'U' || m_input.peek() == 'u');
    m_input.advance();
    assert(m_input.peek() == '+');

    auto cc = m_input.consume();
    assert(cc == '?' || isHexDigit(cc));

    int count = 0;
    uint32_t from = 0;
    if(isHexDigit(cc)) {
        do {
            from = from * 16 + toHexDigit(cc);
            cc = m_input.consume();
            count += 1;
        } while(count < 6 && isHexDigit(cc));
    }

    uint32_t to = from;
    if(count < 6 && cc == '?') {
        m_input.advance();
        do {
            from *= 16;
            to = to * 16 + 0xF;
            cc = m_input.consume();
            count += 1;
        } while(count < 6 && cc == '?');
    } else if(cc == '-' && isHexDigit(m_input.peek(1))) {
        cc = m_input.consume();
        count = 0;
        to = 0;
        do {
            to = to * 16 + toHexDigit(cc);
            cc = m_input.consume();
            count += 1;
        } while(count < 6 && isHexDigit(cc));
    }

    return CssToken(CssToken::Type::UnicodeRange, from, to);
}

CssToken CssTokenizer::consumeIdentLikeToken()
{
    if(isUnicodeRangeSequence())
        return consumeUnicodeRangeToken();
    auto name = consumeName();
    if(iequals(name, "url") && m_input.peek() == '(') {
        auto cc = m_input.consume();
        while(isSpace(cc)) {
            cc = m_input.consume();
        }

        if(cc == '"' || cc == '\'')
            return CssToken(CssToken::Type::Function, name);
        return consumeUrlToken();
    }

    if(m_input.peek() == '(') {
        m_input.advance();
        return CssToken(CssToken::Type::Function, name);
    }

    return CssToken(CssToken::Type::Ident, name);
}

CssToken CssTokenizer::consumeUrlToken()
{
    auto cc = m_input.peek();
    while(isSpace(cc)) {
        cc = m_input.consume();
    }

    size_t count = 0;
    while(true) {
        auto cc = m_input.peek(count);
        if(cc == 0 || cc == '\\' || isSpace(cc))
            break;
        if(cc == ')') {
            auto offset = m_input.offset();
            m_input.advance(count + 1);
            return CssToken(CssToken::Type::Url, m_input.substring(offset, count));
        }

        if(cc == '"' || cc == '\'' || cc == '(' || isNonPrintable(cc)) {
            m_input.advance(count);
            return consumeBadUrlRemnants();
        }

        count += 1;
    }

    std::string output;
    while(true) {
        auto cc = m_input.peek();
        if(cc == 0)
            break;
        if(cc == ')') {
            m_input.advance();
            break;
        }

        if(cc == '\\') {
            if(isEscapeSequence()) {
                appendCodepoint(output, consumeEscape());
                continue;
            }

            return consumeBadUrlRemnants();
        }

        if(isSpace(cc)) {
            do {
                cc = m_input.consume();
            } while(isSpace(cc));
            if(cc == 0)
                break;
            if(cc == ')') {
                m_input.advance();
                break;
            }

            return consumeBadUrlRemnants();
        }

        if(cc == '"' || cc == '\'' || cc == '(' || isNonPrintable(cc))
            return consumeBadUrlRemnants();
        output += cc;
        m_input.advance();
    }

    return CssToken(CssToken::Type::Url, addstring(std::move(output)));
}

CssToken CssTokenizer::consumeBadUrlRemnants()
{
    while(true) {
        auto cc = m_input.peek();
        if(cc == 0)
            break;
        if(cc == ')') {
            m_input.advance();
            break;
        }

        if(isEscapeSequence()) {
            consumeEscape();
        } else {
            m_input.advance();
        }
    }

    return CssToken(CssToken::Type::BadUrl);
}

CssToken CssTokenizer::consumeWhitespaceToken()
{
    auto cc = m_input.peek();
    assert(isSpace(cc));
    do {
        cc = m_input.consume();
    } while(isSpace(cc));
    return CssToken(CssToken::Type::Whitespace);
}

CssToken CssTokenizer::consumeCommentToken()
{
    while(true) {
        auto cc = m_input.peek();
        if(cc == 0)
            break;
        if(cc == '*' && m_input.peek(1) == '/') {
            m_input.advance(2);
            break;
        }

        m_input.advance();
    }

    return CssToken(CssToken::Type::Comment);
}

CssToken CssTokenizer::consumeSolidusToken()
{
    auto cc = m_input.consume();
    if(cc == '*') {
        m_input.advance();
        return consumeCommentToken();
    }

    return CssToken(CssToken::Type::Delim, '/');
}

CssToken CssTokenizer::consumeHashToken()
{
    auto cc = m_input.consume();
    if(isNameChar(cc) || isEscapeSequence()) {
        if(isIdentSequence())
            return CssToken(CssToken::Type::Hash, CssToken::HashType::Identifier, consumeName());
        return CssToken(CssToken::Type::Hash, CssToken::HashType::Unrestricted, consumeName());
    }

    return CssToken(CssToken::Type::Delim, '#');
}

CssToken CssTokenizer::consumePlusSignToken()
{
    if(isNumberSequence())
        return consumeNumericToken();
    m_input.advance();
    return CssToken(CssToken::Type::Delim, '+');
}

CssToken CssTokenizer::consumeHyphenMinusToken()
{
    if(isNumberSequence())
        return consumeNumericToken();
    if(m_input.peek(1) == '-' && m_input.peek(2) == '>') {
        m_input.advance(3);
        return CssToken(CssToken::Type::CDC);
    }

    if(isIdentSequence())
        return consumeIdentLikeToken();
    m_input.advance();
    return CssToken(CssToken::Type::Delim, '-');
}

CssToken CssTokenizer::consumeFullStopToken()
{
    if(isNumberSequence())
        return consumeNumericToken();
    m_input.advance();
    return CssToken(CssToken::Type::Delim, '.');
}

CssToken CssTokenizer::consumeLessThanSignToken()
{
    auto cc = m_input.consume();
    if(cc == '!' && m_input.peek(1) == '-' && m_input.peek(2) == '-') {
        m_input.advance(3);
        return CssToken(CssToken::Type::CDO);
    }

    return CssToken(CssToken::Type::Delim, '<');
}

CssToken CssTokenizer::consumeCommercialAtToken()
{
    m_input.advance();
    if(isIdentSequence())
        return CssToken(CssToken::Type::AtKeyword, consumeName());
    return CssToken(CssToken::Type::Delim, '@');
}

CssToken CssTokenizer::consumeReverseSolidusToken()
{
    if(isEscapeSequence())
        return consumeIdentLikeToken();
    m_input.advance();
    return CssToken(CssToken::Type::Delim, '\\');
}

CssToken CssTokenizer::nextToken()
{
    auto cc = m_input.peek();
    if(cc == 0)
        return CssToken(CssToken::Type::EndOfFile);
    if(isSpace(cc))
        return consumeWhitespaceToken();
    if(isDigit(cc))
        return consumeNumericToken();
    if(isNameStart(cc)) {
        return consumeIdentLikeToken();
    }

    switch(cc) {
    case '/':
        return consumeSolidusToken();
    case '#':
        return consumeHashToken();
    case '+':
        return consumePlusSignToken();
    case '-':
        return consumeHyphenMinusToken();
    case '.':
        return consumeFullStopToken();
    case '<':
        return consumeLessThanSignToken();
    case '@':
        return consumeCommercialAtToken();
    case '\\':
        return consumeReverseSolidusToken();
    case '\"':
        return consumeStringToken();
    case '\'':
        return consumeStringToken();
    default:
        m_input.advance();
    }

    switch(cc) {
    case '(':
        return CssToken(CssToken::Type::LeftParenthesis);
    case ')':
        return CssToken(CssToken::Type::RightParenthesis);
    case '[':
        return CssToken(CssToken::Type::LeftSquareBracket);
    case ']':
        return CssToken(CssToken::Type::RightSquareBracket);
    case '{':
        return CssToken(CssToken::Type::LeftCurlyBracket);
    case '}':
        return CssToken(CssToken::Type::RightCurlyBracket);
    case ',':
        return CssToken(CssToken::Type::Comma);
    case ':':
        return CssToken(CssToken::Type::Colon);
    case ';':
        return CssToken(CssToken::Type::Semicolon);
    default:
        return CssToken(CssToken::Type::Delim, cc);
    }
}

} // namespace plutobook
