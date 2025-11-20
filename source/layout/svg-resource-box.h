#ifndef PLUTOBOOK_SvgRESOURCEBOX_H
#define PLUTOBOOK_SvgRESOURCEBOX_H

#include "svg-container-box.h"

namespace plutobook {

class SvgResourceMarkerBox final : public SvgResourceContainerBox {
public:
    SvgResourceMarkerBox(SvgMarkerElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgResourceMarkerBox() const final { return true; }

    Point refPoint() const;
    Size markerSize() const;

    SvgMarkerElement* element() const;
    Transform localTransform() const final { return m_localTransform; }
    Transform markerTransform(const Point& origin, float angle, float strokeWidth) const;
    Rect markerBoundingBox(const Point& origin, float angle, float strokeWidth) const;
    void renderMarker(const SvgRenderState& state, const Point& origin, float angle, float strokeWidth) const;
    void layout() final;

    const char* name() const final { return "SvgResourceMarkerBox"; }

private:
    Transform m_localTransform;
};

template<>
struct is_a<SvgResourceMarkerBox> {
    static bool check(const Box& box) { return box.isSvgResourceMarkerBox(); }
};

inline SvgMarkerElement* SvgResourceMarkerBox::element() const
{
    return static_cast<SvgMarkerElement*>(node());
}

class SvgResourceClipperBox final : public SvgResourceContainerBox {
public:
    SvgResourceClipperBox(SvgClipPathElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgResourceClipperBox() const final { return true; }
    bool requiresMasking() const;

    SvgClipPathElement* element() const;
    Rect clipBoundingBox(const Box* box) const;
    void applyClipPath(const SvgRenderState& state) const;
    void applyClipMask(const SvgRenderState& state) const;

    const char* name() const final { return "SvgResourceClipperBox"; }
};

template<>
struct is_a<SvgResourceClipperBox> {
    static bool check(const Box& box) { return box.isSvgResourceClipperBox(); }
};

inline SvgClipPathElement* SvgResourceClipperBox::element() const
{
    return static_cast<SvgClipPathElement*>(node());
}

class SvgResourceMaskerBox final : public SvgResourceContainerBox {
public:
    SvgResourceMaskerBox(SvgMaskElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgResourceMaskerBox() const final { return true; }

    SvgMaskElement* element() const;
    Rect maskBoundingBox(const Box* box) const;
    void applyMask(const SvgRenderState& state) const;

    const char* name() const final { return "SvgResourceMaskerBox"; }
};

template<>
struct is_a<SvgResourceMaskerBox> {
    static bool check(const Box& box) { return box.isSvgResourceMaskerBox(); }
};

inline SvgMaskElement* SvgResourceMaskerBox::element() const
{
    return static_cast<SvgMaskElement*>(node());
}

class SvgResourcePaintServerBox : public SvgResourceContainerBox {
public:
    SvgResourcePaintServerBox(SvgElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgResourcePaintServerBox() const final { return true; }

    virtual void applyPaint(const SvgRenderState& state, float opacity) const = 0;

    const char* name() const override { return "SvgResourcePaintServerBox"; }
};

template<>
struct is_a<SvgResourcePaintServerBox> {
    static bool check(const Box& box) { return box.isSvgResourcePaintServerBox(); }
};

class SvgResourcePatternBox final : public SvgResourcePaintServerBox {
public:
    SvgResourcePatternBox(SvgPatternElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgResourcePatternBox() const final { return true; }

    SvgPatternElement* element() const;
    void applyPaint(const SvgRenderState& state, float opacity) const final;
    void build() final;

    const char* name() const final { return "SvgResourcePatternBox"; }

private:
    SvgPatternAttributes m_attributes;
};

template<>
struct is_a<SvgResourcePatternBox> {
    static bool check(const Box& box) { return box.isSvgResourcePatternBox(); }
};

inline SvgPatternElement* SvgResourcePatternBox::element() const
{
    return static_cast<SvgPatternElement*>(node());
}

class SvgGradientStopBox final : public Box {
public:
    SvgGradientStopBox(SvgStopElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgGradientStopBox() const final { return true; }
    SvgStopElement* element() const;

    const char* name() const final { return "SvgGradientStopBox"; }
};

template<>
struct is_a<SvgGradientStopBox> {
    static bool check(const Box& box) { return box.isSvgGradientStopBox(); }
};

inline SvgStopElement* SvgGradientStopBox::element() const
{
    return static_cast<SvgStopElement*>(node());
}

class SvgResourceGradientBox : public SvgResourcePaintServerBox {
public:
    SvgResourceGradientBox(SvgGradientElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgResourceGradientBox() const final { return true; }
    SvgGradientElement* element() const;

    const char* name() const override { return "SvgResourceGradientBox"; }
};

template<>
struct is_a<SvgResourceGradientBox> {
    static bool check(const Box& box) { return box.isSvgResourceGradientBox(); }
};

inline SvgGradientElement* SvgResourceGradientBox::element() const
{
    return static_cast<SvgGradientElement*>(node());
}

class SvgResourceLinearGradientBox final : public SvgResourceGradientBox {
public:
    SvgResourceLinearGradientBox(SvgLinearGradientElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgResourceLinearGradientBox() const final { return true; }
    SvgLinearGradientElement* element() const;
    void applyPaint(const SvgRenderState& state, float opacity) const final;
    void build() final;

    const char* name() const final { return "SvgResourceLinearGradientBox"; }

private:
    SvgLinearGradientAttributes m_attributes;
};

template<>
struct is_a<SvgResourceLinearGradientBox> {
    static bool check(const Box& box) { return box.isSvgResourceLinearGradientBox(); }
};

inline SvgLinearGradientElement* SvgResourceLinearGradientBox::element() const
{
    return static_cast<SvgLinearGradientElement*>(node());
}

class SvgResourceRadialGradientBox final : public SvgResourceGradientBox {
public:
    SvgResourceRadialGradientBox(SvgRadialGradientElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgResourceRadialGradientBox() const final { return true; }

    SvgRadialGradientElement* element() const;
    void applyPaint(const SvgRenderState& state, float opacity) const final;
    void build() final;

    const char* name() const final { return "SvgResourceRadialGradientBox"; }

private:
    SvgRadialGradientAttributes m_attributes;
};

template<>
struct is_a<SvgResourceRadialGradientBox> {
    static bool check(const Box& box) { return box.isSvgResourceRadialGradientBox(); }
};

inline SvgRadialGradientElement* SvgResourceRadialGradientBox::element() const
{
    return static_cast<SvgRadialGradientElement*>(node());
}

} // namespace plutobook

#endif // PLUTOBOOK_SvgRESOURCEBOX_H
