#pragma once

#include "pointer.h"

#include <memory_resource>
#include <vector>
#include <memory>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_node_map.hpp>

namespace plutobook {
    class CssRule;
    class CssRuleData;
    class CssStyleRule;
    class CssImportRule;
    class CssPageRule;
    class CssPageRuleData;
    class CssFontFaceRule;
    class CssCounterStyleRule;
    class CssCounterStyleMap;
    class CssCounterStyle;
    class CssMediaRule;

    enum class CssStyleOrigin : uint8_t;

    using CssRuleList = std::vector<RefPtr<CssRule>>;
    using CssRuleDataList = std::vector<CssRuleData>;
    using CssPageRuleDataList = std::vector<CssPageRuleData>;

    template<typename T>
    class CssRuleDataMap {
    public:
        CssRuleDataMap() = default;

        bool add(const T& name, CssRuleData&& rule);
        const CssRuleDataList* get(const T& name) const;

    private:
        boost::unordered_node_map<T, CssRuleDataList> m_table;
    };

    template<typename T>
    bool CssRuleDataMap<T>::add(const T& name, CssRuleData&& rule) {
        auto [it, inserted] = m_table.try_emplace(name);
        it->second.push_back(std::move(rule));
        return inserted;
    }

    template<typename T>
    const CssRuleDataList* CssRuleDataMap<T>::get(const T& name) const {
        auto it = m_table.find(name);
        if (it == m_table.end())
            return nullptr;
        return &it->second;
    }

    class HeapString;
    class GlobalString;

    class FontFace;
    class FontData;
    class SegmentedFontFace;

    struct FontDataDescription;
    struct FontSelectionDescription;

    class CssFontFaceCache {
    public:
        explicit CssFontFaceCache();

        RefPtr<FontData> get(GlobalString family,
                             const FontDataDescription& description) const;
        void add(GlobalString family,
                 const FontSelectionDescription& description,
                 RefPtr<FontFace> face);

    private:
        boost::unordered_flat_map<
            GlobalString, boost::unordered_flat_map<FontSelectionDescription,
                                                    RefPtr<SegmentedFontFace>>>
            m_table;
    };

    class BoxStyle;
    class Document;
    class Element;
    class Url;

    enum class PseudoType : uint8_t;
    enum class PageMarginType : uint8_t;

    class CssStyleSheet {
    public:
        explicit CssStyleSheet(Document* document);

        RefPtr<BoxStyle> styleForElement(Element* element,
                                         const BoxStyle* parentStyle) const;
        RefPtr<BoxStyle>
        pseudoStyleForElement(Element* element, PseudoType pseudoType,
                              const BoxStyle* parentStyle) const;
        RefPtr<BoxStyle> styleForPage(GlobalString pageName,
                                      uint32_t pageIndex,
                                      PseudoType pseudoType) const;
        RefPtr<BoxStyle> styleForPageMargin(GlobalString pageName,
                                            uint32_t pageIndex,
                                            PageMarginType marginType,
                                            const BoxStyle* pageStyle) const;
        RefPtr<FontData>
        getFontData(GlobalString family,
                    const FontDataDescription& description) const;

        const CssCounterStyle& getCounterStyle(GlobalString name);
        std::string getCounterText(int value, GlobalString listType);
        std::string getMarkerText(int value, GlobalString listType);

        void parseStyle(const std::string_view& content, CssStyleOrigin origin,
                        Url baseUrl);

    private:
        void addRuleList(const CssRuleList& rules);
        void addStyleRule(const RefPtr<CssStyleRule>& rule);
        void addImportRule(const RefPtr<CssImportRule>& rule);
        void addPageRule(const RefPtr<CssPageRule>& rule);
        void addFontFaceRule(const RefPtr<CssFontFaceRule>& rule);
        void addCounterStyleRule(const RefPtr<CssCounterStyleRule>& rule);
        void addMediaRule(const RefPtr<CssMediaRule>& rule);

        Document* m_document;
        uint32_t m_position{0};
        uint32_t m_importDepth{0};

        CssRuleDataMap<HeapString> m_idRules;
        CssRuleDataMap<HeapString> m_classRules;
        CssRuleDataMap<GlobalString> m_tagRules;
        CssRuleDataMap<GlobalString> m_attributeRules;
        CssRuleDataMap<PseudoType> m_pseudoRules;

        CssRuleDataList m_universalRules;
        CssPageRuleDataList m_pageRules;
        CssRuleList m_counterStyleRules;
        CssFontFaceCache m_fontFaceCache;
        std::unique_ptr<CssCounterStyleMap> m_counterStyleMap;
    };
} // namespace plutobook