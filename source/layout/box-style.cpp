#include "box-style.h"
#include "css-rule.h"
#include "document.h"
#include "image-resource.h"
#include "font-resource.h"
#include "geometry.h"

#include "plutobook.hpp"

namespace plutobook {

const Length Length::None(Length::Type::None);
const Length Length::Auto(Length::Type::Auto);
const Length Length::MinContent(Length::Type::MinContent);
const Length Length::MaxContent(Length::Type::MaxContent);
const Length Length::FitContent(Length::Type::FitContent);
const Length Length::ZeroFixed(Length::Type::Fixed);
const Length Length::ZeroPercent(Length::Type::Percent);

RefPtr<BoxStyle> BoxStyle::create(Node* node, PseudoType pseudoType, Display display)
{
    return adoptPtr(new (node->heap()) BoxStyle(node, pseudoType, display));
}

RefPtr<BoxStyle> BoxStyle::create(Node* node, const BoxStyle* parentStyle, PseudoType pseudoType, Display display)
{
    auto newStyle = create(node, pseudoType, display);
    newStyle->inheritFrom(parentStyle);
    return newStyle;
}

RefPtr<BoxStyle> BoxStyle::create(const BoxStyle* parentStyle, PseudoType pseudoType, Display display)
{
    return create(parentStyle->node(), parentStyle, pseudoType, display);
}

RefPtr<BoxStyle> BoxStyle::create(const BoxStyle* parentStyle, Display display)
{
    return create(parentStyle, PseudoType::None, display);
}

Document* BoxStyle::document() const
{
    return m_node->document();
}

Heap* BoxStyle::heap() const
{
    return m_node->heap();
}

Book* BoxStyle::book() const
{
    return document()->book();
}

void BoxStyle::setFont(RefPtr<Font> font)
{
    m_font = std::move(font);
}

float BoxStyle::fontAscent() const
{
    if(auto fontData = m_font->primaryFont())
        return fontData->ascent();
    return 0.f;
}

float BoxStyle::fontDescent() const
{
    if(auto fontData = m_font->primaryFont())
        return fontData->descent();
    return 0.f;
}

float BoxStyle::fontHeight() const
{
    if(auto fontData = m_font->primaryFont())
        return fontData->height();
    return 0.f;
}

float BoxStyle::fontLineGap() const
{
    if(auto fontData = m_font->primaryFont())
        return fontData->lineGap();
    return 0.f;
}

float BoxStyle::fontLineSpacing() const
{
    if(auto fontData = m_font->primaryFont())
        return fontData->lineSpacing();
    return 0.f;
}

const FontDescription& BoxStyle::fontDescription() const
{
    return m_font->description();
}

void BoxStyle::setFontDescription(const FontDescription& description)
{
    if(m_font && description == m_font->description())
        return;
    m_font = document()->createFont(description);
}

float BoxStyle::fontSize() const
{
    return m_font->size();
}

float BoxStyle::fontWeight() const
{
    return m_font->weight();
}

float BoxStyle::fontStretch() const
{
    return m_font->stretch();
}

float BoxStyle::fontStyle() const
{
    return m_font->style();
}

const FontFamilyList& BoxStyle::fontFamily() const
{
    return m_font->family();
}

const FontVariationList& BoxStyle::fontVariationSettings() const
{
    return m_font->variationSettings();
}

Length BoxStyle::left() const
{
    auto value = get(CssPropertyID::Left);
    if(value == nullptr)
        return Length::Auto;
    return convertLengthOrPercentOrAuto(*value);
}

Length BoxStyle::right() const
{
    auto value = get(CssPropertyID::Right);
    if(value == nullptr)
        return Length::Auto;
    return convertLengthOrPercentOrAuto(*value);
}

Length BoxStyle::top() const
{
    auto value = get(CssPropertyID::Top);
    if(value == nullptr)
        return Length::Auto;
    return convertLengthOrPercentOrAuto(*value);
}

Length BoxStyle::bottom() const
{
    auto value = get(CssPropertyID::Bottom);
    if(value == nullptr)
        return Length::Auto;
    return convertLengthOrPercentOrAuto(*value);
}

Length BoxStyle::width() const
{
    auto value = get(CssPropertyID::Width);
    if(value == nullptr)
        return Length::Auto;
    return convertWidthOrHeightLength(*value);
}

Length BoxStyle::height() const
{
    auto value = get(CssPropertyID::Height);
    if(value == nullptr)
        return Length::Auto;
    return convertWidthOrHeightLength(*value);
}

Length BoxStyle::minWidth() const
{
    auto value = get(CssPropertyID::MinWidth);
    if(value == nullptr)
        return Length::Auto;
    return convertWidthOrHeightLength(*value);
}

Length BoxStyle::minHeight() const
{
    auto value = get(CssPropertyID::MinHeight);
    if(value == nullptr)
        return Length::Auto;
    return convertWidthOrHeightLength(*value);
}

Length BoxStyle::maxWidth() const
{
    auto value = get(CssPropertyID::MaxWidth);
    if(value == nullptr)
        return Length::None;
    return convertWidthOrHeightLength(*value);
}

Length BoxStyle::maxHeight() const
{
    auto value = get(CssPropertyID::MaxHeight);
    if(value == nullptr)
        return Length::None;
    return convertWidthOrHeightLength(*value);
}

Length BoxStyle::marginLeft() const
{
    auto value = get(CssPropertyID::MarginLeft);
    if(value == nullptr)
        return Length::ZeroFixed;
    return convertLengthOrPercentOrAuto(*value);
}

Length BoxStyle::marginRight() const
{
    auto value = get(CssPropertyID::MarginRight);
    if(value == nullptr)
        return Length::ZeroFixed;
    return convertLengthOrPercentOrAuto(*value);
}

Length BoxStyle::marginTop() const
{
    auto value = get(CssPropertyID::MarginTop);
    if(value == nullptr)
        return Length::ZeroFixed;
    return convertLengthOrPercentOrAuto(*value);
}

Length BoxStyle::marginBottom() const
{
    auto value = get(CssPropertyID::MarginBottom);
    if(value == nullptr)
        return Length::ZeroFixed;
    return convertLengthOrPercentOrAuto(*value);
}

Length BoxStyle::paddingLeft() const
{
    auto value = get(CssPropertyID::PaddingLeft);
    if(value == nullptr)
        return Length::ZeroFixed;
    return convertLengthOrPercent(*value);
}

Length BoxStyle::paddingRight() const
{
    auto value = get(CssPropertyID::PaddingRight);
    if(value == nullptr)
        return Length::ZeroFixed;
    return convertLengthOrPercent(*value);
}

Length BoxStyle::paddingTop() const
{
    auto value = get(CssPropertyID::PaddingTop);
    if(value == nullptr)
        return Length::ZeroFixed;
    return convertLengthOrPercent(*value);
}

Length BoxStyle::paddingBottom() const
{
    auto value = get(CssPropertyID::PaddingBottom);
    if(value == nullptr)
        return Length::ZeroFixed;
    return convertLengthOrPercent(*value);
}

LineStyle BoxStyle::borderLeftStyle() const
{
    auto value = get(CssPropertyID::BorderLeftStyle);
    if(value == nullptr)
        return LineStyle::None;
    return convertLineStyle(*value);
}

LineStyle BoxStyle::borderRightStyle() const
{
    auto value = get(CssPropertyID::BorderRightStyle);
    if(value == nullptr)
        return LineStyle::None;
    return convertLineStyle(*value);
}

LineStyle BoxStyle::borderTopStyle() const
{
    auto value = get(CssPropertyID::BorderTopStyle);
    if(value == nullptr)
        return LineStyle::None;
    return convertLineStyle(*value);
}

LineStyle BoxStyle::borderBottomStyle() const
{
    auto value = get(CssPropertyID::BorderBottomStyle);
    if(value == nullptr)
        return LineStyle::None;
    return convertLineStyle(*value);
}

Color BoxStyle::borderLeftColor() const
{
    auto value = get(CssPropertyID::BorderLeftColor);
    if(value == nullptr)
        return m_color;
    return convertColor(*value);
}

Color BoxStyle::borderRightColor() const
{
    auto value = get(CssPropertyID::BorderRightColor);
    if(value == nullptr)
        return m_color;
    return convertColor(*value);
}

Color BoxStyle::borderTopColor() const
{
    auto value = get(CssPropertyID::BorderTopColor);
    if(value == nullptr)
        return m_color;
    return convertColor(*value);
}

Color BoxStyle::borderBottomColor() const
{
    auto value = get(CssPropertyID::BorderBottomColor);
    if(value == nullptr)
        return m_color;
    return convertColor(*value);
}

float BoxStyle::borderLeftWidth() const
{
    auto value = get(CssPropertyID::BorderLeftWidth);
    if(value == nullptr)
        return 3.0;
    return convertLineWidth(*value);
}

float BoxStyle::borderRightWidth() const
{
    auto value = get(CssPropertyID::BorderRightWidth);
    if(value == nullptr)
        return 3.0;
    return convertLineWidth(*value);
}

float BoxStyle::borderTopWidth() const
{
    auto value = get(CssPropertyID::BorderTopWidth);
    if(value == nullptr)
        return 3.0;
    return convertLineWidth(*value);
}

float BoxStyle::borderBottomWidth() const
{
    auto value = get(CssPropertyID::BorderBottomWidth);
    if(value == nullptr)
        return 3.0;
    return convertLineWidth(*value);
}

void BoxStyle::getBorderEdgeInfo(BorderEdge edges[], bool includeLeftEdge, bool includeRightEdge) const
{
    edges[BoxSideTop] = BorderEdge(borderTopWidth(), borderTopColor(), borderTopStyle());
    if(includeRightEdge) {
        edges[BoxSideRight] = BorderEdge(borderRightWidth(), borderRightColor(), borderRightStyle());
    }

    edges[BoxSideBottom] = BorderEdge(borderBottomWidth(), borderBottomColor(), borderBottomStyle());
    if(includeLeftEdge) {
        edges[BoxSideLeft] = BorderEdge(borderLeftWidth(), borderLeftColor(), borderLeftStyle());
    }
}

LengthSize BoxStyle::borderTopLeftRadius() const
{
    auto value = get(CssPropertyID::BorderTopLeftRadius);
    if(value == nullptr)
        return LengthSize(Length::ZeroFixed);
    return convertBorderRadius(*value);
}

LengthSize BoxStyle::borderTopRightRadius() const
{
    auto value = get(CssPropertyID::BorderTopRightRadius);
    if(value == nullptr)
        return LengthSize(Length::ZeroFixed);
    return convertBorderRadius(*value);
}

LengthSize BoxStyle::borderBottomLeftRadius() const
{
    auto value = get(CssPropertyID::BorderBottomLeftRadius);
    if(value == nullptr)
        return LengthSize(Length::ZeroFixed);
    return convertBorderRadius(*value);
}

LengthSize BoxStyle::borderBottomRightRadius() const
{
    auto value = get(CssPropertyID::BorderBottomRightRadius);
    if(value == nullptr)
        return LengthSize(Length::ZeroFixed);
    return convertBorderRadius(*value);
}

RoundedRect BoxStyle::getBorderRoundedRect(const Rect& borderRect, bool includeLeftEdge, bool includeRightEdge) const
{
    auto calc = [&borderRect](const LengthSize& size) {
        return Size(size.width().calc(borderRect.w), size.height().calc(borderRect.h));
    };

    RectRadii borderRadii;
    if(includeLeftEdge) {
        borderRadii.tl = calc(borderTopLeftRadius());
        borderRadii.bl = calc(borderBottomLeftRadius());
    }

    if(includeRightEdge) {
        borderRadii.tr = calc(borderTopRightRadius());
        borderRadii.br = calc(borderBottomRightRadius());
    }

    borderRadii.constrain(borderRect.w, borderRect.h);
    return RoundedRect(borderRect, borderRadii);
}

ListStylePosition BoxStyle::listStylePosition() const
{
    auto value = get(CssPropertyID::ListStylePosition);
    if(value == nullptr)
        return ListStylePosition::Outside;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Inside:
        return ListStylePosition::Inside;
    case CssValueID::Outside:
        return ListStylePosition::Outside;
    default:
        assert(false);
    }

