#ifndef SvgLINELAYOUT_H
#define SvgLINELAYOUT_H

#include "line-layout.h"

#include <optional>
#include <map>

namespace plutobook {

struct SvgCharacterPosition {
    std::optional<float> x;
    std::optional<float> y;
    std::optional<float> dx;
    std::optional<float> dy;
    std::optional<float> rotate;
};

using SvgCharacterPositions = std::map<uint32_t, SvgCharacterPosition>;

class SvgTextBox;
class SvgTextPositioningElement;

struct SvgTextPosition {
    SvgTextPosition(SvgTextPositioningElement* element, uint32_t startOffset, uint32_t endOffset)
        : element(element), startOffset(startOffset), endOffset(endOffset)
    {}

    SvgTextPositioningElement* element = nullptr;
    uint32_t startOffset = 0;
    uint32_t endOffset = 0;
};

using SvgTextPositionList = std::vector<SvgTextPosition>;

class SvgLineItemsBuilder final : private LineItemsBuilder {
public:
    SvgLineItemsBuilder(LineItemsData& data, SvgTextPositionList& positions);

    void appendText(Box* box, const HeapString& data);

    void enterInline(Box* box);
    void exitInline(Box* box);

    void enterBlock(Box* box);
    void exitBlock(Box* box);

private:
    SvgTextPositionList& m_textPositions;
    uint32_t m_itemIndex = 0;
};

struct SvgTextFragment {
    explicit SvgTextFragment(const LineItem& item) : item(item) {}
    const LineItem& item;
    TextShapeView shape;
    bool startsNewTextChunk = false;
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
    float angle = 0;
};

using SvgTextFragmentList = std::pmr::vector<SvgTextFragment>;

class SvgTextFragmentsBuilder {
public:
    SvgTextFragmentsBuilder(SvgTextFragmentList& fragments, const LineItemsData& data, const SvgCharacterPositions& positions);

    void layout();

private:
    void handleTextItem(const LineItem& item);
    void handleBidiControl(const LineItem& item);
    SvgTextFragmentList& m_fragments;
    const LineItemsData& m_data;
    const SvgCharacterPositions& m_positions;
    uint32_t m_characterOffset = 0;
    float m_x = 0;
    float m_y = 0;
};

class Rect;
class SvgRenderState;

class SvgLineLayout {
public:
    explicit SvgLineLayout(SvgTextBox* block);

    Rect boundingRect() const;
    void render(const SvgRenderState& state) const;
    void layout();
    void build();

private:
    SvgTextBox* m_block;
    SvgTextPositionList m_textPositions;
    SvgTextFragmentList m_fragments;
    LineItemsData m_data;
};

} // namespace plutobook

#endif // SvgLINELAYOUT_H
