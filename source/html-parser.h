#ifndef PLUTOBOOK_HtmlPARSER_H
#define PLUTOBOOK_HtmlPARSER_H

#include "html-tokenizer.h"

namespace plutobook {

class HtmlElementList {
public:
    HtmlElementList() = default;

    void remove(const Element* element);
    void remove(size_t index);
    void replace(const Element* element, Element* item);
    void replace(size_t index, Element* element);
    void insert(size_t index, Element* element);

    size_t index(const Element* element) const;
    bool contains(const Element* element) const;
    Element* at(size_t index) const { return m_elements.at(index); }

    bool empty() const { return m_elements.empty(); }
    size_t size() const { return m_elements.size(); }

protected:
    std::vector<Element*> m_elements;
};

class HtmlElementStack : public HtmlElementList {
public:
    HtmlElementStack() = default;

    void push(Element* element);
    void pushHtmlHtmlElement(Element* element);
    void pushHtmlHeadElement(Element* element);
    void pushHtmlBodyElement(Element* element);

    void pop();
    void popHtmlHeadElement();
    void popHtmlBodyElement();
    void popUntil(const GlobalString& tagName);
    void popUntil(const Element* element);
    void popUntilNumberedHeaderElement();
    void popUntilTableScopeMarker();
    void popUntilTableBodyScopeMarker();
    void popUntilTableRowScopeMarker();
    void popUntilForeignContentScopeMarker();
    void popUntilPopped(const GlobalString& tagName);
    void popUntilPopped(const Element* element);
    void popUntilNumberedHeaderElementPopped();
    void popAll();

    void generateImpliedEndTags();
    void generateImpliedEndTagsExcept(const GlobalString& tagName);

    void removeHtmlHeadElement(const Element* element);
    void removeHtmlBodyElement();

    void insertAfter(const Element* element, Element* item);

    Element* furthestBlockForFormattingElement(const Element* formattingElement) const;
    Element* topmost(const GlobalString& tagName) const;
    Element* previous(const Element* element) const;
    Element* top() const { return m_elements.back(); }

    Element* htmlElement() const { return m_htmlElement; }
    Element* headElement() const { return m_headElement; }
    Element* bodyElement() const { return m_bodyElement; }

    template<bool isMarker(const Element*)>
    bool inScopeTemplate(const GlobalString& tagName) const;
    bool inScope(const Element* element) const;
    bool inScope(const GlobalString& tagName) const;
    bool inButtonScope(const GlobalString& tagName) const;
    bool inListItemScope(const GlobalString& tagName) const;
    bool inTableScope(const GlobalString& tagName) const;
    bool inSelectScope(const GlobalString& tagName) const;
    bool isNumberedHeaderElementInScope() const;

private:
    Element* m_htmlElement{nullptr};
    Element* m_headElement{nullptr};
    Element* m_bodyElement{nullptr};
};

class HtmlFormattingElementList : public HtmlElementList {
public:
    HtmlFormattingElementList() = default;

    void append(Element* element);
    void appendMarker();
    void clearToLastMarker();

    Element* closestElementInScope(const GlobalString& tagName);
};

class HtmlDocument;

class HtmlParser {
public:
    HtmlParser(HtmlDocument* document, const std::string_view& content);

    bool parse();

private:
    Element* createHtmlElement(const HtmlTokenView& token) const;
    Element* createElement(const HtmlTokenView& token, const GlobalString& namespaceURI) const;
    Element* cloneElement(const Element* element) const;
    Element* currentElement() const { return m_openElements.top(); }

    struct InsertionLocation {
        ContainerNode* parent{nullptr};
        Node* nextChild{nullptr};
    };

    void insertNode(const InsertionLocation& location, Node* child);
    void insertElement(Element* child, ContainerNode* parent);
    void insertElement(Element* child);

    bool shouldFosterParent() const;
    void findFosterLocation(InsertionLocation& location) const;
    void fosterParent(Node* child);

    void reconstructActiveFormattingElements();
    void flushPendingTableCharacters();
    void closeTheCell();

    static void adjustSvgTagNames(HtmlTokenView& token);
    static void adjustSvgAttributes(HtmlTokenView& token);
    static void adjustMathMLAttributes(HtmlTokenView& token);
    static void adjustForeignAttributes(HtmlTokenView& token);

