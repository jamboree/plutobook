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

    class HeapString {
    public:
        HeapString() = default;

        HeapString(const HeapString& other) noexcept
            : m_data(other.m_data), m_offset(other.m_offset),
              m_size(other.m_size) {
            if (m_data && isHeap())
                asHeap()->acquire();
        }

        HeapString(HeapString&& other) noexcept
            : m_data(other.m_data), m_offset(other.m_offset),
              m_size(other.m_size) {
            other.m_data = 0;
            other.m_offset = 0;
            other.m_size = 0;
        }

        ~HeapString() {
            if (m_data && isHeap())
                asHeap()->release();
        }

        static HeapString create(const std::string_view& value) {
            const auto size = value.size();
            if (!size) [[unlikely]]
                return HeapString();
            if (size < sizeof(HeapString)) {
                return HeapString(value.data(), size);
            } else {
                const auto head =
                    new (::operator new(sizeof(Head) + size)) Head;
                std::memcpy(head->m_data, value.data(), size);
                return HeapString(head, 0, size);
            }
        }

        static HeapString concatenate(const std::string_view& a,
                                      const std::string_view& b) {
            const auto size = a.size() + b.size();
            if (!size) [[unlikely]]
                return HeapString();
            if (size < sizeof(HeapString)) {
                char buf[sizeof(HeapString)];
                copy2(buf, a, b);
                return HeapString(buf, size);
            } else {
                const auto head =
                    new (::operator new(sizeof(Head) + size)) Head;
                copy2(head->m_data, a, b);
                return HeapString(head, 0, size);
            }
        }

        HeapString& operator=(HeapString other) noexcept {
            this->~HeapString();
            return *new (this) HeapString(std::move(other));
        }

        bool operator==(const HeapString& other) const noexcept {
            return value() == other.value();
        }

        auto operator<=>(const HeapString& other) const noexcept {
            return value() <=> other.value();
        }

        bool operator==(const std::string_view& other) const noexcept {
            return value() == other;
        }

        auto operator<=>(const std::string_view& other) const noexcept {
            return value() <=> other;
        }

        operator std::string_view() const noexcept { return value(); }

        bool empty() const noexcept { return m_data == 0; }

        const char* data() const noexcept {
            if (isHeap()) {
                return m_data ? asHeap()->m_data + m_offset : nullptr;
            } else {
                return asSmall();
            }
        }

        size_t size() const noexcept { return isHeap() ? m_size : smallSize(); }

        const char* begin() const noexcept { return data(); }

        const char* end() const noexcept {
            if (isHeap()) {
                return m_data ? asHeap()->m_data + (m_offset + m_size)
                              : nullptr;
            } else {
                return asSmall() + smallSize();
            }
        }

        HeapString substring(size_t offset) const noexcept {
            assert(offset <= size());
            return substring(offset, size() - offset);
        }

        HeapString substring(size_t offset, size_t count) const noexcept {
            assert(offset + count <= size());
            if (!count) [[unlikely]]
                return HeapString();
            const char* src;
            if (isHeap()) {
                const auto head = asHeap();
                if (count < sizeof(HeapString)) {
                    src = head->m_data + m_offset;
                } else {
                    head->acquire();
                    return HeapString(head, m_offset + offset, count);
                }
            } else {
                src = asSmall();
            }
            return HeapString(src + offset, count);
        }

        std::string_view value() const noexcept {
            if (isHeap()) {
                return m_data ? std::string_view(asHeap()->m_data + m_offset,
                                                 m_size)
                              : std::string_view();
            } else {
                return std::string_view(asSmall(), smallSize());
            }
        }

    private:
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

        HeapString(const char* data, size_t size) noexcept
            : m_data((size << 1) | 1u) {
            std::memcpy(asSmall(), data, size);
        }

        HeapString(Head* head, size_t offset, size_t size) noexcept
            : m_data(convLE<std::endian::native>(
                  reinterpret_cast<uintptr_t>(head))),
              m_offset(offset), m_size(size) {}

        bool isHeap() const noexcept { return !(m_data & 1u); }

        Head* asHeap() const noexcept {
            return reinterpret_cast<Head*>(convLE<std::endian::native>(m_data));
        }

        char* asSmall() noexcept {
            return reinterpret_cast<char*>(&m_data) + 1;
        }

        const char* asSmall() const noexcept {
            return reinterpret_cast<const char*>(&m_data) + 1;
        }

        size_t smallSize() const noexcept { return (m_data & 0xFFu) >> 1; }

        // Use little-endian so the sentinel bit(1) is on the first byte.
        template<std::endian E>
        static uintptr_t convLE(uintptr_t p) noexcept {
            if constexpr (E == std::endian::little) {
                return p;
            } else {
                static_assert(E == std::endian::big);
                return std::byteswap(p);
            }
        }

        static void copy2(char* p, const std::string_view& a,
                          const std::string_view& b) noexcept {
            std::memcpy(p, a.data(), a.size());
            std::memcpy(p + a.size(), b.data(), b.size());
        }

        uintptr_t m_data = 0;
        unsigned m_offset = 0;
        unsigned m_size = 0;
    };

    inline HeapString createString(const std::string_view& value) {
        return HeapString::create(value);
    }

    inline HeapString concatenateString(const std::string_view& a,
                                        const std::string_view& b) {
        return HeapString::concatenate(a, b);
    }
} // namespace plutobook