    return ListStylePosition::Outside;
}

RefPtr<Image> BoxStyle::listStyleImage() const
{
    auto value = get(CssPropertyID::ListStyleImage);
    if(value == nullptr)
        return nullptr;
    return convertImageOrNone(*value);
}

RefPtr<Image> BoxStyle::backgroundImage() const
{
    auto value = get(CssPropertyID::BackgroundImage);
    if(value == nullptr)
        return nullptr;
    return convertImageOrNone(*value);
}

Color BoxStyle::backgroundColor() const
{
    auto value = get(CssPropertyID::BackgroundColor);
    if(value == nullptr)
        return Color::Transparent;
    return convertColor(*value);
}

BackgroundRepeat BoxStyle::backgroundRepeat() const
{
    auto value = get(CssPropertyID::BackgroundRepeat);
    if(value == nullptr)
        return BackgroundRepeat::Repeat;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Repeat:
        return BackgroundRepeat::Repeat;
    case CssValueID::RepeatX:
        return BackgroundRepeat::RepeatX;
    case CssValueID::RepeatY:
        return BackgroundRepeat::RepeatY;
    case CssValueID::NoRepeat:
        return BackgroundRepeat::NoRepeat;
    default:
        assert(false);
    }

    return BackgroundRepeat::Repeat;
}

BackgroundBox BoxStyle::backgroundOrigin() const
{
    auto value = get(CssPropertyID::BackgroundOrigin);
    if(value == nullptr)
        return BackgroundBox::PaddingBox;
    return convertBackgroundBox(*value);
}

BackgroundBox BoxStyle::backgroundClip() const
{
    auto value = get(CssPropertyID::BackgroundClip);
    if(value == nullptr)
        return BackgroundBox::BorderBox;
    return convertBackgroundBox(*value);
}

BackgroundAttachment BoxStyle::backgroundAttachment() const
{
    auto value = get(CssPropertyID::BackgroundAttachment);
    if(value == nullptr)
        return BackgroundAttachment::Scroll;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Scroll:
        return BackgroundAttachment::Scroll;
    case CssValueID::Fixed:
        return BackgroundAttachment::Fixed;
    case CssValueID::Local:
        return BackgroundAttachment::Local;
    default:
        assert(false);
    }

    return BackgroundAttachment::Scroll;
}

BackgroundSize BoxStyle::backgroundSize() const
{
    auto value = get(CssPropertyID::BackgroundSize);
    if(value == nullptr)
        return BackgroundSize();
    if(auto ident = to<CssIdentValue>(value)) {
        switch(ident->value()) {
        case CssValueID::Contain:
            return BackgroundSize(BackgroundSize::Type::Contain);
        case CssValueID::Cover:
            return BackgroundSize(BackgroundSize::Type::Cover);
        default:
            assert(false);
        }
    }

    const auto& pair = to<CssPairValue>(*value);
    auto width = convertLengthOrPercentOrAuto(*pair.first());
    auto height = convertLengthOrPercentOrAuto(*pair.second());
    return BackgroundSize(width, height);
}

LengthPoint BoxStyle::backgroundPosition() const
{
    auto value = get(CssPropertyID::BackgroundPosition);
    if(value == nullptr)
        return LengthPoint(Length::ZeroFixed);
    return convertPositionCoordinate(*value);
}

ObjectFit BoxStyle::objectFit() const
{
    auto value = get(CssPropertyID::ObjectFit);
    if(value == nullptr)
        return ObjectFit::Fill;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Fill:
        return ObjectFit::Fill;
    case CssValueID::Contain:
        return ObjectFit::Contain;
    case CssValueID::Cover:
        return ObjectFit::Cover;
    case CssValueID::None:
        return ObjectFit::None;
    case CssValueID::ScaleDown:
        return ObjectFit::ScaleDown;
    default:
        assert(false);
    }

    return ObjectFit::Fill;
}

LengthPoint BoxStyle::objectPosition() const
{
    auto value = get(CssPropertyID::ObjectPosition);
    if(value == nullptr)
        return LengthPoint(Length(Length::Type::Percent, 50.f));
    return convertPositionCoordinate(*value);
}

TableLayout BoxStyle::tableLayout() const
{
    auto value = get(CssPropertyID::TableLayout);
    if(value == nullptr)
        return TableLayout::Auto;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Auto:
        return TableLayout::Auto;
    case CssValueID::Fixed:
        return TableLayout::Fixed;
    default:
        assert(false);
    }

    return TableLayout::Auto;
}

float BoxStyle::borderHorizontalSpacing() const
{
    auto value = get(CssPropertyID::BorderHorizontalSpacing);
    if(value == nullptr)
        return 0.0;
    return convertLengthValue(*value);
}

float BoxStyle::borderVerticalSpacing() const
{
    auto value = get(CssPropertyID::BorderVerticalSpacing);
    if(value == nullptr)
        return 0.0;
    return convertLengthValue(*value);
}

TextAnchor BoxStyle::textAnchor() const
{
    auto value = get(CssPropertyID::TextAnchor);
    if(value == nullptr)
        return TextAnchor::Start;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Start:
        return TextAnchor::Start;
    case CssValueID::Middle:
        return TextAnchor::Middle;
    case CssValueID::End:
        return TextAnchor::End;
    default:
        assert(false);
    }

    return TextAnchor::Start;
}

TextTransform BoxStyle::textTransform() const
{
    auto value = get(CssPropertyID::TextTransform);
    if(value == nullptr)
        return TextTransform::None;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::None:
        return TextTransform::None;
    case CssValueID::Capitalize:
        return TextTransform::Capitalize;
    case CssValueID::Uppercase:
        return TextTransform::Uppercase;
    case CssValueID::Lowercase:
        return TextTransform::Lowercase;
    default:
        assert(false);
    }

    return TextTransform::None;
}

TextOverflow BoxStyle::textOverflow() const
{
    auto value = get(CssPropertyID::TextOverflow);
    if(value == nullptr)
        return TextOverflow::Clip;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Clip:
        return TextOverflow::Clip;
    case CssValueID::Ellipsis:
        return TextOverflow::Ellipsis;
    default:
        assert(false);
    }

    return TextOverflow::Clip;
}

constexpr TextDecorationLine operator|(TextDecorationLine a, TextDecorationLine b)
{
    return static_cast<TextDecorationLine>(static_cast<int>(a) | static_cast<int>(b));
}

constexpr TextDecorationLine& operator|=(TextDecorationLine& a, TextDecorationLine b)
{
    return a = a | b;
}

TextDecorationLine BoxStyle::textDecorationLine() const
{
    auto value = get(CssPropertyID::TextDecorationLine);
    if(value == nullptr || value->id() == CssValueID::None)
        return TextDecorationLine::None;
    TextDecorationLine decorations = TextDecorationLine::None;
    for(const auto& decoration : to<CssListValue>(*value)) {
        const auto& ident = to<CssIdentValue>(*decoration);
        switch(ident.value()) {
        case CssValueID::Underline:
            decorations |= TextDecorationLine::Underline;
            break;
        case CssValueID::Overline:
            decorations |= TextDecorationLine::Overline;
            break;
        case CssValueID::LineThrough:
            decorations |= TextDecorationLine::LineThrough;
            break;
        default:
            assert(false);
        }
    }

    return decorations;
}

TextDecorationStyle BoxStyle::textDecorationStyle() const
{
    auto value = get(CssPropertyID::TextDecorationStyle);
    if(value == nullptr)
        return TextDecorationStyle::Solid;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Solid:
        return TextDecorationStyle::Solid;
    case CssValueID::Double:
        return TextDecorationStyle::Double;
    case CssValueID::Dotted:
        return TextDecorationStyle::Dotted;
    case CssValueID::Dashed:
        return TextDecorationStyle::Dashed;
    case CssValueID::Wavy:
        return TextDecorationStyle::Wavy;
    default:
        assert(false);
    }

    return TextDecorationStyle::Solid;
}

Color BoxStyle::textDecorationColor() const
{
    auto value = get(CssPropertyID::TextDecorationColor);
    if(value == nullptr)
        return m_color;
    return convertColor(*value);
}

FontVariantEmoji BoxStyle::fontVariantEmoji() const
{
    auto value = get(CssPropertyID::FontVariantEmoji);
    if(value == nullptr)
        return FontVariantEmoji::Normal;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Normal:
        return FontVariantEmoji::Normal;
    case CssValueID::Unicode:
        return FontVariantEmoji::Unicode;
    case CssValueID::Emoji:
        return FontVariantEmoji::Emoji;
    case CssValueID::Text:
        return FontVariantEmoji::Text;
    default:
        assert(false);
    }

    return FontVariantEmoji::Normal;
}

Hyphens BoxStyle::hyphens() const
{
    auto value = get(CssPropertyID::Hyphens);
    if(value == nullptr)
        return Hyphens::Manual;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::None:
        return Hyphens::None;
    case CssValueID::Auto:
        return Hyphens::Auto;
    case CssValueID::Manual:
        return Hyphens::Manual;
    default:
        assert(false);
    }

    return Hyphens::Manual;
}

Length BoxStyle::textIndent() const
{
    auto value = get(CssPropertyID::TextIndent);
    if(value == nullptr)
        return Length::ZeroFixed;
    return convertLengthOrPercent(*value);
}

float BoxStyle::letterSpacing() const
{
    auto value = get(CssPropertyID::LetterSpacing);
    if(value == nullptr)
        return 0.f;
    return convertSpacing(*value);
}

float BoxStyle::wordSpacing() const
{
    auto value = get(CssPropertyID::WordSpacing);
    if(value == nullptr)
        return 0.f;
    return convertSpacing(*value);
}

