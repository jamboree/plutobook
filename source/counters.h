#pragma once

#include <vector>
#include <cstdint>
#include <boost/unordered/unordered_flat_set.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

namespace plutobook {
    class Box;
    class Document;
    class GlobalString;
    class HeapString;

    class Counters {
    public:
        Counters(Document* document, uint32_t pageCount);

        void push();
        void pop();

        void reset(GlobalString name, int value);
        void increment(GlobalString name, int value);
        void set(GlobalString name, int value);

        void increaseQuoteDepth() { ++m_quoteDepth; }
        void decreaseQuoteDepth() { --m_quoteDepth; }

        uint32_t pageCount() const { return m_pageCount; }
        uint32_t quoteDepth() const { return m_quoteDepth; }

        void update(const Box* box);

        HeapString counterText(GlobalString name,
                               GlobalString listStyle,
                               const HeapString& separator) const;
        HeapString markerText(GlobalString listStyle) const;

    private:
        Document* m_document;
        std::vector<boost::unordered_flat_set<GlobalString>> m_scopes;
        boost::unordered_flat_map<GlobalString, std::vector<int>> m_values;
        uint32_t m_pageCount;
        uint32_t m_quoteDepth{0};
    };
} // namespace plutobook