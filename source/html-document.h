#pragma once

#include "document.h"

#include <optional>

namespace plutobook {
    class HtmlDocument;

    class HtmlElement : public Element {
    public:
        static constexpr ClassKind classKind = ClassKind::HtmlElement;

        HtmlElement(Document* document, GlobalString tagName);

        void buildFirstLetterPseudoBox(Box* parent);
        void buildPseudoBox(Counters& counters, Box* parent,
                            PseudoType pseudoType);
        void buildElementBox(Counters& counters, Box* box);
        void buildBox(Counters& counters, Box* parent) override;

        void collectAttributeStyle(std::string& output,
                                   GlobalString name,
                                   const HeapString& value) const override;

    protected:
        template<typename T = int>
        std::optional<T> parseIntegerAttribute(GlobalString name) const;
        std::optional<unsigned>
        parseNonNegativeIntegerAttribute(GlobalString name) const;
    };

    extern template bool is<HtmlElement>(const Node& value);

    class HtmlBodyElement final : public HtmlElement {
    public:
        HtmlBodyElement(Document* document);

        void collectAttributeStyle(std::string& output,
                                   GlobalString name,
                                   const HeapString& value) const final;
    };

    class HtmlFontElement final : public HtmlElement {
    public:
        HtmlFontElement(Document* document);

        void collectAttributeStyle(std::string& output,
                                   GlobalString name,
                                   const HeapString& value) const final;
    };

    class Image;

    class HtmlImageElement final : public HtmlElement {
    public:
        HtmlImageElement(Document* document);

        void collectAttributeStyle(std::string& output,
                                   GlobalString name,
                                   const HeapString& value) const final;
        const HeapString& altText() const;
        RefPtr<Image> srcImage() const;

        Box* createBox(const RefPtr<BoxStyle>& style) final;
    };

    class HtmlHrElement final : public HtmlElement {
    public:
        HtmlHrElement(Document* document);

        void collectAttributeStyle(std::string& output,
                                   GlobalString name,
                                   const HeapString& value) const final;
    };

    class HtmlBrElement final : public HtmlElement {
    public:
        HtmlBrElement(Document* document);

        Box* createBox(const RefPtr<BoxStyle>& style) final;
    };

    class HtmlWbrElement final : public HtmlElement {
    public:
        HtmlWbrElement(Document* document);

        Box* createBox(const RefPtr<BoxStyle>& style) final;
    };

    class HtmlLiElement final : public HtmlElement {
    public:
        HtmlLiElement(Document* document);

        std::optional<int> value() const;

        void collectAttributeStyle(std::string& output,
                                   GlobalString name,
                                   const HeapString& value) const final;
    };

    class HtmlOlElement final : public HtmlElement {
    public:
        HtmlOlElement(Document* document);

        int start() const;

        void collectAttributeStyle(std::string& output,
                                   GlobalString name,
                                   const HeapString& value) const final;
    };

    class HtmlTableElement final : public HtmlElement {
    public:
        HtmlTableElement(Document* document);

        void parseAttribute(GlobalString name,
                            const HeapString& value) final;

        void collectAdditionalCellAttributeStyle(std::string& output) const;
        void collectAdditionalRowGroupAttributeStyle(std::string& output) const;
        void collectAdditionalColGroupAttributeStyle(std::string& output) const;
        void collectAdditionalAttributeStyle(std::string& output) const final;

        void collectAttributeStyle(std::string& output,
                                   GlobalString name,
                                   const HeapString& value) const final;

    private:
        enum class Rules : uint16_t { Unset, None, Groups, Rows, Cols, All };

        static Rules parseRulesAttribute(std::string_view value);

        enum class Frame : uint16_t {
            Unset,
            Void,
            Above,
            Below,
            Hsides,
            Lhs,
            Rhs,
            Vsides,
            Box,
            Border
        };

        static Frame parseFrameAttribute(std::string_view value);

        uint16_t m_padding = 0;
        uint16_t m_border = 0;
        Rules m_rules = Rules::Unset;
        Frame m_frame = Frame::Unset;
    };

    class HtmlTablePartElement : public HtmlElement {
    public:
        HtmlTablePartElement(Document* document, GlobalString tagName);

        HtmlTableElement* findParentTable() const;

        void collectAttributeStyle(std::string& output,
                                   GlobalString name,
                                   const HeapString& value) const override;
    };

    class HtmlTableSectionElement final : public HtmlTablePartElement {
    public:
        HtmlTableSectionElement(Document* document,
                                GlobalString tagName);

        void collectAdditionalAttributeStyle(std::string& output) const final;
    };

    class HtmlTableRowElement final : public HtmlTablePartElement {
    public:
        HtmlTableRowElement(Document* document);
    };

    class HtmlTableColElement final : public HtmlTablePartElement {
    public:
        HtmlTableColElement(Document* document, GlobalString tagName);

        unsigned span() const;

        void collectAdditionalAttributeStyle(std::string& output) const final;
        Box* createBox(const RefPtr<BoxStyle>& style) final;
    };

    class HtmlTableCellElement final : public HtmlTablePartElement {
    public:
        HtmlTableCellElement(Document* document, GlobalString tagName);

        unsigned colSpan() const;
        unsigned rowSpan() const;

        void collectAdditionalAttributeStyle(std::string& output) const final;
        Box* createBox(const RefPtr<BoxStyle>& style) final;
    };

    class HtmlInputElement final : public HtmlElement {
    public:
        HtmlInputElement(Document* document);

        unsigned size() const;

        Box* createBox(const RefPtr<BoxStyle>& style) final;
    };

    class HtmlTextAreaElement final : public HtmlElement {
    public:
        HtmlTextAreaElement(Document* document);

        unsigned rows() const;
        unsigned cols() const;

        Box* createBox(const RefPtr<BoxStyle>& style) final;
    };

    class HtmlSelectElement final : public HtmlElement {
    public:
        HtmlSelectElement(Document* document);

        unsigned size() const;

        Box* createBox(const RefPtr<BoxStyle>& style) final;
    };

    class HtmlStyleElement final : public HtmlElement {
    public:
        HtmlStyleElement(Document* document);

        const HeapString& type() const;
        const HeapString& media() const;

        void finishParsingDocument() final;
    };

    class HtmlLinkElement final : public HtmlElement {
    public:
        HtmlLinkElement(Document* document);

        const HeapString& rel() const;
        const HeapString& type() const;
        const HeapString& media() const;

        void finishParsingDocument() final;
    };

    class HtmlTitleElement final : public HtmlElement {
    public:
        HtmlTitleElement(Document* document);

        void finishParsingDocument() final;
    };

    class HtmlBaseElement final : public HtmlElement {
    public:
        HtmlBaseElement(Document* document);

        void finishParsingDocument() final;
    };

    class HtmlDocument final : public Document {
    public:
        static constexpr ClassKind classKind = ClassKind::HtmlDocument;

        static std::unique_ptr<HtmlDocument>
        create(Book* book, ResourceFetcher* fetcher, Url baseUrl);

        bool parse(const std::string_view& content) final;

    private:
        HtmlDocument(Book* book, ResourceFetcher* fetcher, Url baseUrl);
    };
} // namespace plutobook