float BoxStyle::lineHeight() const
{
    auto value = get(CssPropertyID::LineHeight);
    if(value == nullptr || value->id() == CssValueID::Normal)
        return fontLineSpacing();
    if(auto percent = to<CssPercentValue>(value))
        return percent->value() * fontSize() / 100.f;
    const auto& length = to<CssLengthValue>(*value);
    if(length.units() == CssLengthUnits::None)
        return length.value() * fontSize();
    return convertLengthValue(length);
}

float BoxStyle::tabWidth(float spaceWidth) const
{
    auto value = get(CssPropertyID::TabSize);
    if(value == nullptr)
        return 8 * spaceWidth;
    const auto& length = to<CssLengthValue>(*value);
    if(length.units() == CssLengthUnits::None)
        return spaceWidth * length.value();
    return convertLengthValue(length);
}

Overflow BoxStyle::overflow() const
{
    auto value = get(CssPropertyID::Overflow);
    if(value == nullptr) {
        if(m_node->isSvgElement())
            return Overflow::Hidden;
        return Overflow::Visible;
    }

    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Auto:
        return Overflow::Auto;
    case CssValueID::Visible:
        return Overflow::Visible;
    case CssValueID::Hidden:
        return Overflow::Hidden;
    case CssValueID::Scroll:
        return Overflow::Scroll;
    default:
        assert(false);
    }

    return Overflow::Visible;
}

std::optional<int> BoxStyle::zIndex() const
{
    auto value = get(CssPropertyID::ZIndex);
    if(value == nullptr)
        return std::nullopt;
    return convertIntegerOrAuto(*value);
}

VerticalAlign BoxStyle::verticalAlign() const
{
    if(m_verticalAlignType != VerticalAlignType::Length)
        return VerticalAlign(m_verticalAlignType);
    auto value = get(CssPropertyID::VerticalAlign);
    return VerticalAlign(m_verticalAlignType, convertLengthOrPercent(*value));
}

LengthBox BoxStyle::clip() const
{
    auto value = get(CssPropertyID::Clip);
    if(value == nullptr || value->id() == CssValueID::Auto)
        return LengthBox(Length::Auto);
    const auto& rect = to<CssRectValue>(*value);
    auto left = convertLengthOrPercentOrAuto(*rect.left());
    auto right = convertLengthOrPercentOrAuto(*rect.right());
    auto top = convertLengthOrPercentOrAuto(*rect.top());
    auto bottom = convertLengthOrPercentOrAuto(*rect.bottom());
    return LengthBox(left, right, top, bottom);
}

Length BoxStyle::flexBasis() const
{
    auto value = get(CssPropertyID::FlexBasis);
    if(value == nullptr)
        return Length::Auto;
    return convertWidthOrHeightLength(*value);
}

float BoxStyle::flexGrow() const
{
    auto value = get(CssPropertyID::FlexGrow);
    if(value == nullptr)
        return 0.0;
    return convertNumber(*value);
}

float BoxStyle::flexShrink() const
{
    auto value = get(CssPropertyID::FlexShrink);
    if(value == nullptr)
        return 1.0;
    return convertNumber(*value);
}

int BoxStyle::order() const
{
    auto value = get(CssPropertyID::Order);
    if(value == nullptr)
        return 0;
    return convertInteger(*value);
}

FlexDirection BoxStyle::flexDirection() const
{
    auto value = get(CssPropertyID::FlexDirection);
    if(value == nullptr)
        return FlexDirection::Row;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Row:
        return FlexDirection::Row;
    case CssValueID::RowReverse:
        return FlexDirection::RowReverse;
    case CssValueID::Column:
        return FlexDirection::Column;
    case CssValueID::ColumnReverse:
        return FlexDirection::ColumnReverse;
    default:
        assert(false);
    }

    return FlexDirection::Row;
}

FlexWrap BoxStyle::flexWrap() const
{
    auto value = get(CssPropertyID::FlexWrap);
    if(value == nullptr)
        return FlexWrap::Nowrap;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Nowrap:
        return FlexWrap::Nowrap;
    case CssValueID::Wrap:
        return FlexWrap::Wrap;
    case CssValueID::WrapReverse:
        return FlexWrap::WrapReverse;
    default:
        assert(false);
    }

    return FlexWrap::Nowrap;
}

AlignContent BoxStyle::justifyContent() const
{
    auto value = get(CssPropertyID::JustifyContent);
    if(value == nullptr)
        return AlignContent::FlexStart;
    return convertAlignContent(*value);
}

AlignContent BoxStyle::alignContent() const
{
    auto value = get(CssPropertyID::AlignContent);
    if(value == nullptr)
        return AlignContent::Stretch;
    return convertAlignContent(*value);
}

AlignItem BoxStyle::alignItems() const
{
    auto value = get(CssPropertyID::AlignItems);
    if(value == nullptr)
        return AlignItem::Stretch;
    return convertAlignItem(*value);
}

AlignItem BoxStyle::alignSelf() const
{
    auto value = get(CssPropertyID::AlignSelf);
    if(value == nullptr)
        return AlignItem::Auto;
    return convertAlignItem(*value);
}

float BoxStyle::outlineOffset() const
{
    auto value = get(CssPropertyID::OutlineOffset);
    if(value == nullptr)
        return 0.0;
    return convertLengthValue(*value);
}

Color BoxStyle::outlineColor() const
{
    auto value = get(CssPropertyID::OutlineColor);
    if(value == nullptr)
        return m_color;
    return convertColor(*value);
}

float BoxStyle::outlineWidth() const
{
    auto value = get(CssPropertyID::OutlineWidth);
    if(value == nullptr)
        return 3.0;
    return convertLineWidth(*value);
}

LineStyle BoxStyle::outlineStyle() const
{
    auto value = get(CssPropertyID::OutlineStyle);
    if(value == nullptr)
        return LineStyle::None;
    return convertLineStyle(*value);
}

BorderEdge BoxStyle::getOutlineEdge() const
{
    return BorderEdge(outlineWidth(), outlineColor(), outlineStyle());
}

int BoxStyle::widows() const
{
    auto value = get(CssPropertyID::Widows);
    if(value == nullptr)
        return 2;
    return convertInteger(*value);
}

int BoxStyle::orphans() const
{
    auto value = get(CssPropertyID::Orphans);
    if(value == nullptr)
        return 2;
    return convertInteger(*value);
}

Color BoxStyle::columnRuleColor() const
{
    auto value = get(CssPropertyID::ColumnRuleColor);
    if(value == nullptr)
        return m_color;
    return convertColor(*value);
}

LineStyle BoxStyle::columnRuleStyle() const
{
    auto value = get(CssPropertyID::ColumnRuleStyle);
    if(value == nullptr)
        return LineStyle::None;
    return convertLineStyle(*value);
}

float BoxStyle::columnRuleWidth() const
{
    auto value = get(CssPropertyID::ColumnRuleWidth);
    if(value == nullptr)
        return 3.0;
    return convertLineWidth(*value);
}

ColumnSpan BoxStyle::columnSpan() const
{
    auto value = get(CssPropertyID::ColumnSpan);
    if(value == nullptr)
        return ColumnSpan::None;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::None:
        return ColumnSpan::None;
    case CssValueID::All:
        return ColumnSpan::All;
    default:
        assert(false);
    }

    return ColumnSpan::None;
}

ColumnFill BoxStyle::columnFill() const
{
    auto value = get(CssPropertyID::ColumnFill);
    if(value == nullptr)
        return ColumnFill::Balance;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Auto:
        return ColumnFill::Auto;
    case CssValueID::Balance:
        return ColumnFill::Balance;
    default:
        assert(false);
    }

    return ColumnFill::Balance;
}

std::optional<float> BoxStyle::rowGap() const
{
    auto value = get(CssPropertyID::RowGap);
    if(value == nullptr)
        return std::nullopt;
    return convertLengthOrNormal(*value);
}

std::optional<float> BoxStyle::columnGap() const
{
    auto value = get(CssPropertyID::ColumnGap);
    if(value == nullptr)
        return std::nullopt;
    return convertLengthOrNormal(*value);
}

std::optional<float> BoxStyle::columnWidth() const
{
    auto value = get(CssPropertyID::ColumnWidth);
    if(value == nullptr)
        return std::nullopt;
    return convertLengthOrAuto(*value);
}

std::optional<int> BoxStyle::columnCount() const
{
    auto value = get(CssPropertyID::ColumnCount);
    if(value == nullptr)
        return std::nullopt;
    return convertIntegerOrAuto(*value);
}

std::optional<float> BoxStyle::pageScale() const
{
    auto value = get(CssPropertyID::PageScale);
    if(value == nullptr || value->id() == CssValueID::Auto)
        return std::nullopt;
    return convertNumberOrPercent(*value);
}

GlobalString BoxStyle::page() const
{
    auto value = get(CssPropertyID::Page);
    if(value == nullptr || value->id() == CssValueID::Auto)
        return emptyGlo;
    return convertCustomIdent(*value);
}

PageSize BoxStyle::getPageSize(const PageSize& deviceSize) const
{
    auto value = get(CssPropertyID::Size);
    if(value == nullptr)
        return deviceSize;
    if(auto ident = to<CssIdentValue>(value)) {
        switch(ident->value()) {
        case CssValueID::Auto:
            return deviceSize;
        case CssValueID::Portrait:
            return deviceSize.portrait();
        case CssValueID::Landscape:
            return deviceSize.landscape();
        default:
            return convertPageSize(*value);
        }
    }

    const auto& pair = to<CssPairValue>(*value);
    if(auto size = to<CssIdentValue>(pair.first())) {
        const auto& orientation = to<CssIdentValue>(*pair.second());
        auto pageSize = convertPageSize(*size);
        switch(orientation.value()) {
        case CssValueID::Portrait:
            return pageSize.portrait();
        case CssValueID::Landscape:
            return pageSize.landscape();
        default:
            assert(false);
        }
    }

    auto width = convertLengthValue(*pair.first());
    auto height = convertLengthValue(*pair.second());
    return PageSize(width * units::px, height * units::px);
}

Paint BoxStyle::fill() const
{
    auto value = get(CssPropertyID::Fill);
    if(value == nullptr)
        return Paint(Color::Black);
    return convertPaint(*value);
}

Paint BoxStyle::stroke() const
{
    auto value = get(CssPropertyID::Stroke);
    if(value == nullptr)
        return Paint();
    return convertPaint(*value);
}

Color BoxStyle::stopColor() const
{
    auto value = get(CssPropertyID::StopColor);
    if(value == nullptr)
        return Color::Black;
    return convertColor(*value);
}

float BoxStyle::opacity() const
{
    auto value = get(CssPropertyID::Opacity);
    if(value == nullptr)
        return 1.f;
    return convertNumberOrPercent(*value);
}

float BoxStyle::stopOpacity() const
{
    auto value = get(CssPropertyID::StopOpacity);
    if(value == nullptr)
        return 1.f;
    return convertNumberOrPercent(*value);
}

float BoxStyle::fillOpacity() const
{
    auto value = get(CssPropertyID::FillOpacity);
    if(value == nullptr)
        return 1.f;
    return convertNumberOrPercent(*value);
}

