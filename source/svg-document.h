#pragma once

#include "xml-document.h"
#include "svg-property.h"

#include <memory>

namespace plutobook {
    using SvgPropertyMap =
        boost::unordered_flat_map<GlobalString, SvgProperty*>;

    class SvgResourceContainerBox;
    class SvgResourceClipperBox;
    class SvgResourceMaskerBox;

    class SvgElement : public Element {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgElement;

        SvgElement(Document* document, GlobalString tagName);

        void parseAttribute(GlobalString name,
                            const HeapString& value) override;
        void collectAttributeStyle(AttributeStyle& style) const override;
        void addProperty(GlobalString name, SvgProperty& value);
        SvgProperty* getProperty(GlobalString name) const;
        Size currentViewportSize() const;

        SvgResourceContainerBox*
        getResourceById(const std::string_view& id) const;
        SvgResourceClipperBox* getClipper(const std::string_view& id) const;
        SvgResourceMaskerBox* getMasker(const std::string_view& id) const;
        Box* createBox(const RefPtr<BoxStyle>& style) override {
            return nullptr;
        }

    private:
        SvgPropertyMap m_properties;
    };

    extern template bool is<SvgElement>(const Node& value);

    inline bool Node::isSvgRootNode() const {
        auto element = to<SvgElement>(this);
        if (element && element->tagName() == svgTag)
            return !is<SvgElement>(*m_parentNode);
        return false;
    }

    class SvgFitToViewBox {
    public:
        SvgFitToViewBox(SvgElement* element);

        const Rect& viewBox() const { return m_viewBox.value(); }
        const SvgPreserveAspectRatio& preserveAspectRatio() const {
            return m_preserveAspectRatio;
        }
        Transform viewBoxToViewTransform(const Size& viewportSize) const;
        Rect getClipRect(const Size& viewportSize) const;

    private:
        SvgRect m_viewBox;
        SvgPreserveAspectRatio m_preserveAspectRatio;
    };

    class SvgURIReference {
    public:
        SvgURIReference(SvgElement* element);

        const std::string& href() const { return m_href.value(); }
        SvgElement* getTargetElement(const Document* document) const;

    private:
        SvgString m_href;
    };

    class SvgResourcePaintServerBox;
    class SvgPaintServer;
    class StrokeData;
    class Paint;

    class SvgGraphicsElement : public SvgElement {
    public:
        SvgGraphicsElement(Document* document, GlobalString tagName);

        const Transform& transform() const { return m_transform.value(); }
        SvgResourcePaintServerBox* getPainter(const std::string_view& id) const;
        SvgPaintServer getPaintServer(const Paint& paint, float opacity) const;
        StrokeData getStrokeData(const BoxStyle* style) const;

    private:
        SvgTransform m_transform;
    };

    class SvgSvgElement final : public SvgGraphicsElement,
                                public SvgFitToViewBox {
    public:
        SvgSvgElement(Document* document);

        const SvgLength& x() const { return m_x; }
        const SvgLength& y() const { return m_y; }
        const SvgLength& width() const { return m_width; }
        const SvgLength& height() const { return m_height; }
        void computeIntrinsicDimensions(float& intrinsicWidth,
                                        float& intrinsicHeight,
                                        double& intrinsicRatio);
        void collectAttributeStyle(AttributeStyle& style) const final;
        Box* createBox(const RefPtr<BoxStyle>& style) final;

    private:
        SvgLength m_x;
        SvgLength m_y;
        SvgLength m_width;
        SvgLength m_height;
    };

    class SvgUseElement final : public SvgGraphicsElement,
                                public SvgURIReference {
    public:
        SvgUseElement(Document* document);

        const SvgLength& x() const { return m_x; }
        const SvgLength& y() const { return m_y; }
        const SvgLength& width() const { return m_width; }
        const SvgLength& height() const { return m_height; }
        Box* createBox(const RefPtr<BoxStyle>& style) final;
        void finishParsingDocument() final;

    private:
        Element* cloneTargetElement(SvgElement* targetElement);
        SvgLength m_x;
        SvgLength m_y;
        SvgLength m_width;
        SvgLength m_height;
    };

    class Image;

