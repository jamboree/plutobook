#include "string-utils.h"
#include <algorithm>

namespace plutobook {

bool iequals(std::string_view a, std::string_view b) {
    if (a.size() != b.size())
        return false;
    const auto aEnd = a.end();
    auto [aIt, bIt] = std::mismatch(a.begin(), aEnd, b.begin(), b.end());
    for (; aIt != aEnd; ++aIt, ++bIt) {
        if (!iequals(*aIt, *bIt)) {
            return false;
        }
    }
    return true;
}

bool matchLower(std::string_view str, std::string_view lower) {
    if (str.size() != lower.size())
        return false;
    const auto aEnd = str.end();
    auto [aIt, bIt] = std::mismatch(str.begin(), aEnd, lower.begin(), lower.end());
    for (; aIt != aEnd; ++aIt, ++bIt) {
        const auto c = *bIt;
        assert(!isUpper(c));
        if (c != toLower(*aIt)) {
            return false;
        }
    }
    return true;
}

bool contains(std::string_view haystack, std::string_view needle,
    bool caseSensitive) {
    if (needle.empty() || needle.length() > haystack.length())
        return false;
    const auto limit = haystack.length() - needle.length();
    for (size_t i = 0; i <= limit; ++i) {
        if (equals(haystack.substr(i, needle.length()), needle,
            caseSensitive)) {
            return true;
        }
    }

    return false;
}

bool includes(std::string_view haystack, std::string_view needle,
    bool caseSensitive) {
    if (needle.empty() || needle.length() > haystack.length())
        return false;
    size_t begin = 0;
    while (true) {
        while (begin < haystack.length() && isSpace(haystack[begin]))
            ++begin;
        if (begin >= haystack.length())
            break;
        size_t end = begin + 1;
        while (end < haystack.length() && !isSpace(haystack[end]))
            ++end;
        if (equals(haystack.substr(begin, end - begin), needle,
            caseSensitive))
            return true;
        begin = end + 1;
    }

    return false;
}

std::string_view toLower(std::string_view input, char buffer[]) {
    const auto up =
        std::ranges::find_if(input, [](char c) { return isUpper(c); });
    if (up == input.end())
        return input;
    size_t i = up - input.begin();
    std::memcpy(buffer, input.data(), i);
    for (; i != input.length(); ++i) {
        buffer[i] = toLower(input[i]);
    }
    return std::string_view(buffer, input.length());
}

void appendCodepoint(std::string& output, uint32_t cp) {
    char c[5] = {0, 0, 0, 0, 0};
    if (cp < 0x80) {
        c[1] = 0;
        c[0] = cp;
    }
    else if (cp < 0x800) {
        c[2] = 0;
        c[1] = (cp & 0x3F) | 0x80;
        cp >>= 6;
        c[0] = cp | 0xC0;
    }
    else if (cp < 0x10000) {
        c[3] = 0;
        c[2] = (cp & 0x3F) | 0x80;
        cp >>= 6;
        c[1] = (cp & 0x3F) | 0x80;
        cp >>= 6;
        c[0] = cp | 0xE0;
    }
    else if (cp < 0x110000) {
        c[4] = 0;
        c[3] = (cp & 0x3F) | 0x80;
        cp >>= 6;
        c[2] = (cp & 0x3F) | 0x80;
        cp >>= 6;
        c[1] = (cp & 0x3F) | 0x80;
        cp >>= 6;
        c[0] = cp | 0xF0;
    }

    output.append(c);
}

}