float BoxStyle::strokeOpacity() const
{
    auto value = get(CssPropertyID::StrokeOpacity);
    if(value == nullptr)
        return 1.f;
    return convertNumberOrPercent(*value);
}

float BoxStyle::strokeMiterlimit() const
{
    auto value = get(CssPropertyID::StrokeMiterlimit);
    if(value == nullptr)
        return 4.f;
    return convertNumber(*value);
}

Length BoxStyle::strokeWidth() const
{
    auto value = get(CssPropertyID::StrokeWidth);
    if(value == nullptr)
        return Length(1.f);
    return convertLengthOrPercent(*value);
}

Length BoxStyle::strokeDashoffset() const
{
    auto value = get(CssPropertyID::StrokeDashoffset);
    if(value == nullptr)
        return Length(0.f);
    return convertLengthOrPercent(*value);
}

LengthList BoxStyle::strokeDasharray() const
{
    auto value = get(CssPropertyID::StrokeDasharray);
    if(value == nullptr || value->id() == CssValueID::None)
        return LengthList();

    LengthList dashes;
    for(const auto& dash : to<CssListValue>(*value)) {
        dashes.push_back(convertLengthOrPercent(*dash));
    }

    return dashes;
}

LineCap BoxStyle::strokeLinecap() const
{
    auto value = get(CssPropertyID::StrokeLinecap);
    if(value == nullptr)
        return LineCap::Butt;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Butt:
        return LineCap::Butt;
    case CssValueID::Round:
        return LineCap::Round;
    case CssValueID::Square:
        return LineCap::Square;
    default:
        assert(false);
    }

    return LineCap::Butt;
}

LineJoin BoxStyle::strokeLinejoin() const
{
    auto value = get(CssPropertyID::StrokeLinejoin);
    if(value == nullptr)
        return LineJoin::Miter;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Miter:
        return LineJoin::Miter;
    case CssValueID::Round:
        return LineJoin::Round;
    case CssValueID::Bevel:
        return LineJoin::Bevel;
    default:
        assert(false);
    }

    return LineJoin::Miter;
}

HeapString BoxStyle::mask() const
{
    auto value = get(CssPropertyID::Mask);
    if(value == nullptr)
        return emptyGlo;
    return convertLocalUrlOrNone(*value);
}

HeapString BoxStyle::clipPath() const
{
    auto value = get(CssPropertyID::ClipPath);
    if(value == nullptr)
        return emptyGlo;
    return convertLocalUrlOrNone(*value);
}

HeapString BoxStyle::markerStart() const
{
    auto value = get(CssPropertyID::MarkerStart);
    if(value == nullptr)
        return emptyGlo;
    return convertLocalUrlOrNone(*value);
}

HeapString BoxStyle::markerMid() const
{
    auto value = get(CssPropertyID::MarkerMid);
    if(value == nullptr)
        return emptyGlo;
    return convertLocalUrlOrNone(*value);
}

HeapString BoxStyle::markerEnd() const
{
    auto value = get(CssPropertyID::MarkerEnd);
    if(value == nullptr)
        return emptyGlo;
    return convertLocalUrlOrNone(*value);
}

AlignmentBaseline BoxStyle::alignmentBaseline() const
{
    auto value = get(CssPropertyID::AlignmentBaseline);
    if(value == nullptr)
        return AlignmentBaseline::Baseline;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Auto:
        return AlignmentBaseline::Auto;
    case CssValueID::Baseline:
        return AlignmentBaseline::Baseline;
    case CssValueID::BeforeEdge:
        return AlignmentBaseline::BeforeEdge;
    case CssValueID::TextBeforeEdge:
        return AlignmentBaseline::TextBeforeEdge;
    case CssValueID::Middle:
        return AlignmentBaseline::Middle;
    case CssValueID::Central:
        return AlignmentBaseline::Central;
    case CssValueID::AfterEdge:
        return AlignmentBaseline::AfterEdge;
    case CssValueID::TextAfterEdge:
        return AlignmentBaseline::TextAfterEdge;
    case CssValueID::Ideographic:
        return AlignmentBaseline::Ideographic;
    case CssValueID::Alphabetic:
        return AlignmentBaseline::Alphabetic;
    case CssValueID::Hanging:
        return AlignmentBaseline::Hanging;
    case CssValueID::Mathematical:
        return AlignmentBaseline::Mathematical;
    default:
        assert(false);
    }

    return AlignmentBaseline::Auto;
}

DominantBaseline BoxStyle::dominantBaseline() const
{
    auto value = get(CssPropertyID::DominantBaseline);
    if(value == nullptr)
        return DominantBaseline::Auto;
    const auto& ident = to<CssIdentValue>(*value);
    switch(ident.value()) {
    case CssValueID::Auto:
        return DominantBaseline::Auto;
    case CssValueID::UseScript:
        return DominantBaseline::UseScript;
    case CssValueID::NoChange:
        return DominantBaseline::NoChange;
    case CssValueID::ResetSize:
        return DominantBaseline::ResetSize;
    case CssValueID::Ideographic:
        return DominantBaseline::Ideographic;
    case CssValueID::Alphabetic:
        return DominantBaseline::Alphabetic;
    case CssValueID::Hanging:
        return DominantBaseline::Hanging;
    case CssValueID::Mathematical:
        return DominantBaseline::Mathematical;
    case CssValueID::Central:
        return DominantBaseline::Central;
    case CssValueID::Middle:
        return DominantBaseline::Middle;
    case CssValueID::TextAfterEdge:
        return DominantBaseline::TextAfterEdge;
    case CssValueID::TextBeforeEdge:
        return DominantBaseline::TextBeforeEdge;
    default:
        assert(false);
    }

    return DominantBaseline::Auto;
}

BaselineShift BoxStyle::baselineShift() const
{
    auto value = get(CssPropertyID::BaselineShift);
    if(value == nullptr)
        return BaselineShiftType::Baseline;
    if(auto ident = to<CssIdentValue>(value)) {
        switch(ident->value()) {
        case CssValueID::Baseline:
            return BaselineShiftType::Baseline;
        case CssValueID::Sub:
            return BaselineShiftType::Sub;
        case CssValueID::Super:
            return BaselineShiftType::Super;
        default:
            assert(false);
        }
    }

    return BaselineShift(BaselineShiftType::Length, convertLengthOrPercent(*value));
}

bool BoxStyle::isOriginalDisplayBlockType() const
{
    auto value = get(CssPropertyID::Display);
    if(value == nullptr)
        return false;
    return isDisplayBlockType(convertDisplay(*value));
}

bool BoxStyle::isOriginalDisplayInlineType() const
{
    auto value = get(CssPropertyID::Display);
    if(value == nullptr)
        return true;
    return isDisplayInlineType(convertDisplay(*value));
}

Point BoxStyle::getTransformOrigin(float width, float height) const
{
    auto value = get(CssPropertyID::TransformOrigin);
    if(value == nullptr)
        return Point(width * 50.f / 100.f, height * 50.f / 100.f);
    const auto& coordinate = convertPositionCoordinate(*value);
    return Point(coordinate.x().calc(width), coordinate.y().calc(height));
}

Transform BoxStyle::getTransform(float width, float height) const
{
    auto value = get(CssPropertyID::Transform);
    if(value == nullptr || value->id() == CssValueID::None)
        return Transform();
    auto origin = getTransformOrigin(width, height);
    auto transform = Transform::makeTranslate(origin.x, origin.y);
    for(const auto& operation : to<CssListValue>(*value)) {
        const auto& function = to<CssFunctionValue>(*operation);
        switch(function.id()) {
        case CssFunctionID::Translate: {
            float firstValue = convertLengthOrPercent(width, *function.at(0));
            float secondValue = 0.f;
            if(function.size() == 2)
                secondValue = convertLengthOrPercent(height, *function.at(1));
            transform.translate(firstValue, secondValue);
            break;
        }

        case CssFunctionID::TranslateX:
            transform.translate(convertLengthOrPercent(width, *function.at(0)), 0.f);
            break;
        case CssFunctionID::TranslateY:
            transform.translate(0.f, convertLengthOrPercent(height, *function.at(0)));
            break;
        case CssFunctionID::Scale: {
            float firstValue = convertNumberOrPercent(*function.at(0));
            float secondValue = firstValue;
            if(function.size() == 2)
                secondValue = convertNumberOrPercent(*function.at(1));
            transform.scale(firstValue, secondValue);
            break;
        }

        case CssFunctionID::ScaleX:
            transform.scale(convertNumberOrPercent(*function.at(0)), 1.f);
            break;
        case CssFunctionID::ScaleY:
            transform.scale(1.f, convertNumberOrPercent(*function.at(0)));
            break;
        case CssFunctionID::Skew: {
            float firstValue = convertAngle(*function.at(0));
            float secondValue = 0.f;
            if(function.size() == 2)
                secondValue = convertAngle(*function.at(1));
            transform.shear(firstValue, secondValue);
            break;
        }

        case CssFunctionID::SkewX:
            transform.shear(convertAngle(*function.at(0)), 0.f);
            break;
        case CssFunctionID::SkewY:
            transform.shear(0.f, convertAngle(*function.at(0)));
            break;
        case CssFunctionID::Rotate:
            transform.rotate(convertAngle(*function.at(0)));
            break;
        default:
            assert(function.id() == CssFunctionID::Matrix && function.size() == 6);
            auto a = convertNumber(*function.at(0));
            auto b = convertNumber(*function.at(1));
            auto c = convertNumber(*function.at(2));
            auto d = convertNumber(*function.at(3));
            auto e = convertNumber(*function.at(4));
            auto f = convertNumber(*function.at(5));
            transform.multiply(Transform(a, b, c, d, e, f));
            break;
        }
    }

    transform.translate(-origin.x, -origin.y);
    return transform;
}

bool BoxStyle::hasTransform() const
{
    auto value = get(CssPropertyID::Transform);
    return value && value->id() != CssValueID::None;
}

bool BoxStyle::hasContent() const
{
    auto value = get(CssPropertyID::Content);
    return value && value->id() != CssValueID::None;
}

bool BoxStyle::hasLineHeight() const
{
    auto value = get(CssPropertyID::LineHeight);
    return value && value->id() != CssValueID::Normal;
}

bool BoxStyle::hasStroke() const
{
    auto value = get(CssPropertyID::Stroke);
    return value && value->id() != CssValueID::None;
}

bool BoxStyle::hasBackground() const
{
    return backgroundColor().isVisible() || backgroundImage();
}

bool BoxStyle::hasColumns() const
{
    return columnCount() || columnWidth();
}

const HeapString& BoxStyle::getQuote(bool open, size_t depth) const
{
    static const GlobalString defaultQuote("\"");
    auto value = get(CssPropertyID::Quotes);
    if(value == nullptr)
        return defaultQuote;
    if(auto ident = to<CssIdentValue>(value)) {
        switch(ident->value()) {
        case CssValueID::Auto:
            return defaultQuote;
        case CssValueID::None:
            return emptyGlo;
        default:
            assert(false);
        }
    }

    const auto& list = to<CssListValue>(*value);
    const auto& pair = to<CssPairValue>(*list.at(std::min(depth, list.size() - 1)));
    const auto& quote = open ? pair.first() : pair.second();
    return to<CssStringValue>(*quote).value();
}

