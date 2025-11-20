#ifndef PLUTOBOOK_SvgCONTAINERBOX_H
#define PLUTOBOOK_SvgCONTAINERBOX_H

#include "svg-box-model.h"

namespace plutobook {

class SvgContainerBox : public SvgBoxModel {
public:
    SvgContainerBox(SvgElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgContainerBox() const final { return true; }

    Rect fillBoundingBox() const override;
    Rect strokeBoundingBox() const override;
    void layout() override;

    void renderChildren(const SvgRenderState& state) const;

    const char* name() const override { return "SvgContainerBox"; }

private:
    mutable Rect m_fillBoundingBox = Rect::Invalid;
    mutable Rect m_strokeBoundingBox = Rect::Invalid;
};

template<>
struct is_a<SvgContainerBox> {
    static bool check(const Box& box) { return box.isSvgContainerBox(); }
};

class SvgHiddenContainerBox : public SvgContainerBox {
public:
    SvgHiddenContainerBox(SvgElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgHiddenContainerBox() const final { return true; }

    void render(const SvgRenderState& state) const override;

    const char* name() const override { return "SvgHiddenContainerBox"; }
};

template<>
struct is_a<SvgHiddenContainerBox> {
    static bool check(const Box& box) { return box.isSvgHiddenContainerBox(); }
};

class SvgTransformableContainerBox final : public SvgContainerBox {
public:
    SvgTransformableContainerBox(SvgGraphicsElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgTransformableContainerBox() const final { return true; }

    SvgGraphicsElement* element() const;
    Transform localTransform() const final { return m_localTransform; }
    void render(const SvgRenderState& state) const final;
    void layout() final;

    const char* name() const final { return "SvgTransformableContainerBox"; }

private:
    Transform m_localTransform;
};

inline SvgGraphicsElement* SvgTransformableContainerBox::element() const
{
    return static_cast<SvgGraphicsElement*>(node());
}

template<>
struct is_a<SvgTransformableContainerBox> {
    static bool check(const Box& box) { return box.isSvgTransformableContainerBox(); }
};

class SvgViewportContainerBox final : public SvgContainerBox {
public:
    SvgViewportContainerBox(SvgSvgElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgViewportContainerBox() const final { return true; }

    SvgSvgElement* element() const;
    Transform localTransform() const final { return m_localTransform; }
    void render(const SvgRenderState& state) const final;
    void layout() final;

    const char* name() const final { return "SvgViewportContainerBox"; }

private:
    Transform m_localTransform;
};

inline SvgSvgElement* SvgViewportContainerBox::element() const
{
    return static_cast<SvgSvgElement*>(node());
}

template<>
struct is_a<SvgViewportContainerBox> {
    static bool check(const Box& box) { return box.isSvgViewportContainerBox(); }
};

class SvgResourceContainerBox : public SvgHiddenContainerBox {
public:
    SvgResourceContainerBox(SvgElement* element, const RefPtr<BoxStyle>& style);

    bool isSvgResourceContainerBox() const final { return true; }

    const char* name() const override { return "SvgResourceContainerBox"; }
};

template<>
struct is_a<SvgResourceContainerBox> {
    static bool check(const Box& box) { return box.isSvgResourceContainerBox(); }
};

} // namespace plutobook

#endif // PLUTOBOOK_SvgCONTAINERBOX_H
