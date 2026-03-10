#include "plutobook-ui.h"
#include "box-view.h"
#include "html-document.h"
#include "xml-document.h"
#include "text-resource.h"
#include "image-resource.h"
#include "font-resource.h"
#include <boost/unordered/unordered_flat_map.hpp>

namespace plutobook {
    template<class Flag>
    struct FlagSet {
        using UnderlyingT = std::underlying_type_t<Flag>;

        FlagSet() = default;

        constexpr FlagSet(Flag val) : flags(static_cast<UnderlyingT>(val)) {}

        constexpr explicit FlagSet(UnderlyingT val) : flags(val) {}

        constexpr explicit operator bool() const { return flags != 0; }

        constexpr bool contains(FlagSet other) const {
            return (flags & other.flags) == other.flags;
        }

        friend constexpr UnderlyingT toUnderlying(FlagSet self) {
            return self.flags;
        }

        friend bool operator==(FlagSet a, FlagSet b) = default;

        friend constexpr FlagSet operator|(FlagSet a, FlagSet b) noexcept {
            return FlagSet(a.flags | b.flags);
        }

        friend constexpr FlagSet operator&(FlagSet a, FlagSet b) noexcept {
            return FlagSet(a.flags & b.flags);
        }

        friend constexpr FlagSet operator^(FlagSet a, FlagSet b) noexcept {
            return FlagSet(a.flags ^ b.flags);
        }

    private:
        UnderlyingT flags = 0;
    };

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
        EventID m_id;
    };

    enum class MouseButton : uint8_t { Left = 1, Middle = 2, Right = 4 };

    struct MouseEvent : Event {
        MouseEvent(EventID id, Point pt, FlagSet<MouseButton> buttons)
            : Event{id}, m_pt(pt), m_buttons(buttons) {}

        Point m_pt;
        FlagSet<MouseButton> m_buttons;
    };

    struct MouseMoveEvent final : MouseEvent {
        MouseMoveEvent(Point pt, FlagSet<MouseButton> buttons)
            : MouseEvent(EventID::MouseMove, pt, buttons) {}
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

    class UIContext final : Context {
    public:
        friend UIContext* createUIContext() { return new UIContext(); }

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

        friend void processMouseMoveEvent(UIContext* ui, float x, float y) {
            ui->processEvent(MouseMoveEvent(Point{x, y}, {}));
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

        bool processEvent(const MouseMoveEvent& evt) {
            updateHoverChain(evt.m_pt, evt.m_buttons);
            return true;
        }

        bool processDefaultEvent(Element* elem, const Event& evt) {
            switch (evt.m_id) {
            case EventID::MouseDown: {
                const auto& mouseEvt = static_cast<const MouseEvent&>(evt);
                if (mouseEvt.m_buttons & MouseButton::Left) {
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

        void updateHoverChain(Point pt, FlagSet<MouseButton> buttons) {
            std::vector<Element*> hoverChain;
            auto elem = getElementAtPoint(pt);
            while (elem) {
                hoverChain.push_back(elem);
                elem = elem->parentElement();
            }
            std::ranges::reverse(hoverChain);
            const auto diff = std::ranges::mismatch(m_hoverChain, hoverChain);
            for (const auto elem :
                 std::ranges::subrange(diff.in1, m_hoverChain.end())) {
                dispatchEvent(elem, MouseEvent(EventID::MouseOut, pt, buttons));
            }
            for (const auto elem :
                 std::ranges::subrange(diff.in2, hoverChain.end())) {
                dispatchEvent(elem, MouseEvent(EventID::MouseIn, pt, buttons));
            }
            if (m_mousePos != pt) {
                for (const auto elem :
                     std::ranges::subrange(m_hoverChain.begin(), diff.in1)) {
                    dispatchEvent(elem,
                                  MouseEvent(EventID::MouseMove, pt, buttons));
                }
                m_mousePos = pt;
            }
            m_hoverChain = std::move(hoverChain);
        }

        std::unique_ptr<Document> m_document;
        // The element that currently has input focus.
        Element* m_focusElem = nullptr;
        // The top-most element being hovered over.
        Element* m_hoverElem = nullptr;
        // The element that was being hovered over when the primary mouse button
        // was pressed most recently.
        Element* m_activeElem = nullptr;

        // The element that was clicked on last.
        Element* m_lastClickElem = nullptr;
        Point m_lastMouseClickPos;

        // Input state; stored from the most recent input events we receive from
        // the application.
        Point m_mousePos;
        bool m_mouseActive = false;

        std::vector<Element*> m_hoverChain;
        std::vector<Element*> m_activeChain;

        ElementEventHandlerMap m_eventMap[uint16_t(EventID::COUNT)];
    };
} // namespace plutobook