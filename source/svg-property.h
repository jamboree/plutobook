#pragma once

#include "geometry.h"

#include <string>

namespace plutobook {
    struct SvgPropertyPtr {
        SvgPropertyPtr() = default;
        SvgPropertyPtr(std::nullptr_t) noexcept {}

        template<class T>
        SvgPropertyPtr(T* ptr) noexcept
            : m_ptr(ptr), m_parse([](void* ptr, std::string_view input) {
                  return static_cast<T*>(ptr)->parse(input);
              }) {}

        explicit operator bool() const noexcept { return m_ptr != nullptr; }

        bool parse(std::string_view input) { return m_parse(m_ptr, input); }

    private:
        void* m_ptr = nullptr;
        bool (*m_parse)(void* ptr, std::string_view input) = nullptr;
    };

    class SvgString final {
    public:
        SvgString() = default;
        explicit SvgString(const std::string& value) : m_value(value) {}

        const std::string& value() const { return m_value; }
        bool parse(std::string_view input);

    private:
        std::string m_value;
    };

    enum SvgUnitsType {
        SvgUnitsTypeUserSpaceOnUse,
        SvgUnitsTypeObjectBoundingBox
    };

    enum SvgMarkerUnitsType {
        SvgMarkerUnitsTypeUserSpaceOnUse,
        SvgMarkerUnitsTypeStrokeWidth
    };

    enum SvgSpreadMethodType {
        SvgSpreadMethodTypePad,
        SvgSpreadMethodTypeReflect,
        SvgSpreadMethodTypeRepeat
    };

    template<typename Enum>
    class SvgEnumeration final {
    public:
        explicit SvgEnumeration(Enum value) : m_value(value) {}

        Enum value() const { return m_value; }
        bool parse(std::string_view input);

    private:
        Enum m_value;
    };

    class SvgAngle final {
    public:
        enum class OrientType { Auto, AutoStartReverse, Angle };

        SvgAngle() = default;
        SvgAngle(float value, OrientType orientType)
            : m_value(value), m_orientType(orientType) {}

        float value() const { return m_value; }
        OrientType orientType() const { return m_orientType; }
        bool parse(std::string_view input);

    private:
        float m_value = 0.f;
        OrientType m_orientType = OrientType::Angle;
    };

    enum class SvgLengthType : uint8_t {
        Number,
        Percentage,
        Ems,
        Exs,
        Pixels,
        Centimeters,
        Millimeters,
        Inches,
        Points,
        Picas,
        Rems,
        Chs
    };

    enum class SvgLengthDirection : uint8_t { Horizontal, Vertical, Diagonal };

    enum class SvgLengthNegativeValuesMode : uint8_t { Allow, Forbid };

    class SvgLength final {
    public:
        SvgLength(SvgLengthDirection direction,
                  SvgLengthNegativeValuesMode negativeMode)
            : SvgLength(0.f, SvgLengthType::Number, direction, negativeMode) {}

        SvgLength(float value, SvgLengthType type, SvgLengthDirection direction,
                  SvgLengthNegativeValuesMode negativeMode)
            : m_value(value), m_type(type), m_direction(direction),
              m_negativeMode(negativeMode) {}

        float value() const { return m_value; }
        SvgLengthType type() const { return m_type; }
        SvgLengthDirection direction() const { return m_direction; }
        SvgLengthNegativeValuesMode negativeMode() const {
            return m_negativeMode;
        }
        bool parse(std::string_view input);

    private:
        float m_value;
        SvgLengthType m_type;
        const SvgLengthDirection m_direction;
        const SvgLengthNegativeValuesMode m_negativeMode;
    };

    class Length;
    class SvgElement;

    class SvgLengthContext {
    public:
        SvgLengthContext(const SvgElement* element,
                         SvgUnitsType unitType = SvgUnitsTypeUserSpaceOnUse)
            : m_element(element), m_unitType(unitType) {}

        float valueForLength(const SvgLength& length) const;
        float valueForLength(
            const Length& length,
            SvgLengthDirection direction = SvgLengthDirection::Diagonal) const;

