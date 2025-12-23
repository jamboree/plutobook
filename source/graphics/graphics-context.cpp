#include "graphics-context.h"
#include "plutobook.hpp"

#include <cairo/cairo.h>
#ifdef PLUTOBOOK_HAS_WEBP
#include <webp/decode.h>
#endif

#ifdef PLUTOBOOK_HAS_TURBOJPEG
#define STBI_NO_JPEG
#include <turbojpeg.h>
#endif

#ifdef CAIRO_HAS_PNG_FUNCTIONS
#define STBI_NO_PNG
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmath>
#include "output-stream.h"

namespace plutobook {

constexpr cairo_fill_rule_t to_cairo_fill_rule(FillRule fillRule)
{
    return fillRule == FillRule::NonZero ? CAIRO_FILL_RULE_WINDING : CAIRO_FILL_RULE_EVEN_ODD;
}

constexpr cairo_operator_t to_cairo_operator(BlendMode blendMode)
{
    switch(blendMode) {
    case BlendMode::Normal:
        return CAIRO_OPERATOR_OVER;
    case BlendMode::Multiply:
        return CAIRO_OPERATOR_MULTIPLY;
    case BlendMode::Screen:
        return CAIRO_OPERATOR_SCREEN;
    case BlendMode::Overlay:
        return CAIRO_OPERATOR_OVERLAY;
    case BlendMode::Darken:
        return CAIRO_OPERATOR_DARKEN;
    case BlendMode::Lighten:
        return CAIRO_OPERATOR_LIGHTEN;
    case BlendMode::ColorDodge:
        return CAIRO_OPERATOR_COLOR_DODGE;
    case BlendMode::ColorBurn:
        return CAIRO_OPERATOR_COLOR_BURN;
    case BlendMode::HardLight:
        return CAIRO_OPERATOR_HARD_LIGHT;
    case BlendMode::SoftLight:
        return CAIRO_OPERATOR_SOFT_LIGHT;
    case BlendMode::Difference:
        return CAIRO_OPERATOR_DIFFERENCE;
    case BlendMode::Exclusion:
        return CAIRO_OPERATOR_EXCLUSION;
    case BlendMode::Hue:
        return CAIRO_OPERATOR_HSL_HUE;
    case BlendMode::Saturation:
        return CAIRO_OPERATOR_HSL_SATURATION;
    case BlendMode::Color:
        return CAIRO_OPERATOR_HSL_COLOR;
    case BlendMode::Luminosity:
        return CAIRO_OPERATOR_HSL_LUMINOSITY;
    }

    return CAIRO_OPERATOR_OVER;
}

static cairo_matrix_t to_cairo_matrix(const Transform& transform)
{
    cairo_matrix_t matrix;
    cairo_matrix_init(&matrix, transform.a, transform.b, transform.c, transform.d, transform.e, transform.f);
    return matrix;
}

static void set_cairo_stroke_data(cairo_t* cr, const StrokeData& strokeData)
{
    const auto& dashes = strokeData.dashArray();
    cairo_set_line_width(cr, strokeData.lineWidth());
    cairo_set_miter_limit(cr, strokeData.miterLimit());
    double* dashBuf;
    double smallDashBuf[16];
    std::unique_ptr<double[]> bigDashBuf;
    if(dashes.size() > 16) {
        bigDashBuf.reset(dashBuf = new double[dashes.size()]);
    } else {
        dashBuf = smallDashBuf;
    }
    std::copy_n(dashes.data(), dashes.size(), dashBuf);
    cairo_set_dash(cr, dashBuf, dashes.size(), strokeData.dashOffset());
    switch(strokeData.lineCap()) {
    case LineCap::Butt:
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
        break;
    case LineCap::Round:
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        break;
    case LineCap::Square:
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
        break;
    }

    switch(strokeData.lineJoin()) {
    case LineJoin::Miter:
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
        break;
    case LineJoin::Round:
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        break;
    case LineJoin::Bevel:
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
        break;
    }
}

static void set_cairo_path(cairo_t* cr, const Path& path)
{
    PathIterator it(path);
    std::array<Point, 3> p;
    while(!it.isDone()) {
        switch(it.currentSegment(p)) {
        case PathCommand::MoveTo:
            cairo_move_to(cr, p[0].x, p[0].y);
            break;
        case PathCommand::LineTo:
            cairo_line_to(cr, p[0].x, p[0].y);
            break;
        case PathCommand::CubicTo:
            cairo_curve_to(cr, p[0].x, p[0].y, p[1].x, p[1].y, p[2].x, p[2].y);
            break;
        case PathCommand::Close:
            cairo_close_path(cr);
            break;
        }

        it.next();
    }
}

static void set_cairo_gradient(cairo_pattern_t* pattern, const GradientInfo& info)
{
    for(const auto& stop : info.stops) {
        auto red = stop.second.red() / 255.0;
        auto green = stop.second.green() / 255.0;
        auto blue = stop.second.blue() / 255.0;
        auto alpha = stop.second.alpha() / 255.0;
        cairo_pattern_add_color_stop_rgba(pattern, stop.first, red, green, blue, alpha * info.opacity);
    }

    switch(info.method) {
    case SpreadMethod::Pad:
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);
        break;
    case SpreadMethod::Reflect:
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REFLECT);
        break;
    case SpreadMethod::Repeat:
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
        break;
    }

    cairo_matrix_t matrix;
    if (info.objectBoundingBox) {
        const auto& bbox = *info.objectBoundingBox;
        matrix = to_cairo_matrix(Transform(bbox.w, 0, 0, bbox.h, bbox.x, bbox.y) * info.transform);
    } else {
        matrix = to_cairo_matrix(info.transform);
    }
    cairo_matrix_invert(&matrix);
    cairo_pattern_set_matrix(pattern, &matrix);
}

