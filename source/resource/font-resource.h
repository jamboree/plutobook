#pragma once

#include "resource.h"
#include "global-string.h"
#include "graphics-manager.h"

#include <vector>
#include <boost/unordered/unordered_flat_map.hpp>
#include <mutex>

namespace plutobook {
    class Document;

    class FontResource final : public Resource {
    public:
        static constexpr ClassKind classKind = ClassKind::Font;

        static RefPtr<FontResource> create(Document* document, const Url& url);
        static bool supportsFormat(const std::string_view& format);
        FaceHandle face() const { return m_face; }

        ~FontResource() final;

    private:
        explicit FontResource(FaceHandle face)
            : Resource(classKind), m_face(face) {}

        FaceHandle m_face;
    };

    using FontSelectionValue = float;

    constexpr FontSelectionValue kNormalFontWeight = 400.f;
    constexpr FontSelectionValue kBoldFontWeight = 700.f;
    constexpr FontSelectionValue kLightFontWeight = 200.f;
    constexpr FontSelectionValue kMinFontWeight = 1.f;
    constexpr FontSelectionValue kMaxFontWeight = 1000.f;

    constexpr FontSelectionValue kNormalFontWidth = 100.f;
    constexpr FontSelectionValue kUltraCondensedFontWidth = 50.f;
    constexpr FontSelectionValue kExtraCondensedFontWidth = 62.5f;
    constexpr FontSelectionValue kCondensedFontWidth = 75.f;
    constexpr FontSelectionValue kSemiCondensedFontWidth = 85.5f;
    constexpr FontSelectionValue kSemiExpandedFontWidth = 112.5f;
    constexpr FontSelectionValue kExpandedFontWidth = 125.f;
    constexpr FontSelectionValue kExtraExpandedFontWidth = 150.f;
    constexpr FontSelectionValue kUltraExpandedFontWidth = 200.f;

    constexpr FontSelectionValue kNormalFontSlope = 0.f;
    constexpr FontSelectionValue kItalicFontSlope = 14.f;
    constexpr FontSelectionValue kObliqueFontSlope = 20.f;
    constexpr FontSelectionValue kMinFontSlope = -90.f;
    constexpr FontSelectionValue kMaxFontSlope = 90.f;

    struct FontSelectionRequest {
        constexpr FontSelectionRequest() = default;
        constexpr FontSelectionRequest(FontSelectionValue weight,
                                       FontSelectionValue width,
                                       FontSelectionValue slope)
            : weight(weight), width(width), slope(slope) {}

        FontSelectionValue weight = kNormalFontWeight;
        FontSelectionValue width = kNormalFontWidth;
        FontSelectionValue slope = kNormalFontSlope;

        bool operator==(const FontSelectionRequest& other) const = default;
        auto operator<=>(const FontSelectionRequest& other) const = default;

        friend std::size_t hash_value(const FontSelectionRequest& self) {
            std::size_t seed = 0;
            boost::hash_combine(seed, self.weight);
            boost::hash_combine(seed, self.width);
            boost::hash_combine(seed, self.slope);
            return seed;
        }
    };

    struct FontSelectionRange {
        constexpr FontSelectionRange() = default;
        constexpr explicit FontSelectionRange(FontSelectionValue value)
            : minimum(value), maximum(value) {}

        constexpr FontSelectionRange(FontSelectionValue minimum,
                                     FontSelectionValue maximum)
            : minimum(minimum), maximum(maximum) {}

        constexpr explicit operator bool() const { return maximum >= minimum; }

        FontSelectionValue minimum = FontSelectionValue(0);
        FontSelectionValue maximum = FontSelectionValue(0);

        bool operator==(const FontSelectionRange& other) const = default;
        auto operator<=>(const FontSelectionRange& other) const = default;

        friend std::size_t hash_value(const FontSelectionRange& self) {
            std::size_t seed = 0;
            boost::hash_combine(seed, self.minimum);
            boost::hash_combine(seed, self.maximum);
            return seed;
        }
    };

    constexpr FontSelectionRange kInvalidFontSelectionRange =
        FontSelectionRange(1, 0);

