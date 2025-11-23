#pragma once

#include "replaced-box.h"
#include "svg-box-model.h"

namespace plutobook {
    class SvgSvgElement;

    class SvgRootBox final : public ReplacedBox {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgRoot;

        SvgRootBox(SvgSvgElement* element, const RefPtr<BoxStyle>& style);

        bool requiresLayer() const final;

        SvgSvgElement* element() const;

        Rect fillBoundingBox() const final;
        Rect strokeBoundingBox() const final;
        Rect paintBoundingBox() const final;

        float computePreferredReplacedWidth() const final;
        float computeReplacedWidth() const final;
        float computeReplacedHeight() const final;

        void
        computeIntrinsicRatioInformation(float& intrinsicWidth,
                                         float& intrinsicHeight,
                                         double& intrinsicRatio) const final;
        void paintReplaced(const PaintInfo& info, const Point& offset) final;
        void layout(FragmentBuilder* fragmentainer) final;
        void build() final;

        const char* name() const final { return "SvgRootBox"; }

    private:
        mutable Rect m_fillBoundingBox = Rect::Invalid;
        mutable Rect m_strokeBoundingBox = Rect::Invalid;
        mutable Rect m_paintBoundingBox = Rect::Invalid;

        const SvgResourceClipperBox* m_clipper = nullptr;
        const SvgResourceMaskerBox* m_masker = nullptr;
    };

    inline SvgSvgElement* SvgRootBox::element() const {
        return static_cast<SvgSvgElement*>(node());
    }

    class SvgImageElement;

    class SvgImageBox final : public SvgBoxModel {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgImage;

        SvgImageBox(SvgImageElement* element, const RefPtr<BoxStyle>& style);

        SvgImageElement* element() const;

        const RefPtr<Image>& image() const { return m_image; }
        Transform localTransform() const final {
            return element()->transform();
        }
        Rect fillBoundingBox() const final;
        Rect strokeBoundingBox() const final;

        void render(const SvgRenderState& state) const final;
        void layout() final;
        void build() final;

        const char* name() const final { return "SvgImageBox"; }

    private:
        RefPtr<Image> m_image;
        mutable Rect m_fillBoundingBox;
    };

    inline SvgImageElement* SvgImageBox::element() const {
        return static_cast<SvgImageElement*>(node());
    }
} // namespace plutobook