    private:
        float viewportDimension(SvgLengthDirection direction) const;
        const SvgElement* m_element;
        const SvgUnitsType m_unitType;
    };

    class SvgLengthList final {
    public:
        SvgLengthList(SvgLengthDirection direction,
                      SvgLengthNegativeValuesMode negativeMode)
            : m_direction(direction), m_negativeMode(negativeMode) {}

        const std::vector<SvgLength>& values() const { return m_values; }
        SvgLengthDirection direction() const { return m_direction; }
        SvgLengthNegativeValuesMode negativeMode() const {
            return m_negativeMode;
        }
        bool parse(std::string_view input);

    private:
        std::vector<SvgLength> m_values;
        const SvgLengthDirection m_direction;
        const SvgLengthNegativeValuesMode m_negativeMode;
    };

    class SvgNumber {
    public:
        SvgNumber() = default;
        explicit SvgNumber(float value) : m_value(value) {}

        float value() const { return m_value; }
        bool parse(std::string_view input);

    protected:
        float m_value = 0.f;
    };

    class SvgNumberPercentage final : public SvgNumber {
    public:
        SvgNumberPercentage() = default;
        explicit SvgNumberPercentage(float value) : SvgNumber(value) {}

        bool parse(std::string_view input);
    };

    class SvgNumberList final {
    public:
        SvgNumberList() = default;

        const std::vector<float>& values() const { return m_values; }
        bool parse(std::string_view input);

    private:
        std::vector<float> m_values;
    };

    class SvgPath final {
    public:
        SvgPath() = default;
        explicit SvgPath(const Path& value) : m_value(value) {}

        const Path& value() const { return m_value; }
        bool parse(std::string_view input);

    private:
        Path m_value;
    };

    class SvgPoint final {
    public:
        SvgPoint() = default;
        explicit SvgPoint(const Point& value) : m_value(value) {}

        const Point& value() const { return m_value; }
        bool parse(std::string_view input);

    private:
        Point m_value;
    };

    class SvgPointList final {
    public:
        SvgPointList() = default;

        const std::vector<Point>& values() const { return m_values; }
        bool parse(std::string_view input);

    private:
        std::vector<Point> m_values;
    };

    class SvgRect final {
    public:
        SvgRect() = default;
        explicit SvgRect(const Rect& value) : m_value(value) {}

        const Rect& value() const { return m_value; }
        bool parse(std::string_view input);

    private:
        Rect m_value = Rect::Invalid;
    };

    class SvgTransform final {
    public:
        SvgTransform() = default;
        explicit SvgTransform(const Transform& value) : m_value(value) {}

        const Transform& value() const { return m_value; }
        bool parse(std::string_view input);

    private:
        Transform m_value;
    };

    class SvgPreserveAspectRatio final {
    public:
        enum class AlignType {
            None,
            xMinYMin,
            xMidYMin,
            xMaxYMin,
            xMinYMid,
            xMidYMid,
            xMaxYMid,
            xMinYMax,
            xMidYMax,
            xMaxYMax
        };

        enum class MeetOrSlice { Meet, Slice };

        SvgPreserveAspectRatio() = default;
        SvgPreserveAspectRatio(AlignType alignType, MeetOrSlice meetOrSlice)
            : m_alignType(alignType), m_meetOrSlice(meetOrSlice) {}

        AlignType alignType() const { return m_alignType; }
        MeetOrSlice meetOrSlice() const { return m_meetOrSlice; }
        bool parse(std::string_view input);

        Rect getClipRect(const Rect& viewBoxRect,
                         const Size& viewportSize) const;
        Transform getTransform(const Rect& viewBoxRect,
                               const Size& viewportSize) const;
        void transformRect(Rect& dstRect, Rect& srcRect) const;

    private:
        AlignType m_alignType = AlignType::xMidYMid;
        MeetOrSlice m_meetOrSlice = MeetOrSlice::Meet;
    };
} // namespace plutobook