    struct FontSelectionDescription {
        constexpr FontSelectionDescription() = default;
        constexpr explicit FontSelectionDescription(
            const FontSelectionRequest& request)
            : weight(request.weight), width(request.width),
              slope(request.slope) {}

        constexpr FontSelectionDescription(FontSelectionRange weight,
                                           FontSelectionRange width,
                                           FontSelectionRange slope)
            : weight(weight), width(width), slope(slope) {}

        FontSelectionRange weight = kInvalidFontSelectionRange;
        FontSelectionRange width = kInvalidFontSelectionRange;
        FontSelectionRange slope = kInvalidFontSelectionRange;

        bool operator==(const FontSelectionDescription& other) const = default;
        auto operator<=>(const FontSelectionDescription& other) const = default;

        friend std::size_t hash_value(const FontSelectionDescription& self) {
            std::size_t seed = 0;
            boost::hash_combine(seed, self.weight);
            boost::hash_combine(seed, self.width);
            boost::hash_combine(seed, self.slope);
            return seed;
        }
    };

    class FontSelectionAlgorithm {
    public:
        explicit FontSelectionAlgorithm(const FontSelectionRequest& request);

        void addCandidate(const FontSelectionDescription& description);

        FontSelectionValue widthDistance(const FontSelectionRange& width) const;
        FontSelectionValue slopeDistance(const FontSelectionRange& slope) const;
        FontSelectionValue
        weightDistance(const FontSelectionRange& weight) const;

        bool isCandidateBetter(
            const FontSelectionDescription& currentCandidate,
            const FontSelectionDescription& previousCandidate) const;

    private:
        FontSelectionRequest m_request;
        FontSelectionRange m_weight;
        FontSelectionRange m_width;
        FontSelectionRange m_slope;
    };

    constexpr FontTag makeFontTag(const std::string_view& tag) {
        assert(tag.length() == 4);
        return FontTag((tag[0] << 24) | (tag[1] << 16) | (tag[2] << 8) |
                       (tag[3]));
    }

    using FontFeature = std::pair<FontTag, int>;
    using FontVariation = std::pair<FontTag, float>;

    using FontFeatureList = std::vector<FontFeature>;
    using FontVariationList = std::vector<FontVariation>;

    constexpr FontSelectionValue kMediumFontSize = 16.f;

    struct FontDataDescription {
        FontSelectionValue size = kMediumFontSize;
        FontSelectionRequest request;
        FontVariationList variations;

        bool operator==(const FontDataDescription& other) const = default;
        auto operator<=>(const FontDataDescription& other) const = default;

        friend std::size_t hash_value(const FontDataDescription& self) {
            std::size_t seed = 0;
            boost::hash_combine(seed, self.size);
            boost::hash_combine(seed, self.request);
            boost::hash_combine(seed, self.variations);
            return seed;
        }
    };

    using FontFamilyList = std::vector<GlobalString>;

    struct FontDescription {
        FontFamilyList families;
        FontDataDescription data;

        bool operator==(const FontDescription& other) const = default;
        auto operator<=>(const FontDescription& other) const = default;

        friend std::size_t hash_value(const FontDescription& self) {
            std::size_t seed = 0;
            boost::hash_combine(seed, self.families);
            boost::hash_combine(seed, self.data);
            return seed;
        }
    };

    using UnicodeRange = std::pair<uint32_t, uint32_t>;
    using UnicodeRangeList = std::vector<UnicodeRange>;

    class FontData;
    class FontDataCache;

    class FontFace : public RefCounted<FontFace> {
    public:
        virtual ~FontFace() = default;
        virtual RefPtr<FontData>
        getFontData(FontDataCache* fontDataCache,
                    const FontDataDescription& description) = 0;

        const FontFeatureList& features() const { return m_features; }
        const FontVariationList& variations() const { return m_variations; }
        const UnicodeRangeList& ranges() const { return m_ranges; }

    protected:
        FontFace(FontFeatureList features, FontVariationList variations,
                 UnicodeRangeList ranges)
            : m_features(std::move(features)),
              m_variations(std::move(variations)), m_ranges(std::move(ranges)) {
        }

