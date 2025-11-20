#ifndef FORMCONTROLBOX_H
#define FORMCONTROLBOX_H

#include "block-box.h"

namespace plutobook {

class HtmlElement;

class TextInputBox final : public BlockFlowBox {
public:
    TextInputBox(HtmlElement* element, const RefPtr<BoxStyle>& style);

    bool isTextInputBox() const final { return true; }

    HtmlElement* element() const;
    std::optional<float> inlineBlockBaseline() const final;
    uint32_t rows() const { return m_rows; }
    uint32_t cols() const { return m_cols; }

    void setRows(uint32_t rows) { m_rows = rows; }
    void setCols(uint32_t cols) { m_cols = cols; }

    void computeIntrinsicWidths(float& minWidth, float& maxWidth) const final;
    void computeHeight(float& y, float& height, float& marginTop, float& marginBottom) const final;

    const char* name() const final { return "TextInputBox"; }

private:
    uint32_t m_rows{1};
    uint32_t m_cols{1};
};

template<>
struct is_a<TextInputBox> {
    static bool check(const Box& box) { return box.isTextInputBox(); }
};

class HtmlSelectElement;

class SelectBox final : public BlockBox {
public:
    SelectBox(HtmlSelectElement* element, const RefPtr<BoxStyle>& style);

    bool isSelectBox() const final { return true; }

    HtmlSelectElement* element() const;
    std::optional<float> inlineBlockBaseline() const final;
    uint32_t size() const { return m_size; }

    void addChild(Box* newChild) final;
    void updateOverflowRect() final;
    void computeIntrinsicWidths(float& minWidth, float& maxWidth) const final;
    void computeHeight(float& y, float& height, float& marginTop, float& marginBottom) const final;
    void paintContents(const PaintInfo& info, const Point& offset, PaintPhase phase) final;
    void layout(FragmentBuilder* fragmentainer) final;

    const char* name() const final { return "SelectBox"; }

private:
    const uint32_t m_size;
};

template<>
struct is_a<SelectBox> {
    static bool check(const Box& box) { return box.isSelectBox(); }
};

} // namespace plutobook

#endif // FORMCONTROLBOX_H
