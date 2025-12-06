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

    class CssValue {
    public:
        using ClassRoot = CssValue;
        using ClassKind = CssValueType;

        ClassKind type() const noexcept {
            return ClassKind((m_data >> 1) & 31u);
        }
        bool hasID(CssValueID id) const;

    protected:
        explicit CssValue(ClassKind type) noexcept
            : m_data((std::to_underlying(type) << 1) | 1u) {}

        template<class T>
        void initValue(T value) noexcept {
            m_data |= uint64_t(std::bit_cast<UintOf<T>>(value)) << 32;
        }

        template<class T>
        void initTag(T value) noexcept {
            m_data |= uint32_t(std::bit_cast<UintOf<T>>(value)) << 6;
        }

        template<class T>
        T valueAs() const noexcept {
            return std::bit_cast<T>(UintOf<T>(m_data >> 32));
        }

        template<class T>
        T tagAs() const noexcept {
            return std::bit_cast<T>(UintOf<T>((m_data & 0xFFFFFFFFu) >> 6));
        }

    private:
        uint64_t m_data;
    };

    class CssSmallValue : public CssValue {
    public:
        using CssValue::CssValue;
    };

    class CssHeapValue : public CssValue, public RefCounted<CssHeapValue> {
    public:
        using CssValue::CssValue;

        virtual ~CssHeapValue() = default;
    };

    class CssValuePtr {
    public:
        CssValuePtr() = default;
        CssValuePtr(std::nullptr_t) noexcept {}

        CssValuePtr(const CssValuePtr& other) noexcept : m_data(other.m_data) {
            if (m_data && isHeap())
                asHeap().ref();
        }

        CssValuePtr(CssValuePtr&& other) noexcept : m_data(other.m_data) {
            other.m_data = 0;
        }

        explicit CssValuePtr(const CssSmallValue& value) noexcept {
            std::memcpy(&m_data, &value, sizeof(CssSmallValue));
            assert(isSmall());
        }

        explicit CssValuePtr(CssHeapValue* ptr) noexcept
            : m_data(reinterpret_cast<uintptr_t>(ptr)) {
            assert(isHeap());
        }

        ~CssValuePtr() {
            if (m_data && isHeap())
                asHeap().deref();
        }

        CssValuePtr& operator=(CssValuePtr other) noexcept {
            this->~CssValuePtr();
            return *new (this) CssValuePtr(std::move(other));
        }

        explicit operator bool() const noexcept { return m_data != 0; }

        const CssValue& operator*() const noexcept {
            if (isHeap())
                return asHeap();
            return asSmall();
        }

        const CssValue* operator->() const noexcept { return &operator*(); }

        bool isSmall() const noexcept { return bool(m_data & 1u); }

        bool isHeap() const noexcept { return !(m_data & 1u); }

        const CssSmallValue& asSmall() const noexcept {
            return *reinterpret_cast<const CssSmallValue*>(&m_data);
        }

        CssHeapValue& asHeap() const noexcept {
            return *reinterpret_cast<CssHeapValue*>(m_data);
        }

        bool operator==(const CssValuePtr&) const = default;

    private:
        uint64_t m_data = 0;
    };

    template<class T>
    class ValPtr : public CssValuePtr {
    public:
        ValPtr() = default;
        ValPtr(std::nullptr_t) noexcept {}

        explicit ValPtr(const T& value) noexcept
            requires std::derived_from<T, CssSmallValue>
            : CssValuePtr(value) {
            static_assert(sizeof(T) == sizeof(CssSmallValue));
        }

        explicit ValPtr(T* ptr) noexcept
            requires std::derived_from<T, CssHeapValue>
            : CssValuePtr(ptr) {}

        const T& operator*() const noexcept {
            if constexpr (std::derived_from<T, CssHeapValue>) {
                return static_cast<const T&>(asHeap());
            } else {
                static_assert(std::derived_from<T, CssSmallValue>);
                return static_cast<const T&>(asSmall());
            }
        }

        const T* operator->() const noexcept { return &operator*(); }
    };

    template<class T>
    inline bool is(const CssValuePtr& value) {
        if (!value)
            return false;
        if constexpr (std::derived_from<T, CssHeapValue>) {
            return value.isHeap() && is<T>(value.asHeap());
        } else {
            static_assert(std::derived_from<T, CssSmallValue>);
            return value.isSmall() && is<T>(value.asSmall());
        }
    }

    template<class T>
    inline ValPtr<T> to(const CssValuePtr& value) {
        if (!is<T>(value))
            return nullptr;
        if constexpr (std::derived_from<T, CssHeapValue>) {
            auto& heap = value.asHeap();
            heap.ref();
            return ValPtr<T>(static_cast<T*>(&heap));
        } else {
            static_assert(std::derived_from<T, CssSmallValue>);
            return ValPtr<T>(static_cast<const T&>(value.asSmall()));
        }
    }

    using CssValueList = std::vector<CssValuePtr>;

    enum class CssStyleOrigin : uint8_t {
        UserAgent = 0 << 1,
        User = 1 << 1,
        PresentationAttribute = 2 << 1,
        Author = 3 << 1,
        Inline = Author | 1,
    };

    class CssProperty {
    public:
        CssProperty(CssPropertyID id, CssStyleOrigin origin, bool important,
                    CssValuePtr value)
            : m_id(id), m_precedence(calcPrecedence(origin, important)),
              m_value(std::move(value)) {}

        CssPropertyID id() const { return m_id; }
        const CssValuePtr& value() const { return m_value; }

    protected:
        static uint8_t calcPrecedence(CssStyleOrigin origin, bool important) {
            auto precedence = std::to_underlying(origin);
            if (important)
                precedence ^= 0b1110; // Don't flip the `inline` bit.
            return precedence;
        }

        CssPropertyID m_id;
        uint8_t m_precedence;
        CssValuePtr m_value;
    };

    using CssPropertyList = std::vector<CssProperty>;

    class CssInitialValue final : public CssSmallValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Initial;

        static ValPtr<CssInitialValue> create();

    private:
        CssInitialValue() noexcept : CssSmallValue(classKind) {}
    };

    inline ValPtr<CssInitialValue> CssInitialValue::create() {
        return ValPtr(CssInitialValue());
    }

    class CssInheritValue final : public CssSmallValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Inherit;

        static ValPtr<CssInheritValue> create();

    private:
        CssInheritValue() noexcept : CssSmallValue(classKind) {}
    };

    inline ValPtr<CssInheritValue> CssInheritValue::create() {
        return ValPtr(CssInheritValue());
    }

    class CssUnsetValue final : public CssSmallValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Unset;

        static ValPtr<CssUnsetValue> create();

    private:
        CssUnsetValue() noexcept : CssSmallValue(classKind) {}
    };

    inline ValPtr<CssUnsetValue> CssUnsetValue::create() {
        return ValPtr(CssUnsetValue());
    }

    class CssIdentValue final : public CssSmallValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Ident;

        static ValPtr<CssIdentValue> create(CssValueID value);

        CssValueID value() const { return valueAs<CssValueID>(); }

    private:
        CssIdentValue(CssValueID value) : CssSmallValue(classKind) {
            initValue(value);
        }
    };

    inline ValPtr<CssIdentValue> CssIdentValue::create(CssValueID value) {
        return ValPtr(CssIdentValue(value));
    }

    inline bool CssValue::hasID(CssValueID id) const {
        if (is<CssIdentValue>(*this))
            return to<CssIdentValue>(*this).value() == id;
        return false;
    }

    class CssCustomIdentValue final : public CssSmallValue {
    public:
        static constexpr ClassKind classKind = ClassKind::CustomIdent;

        static ValPtr<CssCustomIdentValue> create(GlobalString value);

        GlobalString value() const { return valueAs<GlobalString>(); }

    private:
        CssCustomIdentValue(GlobalString value) : CssSmallValue(classKind) {
            initValue(value);
        }
    };

    inline ValPtr<CssCustomIdentValue>
    CssCustomIdentValue::create(GlobalString value) {
        return ValPtr(CssCustomIdentValue(value));
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
        std::string m_chars;
    };

    class CssCustomPropertyValue final : public CssHeapValue {
    public:
        static constexpr ClassKind classKind = ClassKind::CustomProperty;

        static ValPtr<CssCustomPropertyValue>
        create(GlobalString name, RefPtr<CssVariableData> value);

        GlobalString name() const { return valueAs<GlobalString>(); }
        const RefPtr<CssVariableData>& value() const { return m_value; }

    private:
        CssCustomPropertyValue(GlobalString name,
                               RefPtr<CssVariableData> value);
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

    class CssVariableReferenceValue final : public CssHeapValue {
    public:
        static constexpr ClassKind classKind = ClassKind::VariableReference;

        static ValPtr<CssVariableReferenceValue>
        create(const CssParserContext& context, CssPropertyID id,
               bool important, RefPtr<CssVariableData> value);

        const CssParserContext& context() const { return m_context; }
        CssPropertyID id() const { return valueAs<CssPropertyID>(); }
        bool important() const { return tagAs<bool>(); }
        const RefPtr<CssVariableData>& value() const { return m_value; }

        CssPropertyList resolve(const BoxStyle* style) const;

    private:
        CssVariableReferenceValue(const CssParserContext& context,
                                  CssPropertyID id, bool important,
                                  RefPtr<CssVariableData> value);
        CssParserContext m_context;
        RefPtr<CssVariableData> m_value;
    };

    class CssIntegerValue final : public CssSmallValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Integer;

        static ValPtr<CssIntegerValue> create(int value);

        int value() const { return valueAs<int>(); }

    private:
        CssIntegerValue(int value) : CssSmallValue(classKind) {
            initValue(value);
        }
    };

    inline ValPtr<CssIntegerValue> CssIntegerValue::create(int value) {
        return ValPtr(CssIntegerValue(value));
    }

    class CssNumberValue final : public CssSmallValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Number;

        static ValPtr<CssNumberValue> create(float value);

        float value() const { return valueAs<float>(); }

    private:
        CssNumberValue(float value) : CssSmallValue(classKind) {
            initValue(value);
        }
    };

    inline ValPtr<CssNumberValue> CssNumberValue::create(float value) {
        return ValPtr(CssNumberValue(value));
    }

    class CssPercentValue final : public CssSmallValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Percent;

        static ValPtr<CssPercentValue> create(float value);

        float value() const { return valueAs<float>(); }

    private:
        CssPercentValue(float value) : CssSmallValue(classKind) {
            initValue(value);
        }
    };

    inline ValPtr<CssPercentValue> CssPercentValue::create(float value) {
        return ValPtr(CssPercentValue(value));
    }

    class CssAngleValue final : public CssSmallValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Angle;

        enum class Unit { Degrees, Radians, Gradians, Turns };

        static ValPtr<CssAngleValue> create(float value, Unit unit);

        float value() const { return valueAs<float>(); }
        Unit unit() const { return tagAs<Unit>(); }

        float valueInDegrees() const;

    private:
        CssAngleValue(float value, Unit unit) : CssSmallValue(classKind) {
            initValue(value);
            initTag(unit);
        }
    };

    inline ValPtr<CssAngleValue> CssAngleValue::create(float value, Unit unit) {
        return ValPtr(CssAngleValue(value, unit));
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

    class CssLengthValue final : public CssSmallValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Length;

        static ValPtr<CssLengthValue>
        create(float value, CssLengthUnits units = CssLengthUnits::Pixels);

        float value() const { return valueAs<float>(); }
        CssLengthUnits units() const { return tagAs<CssLengthUnits>(); }

    private:
        CssLengthValue(float value, CssLengthUnits units)
            : CssSmallValue(classKind) {
            initValue(value);
            initTag(units);
        }
    };

    inline ValPtr<CssLengthValue> CssLengthValue::create(float value,
                                                         CssLengthUnits units) {
        return ValPtr(CssLengthValue(value, units));
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

    class CssCalcValue final : public CssHeapValue {
        enum : uint8_t { NegativeBit = 1, UnitlessBit = 2 };

    public:
        static constexpr ClassKind classKind = ClassKind::Calc;

        static ValPtr<CssCalcValue> create(bool negative, bool unitless,
                                           CssCalcList values);

        const bool negative() const { return valueAs<uint8_t>() & NegativeBit; }
        const bool unitless() const { return valueAs<uint8_t>() & UnitlessBit; }
        const CssCalcList& values() const { return m_values; }

        float resolve(const CssLengthResolver& resolver) const;

    private:
        CssCalcValue(bool negative, bool unitless, CssCalcList values)
            : CssHeapValue(classKind), m_values(std::move(values)) {
            uint8_t bits = 0;
            if (negative)
                bits |= NegativeBit;
            if (unitless)
                bits |= UnitlessBit;
            initValue(bits);
        }

        CssCalcList m_values;
    };

    inline ValPtr<CssCalcValue>
    CssCalcValue::create(bool negative, bool unitless, CssCalcList values) {
        return ValPtr(new CssCalcValue(negative, unitless, std::move(values)));
    }

    class CssAttrValue final : public CssHeapValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Attr;

        static ValPtr<CssAttrValue> create(GlobalString name,
                                           const HeapString& fallback);

        GlobalString name() const { return valueAs<GlobalString>(); }
        const HeapString& fallback() const { return m_fallback; }

    private:
        CssAttrValue(GlobalString name, const HeapString& fallback)
            : CssHeapValue(classKind), m_fallback(fallback) {
            initValue(name);
        }

        HeapString m_fallback;
    };

    inline ValPtr<CssAttrValue>
    CssAttrValue::create(GlobalString name, const HeapString& fallback) {
        return ValPtr(new CssAttrValue(name, fallback));
    }

    class CssStringValue final : public CssHeapValue {
    public:
        static constexpr ClassKind classKind = ClassKind::String;

        static ValPtr<CssStringValue> create(const HeapString& value);

        const HeapString& value() const { return m_value; }

    private:
        CssStringValue(const HeapString& value)
            : CssHeapValue(classKind), m_value(value) {}
        HeapString m_value;
    };

    inline ValPtr<CssStringValue>
    CssStringValue::create(const HeapString& value) {
        return ValPtr(new CssStringValue(value));
    }

    class CssLocalUrlValue final : public CssHeapValue {
    public:
        static constexpr ClassKind classKind = ClassKind::LocalUrl;

        static ValPtr<CssLocalUrlValue> create(const HeapString& value);

        const HeapString& value() const { return m_value; }

    private:
        CssLocalUrlValue(const HeapString& value)
            : CssHeapValue(classKind), m_value(value) {}
        HeapString m_value;
    };

    inline ValPtr<CssLocalUrlValue>
    CssLocalUrlValue::create(const HeapString& value) {
        return ValPtr(new CssLocalUrlValue(value));
    }

    class CssUrlValue final : public CssHeapValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Url;

        static ValPtr<CssUrlValue> create(Url value);

        const Url& value() const { return m_value; }

    private:
        CssUrlValue(Url value)
            : CssHeapValue(classKind), m_value(std::move(value)) {}
        Url m_value;
    };

    inline ValPtr<CssUrlValue> CssUrlValue::create(Url value) {
        return ValPtr(new CssUrlValue(std::move(value)));
    }

    class Image;

    class CssImageValue final : public CssHeapValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Image;

        static ValPtr<CssImageValue> create(Url value);

        const Url& value() const { return m_value; }
        const RefPtr<Image>& image() const { return m_image; }
        const RefPtr<Image>& fetch(Document* document) const;

    private:
        CssImageValue(Url value);
        Url m_value;
        mutable RefPtr<Image> m_image;
    };

    class CssColorValue final : public CssSmallValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Color;

        static ValPtr<CssColorValue> create(const Color& value);

        const Color& value() const { return valueAs<Color>(); }

    private:
        CssColorValue(Color value) : CssSmallValue(classKind) {
            initValue(value);
        }
    };

    inline ValPtr<CssColorValue> CssColorValue::create(const Color& value) {
        return ValPtr(CssColorValue(value));
    }

    class CssCounterValue final : public CssHeapValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Counter;

        static ValPtr<CssCounterValue> create(GlobalString identifier,
                                              GlobalString listStyle,
                                              const HeapString& separator);

        GlobalString identifier() const { return m_identifier; }
        GlobalString listStyle() const { return m_listStyle; }
        const HeapString& separator() const { return m_separator; }

    private:
        CssCounterValue(GlobalString identifier, GlobalString listStyle,
                        const HeapString& separator)
            : CssHeapValue(classKind), m_identifier(identifier),
              m_listStyle(listStyle), m_separator(separator) {}

        GlobalString m_identifier;
        GlobalString m_listStyle;
        HeapString m_separator;
    };

    inline ValPtr<CssCounterValue>
    CssCounterValue::create(GlobalString identifier, GlobalString listStyle,
                            const HeapString& separator) {
        return ValPtr(new CssCounterValue(identifier, listStyle, separator));
    }

    class CssFontFeatureValue final : public CssSmallValue {
    public:
        static constexpr ClassKind classKind = ClassKind::FontFeature;

        static ValPtr<CssFontFeatureValue> create(GlobalString tag, int value);

        GlobalString tag() const { return tagAs<GlobalString>(); }
        int value() const { return valueAs<int>(); }

    private:
        CssFontFeatureValue(GlobalString tag, int value)
            : CssSmallValue(classKind) {
            initValue(value);
            initTag(tag);
        }
    };

    inline ValPtr<CssFontFeatureValue>
    CssFontFeatureValue::create(GlobalString tag, int value) {
        return ValPtr(CssFontFeatureValue(tag, value));
    }

    class CssFontVariationValue final : public CssSmallValue {
    public:
        static constexpr ClassKind classKind = ClassKind::FontVariation;

        static ValPtr<CssFontVariationValue> create(GlobalString tag,
                                                    float value);

        GlobalString tag() const { return tagAs<GlobalString>(); }
        float value() const { return valueAs<float>(); }

    private:
        CssFontVariationValue(GlobalString tag, float value)
            : CssSmallValue(classKind) {
            initValue(value);
            initTag(tag);
        }
    };

    inline ValPtr<CssFontVariationValue>
    CssFontVariationValue::create(GlobalString tag, float value) {
        return ValPtr(CssFontVariationValue(tag, value));
    }

    class CssUnicodeRangeValue final : public CssHeapValue {
    public:
        static constexpr ClassKind classKind = ClassKind::UnicodeRange;

        static ValPtr<CssUnicodeRangeValue> create(uint32_t from, uint32_t to);

        uint32_t from() const { return m_from; }
        uint32_t to() const { return m_to; }

    private:
        CssUnicodeRangeValue(uint32_t from, uint32_t to)
            : CssHeapValue(classKind), m_from(from), m_to(to) {}

        uint32_t m_from;
        uint32_t m_to;
    };

    inline ValPtr<CssUnicodeRangeValue>
    CssUnicodeRangeValue::create(uint32_t from, uint32_t to) {
        return ValPtr(new CssUnicodeRangeValue(from, to));
    }

    class CssPairValue final : public CssHeapValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Pair;

        static ValPtr<CssPairValue> create(CssValuePtr first,
                                           CssValuePtr second);

        const CssValuePtr& first() const { return m_first; }
        const CssValuePtr& second() const { return m_second; }

    private:
        CssPairValue(CssValuePtr first, CssValuePtr second)
            : CssHeapValue(classKind), m_first(std::move(first)),
              m_second(std::move(second)) {}

        CssValuePtr m_first;
        CssValuePtr m_second;
    };

    inline ValPtr<CssPairValue> CssPairValue::create(CssValuePtr first,
                                                     CssValuePtr second) {
        return ValPtr(new CssPairValue(std::move(first), std::move(second)));
    }

    class CssRectValue final : public CssHeapValue {
    public:
        static constexpr ClassKind classKind = ClassKind::Rect;

        static ValPtr<CssRectValue> create(CssValuePtr top, CssValuePtr right,
                                           CssValuePtr bottom,
                                           CssValuePtr left);

        const CssValuePtr& position(Edge edge) const {
            return m_position[edge];
        }

    private:
        CssRectValue(CssValuePtr top, CssValuePtr right, CssValuePtr bottom,
                     CssValuePtr left)
            : CssHeapValue(classKind), m_position{top, right, bottom, left} {}

        CssValuePtr m_position[4];
    };

    inline ValPtr<CssRectValue> CssRectValue::create(CssValuePtr top,
                                                     CssValuePtr right,
                                                     CssValuePtr bottom,
                                                     CssValuePtr left) {
        return ValPtr(new CssRectValue(std::move(top), std::move(right),
                                       std::move(bottom), std::move(left)));
    }

    class CssAbstractListValue : public CssHeapValue {
    public:
        using Iterator = CssValueList::const_iterator;
        Iterator begin() const { return m_values.begin(); }
        Iterator end() const { return m_values.end(); }

        const CssValuePtr& front() const { return m_values.front(); }
        const CssValuePtr& back() const { return m_values.back(); }
        const CssValuePtr& operator[](size_t index) const {
            return m_values[index];
        }
        const CssValueList& values() const { return m_values; }
        size_t size() const { return m_values.size(); }
        bool empty() const { return m_values.empty(); }

    protected:
        CssAbstractListValue(ClassKind type, CssValueList&& values)
            : CssHeapValue(type), m_values(std::move(values)) {}
        CssValueList m_values;
    };

    class CssListValue final : public CssAbstractListValue {
    public:
        static constexpr ClassKind classKind = ClassKind::List;

        static ValPtr<CssListValue> create(CssValueList values);

    protected:
        CssListValue(CssValueList values)
            : CssAbstractListValue(classKind, std::move(values)) {}
        CssValueList m_values;
    };

    inline ValPtr<CssListValue> CssListValue::create(CssValueList values) {
        return ValPtr(new CssListValue(std::move(values)));
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

        static ValPtr<CssFunctionValue> create(CssFunctionID id,
                                               CssValueList values);

        CssFunctionID id() const { return m_id; }

    private:
        CssFunctionValue(CssFunctionID id, CssValueList values)
            : CssAbstractListValue(classKind, std::move(values)), m_id(id) {}

        CssFunctionID m_id;
    };

    inline ValPtr<CssFunctionValue>
    CssFunctionValue::create(CssFunctionID id, CssValueList values) {
        return ValPtr(new CssFunctionValue(id, std::move(values)));
    }

    class CssUnaryFunctionValue final : public CssHeapValue {
    public:
        static constexpr ClassKind classKind = ClassKind::UnaryFunction;

        static ValPtr<CssUnaryFunctionValue> create(CssFunctionID id,
                                                    CssValuePtr value);

        CssFunctionID id() const { return valueAs<CssFunctionID>(); }
        const CssValuePtr& value() const { return m_value; }

    private:
        CssUnaryFunctionValue(CssFunctionID id, CssValuePtr value)
            : CssHeapValue(classKind), m_value(std::move(value)) {
            initValue(id);
        }

        CssValuePtr m_value;
    };

    inline ValPtr<CssUnaryFunctionValue>
    CssUnaryFunctionValue::create(CssFunctionID id, CssValuePtr value) {
        return ValPtr(new CssUnaryFunctionValue(id, std::move(value)));
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
        CssMediaFeature(CssPropertyID id, CssValuePtr value)
            : m_id(id), m_value(std::move(value)) {}

        CssPropertyID id() const { return m_id; }
        const CssValuePtr& value() const { return m_value; }

    private:
        CssPropertyID m_id;
        CssValuePtr m_value;
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
        ValPtr<CssIdentValue> m_system;
        ValPtr<CssCustomIdentValue> m_extends;
        ValPtr<CssIntegerValue> m_fixed;
        CssValuePtr m_negative;
        CssValuePtr m_prefix;
        CssValuePtr m_suffix;
        ValPtr<CssListValue> m_range;
        ValPtr<CssPairValue> m_pad;
        ValPtr<CssCustomIdentValue> m_fallback;
        ValPtr<CssListValue> m_symbols;
        ValPtr<CssListValue> m_additiveSymbols;
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

        void addProperty(CssPropertyID id, CssValuePtr value) {
            m_properties.emplace_back(id, m_context.origin(), false,
                                      std::move(value));
        }

        bool addProperty(CssPropertyID id, std::string_view value);

        const CssPropertyList& properties() const { return m_properties; }
    };
} // namespace plutobook