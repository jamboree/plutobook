#pragma once

#include "box-style.h"
#include "optional.h"
#include "geometry.h"

#include <memory>

namespace plutobook {
    class OutputStream;

    enum class PaintPhase { Decorations, Floats, Contents, Outlines };

    class GraphicsContext;

    class PaintInfo {
    public:
        PaintInfo(GraphicsContext& context, const Rect& rect)
            : m_context(context), m_rect(rect) {}

        GraphicsContext& operator*() const { return m_context; }
        GraphicsContext* operator->() const { return &m_context; }

        GraphicsContext& context() const { return m_context; }
        const Rect& rect() const { return m_rect; }

    private:
        GraphicsContext& m_context;
        Rect m_rect;
    };

    class Node;
    class BoxLayer;
    class BoxView;
    class BoxModel;
    class BlockBox;
    class BlockFlowBox;
    class LineBox;

    enum class BoxType {
        Flex,
        Inline,
        ListItem,
        InsideListMarker,
        OutsideListMarker,
        Text,
        LineBreak,
        WordBreak,
        Table,
        TableSection,
        TableRow,
        TableColumn,
        TableCell,
        TableCaption,
        BoxView,
        BlockFlow,
        Page,
        PageMargin,
        Leader,
        TargetCounter,
        Image,
        MultiColumnRow,
        MultiColumnSpan,
        MultiColumnFlow,
        TextInput,
        Select,
        SvgHiddenContainer,
        SvgTransformableContainer,
        SvgViewportContainer,
        SvgResourceMarker,
        SvgResourceClipper,
        SvgResourceMasker,
        SvgResourcePattern,
        SvgGradientStop,
        SvgResourceLinearGradient,
        SvgResourceRadialGradient,
        SvgInlineText,
        SvgTSpan,
        SvgText,
        SvgRoot,
        SvgImage,
        SvgPath,
        SvgShape,
    };

    class Box {
    public:
        using ClassRoot = Box;
        using ClassKind = BoxType;

        Box(ClassKind type, Node* node, const RefPtr<BoxStyle>& style);

        virtual ~Box();
        virtual bool avoidsFloats() const { return true; }
        virtual void addChild(Box* newChild);

        ClassKind type() const noexcept { return m_type; }

        Node* node() const { return m_node; }
        BoxStyle* style() const { return m_style.get(); }
        Box* parentBox() const { return m_parentBox; }
        Box* nextSibling() const { return m_nextSibling; }
        Box* prevSibling() const { return m_prevSibling; }
        Box* firstChild() const { return m_firstChild; }
        Box* lastChild() const { return m_lastChild; }

        void setParentBox(Box* box) { m_parentBox = box; }
        void setPrevSibling(Box* box) { m_prevSibling = box; }
        void setNextSibling(Box* box) { m_nextSibling = box; }

        void insertChild(Box* newChild, Box* nextChild);
        void appendChild(Box* newChild);
        void removeChild(Box* child);

        void moveChildrenTo(Box* newParent);

        static Box* create(Node* node, const RefPtr<BoxStyle>& style);
        static Box* createAnonymous(Display display,
                                    const BoxStyle* parentStyle);
        static BlockFlowBox* createAnonymousBlock(const BoxStyle* parentStyle);

        bool canContainFixedPositionedBoxes() const;
        bool canContainAbsolutePositionedBoxes() const;

        BlockBox* containingBlock() const;
        BoxModel* containingBox() const;
        BoxLayer* enclosingLayer() const;
        BoxView* view() const;

        bool isBodyBox() const;
        bool isRootBox() const;
        bool isListMarkerBox() const {
            return isInsideListMarkerBox() || isOutsideListMarkerBox();
        }
        bool isFlexItem() const;

