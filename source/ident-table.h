#pragma once

#include <frozen/unordered_map.h>
#include <frozen/unordered_set.h>
#include <frozen/string.h>

namespace plutobook {
    template<class T, unsigned N>
    using IdentTable = frozen::unordered_map<frozen::string, T, N>;

    template<class T, unsigned N>
    constexpr IdentTable<T, N>
    makeIdentTable(const std::pair<frozen::string, T> (&items)[N]) {
        return IdentTable<T, N>{items};
    }

    template<unsigned N>
    using IdentSet = frozen::unordered_set<frozen::string, N>;

    template<unsigned N>
    consteval IdentSet<N> makeIdentSet(const frozen::string (&items)[N]) {
        return IdentSet<N>{items};
    }
} // namespace plutobook