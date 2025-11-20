#ifndef PLUTOBOOK_HEAPSTRING_H
#define PLUTOBOOK_HEAPSTRING_H

#include <string_view>
#include <memory_resource>
#include <ostream>
#include <cstring>

namespace plutobook {

class HeapString : public std::string_view {
public:
    HeapString() = default;

    const char* begin() const { return data(); }
    const char* end() const { return data() + size(); }

    HeapString substring(size_t offset) const { return substr(offset); }
    HeapString substring(size_t offset, size_t count) const { return substr(offset, count); }

    const std::string_view& value() const { return *this; }

protected:
    HeapString(const std::string_view& value) : std::string_view(value) {}
    friend class Heap;
};

class Heap : public std::pmr::monotonic_buffer_resource {
public:
    explicit Heap(size_t capacity) : monotonic_buffer_resource(capacity) {}

    HeapString createString(const std::string_view& value);
    HeapString concatenateString(const std::string_view& a, const std::string_view& b);
};

inline HeapString Heap::createString(const std::string_view& value)
{
    auto content = static_cast<char*>(allocate(value.size(), alignof(char)));
    std::memcpy(content, value.data(), value.size());
    return HeapString({content, value.size()});
}

inline HeapString Heap::concatenateString(const std::string_view& a, const std::string_view& b)
{
    auto content = static_cast<char*>(allocate(a.size() + b.size(), alignof(char)));
    std::memcpy(content, a.data(), a.size());
    std::memcpy(content + a.size(), b.data(), b.size());
    return HeapString({content, a.size() + b.size()});
}

class HeapMember {
public:
    HeapMember() = default;

    static void* operator new(size_t size, Heap* heap) { return heap->allocate(size); }
    static void* operator new[](size_t size, Heap* heap) { return heap->allocate(size); }

    static void operator delete(void* data, Heap* heap) {}
    static void operator delete[](void* data, Heap* heap) {}

    static void operator delete(void* data) {}
    static void operator delete[](void* data) {}

private:
    HeapMember(const HeapMember&) = delete;
    HeapMember& operator=(const HeapMember&) = delete;
};

} // namespace plutobook

#endif // PLUTOBOOK_HEAPSTRING_H
