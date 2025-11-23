#pragma once

#include "svg-box-model.h"

namespace plutobook {
    class SvgContainerBox : public SvgBoxModel {
    public:
        SvgContainerBox(ClassKind type, SvgElement* element,
                        const RefPtr<BoxStyle>& style);

        Rect fillBoundingBox() const override;
        Rect strokeBoundingBox() const override;
        void layout() override;

        void renderChildren(const SvgRenderState& state) const;

        const char* name() const override { return "SvgContainerBox"; }

    private:
        mutable Rect m_fillBoundingBox = Rect::Invalid;
        mutable Rect m_strokeBoundingBox = Rect::Invalid;
    };

    extern template bool is<SvgContainerBox>(const Box& value);

    class SvgHiddenContainerBox : public SvgContainerBox {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgHiddenContainer;

        SvgHiddenContainerBox(SvgElement* element,
                              const RefPtr<BoxStyle>& style)
            : SvgContainerBox(classKind, element, style) {}
        SvgHiddenContainerBox(ClassKind type, SvgElement* element,
                              const RefPtr<BoxStyle>& style);

        void render(const SvgRenderState& state) const override;

        const char* name() const override { return "SvgHiddenContainerBox"; }
    };

    extern template bool is<SvgHiddenContainerBox>(const Box& value);

    class SvgTransformableContainerBox final : public SvgContainerBox {
    public:
        static constexpr ClassKind classKind =
            ClassKind::SvgTransformableContainer;

        SvgTransformableContainerBox(SvgGraphicsElement* element,
                                     const RefPtr<BoxStyle>& style);

        SvgGraphicsElement* element() const;
        Transform localTransform() const final { return m_localTransform; }
        void render(const SvgRenderState& state) const final;
        void layout() final;

        const char* name() const final {
            return "SvgTransformableContainerBox";
        }

    private:
        Transform m_localTransform;
    };

    inline SvgGraphicsElement* SvgTransformableContainerBox::element() const {
        return static_cast<SvgGraphicsElement*>(node());
    }

    class SvgViewportContainerBox final : public SvgContainerBox {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgViewportContainer;

        SvgViewportContainerBox(SvgSvgElement* element,
                                const RefPtr<BoxStyle>& style);

        SvgSvgElement* element() const;
        Transform localTransform() const final { return m_localTransform; }
        void render(const SvgRenderState& state) const final;
        void layout() final;

        const char* name() const final { return "SvgViewportContainerBox"; }

    private:
        Transform m_localTransform;
    };

    inline SvgSvgElement* SvgViewportContainerBox::element() const {
        return static_cast<SvgSvgElement*>(node());
    }

    class SvgResourceContainerBox : public SvgHiddenContainerBox {
    public:
        SvgResourceContainerBox(ClassKind type, SvgElement* element,
                                const RefPtr<BoxStyle>& style);

        const char* name() const override { return "SvgResourceContainerBox"; }
    };

    extern template bool is<SvgResourceContainerBox>(const Box& value);
} // namespace plutobook