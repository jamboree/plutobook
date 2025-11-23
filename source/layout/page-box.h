#pragma once

#include "global-string.h"
#include "block-box.h"

namespace plutobook {
    class PageMarginBox;

    class PageBox final : public BlockBox {
    public:
        static constexpr ClassKind classKind = ClassKind::Page;

        static std::unique_ptr<PageBox>
        create(const RefPtr<BoxStyle>& style, const GlobalString& pageName,
               uint32_t pageIndex, float pageWidth, float pageHeight,
               float pageScale);

        bool requiresLayer() const final { return true; }

        Rect borderBoundingBox() const final;

        const GlobalString& pageName() const { return m_pageName; }
        uint32_t pageIndex() const { return m_pageIndex; }
        float pageWidth() const { return m_pageWidth; }
        float pageHeight() const { return m_pageHeight; }
        float pageScale() const { return m_pageScale; }

        PageSize pageSize() const;
        Rect pageRect() const;

        PageMarginBox* firstMarginBox() const;
        PageMarginBox* lastMarginBox() const;

        void computeIntrinsicWidths(float& minWidth,
                                    float& maxWidth) const final;
        void computeWidth(float& x, float& width, float& marginLeft,
                          float& marginRight) const final;
        void computeHeight(float& y, float& height, float& marginTop,
                           float& marginBottom) const final;
        void layout(FragmentBuilder* fragmentainer) final;

        void paintRootBackground(const PaintInfo& info) const final;
        void paintContents(const PaintInfo& info, const Point& offset,
                           PaintPhase phase) final;

        const char* name() const final { return "PageBox"; }

    private:
        PageBox(const RefPtr<BoxStyle>& style, const GlobalString& pageName,
                uint32_t pageIndex, float pageWidth, float pageHeight,
                float pageScale);
        GlobalString m_pageName;
        uint32_t m_pageIndex;
        float m_pageWidth;
        float m_pageHeight;
        float m_pageScale;
    };

    inline Rect PageBox::borderBoundingBox() const { return pageRect(); }

    inline Rect PageBox::pageRect() const {
        return Rect(0, 0, m_pageWidth, m_pageHeight);
    }

    enum class PageMarginType : uint8_t;

    class PageMarginBox final : public BlockFlowBox {
    public:
        static constexpr ClassKind classKind = ClassKind::PageMargin;

        PageMarginBox(const RefPtr<BoxStyle>& style, PageMarginType marginType);

        bool requiresLayer() const final { return true; }
        PageMarginType marginType() const { return m_marginType; }
        PageBox* pageBox() const;

        bool isHorizontalFlow() const;
        bool isVerticalFlow() const;

        PageMarginBox* nextMarginBox() const;
        PageMarginBox* prevMarginBox() const;

        float computeVerticalAlignShift() const final;

        void updatePaddings(const Size& availableSize);
        void updateMargins(const Size& availableSize);
        void updateAutoMargins(const Size& availableSize);

        const char* name() const final { return "PageMarginBox"; }

    private:
        PageMarginType m_marginType;
    };

    inline PageBox* PageMarginBox::pageBox() const {
        return static_cast<PageBox*>(parentBox());
    }

    inline PageMarginBox* PageBox::firstMarginBox() const {
        return static_cast<PageMarginBox*>(firstChild());
    }

    inline PageMarginBox* PageBox::lastMarginBox() const {
        return static_cast<PageMarginBox*>(lastChild());
    }

    inline PageMarginBox* PageMarginBox::nextMarginBox() const {
        return static_cast<PageMarginBox*>(nextSibling());
    }

    inline PageMarginBox* PageMarginBox::prevMarginBox() const {
        return static_cast<PageMarginBox*>(prevSibling());
    }

    class Counters;

    class PageLayout {
    public:
        explicit PageLayout(Document* document);

        void layout();

    private:
        void buildPageMargin(const Counters& counters, PageBox* pageBox,
                             PageMarginType marginType);
        void buildPageMargins(const Counters& counters, PageBox* pageBox);

        Document* m_document;
    };
} // namespace plutobook