    class SvgImageElement final : public SvgGraphicsElement,
                                  public SvgURIReference {
    public:
        SvgImageElement(Document* document);

        const SvgLength& x() const { return m_x; }
        const SvgLength& y() const { return m_y; }
        const SvgLength& width() const { return m_width; }
        const SvgLength& height() const { return m_height; }
        const SvgPreserveAspectRatio& preserveAspectRatio() const {
            return m_preserveAspectRatio;
        }
        Box* createBox(const RefPtr<BoxStyle>& style) final;

        RefPtr<Image> image() const;

    private:
        SvgLength m_x;
        SvgLength m_y;
        SvgLength m_width;
        SvgLength m_height;
        SvgPreserveAspectRatio m_preserveAspectRatio;
    };

    class SvgSymbolElement final : public SvgGraphicsElement,
                                   public SvgFitToViewBox {
    public:
        SvgSymbolElement(Document* document);

        Box* createBox(const RefPtr<BoxStyle>& style) final;
    };

    class SvgAElement final : public SvgGraphicsElement,
                              public SvgURIReference {
    public:
        SvgAElement(Document* document);

        Box* createBox(const RefPtr<BoxStyle>& style) final;
    };

    class SvgGElement final : public SvgGraphicsElement {
    public:
        SvgGElement(Document* document);

        Box* createBox(const RefPtr<BoxStyle>& style) final;
    };

    class SvgDefsElement final : public SvgGraphicsElement {
    public:
        SvgDefsElement(Document* document);

        Box* createBox(const RefPtr<BoxStyle>& style) final;
    };

    class SvgResourceMarkerBox;

    class SvgGeometryElement : public SvgGraphicsElement {
    public:
        SvgGeometryElement(Document* document, GlobalString tagName);

        SvgResourceMarkerBox* getMarker(const std::string_view& id) const;
    };

    class SvgPathElement final : public SvgGeometryElement {
    public:
        SvgPathElement(Document* document);

        const Path& path() const { return m_d.value(); }

        Box* createBox(const RefPtr<BoxStyle>& style) final;

    private:
        SvgPath m_d;
    };

    class SvgShapeElement : public SvgGeometryElement {
    public:
        SvgShapeElement(Document* document, GlobalString tagName);

        virtual Rect getPath(Path& path) const = 0;

        Box* createBox(const RefPtr<BoxStyle>& style) final;
    };

    class SvgLineElement final : public SvgShapeElement {
    public:
        SvgLineElement(Document* document);

        const SvgLength& x1() const { return m_x1; }
        const SvgLength& y1() const { return m_y1; }
        const SvgLength& x2() const { return m_x2; }
        const SvgLength& y2() const { return m_y2; }

        Rect getPath(Path& path) const final;

    private:
        SvgLength m_x1;
        SvgLength m_y1;
        SvgLength m_x2;
        SvgLength m_y2;
    };

    class SvgRectElement final : public SvgShapeElement {
    public:
        SvgRectElement(Document* document);

        const SvgLength& x() const { return m_x; }
        const SvgLength& y() const { return m_y; }
        const SvgLength& width() const { return m_width; }
        const SvgLength& height() const { return m_height; }
        const SvgLength& rx() const { return m_rx; }
        const SvgLength& ry() const { return m_ry; }

        Rect getPath(Path& path) const final;

    private:
        SvgLength m_x;
        SvgLength m_y;
        SvgLength m_width;
        SvgLength m_height;
        SvgLength m_rx;
        SvgLength m_ry;
    };

    class SvgEllipseElement final : public SvgShapeElement {
    public:
        SvgEllipseElement(Document* document);

        const SvgLength& cx() const { return m_cx; }
        const SvgLength& cy() const { return m_cy; }
        const SvgLength& rx() const { return m_rx; }
        const SvgLength& ry() const { return m_ry; }

        Rect getPath(Path& path) const final;

    private:
        SvgLength m_cx;
        SvgLength m_cy;
        SvgLength m_rx;
        SvgLength m_ry;
    };

    class SvgCircleElement final : public SvgShapeElement {
    public:
        SvgCircleElement(Document* document);