        FontFeatureList m_features;
        FontVariationList m_variations;
        UnicodeRangeList m_ranges;
    };

    class LocalFontFace final : public FontFace {
    public:
        static RefPtr<LocalFontFace> create(GlobalString family,
                                            FontFeatureList features,
                                            FontVariationList variations,
                                            UnicodeRangeList ranges);

        RefPtr<FontData>
        getFontData(FontDataCache* fontDataCache,
                    const FontDataDescription& description) final;

    private:
        LocalFontFace(GlobalString family, FontFeatureList features,
                      FontVariationList variations, UnicodeRangeList ranges)
            : FontFace(std::move(features), std::move(variations),
                       std::move(ranges)),
              m_family(family) {}

        GlobalString m_family;
    };

    inline RefPtr<LocalFontFace>
    LocalFontFace::create(GlobalString family, FontFeatureList features,
                          FontVariationList variations,
                          UnicodeRangeList ranges) {
        return adoptPtr(new LocalFontFace(family, std::move(features),
                                          std::move(variations),
                                          std::move(ranges)));
    }

    class RemoteFontFace final : public FontFace {
    public:
        static RefPtr<RemoteFontFace> create(FontFeatureList features,
                                             FontVariationList variations,
                                             UnicodeRangeList ranges,
                                             RefPtr<FontResource> resource);

        RefPtr<FontData>
        getFontData(FontDataCache* fontDataCache,
                    const FontDataDescription& description) final;

    private:
        RemoteFontFace(FontFeatureList features, FontVariationList variations,
                       UnicodeRangeList ranges, RefPtr<FontResource> resource)
            : FontFace(std::move(features), std::move(variations),
                       std::move(ranges)),
              m_resource(std::move(resource)) {}

        RefPtr<FontResource> m_resource;
    };

    inline RefPtr<RemoteFontFace> RemoteFontFace::create(
        FontFeatureList features, FontVariationList variations,
        UnicodeRangeList ranges, RefPtr<FontResource> resource) {
        return adoptPtr(
            new RemoteFontFace(std::move(features), std::move(variations),
                               std::move(ranges), std::move(resource)));
    }

    class SegmentedFontData;

    class SegmentedFontFace : public RefCounted<SegmentedFontFace> {
    public:
        static RefPtr<SegmentedFontFace>
        create(const FontSelectionDescription& description);

        const FontSelectionDescription& description() const {
            return m_description;
        }
        RefPtr<FontData> getFontData(FontDataCache* fontDataCache,
                                     const FontDataDescription& description);
        void add(RefPtr<FontFace> face) { m_faces.push_back(std::move(face)); }

    private:
        SegmentedFontFace(const FontSelectionDescription& description)
            : m_description(description) {}
        FontSelectionDescription m_description;
        std::vector<RefPtr<FontFace>> m_faces;
        boost::unordered_flat_map<FontDataDescription,
                                  RefPtr<SegmentedFontData>>
            m_table;
    };

    inline RefPtr<SegmentedFontFace>
    SegmentedFontFace::create(const FontSelectionDescription& description) {
        return adoptPtr(new SegmentedFontFace(description));
    }

    class SimpleFontData;

    class FontData : public RefCounted<FontData> {
    public:
        virtual ~FontData() = default;
        virtual const SimpleFontData* getFontData(uint32_t codepoint,
                                                  bool preferColor) const = 0;

    protected:
        FontData() = default;
    };

    using FontDataList = std::vector<RefPtr<FontData>>;

    struct FontDataInfo {
        float ascent;
        float descent;
        float lineGap;
        float xHeight;
        float zeroWidth;
        float spaceWidth;
        uint16_t zeroGlyph;
        uint16_t spaceGlyph;
        bool hasColor;
    };

    class SimpleFontData final : public FontData {
    public:
        static RefPtr<SimpleFontData> create(FontHandle font,
                                             const FontDataInfo& info,
                                             const FontFeatureList& features) {
            return adoptPtr(new SimpleFontData(font, info, features));
        }

