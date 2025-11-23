#pragma once

#include "svg-container-box.h"

namespace plutobook {
    class SvgResourceMarkerBox final : public SvgResourceContainerBox {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgResourceMarker;

        SvgResourceMarkerBox(SvgMarkerElement* element,
                             const RefPtr<BoxStyle>& style);

        Point refPoint() const;
        Size markerSize() const;

        SvgMarkerElement* element() const;
        Transform localTransform() const final { return m_localTransform; }
        Transform markerTransform(const Point& origin, float angle,
                                  float strokeWidth) const;
        Rect markerBoundingBox(const Point& origin, float angle,
                               float strokeWidth) const;
        void renderMarker(const SvgRenderState& state, const Point& origin,
                          float angle, float strokeWidth) const;
        void layout() final;

        const char* name() const final { return "SvgResourceMarkerBox"; }

    private:
        Transform m_localTransform;
    };

    inline SvgMarkerElement* SvgResourceMarkerBox::element() const {
        return static_cast<SvgMarkerElement*>(node());
    }

    class SvgResourceClipperBox final : public SvgResourceContainerBox {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgResourceClipper;

        SvgResourceClipperBox(SvgClipPathElement* element,
                              const RefPtr<BoxStyle>& style);

        bool requiresMasking() const;

        SvgClipPathElement* element() const;
        Rect clipBoundingBox(const Box* box) const;
        void applyClipPath(const SvgRenderState& state) const;
        void applyClipMask(const SvgRenderState& state) const;

        const char* name() const final { return "SvgResourceClipperBox"; }
    };

    inline SvgClipPathElement* SvgResourceClipperBox::element() const {
        return static_cast<SvgClipPathElement*>(node());
    }

    class SvgResourceMaskerBox final : public SvgResourceContainerBox {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgResourceMasker;

        SvgResourceMaskerBox(SvgMaskElement* element,
                             const RefPtr<BoxStyle>& style);

        SvgMaskElement* element() const;
        Rect maskBoundingBox(const Box* box) const;
        void applyMask(const SvgRenderState& state) const;

        const char* name() const final { return "SvgResourceMaskerBox"; }
    };

    inline SvgMaskElement* SvgResourceMaskerBox::element() const {
        return static_cast<SvgMaskElement*>(node());
    }

    class SvgResourcePaintServerBox : public SvgResourceContainerBox {
    public:
        SvgResourcePaintServerBox(ClassKind type, SvgElement* element,
                                  const RefPtr<BoxStyle>& style);

        virtual void applyPaint(const SvgRenderState& state,
                                float opacity) const = 0;

        const char* name() const override {
            return "SvgResourcePaintServerBox";
        }
    };

    extern template bool is<SvgResourcePaintServerBox>(const Box& value);

    class SvgResourcePatternBox final : public SvgResourcePaintServerBox {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgResourcePattern;

        SvgResourcePatternBox(SvgPatternElement* element,
                              const RefPtr<BoxStyle>& style);

        SvgPatternElement* element() const;
        void applyPaint(const SvgRenderState& state, float opacity) const final;
        void build() final;

        const char* name() const final { return "SvgResourcePatternBox"; }

    private:
        SvgPatternAttributes m_attributes;
    };

    inline SvgPatternElement* SvgResourcePatternBox::element() const {
        return static_cast<SvgPatternElement*>(node());
    }

    class SvgGradientStopBox final : public Box {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgGradientStop;

        SvgGradientStopBox(SvgStopElement* element,
                           const RefPtr<BoxStyle>& style);

        SvgStopElement* element() const;

        const char* name() const final { return "SvgGradientStopBox"; }
    };

    inline SvgStopElement* SvgGradientStopBox::element() const {
        return static_cast<SvgStopElement*>(node());
    }

    class SvgResourceGradientBox : public SvgResourcePaintServerBox {
    public:
        SvgResourceGradientBox(ClassKind type, SvgGradientElement* element,
                               const RefPtr<BoxStyle>& style);

        SvgGradientElement* element() const;

        const char* name() const override { return "SvgResourceGradientBox"; }
    };

    extern template bool is<SvgResourceGradientBox>(const Box& value);

    inline SvgGradientElement* SvgResourceGradientBox::element() const {
        return static_cast<SvgGradientElement*>(node());
    }

    class SvgResourceLinearGradientBox final : public SvgResourceGradientBox {
    public:
        static constexpr ClassKind classKind =
            ClassKind::SvgResourceLinearGradient;

        SvgResourceLinearGradientBox(SvgLinearGradientElement* element,
                                     const RefPtr<BoxStyle>& style);

        SvgLinearGradientElement* element() const;
        void applyPaint(const SvgRenderState& state, float opacity) const final;
        void build() final;

        const char* name() const final {
            return "SvgResourceLinearGradientBox";
        }

    private:
        SvgLinearGradientAttributes m_attributes;
    };

    inline SvgLinearGradientElement*
    SvgResourceLinearGradientBox::element() const {
        return static_cast<SvgLinearGradientElement*>(node());
    }

    class SvgResourceRadialGradientBox final : public SvgResourceGradientBox {
    public:
        static constexpr ClassKind classKind =
            ClassKind::SvgResourceRadialGradient;

        SvgResourceRadialGradientBox(SvgRadialGradientElement* element,
                                     const RefPtr<BoxStyle>& style);

        SvgRadialGradientElement* element() const;
        void applyPaint(const SvgRenderState& state, float opacity) const final;
        void build() final;

        const char* name() const final {
            return "SvgResourceRadialGradientBox";
        }

    private:
        SvgRadialGradientAttributes m_attributes;
    };

    inline SvgRadialGradientElement*
    SvgResourceRadialGradientBox::element() const {
        return static_cast<SvgRadialGradientElement*>(node());
    }
} // namespace plutobook