CssVariableData* BoxStyle::getCustom(const std::string_view& name) const
{
    auto it = m_customProperties.find(name);
    if(it == m_customProperties.end())
        return nullptr;
    return it->second.get();
}

void BoxStyle::setCustom(const GlobalString& name, RefPtr<CssVariableData> value)
{
    m_customProperties.insert_or_assign(name, std::move(value));
}

void BoxStyle::set(CssPropertyID id, RefPtr<CssValue> value)
{
    switch(id) {
    case CssPropertyID::Display:
        m_display = convertDisplay(*value);
        break;
    case CssPropertyID::Position:
        m_position = convertPosition(*value);
        break;
    case CssPropertyID::Float:
        m_floating = convertFloat(*value);
        break;
    case CssPropertyID::Clear:
        m_clear = convertClear(*value);
        break;
    case CssPropertyID::VerticalAlign:
        m_verticalAlignType = convertVerticalAlignType(*value);
        break;
    case CssPropertyID::Direction:
        m_direction = convertDirection(*value);
        break;
    case CssPropertyID::UnicodeBidi:
        m_unicodeBidi = convertUnicodeBidi(*value);
        break;
    case CssPropertyID::Visibility:
        m_visibility = convertVisibility(*value);
        break;
    case CssPropertyID::BoxSizing:
        m_boxSizing = convertBoxSizing(*value);
        break;
    case CssPropertyID::MixBlendMode:
        m_blendMode = convertBlendMode(*value);
        break;
    case CssPropertyID::MaskType:
        m_maskType = convertMaskType(*value);
        break;
    case CssPropertyID::WritingMode:
        m_writingMode = convertWritingMode(*value);
        break;
    case CssPropertyID::TextOrientation:
        m_textOrientation = convertTextOrientation(*value);
        break;
    case CssPropertyID::TextAlign:
        m_textAlign = convertTextAlign(*value);
        break;
    case CssPropertyID::WhiteSpace:
        m_whiteSpace = convertWhiteSpace(*value);
        break;
    case CssPropertyID::WordBreak:
        m_wordBreak = convertWordBreak(*value);
        break;
    case CssPropertyID::OverflowWrap:
        m_overflowWrap = convertOverflowWrap(*value);
        break;
    case CssPropertyID::FillRule:
        m_fillRule = convertFillRule(*value);
        break;
    case CssPropertyID::ClipRule:
        m_clipRule = convertFillRule(*value);
        break;
    case CssPropertyID::CaptionSide:
        m_captionSide = convertCaptionSide(*value);
        break;
    case CssPropertyID::EmptyCells:
        m_emptyCells = convertEmptyCells(*value);
        break;
    case CssPropertyID::BorderCollapse:
        m_borderCollapse = convertBorderCollapse(*value);
        break;
    case CssPropertyID::BreakAfter:
    case CssPropertyID::ColumnBreakAfter:
    case CssPropertyID::PageBreakAfter:
        m_breakAfter = convertBreakBetween(*value);
        break;
    case CssPropertyID::BreakBefore:
    case CssPropertyID::ColumnBreakBefore:
    case CssPropertyID::PageBreakBefore:
        m_breakBefore = convertBreakBetween(*value);
        break;
    case CssPropertyID::BreakInside:
    case CssPropertyID::ColumnBreakInside:
    case CssPropertyID::PageBreakInside:
        m_breakInside = convertBreakInside(*value);
        break;
    case CssPropertyID::Color:
        m_color = convertColor(*value);
        break;
    default:
        break;
    }

    m_properties.insert_or_assign(id, std::move(value));
}

void BoxStyle::reset(CssPropertyID id)
{
    switch(id) {
    case CssPropertyID::Display:
        m_display = Display::Inline;
        break;
    case CssPropertyID::Position:
        m_position = Position::Static;
        break;
    case CssPropertyID::Float:
        m_floating = Float::None;
        break;
    case CssPropertyID::Clear:
        m_clear = Clear::None;
        break;
    case CssPropertyID::VerticalAlign:
        m_verticalAlignType = VerticalAlignType::Baseline;
        break;
    case CssPropertyID::Direction:
        m_direction = Direction::Ltr;
        break;
    case CssPropertyID::UnicodeBidi:
        m_unicodeBidi = UnicodeBidi::Normal;
        break;
    case CssPropertyID::Visibility:
        m_visibility = Visibility::Visible;
        break;
    case CssPropertyID::BoxSizing:
        m_boxSizing = BoxSizing::ContentBox;
        break;
    case CssPropertyID::MixBlendMode:
        m_blendMode = BlendMode::Normal;
        break;
    case CssPropertyID::MaskType:
        m_maskType = MaskType::Luminance;
        break;
    case CssPropertyID::WritingMode:
        m_writingMode = WritingMode::HorizontalTb;
        break;
    case CssPropertyID::TextOrientation:
        m_textOrientation = TextOrientation::Mixed;
        break;
    case CssPropertyID::TextAlign:
        m_textAlign = TextAlign::Left;
        break;
    case CssPropertyID::WhiteSpace:
        m_whiteSpace = WhiteSpace::Normal;
        break;
    case CssPropertyID::WordBreak:
        m_wordBreak = WordBreak::Normal;
        break;
    case CssPropertyID::OverflowWrap:
        m_overflowWrap = OverflowWrap::Normal;
        break;
    case CssPropertyID::FillRule:
        m_fillRule = FillRule::NonZero;
        break;
    case CssPropertyID::ClipRule:
        m_clipRule = FillRule::NonZero;
        break;
    case CssPropertyID::CaptionSide:
        m_captionSide = CaptionSide::Top;
        break;
    case CssPropertyID::EmptyCells:
        m_emptyCells = EmptyCells::Show;
        break;
    case CssPropertyID::BorderCollapse:
        m_borderCollapse = BorderCollapse::Separate;
        break;
    case CssPropertyID::BreakAfter:
    case CssPropertyID::ColumnBreakAfter:
    case CssPropertyID::PageBreakAfter:
        m_breakBefore = BreakBetween::Auto;
        break;
    case CssPropertyID::BreakBefore:
    case CssPropertyID::ColumnBreakBefore:
    case CssPropertyID::PageBreakBefore:
        m_breakBefore = BreakBetween::Auto;
        break;
    case CssPropertyID::BreakInside:
    case CssPropertyID::ColumnBreakInside:
    case CssPropertyID::PageBreakInside:
        m_breakInside = BreakInside::Auto;
        break;
    case CssPropertyID::Color:
        m_color = Color::Black;
        break;
    default:
        break;
    }

    m_properties.erase(id);
}

void BoxStyle::inheritFrom(const BoxStyle* parentStyle)
{
    m_font = parentStyle->font();
    m_direction = parentStyle->direction();
    m_visibility = parentStyle->visibility();
    m_writingMode = parentStyle->writingMode();
    m_textOrientation = parentStyle->textOrientation();
    m_textAlign = parentStyle->textAlign();
    m_whiteSpace = parentStyle->whiteSpace();
    m_wordBreak = parentStyle->wordBreak();
    m_overflowWrap = parentStyle->overflowWrap();
    m_fillRule = parentStyle->fillRule();
    m_clipRule = parentStyle->clipRule();
    m_captionSide = parentStyle->captionSide();
    m_emptyCells = parentStyle->emptyCells();
    m_borderCollapse = parentStyle->borderCollapse();
    m_color = parentStyle->color();
    m_customProperties = parentStyle->customProperties();
    for(const auto& [id, value] : parentStyle->properties()) {
        switch(id) {
        case CssPropertyID::BorderCollapse:
        case CssPropertyID::CaptionSide:
        case CssPropertyID::ClipRule:
        case CssPropertyID::Color:
        case CssPropertyID::Direction:
        case CssPropertyID::DominantBaseline:
        case CssPropertyID::EmptyCells:
        case CssPropertyID::Fill:
        case CssPropertyID::FillOpacity:
        case CssPropertyID::FillRule:
        case CssPropertyID::FontFamily:
        case CssPropertyID::FontFeatureSettings:
        case CssPropertyID::FontKerning:
        case CssPropertyID::FontSize:
        case CssPropertyID::FontStretch:
        case CssPropertyID::FontStyle:
        case CssPropertyID::FontVariantCaps:
        case CssPropertyID::FontVariantEmoji:
        case CssPropertyID::FontVariantEastAsian:
        case CssPropertyID::FontVariantLigatures:
        case CssPropertyID::FontVariantNumeric:
        case CssPropertyID::FontVariantPosition:
        case CssPropertyID::FontVariationSettings:
        case CssPropertyID::FontWeight:
        case CssPropertyID::Hyphens:
        case CssPropertyID::LetterSpacing:
        case CssPropertyID::LineHeight:
        case CssPropertyID::ListStyleImage:
        case CssPropertyID::ListStylePosition:
        case CssPropertyID::ListStyleType:
        case CssPropertyID::MarkerEnd:
        case CssPropertyID::MarkerMid:
        case CssPropertyID::MarkerStart:
        case CssPropertyID::Orphans:
        case CssPropertyID::OverflowWrap:
        case CssPropertyID::PaintOrder:
        case CssPropertyID::Quotes:
        case CssPropertyID::Stroke:
        case CssPropertyID::StrokeDasharray:
        case CssPropertyID::StrokeDashoffset:
        case CssPropertyID::StrokeLinecap:
        case CssPropertyID::StrokeLinejoin:
        case CssPropertyID::StrokeMiterlimit:
        case CssPropertyID::StrokeOpacity:
        case CssPropertyID::StrokeWidth:
        case CssPropertyID::TabSize:
        case CssPropertyID::TextAlign:
        case CssPropertyID::TextAnchor:
        case CssPropertyID::TextDecorationColor:
        case CssPropertyID::TextDecorationLine:
        case CssPropertyID::TextDecorationStyle:
        case CssPropertyID::TextIndent:
        case CssPropertyID::TextOrientation:
        case CssPropertyID::TextTransform:
        case CssPropertyID::Visibility:
        case CssPropertyID::WhiteSpace:
        case CssPropertyID::Widows:
        case CssPropertyID::WordBreak:
        case CssPropertyID::WordSpacing:
        case CssPropertyID::WritingMode:
            m_properties.insert_or_assign(id, value);
            break;
        default:
            break;
        }
    }
}

float BoxStyle::exFontSize() const
{
    if(auto fontData = m_font->primaryFont())
        return fontData->xHeight();
    return fontSize() / 2.f;
}

float BoxStyle::chFontSize() const
{
    if(auto fontData = m_font->primaryFont())
        return fontData->zeroWidth();
    return fontSize() / 2.f;
}

float BoxStyle::remFontSize() const
{
    if(auto style = document()->rootStyle())
        return style->fontSize();
    return kMediumFontSize;
}

