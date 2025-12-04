#pragma once

#include "pointer.h"
#include "global-string.h"
#include "css-tokenizer.h"
#include "css-property-id.h"
#include "color.h"
#include "url.h"

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <forward_list>
#include <memory>
#include <vector>

namespace plutobook {
    enum class CssValueID : uint16_t {
        A3,
        A4,
        A5,
        Absolute,
        Additive,
        AfterEdge,
        All,
        AllPetiteCaps,
        AllSmallCaps,
        Alpha,
        Alphabetic,
        Anywhere,
        Auto,
        Avoid,
        AvoidColumn,
        AvoidPage,
        B4,
        B5,
        Balance,
        Baseline,
        BeforeEdge,
        Bevel,
        BidiOverride,
        Block,
        Bold,
        Bolder,
        BorderBox,
        Both,
        Bottom,
        BreakAll,
        BreakWord,
        Butt,
        Capitalize,
        Center,
        Central,
        Circle,
        Clip,
        CloseQuote,
        Collapse,
        Color,
        ColorBurn,
        ColorDodge,
        Column,
        ColumnReverse,
        CommonLigatures,
        Condensed,
        Contain,
        ContentBox,
        Contextual,
        Cover,
        CurrentColor,
        Cyclic,
        Darken,
        Dashed,
        DiagonalFractions,
        Difference,
        Disc,
        DiscretionaryLigatures,
        Dotted,
        Double,
        Ellipsis,
        Embed,
        Emoji,
        End,
        Evenodd,
        Exclusion,
        Expanded,
        Extends,
        ExtraCondensed,
        ExtraExpanded,
        Fill,
        FitContent,
        Fixed,
        Flex,
        FlexEnd,
        FlexStart,
        FullWidth,
        Groove,
        Hanging,
        HardLight,
        Hidden,
        Hide,
        HistoricalLigatures,
        HorizontalTb,
        Hue,
        Ideographic,
        Infinite,
        Inline,
        InlineBlock,
        InlineFlex,
        InlineTable,
        Inset,
        Inside,
        Isolate,
        IsolateOverride,
        Italic,
        Jis04,
        Jis78,
        Jis83,
        Jis90,
        Justify,
        KeepAll,
        Landscape,
        Large,
        Larger,
        Ledger,
        Left,
        Legal,
        Letter,
        Lighten,
        Lighter,
        LineThrough,
        LiningNums,
        ListItem,
        Local,
        Lowercase,
        Ltr,
        Luminance,
        Luminosity,
        Manual,
        Markers,
        Mathematical,
        MaxContent,
        Medium,
        Middle,
        MinContent,
        Miter,
        Mixed,
        Multiply,
        NoChange,
        NoCloseQuote,
        NoCommonLigatures,
        NoContextual,
        NoDiscretionaryLigatures,
        NoHistoricalLigatures,
        NoOpenQuote,
        NoRepeat,
        NonScalingStroke,
        None,
        Nonzero,
        Normal,
        Nowrap,
        Numeric,
        Oblique,
        Off,
        OldstyleNums,
        On,
        OpenQuote,
        Ordinal,
        Outset,
        Outside,
        Overlay,
        Overline,
        PaddingBox,
        Page,
        PetiteCaps,
        Portrait,
        Pre,
        PreLine,
        PreWrap,
        ProportionalNums,
        ProportionalWidth,
        Recto,
        Relative,
        Repeat,
        RepeatX,
        RepeatY,
        ResetSize,
        Ridge,
        Right,
        Round,
        Row,
        RowReverse,
        Rtl,
        Ruby,
        Saturation,
        ScaleDown,
        Screen,
        Scroll,
        SemiCondensed,
        SemiExpanded,
        Separate,
        Show,
        Simplified,
        SlashedZero,
        Small,
        SmallCaps,
        Smaller,
        SoftLight,
        Solid,
        Space,
        SpaceAround,
        SpaceBetween,
        SpaceEvenly,
        Square,
        StackedFractions,
        Start,
        Static,
        Stretch,
        Stroke,
        Sub,
        Super,
        Symbolic,
        Table,
        TableCaption,
        TableCell,
        TableColumn,
        TableColumnGroup,
        TableFooterGroup,
        TableHeaderGroup,
        TableRow,
        TableRowGroup,
        TabularNums,
        Text,
        TextAfterEdge,
        TextBeforeEdge,
        TextBottom,
        TextTop,
        Thick,
        Thin,
        TitlingCaps,
        Top,
        Traditional,
        UltraCondensed,
        UltraExpanded,
        Underline,
        Unicase,
        Unicode,
        Uppercase,
        Upright,
        UseScript,
        Verso,
        VerticalLr,
        VerticalRl,
        Visible,
        Wavy,
        Wrap,
        WrapReverse,
        XLarge,
        XSmall,
        XxLarge,
        XxSmall,
        XxxLarge,
        LastCssValueID
    };

    constexpr auto kNumCssValueIDs =
        static_cast<int>(CssValueID::LastCssValueID);

    enum class CssValueType {
        Initial,
        Inherit,
        Unset,
        Ident,
        CustomIdent,
        CustomProperty,
        VariableReference,
        Integer,
        Number,
        Percent,
        Angle,
        Length,
        Calc,
        Attr,
        String,
        LocalUrl,
        Url,
        Image,
        Color,
        Counter,
        FontFeature,
        FontVariation,
        UnicodeRange,
        Pair,
        Rect,
        List,
        Function,
        UnaryFunction
    };

    class CssValue : public RefCounted<CssValue> {
    public:
        using ClassRoot = CssValue;
        using ClassKind = CssValueType;

        virtual ~CssValue() = default;
        ClassKind type() const noexcept { return m_type; }
        bool hasID(CssValueID id) const;

    protected:
        explicit CssValue(ClassKind type) noexcept : m_type(type) {}
        ClassKind m_type;
    };

    using CssValueList = std::vector<RefPtr<CssValue>>;

    enum class CssStyleOrigin : uint8_t {
        UserAgent,
        PresentationAttribute,
        Author,
        Inline,
        User
    };

