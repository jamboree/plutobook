#include "global-string.h"
#include "string-utils.h"

#include <deque>
#include <boost/unordered/unordered_flat_set.hpp>

namespace plutobook {

struct GlobalStringTable {
    struct Hash : boost::hash<std::string_view> {
        using is_transparent = void;
        using hash::operator();

        Hash(const GlobalStringTable& map) noexcept : m_map(map) {}

        std::size_t operator()(unsigned id) const {
            return operator()(m_map.m_strings[id]);
        }

        const GlobalStringTable& m_map;
    };

    struct Equal : std::equal_to<std::string_view> {
        using is_transparent = void;
        using equal_to::operator();

        Equal(const GlobalStringTable& map) noexcept : m_map(map) {}

        bool operator()(unsigned a, unsigned b) const { return a == b; }

        bool operator()(std::string_view a, unsigned b) const {
            return a == m_map.m_strings[b];
        }

        const GlobalStringTable& m_map;
    };

    template<unsigned N>
    GlobalStringTable(const std::string_view(&list)[N]) {
        // Reserve index 0 for empty string
        m_foldSet.reserve(N + 1);
        m_strings.emplace_back();
        m_foldSet.emplace(0);
        for (unsigned i = 0; const auto& s : list) {
            assert(!m_foldSet.contains(s));
            m_strings.emplace_back(createString(s));
            m_foldSet.emplace(++i);
        }
    }

    unsigned add(const std::string_view& value) {
        const auto it = m_foldSet.find(value);
        if (it != m_foldSet.end()) {
            return *it;
        }
        const auto id = unsigned(m_strings.size());
        m_strings.emplace_back(createString(value));
        m_foldSet.emplace(id);
        return id;
    }

    std::deque<HeapString> m_strings;
    boost::unordered_flat_set<unsigned, Hash, Equal> m_foldSet{0, *this,
                                                             *this};
};

static GlobalStringTable* globalStringTable()
{
    static GlobalStringTable table({
#define GLOBAL_STRING(id, str) str,
#include "global-string.inc"
    });
    return &table;
}

GlobalString GlobalString::get(const std::string_view& value)
{
    return GlobalString(globalStringTable()->add(value));
}

const HeapString& GlobalString::value() const
{
    return globalStringTable()->m_strings[m_index];
}

GlobalString GlobalString::foldCase() const
{
    if(isEmpty())
        return emptyGlo;
    const auto& entry = value();
    auto size = entry.size();
    auto data = entry.data();

    size_t index = 0;
    while(index < size && !isUpper(data[index]))
        ++index;
    if(index == size) {
        return *this;
    }

    char smallBuf[128];
    std::unique_ptr<char[]> largeBuf;
    char* buffer;
    if (size > sizeof(smallBuf)) {
        largeBuf.reset(new char[size]);
        buffer = largeBuf.get();
    } else {
        buffer = smallBuf;
    }

    std::memcpy(buffer, data, index);
    for (size_t i = index; i != size; ++i) {
        buffer[i] = toLower(data[i]);
    }

    return GlobalString::get({buffer, size});
}

} // namespace plutobook