class FontFeaturesBuilder {
public:
    explicit FontFeaturesBuilder(const CssPropertyMap& properties);

    FontFeatureList build() const;

private:
    void buildKerning(FontFeatureList& features) const;
    void buildVariantLigatures(FontFeatureList& features) const;
    void buildVariantPosition(FontFeatureList& features) const;
    void buildVariantCaps(FontFeatureList& features) const;
    void buildVariantNumeric(FontFeatureList& features) const;
    void buildVariantEastAsian(FontFeatureList& features) const;
    void buildFeatureSettings(FontFeatureList& features) const;

    RefPtr<CssValue> m_kerning;
    RefPtr<CssValue> m_variantLigatures;
    RefPtr<CssValue> m_variantPosition;
    RefPtr<CssValue> m_variantCaps;
    RefPtr<CssValue> m_variantNumeric;
    RefPtr<CssValue> m_variantEastAsian;
    RefPtr<CssValue> m_featureSettings;
};

FontFeaturesBuilder::FontFeaturesBuilder(const CssPropertyMap& properties)
{
    for(const auto& [id, value] : properties) {
        switch(id) {
        case CssPropertyID::FontKerning:
            m_kerning = value;
            break;
        case CssPropertyID::FontVariantLigatures:
            m_variantLigatures = value;
            break;
        case CssPropertyID::FontVariantPosition:
            m_variantPosition = value;
            break;
        case CssPropertyID::FontVariantCaps:
            m_variantCaps = value;
            break;
        case CssPropertyID::FontVariantNumeric:
            m_variantNumeric = value;
            break;
        case CssPropertyID::FontVariantEastAsian:
            m_variantEastAsian = value;
            break;
        case CssPropertyID::FontFeatureSettings:
            m_featureSettings = value;
            break;
        default:
            break;
        }
    }
}

FontFeatureList FontFeaturesBuilder::build() const
{
    FontFeatureList features;
    buildKerning(features);
    buildVariantLigatures(features);
    buildVariantPosition(features);
    buildVariantCaps(features);
    buildVariantNumeric(features);
    buildVariantEastAsian(features);
    buildFeatureSettings(features);
    return features;
}

void FontFeaturesBuilder::buildKerning(FontFeatureList& features) const
{
    if(m_kerning == nullptr)
        return;
    constexpr FontTag kernTag("kern");
    const auto& ident = to<CssIdentValue>(*m_kerning);
    switch(ident.id()) {
    case CssValueID::Auto:
        break;
    case CssValueID::Normal:
        features.emplace_front(kernTag, 1);
        break;
    case CssValueID::None:
        features.emplace_front(kernTag, 0);
        break;
    default:
        assert(false);
    }
}

void FontFeaturesBuilder::buildVariantLigatures(FontFeatureList& features) const
{
    if(m_variantLigatures == nullptr)
        return;
    constexpr FontTag ligaTag("liga");
    constexpr FontTag cligTag("clig");
    constexpr FontTag hligTag("hlig");
    constexpr FontTag dligTag("dlig");
    constexpr FontTag caltTag("calt");
    if(auto ident = to<CssIdentValue>(m_variantLigatures)) {
        if(ident->value() == CssValueID::Normal)
            return;
        assert(ident->value() == CssValueID::None);
        features.emplace_front(ligaTag, 0);
        features.emplace_front(cligTag, 0);
        features.emplace_front(hligTag, 0);
        features.emplace_front(dligTag, 0);
        features.emplace_front(caltTag, 0);
        return;
    }

    for(const auto& value : to<CssListValue>(*m_variantLigatures)) {
        const auto& ident = to<CssIdentValue>(*value);
        switch(ident.id()) {
        case CssValueID::CommonLigatures:
            features.emplace_front(ligaTag, 1);
            features.emplace_front(cligTag, 1);
            break;
        case CssValueID::NoCommonLigatures:
            features.emplace_front(ligaTag, 0);
            features.emplace_front(cligTag, 0);
            break;
        case CssValueID::HistoricalLigatures:
            features.emplace_front(hligTag, 1);
            break;
        case CssValueID::NoHistoricalLigatures:
            features.emplace_front(hligTag, 0);
            break;
        case CssValueID::DiscretionaryLigatures:
            features.emplace_front(dligTag, 1);
            break;
        case CssValueID::NoDiscretionaryLigatures:
            features.emplace_front(dligTag, 0);
            break;
        case CssValueID::Contextual:
            features.emplace_front(caltTag, 1);
            break;
        case CssValueID::NoContextual:
            features.emplace_front(caltTag, 0);
            break;
        default:
            assert(false);
        }
    }
}

void FontFeaturesBuilder::buildVariantPosition(FontFeatureList& features) const
{
    if(m_variantPosition == nullptr)
        return;
    constexpr FontTag subsTag("subs");
    constexpr FontTag supsTag("sups");
    const auto& ident = to<CssIdentValue>(*m_variantPosition);
    switch(ident.id()) {
    case CssValueID::Normal:
        break;
    case CssValueID::Sub:
        features.emplace_front(subsTag, 1);
        break;
    case CssValueID::Super:
        features.emplace_front(supsTag, 1);
        break;
    default:
        assert(false);
    }
}

void FontFeaturesBuilder::buildVariantCaps(FontFeatureList& features) const
{
    if(m_variantCaps == nullptr)
        return;
    constexpr FontTag smcpTag("smcp");
    constexpr FontTag c2scTag("c2sc");
    constexpr FontTag pcapTag("pcap");
    constexpr FontTag c2pcTag("c2pc");
    constexpr FontTag unicTag("unic");
    constexpr FontTag titlTag("titl");
    const auto& ident = to<CssIdentValue>(*m_variantCaps);
    switch(ident.id()) {
    case CssValueID::Normal:
        break;
    case CssValueID::SmallCaps:
        features.emplace_front(smcpTag, 1);
        break;
    case CssValueID::AllSmallCaps:
        features.emplace_front(c2scTag, 1);
        features.emplace_front(smcpTag, 1);
        break;
    case CssValueID::PetiteCaps:
        features.emplace_front(pcapTag, 1);
        break;
    case CssValueID::AllPetiteCaps:
        features.emplace_front(c2pcTag, 1);
        features.emplace_front(pcapTag, 1);
        break;
    case CssValueID::Unicase:
        features.emplace_front(unicTag, 1);
        break;
    case CssValueID::TitlingCaps:
        features.emplace_front(titlTag, 1);
        break;
    default:
        assert(false);
    }
}

void FontFeaturesBuilder::buildVariantNumeric(FontFeatureList& features) const
{
    if(m_variantNumeric == nullptr)
        return;
    if(auto ident = to<CssIdentValue>(m_variantNumeric)) {
        assert(ident->value() == CssValueID::Normal);
        return;
    }

    constexpr FontTag lnumTag("lnum");
    constexpr FontTag onumTag("onum");
    constexpr FontTag pnumTag("pnum");
    constexpr FontTag tnumTag("tnum");
    constexpr FontTag fracTag("frac");
    constexpr FontTag afrcTag("afrc");
    constexpr FontTag ordnTag("ordn");
    constexpr FontTag zeroTag("zero");
    for(const auto& value : to<CssListValue>(*m_variantNumeric)) {
        const auto& ident = to<CssIdentValue>(*value);
        switch(ident.id()) {
        case CssValueID::LiningNums:
            features.emplace_front(lnumTag, 1);
            break;
        case CssValueID::OldstyleNums:
            features.emplace_front(onumTag, 1);
            break;
        case CssValueID::ProportionalNums:
            features.emplace_front(pnumTag, 1);
            break;
        case CssValueID::TabularNums:
            features.emplace_front(tnumTag, 1);
            break;
        case CssValueID::DiagonalFractions:
            features.emplace_front(fracTag, 1);
            break;
        case CssValueID::StackedFractions:
            features.emplace_front(afrcTag, 1);
            break;
        case CssValueID::Ordinal:
            features.emplace_front(ordnTag, 1);
            break;
        case CssValueID::SlashedZero:
            features.emplace_front(zeroTag, 1);
            break;
        default:
            assert(false);
        }
    }
}

void FontFeaturesBuilder::buildVariantEastAsian(FontFeatureList& features) const
{
    if(m_variantEastAsian == nullptr)
        return;
    if(auto ident = to<CssIdentValue>(m_variantEastAsian)) {
        assert(ident->value() == CssValueID::Normal);
        return;
    }

    constexpr FontTag jp78Tag("jp78");
    constexpr FontTag jp83Tag("jp83");
    constexpr FontTag jp90Tag("jp90");
    constexpr FontTag jp04Tag("jp04");
    constexpr FontTag smplTag("smpl");
    constexpr FontTag tradTag("trad");
    constexpr FontTag fwidTag("fwid");
    constexpr FontTag pwidTag("pwid");
    constexpr FontTag rubyTag("ruby");
    for(const auto& value : to<CssListValue>(*m_variantEastAsian)) {
        const auto& ident = to<CssIdentValue>(*value);
        switch(ident.id()) {
        case CssValueID::Jis78:
            features.emplace_front(jp78Tag, 1);
            break;
        case CssValueID::Jis83:
            features.emplace_front(jp83Tag, 1);
            break;
        case CssValueID::Jis90:
            features.emplace_front(jp90Tag, 1);
            break;
        case CssValueID::Jis04:
            features.emplace_front(jp04Tag, 1);
            break;
        case CssValueID::Simplified:
            features.emplace_front(smplTag, 1);
            break;
        case CssValueID::Traditional:
            features.emplace_front(tradTag, 1);
            break;
        case CssValueID::FullWidth:
            features.emplace_front(fwidTag, 1);
            break;
        case CssValueID::ProportionalWidth:
            features.emplace_front(pwidTag, 1);
            break;
        case CssValueID::Ruby:
            features.emplace_front(rubyTag, 1);
            break;
        default:
            assert(false);
        }
    }
}

void FontFeaturesBuilder::buildFeatureSettings(FontFeatureList& features) const
{
    if(m_featureSettings == nullptr)
        return;
    if(auto ident = to<CssIdentValue>(m_featureSettings)) {
        assert(ident->value() == CssValueID::Normal);
        return;
    }

    for(const auto& value : to<CssListValue>(*m_featureSettings)) {
        const auto& feature = to<CssFontFeatureValue>(*value);
        features.emplace_front(feature.tag(), feature.value());
    }
}

FontFeatureList BoxStyle::fontFeatures() const
{
    return FontFeaturesBuilder(properties()).build();
}

float BoxStyle::viewportWidth() const
{
    return document()->viewportWidth();
}

float BoxStyle::viewportHeight() const
{
    return document()->viewportHeight();
}

float BoxStyle::viewportMin() const
{
    return std::min(document()->viewportWidth(), document()->viewportHeight());
}

float BoxStyle::viewportMax() const
{
    return std::max(document()->viewportWidth(), document()->viewportHeight());
}