class DefaultGraphicsManager : public GraphicsManager {
public:
    ImageHandle createImage(const void* data, size_t size) override {
        return ImageHandle::Invalid;
    }

    void destroyImage(ImageHandle handle) override {}

    Size getImageSize(ImageHandle handle) const override {
        std::unreachable();
    }
};

#ifdef CAIRO_HAS_PNG_FUNCTIONS

struct png_read_stream_t {
    const char* data;
    size_t size;
};

static cairo_status_t png_read_function(void* closure, uint8_t* data, uint32_t length)
{
    auto stream = static_cast<png_read_stream_t*>(closure);
    if (length > stream->size)
        return CAIRO_STATUS_READ_ERROR;
    std::memcpy(data, stream->data, length);
    stream->data += length;
    stream->size -= length;
    return CAIRO_STATUS_SUCCESS;
}

#endif // CAIRO_HAS_PNG_FUNCTIONS

static cairo_surface_t* decodeBitmapImage(const char* data, size_t size)
{
#ifdef CAIRO_HAS_PNG_FUNCTIONS
    if (size > 8 && std::memcmp(data, "\x89PNG\r\n\x1A\n", 8) == 0) {
        png_read_stream_t stream = {data, size};
        return cairo_image_surface_create_from_png_stream(png_read_function, &stream);
    }
#endif // CAIRO_HAS_PNG_FUNCTIONS
#ifdef PLUTOBOOK_HAS_TURBOJPEG
    if (size > 3 && std::memcmp(data, "\xFF\xD8\xFF", 3) == 0) {
        int width, height;
        auto tj = tjInitDecompress();
        if (!tj || tjDecompressHeader(tj, (uint8_t*)(data), size, &width, &height) == -1) {
            plutobook_set_error_message("image decode error: %s", tjGetErrorStr());
            tjDestroy(tj);
            return nullptr;
        }

        auto surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
        auto surfaceData = cairo_image_surface_get_data(surface);
        auto surfaceWidth = cairo_image_surface_get_width(surface);
        auto surfaceStride = cairo_image_surface_get_stride(surface);
        auto surfaceHeight = cairo_image_surface_get_height(surface);
        tjDecompress2(tj, (uint8_t*)(data), size, surfaceData, surfaceWidth, surfaceStride, surfaceHeight, TJPF_BGRX, 0);
        tjDestroy(tj);

        auto mimeData = (uint8_t*)std::malloc(size);
        std::memcpy(mimeData, data, size);

        cairo_surface_mark_dirty(surface);
        cairo_surface_set_mime_data(surface, CAIRO_MIME_TYPE_JPEG, mimeData, size, std::free, mimeData);
        return surface;
    }
#endif // PLUTOBOOK_HAS_TURBOJPEG
#ifdef PLUTOBOOK_HAS_WEBP
    if (size > 14 && std::memcmp(data, "RIFF", 4) == 0 && std::memcmp(data + 8, "WEBPVP", 6) == 0) {
        WebPDecoderConfig config;
        if (!WebPInitDecoderConfig(&config)) {
            plutobook_set_error_message("image decode error: WebPInitDecoderConfig failed");
            return nullptr;
        }

        if (WebPGetFeatures((const uint8_t*)(data), size, &config.input) != VP8_STATUS_OK) {
            plutobook_set_error_message("image decode error: WebPGetFeatures failed");
            return nullptr;
        }

        auto surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, config.input.width, config.input.height);
        auto surfaceData = cairo_image_surface_get_data(surface);
        auto surfaceWidth = cairo_image_surface_get_width(surface);
        auto surfaceHeight = cairo_image_surface_get_height(surface);
        auto surfaceStride = cairo_image_surface_get_stride(surface);

        config.output.colorspace = MODE_bgrA;
        config.output.u.RGBA.rgba = surfaceData;
        config.output.u.RGBA.stride = surfaceStride;
        config.output.u.RGBA.size = surfaceStride * surfaceHeight;
        config.output.width = surfaceWidth;
        config.output.height = surfaceHeight;
        config.output.is_external_memory = 1;
        if (WebPDecode((const uint8_t*)(data), size, &config) != VP8_STATUS_OK) {
            plutobook_set_error_message("image decode error: WebPDecode failed");
            return nullptr;
        }

        cairo_surface_mark_dirty(surface);
        return surface;
    }