        const SvgLength& cx() const { return m_cx; }
        const SvgLength& cy() const { return m_cy; }
        const SvgLength& r() const { return m_r; }

        Rect getPath(Path& path) const final;

    private:
        SvgLength m_cx;
        SvgLength m_cy;
        SvgLength m_r;
    };

    class SvgPolyElement : public SvgShapeElement {
    public:
        SvgPolyElement(Document* document, GlobalString tagName);

        const SvgPointList& points() const { return m_points; }

        Rect getPath(Path& path) const final;

    private:
        SvgPointList m_points;
    };

    class SvgTextPositioningElement : public SvgGraphicsElement {
    public:
        SvgTextPositioningElement(Document* document, GlobalString tagName);

        const SvgLengthList& x() const { return m_x; }
        const SvgLengthList& y() const { return m_y; }
        const SvgLengthList& dx() const { return m_dx; }
        const SvgLengthList& dy() const { return m_dy; }
        const SvgNumberList& rotate() const { return m_rotate; }

    private:
        SvgLengthList m_x;
        SvgLengthList m_y;
        SvgLengthList m_dx;
        SvgLengthList m_dy;
        SvgNumberList m_rotate;
    };

    class SvgTSpanElement final : public SvgTextPositioningElement {
    public:
        SvgTSpanElement(Document* document);

        Box* createBox(const RefPtr<BoxStyle>& style) final;
    };

    class SvgTextElement final : public SvgTextPositioningElement {
    public:
        SvgTextElement(Document* document);

        Box* createBox(const RefPtr<BoxStyle>& style) final;
    };

    class SvgMarkerElement final : public SvgElement, public SvgFitToViewBox {
    public:
        SvgMarkerElement(Document* document);

        const SvgLength& refX() const { return m_refX; }
        const SvgLength& refY() const { return m_refY; }
        const SvgLength& markerWidth() const { return m_markerWidth; }
        const SvgLength& markerHeight() const { return m_markerHeight; }
        const SvgAngle& orient() const { return m_orient; }
        const SvgMarkerUnitsType markerUnits() const {
            return m_markerUnits.value();
        }
        Box* createBox(const RefPtr<BoxStyle>& style) final;

    private:
        SvgLength m_refX;
        SvgLength m_refY;
        SvgLength m_markerWidth;
        SvgLength m_markerHeight;
        SvgEnumeration<SvgMarkerUnitsType> m_markerUnits;
        SvgAngle m_orient;
    };

    class SvgClipPathElement final : public SvgGraphicsElement {
    public:
        SvgClipPathElement(Document* document);

        const SvgUnitsType clipPathUnits() const {
            return m_clipPathUnits.value();
        }
        Box* createBox(const RefPtr<BoxStyle>& style) final;

    private:
        SvgEnumeration<SvgUnitsType> m_clipPathUnits;
    };

    class SvgMaskElement final : public SvgElement {
    public:
        SvgMaskElement(Document* document);

        const SvgLength& x() const { return m_x; }
        const SvgLength& y() const { return m_y; }
        const SvgLength& width() const { return m_width; }
        const SvgLength& height() const { return m_height; }
        const SvgUnitsType maskUnits() const { return m_maskUnits.value(); }
        const SvgUnitsType maskContentUnits() const {
            return m_maskContentUnits.value();
        }
        Box* createBox(const RefPtr<BoxStyle>& style) final;

    private:
        SvgLength m_x;
        SvgLength m_y;
        SvgLength m_width;
        SvgLength m_height;
        SvgEnumeration<SvgUnitsType> m_maskUnits;
        SvgEnumeration<SvgUnitsType> m_maskContentUnits;
    };

    class SvgPatternAttributes;

