#pragma once

#include "box-style.h"
#include "geometry.h"
#include "graphics-handle.h"

#include <memory>

typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;
typedef struct hb_font_t hb_font_t;

namespace plutobook {
    class ImageBuffer;

    using GradientStop = std::pair<float, Color>;
    using GradientStops = std::vector<GradientStop>;

    struct LinearGradientValues {
        float x1 = 0.f;
        float y1 = 0.f;
        float x2 = 0.f;
        float y2 = 0.f;
    };

    struct RadialGradientValues {
        float fx = 0.f;
        float fy = 0.f;
        float cx = 0.f;
        float cy = 0.f;
        float r = 0.f;
    };

    enum class SpreadMethod { Pad, Reflect, Repeat };

    using DashArray = std::vector<float>;

    class StrokeData {
    public:
        explicit StrokeData(float lineWidth = 1.f) : m_lineWidth(lineWidth) {}

        void setLineWidth(float lineWidth) { m_lineWidth = lineWidth; }
        float lineWidth() const { return m_lineWidth; }

        void setMiterLimit(float miterLimit) { m_miterLimit = miterLimit; }
        float miterLimit() const { return m_miterLimit; }

        void setDashOffset(float dashOffset) { m_dashOffset = dashOffset; }
        float dashOffset() const { return m_dashOffset; }

        void setDashArray(DashArray dashArray) {
            m_dashArray = std::move(dashArray);
        }
        const DashArray& dashArray() const { return m_dashArray; }

        void setLineCap(LineCap lineCap) { m_lineCap = lineCap; }
        LineCap lineCap() const { return m_lineCap; }

        void setLineJoin(LineJoin lineJoin) { m_lineJoin = lineJoin; }
        LineJoin lineJoin() const { return m_lineJoin; }

    private:
        float m_lineWidth;
        float m_miterLimit{10.f};
        float m_dashOffset{0.f};
        LineCap m_lineCap{LineCap::Butt};
        LineJoin m_lineJoin{LineJoin::Miter};
        DashArray m_dashArray;
    };

    struct GlyphRef {
        unsigned m_index;
        Point m_position;
    };

    class GraphicsManager {
    public:
        virtual ImageHandle createImage(const void* data, size_t size) = 0;
        virtual void destroyImage(ImageHandle handle) = 0;
        virtual Size getImageSize(ImageHandle handle) const = 0;
    };

    class GraphicsContext {
    public:
        GraphicsContext() = default;
        GraphicsContext(const GraphicsContext&) = delete;
        GraphicsContext& operator=(const GraphicsContext&) = delete;

        virtual void setColor(const Color& color) = 0;
        virtual void setLinearGradient(const LinearGradientValues& values,
                                       const GradientStops& stops,
                                       const Transform& transform,
                                       SpreadMethod method, float opacity) = 0;
        virtual void setRadialGradient(const RadialGradientValues& values,
                                       const GradientStops& stops,
                                       const Transform& transform,
                                       SpreadMethod method, float opacity) = 0;
        virtual void setPattern(cairo_surface_t* pattern,
                                const Transform& transform) = 0;

        virtual void translate(float tx, float ty) = 0;
        virtual void scale(float sx, float sy) = 0;
        virtual void rotate(float angle) = 0;

        virtual Transform getTransform() const = 0;
        virtual void addTransform(const Transform& transform) = 0;
        virtual void setTransform(const Transform& transform) = 0;
        virtual void resetTransform() = 0;

        virtual void fillRect(const Rect& rect) = 0;
        virtual void fillRoundedRect(const RoundedRect& rrect) = 0;
        virtual void fillPath(const Path& path,
                              FillRule fillRule = FillRule::NonZero) = 0;
        virtual void fillGlyphs(hb_font_t* font, const GlyphRef glyphs[],
                                unsigned glyphCount) = 0;
        virtual void fillImage(ImageHandle image, const Rect& dstRect,
                               const Rect& srcRect) = 0;
        virtual void fillImagePattern(ImageHandle image, const Rect& destRect,
                                      const Size& size, const Size& scale,
                                      const Point& phase) = 0;

        virtual void outlineRect(const Rect& rect, float lineWidth) = 0;
        virtual void outlineRoundedRect(const RoundedRect& rrect,
                                        float lineWidth) = 0;
        virtual void strokePath(const Path& path,
                                const StrokeData& strokeData) = 0;

