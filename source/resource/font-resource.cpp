#include "font-resource.h"
#include "document.h"
#include "string-utils.h"
#include "ident-table.h"

#include "plutobook.hpp"

#include <fontconfig/fontconfig.h>
#include <harfbuzz/hb.h>

#include <numbers>
#include <cmath>

// For CairoGraphicsManager
#include <fontconfig/fcfreetype.h>
#include <cairo/cairo-ft.h>
#include <harfbuzz/hb-ft.h>

namespace plutobook {

hb_face_t* createHBFaceFromResource(ResourceData resource)
{
    auto blob = hb_blob_create_or_fail(
        resource.content(), resource.contentLength(),
        HB_MEMORY_MODE_READONLY, resource.get(), [](void* p) {
            plutobook_resource_data_destroy(
                static_cast<plutobook_resource_data_t*>(p));
        });
    if (!blob) {
        return nullptr;
    }
    resource.release();
    auto face = hb_face_create_or_fail(blob, 0);
    hb_blob_destroy(blob);
    return face;
}

RefPtr<FontResource> FontResource::create(Document* document, const Url& url)
{
    auto resource = ResourceLoader::loadUrl(url, document->customResourceFetcher());
    if(resource.isNull())
        return nullptr;
    auto face = graphicsManager().createFaceFromResource(std::move(resource));
    if(face == FaceHandle::Invalid) {
        plutobook_set_error_message("Unable to load font '%s': %s", url.value().data(), plutobook_get_error_message());
        return nullptr;
    }

    return adoptPtr(new FontResource(face));
}

bool FontResource::supportsFormat(const std::string_view& format)
{
    char buffer[32];
    return format.length() <= sizeof(buffer) &&
           makeIdentSet({
                            "opentype",
                            "opentype-variations",
                            "truetype",
                            "truetype-variations",
                            "woff",
                            "woff-variations",
#ifdef FT_CONFIG_OPTION_USE_BROTLI
                            "woff2",
                            "woff2-variations",
#endif
                        })
               .contains(toLower(format, buffer));
}

FontResource::~FontResource()
{
    graphicsManager().destroyFace(m_face);
}

FontSelectionAlgorithm::FontSelectionAlgorithm(const FontSelectionRequest& request)
    : m_request(request)
{
}

void FontSelectionAlgorithm::addCandidate(const FontSelectionDescription& description)
{
    assert(description.weight && description.width && description.slope);

    m_weight.minimum = std::min(m_weight.minimum, description.weight.minimum);
    m_weight.maximum = std::max(m_weight.maximum, description.weight.maximum);

    m_width.minimum = std::min(m_width.minimum, description.width.minimum);
    m_width.maximum = std::max(m_width.maximum, description.width.maximum);

    m_slope.minimum = std::min(m_slope.minimum, description.slope.minimum);
    m_slope.maximum = std::max(m_slope.maximum, description.slope.maximum);
}

FontSelectionValue FontSelectionAlgorithm::widthDistance(const FontSelectionRange& width) const
{
    if(m_request.width >= width.minimum && m_request.width <= width.maximum)
        return FontSelectionValue(0);
    if(m_request.width > kNormalFontWidth) {
        if(width.minimum > m_request.width)
            return width.minimum - m_request.width;
        assert(width.maximum < m_request.width);
        auto threshold = std::max(m_request.width, m_width.maximum);
        return threshold - width.maximum;
    }

    if(width.maximum < m_request.width)
        return m_request.width - width.maximum;
    assert(width.minimum > m_request.width);
    auto threshold = std::min(m_request.width, m_width.minimum);
    return width.minimum - threshold;
}

FontSelectionValue FontSelectionAlgorithm::slopeDistance(const FontSelectionRange& slope) const
{
    if(m_request.slope >= slope.minimum && m_request.slope <= slope.maximum)
        return FontSelectionValue(0);
    if(m_request.slope >= kItalicFontSlope) {
        if(slope.minimum > m_request.slope)
            return slope.minimum - m_request.slope;
        assert(m_request.slope > slope.maximum);
        auto threshold = std::max(m_request.slope, m_slope.maximum);
        return threshold - slope.maximum;
    }

    if(m_request.slope >= FontSelectionValue(0)) {
        if(slope.maximum >= FontSelectionValue(0) && slope.maximum < m_request.slope)
            return m_request.slope - slope.maximum;
        if(slope.minimum > m_request.slope)
            return slope.minimum;
        assert(slope.maximum < FontSelectionValue(0));
        auto threshold = std::max(m_request.slope, m_slope.maximum);
        return threshold - slope.maximum;
    }

    if(m_request.slope > -kItalicFontSlope) {
        if(slope.minimum > m_request.slope && slope.minimum <= FontSelectionValue(0))
            return slope.minimum - m_request.slope;
        if(slope.maximum < m_request.slope)
            return -slope.maximum;
        assert(slope.minimum > FontSelectionValue(0));
        auto threshold = std::min(m_request.slope, m_slope.minimum);
        return slope.minimum - threshold;
    }

    if(slope.maximum < m_request.slope)
        return m_request.slope - slope.maximum;
    assert(slope.minimum > m_request.slope);
    auto threshold = std::min(m_request.slope, m_slope.minimum);
    return slope.minimum - threshold;
}

FontSelectionValue FontSelectionAlgorithm::weightDistance(const FontSelectionRange& weight) const
{
    constexpr FontSelectionValue kUpperWeightSearchThreshold = 500.f;
    constexpr FontSelectionValue kLowerWeightSearchThreshold = 400.f;
    if(m_request.weight >= weight.minimum && m_request.weight <= weight.maximum)
        return FontSelectionValue(0);
    if(m_request.weight >= kLowerWeightSearchThreshold && m_request.weight <= kUpperWeightSearchThreshold) {
        if(weight.minimum > m_request.weight && weight.minimum <= kUpperWeightSearchThreshold)
            return weight.minimum - m_request.weight;
        if(weight.maximum < m_request.weight)
            return kUpperWeightSearchThreshold - weight.maximum;
        assert(weight.minimum > kUpperWeightSearchThreshold);
        auto threshold = std::min(m_request.weight, m_weight.minimum);
        return weight.minimum - threshold;
    }

    if(m_request.weight < kLowerWeightSearchThreshold) {
        if(weight.maximum < m_request.weight)
            return m_request.weight - weight.maximum;
        assert(weight.minimum > m_request.weight);
        auto threshold = std::min(m_request.weight, m_weight.minimum);
        return weight.minimum - threshold;
    }

    assert(m_request.weight >= kUpperWeightSearchThreshold);
    if(weight.minimum > m_request.weight)
        return weight.minimum - m_request.weight;
    assert(weight.maximum < m_request.weight);
    auto threshold = std::max(m_request.weight, m_weight.maximum);
    return threshold - weight.maximum;
}

bool FontSelectionAlgorithm::isCandidateBetter(const FontSelectionDescription& currentCandidate, const FontSelectionDescription& previousCandidate) const
{
    auto widthDelta = widthDistance(currentCandidate.width) - widthDistance(previousCandidate.width);
    if(widthDelta < FontSelectionValue(0))
        return true;
    if(widthDelta > FontSelectionValue(0))
        return false;
    auto slopeDelta = slopeDistance(currentCandidate.slope) - slopeDistance(previousCandidate.slope);
    if(slopeDelta < FontSelectionValue(0))
        return true;
    if(slopeDelta > FontSelectionValue(0))
        return false;
    return weightDistance(currentCandidate.weight) < weightDistance(previousCandidate.weight);
}

RefPtr<FontData> LocalFontFace::getFontData(FontDataCache* fontDataCache, const FontDataDescription& description)
{
    return fontDataCache->getFontData(m_family, description);
}

#define FLT_TO_HB(v) static_cast<hb_position_t>(std::scalbnf(v, +8))
#define HB_TO_FLT(v) std::scalbnf(v, -8)

static std::vector<hb_variation_t>
sortVariations(const FontDataDescription& description,
               const FontVariationList& baseVariations) {
    constexpr FontTag wghtTag = makeFontTag("wght");
    constexpr FontTag wdthTag = makeFontTag("wdth");
    constexpr FontTag slntTag = makeFontTag("slnt");

    std::vector<hb_variation_t> variations;
    variations.reserve(3 + description.variations.size() +
                       baseVariations.size());

    variations.push_back({wghtTag, description.request.weight});
    variations.push_back({wdthTag, description.request.width});
    variations.push_back({slntTag, description.request.slope});
    for (const auto& [tag, value] : description.variations) {
        variations.push_back({tag, value});
    }
    for (const auto& [tag, value] : baseVariations) {
        variations.push_back({tag, value});
    }
    const auto byTag = [](const hb_variation_t& variation) {
        return variation.tag;
    };
    std::ranges::stable_sort(variations, std::ranges::less{}, byTag);
    const auto rng =
        std::ranges::unique(variations, std::ranges::equal_to{}, byTag);
    variations.erase(rng.begin(), rng.end());
    return variations;
}

static FontDataInfo getHBFontDataInfo(hb_font_t* font) {
    const auto get_glyph = [font](hb_codepoint_t unicode) {
        hb_codepoint_t glyph;
        return hb_font_get_glyph(font, unicode, 0, &glyph)
                   ? glyph
                   : ~hb_codepoint_t(0);
    };

    const auto glyph_extents = [font](hb_codepoint_t glyph) {
        hb_glyph_extents_t extents{};
        if (glyph != ~hb_codepoint_t(0)) {
            hb_font_get_glyph_extents(font, glyph, &extents);
        }
        return extents;
    };

    auto zeroGlyph = get_glyph('0');
    auto spaceGlyph = get_glyph(' ');
    auto xGlyph = get_glyph('x');

    hb_font_extents_t font_extents;
    hb_font_get_extents_for_direction(font, HB_DIRECTION_LTR, &font_extents);

    FontDataInfo info;
    info.ascent = HB_TO_FLT(font_extents.ascender);
    info.descent = HB_TO_FLT(-font_extents.descender);
    info.lineGap = HB_TO_FLT(font_extents.line_gap);
    info.xHeight = HB_TO_FLT(-glyph_extents(xGlyph).height);
    info.spaceWidth = HB_TO_FLT(glyph_extents(spaceGlyph).width);
    info.zeroWidth = HB_TO_FLT(glyph_extents(zeroGlyph).width);
    info.zeroGlyph = zeroGlyph;
    info.spaceGlyph = spaceGlyph;
    // info.hasColor = FT_HAS_COLOR(face);
    info.hasColor = false;

    return info;
}

hb_font_t* createHBFont(hb_face_t* face, const FontDataDescription& description,
                        const FontVariationList& baseVariations,
                        FontDataInfo* info) {
    const auto font = hb_font_create(face);

    hb_font_set_scale(font, FLT_TO_HB(description.size),
                      FLT_TO_HB(description.size));

    const auto variations = sortVariations(description, baseVariations);
    hb_font_set_variations(font, variations.data(), variations.size());
    hb_font_make_immutable(font);

    if (info != nullptr) {
        *info = getHBFontDataInfo(font);
    }

    return font;
}

RefPtr<FontData> RemoteFontFace::getFontData(FontDataCache* /*fontDataCache*/, const FontDataDescription& description)
{
    FontDataInfo info;
    const auto font = graphicsManager().createFont(m_resource->face(), description, m_variations, &info);
    return SimpleFontData::create(font, info, m_features);
}

RefPtr<FontData> SegmentedFontFace::getFontData(FontDataCache* fontDataCache, const FontDataDescription& description)
{
    auto& fontData = m_table[description];
    if(fontData != nullptr)
        return fontData;
    FontDataRangeList fonts;
    for(const auto& face : m_faces) {
        if(auto font = face->getFontData(fontDataCache, description)) {
            const auto& ranges = face->ranges();
            if(ranges.empty()) {
                fonts.emplace_back(0, 0x10FFFF, std::move(font));
            } else {
                for(const auto& range : ranges) {
                    fonts.emplace_back(range.first, range.second, font);
                }
            }
        }
    }

    if(!fonts.empty())
        fontData = SegmentedFontData::create(std::move(fonts));
    return fontData;
}

const SimpleFontData* SimpleFontData::getFontData(uint32_t codepoint, bool preferColor) const
{
    if(preferColor && !m_info.hasColor)
        return nullptr;
    return graphicsManager().hasCodepoint(m_hbFont, codepoint) ? this : nullptr;
}

SimpleFontData::~SimpleFontData()
{
    graphicsManager().destroyFont(m_hbFont);
}

const SimpleFontData* FontDataRange::getFontData(uint32_t codepoint, bool preferColor) const
{
    if(m_from <= codepoint && m_to >= codepoint)
        return m_data->getFontData(codepoint, preferColor);
    return nullptr;
}

const SimpleFontData* SegmentedFontData::getFontData(uint32_t codepoint, bool preferColor) const
{
    for(const auto& font : m_fonts) {
        if(auto fontData = font.getFontData(codepoint, preferColor)) {
            return fontData;
        }
    }

    return nullptr;
}

constexpr int fcWeight(FontSelectionValue weight)
{
    if(weight < FontSelectionValue(150))
        return FC_WEIGHT_THIN;
    if(weight < FontSelectionValue(250))
        return FC_WEIGHT_ULTRALIGHT;
    if(weight < FontSelectionValue(350))
        return FC_WEIGHT_LIGHT;
    if(weight < FontSelectionValue(450))
        return FC_WEIGHT_REGULAR;
    if(weight < FontSelectionValue(550))
        return FC_WEIGHT_MEDIUM;
    if(weight < FontSelectionValue(650))
        return FC_WEIGHT_SEMIBOLD;
    if(weight < FontSelectionValue(750))
        return FC_WEIGHT_BOLD;
    if(weight < FontSelectionValue(850))
        return FC_WEIGHT_EXTRABOLD;
    return FC_WEIGHT_ULTRABLACK;
}

constexpr int fcWidth(FontSelectionValue width)
{
    if(width <= kUltraCondensedFontWidth)
        return FC_WIDTH_ULTRACONDENSED;
    if(width <= kExtraCondensedFontWidth)
        return FC_WIDTH_EXTRACONDENSED;
    if(width <= kCondensedFontWidth)
        return FC_WIDTH_CONDENSED;
    if(width <= kSemiCondensedFontWidth)
        return FC_WIDTH_SEMICONDENSED;
    if(width <= kNormalFontWidth)
        return FC_WIDTH_NORMAL;
    if(width <= kSemiExpandedFontWidth)
        return FC_WIDTH_SEMIEXPANDED;
    if(width <= kExpandedFontWidth)
        return FC_WIDTH_EXPANDED;
    if(width <= kExtraExpandedFontWidth)
        return FC_WIDTH_EXTRAEXPANDED;
    return FC_WIDTH_ULTRAEXPANDED;
}

constexpr int fcSlant(FontSelectionValue slope)
{
    if(slope <= kNormalFontSlope)
        return FC_SLANT_ROMAN;
    if(slope <= kItalicFontSlope)
        return FC_SLANT_ITALIC;
    return FC_SLANT_OBLIQUE;
}

hb_face_t* createHBFaceForPattern(FcPattern* pattern)
{
    FcChar8* filename = nullptr;
    int id = 0;
    FcResult ret;

    ret = FcPatternGetString(pattern, FC_FILE, 0, &filename);
    if (ret != FcResultMatch)
        return nullptr;
    /* If FC_INDEX is not set, we just use 0 */
    ret = FcPatternGetInteger(pattern, FC_INDEX, 0, &id);
    if (ret == FcResultOutOfMemory)
        return nullptr;
    return hb_face_create_from_file_or_fail(reinterpret_cast<const char*>(filename), id);
}

static RefPtr<SimpleFontData> createFontDataFromPattern(FcPattern* pattern, const FontDataDescription& description)
{
    if(pattern == nullptr)
        return nullptr;
#if 0
    FcMatrix matrix;
    FcMatrixInit(&matrix);
    {
        int matchMatrixIndex = 0;
        FcMatrix* matchMatrix = nullptr;
        while (FcPatternGetMatrix(pattern, FC_MATRIX, matchMatrixIndex, &matchMatrix) == FcResultMatch) {
            FcMatrixMultiply(&matrix, &matrix, matchMatrix);
            ++matchMatrixIndex;
        }
    }

    cairo_matrix_t ctm;
    cairo_matrix_init_identity(&ctm);

    cairo_matrix_t ftm;
    cairo_matrix_init(&ftm, 1.0, -matrix.yx, -matrix.xy, 1.0, 0.0, 0.0);
    cairo_matrix_scale(&ftm, description.size, description.size);
#endif // 0

    FontFeatureList featureSettings;
    {
        int matchFeatureIndex = 0;
        char* matchFeatureName = nullptr;
        while (FcPatternGetString(pattern, FC_FONT_FEATURES, matchFeatureIndex, (FcChar8**)(&matchFeatureName)) == FcResultMatch) {
            hb_feature_t feature;
            if (hb_feature_from_string(matchFeatureName, -1, &feature))
                featureSettings.emplace_back(FontTag(feature.tag), feature.value);
            ++matchFeatureIndex;
        }
    }

    FontVariationList variationSettings;
    {
        int matchVariationIndex = 0;
        char* matchVariationName = nullptr;
        while (FcPatternGetString(pattern, FC_FONT_VARIATIONS, matchVariationIndex, (FcChar8**)(&matchVariationName)) == FcResultMatch) {
            hb_variation_t variation;
            if (hb_variation_from_string(matchVariationName, -1, &variation))
                variationSettings.emplace_back(FontTag(variation.tag), variation.value);
            ++matchVariationIndex;
        }
    }

    auto face = graphicsManager().createFaceForPattern(pattern);
    FcPatternDestroy(pattern);
    if (face == FaceHandle::Invalid) {
        return nullptr;
    }
    FontDataInfo info;
    const auto font = graphicsManager().createFont(face, description, variationSettings, &info);
    graphicsManager().destroyFace(face);
    return SimpleFontData::create(font, info, featureSettings);
}

static bool isGenericFamilyName(const std::string_view& familyName)
{
    char buffer[16];
    return familyName.length() <= sizeof(buffer) &&
           makeIdentSet({"sans", "sans-serif", "serif", "monospace", "fantasy",
                         "cursive", "emoji"})
               .contains(toLower(familyName, buffer));
}

static RefPtr<SimpleFontData> createFontData(FcConfig* config, GlobalString family, const FontDataDescription& description)
{
    auto pattern = FcPatternCreate();
    FcPatternAddDouble(pattern, FC_PIXEL_SIZE, description.size);
    FcPatternAddInteger(pattern, FC_WEIGHT, fcWeight(description.request.weight));
    FcPatternAddInteger(pattern, FC_WIDTH, fcWidth(description.request.width));
    FcPatternAddInteger(pattern, FC_SLANT, fcSlant(description.request.slope));

    std::string familyName(family);
    FcPatternAddString(pattern, FC_FAMILY, (FcChar8*)(familyName.data()));
    FcPatternAddBool(pattern, FC_SCALABLE, FcTrue);
    if(iequals(familyName, "emoji")) {
        FcPatternAddBool(pattern, FC_COLOR, FcTrue);
    }

    FcConfigSubstitute(config, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    char* configFamilyName = nullptr;
    FcPatternGetString(pattern, FC_FAMILY, 0, (FcChar8**)(&configFamilyName));

    FcResult matchResult;
    auto matchPattern = FcFontMatch(config, pattern, &matchResult);
    if(matchResult == FcResultMatch && !isGenericFamilyName(familyName)) {
        matchResult = FcResultNoMatch;
        FcValue matchValue;
        FcValueBinding matchBinding;
        int matchFamilyIndex = 0;
        while(FcPatternGetWithBinding(matchPattern, FC_FAMILY, matchFamilyIndex, &matchValue, &matchBinding) == FcResultMatch) {
            std::string_view matchFamilyName = (const char*)(matchValue.u.s);
            if (matchBinding == FcValueBindingStrong || iequals(matchFamilyName, configFamilyName) || iequals(matchFamilyName, familyName)) {
               matchResult = FcResultMatch;
               break;
            }

            ++matchFamilyIndex;
        }
    }

    FcPatternDestroy(pattern);
    if(matchResult == FcResultMatch)
        return createFontDataFromPattern(matchPattern, description);
    FcPatternDestroy(matchPattern);
    return nullptr;
}

RefPtr<SimpleFontData> FontDataCache::getFontData(GlobalString family, const FontDataDescription& description)
{
    std::lock_guard guard(m_mutex);
    auto [it, inserted] = m_table.try_emplace(FontDataKey(family, description));
    if(inserted)
        it->second = createFontData(m_config, family, description);
    return it->second;
}

RefPtr<SimpleFontData> FontDataCache::getFontData(uint32_t codepoint, bool preferColor, const FontDataDescription& description)
{
    std::lock_guard guard(m_mutex);
    auto pattern = FcPatternCreate();
    auto charSet = FcCharSetCreate();

    FcCharSetAddChar(charSet, codepoint);
    FcPatternAddCharSet(pattern, FC_CHARSET, charSet);
    FcPatternAddDouble(pattern, FC_PIXEL_SIZE, description.size);
    FcPatternAddInteger(pattern, FC_WEIGHT, fcWeight(description.request.weight));
    FcPatternAddInteger(pattern, FC_WIDTH, fcWidth(description.request.width));
    FcPatternAddInteger(pattern, FC_SLANT, fcSlant(description.request.slope));
    FcPatternAddBool(pattern, FC_SCALABLE, FcTrue);
    if(preferColor) {
        FcPatternAddBool(pattern, FC_COLOR, FcTrue);
    }

    FcConfigSubstitute(m_config, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult matchResult;
    auto matchPattern = FcFontMatch(m_config, pattern, &matchResult);
    if(matchResult == FcResultMatch) {
        matchResult = FcResultNoMatch;
        int matchCharSetIndex = 0;
        FcCharSet* matchCharSet = nullptr;
        while(FcPatternGetCharSet(matchPattern, FC_CHARSET, matchCharSetIndex, &matchCharSet) == FcResultMatch) {
            if(FcCharSetHasChar(matchCharSet, codepoint)) {
                matchResult = FcResultMatch;
                break;
            }

            ++matchCharSetIndex;
        }
    }

    FcCharSetDestroy(charSet);
    FcPatternDestroy(pattern);
    if(matchResult == FcResultMatch)
        return createFontDataFromPattern(matchPattern, description);
    FcPatternDestroy(matchPattern);
    return nullptr;
}

bool FontDataCache::isFamilyAvailable(GlobalString family)
{
    std::lock_guard guard(m_mutex);
    for(auto nameSet : { FcSetSystem, FcSetApplication }) {
        auto allFonts = FcConfigGetFonts(m_config, nameSet);
        if(allFonts == nullptr)
            continue;
        for(int fontIndex = 0; fontIndex < allFonts->nfont; ++fontIndex) {
            auto matchPattern = allFonts->fonts[fontIndex];
            int matchFamilyIndex = 0;
            char* matchFamilyName = nullptr;
            while(FcPatternGetString(matchPattern, FC_FAMILY, matchFamilyIndex, (FcChar8**)(&matchFamilyName)) == FcResultMatch) {
                if(iequals(family.value(), matchFamilyName))
                    return true;
                ++matchFamilyIndex;
            }
        }
    }

    return false;
}

FontDataCache::~FontDataCache()
{
    FcConfigDestroy(m_config);
}

FontDataCache::FontDataCache()
    : m_config(FcInitLoadConfigAndFonts())
{
}

RefPtr<Font> Font::create(Document* document, const FontDescription& description)
{
    return adoptPtr(new Font(document, description));
}

const SimpleFontData* Font::getFontData(uint32_t codepoint, bool preferColor)
{
    for(const auto& font : m_fonts) {
        if(auto fontData = font->getFontData(codepoint, preferColor)) {
            return fontData;
        }
    }

    if(preferColor) {
        if(m_emojiFont == nullptr) {
            static const auto emoji = GlobalString::get("emoji");
            if(auto fontData = m_document->fontDataCache()->getFontData(emoji, m_description.data)) {
                m_emojiFont = fontData.get();
                m_fonts.push_back(std::move(fontData));
            }
        }

        return m_emojiFont;
    }

    if(auto fontData = m_document->fontDataCache()->getFontData(codepoint, preferColor, m_description.data)) {
        m_fonts.push_back(fontData);
        return fontData.get();
    }

    return m_primaryFont;
}

Font::Font(Document* document, const FontDescription& description)
    : m_document(document)
    , m_description(description)
{
    for(const auto& family : description.families) {
        if(auto font = document->styleSheet().getFontData(family, description.data)) {
            if(m_primaryFont == nullptr)
                m_primaryFont = font->getFontData(' ', false);
            m_fonts.push_back(std::move(font));
        }
    }

    if(m_primaryFont == nullptr) {
        static const auto serif = GlobalString::get("serif");
        if(auto fontData = m_document->fontDataCache()->getFontData(serif, description.data)) {
            m_primaryFont = fontData.get();
            m_fonts.push_back(std::move(fontData));
        }
    }
}

struct CairoGraphicsManager::Face {
    cairo_font_face_t* m_face;
    FcCharSet* m_charSet;
    static inline cairo_user_data_key_t key;

    ~Face() {
        FcCharSetDestroy(m_charSet);
        cairo_font_face_destroy(m_face);
    }
};

FaceHandle CairoGraphicsManager::createFaceFromResource(ResourceData resource) {
    thread_local FT_Library ftLibrary;
    if (ftLibrary == nullptr) {
        if (auto error = FT_Init_FreeType(&ftLibrary)) {
            plutobook_set_error_message("font decode error: %s",
                                        FT_Error_String(error));
            return FaceHandle::Invalid;
        }
    }

    FT_Face ftFace = nullptr;
    if (auto error =
            FT_New_Memory_Face(ftLibrary, (FT_Byte*)(resource.content()),
                               resource.contentLength(), 0, &ftFace)) {
        plutobook_set_error_message("font decode error: %s",
                                    FT_Error_String(error));
        return FaceHandle::Invalid;
    }

    auto face = cairo_ft_font_face_create_for_ft_face(ftFace, FT_LOAD_DEFAULT);
    cairo_font_face_set_user_data(
        face, &Face::key, resource.get(), [](void* p) {
            plutobook_resource_data_destroy(
                static_cast<plutobook_resource_data_t*>(p));
        });
    if (auto status = cairo_font_face_status(face)) {
        FT_Done_Face(ftFace);
        return FaceHandle::Invalid;
    }
    resource.release();
    return std::bit_cast<FaceHandle>(
        new Face{face, FcFreeTypeCharSet(ftFace, nullptr)});
}

FaceHandle CairoGraphicsManager::createFaceForPattern(FcPattern* pattern) {
    int matchCharSetIndex = 0;
    FcCharSet* matchCharSet = nullptr;

    auto charSet = FcCharSetCreate();
    while (FcPatternGetCharSet(pattern, FC_CHARSET, matchCharSetIndex,
                               &matchCharSet) == FcResultMatch) {
        FcCharSetMerge(charSet, matchCharSet, nullptr);
        ++matchCharSetIndex;
    }

    auto face = cairo_ft_font_face_create_for_pattern(pattern);
    return std::bit_cast<FaceHandle>(new Face{face, charSet});
}

void CairoGraphicsManager::destroyFace(FaceHandle face) {
    delete std::bit_cast<Face*>(face);
}

struct CairoGraphicsManager::Font {
    cairo_scaled_font_t* m_font;
    hb_font_t* m_hbFont;
    FcCharSet* m_charSet;

    ~Font() {
        FcCharSetDestroy(m_charSet);
        hb_font_destroy(m_hbFont);
        cairo_scaled_font_destroy(m_font);
    }

    static hb_bool_t nominal_glyph_func(hb_font_t*, void* context,
                                        hb_codepoint_t unicode,
                                        hb_codepoint_t* glyph, void*) {
        auto font = static_cast<cairo_scaled_font_t*>(context);
        if (auto face = cairo_ft_scaled_font_lock_face(font)) {
            *glyph = FcFreeTypeCharIndex(face, unicode);
            cairo_ft_scaled_font_unlock_face(font);
            return !!*glyph;
        }

        return false;
    };

    static hb_bool_t variation_glyph_func(hb_font_t*, void* context,
                                          hb_codepoint_t unicode,
                                          hb_codepoint_t variation,
                                          hb_codepoint_t* glyph, void*) {
        auto font = static_cast<cairo_scaled_font_t*>(context);
        if (auto face = cairo_ft_scaled_font_lock_face(font)) {
            *glyph = FT_Face_GetCharVariantIndex(face, unicode, variation);
            cairo_ft_scaled_font_unlock_face(font);
            return !!*glyph;
        }

        return false;
    };

    static hb_position_t glyph_h_advance_func(hb_font_t*, void* context,
                                              hb_codepoint_t index, void*) {
        auto font = static_cast<cairo_scaled_font_t*>(context);
        cairo_text_extents_t extents;
        cairo_glyph_t glyph = {index, 0, 0};
        cairo_scaled_font_glyph_extents(font, &glyph, 1, &extents);
        return FLT_TO_HB(extents.x_advance);
    };

    static hb_bool_t glyph_extents_func(hb_font_t*, void* context,
                                        hb_codepoint_t index,
                                        hb_glyph_extents_t* extents, void*) {
        auto font = static_cast<cairo_scaled_font_t*>(context);
        cairo_text_extents_t glyph_extents;
        cairo_glyph_t glyph = {index, 0, 0};
        cairo_scaled_font_glyph_extents(font, &glyph, 1, &glyph_extents);

        extents->x_bearing = FLT_TO_HB(glyph_extents.x_bearing);
        extents->y_bearing = FLT_TO_HB(glyph_extents.y_bearing);
        extents->width = FLT_TO_HB(glyph_extents.width);
        extents->height = FLT_TO_HB(glyph_extents.height);
        return true;
    };
};

static cairo_scaled_font_t*
createScaledFont(cairo_font_face_t* face,
                 const FontDataDescription& description,
                 const std::vector<hb_variation_t>& variations) {
    const auto slopeAngle =
        -std::tan(description.request.slope * std::numbers::pi / 180.0);
    cairo_matrix_t ctm;
    cairo_matrix_init_identity(&ctm);

    cairo_matrix_t ftm;
    cairo_matrix_init(&ftm, 1, 0, slopeAngle, 1, 0, 0);
    cairo_matrix_scale(&ftm, description.size, description.size);

    auto options = cairo_font_options_create();
    std::string vars;
    for (const auto& [tag, value] : variations) {
        const char name[4] = {HB_UNTAG(tag)};
        if (!vars.empty())
            vars += ',';
        vars.append(name, 4);
        vars += '=';
        vars += toString(value);
    }
    cairo_font_options_set_variations(options, vars.c_str());
    cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_OFF);

    auto font = cairo_scaled_font_create(face, &ftm, &ctm, options);
    cairo_font_options_destroy(options);
    return font;
}

static FontDataInfo getScaledFontDataInfo(cairo_scaled_font_t* font,
                                          FT_Face ftFace) {
    auto zeroGlyph = FcFreeTypeCharIndex(ftFace, '0');
    auto spaceGlyph = FcFreeTypeCharIndex(ftFace, ' ');
    auto xGlyph = FcFreeTypeCharIndex(ftFace, 'x');
    auto glyph_extents = [font](unsigned long index) {
        cairo_glyph_t glyph = {index, 0, 0};
        cairo_text_extents_t extents;
        cairo_scaled_font_glyph_extents(font, &glyph, 1, &extents);
        return extents;
    };

    cairo_font_extents_t font_extents;
    cairo_scaled_font_extents(font, &font_extents);

    FontDataInfo info;
    info.ascent = font_extents.ascent;
    info.descent = font_extents.descent;
    info.lineGap =
        font_extents.height - font_extents.ascent - font_extents.descent;
    info.xHeight = glyph_extents(xGlyph).height;
    info.spaceWidth = glyph_extents(spaceGlyph).x_advance;
    info.zeroWidth = glyph_extents(zeroGlyph).x_advance;
    info.zeroGlyph = zeroGlyph;
    info.spaceGlyph = spaceGlyph;
    info.hasColor = FT_HAS_COLOR(ftFace);

    return info;
}

FontHandle
CairoGraphicsManager::createFont(FaceHandle face,
                                 const FontDataDescription& description,
                                 const FontVariationList& baseVariations,
    FontDataInfo* info) {
    const auto facePtr = std::bit_cast<Face*>(face);
    const auto variations = sortVariations(description, baseVariations);
    const auto font =
        createScaledFont(facePtr->m_face, description, variations);

    const auto ftFace = cairo_ft_scaled_font_lock_face(font);
    if (ftFace == nullptr) {
        cairo_scaled_font_destroy(font);
        return FontHandle::Invalid;
    }

    if (info != nullptr) {
        *info = getScaledFontDataInfo(font, ftFace);
    }

    const auto hbFont = hb_ft_font_create_referenced(ftFace);
    hb_font_set_scale(hbFont, FLT_TO_HB(description.size),
                      FLT_TO_HB(description.size));

    hb_font_set_variations(hbFont, variations.data(), variations.size());
    static hb_font_funcs_t* hbFunctions = [] {
        auto hbFunctions = hb_font_funcs_create();
        hb_font_funcs_set_nominal_glyph_func(
            hbFunctions, Font::nominal_glyph_func, nullptr, nullptr);
        hb_font_funcs_set_variation_glyph_func(
            hbFunctions, Font::variation_glyph_func, nullptr, nullptr);
        hb_font_funcs_set_glyph_h_advance_func(
            hbFunctions, Font::glyph_h_advance_func, nullptr, nullptr);
        hb_font_funcs_set_glyph_extents_func(
            hbFunctions, Font::glyph_extents_func, nullptr, nullptr);
        hb_font_funcs_make_immutable(hbFunctions);
        return hbFunctions;
    }();

    hb_font_set_funcs(hbFont, hbFunctions, font, nullptr);
    hb_font_make_immutable(hbFont);
    cairo_ft_scaled_font_unlock_face(font);

    const auto charSet = FcCharSetCopy(facePtr->m_charSet);
    return std::bit_cast<FontHandle>(new Font{font, hbFont, charSet});
}

hb_font_t* CairoGraphicsManager::getHBFont(FontHandle font) const {
    return std::bit_cast<Font*>(font)->m_hbFont;
}

bool CairoGraphicsManager::hasCodepoint(FontHandle font,
                                        uint32_t codepoint) const {
    return FcCharSetHasChar(std::bit_cast<Font*>(font)->m_charSet, codepoint);
}

void CairoGraphicsManager::destroyFont(FontHandle font) {
    delete std::bit_cast<Font*>(font);
}

cairo_scaled_font_t* CairoGraphicsManager::getScaledFont(FontHandle font) {
    return std::bit_cast<Font*>(font)->m_font;
}

} // namespace plutobook