    class SvgPatternElement final : public SvgElement,
                                    public SvgURIReference,
                                    public SvgFitToViewBox {
    public:
        SvgPatternElement(Document* document);

        const SvgLength& x() const { return m_x; }
        const SvgLength& y() const { return m_y; }
        const SvgLength& width() const { return m_width; }
        const SvgLength& height() const { return m_height; }
        const Transform& patternTransform() const {
            return m_patternTransform.value();
        }
        const SvgUnitsType patternUnits() const {
            return m_patternUnits.value();
        }
        const SvgUnitsType patternContentUnits() const {
            return m_patternContentUnits.value();
        }
        SvgPatternAttributes collectPatternAttributes() const;
        Box* createBox(const RefPtr<BoxStyle>& style) final;

    private:
        SvgLength m_x;
        SvgLength m_y;
        SvgLength m_width;
        SvgLength m_height;
        SvgTransform m_patternTransform;
        SvgEnumeration<SvgUnitsType> m_patternUnits;
        SvgEnumeration<SvgUnitsType> m_patternContentUnits;
    };

    class SvgPatternAttributes {
    public:
        SvgPatternAttributes() = default;

        const SvgLength& x() const { return m_x->x(); }
        const SvgLength& y() const { return m_y->y(); }
        const SvgLength& width() const { return m_width->width(); }
        const SvgLength& height() const { return m_height->height(); }
        const Transform& patternTransform() const {
            return m_patternTransform->patternTransform();
        }
        SvgUnitsType patternUnits() const {
            return m_patternUnits->patternUnits();
        }
        SvgUnitsType patternContentUnits() const {
            return m_patternContentUnits->patternContentUnits();
        }
        const Rect& viewBox() const { return m_viewBox->viewBox(); }
        const SvgPreserveAspectRatio& preserveAspectRatio() const {
            return m_preserveAspectRatio->preserveAspectRatio();
        }
        const SvgPatternElement* patternContentElement() const {
            return m_patternContentElement;
        }

        bool hasX() const { return m_x; }
        bool hasY() const { return m_y; }
        bool hasWidth() const { return m_width; }
        bool hasHeight() const { return m_height; }
        bool hasPatternTransform() const { return m_patternTransform; }
        bool hasPatternUnits() const { return m_patternUnits; }
        bool hasPatternContentUnits() const { return m_patternContentUnits; }
        bool hasViewBox() const { return m_viewBox; }
        bool hasPreserveAspectRatio() const { return m_preserveAspectRatio; }
        bool hasPatternContentElement() const {
            return m_patternContentElement;
        }

        void setX(const SvgPatternElement* value) { m_x = value; }
        void setY(const SvgPatternElement* value) { m_y = value; }
        void setWidth(const SvgPatternElement* value) { m_width = value; }
        void setHeight(const SvgPatternElement* value) { m_height = value; }
        void setPatternTransform(const SvgPatternElement* value) {
            m_patternTransform = value;
        }
        void setPatternUnits(const SvgPatternElement* value) {
            m_patternUnits = value;
        }
        void setPatternContentUnits(const SvgPatternElement* value) {
            m_patternContentUnits = value;
        }
        void setViewBox(const SvgPatternElement* value) { m_viewBox = value; }
        void setPreserveAspectRatio(const SvgPatternElement* value) {
            m_preserveAspectRatio = value;
        }
        void setPatternContentElement(const SvgPatternElement* value) {
            m_patternContentElement = value;
        }

        void setDefaultValues(const SvgPatternElement* element) {
            if (!m_x) {
                m_x = element;
            }
            if (!m_y) {
                m_y = element;
            }
            if (!m_width) {
                m_width = element;
            }
            if (!m_height) {
                m_height = element;
            }
            if (!m_patternTransform) {
                m_patternTransform = element;
            }
            if (!m_patternUnits) {
                m_patternUnits = element;
            }
            if (!m_patternContentUnits) {
                m_patternContentUnits = element;
            }
            if (!m_viewBox) {
                m_viewBox = element;
            }
            if (!m_preserveAspectRatio) {
                m_preserveAspectRatio = element;
            }
            if (!m_patternContentElement) {
                m_patternContentElement = element;
            }
        }

    private:
        const SvgPatternElement* m_x{nullptr};
        const SvgPatternElement* m_y{nullptr};
        const SvgPatternElement* m_width{nullptr};
        const SvgPatternElement* m_height{nullptr};
        const SvgPatternElement* m_patternTransform{nullptr};
        const SvgPatternElement* m_patternUnits{nullptr};
        const SvgPatternElement* m_patternContentUnits{nullptr};
        const SvgPatternElement* m_viewBox{nullptr};
        const SvgPatternElement* m_preserveAspectRatio{nullptr};
        const SvgPatternElement* m_patternContentElement{nullptr};
    };

