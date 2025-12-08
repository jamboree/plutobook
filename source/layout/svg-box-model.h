#pragma once

#include "box.h"
#include "svg-document.h"
#include "graphics-context.h"

namespace plutobook {
    enum class SvgRenderMode { Painting, Clipping };

    class SvgBlendInfo {
    public:
        SvgBlendInfo(const SvgResourceClipperBox* clipper,
                     const SvgResourceMaskerBox* masker, const BoxStyle* style)
            : SvgBlendInfo(clipper, masker, style->opacity(),
                           style->blendMode()) {}

        SvgBlendInfo(const SvgResourceClipperBox* clipper,
                     const SvgResourceMaskerBox* masker, float opacity,
                     BlendMode blendMode)
            : m_clipper(clipper), m_masker(masker), m_opacity(opacity),
              m_blendMode(blendMode) {}

        const SvgResourceClipperBox* clipper() const { return m_clipper; }
        const SvgResourceMaskerBox* masker() const { return m_masker; }
        const float opacity() const { return m_opacity; }
        const BlendMode blendMode() const { return m_blendMode; }

        bool requiresCompositing(SvgRenderMode mode) const;

    private:
        const SvgResourceClipperBox* m_clipper;
        const SvgResourceMaskerBox* m_masker;
        const float m_opacity;
        const BlendMode m_blendMode;
    };

    class SvgRenderState {
    public:
        SvgRenderState(const SvgBlendInfo& info, const Box* box,
                       const SvgRenderState& parent,
                       const Transform& localTransform);
        SvgRenderState(const SvgBlendInfo& info, const Box* box,
                       const SvgRenderState& parent, SvgRenderMode mode,
                       GraphicsContext& context);
        SvgRenderState(const SvgBlendInfo& info, const Box* box,
                       const SvgRenderState* parent, SvgRenderMode mode,
                       GraphicsContext& context,
                       const Transform& currentTransform);

        ~SvgRenderState();

        const Box* box() const { return m_box; }
        const SvgRenderState* parent() const { return m_parent; }
        const SvgBlendInfo& info() const { return m_info; }
        GraphicsContext& context() const { return m_context; }
        const Transform& currentTransform() const { return m_currentTransform; }
        const SvgRenderMode mode() const { return m_mode; }

        Rect fillBoundingBox() const { return m_box->fillBoundingBox(); }
        Rect paintBoundingBox() const { return m_box->paintBoundingBox(); }

        bool hasCycleReference(const Box* box) const;

    private:
        const Box* m_box;
        const SvgRenderState* m_parent;
        const SvgBlendInfo& m_info;
        GraphicsContext& m_context;
        const Transform m_currentTransform;
        const SvgRenderMode m_mode;
        const bool m_requiresCompositing;
    };

    class SvgPaintServer {
    public:
        SvgPaintServer() = default;
        SvgPaintServer(const SvgResourcePaintServerBox* painter,
                       const Color& color, float opacity)
            : m_painter(painter), m_color(color), m_opacity(opacity) {}

        bool isRenderable() const {
            return m_opacity > 0.f && (m_painter || m_color.alpha() > 0);
        }

        const SvgResourcePaintServerBox* painter() const { return m_painter; }
        const Color& color() const { return m_color; }
        float opacity() const { return m_opacity; }

        void applyPaint(const SvgRenderState& state) const;

    private:
        const SvgResourcePaintServerBox* m_painter{nullptr};
        Color m_color;
        float m_opacity{0.f};
    };

    class SvgBoxModel : public Box {
    public:
        SvgBoxModel(ClassKind type, SvgElement* element,
                    const RefPtr<BoxStyle>& style);

        SvgElement* element() const;
        Rect paintBoundingBox() const override;
        virtual void render(const SvgRenderState& state) const = 0;
        virtual void layout();
        void build() override;

        const SvgResourceClipperBox* clipper() const { return m_clipper; }
        const SvgResourceMaskerBox* masker() const { return m_masker; }

    protected:
        mutable Rect m_paintBoundingBox = Rect::Invalid;
        const SvgResourceClipperBox* m_clipper = nullptr;
        const SvgResourceMaskerBox* m_masker = nullptr;
    };

    extern template bool is<SvgBoxModel>(const Box& value);

    inline SvgElement* SvgBoxModel::element() const {
        return static_cast<SvgElement*>(node());
    }
} // namespace plutobook