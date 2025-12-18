#include "font-resource.h"
#include "document.h"
#include "string-utils.h"
#include "ident-table.h"

#include "plutobook.hpp"

#include <fontconfig/fontconfig.h>
//#include <fontconfig/fcfreetype.h>

//#include <cairo/cairo-ft.h>
#include <harfbuzz/hb.h>

#include <numbers>
#include <cmath>

namespace plutobook {

static hb_face_t* createFaceFromResource(ResourceData resource)
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
    auto face = createFaceFromResource(std::move(resource));
    if(face == nullptr) {
        plutobook_set_error_message("Unable to load font '%s': %s", url.value().data(), plutobook_get_error_message());
        return nullptr;
    }

#if 0
    static cairo_user_data_key_t key;
    auto face = cairo_ft_font_face_create_for_ft_face(fontData->face(), FT_LOAD_DEFAULT);
    cairo_font_face_set_user_data(face, &key, fontData, FTFontDataDestroy);
    if (auto status = cairo_font_face_status(face)) {
        plutobook_set_error_message("Unable to load font '%s': %s", url.value().data(), cairo_status_to_string(status));
        FTFontDataDestroy(fontData);
        return nullptr;
    }
#endif // 0

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
    hb_face_destroy(m_face);
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

RefPtr<FontData> LocalFontFace::getFontData(const FontDataDescription& description)
{
    return fontDataCache()->getFontData(m_family, description);
}

static std::string buildVariationSettings(const FontDataDescription& description, const FontVariationList& variations)
{
    constexpr FontTag wghtTag("wght");
    constexpr FontTag wdthTag("wdth");
    constexpr FontTag slntTag("slnt");

    boost::unordered_flat_map<FontTag, float> variationSettings(description.variations.begin(), description.variations.end());

    variationSettings.emplace(wghtTag, description.request.weight);
    variationSettings.emplace(wdthTag, description.request.width);
    variationSettings.emplace(slntTag, description.request.slope);

    std::string output;
    for(const auto& [tag, value] : variationSettings) {
        const char name[4] = {
            static_cast<char>(0xFF & (tag.value() >> 24)),
            static_cast<char>(0xFF & (tag.value() >> 16)),
            static_cast<char>(0xFF & (tag.value() >> 8)),
            static_cast<char>(0xFF & (tag.value() >> 0))
        };

        if(!output.empty())
            output += ',';
        output.append(name, 4);
        output += '=';
        output += toString(value);
    }

    return output;
}

RefPtr<FontData> RemoteFontFace::getFontData(const FontDataDescription& description)
{
    const auto slopeAngle = -std::tan(description.request.slope * std::numbers::pi / 180.0);
#if 0
    cairo_matrix_t ctm;
    cairo_matrix_init_identity(&ctm);

    cairo_matrix_t ftm;
    cairo_matrix_init(&ftm, 1, 0, slopeAngle, 1, 0, 0);
    cairo_matrix_scale(&ftm, description.size, description.size);

    auto options = cairo_font_options_create();
    auto variations = buildVariationSettings(description, m_variations);
    cairo_font_options_set_variations(options, variations.data());
    cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_OFF);

    auto charSet = FcCharSetCopy(m_resource->charSet());
    auto face = cairo_font_face_reference(m_resource->face());
    auto font = cairo_scaled_font_create(face, &ftm, &ctm, options);

    cairo_font_face_destroy(face);
    cairo_font_options_destroy(options);
#endif
    return SimpleFontData::create(m_resource->face(), description, m_features);
}

RefPtr<FontData> SegmentedFontFace::getFontData(const FontDataDescription& description)
{
    auto& fontData = m_table[description];
    if(fontData != nullptr)
        return fontData;
    FontDataRangeList fonts;
    for(const auto& face : m_faces) {
        if(auto font = face->getFontData(description)) {
            const auto& ranges = face->ranges();
            if(ranges.empty()) {
                fonts.emplace_front(0, 0x10FFFF, std::move(font));
            } else {
                for(const auto& range : ranges) {
                    fonts.emplace_front(range.first, range.second, font);
                }
            }
        }
    }

    if(!fonts.empty())
        fontData = SegmentedFontData::create(std::move(fonts));
    return fontData;
}

#define FLT_TO_HB(v) static_cast<hb_position_t>(std::scalbnf(v, +8))
#define HB_TO_FLT(v) std::scalbnf(v, -8)