        bool isBoxModel() const;
        bool isBoxFrame() const;
        bool isBoxView() const;
        bool isTextBox() const;
        bool isLineBreakBox() const;
        bool isWordBreakBox() const;
        bool isContentBox() const;
        bool isLeaderBox() const;
        bool isTargetCounterBox() const;
        bool isInlineBox() const;
        bool isBlockBox() const;
        bool isBlockFlowBox() const;
        bool isFlexBox() const;
        bool isReplacedBox() const;
        bool isImageBox() const;
        bool isListItemBox() const;
        bool isInsideListMarkerBox() const;
        bool isOutsideListMarkerBox() const;
        bool isMultiColumnRowBox() const;
        bool isMultiColumnSpanBox() const;
        bool isMultiColumnFlowBox() const;
        bool isPageBox() const;
        bool isPageMarginBox() const;
        bool isTableBox() const;
        bool isTableCellBox() const;
        bool isTableColumnBox() const;
        bool isTableRowBox() const;
        bool isTableCaptionBox() const;
        bool isTableSectionBox() const;
        bool isTextInputBox() const;
        bool isSelectBox() const;
        bool isSvgInlineTextBox() const;
        bool isSvgTSpanBox() const;
        bool isSvgTextBox() const;
        bool isSvgBoxModel() const;
        bool isSvgRootBox() const;
        bool isSvgImageBox() const;
        bool isSvgGeometryBox() const;
        bool isSvgPathBox() const;
        bool isSvgShapeBox() const;
        bool isSvgContainerBox() const;
        bool isSvgHiddenContainerBox() const;
        bool isSvgTransformableContainerBox() const;
        bool isSvgViewportContainerBox() const;
        bool isSvgResourceContainerBox() const;
        bool isSvgResourceMarkerBox() const;
        bool isSvgResourceClipperBox() const;
        bool isSvgResourceMaskerBox() const;
        bool isSvgResourcePaintServerBox() const;
        bool isSvgResourcePatternBox() const;
        bool isSvgGradientStopBox() const;
        bool isSvgResourceGradientBox() const;
        bool isSvgResourceLinearGradientBox() const;
        bool isSvgResourceRadialGradientBox() const;

        bool isRelativePositioned() const {
            return m_style->position() == Position::Relative;
        }
        bool isFixedPositioned() const {
            return m_style->position() == Position::Fixed;
        }

        bool isAnonymous() const { return m_isAnonymous; }
        bool isAnonymousBlock() const { return m_isAnonymousBlock; }
        bool isChildrenInline() const { return m_isChildrenInline; }
        bool isInline() const { return m_isInline; }
        bool isFloating() const { return m_isFloating; }
        bool isPositioned() const { return m_isPositioned; }
        bool isFloatingOrPositioned() const {
            return m_isFloating || m_isPositioned;
        }
        bool isReplaced() const { return m_isReplaced; }
        bool isOverflowHidden() const { return m_isOverflowHidden; }
        bool isBackgroundStolen() const { return m_isBackgroundStolen; }
        bool isColumnSpanner() const { return m_isColumnSpanner; }

        void setIsAnonymous(bool value) { m_isAnonymous = value; }
        void setIsAnonymousBlock(bool value) { m_isAnonymousBlock = value; }
        void setIsChildrenInline(bool value) { m_isChildrenInline = value; }
        void setIsInline(bool value) { m_isInline = value; }
        void setIsFloating(bool value) { m_isFloating = value; }
        void setIsPositioned(bool value) { m_isPositioned = value; }
        void setIsReplaced(bool value) { m_isReplaced = value; }
        void setIsOverflowHidden(bool value) { m_isOverflowHidden = value; }
        void setIsBackgroundStolen(bool value) { m_isBackgroundStolen = value; }
        void setIsColumnSpanner(bool value) { m_isColumnSpanner = value; }

        bool hasColumnFlowBox() const { return m_hasColumnFlowBox; }
        bool hasTransform() const { return m_hasTransform; }
        bool hasLayer() const { return m_hasLayer; }

        void setHasColumnFlowBox(bool value) { m_hasColumnFlowBox = value; }
        void setHasTransform(bool value) { m_hasTransform = value; }
        void setHasLayer(bool value) { m_hasLayer = value; }

        Document* document() const { return m_style->document(); }

        void paintAnnotation(GraphicsContext& context, const Rect& rect) const;

        virtual Rect fillBoundingBox() const { return Rect::Invalid; }
        virtual Rect strokeBoundingBox() const { return Rect::Invalid; }
        virtual Rect paintBoundingBox() const { return Rect::Invalid; }
        virtual Transform localTransform() const { return Transform::Identity; }

        virtual void build();

        static void serializeStart(OutputStream& o, int indent,
                                   bool selfClosing, const Box* box,
                                   const LineBox* line);
        static void serializeEnd(OutputStream& o, int indent, bool selfClosing,
                                 const Box* box, const LineBox* line);

        void serialize(OutputStream& o, int indent) const;
        virtual void serializeChildren(OutputStream& o, int indent) const;

        virtual const char* name() const { return "Box"; }