#endif // PLUTOBOOK_HAS_WEBP

    int width, height, channels;
    auto imageData = stbi_load_from_memory((const stbi_uc*)(data), size, &width, &height, &channels, STBI_rgb_alpha);
    if (imageData == nullptr) {
        plutobook_set_error_message("image decode error: %s", stbi_failure_reason());
        return nullptr;
    }

    auto surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    auto surfaceData = cairo_image_surface_get_data(surface);
    auto surfaceWidth = cairo_image_surface_get_width(surface);
    auto surfaceHeight = cairo_image_surface_get_height(surface);
    auto surfaceStride = cairo_image_surface_get_stride(surface);
    for (int y = 0; y < surfaceHeight; y++) {
        uint32_t* src = (uint32_t*)(imageData + surfaceStride * y);
        uint32_t* dst = (uint32_t*)(surfaceData + surfaceStride * y);
        for (int x = 0; x < surfaceWidth; x++) {
            uint32_t a = (src[x] >> 24) & 0xFF;
            uint32_t b = (src[x] >> 16) & 0xFF;
            uint32_t g = (src[x] >> 8) & 0xFF;
            uint32_t r = (src[x] >> 0) & 0xFF;

            uint32_t pr = (r * a) / 255;
            uint32_t pg = (g * a) / 255;
            uint32_t pb = (b * a) / 255;
            dst[x] = (a << 24) | (pr << 16) | (pg << 8) | pb;
        }
    }

    stbi_image_free(imageData);
    cairo_surface_mark_dirty(surface);
    return surface;
}

