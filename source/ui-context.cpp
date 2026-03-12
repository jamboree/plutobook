#include "plutobook-ui.h"
#include "box-view.h"
#include "css-rule.h"
#include "html-document.h"
#include "xml-document.h"
#include "text-resource.h"
#include "image-resource.h"
#include "font-resource.h"
#include <boost/unordered/unordered_flat_map.hpp>

namespace plutobook {
    enum class EventID : uint16_t {
        // Core events
        MouseUp,
        MouseDown,
        MouseScroll,
        MouseIn,
        MouseOut,
        MouseMove,
        KeyUp,
        KeyDown,
        Click,
        DoubleClick,

        COUNT
    };

    struct Event {
        Event(EventID id) : m_id(id) {}

        EventID m_id;
    };

    struct MouseInEvent final : Event {
        MouseInEvent() : Event(EventID::MouseIn) {}
    };

    struct MouseOutEvent final : Event {
        MouseOutEvent() : Event(EventID::MouseOut) {}
    };

    struct MouseButtonEvent : Event {
        MouseButtonEvent(EventID id, MouseButton button)
            : Event(id), m_button(button) {}

        MouseButton m_button;
    };

    struct MousePointEvent : Event {
        MousePointEvent(EventID id, Point pt) : Event(id), m_pt(pt) {}

        Point m_pt;
    };

    struct MouseMoveEvent final : MousePointEvent {
        MouseMoveEvent(Point pt) : MousePointEvent(EventID::MouseMove, pt) {}
    };

    struct MouseUpEvent final : MouseButtonEvent {
        MouseUpEvent(MouseButton button)
            : MouseButtonEvent(EventID::MouseUp, button) {}
    };

    struct MouseDownEvent final : MouseButtonEvent {
        MouseDownEvent(MouseButton button)
            : MouseButtonEvent(EventID::MouseDown, button) {}
    };

    struct ClickEvent final : MouseButtonEvent {
        ClickEvent(MouseButton button)
            : MouseButtonEvent(EventID::Click, button) {}
    };

    struct DoubleClickEvent final : MouseButtonEvent {
        DoubleClickEvent(MouseButton button)
            : MouseButtonEvent(EventID::DoubleClick, button) {}
    };

    struct EventHandlerHeader {
        EventHandlerHeader* m_prev;
        EventHandlerHeader* m_next;

        void unlink() noexcept {
            m_prev->m_next = m_next;
            m_next->m_prev = m_prev;
            m_prev = nullptr;
            m_next = nullptr;
        }
    };

    struct EventHandlerList {
        EventHandlerHeader m_header{&m_header, &m_header};

        bool isEmpty() const noexcept { return m_header.m_next == &m_header; }

        void push(EventHandlerHeader* node) noexcept {
            m_header.m_prev->m_next = node;
            node->m_prev = m_header.m_prev;
            node->m_next = &m_header;
            m_header.m_prev = node;
        }
    };

    struct EventHandler : EventHandlerHeader {
        bool (&m_callback)(Element*, const Event&);
    };

    using ElementEventHandlerMap =
        boost::unordered_flat_map<Element*, EventHandlerList>;

    class FdOutputStream final : public OutputStream {
    public:
        explicit FdOutputStream(FILE* fd) : m_handle(fd) {}

        size_t write(const char* data, size_t length) final {
            return fwrite(data, 1, length, m_handle);
        }

    private:
        FILE* m_handle;
    };

    class UIContext final : Context {
    public:
        UIContext(UISystem* system) : m_system(system) {}

        friend UIContext* createUIContext(UISystem* system) {
            return new UIContext(system);
        }

        friend void destroyUIContext(UIContext* ui) { delete ui; }

        friend void setSize(UIContext* ui, float width, float height) {
            ui->m_document->setContainerSize(width, height);
        }

        friend void getSize(UIContext* ui, float& width, float& height) {
            width = ui->m_document->width();
            height = ui->m_document->height();
        }

        friend bool loadUrl(UIContext* ui, std::string_view url) {
            auto completeUrl = ResourceLoader::completeUrl(url);
            auto resource = ResourceLoader::loadUrl(completeUrl);
            if (resource.isNull())
                return false;
            if (ui->loadData(resource.content(), resource.contentLength(),
                             resource.mimeType(), resource.textEncoding(),
                             completeUrl.base())) {
                return true;
            }

            plutobook_set_error_message("Unable to load URL '%s': %s",
                                        completeUrl.value().data(),
                                        plutobook_get_error_message());
            return false;
        }

        friend bool update(UIContext* ui) {
            if (ui->m_document->isDirty()) {
                ui->m_document->build();
                ui->m_document->layout();
                return true;
            }
            return false;
        }

