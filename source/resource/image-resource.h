#pragma once

#include "resource.h"
#include "geometry.h"

#include <memory>

typedef struct _cairo_surface cairo_surface_t;

namespace plutobook {
    class Image;
    class Document;

    class ImageResource final : public Resource {
    public:
        static constexpr ClassKind classKind = ClassKind::Image;

        static RefPtr<ImageResource> create(Document* document, const Url& url);
        static RefPtr<Image> decode(const char* data, size_t size,
                                    const std::string_view& mimeType,
                                    const std::string_view& textEncoding,
                                    const std::string_view& baseUrl,
                                    ResourceFetcher* fetcher);
        static bool supportsMimeType(const std::string_view& mimeType);
        const RefPtr<Image>& image() const { return m_image; }

    private:
        ImageResource(RefPtr<Image> image)
            : Resource(classKind), m_image(std::move(image)) {}
        RefPtr<Image> m_image;
    };

    class GraphicsContext;

    enum class ImageType {
        Bitmap,
        Svg,
    };

    class Image : public RefCounted<Image> {
    public:
        using ClassRoot = Image;
        using ClassKind = ImageType;

        explicit Image(ClassKind type) : m_type(type) {}
        virtual ~Image() = default;

        ClassKind type() const noexcept { return m_type; }

        void drawTiled(GraphicsContext& context, const Rect& destRect,
                       const Rect& tileRect);

        virtual void draw(GraphicsContext& context, const Rect& dstRect,
                          const Rect& srcRect) = 0;
        virtual void drawPattern(GraphicsContext& context, const Rect& destRect,
                                 const Size& size, const Size& scale,
                                 const Point& phase) = 0;
        virtual void computeIntrinsicDimensions(float& intrinsicWidth,
                                                float& intrinsicHeight,
                                                double& intrinsicRatio) = 0;

        virtual void setContainerSize(const Size& size) = 0;
        virtual Size intrinsicSize() const = 0;
        virtual Size size() const = 0;

    protected:
        ClassKind m_type;
    };

    class BitmapImage final : public Image {
    public:
        static constexpr ClassKind classKind = ClassKind::Bitmap;

        static RefPtr<BitmapImage> create(const char* data, size_t size);

        void draw(GraphicsContext& context, const Rect& dstRect,
                  const Rect& srcRect) final;
        void drawPattern(GraphicsContext& context, const Rect& destRect,
                         const Size& size, const Size& scale,
                         const Point& phase) final;
        void computeIntrinsicDimensions(float& intrinsicWidth,
                                        float& intrinsicHeight,
                                        double& intrinsicRatio) final;

        void setContainerSize(const Size& size) final {}
        Size intrinsicSize() const final { return m_intrinsicSize; }
        Size size() const final { return m_intrinsicSize; }

        ~BitmapImage() final;

    private:
        BitmapImage(cairo_surface_t* surface);
        cairo_surface_t* m_surface;
        Size m_intrinsicSize;
    };

    class SvgDocument;
    class Heap;

    class SvgImage final : public Image {
    public:
        static constexpr ClassKind classKind = ClassKind::Svg;

        static RefPtr<SvgImage> create(const std::string_view& content,
                                       const std::string_view& baseUrl,
                                       ResourceFetcher* fetcher);

        void draw(GraphicsContext& context, const Rect& dstRect,
                  const Rect& srcRect) final;
        void drawPattern(GraphicsContext& context, const Rect& destRect,
                         const Size& size, const Size& scale,
                         const Point& phase) final;
        void computeIntrinsicDimensions(float& intrinsicWidth,
                                        float& intrinsicHeight,
                                        double& intrinsicRatio) final;

        void setContainerSize(const Size& size) final;
        Size intrinsicSize() const final;
        Size size() const final;

    private:
        SvgImage(std::unique_ptr<Heap> heap,
                 std::unique_ptr<SvgDocument> document);
        std::unique_ptr<Heap> m_heap;
        std::unique_ptr<SvgDocument> m_document;
        Size m_containerSize;
    };
} // namespace plutobook