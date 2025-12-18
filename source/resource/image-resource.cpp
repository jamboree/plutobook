#include "image-resource.h"
#include "text-resource.h"
#include "svg-document.h"
#include "graphics-context.h"
#include "string-utils.h"
#include "ident-table.h"

#include "plutobook.hpp"

#include <cstring>
#include <cmath>

namespace plutobook {

RefPtr<ImageResource> ImageResource::create(Document* document, const Url& url)
{
    auto resource = ResourceLoader::loadUrl(url, document->customResourceFetcher());
    if(resource.isNull())
        return nullptr;
    auto image = decode(resource.content(), resource.contentLength(), resource.mimeType(), resource.textEncoding(), url.base(), document->customResourceFetcher());
    if(image == nullptr) {
        plutobook_set_error_message("Unable to load image '%s': %s", url.value().data(), plutobook_get_error_message());
        return nullptr;
    }

    return adoptPtr(new ImageResource(std::move(image)));
}

RefPtr<Image> ImageResource::decode(const char* data, size_t size, const std::string_view& mimeType, const std::string_view& textEncoding, const std::string_view& baseUrl, ResourceFetcher* fetcher)
{
    if(iequals(mimeType, "image/svg+xml"))
        return SvgImage::create(TextResource::decode(data, size, mimeType, textEncoding), baseUrl, fetcher);
    return BitmapImage::create(data, size);
}

bool ImageResource::supportsMimeType(const std::string_view& mimeType)
{
    char buffer[16];
    if (mimeType.length() > sizeof(buffer))
        return false;
    auto lower = toLower(mimeType, buffer);
    if (!lower.starts_with("image/"))
        return false;
    lower.remove_prefix(6);
    return makeIdentSet({
                            "jpeg",
                            "png",
                            "svg+xml",
                            "gif",
                            "bmp",
#ifdef PLUTOBOOK_HAS_WEBP
                            "webp",
#endif
                        })
        .contains(lower);
}

RefPtr<BitmapImage> BitmapImage::create(const char* data, size_t size)
{
    Size extent;
    auto image = graphicsManager().createImage(data, size, extent);
    if (image == ImageHandle::Invalid) {
        return nullptr;
    }
    return adoptPtr(new BitmapImage(image, extent));
}

void Image::drawTiled(GraphicsContext& context, const Rect& destRect, const Rect& tileRect)
{
    const Size imageSize(size());
    if(imageSize.isEmpty() || destRect.isEmpty() || tileRect.isEmpty()) {
        return;
    }

    const Size scale(tileRect.w / imageSize.w, tileRect.h / imageSize.h);
    const Point phase = {
        destRect.x + std::fmod(std::fmod(-tileRect.x, tileRect.w) - tileRect.w, tileRect.w),
        destRect.y + std::fmod(std::fmod(-tileRect.y, tileRect.h) - tileRect.h, tileRect.h)
    };

    const Rect oneTileRect(phase, tileRect.size());
    if(!oneTileRect.contains(destRect)) {
        drawPattern(context, destRect, imageSize, scale, phase);
    } else {
        const Rect srcRect = {
            (destRect.x - oneTileRect.x) / scale.w,
            (destRect.y - oneTileRect.y) / scale.h,
            (destRect.w / scale.w),
            (destRect.h / scale.h)
        };

        draw(context, destRect, srcRect);
    }
}

void BitmapImage::draw(GraphicsContext& context, const Rect& dstRect, const Rect& srcRect)
{
    if(dstRect.isEmpty() || srcRect.isEmpty()) {
        return;
    }
    context.fillImage(m_image, dstRect, srcRect);
}

void BitmapImage::drawPattern(GraphicsContext& context, const Rect& destRect, const Size& size, const Size& scale, const Point& phase)
{
    assert(!destRect.isEmpty() && !size.isEmpty() && !scale.isEmpty());
    context.fillImagePattern(m_image, destRect, size, scale, phase);
}

void BitmapImage::computeIntrinsicDimensions(float& intrinsicWidth, float& intrinsicHeight, double& intrinsicRatio)
{
    intrinsicWidth = m_intrinsicSize.w;
    intrinsicHeight = m_intrinsicSize.h;
    if(intrinsicHeight > 0) {
        intrinsicRatio = intrinsicWidth / intrinsicHeight;
    } else {
        intrinsicRatio = 0;
    }
}

BitmapImage::~BitmapImage()
{
    graphicsManager().destroyImage(m_image);
}

BitmapImage::BitmapImage(ImageHandle image, const Size& size)
    : Image(classKind), m_image(image), m_intrinsicSize(size)
{
}

RefPtr<SvgImage> SvgImage::create(const std::string_view& content, const std::string_view& baseUrl, ResourceFetcher* fetcher)
{
    auto document = SvgDocument::create(nullptr, fetcher, ResourceLoader::completeUrl(baseUrl));
    if(!document->parse(content))
        return nullptr;
    if(!document->rootElement()->isOfType(svgNs, svgTag)) {
        plutobook_set_error_message("invalid Svg image: root element must be <svg> in the \"http://www.w3.org/2000/svg\" namespace");
        return nullptr;
    }

    return adoptPtr(new SvgImage(std::move(document)));
}

void SvgImage::draw(GraphicsContext& context, const Rect& dstRect, const Rect& srcRect)
{
    if(dstRect.isEmpty() || srcRect.isEmpty()) {
        return;
    }

    auto xScale = dstRect.w / srcRect.w;
    auto yScale = dstRect.h / srcRect.h;

    auto xOffset = dstRect.x - (srcRect.x * xScale);
    auto yOffset = dstRect.y - (srcRect.y * yScale);

    context.save();
    context.clipRect(dstRect);
    context.translate(xOffset, yOffset);
    context.scale(xScale, yScale);
    m_document->render(context, srcRect);
    context.restore();
}

void SvgImage::drawPattern(GraphicsContext& context, const Rect& destRect, const Size& size, const Size& scale, const Point& phase)
{
    assert(!destRect.isEmpty() && !size.isEmpty() && !scale.isEmpty());

#if 0
    cairo_matrix_t pattern_matrix;
    cairo_matrix_init(&pattern_matrix, 1, 0, 0, 1, -phase.x, -phase.y);

    cairo_rectangle_t pattern_rectangle = {0, 0, size.w * scale.w, size.h * scale.h};
    auto pattern_surface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, &pattern_rectangle);
    auto pattern_canvas = cairo_create(pattern_surface);