    private:
        ClassKind m_type;
        bool m_isAnonymous : 1 {false};
        bool m_isAnonymousBlock : 1 {false};
        bool m_isChildrenInline : 1 {false};
        bool m_isInline : 1 {false};
        bool m_isFloating : 1 {false};
        bool m_isPositioned : 1 {false};
        bool m_isReplaced : 1 {false};
        bool m_isOverflowHidden : 1 {false};
        bool m_isBackgroundStolen : 1 {false};
        bool m_isColumnSpanner : 1 {false};
        bool m_hasColumnFlowBox : 1 {false};
        bool m_hasTransform : 1 {false};
        bool m_hasLayer : 1 {false};
        Node* m_node;
        RefPtr<BoxStyle> m_style;
        Box* m_parentBox{nullptr};
        Box* m_nextSibling{nullptr};
        Box* m_prevSibling{nullptr};
        Box* m_firstChild{nullptr};
        Box* m_lastChild{nullptr};
    };

    class BoxModel : public Box {
    public:
        BoxModel(ClassKind type, Node* node, const RefPtr<BoxStyle>& style);
        ~BoxModel() override;

        void addChild(Box* newChild) override;

        void paintBackgroundStyle(const PaintInfo& info, const Rect& borderRect,
                                  const BoxStyle* backgroundStyle,
                                  bool includeLeftEdge = true,
                                  bool includeRightEdge = true) const;
        void paintBackground(const PaintInfo& info, const Rect& borderRect,
                             bool includeLeftEdge = true,
                             bool includeRightEdge = true) const;

        virtual void paintRootBackground(const PaintInfo& info) const {}

        void paintBorder(const PaintInfo& info, const Rect& borderRect,
                         bool includeLeftEdge = true,
                         bool includeRightEdge = true) const;
        void paintOutline(const PaintInfo& info, const Rect& borderRect) const;

        virtual void paint(const PaintInfo& info, const Point& offset,
                           PaintPhase phase);

        virtual Rect visualOverflowRect() const = 0;
        virtual Rect borderBoundingBox() const = 0;
        virtual bool requiresLayer() const = 0;

        BoxLayer* layer() const { return m_layer.get(); }

        void paintLayer(GraphicsContext& context, const Rect& rect);
        void updateLayerPosition();

        float relativePositionOffsetX() const;
        float relativePositionOffsetY() const;
        Point relativePositionOffset() const;

        float
        containingBlockWidthForPositioned(const BoxModel* container) const;
        float containingBlockWidthForPositioned() const {
            return containingBlockWidthForPositioned(containingBox());
        }

        float
        containingBlockHeightForPositioned(const BoxModel* container) const;
        float containingBlockHeightForPositioned() const {
            return containingBlockHeightForPositioned(containingBox());
        }

        virtual float
        containingBlockWidthForContent(const BlockBox* container) const;
        float containingBlockWidthForContent() const {
            return containingBlockWidthForContent(containingBlock());
        }

        Optional<float>
        containingBlockHeightForContent(const BlockBox* container) const;
        Optional<float> containingBlockHeightForContent() const {
            return containingBlockHeightForContent(containingBlock());
        }

        float margin(Edge edge) const { return m_margin[edge]; }

        float marginWidth() const {
            return margin(LeftEdge) + margin(RightEdge);
        }
        float marginHeight() const {
            return margin(TopEdge) + margin(BottomEdge);
        }

        void setMargin(Edge edge, float value) { m_margin[edge] = value; }

        void updateVerticalMargins(const BlockBox* container);
        void updateHorizontalMargins(const BlockBox* container);
        void updateMarginWidths(const BlockBox* container);

        float padding(Edge edge) const { return m_padding[edge]; }

        float paddingWidth() const {
            return padding(LeftEdge) + padding(RightEdge);
        }
        float paddingHeight() const {
            return padding(TopEdge) + padding(BottomEdge);
        }

        void setPadding(Edge edge, float value) { m_padding[edge] = value; }

        void updateVerticalPaddings(const BlockBox* container);
        void updateHorizontalPaddings(const BlockBox* container);
        void updatePaddingWidths(const BlockBox* container);

        virtual void computeBorderWidths(float& borderTop, float& borderBottom,
                                         float& borderLeft,
                                         float& borderRight) const;

        float border(Edge edge) const;

        float borderWidth() const {
            return border(LeftEdge) + border(RightEdge);
        }
        float borderHeight() const {
            return border(TopEdge) + border(BottomEdge);
        }

        float borderAndPadding(Edge edge) const {
            return border(edge) + padding(edge);
        }

        float borderAndPaddingWidth() const {
            return borderWidth() + paddingWidth();
        }
        float borderAndPaddingHeight() const {
            return borderHeight() + paddingHeight();
        }

        float marginStart(Direction direction) const {
            return direction == Direction::Ltr ? margin(LeftEdge)
                                               : margin(RightEdge);
        }
        float marginEnd(Direction direction) const {
            return direction == Direction::Ltr ? margin(RightEdge)
                                               : margin(LeftEdge);
        }