        virtual void clipRect(const Rect& rect,
                              FillRule clipRule = FillRule::NonZero) = 0;
        virtual void clipRoundedRect(const RoundedRect& rrect,
                                     FillRule clipRule = FillRule::NonZero) = 0;
        virtual void clipPath(const Path& path,
                              FillRule clipRule = FillRule::NonZero) = 0;

        virtual void clipOutRect(const Rect& rect) = 0;
        virtual void clipOutRoundedRect(const RoundedRect& rrect) = 0;
        virtual void clipOutPath(const Path& path) = 0;

        virtual void save() = 0;
        virtual void restore() = 0;

        virtual void pushGroup() = 0;
        virtual void popGroup(float opacity,
                              BlendMode blendMode = BlendMode::Normal) = 0;
        virtual void applyMask(const ImageBuffer& maskImage) = 0;

        virtual void addLinkAnnotation(const std::string_view& dest,
                                       const std::string_view& uri,
                                       const Rect& rect) = 0;
        virtual void addLinkDestination(const std::string_view& name,
                                        const Point& location) = 0;
    };

    class CairoGraphicsContext final : public GraphicsContext {
    public:
        CairoGraphicsContext() = delete;
        explicit CairoGraphicsContext(cairo_t* canvas);
        ~CairoGraphicsContext();

        void setColor(const Color& color) override;
        void setLinearGradient(const LinearGradientValues& values,
                               const GradientStops& stops,
                               const Transform& transform, SpreadMethod method,
                               float opacity) override;
        void setRadialGradient(const RadialGradientValues& values,
                               const GradientStops& stops,
                               const Transform& transform, SpreadMethod method,
                               float opacity) override;
        void setPattern(cairo_surface_t* surface,
                        const Transform& transform) override;

        void translate(float tx, float ty) override;
        void scale(float sx, float sy) override;
        void rotate(float angle) override;

        Transform getTransform() const override;
        void addTransform(const Transform& transform) override;
        void setTransform(const Transform& transform) override;
        void resetTransform() override;

        void fillRect(const Rect& rect) override;
        void fillRoundedRect(const RoundedRect& rrect) override;
        void fillPath(const Path& path,
                      FillRule fillRule = FillRule::NonZero) override;
        void fillGlyphs(hb_font_t* font, const GlyphRef glyphs[],
                        unsigned glyphCount) override;
        void fillImage(ImageHandle image, const Rect& dstRect,
                       const Rect& srcRect) override;
        void fillImagePattern(ImageHandle image, const Rect& destRect,
                              const Size& size, const Size& scale,
                              const Point& phase) override;

        void outlineRect(const Rect& rect, float lineWidth) override;
        void outlineRoundedRect(const RoundedRect& rrect,
                                float lineWidth) override;
        void strokePath(const Path& path,
                        const StrokeData& strokeData) override;

        void clipRect(const Rect& rect,
                      FillRule clipRule = FillRule::NonZero) override;
        void clipRoundedRect(const RoundedRect& rrect,
                             FillRule clipRule = FillRule::NonZero) override;
        void clipPath(const Path& path,
                      FillRule clipRule = FillRule::NonZero) override;

        void clipOutRect(const Rect& rect) override;
        void clipOutRoundedRect(const RoundedRect& rrect) override;
        void clipOutPath(const Path& path) override;

        void save() override;
        void restore() override;

        void pushGroup() override;
        void popGroup(float opacity,
                      BlendMode blendMode = BlendMode::Normal) override;
        void applyMask(const ImageBuffer& maskImage) override;

        void addLinkAnnotation(const std::string_view& dest,
                               const std::string_view& uri,
                               const Rect& rect) override;
        void addLinkDestination(const std::string_view& name,
                                const Point& location) override;

        cairo_t* canvas() const { return m_canvas; }

    private:
        cairo_t* m_canvas;
    };

    class ImageBuffer {
    public:
        static std::unique_ptr<ImageBuffer> create(const Rect& rect);
        static std::unique_ptr<ImageBuffer> create(float x, float y,
                                                   float width, float height);

        cairo_surface_t* surface() const { return m_surface; }
        cairo_t* canvas() const { return m_canvas; }

        const int x() const { return m_x; }
        const int y() const { return m_y; }

        const int width() const;
        const int height() const;

        void convertToLuminanceMask();

        ~ImageBuffer();

    private:
        ImageBuffer(int x, int y, int width, int height);
        cairo_surface_t* m_surface;
        cairo_t* m_canvas;
        const int m_x;
        const int m_y;
    };
} // namespace plutobook