class CairoGraphicsManager : public GraphicsManager {
public:
    ImageHandle createImage(const void* data, size_t size) override {
        const auto surface =
            decodeBitmapImage(static_cast<const char*>(data), size);
        if (surface == nullptr)
            return ImageHandle::Invalid;
        if (auto status = cairo_surface_status(surface)) {
            plutobook_set_error_message("image decode error: %s",
                                        cairo_status_to_string(status));
            return ImageHandle::Invalid;
        }
        return std::bit_cast<ImageHandle>(surface);
    }

    void destroyImage(ImageHandle handle) override {
        const auto surface = std::bit_cast<cairo_surface_t*>(handle);
        cairo_surface_destroy(surface);
    }

    Size getImageSize(ImageHandle handle) const override {
        const auto surface = std::bit_cast<cairo_surface_t*>(handle);
        Size size;
        size.w = cairo_image_surface_get_width(surface);
        size.h = cairo_image_surface_get_height(surface);
        return size;
    }
};

//DefaultGraphicsManager defaultGraphicsManager;
CairoGraphicsManager defaultGraphicsManager;
GraphicsManager* g_graphicsManager = &defaultGraphicsManager;

void setGraphicsManager(GraphicsManager& manager)
{
    g_graphicsManager = &manager;
}

GraphicsManager& graphicsManager()
{
    return *g_graphicsManager;
}

CairoGraphicsContext::CairoGraphicsContext(cairo_t* canvas)
    : m_canvas(cairo_reference(canvas))
{
}

CairoGraphicsContext::~CairoGraphicsContext()
{
    cairo_destroy(m_canvas);
}

void CairoGraphicsContext::setColor(const Color& color)
{
    auto red = color.red() / 255.0;
    auto green = color.green() / 255.0;
    auto blue = color.blue() / 255.0;
    auto alpha = color.alpha() / 255.0;
    cairo_set_source_rgba(m_canvas, red, green, blue, alpha);
}

void CairoGraphicsContext::setLinearGradient(const LinearGradientValues& values, const GradientInfo& info)
{
    auto pattern = cairo_pattern_create_linear(values.x1, values.y1, values.x2, values.y2);
    set_cairo_gradient(pattern, info);
    cairo_set_source(m_canvas, pattern);
    cairo_pattern_destroy(pattern);
}

void CairoGraphicsContext::setRadialGradient(const RadialGradientValues& values, const GradientInfo& info)
{
    auto pattern = cairo_pattern_create_radial(values.fx, values.fy, 0, values.cx, values.cy, values.r);
    set_cairo_gradient(pattern, info);
    cairo_set_source(m_canvas, pattern);
    cairo_pattern_destroy(pattern);
}

void CairoGraphicsContext::setPattern(cairo_surface_t* surface, const Transform& transform)
{
    auto pattern = cairo_pattern_create_for_surface(surface);
    auto matrix = to_cairo_matrix(transform);
    cairo_matrix_invert(&matrix);
    cairo_pattern_set_matrix(pattern, &matrix);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
    cairo_set_source(m_canvas, pattern);
    cairo_pattern_destroy(pattern);
}

void CairoGraphicsContext::translate(float tx, float ty)
{
    cairo_translate(m_canvas, tx, ty);
}

void CairoGraphicsContext::scale(float sx, float sy)
{
    cairo_scale(m_canvas, sx, sy);
}

void CairoGraphicsContext::rotate(float angle)
{
    cairo_rotate(m_canvas, deg2rad(angle));
}

Transform CairoGraphicsContext::getTransform() const
{
    cairo_matrix_t matrix;
    cairo_get_matrix(m_canvas, &matrix);
    return Transform(matrix.xx, matrix.yx, matrix.xy, matrix.yy, matrix.x0, matrix.y0);
}

void CairoGraphicsContext::addTransform(const Transform& transform)
{
    cairo_matrix_t matrix = to_cairo_matrix(transform);
    cairo_transform(m_canvas, &matrix);
}

void CairoGraphicsContext::setTransform(const Transform& transform)
{
    cairo_matrix_t matrix = to_cairo_matrix(transform);
    cairo_set_matrix(m_canvas, &matrix);
}