    class CssProperty {
    public:
        CssProperty(CssPropertyID id, CssStyleOrigin origin, bool important,
                    RefPtr<CssValue> value)
            : m_id(id), m_origin(origin), m_important(important),
              m_value(std::move(value)) {}

        CssPropertyID id() const { return m_id; }
        CssStyleOrigin origin() const { return m_origin; }
        bool important() const { return m_important; }
        const RefPtr<CssValue>& value() const { return m_value; }

    protected:
        CssPropertyID m_id;
        CssStyleOrigin m_origin;
        bool m_important;
        RefPtr<CssValue> m_value;
    };

    using CssPropertyList = std::vector<CssProperty>;

    class CssInitialValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Initial;

        static RefPtr<CssInitialValue> create();

    private:
        CssInitialValue() noexcept : CssValue(classKind) {}
        friend class CssValuePool;
    };

    class CssInheritValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Inherit;

        static RefPtr<CssInheritValue> create();

    private:
        CssInheritValue() noexcept : CssValue(classKind) {}
        friend class CssValuePool;
    };

    class CssUnsetValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Unset;

        static RefPtr<CssUnsetValue> create();

    private:
        CssUnsetValue() noexcept : CssValue(classKind) {}
        friend class CssValuePool;
    };

    class CssIdentValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Ident;

        static RefPtr<CssIdentValue> create(CssValueID value);

        CssValueID value() const { return m_value; }

    private:
        CssIdentValue(CssValueID value) : CssValue(classKind), m_value(value) {}
        friend class CssValuePool;
        CssValueID m_value;
    };

    inline bool CssValue::hasID(CssValueID id) const {
        if (is<CssIdentValue>(*this))
            return to<CssIdentValue>(*this).value() == id;
        return false;
    }

    class CssCustomIdentValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::CustomIdent;

        static RefPtr<CssCustomIdentValue> create(GlobalString value);

        GlobalString value() const { return m_value; }

    private:
        CssCustomIdentValue(GlobalString value)
            : CssValue(classKind), m_value(value) {}
        GlobalString m_value;
    };

    inline RefPtr<CssCustomIdentValue>
    CssCustomIdentValue::create(GlobalString value) {
        return adoptPtr(new CssCustomIdentValue(value));
    }

    class BoxStyle;

    class CssVariableData : public RefCounted<CssVariableData> {
    public:
        static RefPtr<CssVariableData> create(const CssTokenStream& value);

        bool
        resolve(const BoxStyle* style, CssTokenList& tokens,
                boost::unordered_flat_set<CssVariableData*>& references) const;

    private:
        CssVariableData(const CssTokenStream& value);
        bool
        resolve(CssTokenStream input, const BoxStyle* style,
                CssTokenList& tokens,
                boost::unordered_flat_set<CssVariableData*>& references) const;
        bool resolveVar(
            CssTokenStream input, const BoxStyle* style, CssTokenList& tokens,
            boost::unordered_flat_set<CssVariableData*>& references) const;
        std::vector<CssToken> m_tokens;
    };

    class CssCustomPropertyValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::CustomProperty;

        static RefPtr<CssCustomPropertyValue>
        create(GlobalString name, RefPtr<CssVariableData> value);

        GlobalString name() const { return m_name; }
        const RefPtr<CssVariableData>& value() const { return m_value; }

    private:
        CssCustomPropertyValue(GlobalString name,
                               RefPtr<CssVariableData> value);
        GlobalString m_name;
        RefPtr<CssVariableData> m_value;
    };

    class Node;

    class CssParserContext {
    public:
        CssParserContext(const Node* node, CssStyleOrigin origin, Url baseUrl);

        bool inHtmlDocument() const { return m_inHtmlDocument; }
        bool inSvgElement() const { return m_inSvgElement; }

        CssStyleOrigin origin() const { return m_origin; }
        const Url& baseUrl() const { return m_baseUrl; }

        Url completeUrl(std::string_view url) const {
            return m_baseUrl.complete(url);
        }

    private:
        bool m_inHtmlDocument;
        bool m_inSvgElement;
        CssStyleOrigin m_origin;
        Url m_baseUrl;
    };

    class CssVariableReferenceValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::VariableReference;

        static RefPtr<CssVariableReferenceValue>
        create(const CssParserContext& context, CssPropertyID id,
               bool important, RefPtr<CssVariableData> value);

        const CssParserContext& context() const { return m_context; }
        CssPropertyID id() const { return m_id; }
        bool important() const { return m_important; }
        const RefPtr<CssVariableData>& value() const { return m_value; }

        CssPropertyList resolve(const BoxStyle* style) const;

    private:
        CssVariableReferenceValue(const CssParserContext& context,
                                  CssPropertyID id, bool important,
                                  RefPtr<CssVariableData> value);
        CssParserContext m_context;
        CssPropertyID m_id;
        bool m_important;
        RefPtr<CssVariableData> m_value;
    };

    class CssIntegerValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Integer;

        static RefPtr<CssIntegerValue> create(int value);

        int value() const { return m_value; }

    private:
        CssIntegerValue(int value) : CssValue(classKind), m_value(value) {}
        int m_value;
    };

    inline RefPtr<CssIntegerValue> CssIntegerValue::create(int value) {
        return adoptPtr(new CssIntegerValue(value));
    }

    class CssNumberValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Number;

        static RefPtr<CssNumberValue> create(float value);

        float value() const { return m_value; }

    private:
        CssNumberValue(float value) : CssValue(classKind), m_value(value) {}
        float m_value;
    };

    inline RefPtr<CssNumberValue> CssNumberValue::create(float value) {
        return adoptPtr(new CssNumberValue(value));
    }

    class CssPercentValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Percent;

        static RefPtr<CssPercentValue> create(float value);

        float value() const { return m_value; }

    private:
        CssPercentValue(float value) : CssValue(classKind), m_value(value) {}
        float m_value;
    };

    inline RefPtr<CssPercentValue> CssPercentValue::create(float value) {
        return adoptPtr(new CssPercentValue(value));
    }

    class CssAngleValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Angle;

        enum class Unit { Degrees, Radians, Gradians, Turns };

        static RefPtr<CssAngleValue> create(float value, Unit unit);

        float value() const { return m_value; }
        Unit unit() const { return m_unit; }

        float valueInDegrees() const;

    private:
        CssAngleValue(float value, Unit unit)
            : CssValue(classKind), m_value(value), m_unit(unit) {}

        float m_value;
        Unit m_unit;
    };

    inline RefPtr<CssAngleValue> CssAngleValue::create(float value, Unit unit) {
        return adoptPtr(new CssAngleValue(value, unit));
    }

    enum class CssLengthUnits : uint8_t {
        None,
        Pixels,
        Points,
        Picas,
        Centimeters,
        Millimeters,
        Inches,
        ViewportWidth,
        ViewportHeight,
        ViewportMin,
        ViewportMax,
        Ems,
        Exs,
        Chs,
        Rems
    };

    class CssLengthValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Length;

        static RefPtr<CssLengthValue>
        create(float value, CssLengthUnits units = CssLengthUnits::Pixels);

        float value() const { return m_value; }
        CssLengthUnits units() const { return m_units; }

    private:
        CssLengthValue(float value, CssLengthUnits units)
            : CssValue(classKind), m_value(value), m_units(units) {}

        float m_value;
        CssLengthUnits m_units;
    };

    inline RefPtr<CssLengthValue> CssLengthValue::create(float value,
                                                         CssLengthUnits units) {
        return adoptPtr(new CssLengthValue(value, units));
    }

    class Font;
    class Document;

    class CssLengthResolver {
    public:
        CssLengthResolver(const Document* document, const Font* font);

        float resolveLength(const CssValue& value) const;
        float resolveLength(const CssLengthValue& length) const;
        float resolveLength(float value, CssLengthUnits units) const;

    private:
        float emFontSize() const;
        float exFontSize() const;
        float chFontSize() const;
        float remFontSize() const;

        float viewportWidth() const;
        float viewportHeight() const;
        float viewportMin() const;
        float viewportMax() const;

        const Document* m_document;
        const Font* m_font;
    };

    enum class CssCalcOperator : uint8_t { None, Add, Sub, Mul, Div, Min, Max };

    struct CssCalc {
        CssCalc() = default;
        explicit CssCalc(CssCalcOperator op) : op(op) {}
        CssCalc(float value, CssLengthUnits units = CssLengthUnits::None)
            : value(value), units(units) {}

        float value = 0;
        CssLengthUnits units = CssLengthUnits::None;
        CssCalcOperator op = CssCalcOperator::None;
    };

    using CssCalcList = std::vector<CssCalc>;

    class CssCalcValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Calc;

        static RefPtr<CssCalcValue> create(bool negative, bool unitless,
                                           CssCalcList values);

        const bool negative() const { return m_negative; }
        const bool unitless() const { return m_unitless; }
        const CssCalcList& values() const { return m_values; }

        float resolve(const CssLengthResolver& resolver) const;

    private:
        CssCalcValue(bool negative, bool unitless, CssCalcList values)
            : CssValue(classKind), m_negative(negative), m_unitless(unitless),
              m_values(std::move(values)) {}

        const bool m_negative;
        const bool m_unitless;
        CssCalcList m_values;
    };

    inline RefPtr<CssCalcValue>
    CssCalcValue::create(bool negative, bool unitless, CssCalcList values) {
        return adoptPtr(
            new CssCalcValue(negative, unitless, std::move(values)));
    }

    class CssAttrValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Attr;

        static RefPtr<CssAttrValue> create(GlobalString name,
                                           const HeapString& fallback);

        GlobalString name() const { return m_name; }
        const HeapString& fallback() const { return m_fallback; }

    private:
        CssAttrValue(GlobalString name, const HeapString& fallback)
            : CssValue(classKind), m_name(name), m_fallback(fallback) {}

        GlobalString m_name;
        HeapString m_fallback;
    };

    inline RefPtr<CssAttrValue>
    CssAttrValue::create(GlobalString name, const HeapString& fallback) {
        return adoptPtr(new CssAttrValue(name, fallback));
    }

    class CssStringValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::String;

        static RefPtr<CssStringValue> create(const HeapString& value);

        const HeapString& value() const { return m_value; }

    private:
        CssStringValue(const HeapString& value)
            : CssValue(classKind), m_value(value) {}
        HeapString m_value;
    };

    inline RefPtr<CssStringValue>
    CssStringValue::create(const HeapString& value) {
        return adoptPtr(new CssStringValue(value));
    }

    class CssLocalUrlValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::LocalUrl;

        static RefPtr<CssLocalUrlValue> create(const HeapString& value);

        const HeapString& value() const { return m_value; }

    private:
        CssLocalUrlValue(const HeapString& value)
            : CssValue(classKind), m_value(value) {}
        HeapString m_value;
    };

    inline RefPtr<CssLocalUrlValue>
    CssLocalUrlValue::create(const HeapString& value) {
        return adoptPtr(new CssLocalUrlValue(value));
    }

    class CssUrlValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Url;

        static RefPtr<CssUrlValue> create(Url value);

        const Url& value() const { return m_value; }

    private:
        CssUrlValue(Url value)
            : CssValue(classKind), m_value(std::move(value)) {}
        Url m_value;
    };

    inline RefPtr<CssUrlValue> CssUrlValue::create(Url value) {
        return adoptPtr(new CssUrlValue(std::move(value)));
    }

    class Image;

    class CssImageValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Image;

        static RefPtr<CssImageValue> create(Url value);

        const Url& value() const { return m_value; }
        const RefPtr<Image>& image() const { return m_image; }
        const RefPtr<Image>& fetch(Document* document) const;

    private:
        CssImageValue(Url value);
        Url m_value;
        mutable RefPtr<Image> m_image;
    };

    class CssColorValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Color;

        static RefPtr<CssColorValue> create(const Color& value);

        const Color& value() const { return m_value; }

    private:
        CssColorValue(Color value) : CssValue(classKind), m_value(value) {}
        Color m_value;
    };

    inline RefPtr<CssColorValue> CssColorValue::create(const Color& value) {
        return adoptPtr(new CssColorValue(value));
    }

    class CssCounterValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Counter;

        static RefPtr<CssCounterValue> create(GlobalString identifier,
                                              GlobalString listStyle,
                                              const HeapString& separator);

        GlobalString identifier() const { return m_identifier; }
        GlobalString listStyle() const { return m_listStyle; }
        const HeapString& separator() const { return m_separator; }

    private:
        CssCounterValue(GlobalString identifier, GlobalString listStyle,
                        const HeapString& separator)
            : CssValue(classKind), m_identifier(identifier),
              m_listStyle(listStyle), m_separator(separator) {}

        GlobalString m_identifier;
        GlobalString m_listStyle;
        HeapString m_separator;
    };

    inline RefPtr<CssCounterValue>
    CssCounterValue::create(GlobalString identifier, GlobalString listStyle,
                            const HeapString& separator) {
        return adoptPtr(new CssCounterValue(identifier, listStyle, separator));
    }

    class CssFontFeatureValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::FontFeature;

        static RefPtr<CssFontFeatureValue> create(GlobalString tag, int value);

        GlobalString tag() const { return m_tag; }
        int value() const { return m_value; }

    private:
        CssFontFeatureValue(GlobalString tag, int value)
            : CssValue(classKind), m_tag(tag), m_value(value) {}

        GlobalString m_tag;
        int m_value;
    };

    inline RefPtr<CssFontFeatureValue>
    CssFontFeatureValue::create(GlobalString tag, int value) {
        return adoptPtr(new CssFontFeatureValue(tag, value));
    }

    class CssFontVariationValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::FontVariation;

        static RefPtr<CssFontVariationValue> create(GlobalString tag,
                                                    float value);

        GlobalString tag() const { return m_tag; }
        float value() const { return m_value; }

    private:
        CssFontVariationValue(GlobalString tag, float value)
            : CssValue(classKind), m_tag(tag), m_value(value) {}

        GlobalString m_tag;
        float m_value;
    };

    inline RefPtr<CssFontVariationValue>
    CssFontVariationValue::create(GlobalString tag, float value) {
        return adoptPtr(new CssFontVariationValue(tag, value));
    }

    class CssUnicodeRangeValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::UnicodeRange;

        static RefPtr<CssUnicodeRangeValue> create(uint32_t from, uint32_t to);

        uint32_t from() const { return m_from; }
        uint32_t to() const { return m_to; }

    private:
        CssUnicodeRangeValue(uint32_t from, uint32_t to)
            : CssValue(classKind), m_from(from), m_to(to) {}

        uint32_t m_from;
        uint32_t m_to;
    };

    inline RefPtr<CssUnicodeRangeValue>
    CssUnicodeRangeValue::create(uint32_t from, uint32_t to) {
        return adoptPtr(new CssUnicodeRangeValue(from, to));
    }

    class CssPairValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Pair;

        static RefPtr<CssPairValue> create(RefPtr<CssValue> first,
                                           RefPtr<CssValue> second);

        const RefPtr<CssValue>& first() const { return m_first; }
        const RefPtr<CssValue>& second() const { return m_second; }

    private:
        CssPairValue(RefPtr<CssValue> first, RefPtr<CssValue> second)
            : CssValue(classKind), m_first(std::move(first)),
              m_second(std::move(second)) {}

        RefPtr<CssValue> m_first;
        RefPtr<CssValue> m_second;
    };

    inline RefPtr<CssPairValue> CssPairValue::create(RefPtr<CssValue> first,
                                                     RefPtr<CssValue> second) {
        return adoptPtr(new CssPairValue(std::move(first), std::move(second)));
    }

    class CssRectValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Rect;

        static RefPtr<CssRectValue> create(RefPtr<CssValue> top,
                                           RefPtr<CssValue> right,
                                           RefPtr<CssValue> bottom,
                                           RefPtr<CssValue> left);

        const RefPtr<CssValue>& position(Edge edge) const {
            return m_position[edge];
        }

    private:
        CssRectValue(RefPtr<CssValue> top, RefPtr<CssValue> right,
                     RefPtr<CssValue> bottom, RefPtr<CssValue> left)
            : CssValue(classKind), m_position{top, right, bottom, left} {}

        RefPtr<CssValue> m_position[4];
    };

    inline RefPtr<CssRectValue> CssRectValue::create(RefPtr<CssValue> top,
                                                     RefPtr<CssValue> right,
                                                     RefPtr<CssValue> bottom,
                                                     RefPtr<CssValue> left) {
        return adoptPtr(new CssRectValue(std::move(top), std::move(right),
                                         std::move(bottom), std::move(left)));
    }

    class CssAbstractListValue : public CssValue {
    public:
        using Iterator = CssValueList::const_iterator;
        Iterator begin() const { return m_values.begin(); }
        Iterator end() const { return m_values.end(); }

        const RefPtr<CssValue>& front() const { return m_values.front(); }
        const RefPtr<CssValue>& back() const { return m_values.back(); }
        const RefPtr<CssValue>& operator[](size_t index) const {
            return m_values[index];
        }
        const CssValueList& values() const { return m_values; }
        size_t size() const { return m_values.size(); }
        bool empty() const { return m_values.empty(); }

    protected:
        CssAbstractListValue(ClassKind type, CssValueList&& values)
            : CssValue(type), m_values(std::move(values)) {}
        CssValueList m_values;
    };

    class CssListValue final : public CssAbstractListValue {
    public:
        static constexpr ClassKind classKind = ClassKind::List;

        static RefPtr<CssListValue> create(CssValueList values);

    protected:
        CssListValue(CssValueList values)
            : CssAbstractListValue(classKind, std::move(values)) {}
        CssValueList m_values;
    };

    inline RefPtr<CssListValue> CssListValue::create(CssValueList values) {
        return adoptPtr(new CssListValue(std::move(values)));
    }

    enum class CssFunctionID {
        Element,
        Format,
        Leader,
        Local,
        Matrix,
        Qrcode,
        Rotate,
        Running,
        Scale,
        ScaleX,
        ScaleY,
        Skew,
        SkewX,
        SkewY,
        TargetCounter,
        TargetCounters,
        Translate,
        TranslateX,
        TranslateY
    };

    class CssFunctionValue final : public CssAbstractListValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Function;

        static RefPtr<CssFunctionValue> create(CssFunctionID id,
                                               CssValueList values);

        CssFunctionID id() const { return m_id; }

    private:
        CssFunctionValue(CssFunctionID id, CssValueList values)
            : CssAbstractListValue(classKind, std::move(values)), m_id(id) {}

        CssFunctionID m_id;
    };

    inline RefPtr<CssFunctionValue>
    CssFunctionValue::create(CssFunctionID id, CssValueList values) {
        return adoptPtr(new CssFunctionValue(id, std::move(values)));
    }

    class CssUnaryFunctionValue final : public CssValue {
    public:
        static constexpr ClassKind classKind = ClassKind::UnaryFunction;

        static RefPtr<CssUnaryFunctionValue> create(CssFunctionID id,
                                                    RefPtr<CssValue> value);

        CssFunctionID id() const { return m_id; }
        const RefPtr<CssValue>& value() const { return m_value; }

    private:
        CssUnaryFunctionValue(CssFunctionID id, RefPtr<CssValue> value)
            : CssValue(classKind), m_id(id), m_value(std::move(value)) {}

        CssFunctionID m_id;
        RefPtr<CssValue> m_value;
    };

    inline RefPtr<CssUnaryFunctionValue>
    CssUnaryFunctionValue::create(CssFunctionID id, RefPtr<CssValue> value) {
        return adoptPtr(new CssUnaryFunctionValue(id, std::move(value)));
    }

    class CssSimpleSelector;
    class CssComplexSelector;

    using CssCompoundSelector = std::forward_list<CssSimpleSelector>;
    using CssSelector = std::forward_list<CssComplexSelector>;

    using CssCompoundSelectorList = std::forward_list<CssCompoundSelector>;
    using CssSelectorList = std::forward_list<CssSelector>;

    using CssPageSelector = CssCompoundSelector;
    using CssPageSelectorList = CssCompoundSelectorList;

    enum class PseudoType : uint8_t;

    class CssSimpleSelector {
    public:
        enum class MatchType {
            Universal,
            Namespace,
            Tag,
            Id,
            Class,
            AttributeContains,
            AttributeDashEquals,
            AttributeEndsWith,
            AttributeEquals,
            AttributeHas,
            AttributeIncludes,
            AttributeStartsWith,
            PseudoClassActive,
            PseudoClassAnyLink,
            PseudoClassChecked,
            PseudoClassDisabled,
            PseudoClassEmpty,
            PseudoClassEnabled,
            PseudoClassFirstChild,
            PseudoClassFirstOfType,
            PseudoClassFocus,
            PseudoClassFocusVisible,
            PseudoClassFocusWithin,
            PseudoClassHas,
            PseudoClassHover,
            PseudoClassIs,
            PseudoClassLang,
            PseudoClassLastChild,
            PseudoClassLastOfType,
            PseudoClassLink,
            PseudoClassLocalLink,
            PseudoClassNot,
            PseudoClassNthChild,
            PseudoClassNthLastChild,
            PseudoClassNthLastOfType,
            PseudoClassNthOfType,
            PseudoClassOnlyChild,
            PseudoClassOnlyOfType,
            PseudoClassRoot,
            PseudoClassScope,
            PseudoClassTarget,
            PseudoClassTargetWithin,
            PseudoClassVisited,
            PseudoClassWhere,
            PseudoElementAfter,
            PseudoElementBefore,
            PseudoElementFirstLetter,
            PseudoElementFirstLine,
            PseudoElementMarker,
            PseudoPageBlank,
            PseudoPageFirst,
            PseudoPageLeft,
            PseudoPageName,
            PseudoPageNth,
            PseudoPageRight
        };

        enum class AttributeCaseType { Sensitive, InSensitive };

        using MatchPattern = std::pair<int, int>;

        explicit CssSimpleSelector(MatchType matchType)
            : m_matchType(matchType) {}
        CssSimpleSelector(MatchType matchType, GlobalString name)
            : m_matchType(matchType), m_name(name) {}
        CssSimpleSelector(MatchType matchType, const HeapString& value)
            : m_matchType(matchType), m_value(value) {}
        CssSimpleSelector(MatchType matchType, const MatchPattern& matchPattern)
            : m_matchType(matchType), m_matchPattern(matchPattern) {}
        CssSimpleSelector(MatchType matchType, CssSelectorList subSelectors)
            : m_matchType(matchType), m_subSelectors(std::move(subSelectors)) {}
        CssSimpleSelector(MatchType matchType,
                          AttributeCaseType attributeCaseType,
                          GlobalString name, const HeapString& value)
            : m_matchType(matchType), m_attributeCaseType(attributeCaseType),
              m_name(name), m_value(value) {}

        MatchType matchType() const { return m_matchType; }
        AttributeCaseType attributeCaseType() const {
            return m_attributeCaseType;
        }
        const MatchPattern& matchPattern() const { return m_matchPattern; }
        GlobalString name() const { return m_name; }
        const HeapString& value() const { return m_value; }
        const CssSelectorList& subSelectors() const { return m_subSelectors; }
        bool isCaseSensitive() const {
            return m_attributeCaseType == AttributeCaseType::Sensitive;
        }

        bool matchnth(int count) const;
        PseudoType pseudoType() const;
        uint32_t specificity() const;

    private:
        MatchType m_matchType;
        AttributeCaseType m_attributeCaseType;
        MatchPattern m_matchPattern;
        GlobalString m_name;
        HeapString m_value;
        CssSelectorList m_subSelectors;
    };

    class CssComplexSelector {
    public:
        enum class Combinator {
            None,
            Descendant,
            Child,
            DirectAdjacent,
            InDirectAdjacent
        };

        CssComplexSelector(Combinator combinator,
                           CssCompoundSelector compoundSelector)
            : m_combinator(combinator),
              m_compoundSelector(std::move(compoundSelector)) {}

        Combinator combinator() const { return m_combinator; }
        const CssCompoundSelector& compoundSelector() const {
            return m_compoundSelector;
        }

    private:
        Combinator m_combinator;
        CssCompoundSelector m_compoundSelector;
    };

    enum class CssRuleType : uint8_t {
        Style,
        Media,
        Import,
        Namespace,
        FontFace,
        CounterStyle,
        Page,
        PageMargin
    };

    class CssRule : public RefCounted<CssRule> {
    public:
        using ClassRoot = CssRule;
        using ClassKind = CssRuleType;

        virtual ~CssRule() = default;
        ClassKind type() const noexcept { return m_type; }

    protected:
        explicit CssRule(ClassKind type) noexcept : m_type(type) {}
        ClassKind m_type;
    };

    using CssRuleList = std::vector<RefPtr<CssRule>>;

    class CssStyleRule final : public CssRule {
    public:
        static constexpr ClassKind classKind = ClassKind::Style;

        static RefPtr<CssStyleRule> create(CssSelectorList selectors,
                                           CssPropertyList properties);

        const CssSelectorList& selectors() const { return m_selectors; }
        const CssPropertyList& properties() const { return m_properties; }

    private:
        CssStyleRule(CssSelectorList selectors, CssPropertyList properties)
            : CssRule(classKind), m_selectors(std::move(selectors)),
              m_properties(std::move(properties)) {}

        CssSelectorList m_selectors;
        CssPropertyList m_properties;
    };

    inline RefPtr<CssStyleRule>
    CssStyleRule::create(CssSelectorList selectors,
                         CssPropertyList properties) {
        return adoptPtr(
            new CssStyleRule(std::move(selectors), std::move(properties)));
    }

    class CssMediaFeature {
    public:
        CssMediaFeature(CssPropertyID id, RefPtr<CssValue> value)
            : m_id(id), m_value(std::move(value)) {}

        CssPropertyID id() const { return m_id; }
        const RefPtr<CssValue>& value() const { return m_value; }

    private:
        CssPropertyID m_id;
        RefPtr<CssValue> m_value;
    };

    using CssMediaFeatureList = std::forward_list<CssMediaFeature>;

    class CssMediaQuery {
    public:
        enum class Type { None, All, Print, Screen };

        enum class Restrictor { None, Only, Not };

        CssMediaQuery(Type type, Restrictor restrictor,
                      CssMediaFeatureList features)
            : m_type(type), m_restrictor(restrictor),
              m_features(std::move(features)) {}

        Type type() const { return m_type; }
        Restrictor restrictor() const { return m_restrictor; }
        const CssMediaFeatureList& features() const { return m_features; }

    private:
        Type m_type;
        Restrictor m_restrictor;
        CssMediaFeatureList m_features;
    };

    using CssMediaQueryList = std::forward_list<CssMediaQuery>;

    class CssMediaRule final : public CssRule {
    public:
        static constexpr ClassKind classKind = ClassKind::Media;

        static RefPtr<CssMediaRule> create(CssMediaQueryList queries,
                                           CssRuleList rules);

        const CssMediaQueryList& queries() const { return m_queries; }
        const CssRuleList& rules() const { return m_rules; }

    private:
        CssMediaRule(CssMediaQueryList queries, CssRuleList rules)
            : CssRule(classKind), m_queries(std::move(queries)),
              m_rules(std::move(rules)) {}

        CssMediaQueryList m_queries;
        CssRuleList m_rules;
    };

    inline RefPtr<CssMediaRule> CssMediaRule::create(CssMediaQueryList queries,
                                                     CssRuleList rules) {
        return adoptPtr(new CssMediaRule(std::move(queries), std::move(rules)));
    }

    class CssImportRule final : public CssRule {
    public:
        static constexpr ClassKind classKind = ClassKind::Import;

        static RefPtr<CssImportRule> create(CssStyleOrigin origin, Url href,
                                            CssMediaQueryList queries);

        CssStyleOrigin origin() const { return m_origin; }
        const Url& href() const { return m_href; }
        const CssMediaQueryList& queries() const { return m_queries; }

    private:
        CssImportRule(CssStyleOrigin origin, Url href,
                      CssMediaQueryList queries)
            : CssRule(classKind), m_origin(origin), m_href(std::move(href)),
              m_queries(std::move(queries)) {}

        CssStyleOrigin m_origin;
        Url m_href;
        CssMediaQueryList m_queries;
    };

    inline RefPtr<CssImportRule>
    CssImportRule::create(CssStyleOrigin origin, Url href,
                          CssMediaQueryList queries) {
        return adoptPtr(
            new CssImportRule(origin, std::move(href), std::move(queries)));
    }

    class CssNamespaceRule final : public CssRule {
    public:
        static constexpr ClassKind classKind = ClassKind::Namespace;

        static RefPtr<CssNamespaceRule> create(GlobalString prefix,
                                               GlobalString uri);

        GlobalString prefix() const { return m_prefix; }
        GlobalString uri() const { return m_uri; }

    private:
        CssNamespaceRule(GlobalString prefix, GlobalString uri)
            : CssRule(classKind), m_prefix(prefix), m_uri(uri) {}

        GlobalString m_prefix;
        GlobalString m_uri;
    };

    inline RefPtr<CssNamespaceRule>
    CssNamespaceRule::create(GlobalString prefix, GlobalString uri) {
        return adoptPtr(new CssNamespaceRule(prefix, uri));
    }

    class CssFontFaceRule final : public CssRule {
    public:
        static constexpr ClassKind classKind = ClassKind::FontFace;

        static RefPtr<CssFontFaceRule> create(CssPropertyList properties);

        const CssPropertyList& properties() const { return m_properties; }

    private:
        CssFontFaceRule(CssPropertyList properties)
            : CssRule(classKind), m_properties(std::move(properties)) {}

        CssPropertyList m_properties;
    };

    inline RefPtr<CssFontFaceRule>
    CssFontFaceRule::create(CssPropertyList properties) {
        return adoptPtr(new CssFontFaceRule(std::move(properties)));
    }

    class CssCounterStyleRule final : public CssRule {
    public:
        static constexpr ClassKind classKind = ClassKind::CounterStyle;

        static RefPtr<CssCounterStyleRule> create(GlobalString name,
                                                  CssPropertyList properties);

        GlobalString name() const { return m_name; }
        const CssPropertyList& properties() const { return m_properties; }

    private:
        CssCounterStyleRule(GlobalString name, CssPropertyList properties)
            : CssRule(classKind), m_name(name),
              m_properties(std::move(properties)) {}

        GlobalString m_name;
        CssPropertyList m_properties;
    };

    inline RefPtr<CssCounterStyleRule>
    CssCounterStyleRule::create(GlobalString name, CssPropertyList properties) {
        return adoptPtr(new CssCounterStyleRule(name, std::move(properties)));
    }

    enum class PageMarginType : uint8_t {
        TopLeftCorner,
        TopLeft,
        TopCenter,
        TopRight,
        TopRightCorner,
        RightTop,
        RightMiddle,
        RightBottom,
        BottomRightCorner,
        BottomRight,
        BottomCenter,
        BottomLeft,
        BottomLeftCorner,
        LeftBottom,
        LeftMiddle,
        LeftTop,
        None
    };

    class CssPageMarginRule final : public CssRule {
    public:
        static constexpr ClassKind classKind = ClassKind::PageMargin;

        static RefPtr<CssPageMarginRule> create(PageMarginType marginType,
                                                CssPropertyList properties);

        PageMarginType marginType() const { return m_marginType; }
        const CssPropertyList& properties() const { return m_properties; }

    private:
        CssPageMarginRule(PageMarginType marginType, CssPropertyList properties)
            : CssRule(classKind), m_marginType(marginType),
              m_properties(std::move(properties)) {}

        PageMarginType m_marginType;
        CssPropertyList m_properties;
    };

    inline RefPtr<CssPageMarginRule>
    CssPageMarginRule::create(PageMarginType marginType,
                              CssPropertyList properties) {
        return adoptPtr(
            new CssPageMarginRule(marginType, std::move(properties)));
    }

    using CssPageMarginRuleList = std::vector<RefPtr<CssPageMarginRule>>;

    class CssPageRule final : public CssRule {
    public:
        static constexpr ClassKind classKind = ClassKind::Page;

        static RefPtr<CssPageRule> create(CssPageSelectorList selectors,
                                          CssPageMarginRuleList margins,
                                          CssPropertyList properties);

        const CssPageSelectorList& selectors() const { return m_selectors; }
        const CssPageMarginRuleList& margins() const { return m_margins; }
        const CssPropertyList& properties() const { return m_properties; }

    private:
        CssPageRule(CssPageSelectorList selectors,
                    CssPageMarginRuleList margins, CssPropertyList properties)
            : CssRule(classKind), m_selectors(std::move(selectors)),
              m_margins(std::move(margins)),
              m_properties(std::move(properties)) {}

        CssPageSelectorList m_selectors;
        CssPageMarginRuleList m_margins;
        CssPropertyList m_properties;
    };

    inline RefPtr<CssPageRule>
    CssPageRule::create(CssPageSelectorList selectors,
                        CssPageMarginRuleList margins,
                        CssPropertyList properties) {
        return adoptPtr(new CssPageRule(
            std::move(selectors), std::move(margins), std::move(properties)));
    }

    class Element;

    class CssRuleData {
    public:
        CssRuleData(const RefPtr<CssStyleRule>& rule,
                    const CssSelector& selector, uint32_t specificity,
                    uint32_t position)
            : m_rule(rule), m_selector(&selector), m_specificity(specificity),
              m_position(position) {}

        const RefPtr<CssStyleRule>& rule() const { return m_rule; }
        const CssSelector* selector() const { return m_selector; }
        const CssPropertyList& properties() const {
            return m_rule->properties();
        }
        const uint32_t specificity() const { return m_specificity; }
        const uint32_t position() const { return m_position; }

        bool match(const Element* element, PseudoType pseudoType) const;

    private:
        static bool matchSelector(const Element* element, PseudoType pseudoType,
                                  const CssSelector& selector);
        static bool matchCompoundSelector(const Element* element,
                                          PseudoType pseudoType,
                                          const CssCompoundSelector& selector);
        static bool matchSimpleSelector(const Element* element,
                                        const CssSimpleSelector& selector);

        static bool matchNamespaceSelector(const Element* element,
                                           const CssSimpleSelector& selector);
        static bool matchTagSelector(const Element* element,
                                     const CssSimpleSelector& selector);
        static bool matchIdSelector(const Element* element,
                                    const CssSimpleSelector& selector);
        static bool matchClassSelector(const Element* element,
                                       const CssSimpleSelector& selector);

        static bool
        matchAttributeHasSelector(const Element* element,
                                  const CssSimpleSelector& selector);
        static bool
        matchAttributeEqualsSelector(const Element* element,
                                     const CssSimpleSelector& selector);
        static bool
        matchAttributeIncludesSelector(const Element* element,
                                       const CssSimpleSelector& selector);
        static bool
        matchAttributeContainsSelector(const Element* element,
                                       const CssSimpleSelector& selector);
        static bool
        matchAttributeDashEqualsSelector(const Element* element,
                                         const CssSimpleSelector& selector);
        static bool
        matchAttributeStartsWithSelector(const Element* element,
                                         const CssSimpleSelector& selector);
        static bool
        matchAttributeEndsWithSelector(const Element* element,
                                       const CssSimpleSelector& selector);

        static bool
        matchPseudoClassIsSelector(const Element* element,
                                   const CssSimpleSelector& selector);
        static bool
        matchPseudoClassNotSelector(const Element* element,
                                    const CssSimpleSelector& selector);
        static bool
        matchPseudoClassHasSelector(const Element* element,
                                    const CssSimpleSelector& selector);

        static bool
        matchPseudoClassLinkSelector(const Element* element,
                                     const CssSimpleSelector& selector);
        static bool
        matchPseudoClassLocalLinkSelector(const Element* element,
                                          const CssSimpleSelector& selector);
        static bool
        matchPseudoClassEnabledSelector(const Element* element,
                                        const CssSimpleSelector& selector);
        static bool
        matchPseudoClassDisabledSelector(const Element* element,
                                         const CssSimpleSelector& selector);
        static bool
        matchPseudoClassCheckedSelector(const Element* element,
                                        const CssSimpleSelector& selector);
        static bool
        matchPseudoClassLangSelector(const Element* element,
                                     const CssSimpleSelector& selector);

        static bool
        matchPseudoClassRootSelector(const Element* element,
                                     const CssSimpleSelector& selector);
        static bool
        matchPseudoClassEmptySelector(const Element* element,
                                      const CssSimpleSelector& selector);

        static bool
        matchPseudoClassFirstChildSelector(const Element* element,
                                           const CssSimpleSelector& selector);
        static bool
        matchPseudoClassLastChildSelector(const Element* element,
                                          const CssSimpleSelector& selector);
        static bool
        matchPseudoClassOnlyChildSelector(const Element* element,
                                          const CssSimpleSelector& selector);

        static bool
        matchPseudoClassFirstOfTypeSelector(const Element* element,
                                            const CssSimpleSelector& selector);
        static bool
        matchPseudoClassLastOfTypeSelector(const Element* element,
                                           const CssSimpleSelector& selector);
        static bool
        matchPseudoClassOnlyOfTypeSelector(const Element* element,
                                           const CssSimpleSelector& selector);

        static bool
        matchPseudoClassNthChildSelector(const Element* element,
                                         const CssSimpleSelector& selector);
        static bool
        matchPseudoClassNthLastChildSelector(const Element* element,
                                             const CssSimpleSelector& selector);

        static bool
        matchPseudoClassNthOfTypeSelector(const Element* element,
                                          const CssSimpleSelector& selector);
        static bool matchPseudoClassNthLastOfTypeSelector(
            const Element* element, const CssSimpleSelector& selector);

        RefPtr<CssStyleRule> m_rule;
        const CssSelector* m_selector;
        uint32_t m_specificity;
        uint32_t m_position;
    };

    class CssPageRuleData {
    public:
        CssPageRuleData(const RefPtr<CssPageRule>& rule,
                        const CssPageSelector* selector, uint32_t specificity,
                        uint32_t position)
            : m_rule(rule), m_selector(selector), m_specificity(specificity),
              m_position(position) {}

        const RefPtr<CssPageRule>& rule() const { return m_rule; }
        const CssPageSelector* selector() const { return m_selector; }
        const CssPropertyList& properties() const {
            return m_rule->properties();
        }
        const CssPageMarginRuleList& margins() const {
            return m_rule->margins();
        }
        const uint32_t specificity() const { return m_specificity; }
        const uint32_t position() const { return m_position; }

        bool match(GlobalString pageName, uint32_t pageIndex,
                   PseudoType pseudoType) const;

    private:
        static bool matchSelector(GlobalString pageName, uint32_t pageIndex,
                                  PseudoType pseudoType,
                                  const CssSimpleSelector& selector);
        RefPtr<CssPageRule> m_rule;
        const CssPageSelector* m_selector;
        uint32_t m_specificity;
        uint32_t m_position;
    };

    class CssCounterStyle : public RefCounted<CssCounterStyle> {
    public:
        static RefPtr<CssCounterStyle> create(RefPtr<CssCounterStyleRule> rule);

        std::string generateInitialRepresentation(int value) const;
        std::string generateFallbackRepresentation(int value) const;
        std::string generateRepresentation(int value) const;

        bool rangeContains(int value) const;
        bool needsNegativeSign(int value) const;

        GlobalString name() const;
        GlobalString extendsName() const;
        GlobalString fallbackName() const;
        const CssValueID system() const;

        const HeapString& prefix() const;
        const HeapString& suffix() const;

        void setFallbackStyle(CssCounterStyle& fallbackStyle) {
            m_fallbackStyle = fallbackStyle;
        }
        const RefPtr<CssCounterStyle>& fallbackStyle() const {
            return m_fallbackStyle;
        }
        void extend(const CssCounterStyle& extended);

        static CssCounterStyle& defaultStyle();

    private:
        explicit CssCounterStyle(RefPtr<CssCounterStyleRule> rule);
        RefPtr<CssCounterStyleRule> m_rule;
        RefPtr<CssIdentValue> m_system;
        RefPtr<CssCustomIdentValue> m_extends;
        RefPtr<CssIntegerValue> m_fixed;
        RefPtr<CssValue> m_negative;
        RefPtr<CssValue> m_prefix;
        RefPtr<CssValue> m_suffix;
        RefPtr<CssListValue> m_range;
        RefPtr<CssPairValue> m_pad;
        RefPtr<CssCustomIdentValue> m_fallback;
        RefPtr<CssListValue> m_symbols;
        RefPtr<CssListValue> m_additiveSymbols;
        mutable RefPtr<CssCounterStyle> m_fallbackStyle;
    };

    class CssCounterStyleMap {
    public:
        static std::unique_ptr<CssCounterStyleMap>
        create(const CssRuleList& rules, const CssCounterStyleMap* parent);

        CssCounterStyle* findCounterStyle(GlobalString name) const;

    private:
        CssCounterStyleMap(const CssRuleList& rules,
                           const CssCounterStyleMap* parent);
        const CssCounterStyleMap* m_parent;
        boost::unordered_flat_map<GlobalString, RefPtr<CssCounterStyle>>
            m_counterStyles;
    };

    const CssCounterStyleMap* userAgentCounterStyleMap();

    class AttributeStyle {
        CssParserContext m_context;
        CssPropertyList m_properties;
        CssTokenizer m_tokenizer;

    public:
        explicit AttributeStyle(const Node* node);

        void addProperty(CssPropertyID id, CssValueID value) {
            m_properties.emplace_back(id, m_context.origin(), false,
                                      CssIdentValue::create(value));
        }

        void addProperty(CssPropertyID id, RefPtr<CssValue> value) {
            m_properties.emplace_back(id, m_context.origin(), false,
                                      std::move(value));
        }

        bool addProperty(CssPropertyID id, std::string_view value);

        const CssPropertyList& properties() const { return m_properties; }
    };
} // namespace plutobook