    class Color;

    class SvgStopElement final : public SvgElement {
    public:
        SvgStopElement(Document* document);

        const float offset() const { return m_offset.value(); }
        Color stopColorIncludingOpacity() const;
        Box* createBox(const RefPtr<BoxStyle>& style) final;

    private:
        SvgNumberPercentage m_offset;
    };

    class SvgGradientAttributes;

    class SvgGradientElement : public SvgElement, public SvgURIReference {
    public:
        SvgGradientElement(Document* document, GlobalString tagName);

        const Transform& gradientTransform() const {
            return m_gradientTransform.value();
        }
        const SvgUnitsType gradientUnits() const {
            return m_gradientUnits.value();
        }
        const SvgSpreadMethodType spreadMethod() const {
            return m_spreadMethod.value();
        }
        void collectGradientAttributes(SvgGradientAttributes& attributes) const;

    private:
        SvgTransform m_gradientTransform;
        SvgEnumeration<SvgUnitsType> m_gradientUnits;
        SvgEnumeration<SvgSpreadMethodType> m_spreadMethod;
    };

    class SvgGradientAttributes {
    public:
        SvgGradientAttributes() = default;

        const Transform& gradientTransform() const {
            return m_gradientTransform->gradientTransform();
        }
        SvgSpreadMethodType spreadMethod() const {
            return m_spreadMethod->spreadMethod();
        }
        SvgUnitsType gradientUnits() const {
            return m_gradientUnits->gradientUnits();
        }
        const SvgGradientElement* gradientContentElement() const {
            return m_gradientContentElement;
        }

        bool hasGradientTransform() const { return m_gradientTransform; }
        bool hasSpreadMethod() const { return m_spreadMethod; }
        bool hasGradientUnits() const { return m_gradientUnits; }
        bool hasGradientContentElement() const {
            return m_gradientContentElement;
        }

        void setGradientTransform(const SvgGradientElement* value) {
            m_gradientTransform = value;
        }
        void setSpreadMethod(const SvgGradientElement* value) {
            m_spreadMethod = value;
        }
        void setGradientUnits(const SvgGradientElement* value) {
            m_gradientUnits = value;
        }
        void setGradientContentElement(const SvgGradientElement* value) {
            m_gradientContentElement = value;
        }

        void setDefaultValues(const SvgGradientElement* element) {
            if (!m_gradientTransform) {
                m_gradientTransform = element;
            }
            if (!m_spreadMethod) {
                m_spreadMethod = element;
            }
            if (!m_gradientUnits) {
                m_gradientUnits = element;
            }
            if (!m_gradientContentElement) {
                m_gradientContentElement = element;
            }
        }

    private:
        const SvgGradientElement* m_gradientTransform{nullptr};
        const SvgGradientElement* m_spreadMethod{nullptr};
        const SvgGradientElement* m_gradientUnits{nullptr};
        const SvgGradientElement* m_gradientContentElement{nullptr};
    };

    class SvgLinearGradientAttributes;

    class SvgLinearGradientElement final : public SvgGradientElement {
    public:
        SvgLinearGradientElement(Document* document);

        const SvgLength& x1() const { return m_x1; }
        const SvgLength& y1() const { return m_y1; }
        const SvgLength& x2() const { return m_x2; }
        const SvgLength& y2() const { return m_y2; }
        SvgLinearGradientAttributes collectGradientAttributes() const;
        Box* createBox(const RefPtr<BoxStyle>& style) final;

    private:
        SvgLength m_x1;
        SvgLength m_y1;
        SvgLength m_x2;
        SvgLength m_y2;
    };

    class SvgLinearGradientAttributes : public SvgGradientAttributes {
    public:
        SvgLinearGradientAttributes() = default;

        const SvgLength& x1() const { return m_x1->x1(); }
        const SvgLength& y1() const { return m_y1->y1(); }
        const SvgLength& x2() const { return m_x2->x2(); }
        const SvgLength& y2() const { return m_y2->y2(); }