RefPtr<CssValue> BoxStyle::resolveLength(const RefPtr<CssValue>& value) const
{
    if(is<CssLengthValue>(value)) {
        const auto& length = to<CssLengthValue>(*value);
        switch(length.units()) {
        case CssLengthUnits::None:
        case CssLengthUnits::Pixels:
        case CssLengthUnits::Points:
        case CssLengthUnits::Picas:
        case CssLengthUnits::Centimeters:
        case CssLengthUnits::Millimeters:
        case CssLengthUnits::Inches:
            return value;
        case CssLengthUnits::ViewportWidth:
        case CssLengthUnits::ViewportHeight:
        case CssLengthUnits::ViewportMin:
        case CssLengthUnits::ViewportMax:
        case CssLengthUnits::Ems:
        case CssLengthUnits::Exs:
        case CssLengthUnits::Chs:
        case CssLengthUnits::Rems:
            break;
        }
    }

    return CssLengthValue::create(heap(), convertLengthValue(*value));
}

float BoxStyle::convertLengthValue(const CssValue& value) const
{
    return CssLengthResolver(document(), font()).resolveLength(value);
}

float BoxStyle::convertLineWidth(const CssValue& value) const
{
    if(is<CssIdentValue>(value)) {
        const auto& ident = to<CssIdentValue>(value);
        switch(ident.value()) {
        case CssValueID::Thin:
            return 1.0;
        case CssValueID::Medium:
            return 3.0;
        case CssValueID::Thick:
            return 5.0;
        default:
            assert(false);
        }
    }

    return convertLengthValue(value);
}

float BoxStyle::convertSpacing(const CssValue& value) const
{
    if(is<CssIdentValue>(value)) {
        const auto& ident = to<CssIdentValue>(value);
        assert(ident.value() == CssValueID::Normal);
        return 0.f;
    }

    return convertLengthValue(value);
}

float BoxStyle::convertLengthOrPercent(float maximum, const CssValue& value) const
{
    if(is<CssPercentValue>(value)) {
        const auto& percent = to<CssPercentValue>(value);
        return percent.value() * maximum / 100.f;
    }

    return convertLengthValue(value);
}

std::optional<float> BoxStyle::convertLengthOrAuto(const CssValue& value) const
{
    if(is<CssIdentValue>(value)) {
        const auto& ident = to<CssIdentValue>(value);
        assert(ident.value() == CssValueID::Auto);
        return std::nullopt;
    }

    return convertLengthValue(value);
}

std::optional<float> BoxStyle::convertLengthOrNormal(const CssValue& value) const
{
    if(is<CssIdentValue>(value)) {
        const auto& ident = to<CssIdentValue>(value);
        assert(ident.value() == CssValueID::Normal);
        return std::nullopt;
    }

    return convertLengthValue(value);
}

Length BoxStyle::convertLength(const CssValue& value) const
{
    return Length(Length::Type::Fixed, convertLengthValue(value));
}

Length BoxStyle::convertLengthOrPercent(const CssValue& value) const
{
    if(is<CssPercentValue>(value)) {
        const auto& percent = to<CssPercentValue>(value);
        return Length(Length::Type::Percent, percent.value());
    }

    return convertLength(value);
}

Length BoxStyle::convertLengthOrPercentOrAuto(const CssValue& value) const
{
    if(is<CssIdentValue>(value)) {
        const auto& ident = to<CssIdentValue>(value);
        assert(ident.value() == CssValueID::Auto);
        return Length::Auto;
    }

    return convertLengthOrPercent(value);
}

Length BoxStyle::convertLengthOrPercentOrNone(const CssValue& value) const
{
    if(is<CssIdentValue>(value)) {
        const auto& ident = to<CssIdentValue>(value);
        assert(ident.value() == CssValueID::None);
        return Length::None;
    }

    return convertLengthOrPercent(value);
}

Length BoxStyle::convertWidthOrHeightLength(const CssValue& value) const
{
    if(is<CssIdentValue>(value)) {
        const auto& ident = to<CssIdentValue>(value);
        switch(ident.value()) {
        case CssValueID::None:
            return Length::None;
        case CssValueID::Auto:
            return Length::Auto;
        case CssValueID::MinContent:
            return Length::MinContent;
        case CssValueID::MaxContent:
            return Length::MaxContent;
        case CssValueID::FitContent:
            return Length::FitContent;
        default:
            assert(false);
        };
    }

    return convertLengthOrPercent(value);
}

Length BoxStyle::convertPositionComponent(CssValueID min, CssValueID max, const CssValue& value) const
{
    if(is<CssIdentValue>(value)) {
        const auto& ident = to<CssIdentValue>(value);
        constexpr auto mid = CssValueID::Center;
        if(min == ident.value())
            return Length(Length::Type::Percent, 0);
        if(mid == ident.value())
            return Length(Length::Type::Percent, 50);
        if(max == ident.value())
            return Length(Length::Type::Percent, 100);
        assert(false);
    }

    return convertLengthOrPercent(value);
}

LengthPoint BoxStyle::convertPositionCoordinate(const CssValue& value) const
{
    const auto& pair = to<CssPairValue>(value);
    auto horizontal = convertPositionComponent(CssValueID::Left, CssValueID::Right, *pair.first());
    auto vertical = convertPositionComponent(CssValueID::Top, CssValueID::Bottom, *pair.second());
    return LengthPoint(horizontal, vertical);
}

LengthSize BoxStyle::convertBorderRadius(const CssValue& value) const
{
    const auto& pair = to<CssPairValue>(value);
    auto horizontal = convertLengthOrPercent(*pair.first());
    auto vertical = convertLengthOrPercent(*pair.second());
    return LengthSize(horizontal, vertical);
}

Color BoxStyle::convertColor(const CssValue& value) const
{
    if(is<CssIdentValue>(value)) {
        const auto& ident = to<CssIdentValue>(value);
        assert(ident.value() == CssValueID::CurrentColor);
        return m_color;
    }

    return to<CssColorValue>(value).value();
}

Paint BoxStyle::convertPaint(const CssValue& value) const
{
    if(value.id() == CssValueID::None)
        return Paint();
    if(is<CssLocalUrlValue>(value)) {
        const auto& url = to<CssLocalUrlValue>(value);
        return Paint(url.value());
    }

    if(is<CssPairValue>(value)) {
        const auto& pair = to<CssPairValue>(value);
        const auto& url = to<CssLocalUrlValue>(*pair.first());
        if(auto ident = to<CssIdentValue>(pair.second())) {
            if(ident->value() == CssValueID::None) {
                return Paint(url.value());
            }
        }

        return Paint(url.value(), convertColor(*pair.second()));
    }

    return Paint(convertColor(value));
}

RefPtr<Image> BoxStyle::convertImage(const CssValue& value) const
{
    return to<CssImageValue>(value).fetch(document());
}

RefPtr<Image> BoxStyle::convertImageOrNone(const CssValue& value) const
{
    if(is<CssIdentValue>(value)) {
        const auto& ident = to<CssIdentValue>(value);
        assert(ident.value() == CssValueID::None);
        return nullptr;
    }

    return convertImage(value);
}

Display BoxStyle::convertDisplay(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::None:
        return Display::None;
    case CssValueID::Block:
        return Display::Block;
    case CssValueID::Flex:
        return Display::Flex;
    case CssValueID::Inline:
        return Display::Inline;
    case CssValueID::InlineBlock:
        return Display::InlineBlock;
    case CssValueID::InlineFlex:
        return Display::InlineFlex;
    case CssValueID::InlineTable:
        return Display::InlineTable;
    case CssValueID::ListItem:
        return Display::ListItem;
    case CssValueID::Table:
        return Display::Table;
    case CssValueID::TableCaption:
        return Display::TableCaption;
    case CssValueID::TableCell:
        return Display::TableCell;
    case CssValueID::TableColumn:
        return Display::TableColumn;
    case CssValueID::TableColumnGroup:
        return Display::TableColumnGroup;
    case CssValueID::TableFooterGroup:
        return Display::TableFooterGroup;
    case CssValueID::TableHeaderGroup:
        return Display::TableHeaderGroup;
    case CssValueID::TableRow:
        return Display::TableRow;
    case CssValueID::TableRowGroup:
        return Display::TableRowGroup;
    default:
        assert(false);
    }

    return Display::None;
}

Position BoxStyle::convertPosition(const CssValue& value)
{
    if(is<CssUnaryFunctionValue>(value)) {
        const auto& function = to<CssUnaryFunctionValue>(value);
        assert(function.id() == CssFunctionID::Running);
        return Position::Running;
    }

    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Static:
        return Position::Static;
    case CssValueID::Relative:
        return Position::Relative;
    case CssValueID::Absolute:
        return Position::Absolute;
    case CssValueID::Fixed:
        return Position::Fixed;
    default:
        assert(false);
    }

    return Position::Static;
}

Float BoxStyle::convertFloat(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::None:
        return Float::None;
    case CssValueID::Left:
        return Float::Left;
    case CssValueID::Right:
        return Float::Right;
    default:
        assert(false);
    }

    return Float::None;
}

Clear BoxStyle::convertClear(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::None:
        return Clear::None;
    case CssValueID::Left:
        return Clear::Left;
    case CssValueID::Right:
        return Clear::Right;
    case CssValueID::Both:
        return Clear::Both;
    default:
        assert(false);
    }

    return Clear::None;
}

VerticalAlignType BoxStyle::convertVerticalAlignType(const CssValue& value)
{
    if(is<CssIdentValue>(value)) {
        const auto& ident = to<CssIdentValue>(value);
        switch(ident.value()) {
        case CssValueID::Baseline:
            return VerticalAlignType::Baseline;
        case CssValueID::Sub:
            return VerticalAlignType::Sub;
        case CssValueID::Super:
            return VerticalAlignType::Super;
        case CssValueID::TextTop:
            return VerticalAlignType::TextTop;
        case CssValueID::TextBottom:
            return VerticalAlignType::TextBottom;
        case CssValueID::Middle:
            return VerticalAlignType::Middle;
        case CssValueID::Top:
            return VerticalAlignType::Top;
        case CssValueID::Bottom:
            return VerticalAlignType::Bottom;
        default:
            assert(false);
        }
    }

    return VerticalAlignType::Length;
}

Direction BoxStyle::convertDirection(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Ltr:
        return Direction::Ltr;
    case CssValueID::Rtl:
        return Direction::Rtl;
    default:
        assert(false);
    }

    return Direction::Ltr;
}

UnicodeBidi BoxStyle::convertUnicodeBidi(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Normal:
        return UnicodeBidi::Normal;
    case CssValueID::Embed:
        return UnicodeBidi::Embed;
    case CssValueID::BidiOverride:
        return UnicodeBidi::BidiOverride;
    case CssValueID::Isolate:
        return UnicodeBidi::Isolate;
    case CssValueID::IsolateOverride:
        return UnicodeBidi::IsolateOverride;
    default:
        assert(false);
    }

    return UnicodeBidi::Normal;
}

Visibility BoxStyle::convertVisibility(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Visible:
        return Visibility::Visible;
    case CssValueID::Hidden:
        return Visibility::Hidden;
    case CssValueID::Collapse:
        return Visibility::Collapse;
    default:
        assert(false);
    }

    return Visibility::Visible;
}