void CairoGraphicsContext::resetTransform()
{
    cairo_identity_matrix(m_canvas);
}

void CairoGraphicsContext::fillRect(const Rect& rect)
{
    cairo_new_path(m_canvas);
    cairo_rectangle(m_canvas, rect.x, rect.y, rect.w, rect.h);
    cairo_set_fill_rule(m_canvas, CAIRO_FILL_RULE_WINDING);
    cairo_fill(m_canvas);
}

void CairoGraphicsContext::fillRoundedRect(const RoundedRect& rrect)
{
    if(!rrect.isRounded()) {
        fillRect(rrect.rect());
        return;
    }

    Path path;
    path.addRoundedRect(rrect);
    fillPath(path, FillRule::NonZero);
}

void CairoGraphicsContext::fillPath(const Path& path, FillRule fillRule)
{
    cairo_new_path(m_canvas);
    set_cairo_path(m_canvas, path);
    cairo_set_fill_rule(m_canvas, to_cairo_fill_rule(fillRule));
    cairo_fill(m_canvas);
}

void CairoGraphicsContext::fillGlyphs(hb_font_t* font, const GlyphRef glyphs[], unsigned glyphCount)
{
    //TODO
}

void CairoGraphicsContext::fillImage(ImageHandle image, const Rect& dstRect,
    const Rect& srcRect)
{
    const auto surface = std::bit_cast<cairo_surface_t*>(image);
    auto xScale = srcRect.w / dstRect.w;
    auto yScale = srcRect.h / dstRect.h;
    cairo_matrix_t matrix = {xScale, 0, 0, yScale, srcRect.x, srcRect.y};

    auto pattern = cairo_pattern_create_for_surface(surface);
    cairo_pattern_set_matrix(pattern, &matrix);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_NONE);

    cairo_save(m_canvas);
    cairo_set_fill_rule(m_canvas, CAIRO_FILL_RULE_WINDING);
    cairo_translate(m_canvas, dstRect.x, dstRect.y);
    cairo_rectangle(m_canvas, 0, 0, dstRect.w, dstRect.h);
    cairo_set_source(m_canvas, pattern);
    cairo_fill(m_canvas);
    cairo_restore(m_canvas);
    cairo_pattern_destroy(pattern);
}

void CairoGraphicsContext::fillImagePattern(ImageHandle image, const Rect& destRect,
    const Size& size, const Size& scale,
    const Point& phase)
{
    const auto surface = std::bit_cast<cairo_surface_t*>(image);
    cairo_matrix_t matrix;
    cairo_matrix_init(&matrix, scale.w, 0, 0, scale.h, phase.x, phase.y);
    cairo_matrix_invert(&matrix);

    auto pattern = cairo_pattern_create_for_surface(surface);
    cairo_pattern_set_matrix(pattern, &matrix);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

    cairo_save(m_canvas);
    cairo_set_fill_rule(m_canvas, CAIRO_FILL_RULE_WINDING);
    cairo_rectangle(m_canvas, destRect.x, destRect.y, destRect.w, destRect.h);
    cairo_set_source(m_canvas, pattern);
    cairo_fill(m_canvas);
    cairo_restore(m_canvas);
    cairo_pattern_destroy(pattern);
}