        bool hasX1() const { return m_x1; }
        bool hasY1() const { return m_y1; }
        bool hasX2() const { return m_x2; }
        bool hasY2() const { return m_y2; }

        void setX1(const SvgLinearGradientElement* value) { m_x1 = value; }
        void setY1(const SvgLinearGradientElement* value) { m_y1 = value; }
        void setX2(const SvgLinearGradientElement* value) { m_x2 = value; }
        void setY2(const SvgLinearGradientElement* value) { m_y2 = value; }

        void setDefaultValues(const SvgLinearGradientElement* element) {
            SvgGradientAttributes::setDefaultValues(element);
            if (!m_x1) {
                m_x1 = element;
            }
            if (!m_y1) {
                m_y1 = element;
            }
            if (!m_x2) {
                m_x2 = element;
            }
            if (!m_y2) {
                m_y2 = element;
            }
        }

    private:
        const SvgLinearGradientElement* m_x1{nullptr};
        const SvgLinearGradientElement* m_y1{nullptr};
        const SvgLinearGradientElement* m_x2{nullptr};
        const SvgLinearGradientElement* m_y2{nullptr};
    };

    class SvgRadialGradientAttributes;

    class SvgRadialGradientElement final : public SvgGradientElement {
    public:
        SvgRadialGradientElement(Document* document);

        const SvgLength& cx() const { return m_cx; }
        const SvgLength& cy() const { return m_cy; }
        const SvgLength& r() const { return m_r; }
        const SvgLength& fx() const { return m_fx; }
        const SvgLength& fy() const { return m_fy; }
        SvgRadialGradientAttributes collectGradientAttributes() const;
        Box* createBox(const RefPtr<BoxStyle>& style) final;

    private:
        SvgLength m_cx;
        SvgLength m_cy;
        SvgLength m_r;
        SvgLength m_fx;
        SvgLength m_fy;
    };

    class SvgRadialGradientAttributes : public SvgGradientAttributes {
    public:
        SvgRadialGradientAttributes() = default;

        const SvgLength& cx() const { return m_cx->cx(); }
        const SvgLength& cy() const { return m_cy->cy(); }
        const SvgLength& r() const { return m_r->r(); }
        const SvgLength& fx() const { return m_fx ? m_fx->fx() : m_cx->cx(); }
        const SvgLength& fy() const { return m_fy ? m_fy->fy() : m_cy->cy(); }

        bool hasCx() const { return m_cx; }
        bool hasCy() const { return m_cy; }
        bool hasR() const { return m_r; }
        bool hasFx() const { return m_fx; }
        bool hasFy() const { return m_fy; }

        void setCx(const SvgRadialGradientElement* value) { m_cx = value; }
        void setCy(const SvgRadialGradientElement* value) { m_cy = value; }
        void setR(const SvgRadialGradientElement* value) { m_r = value; }
        void setFx(const SvgRadialGradientElement* value) { m_fx = value; }
        void setFy(const SvgRadialGradientElement* value) { m_fy = value; }

        void setDefaultValues(const SvgRadialGradientElement* element) {
            SvgGradientAttributes::setDefaultValues(element);
            if (!m_cx) {
                m_cx = element;
            }
            if (!m_cy) {
                m_cy = element;
            }
            if (!m_r) {
                m_r = element;
            }
        }

    private:
        const SvgRadialGradientElement* m_cx{nullptr};
        const SvgRadialGradientElement* m_cy{nullptr};
        const SvgRadialGradientElement* m_r{nullptr};
        const SvgRadialGradientElement* m_fx{nullptr};
        const SvgRadialGradientElement* m_fy{nullptr};
    };

    class SvgStyleElement final : public SvgElement {
    public:
        SvgStyleElement(Document* document);

        const HeapString& type() const;
        const HeapString& media() const;

        void finishParsingDocument() final;
    };

    class SvgDocument final : public XmlDocument {
    public:
        static constexpr ClassKind classKind = ClassKind::SvgDocument;

        static std::unique_ptr<SvgDocument>
        create(Book* book, ResourceFetcher* fetcher, Url baseUrl);

    private:
        SvgDocument(Book* book, ResourceFetcher* fetcher, Url baseUrl);
    };
} // namespace plutobook