        float borderStart(Direction direction) const {
            return direction == Direction::Ltr ? border(LeftEdge)
                                               : border(RightEdge);
        }
        float borderEnd(Direction direction) const {
            return direction == Direction::Ltr ? border(RightEdge)
                                               : border(LeftEdge);
        }

        float paddingStart(Direction direction) const {
            return direction == Direction::Ltr ? padding(LeftEdge)
                                               : padding(RightEdge);
        }
        float paddingEnd(Direction direction) const {
            return direction == Direction::Ltr ? padding(RightEdge)
                                               : padding(LeftEdge);
        }

        float marginStart() const { return marginStart(style()->direction()); }
        float marginEnd() const { return marginEnd(style()->direction()); }

        float borderStart() const { return borderStart(style()->direction()); }
        float borderEnd() const { return borderEnd(style()->direction()); }

        float paddingStart() const {
            return paddingStart(style()->direction());
        }
        float paddingEnd() const { return paddingEnd(style()->direction()); }

        void build() override;

        const char* name() const override { return "BoxModel"; }

    private:
        std::unique_ptr<BoxLayer> m_layer;

        float m_margin[4] = {};
        float m_padding[4] = {};
        mutable float m_border[4] = {-1, -1, -1, -1};

        friend class BoxFrame;
    };

    extern template bool is<BoxModel>(const Box& value);

    class ReplacedLineBox;
    class FragmentBuilder;

    class BoxFrame : public BoxModel {
    public:
        BoxFrame(ClassKind type, Node* node, const RefPtr<BoxStyle>& style);
        ~BoxFrame() override;

        bool requiresLayer() const override;

        BoxFrame* parentBoxFrame() const;
        BoxFrame* nextBoxFrame() const;
        BoxFrame* prevBoxFrame() const;
        BoxFrame* firstBoxFrame() const;
        BoxFrame* lastBoxFrame() const;

        ReplacedLineBox* line() const { return m_line.get(); }
        void setLine(std::unique_ptr<ReplacedLineBox> line);

        float x() const { return m_x; }
        float y() const { return m_y; }
        float width() const { return m_width; }
        float height() const { return m_height; }

        void setX(float x) { m_x = x; }
        void setY(float y) { m_y = y; }
        void setWidth(float width) { m_width = width; }
        void setHeight(float height) { m_height = height; }

        void setLocation(float x, float y) {
            m_x = x;
            m_y = y;
        }
        void setSize(float width, float height) {
            m_width = width;
            m_height = height;
        }

        Point location() const { return Point(m_x, m_y); }
        Size size() const { return Size(m_width, m_height); }

        float borderBoxWidth() const { return m_width; }
        float borderBoxHeight() const { return m_height; }

        float paddingBoxWidth() const {
            return borderBoxWidth() - borderWidth();
        }
        float paddingBoxHeight() const {
            return borderBoxHeight() - borderHeight();
        }

        float contentBoxWidth() const {
            return paddingBoxWidth() - paddingWidth();
        }
        float contentBoxHeight() const {
            return paddingBoxHeight() - paddingHeight();
        }

        float marginBoxWidth() const {
            return borderBoxWidth() + marginWidth();
        }
        float marginBoxHeight() const {
            return borderBoxHeight() + marginHeight();
        }

        Size borderBoxSize() const {
            return Size(borderBoxWidth(), borderBoxHeight());
        }
        Size paddingBoxSize() const {
            return Size(paddingBoxWidth(), paddingBoxHeight());
        }
        Size contentBoxSize() const {
            return Size(contentBoxWidth(), contentBoxHeight());
        }
        Size marginBoxSize() const {
            return Size(marginBoxWidth(), marginBoxHeight());
        }

        Rect borderBoxRect() const {
            return Rect(0, 0, borderBoxWidth(), borderBoxHeight());
        }
        Rect paddingBoxRect() const {
            return Rect(border(LeftEdge), border(TopEdge), paddingBoxWidth(),
                        paddingBoxHeight());
        }
        Rect contentBoxRect() const {
            return Rect(border(LeftEdge) + padding(LeftEdge),
                        border(TopEdge) + padding(TopEdge), contentBoxWidth(),
                        contentBoxHeight());
        }
        Rect marginBoxRect() const {
            return Rect(-margin(LeftEdge), -margin(RightEdge), marginBoxWidth(),
                        marginBoxHeight());
        }

        Rect visualOverflowRect() const override;
        Rect borderBoundingBox() const override;
        Rect paintBoundingBox() const override;

        float overrideWidth() const { return m_overrideWidth; }
        float overrideHeight() const { return m_overrideHeight; }