    auto pattern = cairo_pattern_create_for_surface(pattern_surface);
    cairo_pattern_set_matrix(pattern, &pattern_matrix);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

    CairoGraphicsContext pattern_context(pattern_canvas);
    m_document->render(pattern_context, Rect::Infinite);

    cairo_t* canvas;
    if (const auto gfx = dynamic_cast<CairoGraphicsContext*>(&context))
        canvas = gfx->canvas();
    else
        return;
    cairo_save(canvas);
    cairo_set_fill_rule(canvas, CAIRO_FILL_RULE_WINDING);
    cairo_rectangle(canvas, destRect.x, destRect.y, destRect.w, destRect.h);
    cairo_set_source(canvas, pattern);
    cairo_fill(canvas);
    cairo_restore(canvas);

    cairo_pattern_destroy(pattern);
    cairo_destroy(pattern_canvas);
    cairo_surface_destroy(pattern_surface);
#endif // 0
}

static SvgSvgElement* toSvgRootElement(Element* element)
{
    assert(element && element->isOfType(svgNs, svgTag));
    return static_cast<SvgSvgElement*>(element);
}

void SvgImage::computeIntrinsicDimensions(float& intrinsicWidth, float& intrinsicHeight, double& intrinsicRatio)
{
    auto rootElement = toSvgRootElement(m_document->rootElement());
    rootElement->computeIntrinsicDimensions(intrinsicWidth, intrinsicHeight, intrinsicRatio);
}

void SvgImage::setContainerSize(const Size& size)
{
    m_containerSize = size;
    if(m_document->setContainerSize(size.w, size.h)) {
        m_document->layout();
    }
}

Size SvgImage::intrinsicSize() const
{
    float intrinsicWidth = 0.f;
    float intrinsicHeight = 0.f;
    double intrinsicRatio = 0.0;

    auto rootElement = toSvgRootElement(m_document->rootElement());
    rootElement->computeIntrinsicDimensions(intrinsicWidth, intrinsicHeight, intrinsicRatio);
    if(intrinsicRatio && (!intrinsicWidth || !intrinsicHeight)) {
        if(!intrinsicWidth && intrinsicHeight)
            intrinsicWidth = intrinsicHeight * intrinsicRatio;
        else if(intrinsicWidth && !intrinsicHeight) {
            intrinsicHeight = intrinsicWidth / intrinsicRatio;
        }
    }

    if(intrinsicWidth > 0 && intrinsicHeight > 0) {
        return Size(intrinsicWidth, intrinsicHeight);
    }

    const auto& viewBoxRect = rootElement->viewBox();
    if(viewBoxRect.isValid())
        return viewBoxRect.size();
    return Size(300, 150);
}

Size SvgImage::size() const
{
    return m_containerSize;
}

SvgImage::SvgImage(std::unique_ptr<SvgDocument> document)
    : Image(classKind), m_document(std::move(document))
{
    m_document->build();
}

} // namespace plutobook