RefPtr<SimpleFontData> SimpleFontData::create(hb_face_t* face,
    const FontDataDescription& description,
    const FontFeatureList& features)
{
    auto hbFont = hb_font_create(face);

    hb_font_set_scale(hbFont, FLT_TO_HB(description.size), FLT_TO_HB(description.size));

    std::vector<hb_variation_t> settings;
    for (const auto& variation : description.variations) {
        hb_variation_t setting{variation.first.value(), variation.second};
        settings.push_back(setting);
    }

    hb_font_set_variations(hbFont, settings.data(), settings.size());

    hb_font_make_immutable(hbFont);

    const auto get_glyph = [hbFont](hb_codepoint_t unicode) {
        hb_codepoint_t glyph;
        return hb_font_get_glyph(hbFont, unicode, 0, &glyph) ? glyph : ~hb_codepoint_t(0);
    };

    const auto glyph_extents = [hbFont](hb_codepoint_t glyph) {
        hb_glyph_extents_t extents{};
        if (glyph != ~hb_codepoint_t(0)) {
            hb_font_get_glyph_extents(hbFont, glyph, &extents);
        }
        return extents;
    };

    auto zeroGlyph = get_glyph('0');
    auto spaceGlyph = get_glyph(' ');
    auto xGlyph = get_glyph('x');

    hb_font_extents_t font_extents;
    hb_font_get_extents_for_direction(hbFont, HB_DIRECTION_LTR, &font_extents);

    FontDataInfo info;
    info.ascent = HB_TO_FLT(font_extents.ascender);
    info.descent = HB_TO_FLT(-font_extents.descender);
    info.lineGap = HB_TO_FLT(font_extents.line_gap);
    info.xHeight = HB_TO_FLT(-glyph_extents(xGlyph).height);
    info.spaceWidth = HB_TO_FLT(glyph_extents(spaceGlyph).width);
    info.zeroWidth = HB_TO_FLT(glyph_extents(zeroGlyph).width);
    info.zeroGlyph = zeroGlyph;
    info.spaceGlyph = spaceGlyph;
    //info.hasColor = FT_HAS_COLOR(face);
    info.hasColor = false;

    return adoptPtr(new SimpleFontData(hbFont, info, features));
}

const SimpleFontData* SimpleFontData::getFontData(uint32_t codepoint, bool preferColor) const
{
    if(preferColor && !m_info.hasColor)
        return nullptr;
    hb_codepoint_t glyph;
    return hb_font_get_glyph(m_hbFont, codepoint, 0, &glyph) ? this : nullptr;
}

SimpleFontData::~SimpleFontData()
{
    hb_font_destroy(m_hbFont);
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

static hb_face_t* createFTFaceForPattern(FcPattern* pattern)
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
    FcMatrix matrix;
    FcMatrixInit(&matrix);

    int matchMatrixIndex = 0;
    FcMatrix* matchMatrix = nullptr;
    while(FcPatternGetMatrix(pattern, FC_MATRIX, matchMatrixIndex, &matchMatrix) == FcResultMatch) {
        FcMatrixMultiply(&matrix, &matrix, matchMatrix);
        ++matchMatrixIndex;
    }

    int matchCharSetIndex = 0;
    FcCharSet* matchCharSet = nullptr;

    auto charSet = FcCharSetCreate();
    while(FcPatternGetCharSet(pattern, FC_CHARSET, matchCharSetIndex, &matchCharSet) == FcResultMatch) {
        FcCharSetMerge(charSet, matchCharSet, nullptr);
        ++matchCharSetIndex;
    }

#if 0
    cairo_matrix_t ctm;
    cairo_matrix_init_identity(&ctm);

    cairo_matrix_t ftm;
    cairo_matrix_init(&ftm, 1.0, -matrix.yx, -matrix.xy, 1.0, 0.0, 0.0);
    cairo_matrix_scale(&ftm, description.size, description.size);
#endif // 0

    FontFeatureList featureSettings;
    FontVariationList variationSettings;

    int matchFeatureIndex = 0;
    char* matchFeatureName = nullptr;
    while(FcPatternGetString(pattern, FC_FONT_FEATURES, matchFeatureIndex, (FcChar8**)(&matchFeatureName)) == FcResultMatch) {
        hb_feature_t feature;
        if(hb_feature_from_string(matchFeatureName, -1, &feature))
            featureSettings.emplace_front(feature.tag, feature.value);
        ++matchFeatureIndex;
    }

    int matchVariationIndex = 0;
    char* matchVariationName = nullptr;
    while(FcPatternGetString(pattern, FC_FONT_VARIATIONS, matchVariationIndex, (FcChar8**)(&matchVariationName)) == FcResultMatch) {
        hb_variation_t variation;
        if(hb_variation_from_string(matchVariationName, -1, &variation))
            variationSettings.emplace_front(variation.tag, variation.value);
        ++matchVariationIndex;
    }

#if 0
    auto options = cairo_font_options_create();
    auto variations = buildVariationSettings(description, variationSettings);
    cairo_font_options_set_variations(options, variations.data());
    cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_OFF);

    auto face = cairo_ft_font_face_create_for_pattern(pattern);
    auto font = cairo_scaled_font_create(face, &ftm, &ctm, options);

    cairo_font_face_destroy(face);
    cairo_font_options_destroy(options);
#endif // 0
    auto face = createFTFaceForPattern(pattern);
    FcPatternDestroy(pattern);
    if (face == nullptr) {
        return nullptr;
    }
    const auto ret = SimpleFontData::create(face, description, featureSettings);
    hb_face_destroy(face);
    return ret;
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
    if(familyName == "emoji") {
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
            if (matchBinding == FcValueBindingStrong || matchFamilyName == configFamilyName || matchFamilyName == familyName) {
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
                if(family.value() == matchFamilyName)
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

FontDataCache* fontDataCache()
{
    static FontDataCache fontCache;
    return &fontCache;
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
            if(auto fontData = fontDataCache()->getFontData(emoji, m_description.data)) {
                m_emojiFont = fontData.get();
                m_fonts.push_back(std::move(fontData));
            }
        }

        return m_emojiFont;
    }

    if(auto fontData = fontDataCache()->getFontData(codepoint, preferColor, m_description.data)) {
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
        if(auto fontData = fontDataCache()->getFontData(serif, description.data)) {
            m_primaryFont = fontData.get();
            m_fonts.push_back(std::move(fontData));
        }
    }
}

} // namespace plutobook
