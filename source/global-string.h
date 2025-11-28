#pragma once

#include "heap-string.h"

namespace plutobook {
    enum class GlobalStringId : unsigned {
        emptyGlo = 0,
#define GLOBAL_STRING(id, str) id,
#include "global-string.inc"
        frameTag = frameGlo,
        frameAttr = frameGlo,
        spanTag = spanGlo,
        spanAttr = spanGlo,
        styleTag = styleGlo,
        styleAttr = styleGlo,
        textTag = textGlo,
        textAttr = textGlo,
    };

    class GlobalString {
    public:
        GlobalString() = default;
        explicit GlobalString(const std::string_view& value);
        constexpr GlobalString(GlobalStringId id) : m_index(unsigned(id)) {}

        const HeapString& value() const;

        constexpr bool isEmpty() const { return m_index == 0; }

        constexpr GlobalStringId asId() const {
            return GlobalStringId(m_index);
        }

        GlobalString foldCase() const;

        operator const std::string_view&() const { return value(); }
        operator const HeapString&() const { return value(); }

        bool operator==(const GlobalString&) const = default;
        auto operator<=>(const GlobalString&) const = default;

        friend std::size_t hash_value(GlobalString key) {
            return boost::hash<unsigned>{}(key.m_index);
        }

    private:
        unsigned m_index = 0;
    };

    inline std::ostream& operator<<(std::ostream& o, GlobalString in) {
        return o << std::string_view(in);
    }

    inline GlobalString operator""_glo(const char* data, size_t length) {
        return GlobalString({data, length});
    }

    constexpr GlobalString emptyGlo(GlobalStringId::emptyGlo);
#define GLOBAL_STRING(id, str) constexpr GlobalString id(GlobalStringId::id);
#include "global-string.inc"
    constexpr GlobalString frameTag(GlobalStringId::frameTag);
    constexpr GlobalString frameAttr(GlobalStringId::frameAttr);
    constexpr GlobalString spanTag(GlobalStringId::spanTag);
    constexpr GlobalString spanAttr(GlobalStringId::spanAttr);
    constexpr GlobalString styleTag(GlobalStringId::styleTag);
    constexpr GlobalString styleAttr(GlobalStringId::styleAttr);
    constexpr GlobalString textTag(GlobalStringId::textTag);
    constexpr GlobalString textAttr(GlobalStringId::textAttr);
} // namespace plutobook