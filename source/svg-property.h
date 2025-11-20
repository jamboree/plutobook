#ifndef PLUTOBOOK_SvgPROPERTY_H
#define PLUTOBOOK_SvgPROPERTY_H

#include "geometry.h"

#include <string>

namespace plutobook {

class SvgProperty {
public:
    SvgProperty() = default;
    virtual ~SvgProperty() = default;
    virtual bool parse(std::string_view input) = 0;
};

class SvgString final : public SvgProperty {
public:
    SvgString() = default;
    explicit SvgString(const std::string& value) : m_value(value) {}

    const std::string& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    std::string m_value;
};

using SvgEnumerationEntry = std::pair<int, std::string_view>;
using SvgEnumerationEntries = std::vector<SvgEnumerationEntry>;

class SvgEnumerationBase : public SvgProperty {
public:
    SvgEnumerationBase(int value, const SvgEnumerationEntries& entries)
        : m_value(value), m_entries(entries)
    {}

    int value() const { return m_value; }
    bool parse(std::string_view input) override;

protected:
    int m_value;
    const SvgEnumerationEntries& m_entries;
};

template<typename Enum>
const SvgEnumerationEntries& getEnumerationEntries();

enum SvgUnitsType {
    SvgUnitsTypeUserSpaceOnUse,
    SvgUnitsTypeObjectBoundingBox
};

template<>
const SvgEnumerationEntries& getEnumerationEntries<SvgUnitsType>();

enum SvgMarkerUnitsType {
    SvgMarkerUnitsTypeUserSpaceOnUse,
    SvgMarkerUnitsTypeStrokeWidth
};

template<>
const SvgEnumerationEntries& getEnumerationEntries<SvgMarkerUnitsType>();

enum SvgSpreadMethodType {
    SvgSpreadMethodTypePad,
    SvgSpreadMethodTypeReflect,
    SvgSpreadMethodTypeRepeat
};

template<>
const SvgEnumerationEntries& getEnumerationEntries<SvgSpreadMethodType>();

template<typename Enum>
class SvgEnumeration final : public SvgEnumerationBase {
public:
    explicit SvgEnumeration(Enum value)
        : SvgEnumerationBase(value, getEnumerationEntries<Enum>())
    {}

    Enum value() const { return static_cast<Enum>(m_value); }
};

class SvgAngle final : public SvgProperty {
public:
    enum class OrientType {
        Auto,
        AutoStartReverse,
        Angle
    };

    SvgAngle() = default;
    SvgAngle(float value, OrientType orientType)
        : m_value(value), m_orientType(orientType)
    {}

    float value() const { return m_value; }
    OrientType orientType() const { return m_orientType; }
    bool parse(std::string_view input) final;

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

enum class SvgLengthDirection : uint8_t {
    Horizontal,
    Vertical,
    Diagonal
};

enum class SvgLengthNegativeValuesMode : uint8_t {
    Allow,
    Forbid
};

class SvgLength final : public SvgProperty {
public:
    SvgLength(SvgLengthDirection direction, SvgLengthNegativeValuesMode negativeMode)
        : SvgLength(0.f, SvgLengthType::Number, direction, negativeMode)
    {}

    SvgLength(float value, SvgLengthType type, SvgLengthDirection direction, SvgLengthNegativeValuesMode negativeMode)
        : m_value(value), m_type(type), m_direction(direction), m_negativeMode(negativeMode)
    {}

    float value() const { return m_value; }
    SvgLengthType type() const { return m_type; }
    SvgLengthDirection direction() const { return m_direction; }
    SvgLengthNegativeValuesMode negativeMode() const { return m_negativeMode; }
    bool parse(std::string_view input) final;

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
    SvgLengthContext(const SvgElement* element, SvgUnitsType unitType = SvgUnitsTypeUserSpaceOnUse)
        : m_element(element), m_unitType(unitType)
    {}

    float valueForLength(const SvgLength& length) const;
    float valueForLength(const Length& length, SvgLengthDirection direction = SvgLengthDirection::Diagonal) const;

private:
    float viewportDimension(SvgLengthDirection direction) const;
    const SvgElement* m_element;
    const SvgUnitsType m_unitType;
};

class SvgLengthList final : public SvgProperty {
public:
    SvgLengthList(SvgLengthDirection direction, SvgLengthNegativeValuesMode negativeMode)
        : m_direction(direction), m_negativeMode(negativeMode)
    {}

    const std::vector<SvgLength>& values() const { return m_values; }
    SvgLengthDirection direction() const { return m_direction; }
    SvgLengthNegativeValuesMode negativeMode() const { return m_negativeMode; }
    bool parse(std::string_view input) final;

private:
    std::vector<SvgLength> m_values;
    const SvgLengthDirection m_direction;
    const SvgLengthNegativeValuesMode m_negativeMode;
};

class SvgNumber : public SvgProperty {
public:
    SvgNumber() = default;
    explicit SvgNumber(float value) : m_value(value) {}

    float value() const { return m_value; }
    bool parse(std::string_view input) override;

protected:
    float m_value = 0.f;
};

class SvgNumberPercentage final : public SvgNumber {
public:
    SvgNumberPercentage() = default;
    explicit SvgNumberPercentage(float value) : SvgNumber(value) {}

    bool parse(std::string_view input) final;
};

class SvgNumberList final : public SvgProperty {
public:
    SvgNumberList() = default;

    const std::vector<float>& values() const { return m_values; }
    bool parse(std::string_view input) final;

private:
    std::vector<float> m_values;
};

class SvgPath final : public SvgProperty {
public:
    SvgPath() = default;
    explicit SvgPath(const Path& value) : m_value(value) {}

    const Path& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    Path m_value;
};

class SvgPoint final : public SvgProperty {
public:
    SvgPoint() = default;
    explicit SvgPoint(const Point& value) : m_value(value) {}

    const Point& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    Point m_value;
};

class SvgPointList final : public SvgProperty {
public:
    SvgPointList() = default;

    const std::vector<Point>& values() const { return m_values; }
    bool parse(std::string_view input) final;

private:
    std::vector<Point> m_values;
};

class SvgRect final : public SvgProperty {
public:
    SvgRect() = default;
    explicit SvgRect(const Rect& value) : m_value(value) {}

    const Rect& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    Rect m_value = Rect::Invalid;
};

class SvgTransform final : public SvgProperty {
public:
    SvgTransform() = default;
    explicit SvgTransform(const Transform& value) : m_value(value) {}

    const Transform& value() const { return m_value; }
    bool parse(std::string_view input) final;

private:
    Transform m_value;
};

class SvgPreserveAspectRatio final : public SvgProperty {
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

    enum class MeetOrSlice {
        Meet,
        Slice
    };

    SvgPreserveAspectRatio() = default;
    SvgPreserveAspectRatio(AlignType alignType, MeetOrSlice meetOrSlice)
        : m_alignType(alignType), m_meetOrSlice(meetOrSlice)
    {}

    AlignType alignType() const { return m_alignType; }
    MeetOrSlice meetOrSlice() const { return m_meetOrSlice; }
    bool parse(std::string_view input) final;

    Rect getClipRect(const Rect& viewBoxRect, const Size& viewportSize) const;
    Transform getTransform(const Rect& viewBoxRect, const Size& viewportSize) const;
    void transformRect(Rect& dstRect, Rect& srcRect) const;

private:
    AlignType m_alignType = AlignType::xMidYMid;
    MeetOrSlice m_meetOrSlice = MeetOrSlice::Meet;
};

} // namespace plutobook

#endif // PLUTOBOOK_SvgPROPERTY_H
