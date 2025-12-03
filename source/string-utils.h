#pragma once

#include <string>
#include <cstdint>
#include <cassert>
#include <charconv>

namespace plutobook {
    constexpr bool isSpace(uint8_t cc) {
        return (cc == ' ' || cc == '\n' || cc == '\t' || cc == '\r' ||
                cc == '\f');
    }
    constexpr bool isDigit(uint8_t cc) { return (cc >= '0' && cc <= '9'); }
    constexpr bool isUpper(uint8_t cc) { return (cc >= 'A' && cc <= 'Z'); }
    constexpr bool isLower(uint8_t cc) { return (cc >= 'a' && cc <= 'z'); }
    constexpr bool isAlpha(uint8_t cc) { return isUpper(cc) || isLower(cc); }
    constexpr bool isAlnum(uint8_t cc) { return isDigit(cc) || isAlpha(cc); }

    constexpr bool isHexUpper(uint8_t cc) { return (cc >= 'A' && cc <= 'F'); }
    constexpr bool isHexLower(uint8_t cc) { return (cc >= 'a' && cc <= 'f'); }
    constexpr bool isHexAlpha(uint8_t cc) {
        return isHexUpper(cc) || isHexLower(cc);
    }
    constexpr bool isHexDigit(uint8_t cc) {
        return isDigit(cc) || isHexAlpha(cc);
    }

    constexpr int toHexDigit(uint8_t cc) {
        if (isDigit(cc))
            return cc - '0';
        if (isHexUpper(cc))
            return 10 + cc - 'A';
        if (isHexLower(cc))
            return 10 + cc - 'a';
        return 0;
    }

    constexpr int toHexByte(uint8_t a, uint8_t b) {
        return (toHexDigit(a) << 4) | toHexDigit(b);
    }

    constexpr uint8_t kAsciiUpperToLower = 'a' - 'A';

    constexpr char toLower(uint8_t cc) {
        return isUpper(cc) ? cc + kAsciiUpperToLower : cc;
    }

    constexpr bool iequals(uint8_t a, uint8_t b) {
        return toLower(a) == toLower(b);
    }

    bool iequals(std::string_view a, std::string_view b);

    constexpr bool equals(uint8_t a, uint8_t b, bool caseSensitive) {
        return caseSensitive ? a == b : iequals(a, b);
    }

    inline bool equals(std::string_view a, std::string_view b,
                       bool caseSensitive) {
        return caseSensitive ? a == b : iequals(a, b);
    }

    bool matchLower(std::string_view str, std::string_view lower);

    bool contains(std::string_view haystack, std::string_view needle,
                  bool caseSensitive);

    bool includes(std::string_view haystack, std::string_view needle,
                  bool caseSensitive);

    inline bool startswith(std::string_view input, std::string_view prefix,
                           bool caseSensitive) {
        if (prefix.empty() || prefix.length() > input.length())
            return false;
        return equals(input.substr(0, prefix.length()), prefix, caseSensitive);
    }

    inline bool endswith(std::string_view input, std::string_view suffix,
                         bool caseSensitive) {
        if (suffix.empty() || suffix.length() > input.length())
            return false;
        return equals(
            input.substr(input.length() - suffix.length(), suffix.length()),
            suffix, caseSensitive);
    }

    inline bool dashequals(std::string_view input, std::string_view prefix,
                           bool caseSensitive) {
        if (startswith(input, prefix, caseSensitive))
            return (input.length() == prefix.length() ||
                    input.at(prefix.length()) == '-');
        return false;
    }

    constexpr void stripLeadingSpaces(std::string_view& input) {
        while (!input.empty() && isSpace(input.front())) {
            input.remove_prefix(1);
        }
    }

    constexpr void stripTrailingSpaces(std::string_view& input) {
        while (!input.empty() && isSpace(input.back())) {
            input.remove_suffix(1);
        }
    }

    constexpr void stripLeadingAndTrailingSpaces(std::string_view& input) {
        stripLeadingSpaces(input);
        stripTrailingSpaces(input);
    }

    inline std::string toString(int value) {
        char buffer[16];
        const auto [p, ec] = std::to_chars(buffer, std::end(buffer), value);
        assert(ec == std::errc());
        return std::string(buffer, p);
    }

    inline std::string toString(float value) {
        char buffer[32];
        const auto [p, ec] = std::to_chars(buffer, std::end(buffer), value);
        assert(ec == std::errc());
        return std::string(buffer, p);
    }

    std::string_view toLower(std::string_view input, char buffer[]);

    void appendCodepoint(std::string& output, uint32_t cp);
} // namespace plutobook