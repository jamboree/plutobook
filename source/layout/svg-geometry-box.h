#ifndef PLUTOBOOK_SvgGEOMETRYBOX_H
#define PLUTOBOOK_SvgGEOMETRYBOX_H

#include "svg-box-model.h"

namespace plutobook {

class SvgMarkerPosition {
public:
    SvgMarkerPosition(const SvgResourceMarkerBox* marker, const Point& origin, float angle)
        : m_marker(marker), m_origin(origin), m_angle(angle)
    {}

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
    SvgGeometryBox(SvgGeometryElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgGeometryBox() const final { return true; }

    virtual const Path& path() const = 0;

    SvgGeometryElement* element() const;
    Transform localTransform() const override { return element()->transform(); }
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

template<>
struct is_a<SvgGeometryBox> {
    static bool check(const Box& box) { return box.isSvgGeometryBox(); }
};

inline SvgGeometryElement* SvgGeometryBox::element() const
{
    return static_cast<SvgGeometryElement*>(node());
}

class SvgPathBox final : public SvgGeometryBox {
public:
    SvgPathBox(SvgPathElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgPathBox() const final { return true; }

    SvgPathElement* element() const;
    const Path& path() const final { return element()->path(); }

    const char* name() const final { return "SvgPathBox"; }
};

template<>
struct is_a<SvgPathBox> {
    static bool check(const Box& box) { return box.isSvgPathBox(); }
};

inline SvgPathElement* SvgPathBox::element() const
{
    return static_cast<SvgPathElement*>(node());
}

class SvgShapeBox final : public SvgGeometryBox {
public:
    SvgShapeBox(SvgShapeElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgShapeBox() const final { return true; }

    SvgShapeElement* element() const;
    const Path& path() const final { return m_path; }
    void layout() final;

    const char* name() const final { return "SvgShapeBox"; }

private:
    Path m_path;
};

template<>
struct is_a<SvgShapeBox> {
    static bool check(const Box& box) { return box.isSvgShapeBox(); }
};

inline SvgShapeElement* SvgShapeBox::element() const
{
    return static_cast<SvgShapeElement*>(node());
}

} // namespace plutobook

#endif // PLUTOBOOK_SvgGEOMETRYBOX_H