        friend void output(UIContext* ui, FILE* fd) {
            FdOutputStream output(fd);
            ui->m_document->serialize(output);
        }

        friend void processMouseIn(UIContext* ui) {
            ui->processEvent(MouseInEvent());
        }

        friend void processMouseOut(UIContext* ui) {
            ui->processEvent(MouseOutEvent());
        }

        friend void processMouseMove(UIContext* ui, float x, float y) {
            ui->processEvent(MouseMoveEvent(Point{x, y}));
        }

        friend void processMouseDown(UIContext* ui, MouseButton button) {
            ui->processEvent(MouseDownEvent(button));
        }

        friend void processMouseUp(UIContext* ui, MouseButton button) {
            ui->processEvent(MouseUpEvent(button));
        }

        friend void renderDocument(const UIContext* ui,
                                   GraphicsContext& context, float x, float y,
                                   float width, float height) {
            ui->m_document->render(context, Rect(x, y, width, height));
        }

        float viewportWidth() const override { return 0.f; }
        float viewportHeight() const override { return 0.f; }
        MediaType mediaType() const override { return MediaType::Screen; }
        PageSize pageSize() const override { return PageSize(); }
        PageMargins pageMargins() const override { return PageMargins::None; }

        void addEventHandler(Element* elem, EventID evtID,
                             EventHandler* handler) {
            m_eventMap[uint16_t(evtID)][elem].push(handler);
        }

        void removeEventHandler(Element* elem, EventID evtID,
                                EventHandler* handler) {
            auto& map = m_eventMap[uint16_t(evtID)];
            const auto it = map.find(elem);
            if (it != map.end()) {
                handler->unlink();
                if (it->second.isEmpty())
                    map.erase(it);
            }
        }

        bool dispatchEvent(Element* elem, const Event& evt) {
            auto& map = m_eventMap[uint16_t(evt.m_id)];
            const auto it = map.find(elem);
            if (it != map.end()) {
                const auto& header = it->second.m_header;
                for (auto node = header.m_next; node != &header;) {
                    const auto handler = static_cast<EventHandler*>(node);
                    node = node->m_next;
                    if (handler->m_callback(elem, evt))
                        return true;
                }
            }
            return processDefaultEvent(elem, evt);
        }

        bool processEvent(const MouseInEvent&) {
            updateHoverChain(m_mousePos, m_document->rootElement());
            return true;
        }

        bool processEvent(const MouseOutEvent&) {
            updateHoverChain(m_mousePos, nullptr);
            return true;
        }

        bool processEvent(const MouseMoveEvent& evt) {
            updateHoverChain(evt.m_pt, getElementAtPoint(evt.m_pt));
            return true;
        }

        bool processEvent(const MouseDownEvent& evt) {
            bool propagate = true;
            const auto hoverElem =
                m_hoverChain.empty() ? nullptr : m_hoverChain.back();
            if (evt.m_button == MouseButton::Left) {
                Element* activeElem = nullptr;
                auto focusElem = hoverElem;
                if (hoverElem) {
                    focusElem = findFocusElement(hoverElem);
                }
                activeElem = focusElem;
                if (hoverElem) {
                    propagate = !dispatchEvent(hoverElem, evt);
                }
                if (propagate) {
                    if (m_lastClickElem == activeElem) {
                        if (hoverElem) {
                            propagate = !dispatchEvent(
                                hoverElem, DoubleClickEvent(evt.m_button));
                            m_lastClickElem = nullptr;
                        }
                    } else {
                        m_lastClickElem = activeElem;
                    }
                }
                m_lastClickPos = m_mousePos;
                m_activeChain.append_range(m_hoverChain);
            } else if (hoverElem) {
                propagate = !dispatchEvent(hoverElem, evt);
            }
            return true;
        }

        bool processEvent(const MouseUpEvent& evt) {
            const auto hoverElem =
                m_hoverChain.empty() ? nullptr : m_hoverChain.back();
            if (evt.m_button == MouseButton::Left) {
                if (hoverElem) {
                    dispatchEvent(hoverElem, evt);
                    const auto activeElem =
                        m_activeChain.empty() ? nullptr : m_activeChain.back();
                    if (findFocusElement(hoverElem) == activeElem) {
                        dispatchEvent(activeElem, ClickEvent(evt.m_button));
                    }
                }
                resetActiveChain();
            } else if (hoverElem) {
                dispatchEvent(hoverElem, evt);
            }
            return true;
        }

