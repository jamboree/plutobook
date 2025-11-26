#pragma once

#include "heap-string.h"

namespace plutobook {
    enum class GlobalStringId {
        emptyGlo = 0,
#define GLOBAL_STRING(id, str) id,
#include "global-string.inc"
    };

    class GlobalString {
    public:
        GlobalString() = default;
        explicit GlobalString(const std::string_view& value);
        constexpr GlobalString(GlobalStringId id) : m_index(unsigned(id)) {}

        const HeapString& value() const;

        bool isEmpty() const { return m_index == 0; }

        constexpr unsigned index() const { return m_index; }

        GlobalString foldCase() const;

        operator const std::string_view&() const { return value(); }
        operator const HeapString&() const { return value(); }

        bool operator==(const GlobalString& o) const = default;

        friend std::size_t hash_value(GlobalString key) {
            return boost::hash<unsigned>{}(key.m_index);
        }

    private:
        friend class GlobalStringTable;

        constexpr explicit GlobalString(unsigned index) : m_index(index) {}

        unsigned m_index = 0;
    };

    inline std::ostream& operator<<(std::ostream& o, const GlobalString& in) {
        return o << std::string_view(in);
    }

    inline GlobalString operator""_glo(const char* data, size_t length) {
        return GlobalString({data, length});
    }

    constexpr GlobalString emptyGlo(GlobalStringId::emptyGlo);
#define GLOBAL_STRING(id, str) constexpr GlobalString id(GlobalStringId::id);
#include "global-string.inc"
} // namespace plutobook