LineStyle BoxStyle::convertLineStyle(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::None:
        return LineStyle::None;
    case CssValueID::Hidden:
        return LineStyle::Hidden;
    case CssValueID::Inset:
        return LineStyle::Inset;
    case CssValueID::Groove:
        return LineStyle::Groove;
    case CssValueID::Outset:
        return LineStyle::Outset;
    case CssValueID::Ridge:
        return LineStyle::Ridge;
    case CssValueID::Dotted:
        return LineStyle::Dotted;
    case CssValueID::Dashed:
        return LineStyle::Dashed;
    case CssValueID::Solid:
        return LineStyle::Solid;
    case CssValueID::Double:
        return LineStyle::Double;
    default:
        assert(false);
    }

    return LineStyle::None;
}

BackgroundBox BoxStyle::convertBackgroundBox(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::BorderBox:
        return BackgroundBox::BorderBox;
    case CssValueID::PaddingBox:
        return BackgroundBox::PaddingBox;
    case CssValueID::ContentBox:
        return BackgroundBox::ContentBox;
    default:
        assert(false);
    }

    return BackgroundBox::BorderBox;
}

WritingMode BoxStyle::convertWritingMode(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::HorizontalTb:
        return WritingMode::HorizontalTb;
    case CssValueID::VerticalRl:
        return WritingMode::VerticalRl;
    case CssValueID::VerticalLr:
        return WritingMode::VerticalLr;
    default:
        assert(false);
    }

    return WritingMode::HorizontalTb;
}

TextOrientation BoxStyle::convertTextOrientation(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Mixed:
        return TextOrientation::Mixed;
    case CssValueID::Upright:
        return TextOrientation::Upright;
    default:
        assert(false);
    }

    return TextOrientation::Mixed;
}

TextAlign BoxStyle::convertTextAlign(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Left:
        return TextAlign::Left;
    case CssValueID::Right:
        return TextAlign::Right;
    case CssValueID::Center:
        return TextAlign::Center;
    case CssValueID::Justify:
        return TextAlign::Justify;
    case CssValueID::Start:
        return TextAlign::Start;
    case CssValueID::End:
        return TextAlign::End;
    default:
        assert(false);
    }

    return TextAlign::Left;
}

WhiteSpace BoxStyle::convertWhiteSpace(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Normal:
        return WhiteSpace::Normal;
    case CssValueID::Pre:
        return WhiteSpace::Pre;
    case CssValueID::PreWrap:
        return WhiteSpace::PreWrap;
    case CssValueID::PreLine:
        return WhiteSpace::PreLine;
    case CssValueID::Nowrap:
        return WhiteSpace::Nowrap;
    default:
        assert(false);
    }

    return WhiteSpace::Normal;
}

WordBreak BoxStyle::convertWordBreak(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Normal:
        return WordBreak::Normal;
    case CssValueID::KeepAll:
        return WordBreak::KeepAll;
    case CssValueID::BreakAll:
        return WordBreak::BreakAll;
    case CssValueID::BreakWord:
        return WordBreak::BreakWord;
    default:
        assert(false);
    }

    return WordBreak::Normal;
}

OverflowWrap BoxStyle::convertOverflowWrap(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Normal:
        return OverflowWrap::Normal;
    case CssValueID::Anywhere:
        return OverflowWrap::Anywhere;
    case CssValueID::BreakWord:
        return OverflowWrap::BreakWord;
    default:
        assert(false);
    }

    return OverflowWrap::Normal;
}

BoxSizing BoxStyle::convertBoxSizing(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::BorderBox:
        return BoxSizing::BorderBox;
    case CssValueID::ContentBox:
        return BoxSizing::ContentBox;
    default:
        assert(false);
    }

    return BoxSizing::BorderBox;
}

BlendMode BoxStyle::convertBlendMode(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Normal:
        return BlendMode::Normal;
    case CssValueID::Multiply:
        return BlendMode::Multiply;
    case CssValueID::Screen:
        return BlendMode::Screen;
    case CssValueID::Overlay:
        return BlendMode::Overlay;
    case CssValueID::Darken:
        return BlendMode::Darken;
    case CssValueID::Lighten:
        return BlendMode::Lighten;
    case CssValueID::ColorDodge:
        return BlendMode::ColorDodge;
    case CssValueID::ColorBurn:
        return BlendMode::ColorBurn;
    case CssValueID::HardLight:
        return BlendMode::HardLight;
    case CssValueID::SoftLight:
        return BlendMode::SoftLight;
    case CssValueID::Difference:
        return BlendMode::Difference;
    case CssValueID::Exclusion:
        return BlendMode::Exclusion;
    case CssValueID::Hue:
        return BlendMode::Hue;
    case CssValueID::Saturation:
        return BlendMode::Saturation;
    case CssValueID::Color:
        return BlendMode::Color;
    case CssValueID::Luminosity:
        return BlendMode::Luminosity;
    default:
        assert(false);
    }

    return BlendMode::Normal;
}

MaskType BoxStyle::convertMaskType(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Luminance:
        return MaskType::Luminance;
    case CssValueID::Alpha:
        return MaskType::Alpha;
    default:
        assert(false);
    }

    return MaskType::Luminance;
}

AlignContent BoxStyle::convertAlignContent(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::FlexStart:
        return AlignContent::FlexStart;
    case CssValueID::FlexEnd:
        return AlignContent::FlexEnd;
    case CssValueID::Center:
        return AlignContent::Center;
    case CssValueID::SpaceBetween:
        return AlignContent::SpaceBetween;
    case CssValueID::SpaceAround:
        return AlignContent::SpaceAround;
    case CssValueID::SpaceEvenly:
        return AlignContent::SpaceEvenly;
    case CssValueID::Stretch:
        return AlignContent::Stretch;
    default:
        assert(false);
    }

    return AlignContent::FlexStart;
}

AlignItem BoxStyle::convertAlignItem(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Auto:
        return AlignItem::Auto;
    case CssValueID::FlexStart:
        return AlignItem::FlexStart;
    case CssValueID::FlexEnd:
        return AlignItem::FlexEnd;
    case CssValueID::Center:
        return AlignItem::Center;
    case CssValueID::Stretch:
        return AlignItem::Stretch;
    case CssValueID::Baseline:
        return AlignItem::Baseline;
    default:
        assert(false);
    }

    return AlignItem::Auto;
}

FillRule BoxStyle::convertFillRule(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Nonzero:
        return FillRule::NonZero;
    case CssValueID::Evenodd:
        return FillRule::EvenOdd;
    default:
        assert(false);
    }

    return FillRule::NonZero;
}

CaptionSide BoxStyle::convertCaptionSide(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Top:
        return CaptionSide::Top;
    case CssValueID::Bottom:
        return CaptionSide::Bottom;
    default:
        assert(false);
    }

    return CaptionSide::Top;
}

EmptyCells BoxStyle::convertEmptyCells(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Show:
        return EmptyCells::Show;
    case CssValueID::Hide:
        return EmptyCells::Hide;
    default:
        assert(false);
    }

    return EmptyCells::Show;
}

BorderCollapse BoxStyle::convertBorderCollapse(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Separate:
        return BorderCollapse::Separate;
    case CssValueID::Collapse:
        return BorderCollapse::Collapse;
    default:
        assert(false);
    }

    return BorderCollapse::Separate;
}

BreakBetween BoxStyle::convertBreakBetween(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Auto:
        return BreakBetween::Auto;
    case CssValueID::Avoid:
        return BreakBetween::Avoid;
    case CssValueID::AvoidColumn:
        return BreakBetween::AvoidColumn;
    case CssValueID::AvoidPage:
        return BreakBetween::AvoidPage;
    case CssValueID::Column:
        return BreakBetween::Column;
    case CssValueID::Page:
        return BreakBetween::Page;
    case CssValueID::Left:
        return BreakBetween::Left;
    case CssValueID::Right:
        return BreakBetween::Right;
    case CssValueID::Recto:
        return BreakBetween::Recto;
    case CssValueID::Verso:
        return BreakBetween::Verso;
    default:
        assert(false);
    }

    return BreakBetween::Auto;
}

BreakInside BoxStyle::convertBreakInside(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::Auto:
        return BreakInside::Auto;
    case CssValueID::Avoid:
        return BreakInside::Avoid;
    case CssValueID::AvoidColumn:
        return BreakInside::AvoidColumn;
    case CssValueID::AvoidPage:
        return BreakInside::AvoidPage;
    default:
        assert(false);
    }

    return BreakInside::Auto;
}

PageSize BoxStyle::convertPageSize(const CssValue& value)
{
    const auto& ident = to<CssIdentValue>(value);
    switch(ident.value()) {
    case CssValueID::A3:
        return PageSize::A3;
    case CssValueID::A4:
        return PageSize::A4;
    case CssValueID::A5:
        return PageSize::A5;
    case CssValueID::B4:
        return PageSize::B4;
    case CssValueID::B5:
        return PageSize::B5;
    case CssValueID::Ledger:
        return PageSize::Ledger;
    case CssValueID::Legal:
        return PageSize::Legal;
    case CssValueID::Letter:
        return PageSize::Letter;
    default:
        assert(false);
    }

    return PageSize::A4;
}

int BoxStyle::convertInteger(const CssValue& value)
{
    return to<CssIntegerValue>(value).value();
}

std::optional<int> BoxStyle::convertIntegerOrAuto(const CssValue& value)
{
    if(is<CssIdentValue>(value)) {
        const auto& ident = to<CssIdentValue>(value);
        assert(ident.value() == CssValueID::Auto);
        return std::nullopt;
    }

    return convertInteger(value);
}

float BoxStyle::convertNumber(const CssValue& value)
{
    return to<CssNumberValue>(value).value();
}

float BoxStyle::convertNumberOrPercent(const CssValue& value)
{
    if(is<CssPercentValue>(value)) {
        const auto& percent = to<CssPercentValue>(value);
        return percent.value() / 100.f;
    }

    return convertNumber(value);
}

float BoxStyle::convertAngle(const CssValue& value)
{
    return to<CssAngleValue>(value).valueInDegrees();
}

GlobalString BoxStyle::convertCustomIdent(const CssValue& value)
{
    return to<CssCustomIdentValue>(value).value();
}

HeapString BoxStyle::convertLocalUrl(const CssValue& value)
{
    return to<CssLocalUrlValue>(value).value();
}

HeapString BoxStyle::convertLocalUrlOrNone(const CssValue& value)
{
    if(is<CssIdentValue>(value)) {
        const auto& ident = to<CssIdentValue>(value);
        assert(ident.value() == CssValueID::None);
        return emptyGlo;
    }

    return convertLocalUrl(value);
}

BoxStyle::~BoxStyle() = default;

BoxStyle::BoxStyle(Node* node, PseudoType pseudoType, Display display)
    : m_node(node), m_properties(node->heap())
    , m_customProperties(node->heap())
    , m_pseudoType(pseudoType), m_display(display)
{
}

} // namespace plutobook