        FontHandle font() const { return m_hbFont; }
        const FontDataInfo& info() const { return m_info; }
        const FontFeatureList& features() const { return m_features; }

        const SimpleFontData* getFontData(uint32_t codepoint,
                                          bool preferColor) const final;

        float ascent() const { return m_info.ascent; }
        float descent() const { return m_info.descent; }
        float height() const { return m_info.ascent + m_info.descent; }
        float xHeight() const { return m_info.xHeight; }
        float lineGap() const { return m_info.lineGap; }
        float lineSpacing() const {
            return m_info.ascent + m_info.descent + m_info.lineGap;
        }
        float zeroWidth() const { return m_info.zeroWidth; }
        float spaceWidth() const { return m_info.spaceWidth; }
        uint16_t zeroGlyph() const { return m_info.zeroGlyph; }
        uint16_t spaceGlyph() const { return m_info.spaceGlyph; }

        ~SimpleFontData() final;

    private:
        SimpleFontData(FontHandle font, const FontDataInfo& info,
                       const FontFeatureList& features)
            : m_hbFont(font), m_info(info), m_features(features) {}

        FontHandle m_hbFont;
        FontDataInfo m_info;
        FontFeatureList m_features;
    };

    class FontDataRange {
    public:
        FontDataRange(uint32_t from, uint32_t to, RefPtr<FontData> data)
            : m_from(from), m_to(to), m_data(std::move(data)) {}

        const SimpleFontData* getFontData(uint32_t codepoint,
                                          bool preferColor) const;

    private:
        uint32_t m_from;
        uint32_t m_to;
        RefPtr<FontData> m_data;
    };

    using FontDataRangeList = std::vector<FontDataRange>;

    class SegmentedFontData final : public FontData {
    public:
        static RefPtr<SegmentedFontData> create(FontDataRangeList fonts);

        const SimpleFontData* getFontData(uint32_t codepoint,
                                          bool preferColor) const final;

    private:
        SegmentedFontData(FontDataRangeList fonts)
            : m_fonts(std::move(fonts)) {}
        FontDataRangeList m_fonts;
    };

    inline RefPtr<SegmentedFontData>
    SegmentedFontData::create(FontDataRangeList fonts) {
        return adoptPtr(new SegmentedFontData(std::move(fonts)));
    }

    class FontDataCache {
    public:
        RefPtr<SimpleFontData>
        getFontData(GlobalString family,
                    const FontDataDescription& description);
        RefPtr<SimpleFontData>
        getFontData(uint32_t codepoint, bool preferColor,
                    const FontDataDescription& description);

        bool isFamilyAvailable(GlobalString family);

        FontDataCache();
        ~FontDataCache();

    private:
        using FontDataKey = std::pair<GlobalString, FontDataDescription>;

        FcConfig* m_config;
        std::mutex m_mutex;
        boost::unordered_flat_map<FontDataKey, RefPtr<SimpleFontData>> m_table;
    };

    class Font : public RefCounted<Font> {
    public:
        static RefPtr<Font> create(Document* document,
                                   const FontDescription& description);

        Document* document() const { return m_document; }
        const FontDescription& description() const { return m_description; }
        const FontDataList& fonts() const { return m_fonts; }
        const SimpleFontData* primaryFont() const { return m_primaryFont; }

        float size() const { return m_description.data.size; }
        float weight() const { return m_description.data.request.weight; }
        float stretch() const { return m_description.data.request.width; }
        float slope() const { return m_description.data.request.slope; }

        const FontFamilyList& family() const { return m_description.families; }
        const FontVariationList& variationSettings() const {
            return m_description.data.variations;
        }

        const SimpleFontData* getFontData(uint32_t codepoint, bool preferColor);

    private:
        Font(Document* document, const FontDescription& description);
        Document* m_document;
        FontDescription m_description;
        FontDataList m_fonts;
        const SimpleFontData* m_primaryFont{nullptr};
        const SimpleFontData* m_emojiFont{nullptr};
    };
} // namespace plutobook