#ifndef PLUTOBOOK_CONTENTBOX_H
#define PLUTOBOOK_CONTENTBOX_H

#include "global-string.h"
#include "text-box.h"

namespace plutobook {

class ContentBox : public TextBox {
public:
    ContentBox(const RefPtr<BoxStyle>& style);

    bool isContentBox() const final { return true; }

    const char* name() const override { return "ContentBox"; }
};

template<>
struct is_a<ContentBox> {
    static bool check(const Box& box) { return box.isContentBox(); }
};

class LeaderBox final : public ContentBox {
public:
    LeaderBox(const RefPtr<BoxStyle>& style);

    bool isLeaderBox() const final { return true; }

    const char* name() const final { return "LeaderBox"; }
};

template<>
struct is_a<LeaderBox> {
    static bool check(const Box& box) { return box.isLeaderBox(); }
};

class TargetCounterBox final : public ContentBox {
public:
    TargetCounterBox(const RefPtr<BoxStyle>& style, const HeapString& fragment, const GlobalString& identifier, const HeapString& seperator, const GlobalString& listStyle);

    bool isTargetCounterBox() const final { return true; }

    void build() final;

    const char* name() const final { return "TargetCounterBox"; }

private:
    HeapString m_fragment;
    GlobalString m_identifier;
    HeapString m_seperator;
    GlobalString m_listStyle;
};

template<>
struct is_a<TargetCounterBox> {
    static bool check(const Box& box) { return box.isTargetCounterBox(); }
};

class CssCounterValue;
class CssFunctionValue;
class CssAttrValue;

class Counters;
class Element;

class ContentBoxBuilder {
public:
    ContentBoxBuilder(Counters& counters, Element* element, Box* box);

    void build();

private:
    void addText(const HeapString& text);
    void addLeaderText(const HeapString& text);
    void addLeader(const CssValue& value);
    void addElement(const CssValue& value);
    void addCounter(const CssCounterValue& counter);
    void addTargetCounter(const CssFunctionValue& function);
    void addQuote(CssValueID value);
    void addQrCode(const CssFunctionValue& function);
    void addImage(RefPtr<Image> image);

    const HeapString& resolveAttr(const CssAttrValue& attr) const;

    Counters& m_counters;
    Element* m_element;
    Box* m_box;
    BoxStyle* m_style;
    TextBox* m_lastTextBox{nullptr};
};

} // namespace plutobook

#endif // PLUTOBOOK_CONTENTBOX_H