void CairoGraphicsContext::outlineRect(const Rect& rect, float lineWidth)
{
    const auto inner = rect - RectOutsets(lineWidth);
    cairo_new_path(m_canvas);
    cairo_rectangle(m_canvas, rect.x, rect.y, rect.w, rect.h);
    cairo_rectangle(m_canvas, inner.x, inner.y, inner.w, inner.h);
    cairo_set_fill_rule(m_canvas, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_fill(m_canvas);
}

void CairoGraphicsContext::outlineRoundedRect(const RoundedRect& rrect, float lineWidth)
{
    if(!rrect.isRounded()) {
        outlineRect(rrect.rect(), lineWidth);
        return;
    }

    Path path;
    path.addRoundedRect(rrect);
    path.addRoundedRect(rrect - RectOutsets(lineWidth));
    fillPath(path, FillRule::EvenOdd);
}

void CairoGraphicsContext::strokePath(const Path& path, const StrokeData& strokeData)
{
    cairo_new_path(m_canvas);
    set_cairo_path(m_canvas, path);
    set_cairo_stroke_data(m_canvas, strokeData);
    cairo_stroke(m_canvas);
}

void CairoGraphicsContext::clipRect(const Rect& rect, FillRule clipRule)
{
    cairo_new_path(m_canvas);
    cairo_rectangle(m_canvas, rect.x, rect.y, rect.w, rect.h);
    cairo_set_fill_rule(m_canvas, to_cairo_fill_rule(clipRule));
    cairo_clip(m_canvas);
}

void CairoGraphicsContext::clipRoundedRect(const RoundedRect& rrect, FillRule clipRule)
{
    if(!rrect.isRounded()) {
        clipRect(rrect.rect(), clipRule);
        return;
    }

    Path path;
    path.addRoundedRect(rrect);
    clipPath(path, clipRule);
}

void CairoGraphicsContext::clipPath(const Path& path, FillRule clipRule)
{
    cairo_new_path(m_canvas);
    set_cairo_path(m_canvas, path);
    cairo_set_fill_rule(m_canvas, to_cairo_fill_rule(clipRule));
    cairo_clip(m_canvas);
}

void CairoGraphicsContext::clipOutRect(const Rect& rect)
{
    double x1, y1, x2, y2;
    cairo_clip_extents(m_canvas, &x1, &y1, &x2, &y2);
    cairo_new_path(m_canvas);
    cairo_rectangle(m_canvas, x1, y1, x2 - x1, y2 - y1);
    cairo_rectangle(m_canvas, rect.x, rect.y, rect.w, rect.h);
    cairo_set_fill_rule(m_canvas, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_clip(m_canvas);
}

void CairoGraphicsContext::clipOutRoundedRect(const RoundedRect& rrect)
{
    if(!rrect.isRounded()) {
        clipOutRect(rrect.rect());
        return;
    }

    Path path;
    path.addRoundedRect(rrect);
    clipOutPath(path);
}

void CairoGraphicsContext::clipOutPath(const Path& path)
{
    double x1, y1, x2, y2;
    cairo_clip_extents(m_canvas, &x1, &y1, &x2, &y2);
    cairo_new_path(m_canvas);
    cairo_rectangle(m_canvas, x1, y1, x2 - x1, y2 - y1);
    set_cairo_path(m_canvas, path);
    cairo_set_fill_rule(m_canvas, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_clip(m_canvas);
}

void CairoGraphicsContext::save()
{
    cairo_save(m_canvas);
}

void CairoGraphicsContext::restore()
{
    cairo_restore(m_canvas);
}

void CairoGraphicsContext::pushGroup()
{
    cairo_push_group(m_canvas);
}

void CairoGraphicsContext::popGroup(float opacity, BlendMode blendMode)
{
    cairo_pop_group_to_source(m_canvas);
    cairo_set_operator(m_canvas, to_cairo_operator(blendMode));
    cairo_paint_with_alpha(m_canvas, opacity);
    cairo_set_operator(m_canvas, CAIRO_OPERATOR_OVER);
}

void CairoGraphicsContext::applyMask(const ImageBuffer& maskImage)
{
    cairo_matrix_t matrix;
    cairo_get_matrix(m_canvas, &matrix);
    cairo_identity_matrix(m_canvas);
    cairo_set_source_surface(m_canvas, maskImage.surface(), maskImage.x(), maskImage.y());
    cairo_set_operator(m_canvas, CAIRO_OPERATOR_DEST_IN);
    cairo_paint(m_canvas);
    cairo_set_operator(m_canvas, CAIRO_OPERATOR_OVER);
    cairo_set_matrix(m_canvas, &matrix);
}

static void append_attribute(OutputStream& output, const std::string_view& name, const std::string_view& value)
{
    output << name << "='";
    for(auto cc : value) {
        if(cc == '\\' || cc == '\'')
            output << '\\';
        output << cc;
    }

    output << '\'';
}

void CairoGraphicsContext::addLinkAnnotation(const std::string_view& dest, const std::string_view& uri, const Rect& rect)
{
    if(dest.empty() && uri.empty())
        return;
    double x = rect.x, y = rect.y;
    double w = rect.w, h = rect.h;
    cairo_user_to_device(m_canvas, &x, &y);
    cairo_user_to_device_distance(m_canvas, &w, &h);

    StringOutputStream ss;
    ss << "rect=[" << x << ' '  << y << ' '  << w << ' '  << h << ']';
    if(!dest.empty()) {
        append_attribute(ss, "dest", dest);
    } else if(!uri.empty()) {
        append_attribute(ss, "uri", uri);
    }

    cairo_tag_begin(m_canvas, CAIRO_TAG_LINK, ss.str().data());
    cairo_tag_end(m_canvas, CAIRO_TAG_LINK);
}

void CairoGraphicsContext::addLinkDestination(const std::string_view& name, const Point& location)
{
    if(name.empty())
        return;
    double x = location.x;
    double y = location.y;
    cairo_user_to_device(m_canvas, &x, &y);

    StringOutputStream ss;
    append_attribute(ss, "name", name);
    ss << " x=" << x << " y=" << y;

    cairo_tag_begin(m_canvas, CAIRO_TAG_DEST, ss.str().data());
    cairo_tag_end(m_canvas, CAIRO_TAG_DEST);
}

std::unique_ptr<ImageBuffer> ImageBuffer::create(const Rect& rect)
{
    return create(rect.x, rect.y, rect.w, rect.h);
}

std::unique_ptr<ImageBuffer> ImageBuffer::create(float x, float y, float width, float height)
{
    if(width <= 0.f || height <= 0.f)
        return std::unique_ptr<ImageBuffer>(new ImageBuffer(0, 0, 1, 1));
    auto l = static_cast<int>(std::floor(x));
    auto t = static_cast<int>(std::floor(y));
    auto r = static_cast<int>(std::ceil(x + width));
    auto b = static_cast<int>(std::ceil(y + height));
    return std::unique_ptr<ImageBuffer>(new ImageBuffer(l, t, r - l, b - t));
}

const int ImageBuffer::width() const
{
    return cairo_image_surface_get_width(m_surface);
}

const int ImageBuffer::height() const
{
    return cairo_image_surface_get_height(m_surface);
}

void ImageBuffer::convertToLuminanceMask()
{
    auto width = cairo_image_surface_get_width(m_surface);
    auto height = cairo_image_surface_get_height(m_surface);
    auto stride = cairo_image_surface_get_stride(m_surface);
    auto data = cairo_image_surface_get_data(m_surface);
    for(int y = 0; y < height; y++) {
        auto pixels = reinterpret_cast<uint32_t*>(data + stride * y);
        for(int x = 0; x < width; x++) {
            auto pixel = pixels[x];
            auto a = (pixel >> 24) & 0xFF;
            auto r = (pixel >> 16) & 0xFF;
            auto g = (pixel >> 8) & 0xFF;
            auto b = (pixel >> 0) & 0xFF;
            if(a) {
                r = (r * 255) / a;
                g = (g * 255) / a;
                b = (b * 255) / a;
            }

            auto l = (r * 0.2125 + g * 0.7154 + b * 0.0721);
            pixels[x] = static_cast<uint32_t>(l * (a / 255.0)) << 24;
        }
    }
}

ImageBuffer::~ImageBuffer()
{
    cairo_destroy(m_canvas);
    cairo_surface_destroy(m_surface);
}

ImageBuffer::ImageBuffer(int x, int y, int width, int height)
    : m_surface(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height))
    , m_canvas(cairo_create(m_surface))
    , m_x(x), m_y(y)
{
    cairo_translate(m_canvas, -x, -y);
}

} // namespace plutobook