        bool processDefaultEvent(Element* elem, const Event& evt) {
            switch (evt.m_id) {
            case EventID::MouseDown: {
                const auto& mouseEvt =
                    static_cast<const MouseButtonEvent&>(evt);
                if (mouseEvt.m_button == MouseButton::Left) {
                    if (!elem->active()) {
                        elem->setActive(true);
                        elem->setDirtyStyle();
                    }
                }
                return true;
            }
            case EventID::MouseIn:
                if (!elem->hover()) {
                    elem->setHover(true);
                    elem->setDirtyStyle();
                }
                return true;
            case EventID::MouseOut:
                if (elem->hover()) {
                    elem->setHover(false);
                    elem->setDirtyStyle();
                }
                return true;
            }
            return true;
        }

    private:
        bool loadData(const char* data, size_t length,
                      std::string_view mimeType, std::string_view textEncoding,
                      std::string_view baseUrl) {
            if (TextResource::isXmlMimeType(mimeType))
                return loadXml(
                    TextResource::decode(data, length, mimeType, textEncoding),
                    baseUrl);
            return loadHtml(
                TextResource::decode(data, length, mimeType, textEncoding),
                baseUrl);
        }

        bool loadXml(std::string_view content, std::string_view baseUrl) {
            m_document = XmlDocument::create(
                this, nullptr, ResourceLoader::completeUrl(baseUrl));
            return m_document->parse(content);
        }

        bool loadHtml(std::string_view content, std::string_view baseUrl) {
            m_document = HtmlDocument::create(
                this, nullptr, ResourceLoader::completeUrl(baseUrl));
            return m_document->parse(content);
        }

        Element* getElementAtPoint(Point pt) const {
            if (const auto box = m_document->box()->getBoxAtPoint(pt)) {
                return to<Element>(box->node());
            }
            return nullptr;
        }

        Element* findFocusElement(Element* elem) {
            if (!m_document->focus()) {
                return nullptr;
            }
            while (elem && !elem->focus()) {
                elem = elem->parentElement();
            }
            return elem;
        }

        static Cursor getCursor(CssValuePtr valuePtr) {
            if (const auto ident = to<CssIdentValue>(valuePtr)) {
                switch (ident->value()) {
                case CssValueID::Default: return Cursor::Default;
                case CssValueID::None: return Cursor::None;
                case CssValueID::Pointer: return Cursor::Pointer;
                case CssValueID::Text: return Cursor::Text;
                }
            }
            return Cursor::Default;
        }

        void updateHoverChain(Point pt, Element* hoverElem) {
            auto cursor = Cursor::Default;
            std::vector<Element*> hoverChain;
            if (hoverElem) {
                cursor =
                    m_hoverChain.empty() || hoverElem != m_hoverChain.back()
                        ? getCursor(
                              hoverElem->style()->get(CssPropertyID::Cursor))
                        : m_cursor;
                do {
                    hoverChain.push_back(hoverElem);
                    hoverElem = hoverElem->parentElement();
                } while (hoverElem);
            }
            if (m_cursor != cursor) {
                m_system->setCursor(cursor);
                m_cursor = cursor;
            }
            std::ranges::reverse(hoverChain);
            const auto diff = std::ranges::mismatch(m_hoverChain, hoverChain);
            for (const auto elem :
                 std::ranges::subrange(diff.in1, m_hoverChain.end())) {
                dispatchEvent(elem, Event(EventID::MouseOut));
            }
            for (const auto elem :
                 std::ranges::subrange(diff.in2, hoverChain.end())) {
                dispatchEvent(elem, Event(EventID::MouseIn));
            }
            if (m_mousePos != pt) {
                for (const auto elem :
                     std::ranges::subrange(m_hoverChain.begin(), diff.in1)) {
                    dispatchEvent(elem, MouseMoveEvent(pt));
                }
                m_mousePos = pt;
            }
            m_hoverChain = std::move(hoverChain);
        }

        void resetActiveChain() {
            for (const auto elem : m_activeChain) {
                if (elem->active()) {
                    elem->setActive(false);
                    elem->setDirtyStyle();
                }
            }
            m_activeChain.clear();
        }

        UISystem* m_system;
        std::unique_ptr<Document> m_document;
        // The element that currently has input focus.
        Element* m_focusElem = nullptr;
#if 0
        // The top-most element being hovered over.
        Element* m_hoverElem = nullptr;
        // The element that was being hovered over when the primary mouse button
        // was pressed most recently.
        Element* m_activeElem = nullptr;
#endif // 0

        // The element that was clicked on last.
        Element* m_lastClickElem = nullptr;
        Point m_lastClickPos;

        // Input state; stored from the most recent input events we receive from
        // the application.
        Point m_mousePos;

        std::vector<Element*> m_hoverChain;
        std::vector<Element*> m_activeChain;

        Cursor m_cursor = Cursor::Default;

        ElementEventHandlerMap m_eventMap[uint16_t(EventID::COUNT)];
    };
} // namespace plutobook