#include "plutobook.hpp"
#include "html-document.h"
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

    class PLUTOBOOK_API UIContext final : Context {
    public:
        UIContext(const PageSize& pageSize, const PageMargins& pageMargins,
                  MediaType mediaType)
            : m_pageSize(pageSize), m_pageMargins(pageMargins),
              m_mediaType(mediaType) {}

        float viewportWidth() const override {
            return std::max(0.f, m_pageSize.width() - m_pageMargins.left() -
                                     m_pageMargins.right()) /
                   units::px;
        }
        float viewportHeight() const override {
            return std::max(0.f, m_pageSize.height() - m_pageMargins.top() -
                                     m_pageMargins.bottom()) /
                   units::px;
        }
        MediaType mediaType() const override { return m_mediaType; }
        PageSize pageSize() const override { return m_pageSize; }
        PageMargins pageMargins() const override { return m_pageMargins; }

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
            return false;
        }

        bool processMouseMove(const MouseMoveEvent& evt) {
            if (Element* newHoverElem = findElementAtPoint(evt.m_pt)) {
                if (newHoverElem != m_hoverElem) {
                    if (m_hoverElem) {
                        dispatchEvent(m_hoverElem,
                                      MouseEvent(EventID::MouseOut, evt.m_pt,
                                                 evt.m_buttons));
                    }
                    if (newHoverElem) {
                        dispatchEvent(newHoverElem,
                                      MouseEvent(EventID::MouseIn, evt.m_pt,
                                                 evt.m_buttons));
                    }
                    m_hoverElem = newHoverElem;
                }
            } else {
                if (m_hoverElem) {
                    dispatchEvent(
                        m_hoverElem,
                        MouseEvent(EventID::MouseOut, evt.m_pt, evt.m_buttons));
                    m_hoverElem = nullptr;
                }
            }
            return true;
        }

    private:
        Element* findElementAtPoint(Point pt) const;

        void updateHoverChain(Point pt, FlagSet<MouseButton> buttons) {
            std::vector<Element*> hoverChain;
            auto elem = findElementAtPoint(pt);
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
        }

        PageSize m_pageSize;
        PageMargins m_pageMargins;
        MediaType m_mediaType;

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