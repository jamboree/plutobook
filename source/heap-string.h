#pragma once

#include <atomic>
#include <ostream>
#include <string_view>
#include <boost/container_hash/hash.hpp>

namespace plutobook {
    struct StrEqual : std::equal_to<std::string_view> {
        using is_transparent = void;
    };

    struct StrHash : boost::hash<std::string_view> {
        using is_transparent = void;
    };

    class HeapString;

    HeapString createString(const std::string_view& value);

    HeapString concatenateString(const std::string_view& a,
                                 const std::string_view& b);

    class HeapString : public std::string_view {
    public:
        HeapString() = default;

        HeapString(const HeapString& other) noexcept
            : std::string_view(other), m_head(other.m_head) {
            if (m_head)
                m_head->acquire();
        }

        HeapString(HeapString&& other) noexcept
            : std::string_view(other), m_head(other.m_head) {
            static_cast<std::string_view&>(other) = {};
            other.m_head = nullptr;
        }

        ~HeapString() {
            if (m_head)
                m_head->release();
        }

        HeapString& operator=(HeapString other) noexcept {
            this->~HeapString();
            return *new (this) HeapString(std::move(other));
        }

        const char* begin() const { return data(); }

        const char* end() const { return data() + size(); }

        HeapString substring(size_t offset) const {
            return HeapString(*this, substr(offset));
        }

        HeapString substring(size_t offset, size_t count) const {
            return HeapString(*this, substr(offset, count));
        }

        const std::string_view& value() const { return *this; }

        friend HeapString createString(const std::string_view& value) {
            const auto head =
                new (::operator new(sizeof(Head) + value.size())) Head;
            std::memcpy(head->m_data, value.data(), value.size());
            return HeapString(head, value.size());
        }

        friend HeapString concatenateString(const std::string_view& a,
                                            const std::string_view& b) {
            const auto size = a.size() + b.size();
            const auto head = new (::operator new(sizeof(Head) + size)) Head;
            auto p = head->m_data;
            std::memcpy(p, a.data(), a.size());
            p += a.size();
            std::memcpy(p, b.data(), b.size());
            return HeapString(head, size);
        }

    protected:
        struct Head {
            std::atomic<unsigned> m_refCount{1};
            char m_data[];

            void acquire() noexcept {
                m_refCount.fetch_add(1u, std::memory_order::relaxed);
            };

            void release() noexcept {
                if (m_refCount.fetch_sub(1u, std::memory_order::relaxed) ==
                    1u) {
                    this->~Head();
                    ::operator delete(this);
                }
            }
        };

        HeapString(Head* head, size_t size) noexcept
            : std::string_view(head->m_data, size), m_head(head) {}

        HeapString(const HeapString& other, std::string_view str) noexcept
            : std::string_view(str), m_head(other.m_head) {
            m_head->acquire();
        }

        Head* m_head = nullptr;
    };
} // namespace plutobook