    void insertDoctype(const HtmlTokenView& token);
    void insertComment(const HtmlTokenView& token, ContainerNode* parent);
    void insertHtmlHtmlElement(const HtmlTokenView& token);
    void insertHeadElement(const HtmlTokenView& token);
    void insertHtmlBodyElement(const HtmlTokenView& token);
    void insertHtmlFormElement(const HtmlTokenView& token);
    void insertSelfClosingHtmlElement(const HtmlTokenView& token);
    void insertHtmlElement(const HtmlTokenView& token);
    void insertHtmlFormattingElement(const HtmlTokenView& token);
    void insertForeignElement(const HtmlTokenView& token, const GlobalString& namespaceURI);
    void insertTextNode(const std::string_view& data);

    enum class InsertionMode {
        Initial,
        BeforeHtml,
        BeforeHead,
        InHead,
        InHeadNoscript,
        AfterHead,
        InBody,
        Text,
        InTable,
        InTableText,
        InCaption,
        InColumnGroup,
        InTableBody,
        InRow,
        InCell,
        InSelect,
        InSelectInTable,
        AfterBody,
        InFrameset,
        AfterFrameset,
        AfterAfterBody,
        AfterAfterFrameset,
        InForeignContent
    };

    void resetInsertionModeAppropriately();
    InsertionMode currentInsertionMode(const HtmlTokenView& token) const;

    void handleInitialMode(HtmlTokenView& token);
    void handleBeforeHtmlMode(HtmlTokenView& token);
    void handleBeforeHeadMode(HtmlTokenView& token);
    void handleInHeadMode(HtmlTokenView& token);
    void handleInHeadNoscriptMode(HtmlTokenView& token);
    void handleAfterHeadMode(HtmlTokenView& token);
    void handleInBodyMode(HtmlTokenView& token);
    void handleTextMode(HtmlTokenView& token);
    void handleInTableMode(HtmlTokenView& token);
    void handleInTableTextMode(HtmlTokenView& token);
    void handleInCaptionMode(HtmlTokenView& token);
    void handleInColumnGroupMode(HtmlTokenView& token);
    void handleInTableBodyMode(HtmlTokenView& token);
    void handleInRowMode(HtmlTokenView& token);
    void handleInCellMode(HtmlTokenView& token);
    void handleInSelectMode(HtmlTokenView& token);
    void handleInSelectInTableMode(HtmlTokenView& token);
    void handleAfterBodyMode(HtmlTokenView& token);
    void handleInFramesetMode(HtmlTokenView& token);
    void handleAfterFramesetMode(HtmlTokenView& token);
    void handleAfterAfterBodyMode(HtmlTokenView& token);
    void handleAfterAfterFramesetMode(HtmlTokenView& token);
    void handleInForeignContentMode(HtmlTokenView& token);

    void handleFakeStartTagToken(const GlobalString& tagName);
    void handleFakeEndTagToken(const GlobalString& tagName);

    void handleFormattingEndTagToken(HtmlTokenView& token);
    void handleOtherFormattingEndTagToken(HtmlTokenView& token);

    void handleErrorToken(HtmlTokenView& token);
    void handleRCDataToken(HtmlTokenView& token);
    void handleRawTextToken(HtmlTokenView& token);
    void handleScriptDataToken(HtmlTokenView& token);

    void handleDoctypeToken(HtmlTokenView& token);
    void handleCommentToken(HtmlTokenView& token);
    void handleToken(HtmlTokenView& token, InsertionMode mode);
    void handleToken(HtmlTokenView& token) { handleToken(token, m_insertionMode); }

    HtmlDocument* m_document;
    Element* m_form{nullptr};
    Element* m_head{nullptr};

    HtmlTokenizer m_tokenizer;
    HtmlElementStack m_openElements;
    HtmlFormattingElementList m_activeFormattingElements;
    std::string m_pendingTableCharacters;

    InsertionMode m_insertionMode{InsertionMode::Initial};
    InsertionMode m_originalInsertionMode{InsertionMode::Initial};
    bool m_inQuirksMode{false};
    bool m_framesetOk{false};
    bool m_fosterRedirecting{false};
    bool m_skipLeadingNewline{false};
};

} // namespace plutobook

#endif // PLUTOBOOK_HtmlPARSER_H
