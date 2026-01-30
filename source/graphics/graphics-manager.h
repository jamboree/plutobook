#pragma once

#include <vector>

typedef struct _FcConfig FcConfig;
typedef struct _FcPattern FcPattern;
typedef struct hb_font_t hb_font_t;
typedef struct _cairo_scaled_font cairo_scaled_font_t;

namespace plutobook {
    class Size;
    class ResourceData;
    struct FontDataDescription;
    struct FontDataInfo;
    enum FontTag : uint32_t;

    using FontFeature = std::pair<FontTag, int>;
    using FontVariation = std::pair<FontTag, float>;
    using FontFeatureList = std::vector<FontFeature>;
    using FontVariationList = std::vector<FontVariation>;

    enum class ImageHandle : uintptr_t { Invalid };
    enum class FaceHandle : uintptr_t { Invalid };
    enum class FontHandle : uintptr_t { Invalid };

    class GraphicsManager {
    public:
        virtual ImageHandle createImage(const void* data, size_t size) = 0;
        virtual void destroyImage(ImageHandle handle) = 0;
        virtual Size getImageSize(ImageHandle handle) const = 0;

        virtual FaceHandle createFaceFromResource(ResourceData resource) = 0;
        virtual FaceHandle createFaceForPattern(FcPattern* pattern) = 0;
        virtual void destroyFace(FaceHandle face) = 0;
        virtual FontHandle createFont(FaceHandle face,
                                      const FontDataDescription& description,
                                      const FontVariationList& baseVariations,
                                      FontDataInfo* info) = 0;
        virtual hb_font_t* getHBFont(FontHandle font) const = 0;
        virtual bool hasCodepoint(FontHandle font,
                                  uint32_t codepoint) const = 0;
        virtual void destroyFont(FontHandle font) = 0;
    };

    class CairoGraphicsManager : public GraphicsManager {
        struct Face;
        struct Font;

    public:
        ImageHandle createImage(const void* data, size_t size) override;
        void destroyImage(ImageHandle handle) override;
        Size getImageSize(ImageHandle handle) const override;
        FaceHandle createFaceFromResource(ResourceData resource) override;
        FaceHandle createFaceForPattern(FcPattern* pattern) override;
        void destroyFace(FaceHandle face) override;
        FontHandle createFont(FaceHandle face,
                              const FontDataDescription& description,
                              const FontVariationList& baseVariations,
                              FontDataInfo* info) override;
        hb_font_t* getHBFont(FontHandle font) const override;
        bool hasCodepoint(FontHandle font, uint32_t codepoint) const override;
        void destroyFont(FontHandle font) override;

        static cairo_scaled_font_t* getScaledFont(FontHandle font);
    };
} // namespace plutobook