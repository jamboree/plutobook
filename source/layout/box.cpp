#include "box.h"
#include "box-layer.h"
#include "box-view.h"
#include "flex-box.h"
#include "list-item-box.h"
#include "table-box.h"
#include "line-box.h"
#include "text-box.h"
#include "content-box.h"
#include "replaced-box.h"
#include "multi-column-box.h"
#include "page-box.h"
#include "form-control-box.h"
#include "svg-text-box.h"
#include "svg-replaced-box.h"
#include "svg-resource-box.h"
#include "svg-geometry-box.h"
#include "border-painter.h"
#include "graphics-context.h"
#include "image-resource.h"
#include "document.h"

#include <cmath>

namespace plutobook {
template<class F>
decltype(auto) visit(Box* p, F&& f) {
    switch (p->type()) {
    case Box::ClassKind::Flex: return f(static_cast<FlexBox*>(p));
    case Box::ClassKind::Inline: return f(static_cast<InlineBox*>(p));
    case Box::ClassKind::ListItem: return f(static_cast<ListItemBox*>(p));
    case Box::ClassKind::InsideListMarker: return f(static_cast<InsideListMarkerBox*>(p));
    case Box::ClassKind::OutsideListMarker: return f(static_cast<OutsideListMarkerBox*>(p));
    case Box::ClassKind::Text: return f(static_cast<TextBox*>(p));
    case Box::ClassKind::LineBreak: return f(static_cast<LineBreakBox*>(p));
    case Box::ClassKind::WordBreak: return f(static_cast<WordBreakBox*>(p));
    case Box::ClassKind::Table: return f(static_cast<TableBox*>(p));
    case Box::ClassKind::TableSection: return f(static_cast<TableSectionBox*>(p));
    case Box::ClassKind::TableRow: return f(static_cast<TableRowBox*>(p));
    case Box::ClassKind::TableColumn: return f(static_cast<TableColumnBox*>(p));
    case Box::ClassKind::TableCell: return f(static_cast<TableCellBox*>(p));
    case Box::ClassKind::TableCaption: return f(static_cast<TableCaptionBox*>(p));
    case Box::ClassKind::BoxView: return f(static_cast<BoxView*>(p));
    case Box::ClassKind::BlockFlow: return f(static_cast<BlockFlowBox*>(p));
    case Box::ClassKind::Page: return f(static_cast<PageBox*>(p));
    case Box::ClassKind::PageMargin: return f(static_cast<PageMarginBox*>(p));
    case Box::ClassKind::Leader: return f(static_cast<LeaderBox*>(p));
    case Box::ClassKind::TargetCounter: return f(static_cast<TargetCounterBox*>(p));
    case Box::ClassKind::Image: return f(static_cast<ImageBox*>(p));
    case Box::ClassKind::MultiColumnRow: return f(static_cast<MultiColumnRowBox*>(p));
    case Box::ClassKind::MultiColumnSpan: return f(static_cast<MultiColumnSpanBox*>(p));
    case Box::ClassKind::MultiColumnFlow: return f(static_cast<MultiColumnFlowBox*>(p));
    case Box::ClassKind::TextInput: return f(static_cast<TextInputBox*>(p));
    case Box::ClassKind::Select: return f(static_cast<SelectBox*>(p));
    case Box::ClassKind::SvgHiddenContainer: return f(static_cast<SvgHiddenContainerBox*>(p));
    case Box::ClassKind::SvgTransformableContainer: return f(static_cast<SvgTransformableContainerBox*>(p));
    case Box::ClassKind::SvgViewportContainer: return f(static_cast<SvgViewportContainerBox*>(p));
    case Box::ClassKind::SvgResourceMarker: return f(static_cast<SvgResourceMarkerBox*>(p));
    case Box::ClassKind::SvgResourceClipper: return f(static_cast<SvgResourceClipperBox*>(p));
    case Box::ClassKind::SvgResourceMasker: return f(static_cast<SvgResourceMaskerBox*>(p));
    case Box::ClassKind::SvgResourcePattern: return f(static_cast<SvgResourcePatternBox*>(p));
    case Box::ClassKind::SvgGradientStop: return f(static_cast<SvgGradientStopBox*>(p));
    case Box::ClassKind::SvgResourceLinearGradient: return f(static_cast<SvgResourceLinearGradientBox*>(p));
    case Box::ClassKind::SvgResourceRadialGradient: return f(static_cast<SvgResourceRadialGradientBox*>(p));
    case Box::ClassKind::SvgInlineText: return f(static_cast<SvgInlineTextBox*>(p));
    case Box::ClassKind::SvgTSpan: return f(static_cast<SvgTSpanBox*>(p));
    case Box::ClassKind::SvgText: return f(static_cast<SvgTextBox*>(p));
    case Box::ClassKind::SvgRoot: return f(static_cast<SvgRootBox*>(p));
    case Box::ClassKind::SvgImage: return f(static_cast<SvgImageBox*>(p));
    case Box::ClassKind::SvgPath: return f(static_cast<SvgPathBox*>(p));
    case Box::ClassKind::SvgShape: return f(static_cast<SvgShapeBox*>(p));
    default: std::unreachable();
    }
}

template bool is<BoxModel>(const Box& value);
template bool is<BoxFrame>(const Box& value);
template bool is<TextBox>(const Box& value);
template bool is<ContentBox>(const Box& value);
template bool is<InlineBox>(const Box& value);
template bool is<BlockBox>(const Box& value);
template bool is<BlockFlowBox>(const Box& value);
template bool is<ReplacedBox>(const Box& value);
template bool is<SvgBoxModel>(const Box& value);
template bool is<SvgGeometryBox>(const Box& value);
template bool is<SvgContainerBox>(const Box& value);
template bool is<SvgHiddenContainerBox>(const Box& value);
template bool is<SvgResourceContainerBox>(const Box& value);
template bool is<SvgResourcePaintServerBox>(const Box& value);
template bool is<SvgResourceGradientBox>(const Box& value);

Box::Box(ClassKind type, Node* node, const RefPtr<BoxStyle>& style)
    : m_type(type), m_node(node), m_style(style)
{
    if(node) {
        node->setBox(this);
    }
}

Box::~Box()
{
    auto child = m_firstChild;
    while(child) {
        auto nextChild = child->nextSibling();
        child->setParentBox(nullptr);
        child->setNextSibling(nullptr);
        child->setPrevSibling(nullptr);
        delete child;
        child = nextChild;
    }

    if(m_parentBox)
        m_parentBox->removeChild(this);
    if(m_node) {
        m_node->setBox(nullptr);
    }
}

void Box::addChild(Box* newChild)
{
    appendChild(newChild);
}

void Box::insertChild(Box* newChild, Box* nextChild)
{
    if(nextChild == nullptr) {
        appendChild(newChild);
        return;
    }

    assert(nextChild->parentBox() == this);
    assert(newChild->parentBox() == nullptr);
    assert(newChild->nextSibling() == nullptr);
    assert(newChild->prevSibling() == nullptr);

    auto prevChild = nextChild->prevSibling();
    nextChild->setPrevSibling(newChild);
    assert(m_lastChild != prevChild);
    if(prevChild == nullptr) {
        assert(m_firstChild == nextChild);
        m_firstChild = newChild;
    } else {
        assert(m_firstChild != nextChild);
        prevChild->setNextSibling(newChild);
    }

    newChild->setParentBox(this);
    newChild->setNextSibling(nextChild);
    newChild->setPrevSibling(prevChild);
}

void Box::appendChild(Box* newChild)
{
    assert(newChild->parentBox() == nullptr);
    assert(newChild->nextSibling() == nullptr);
    assert(newChild->prevSibling() == nullptr);
    newChild->setParentBox(this);
    if(m_lastChild == nullptr) {
        assert(m_firstChild == nullptr);
        m_firstChild = m_lastChild = newChild;
        return;
    }

    newChild->setPrevSibling(m_lastChild);
    m_lastChild->setNextSibling(newChild);
    m_lastChild = newChild;
}

void Box::removeChild(Box* child)
{
    assert(child->parentBox() == this);
    auto nextChild = child->nextSibling();
    auto prevChild = child->prevSibling();
    if(nextChild)
        nextChild->setPrevSibling(prevChild);
    if(prevChild) {
        prevChild->setNextSibling(nextChild);
    }

    if(m_firstChild == child)
        m_firstChild = nextChild;
    if(m_lastChild == child) {
        m_lastChild = prevChild;
    }

    child->setParentBox(nullptr);
    child->setPrevSibling(nullptr);
    child->setNextSibling(nullptr);
}

void Box::moveChildrenTo(Box* newParent)
{
    auto child = m_firstChild;
    while(child) {
        auto nextChild = child->nextSibling();
        removeChild(child);
        newParent->appendChild(child);
        child = nextChild;
    }
}

Box* Box::create(Node* node, const RefPtr<BoxStyle>& style)
{
    if(style->pseudoType() == PseudoType::Marker) {
        if(style->listStylePosition() == ListStylePosition::Inside)
            return new (style->heap()) InsideListMarkerBox(style);
        return new (style->heap()) OutsideListMarkerBox(style);
    }

    switch(style->display()) {
    case Display::Inline:
        return new (style->heap()) InlineBox(node, style);
    case Display::Block:
    case Display::InlineBlock:
        return new (style->heap()) BlockFlowBox(node, style);
    case Display::Flex:
    case Display::InlineFlex:
        return new (style->heap()) FlexBox(node, style);
    case Display::Table:
    case Display::InlineTable:
        return new (style->heap()) TableBox(node, style);
    case Display::ListItem:
        return new (style->heap()) ListItemBox(node, style);
    case Display::TableCell:
        return new (style->heap()) TableCellBox(node, style);
    case Display::TableRow:
        return new (style->heap()) TableRowBox(node, style);
    case Display::TableCaption:
        return new (style->heap()) TableCaptionBox(node, style);
    case Display::TableColumn:
    case Display::TableColumnGroup:
        return new (style->heap()) TableColumnBox(node, style);
    case Display::TableRowGroup:
    case Display::TableHeaderGroup:
    case Display::TableFooterGroup:
        return new (style->heap()) TableSectionBox(node, style);
    default:
        assert(false);
    }

    return nullptr;
}

Box* Box::createAnonymous(Display display, const BoxStyle* parentStyle)
{
    auto newBox = create(nullptr, BoxStyle::create(parentStyle, display));
    newBox->setIsAnonymous(true);
    return newBox;
}

BlockFlowBox* Box::createAnonymousBlock(const BoxStyle* parentStyle)
{
    auto newStyle = BoxStyle::create(parentStyle, Display::Block);
    auto newBlock = new (newStyle->heap()) BlockFlowBox(nullptr, newStyle);
    newBlock->setIsAnonymousBlock(true);
    newBlock->setIsAnonymous(true);
    return newBlock;
}

bool Box::canContainFixedPositionedBoxes() const
{
    return (hasTransform() && isBlockBox()) || !parentBox();
}

bool Box::canContainAbsolutePositionedBoxes() const
{
    return style()->position() != Position::Static || canContainFixedPositionedBoxes();
}

BlockBox* Box::containingBlock() const
{
    auto parent = parentBox();
    if(style()->position() == Position::Static || style()->position() == Position::Relative || isTextBox()) {
        while(parent && !parent->isBlockBox())
            parent = parent->parentBox();
        return to<BlockBox>(parent);
    }

    if(style()->position() == Position::Fixed) {
        while(parent && !parent->canContainFixedPositionedBoxes()) {
            parent = parent->parentBox();
        }
    } else {
        while(parent && !parent->canContainAbsolutePositionedBoxes()) {
            parent = parent->parentBox();
        }
    }

    if(parent && !parent->isBlockBox())
        parent = parent->containingBlock();
    while(parent && parent->isAnonymous())
        parent = parent->containingBlock();
    return to<BlockBox>(parent);
}

BoxModel* Box::containingBox() const
{
    auto parent = parentBox();
    if(!isTextBox()) {
        if(style()->position() == Position::Fixed) {
            while(parent && !parent->canContainFixedPositionedBoxes()) {
                parent = parent->parentBox();
            }
        } else if(style()->position() == Position::Absolute) {
            while(parent && !parent->canContainAbsolutePositionedBoxes()) {
                parent = parent->parentBox();
            }
        }
    }

    return to<BoxModel>(parent);
}

BoxLayer* Box::enclosingLayer() const
{
    for(auto current = this; current; current = current->parentBox()) {
        if(current->hasLayer()) {
            return to<BoxModel>(*current).layer();
        }
    }

    return nullptr;
}

BoxView* Box::view() const
{
    return document()->box();
}

bool Box::isBodyBox() const
{
    return m_node && m_node->isOfType(xhtmlNs, bodyTag);
}

bool Box::isRootBox() const
{
    return m_node && m_node->isRootNode();
}

bool Box::isFlexItem() const
{
    return m_parentBox && m_parentBox->isFlexBox();
}

bool Box::isBoxModel() const { return is<BoxModel>(*this); }
bool Box::isBoxFrame() const { return is<BoxFrame>(*this); }
bool Box::isBoxView() const { return is<BoxView>(*this); }
bool Box::isTextBox() const { return is<TextBox>(*this); }
bool Box::isLineBreakBox() const { return is<LineBreakBox>(*this); }
bool Box::isWordBreakBox() const { return is<WordBreakBox>(*this); }
bool Box::isContentBox() const { return is<ContentBox>(*this); }
bool Box::isLeaderBox() const { return is<LeaderBox>(*this); }
bool Box::isTargetCounterBox() const { return is<TargetCounterBox>(*this); }
bool Box::isInlineBox() const { return is<InlineBox>(*this); }
bool Box::isBlockBox() const { return is<BlockBox>(*this); }
bool Box::isBlockFlowBox() const { return is<BlockFlowBox>(*this); }
bool Box::isFlexBox() const { return is<FlexBox>(*this); }
bool Box::isReplacedBox() const { return is<ReplacedBox>(*this); }
bool Box::isImageBox() const { return is<ImageBox>(*this); }
bool Box::isListItemBox() const { return is<ListItemBox>(*this); }
bool Box::isInsideListMarkerBox() const { return is<InsideListMarkerBox>(*this); }
bool Box::isOutsideListMarkerBox() const { return is<OutsideListMarkerBox>(*this); }
bool Box::isMultiColumnRowBox() const { return is<MultiColumnRowBox>(*this); }
bool Box::isMultiColumnSpanBox() const { return is<MultiColumnSpanBox>(*this); }
bool Box::isMultiColumnFlowBox() const { return is<MultiColumnFlowBox>(*this); }
bool Box::isPageBox() const { return is<PageBox>(*this); }
bool Box::isPageMarginBox() const { return is<PageMarginBox>(*this); }
bool Box::isTableBox() const { return is<TableBox>(*this); }
bool Box::isTableCellBox() const { return is<TableCellBox>(*this); }
bool Box::isTableColumnBox() const { return is<TableColumnBox>(*this); }
bool Box::isTableRowBox() const { return is<TableRowBox>(*this); }
bool Box::isTableCaptionBox() const { return is<TableCaptionBox>(*this); }
bool Box::isTableSectionBox() const { return is<TableSectionBox>(*this); }
bool Box::isTextInputBox() const { return is<TextInputBox>(*this); }
bool Box::isSelectBox() const { return is<SelectBox>(*this); }
bool Box::isSvgInlineTextBox() const { return is<SvgInlineTextBox>(*this); }
bool Box::isSvgTSpanBox() const { return is<SvgTSpanBox>(*this); }
bool Box::isSvgTextBox() const { return is<SvgTextBox>(*this); }
bool Box::isSvgBoxModel() const { return is<SvgBoxModel>(*this); }
bool Box::isSvgRootBox() const { return is<SvgRootBox>(*this); }
bool Box::isSvgImageBox() const { return is<SvgImageBox>(*this); }
bool Box::isSvgGeometryBox() const { return is<SvgGeometryBox>(*this); }
bool Box::isSvgPathBox() const { return is<SvgPathBox>(*this); }
bool Box::isSvgShapeBox() const { return is<SvgShapeBox>(*this); }
bool Box::isSvgContainerBox() const { return is<SvgContainerBox>(*this); }
bool Box::isSvgHiddenContainerBox() const { return is<SvgHiddenContainerBox>(*this); }
bool Box::isSvgTransformableContainerBox() const { return is<SvgTransformableContainerBox>(*this); }
bool Box::isSvgViewportContainerBox() const { return is<SvgViewportContainerBox>(*this); }
bool Box::isSvgResourceContainerBox() const { return is<SvgResourceContainerBox>(*this); }
bool Box::isSvgResourceMarkerBox() const { return is<SvgResourceMarkerBox>(*this); }
bool Box::isSvgResourceClipperBox() const { return is<SvgResourceClipperBox>(*this); }
bool Box::isSvgResourceMaskerBox() const { return is<SvgResourceMaskerBox>(*this); }
bool Box::isSvgResourcePaintServerBox() const { return is<SvgResourcePaintServerBox>(*this); }
bool Box::isSvgResourcePatternBox() const { return is<SvgResourcePatternBox>(*this); }
bool Box::isSvgGradientStopBox() const { return is<SvgGradientStopBox>(*this); }
bool Box::isSvgResourceGradientBox() const { return is<SvgResourceGradientBox>(*this); }
bool Box::isSvgResourceLinearGradientBox() const { return is<SvgResourceLinearGradientBox>(*this); }
bool Box::isSvgResourceRadialGradientBox() const { return is<SvgResourceRadialGradientBox>(*this); }

void Box::paintAnnotation(GraphicsContext& context, const Rect& rect) const
{
    if(m_node == nullptr || !m_node->isElementNode())
        return;
    const auto& element = to<Element>(*m_node);
    if(element.isLinkDestination())
        context.addLinkDestination(element.id(), rect.origin());
    if(element.isLinkSource()) {
        const auto& baseUrl = element.document()->baseUrl();
        auto completeUrl = element.getUrlAttribute(hrefAttr);
        auto fragmentName = completeUrl.fragment();
        if(!fragmentName.empty() && baseUrl == completeUrl.base()) {
            context.addLinkAnnotation(fragmentName.substr(1), emptyGlo, rect);
        } else {
            context.addLinkAnnotation(emptyGlo, completeUrl.value(), rect);
        }
    }
}

void Box::build()
{
    auto child = m_firstChild;
    while(child) {
        child->build();
        child = child->nextSibling();
    }
}

static void writeIndent(std::ostream& o, int indent)
{
    for(int i = 0; i < indent; i++) {
        o << ' ';
    }
}

static void writeNewline(std::ostream& o)
{
    o << '\n';
}

void Box::serializeStart(std::ostream& o, int indent, bool selfClosing, const Box* box, const LineBox* line)
{
    auto name = line ? line->name() : box->name();
    writeIndent(o, indent);
    o << '<' << name;
    auto element = to<Element>(box->node());
    if(element == nullptr) {
        switch(box->style()->pseudoType()) {
        case PseudoType::Before:
            o << "::before";
            break;
        case PseudoType::After:
            o << "::after";
            break;
        case PseudoType::Marker:
            o << "::marker";
            break;
        case PseudoType::FirstLetter:
            o << "::first-letter";
            break;
        default:
            break;
        }
    } else {
        o << ':' << element->tagName();
        const auto& id = element->id();
        if(!id.empty()) {
            o << '#' << id;
        }
    }

    if(box->isAnonymous())
        o << " anonymous";
    if(box->isPositioned() && !box->isBoxView()) {
        o << " positioned";
    } else if(box->isFloating()) {
        o << " floating";
    }

    auto rect = line ? line->rect() : box->paintBoundingBox();
    if(!rect.isEmpty()) {
        o << " rect=\'";
        o << rect.x;
        o << ' ';
        o << rect.y;
        o << ' ';
        o << rect.w;
        o << ' ';
        o << rect.h;
        o << '\'';
    }

    if(selfClosing) {
        o << "/>";
    } else {
        o << '>';
        if (!line || !is<TextLineBox>(*line)) {
            writeNewline(o);
        }
    }
}

void Box::serializeEnd(std::ostream& o, int indent, bool selfClosing, const Box* box, const LineBox* line)
{
    if(selfClosing) {
        writeNewline(o);
    } else {
        auto name = line ? line->name() : box->name();
        if(!line || !is<TextLineBox>(*line)) {
            writeIndent(o, indent);
        }

        o << "</" << name << ">\n";
    }
}

void Box::serialize(std::ostream& o, int indent) const
{
    serializeStart(o, indent, !m_firstChild, this, nullptr);
    serializeChildren(o, indent + 2);
    serializeEnd(o, indent, !m_firstChild, this, nullptr);
}

void Box::serializeChildren(std::ostream& o, int indent) const
{
    auto child = m_firstChild;
    while(child) {
        child->serialize(o, indent);
        child = child->nextSibling();
    }
}

BoxModel::BoxModel(ClassKind type, Node* node, const RefPtr<BoxStyle>& style)
    : Box(type, node, style)
{
    setIsInline(style->isDisplayInlineType());
}

BoxModel::~BoxModel() = default;

void BoxModel::addChild(Box* newChild)
{
    if(!newChild->isTableCellBox() && !newChild->isTableRowBox()
        && !newChild->isTableCaptionBox() && !newChild->isTableColumnBox()
        && !newChild->isTableSectionBox()) {
        appendChild(newChild);
        return;
    }

    auto lastTable = lastChild();
    if(lastTable && lastTable->isAnonymous() && lastTable->isTableBox()) {
        lastTable->addChild(newChild);
        return;
    }

    auto newTable = createAnonymous(Display::Table, style());
    appendChild(newTable);
    newTable->addChild(newChild);
}

static Size computeBackgroundImageIntrinsicSize(const RefPtr<Image>& backgroundImage, const Size& positioningAreaSize)
{
    float intrinsicWidth = 0;
    float intrinsicHeight = 0;
    double intrinsicRatio = 0;
    backgroundImage->computeIntrinsicDimensions(intrinsicWidth, intrinsicHeight, intrinsicRatio);
    if(intrinsicWidth > 0 && intrinsicHeight > 0) {
        return Size(intrinsicWidth, intrinsicHeight);
    }

    if(intrinsicWidth > 0 || intrinsicHeight > 0) {
        if(intrinsicRatio > 0) {
            if(intrinsicWidth > 0)
                return Size(intrinsicWidth, intrinsicWidth / intrinsicRatio);
            return Size(intrinsicHeight * intrinsicRatio, intrinsicHeight);
        }

        if(intrinsicWidth > 0)
            return Size(intrinsicWidth, positioningAreaSize.h);
        return Size(positioningAreaSize.w, intrinsicHeight);
    }

    if(intrinsicRatio > 0) {
        auto solutionWidth = positioningAreaSize.h * intrinsicRatio;
        auto solutionHeight = positioningAreaSize.w / intrinsicRatio;
        if(solutionWidth <= positioningAreaSize.w) {
            if(solutionHeight < positioningAreaSize.h) {
                auto areaOne = solutionWidth * positioningAreaSize.h;
                auto areaTwo = solutionHeight * positioningAreaSize.w;
                if(areaOne < areaTwo)
                    return Size(positioningAreaSize.w, solutionHeight);
                return Size(solutionWidth, positioningAreaSize.h);
            }

            return Size(solutionWidth, positioningAreaSize.h);
        }

        assert(solutionHeight <= positioningAreaSize.h);
        return Size(positioningAreaSize.w, solutionHeight);
    }

    return positioningAreaSize;
}

void BoxModel::paintBackgroundStyle(const PaintInfo& info, const Rect& borderRect, const BoxStyle* backgroundStyle, bool includeLeftEdge, bool includeRightEdge) const
{
    auto backgroundColor = backgroundStyle->backgroundColor();
    auto backgroundImage = backgroundStyle->backgroundImage();
    if(backgroundImage == nullptr && !backgroundColor.alpha())
        return;
    auto clipRect = style()->getBorderRoundedRect(borderRect, includeLeftEdge, includeRightEdge);
    auto backgroundClip = backgroundStyle->backgroundClip();
    if(backgroundClip == BackgroundBox::PaddingBox || backgroundClip == BackgroundBox::ContentBox) {
        auto topWidth = borderTop();
        auto rightWidth = borderRight();
        auto bottomWidth = borderBottom();
        auto leftWidth = borderLeft();
        if(backgroundClip == BackgroundBox::ContentBox) {
            topWidth += paddingTop();
            rightWidth += paddingRight();
            bottomWidth += paddingBottom();
            leftWidth += paddingLeft();
        }

        if(!includeLeftEdge)
            leftWidth = 0.f;
        if(!includeRightEdge)
            rightWidth = 0.f;
        clipRect.shrink(topWidth, rightWidth, bottomWidth, leftWidth);
    }

    if(!clipRect.rect().intersects(info.rect()))
        return;
    auto clipping = backgroundClip == BackgroundBox::PaddingBox || backgroundClip == BackgroundBox::ContentBox || clipRect.isRounded();
    if(clipping) {
        info->save();
        info->clipRoundedRect(clipRect);
    }

    info->setColor(backgroundColor);
    info->fillRect(borderRect);
    if(backgroundImage) {
        Rect positioningArea(0, 0, borderRect.w, borderRect.h);
        auto backgroundOrigin = backgroundStyle->backgroundOrigin();
        if(backgroundOrigin == BackgroundBox::PaddingBox || backgroundOrigin == BackgroundBox::ContentBox) {
            auto topWidth = borderTop();
            auto rightWidth = borderRight();
            auto bottomWidth = borderBottom();
            auto leftWidth = borderLeft();
            if(backgroundOrigin == BackgroundBox::ContentBox) {
                topWidth += paddingTop();
                rightWidth += paddingRight();
                bottomWidth += paddingBottom();
                leftWidth += paddingLeft();
            }

            positioningArea.shrink(topWidth, rightWidth, bottomWidth, leftWidth);
        }

        Rect tileRect;
        auto intrinsicSize = computeBackgroundImageIntrinsicSize(backgroundImage, positioningArea.size());
        auto backgroundSize = backgroundStyle->backgroundSize();
        switch(backgroundSize.type()) {
        case BackgroundSize::Type::Contain:
        case BackgroundSize::Type::Cover: {
            auto xScale = positioningArea.w / intrinsicSize.w;
            auto yScale = positioningArea.h / intrinsicSize.h;
            auto scale = backgroundSize.type() == BackgroundSize::Type::Contain ? std::min(xScale, yScale) : std::max(xScale, yScale);
            tileRect.w = intrinsicSize.w * scale;
            tileRect.h = intrinsicSize.h * scale;
            break;
        }

        case BackgroundSize::Type::Length:
            const auto& widthLength = backgroundSize.width();
            const auto& heightLength = backgroundSize.height();
            if(widthLength.isFixed())
                tileRect.w = widthLength.value();
            else if(widthLength.isPercent())
                tileRect.w = widthLength.calc(positioningArea.w);
            else {
                tileRect.w = positioningArea.w;
            }

            if(heightLength.isFixed())
                tileRect.h = heightLength.value();
            else if(heightLength.isPercent())
                tileRect.h = heightLength.calc(positioningArea.h);
            else {
                tileRect.h = positioningArea.h;
            }

            if(widthLength.isAuto() && !heightLength.isAuto()) {
                tileRect.w = intrinsicSize.w * tileRect.h / intrinsicSize.h;
            } else if(!widthLength.isAuto() && heightLength.isAuto()) {
                tileRect.h = intrinsicSize.h * tileRect.w / intrinsicSize.w;
            } else if(widthLength.isAuto() && heightLength.isAuto()) {
                tileRect.w = intrinsicSize.w;
                tileRect.h = intrinsicSize.h;
            }
        }

        auto backgroundPosition = backgroundStyle->backgroundPosition();
        const Point positionOffset = {
            backgroundPosition.x().calcMin(positioningArea.w - tileRect.w),
            backgroundPosition.y().calcMin(positioningArea.h - tileRect.h)
        };

        Rect destRect(borderRect);
        auto backgroundRepeat = backgroundStyle->backgroundRepeat();
        if(backgroundRepeat == BackgroundRepeat::Repeat || backgroundRepeat == BackgroundRepeat::RepeatX) {
            tileRect.x = tileRect.w - std::fmod(positionOffset.x + positioningArea.x, tileRect.w);
        } else {
            destRect.x += std::max(0.f, positionOffset.x + positioningArea.x);
            tileRect.x = -std::min(0.f, positionOffset.x + positioningArea.x);
            destRect.w = tileRect.w - tileRect.x;
        }

        if(backgroundRepeat == BackgroundRepeat::Repeat || backgroundRepeat == BackgroundRepeat::RepeatY) {
            tileRect.y = tileRect.h - std::fmod(positionOffset.y + positioningArea.y, tileRect.h);
        } else {
            destRect.y += std::max(0.f, positionOffset.y + positioningArea.y);
            tileRect.y = -std::min(0.f, positionOffset.y + positioningArea.y);
            destRect.h = tileRect.h - tileRect.y;
        }

        destRect.intersect(borderRect);
        if(destRect.intersects(info.rect())) {
            backgroundImage->setContainerSize(tileRect.size());
            backgroundImage->drawTiled(*info, destRect, tileRect);
        }
    }

    if(clipping) {
        info->restore();
    }
}

void BoxModel::paintBackground(const PaintInfo& info, const Rect& borderRect, bool includeLeftEdge, bool includeRightEdge) const
{
    if(!isBackgroundStolen()) {
        paintBackgroundStyle(info, borderRect, style(), includeLeftEdge, includeRightEdge);
    }
}

void BoxModel::paintBorder(const PaintInfo& info, const Rect& borderRect, bool includeLeftEdge, bool includeRightEdge) const
{
    BorderPainter::paintBorder(info, borderRect, style(), includeLeftEdge, includeRightEdge);
}

void BoxModel::paintOutline(const PaintInfo& info, const Rect& borderRect) const
{
    BorderPainter::paintOutline(info, borderRect, style());
    paintAnnotation(*info, borderRect);
}

void BoxModel::paint(const PaintInfo& info, const Point& offset, PaintPhase phase)
{
    assert(false);
}

void BoxModel::paintLayer(GraphicsContext& context, const Rect& rect)
{
    m_layer->paint(context, rect);
}

void BoxModel::updateLayerPosition()
{
    m_layer->updatePosition();
}

float BoxModel::relativePositionOffsetX() const
{
    auto container = containingBlock();

    auto leftLength = style()->left();
    auto rightLength = style()->right();

    auto availableWidth = containingBlockWidthForContent(container);
    if(!leftLength.isAuto()) {
        if(!rightLength.isAuto() && container->style()->isRightToLeftDirection())
            return -rightLength.calc(availableWidth);
        return leftLength.calc(availableWidth);
    }

    if(!rightLength.isAuto())
        return -rightLength.calc(availableWidth);
    return 0;
}

float BoxModel::relativePositionOffsetY() const
{
    auto container = containingBlock();

    auto topLength = style()->top();
    auto bottomLength = style()->bottom();

    auto availableHeight = containingBlockHeightForContent(container);
    if(!topLength.isAuto() && (availableHeight || !topLength.isPercent()))
        return topLength.calc(availableHeight.value_or(0.f));
    if(!bottomLength.isAuto() && (availableHeight || !bottomLength.isPercent()))
        return -bottomLength.calc(availableHeight.value_or(0.f));
    return 0;
}

Point BoxModel::relativePositionOffset() const
{
    auto xOffset = relativePositionOffsetX();
    auto yOffset = relativePositionOffsetY();

    return Point(xOffset, yOffset);
}

float BoxModel::containingBlockWidthForPositioned(const BoxModel* container) const
{
    if(container->isBoxView())
        return document()->containerWidth();
    if(auto box = to<BoxFrame>(container))
        return box->paddingBoxWidth();
    return to<InlineBox>(*container).innerPaddingBoxWidth();
}

float BoxModel::containingBlockHeightForPositioned(const BoxModel* container) const
{
    if(container->isBoxView())
        return document()->containerHeight();
    if(auto box = to<BoxFrame>(container))
        return box->paddingBoxHeight();
    return to<InlineBox>(*container).innerPaddingBoxHeight();
}

float BoxModel::containingBlockWidthForContent(const BlockBox* container) const
{
    if(container)
        return container->availableWidth();
    return 0.f;
}

std::optional<float> BoxModel::containingBlockHeightForContent(const BlockBox* container) const
{
    return container->availableHeight();
}

void BoxModel::updateVerticalMargins(const BlockBox* container)
{
    auto containerWidth = containingBlockWidthForContent(container);
    m_marginTop = style()->marginTop().calcMin(containerWidth);
    m_marginBottom = style()->marginBottom().calcMin(containerWidth);
}

void BoxModel::updateHorizontalMargins(const BlockBox* container)
{
    auto containerWidth = containingBlockWidthForContent(container);
    m_marginLeft = style()->marginLeft().calcMin(containerWidth);
    m_marginRight = style()->marginRight().calcMin(containerWidth);
}

void BoxModel::updateMarginWidths(const BlockBox* container)
{
    updateVerticalMargins(container);
    updateHorizontalMargins(container);
}

void BoxModel::updateVerticalPaddings(const BlockBox* container)
{
    auto containerWidth = containingBlockWidthForContent(container);
    m_paddingTop = style()->paddingTop().calcMin(containerWidth);
    m_paddingBottom = style()->paddingBottom().calcMin(containerWidth);
}

void BoxModel::updateHorizontalPaddings(const BlockBox* container)
{
    auto containerWidth = containingBlockWidthForContent(container);
    m_paddingLeft = style()->paddingLeft().calcMin(containerWidth);
    m_paddingRight = style()->paddingRight().calcMin(containerWidth);
}

void BoxModel::updatePaddingWidths(const BlockBox* container)
{
    updateVerticalPaddings(container);
    updateHorizontalPaddings(container);
}

void BoxModel::computeBorderWidths(float& borderTop, float& borderBottom, float& borderLeft, float& borderRight) const
{
    auto calc = [](LineStyle style, float width) {
        if(style > LineStyle::Hidden)
            return width;
        return 0.f;
    };

    borderTop = calc(style()->borderTopStyle(), style()->borderTopWidth());
    borderBottom = calc(style()->borderBottomStyle(), style()->borderBottomWidth());
    borderLeft = calc(style()->borderLeftStyle(), style()->borderLeftWidth());
    borderRight = calc(style()->borderRightStyle(), style()->borderRightWidth());
}

float BoxModel::borderTop() const
{
    if(m_borderTop < 0)
        computeBorderWidths(m_borderTop, m_borderBottom, m_borderLeft, m_borderRight);
    return m_borderTop;
}

float BoxModel::borderBottom() const
{
    if(m_borderBottom < 0)
        computeBorderWidths(m_borderTop, m_borderBottom, m_borderLeft, m_borderRight);
    return m_borderBottom;
}

float BoxModel::borderLeft() const
{
    if(m_borderLeft < 0)
        computeBorderWidths(m_borderTop, m_borderBottom, m_borderLeft, m_borderRight);
    return m_borderLeft;
}

float BoxModel::borderRight() const
{
    if(m_borderRight < 0)
        computeBorderWidths(m_borderTop, m_borderBottom, m_borderLeft, m_borderRight);
    return m_borderRight;
}

void BoxModel::build()
{
    if(requiresLayer()) {
        assert(!hasLayer());
        m_layer = BoxLayer::create(this, enclosingLayer());
        setHasLayer(true);
    }

    Box::build();
}

BoxFrame::BoxFrame(ClassKind type, Node* node, const RefPtr<BoxStyle>& style)
    : BoxModel(type, node, style)
{
    setHasTransform(style->hasTransform());
    switch(style->position()) {
    case Position::Static:
    case Position::Relative:
        setIsPositioned(false);
        break;
    default:
        setIsPositioned(true);
        break;
    }

    setIsOverflowHidden(style->isOverflowHidden());
    switch(style->floating()) {
    case Float::None:
        setIsFloating(false);
        break;
    default:
        setIsFloating(true);
        break;
    }
}

bool BoxFrame::requiresLayer() const
{
    return isPositioned() || isRelativePositioned() || isOverflowHidden() || hasTransform() || hasColumnFlowBox()
        || style()->hasOpacity() || style()->hasBlendMode() || style()->zIndex();
}

BoxFrame::~BoxFrame() = default;

void BoxFrame::setLine(std::unique_ptr<ReplacedLineBox> line)
{
    m_line = std::move(line);
}

void BoxFrame::computePreferredWidths(float& minPreferredWidth, float& maxPreferredWidth) const
{
    assert(false);
}

float BoxFrame::minPreferredWidth() const
{
    if(m_minPreferredWidth < 0)
        computePreferredWidths(m_minPreferredWidth, m_maxPreferredWidth);
    return m_minPreferredWidth;
}

float BoxFrame::maxPreferredWidth() const
{
    if(m_maxPreferredWidth < 0)
        computePreferredWidths(m_minPreferredWidth, m_maxPreferredWidth);
    return m_maxPreferredWidth;
}

float BoxFrame::adjustBorderBoxWidth(float width) const
{
    if(style()->boxSizing() == BoxSizing::ContentBox)
        return width + borderAndPaddingWidth();
    return std::max(width, borderAndPaddingWidth());
}

float BoxFrame::adjustBorderBoxHeight(float height) const
{
    if(style()->boxSizing() == BoxSizing::ContentBox)
        return height + borderAndPaddingHeight();
    return std::max(height, borderAndPaddingHeight());
}

float BoxFrame::adjustContentBoxWidth(float width) const
{
    if(style()->boxSizing() == BoxSizing::BorderBox)
        width -= borderAndPaddingWidth();
    return std::max(0.f, width);
}

float BoxFrame::adjustContentBoxHeight(float height) const
{
    if(style()->boxSizing() == BoxSizing::BorderBox)
        height -= borderAndPaddingHeight();
    return std::max(0.f, height);
}

void BoxFrame::computeHorizontalStaticDistance(Length& leftLength, Length& rightLength, const BoxModel* container, float containerWidth) const
{
    if(!leftLength.isAuto() || !rightLength.isAuto())
        return;
    auto parent = parentBox();
    if(parent->style()->direction() == Direction::Ltr) {
        auto staticPosition = layer()->staticLeft() - container->borderLeft();
        for(; parent && parent != container; parent = parent->containingBox()) {
            if(auto box = to<BoxFrame>(parent)) {
                staticPosition += box->x();
                if(box->isRelativePositioned()) {
                    staticPosition += box->relativePositionOffsetX();
                }
            }
        }

        leftLength = Length(Length::Type::Fixed, staticPosition);
    } else {
        auto staticPosition = layer()->staticLeft() + containerWidth + container->borderRight();
        while(parent && !parent->isBoxFrame())
            parent = parent->parentBox();
        if(auto box = to<BoxFrame>(parent))
            staticPosition -= box->width();
        for(; parent && parent != container; parent = parent->containingBox()) {
            if(auto box = to<BoxFrame>(parent)) {
                staticPosition -= box->x();
                if(box->isRelativePositioned()) {
                    staticPosition -= box->relativePositionOffsetX();
                }
            }
        }

        rightLength = Length(Length::Type::Fixed, staticPosition);
    }
}

void BoxFrame::computeVerticalStaticDistance(Length& topLength, Length& bottomLength, const BoxModel* container) const
{
    if(!topLength.isAuto() || !bottomLength.isAuto())
        return;
    auto staticTop = layer()->staticTop() - container->borderTop();
    for(auto parent = parentBox(); parent && parent != container; parent = parent->containingBox()) {
        if(auto box = to<BoxFrame>(parent)) {
            staticTop += box->y();
            if(box->isRelativePositioned()) {
                staticTop += box->relativePositionOffsetY();
            }
        }
    }

    topLength = Length(Length::Type::Fixed, staticTop);
}

void BoxFrame::computeHorizontalMargins(float& marginLeft, float& marginRight, float childWidth, const BlockBox* container, float containerWidth) const
{
    if(isFlexItem() || isTableCellBox())
        return;
    auto marginLeftLength = style()->marginLeft();
    auto marginRightLength = style()->marginRight();
    if(isInline() || isFloating()) {
        marginLeft = marginLeftLength.calcMin(containerWidth);
        marginRight = marginRightLength.calcMin(containerWidth);
        return;
    }

    auto containerBlock = to<BlockFlowBox>(container);
    if(containerBlock && containerBlock->containsFloats() && avoidsFloats())
        containerWidth = containerBlock->availableWidthForLine(y(), false);
    if(childWidth < containerWidth) {
        if(marginLeftLength.isAuto() && marginRightLength.isAuto()) {
            marginLeft = std::max(0.f, (containerWidth - childWidth) / 2.f);
            marginRight = containerWidth - childWidth - marginLeft;
            return;
        }

        if(marginRightLength.isAuto()) {
            marginLeft = marginLeftLength.calc(containerWidth);
            marginRight = containerWidth - childWidth - marginLeft;
            return;
        }

        if(marginLeftLength.isAuto()) {
            marginRight = marginRightLength.calc(containerWidth);
            marginLeft = containerWidth - childWidth - marginRight;
            return;
        }
    }

    marginLeft = marginLeftLength.calcMin(containerWidth);
    marginRight = marginRightLength.calcMin(containerWidth);
}

void BoxFrame::computeVerticalMargins(float& marginTop, float& marginBottom) const
{
    if(isFlexItem() || isTableCellBox())
        return;
    auto containerWidth = containingBlockWidthForContent();
    marginTop = style()->marginTop().calcMin(containerWidth);
    marginBottom = style()->marginBottom().calcMin(containerWidth);
}

float BoxFrame::computeIntrinsicWidthUsing(const Length& widthLength, float containerWidth) const
{
    if(widthLength.isMinContent())
        return minPreferredWidth();
    if(widthLength.isMaxContent()) {
        return maxPreferredWidth();
    }

    assert(widthLength.isFitContent());
    auto marginLeft = style()->marginLeft().calcMin(containerWidth);
    auto marginRight = style()->marginRight().calcMin(containerWidth);
    auto width = containerWidth - marginLeft - marginRight;
    return std::max(minPreferredWidth(), std::min(width, maxPreferredWidth()));
}

void BoxFrame::computeWidth(float& x, float& width, float& marginLeft, float& marginRight) const
{
    assert(false);
}

void BoxFrame::computeHeight(float& y, float& height, float& marginTop, float& marginBottom) const
{
    assert(false);
}

void BoxFrame::updateWidth()
{
    computeWidth(m_x, m_width, m_marginLeft, m_marginRight);
}

void BoxFrame::updateHeight()
{
    computeHeight(m_y, m_height, m_marginTop, m_marginBottom);
}

float BoxFrame::maxMarginTop(bool positive) const
{
    return positive ? std::max(0.f, m_marginTop) : -std::min(0.f, m_marginTop);
}

float BoxFrame::maxMarginBottom(bool positive) const
{
    return positive ? std::max(0.f, m_marginBottom) : -std::min(0.f, m_marginBottom);
}

float BoxFrame::collapsedMarginTop() const
{
    return maxMarginTop(true) - maxMarginTop(false);
}

float BoxFrame::collapsedMarginBottom() const
{
    return maxMarginBottom(true) - maxMarginBottom(false);
}

void BoxFrame::updateOverflowRect()
{
    Rect borderRect(0, 0, m_width, m_height);
    auto outlineEdge = style()->getOutlineEdge();
    if(outlineEdge.isRenderable()) {
        borderRect.inflate(outlineEdge.width() + style()->outlineOffset());
    }

    m_overflowTop = std::min(0.f, borderRect.y);
    m_overflowBottom = std::max(m_height, borderRect.bottom());
    m_overflowLeft = std::min(0.f, borderRect.x);
    m_overflowRight = std::max(m_width, borderRect.right());
}

void BoxFrame::addOverflowRect(const BoxFrame* child, float dx, float dy)
{
    if(child->hasLayer())
        return;
    auto overflowRect = child->visualOverflowRect();
    overflowRect.translate(dx, dy);
    addOverflowRect(overflowRect);
}

void BoxFrame::addOverflowRect(float top, float bottom, float left, float right)
{
    m_overflowTop = std::min(top, m_overflowTop);
    m_overflowBottom = std::max(bottom, m_overflowBottom);
    m_overflowLeft = std::min(left, m_overflowLeft);
    m_overflowRight = std::max(right, m_overflowRight);
}

void BoxFrame::addOverflowRect(const Rect& overflowRect)
{
    addOverflowRect(overflowRect.y, overflowRect.bottom(), overflowRect.x, overflowRect.right());
}

void BoxFrame::paintOutlines(const PaintInfo& info, const Point& offset)
{
    Rect borderRect(offset, size());
    paintOutline(info, borderRect);
}

void BoxFrame::paintDecorations(const PaintInfo& info, const Point& offset)
{
    Rect borderRect(offset, size());
    paintBackground(info, borderRect);
    paintBorder(info, borderRect);
}

void BoxFrame::layout(FragmentBuilder* fragmentainer)
{
    assert(false);
}

} // namespace plutobook
