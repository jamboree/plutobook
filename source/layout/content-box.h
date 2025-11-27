#pragma once

#include "global-string.h"
#include "text-box.h"

namespace plutobook {
    class ContentBox : public TextBox {
    public:
        ContentBox(ClassKind type, const RefPtr<BoxStyle>& style);

        const char* name() const override { return "ContentBox"; }
    };

    extern template bool is<ContentBox>(const Box& value);

    class LeaderBox final : public ContentBox {
    public:
        static constexpr ClassKind classKind = ClassKind::Leader;

        LeaderBox(const RefPtr<BoxStyle>& style);

        const char* name() const final { return "LeaderBox"; }
    };

    class TargetCounterBox final : public ContentBox {
    public:
        static constexpr ClassKind classKind = ClassKind::TargetCounter;

        TargetCounterBox(const RefPtr<BoxStyle>& style,
                         const HeapString& fragment,
                         GlobalString identifier,
                         const HeapString& seperator,
                         GlobalString listStyle);

        void build() final;

        const char* name() const final { return "TargetCounterBox"; }

    private:
        HeapString m_fragment;
        GlobalString m_identifier;
        HeapString m_seperator;
        GlobalString m_listStyle;
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