        void setOverrideWidth(float width) { m_overrideWidth = width; }
        void setOverrideHeight(float height) { m_overrideHeight = height; }

        bool hasOverrideWidth() const { return m_overrideWidth >= 0; }
        bool hasOverrideHeight() const { return m_overrideHeight >= 0; }

        void setOverrideSize(float width, float height) {
            m_overrideWidth = width;
            m_overrideHeight = height;
        }
        void clearOverrideSize() { setOverrideSize(-1, -1); }

        virtual void computePreferredWidths(float& minPreferredWidth,
                                            float& maxPreferredWidth) const;

        float minPreferredWidth() const;
        float maxPreferredWidth() const;

        float adjustBorderBoxWidth(float width) const;
        float adjustBorderBoxHeight(float height) const;
        float adjustContentBoxWidth(float width) const;
        float adjustContentBoxHeight(float height) const;

        void computeHorizontalStaticDistance(Length& leftLength,
                                             Length& rightLength,
                                             const BoxModel* container,
                                             float containerWidth) const;
        void computeVerticalStaticDistance(Length& topLength,
                                           Length& bottomLength,
                                           const BoxModel* container) const;

        void computeHorizontalMargins(float& marginLeft, float& marginRight,
                                      float childWidth,
                                      const BlockBox* container,
                                      float containerWidth) const;
        void computeVerticalMargins(float& marginTop,
                                    float& marginBottom) const;

        float computeIntrinsicWidthUsing(const Length& widthLength,
                                         float containerWidth) const;

        virtual void computeWidth(float& x, float& width, float& marginLeft,
                                  float& marginRight) const;
        virtual void computeHeight(float& y, float& height, float& marginTop,
                                   float& marginBottom) const;

        void updateWidth();
        void updateHeight();

        virtual bool isSelfCollapsingBlock() const { return false; }

        virtual float maxMarginTop(bool positive) const;
        virtual float maxMarginBottom(bool positive) const;

        float collapsedMarginTop() const;
        float collapsedMarginBottom() const;

        virtual Optional<float> firstLineBaseline() const {
            return std::nullopt;
        }
        virtual Optional<float> lastLineBaseline() const {
            return std::nullopt;
        }
        virtual Optional<float> inlineBlockBaseline() const {
            return std::nullopt;
        }

        float overflow(Edge edge) const { return m_overflow[edge]; }

        virtual void updateOverflowRect();

        void addOverflowRect(const BoxFrame* child, float dx, float dy);
        void addOverflowRect(float top, float bottom, float left, float right);
        void addOverflowRect(const Rect& overflowRect);

        virtual void paintOutlines(const PaintInfo& info, const Point& offset);
        virtual void paintDecorations(const PaintInfo& info,
                                      const Point& offset);
        virtual void layout(FragmentBuilder* fragmentainer);

        const char* name() const override { return "BoxFrame"; }

    private:
        std::unique_ptr<ReplacedLineBox> m_line;

        float m_x{0};
        float m_y{0};
        float m_width{0};
        float m_height{0};

        float m_overrideWidth{-1};
        float m_overrideHeight{-1};

        float m_overflow[4] = {};

        mutable float m_minPreferredWidth{-1};
        mutable float m_maxPreferredWidth{-1};
    };

    extern template bool is<BoxFrame>(const Box& value);

    inline BoxFrame* BoxFrame::parentBoxFrame() const {
        return static_cast<BoxFrame*>(parentBox());
    }

    inline BoxFrame* BoxFrame::nextBoxFrame() const {
        return static_cast<BoxFrame*>(nextSibling());
    }

    inline BoxFrame* BoxFrame::prevBoxFrame() const {
        return static_cast<BoxFrame*>(prevSibling());
    }

    inline BoxFrame* BoxFrame::firstBoxFrame() const {
        return static_cast<BoxFrame*>(firstChild());
    }

    inline BoxFrame* BoxFrame::lastBoxFrame() const {
        return static_cast<BoxFrame*>(lastChild());
    }

    inline Rect BoxFrame::visualOverflowRect() const {
        if (!isOverflowHidden())
            return Rect(m_overflow[LeftEdge], m_overflow[TopEdge],
                        m_overflow[RightEdge] - m_overflow[LeftEdge],
                        m_overflow[BottomEdge] - m_overflow[TopEdge]);
        return borderBoxRect();
    }

    inline Rect BoxFrame::borderBoundingBox() const {
        return Rect(m_x, m_y, m_width, m_height);
    }

    inline Rect BoxFrame::paintBoundingBox() const {
        return borderBoundingBox();
    }
} // namespace plutobook