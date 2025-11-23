#pragma once

#include "svg-box-model.h"

namespace plutobook {
    class SvgMarkerPosition {
    public:
        SvgMarkerPosition(const SvgResourceMarkerBox* marker,
                          const Point& origin, float angle)
            : m_marker(marker), m_origin(origin), m_angle(angle) {}

        const SvgResourceMarkerBox* marker() const { return m_marker; }
        const Point& origin() const { return m_origin; }
        float angle() const { return m_angle; }

        Rect markerBoundingBox(float strokeWidth) const;
        void renderMarker(const SvgRenderState& state, float strokeWidth) const;

    private:
        const SvgResourceMarkerBox* m_marker;
        Point m_origin;
        float m_angle;
    };

    using SvgMarkerPositionList = std::vector<SvgMarkerPosition>;

    class SvgGeometryBox : public SvgBoxModel {
    public:
        SvgGeometryBox(ClassKind type, SvgGeometryElement* element,
                       const RefPtr<BoxStyle>& style);

        virtual const Path& path() const = 0;

        SvgGeometryElement* element() const;
        Transform localTransform() const override {
            return element()->transform();
        }
        Rect fillBoundingBox() const override;
        Rect strokeBoundingBox() const override;

        void render(const SvgRenderState& state) const override;
        void layout() override;
        void build() override;

        const char* name() const override { return "SvgGeometryBox"; }

    protected:
        void updateMarkerPositions();

        SvgPaintServer m_fill;
        SvgPaintServer m_stroke;
        SvgMarkerPositionList m_markerPositions;

        const SvgResourceMarkerBox* m_markerStart = nullptr;
        const SvgResourceMarkerBox* m_markerMid = nullptr;
        const SvgResourceMarkerBox* m_markerEnd = nullptr;

        mutable Rect m_fillBoundingBox = Rect::Invalid;
        mutable Rect m_strokeBoundingBox = Rect::Invalid;
    };

    extern template bool is<SvgGeometryBox>(const Box& value);

    inline SvgGeometryElement* SvgGeometryBox::element() const {
        return static_cast<SvgGeometryElement*>(node());
    }

    class SvgPathBox final : public SvgGeometryBox {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgPath;

        SvgPathBox(SvgPathElement* element, const RefPtr<BoxStyle>& style);

        SvgPathElement* element() const;
        const Path& path() const final { return element()->path(); }

        const char* name() const final { return "SvgPathBox"; }
    };

    inline SvgPathElement* SvgPathBox::element() const {
        return static_cast<SvgPathElement*>(node());
    }

    class SvgShapeBox final : public SvgGeometryBox {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgShape;

        SvgShapeBox(SvgShapeElement* element, const RefPtr<BoxStyle>& style);

        SvgShapeElement* element() const;
        const Path& path() const final { return m_path; }
        void layout() final;

        const char* name() const final { return "SvgShapeBox"; }

    private:
        Path m_path;
    };

    inline SvgShapeElement* SvgShapeBox::element() const {
        return static_cast<SvgShapeElement*>(node());
    }
} // namespace plutobook