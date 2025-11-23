#pragma once

#include <algorithm>
#include <cstdint>
#include <cassert>
#include <atomic>

namespace plutobook {
    template<typename T>
    class RefCounted {
    public:
        RefCounted() = default;

        void ref() { ++m_refCount; }
        void deref() {
            if (--m_refCount == 0) {
                delete static_cast<T*>(this);
            }
        }

        uint32_t refCount() const { return m_refCount; }
        bool hasOneRefCount() const { return m_refCount == 1; }

    private:
        RefCounted(const RefCounted&) = delete;
        RefCounted& operator=(const RefCounted&) = delete;
        std::atomic_uint32_t m_refCount{1};
    };

    template<typename T>
    inline void refIfNotNull(T* ptr) {
        if (ptr) {
            ptr->ref();
        }
    }

    template<typename T>
    inline void derefIfNotNull(T* ptr) {
        if (ptr) {
            ptr->deref();
        }
    }

    template<typename T>
    class RefPtr;
    template<typename T>
    RefPtr<T> adoptPtr(T*);

    template<typename T>
    class RefPtr {
    public:
        RefPtr() = default;
        RefPtr(std::nullptr_t) : m_ptr(nullptr) {}
        RefPtr(T* ptr) : m_ptr(ptr) { refIfNotNull(m_ptr); }
        RefPtr(T& ref) : m_ptr(&ref) { m_ptr->ref(); }
        RefPtr(const RefPtr& p) : m_ptr(p.get()) { refIfNotNull(m_ptr); }
        RefPtr(RefPtr&& p) : m_ptr(p.release()) {}

        template<typename U>
        RefPtr(const RefPtr<U>& p) : m_ptr(p.get()) {
            refIfNotNull(m_ptr);
        }

        template<typename U>
        RefPtr(RefPtr<U>&& p) : m_ptr(p.release()) {}

        ~RefPtr() { derefIfNotNull(m_ptr); }

        T* get() const { return m_ptr; }
        T& operator*() const {
            assert(m_ptr);
            return *m_ptr;
        }

        T* operator->() const {
            assert(m_ptr);
            return m_ptr;
        }

        operator T*() const { return m_ptr; }
        operator bool() const { return !!m_ptr; }

        RefPtr& operator=(std::nullptr_t) {
            clear();
            return *this;
        }

        RefPtr& operator=(T* o) {
            RefPtr p = o;
            swap(p);
            return *this;
        }

        RefPtr& operator=(T& o) {
            RefPtr p = o;
            swap(p);
            return *this;
        }

        RefPtr& operator=(const RefPtr& o) {
            RefPtr p = o;
            swap(p);
            return *this;
        }

        RefPtr& operator=(RefPtr&& o) {
            RefPtr p = std::move(o);
            swap(p);
            return *this;
        }

        template<typename U>
        RefPtr& operator=(const RefPtr<U>& o) {
            RefPtr p = o;
            swap(p);
            return *this;
        }

        template<typename U>
        RefPtr& operator=(RefPtr<U>&& o) {
            RefPtr p = std::move(o);
            swap(p);
            return *this;
        }

        void swap(RefPtr& o) { std::swap(m_ptr, o.m_ptr); }

        T* release() {
            T* ptr = m_ptr;
            m_ptr = nullptr;
            return ptr;
        }

        void clear() {
            derefIfNotNull(m_ptr);
            m_ptr = nullptr;
        }

        template<typename U>
        bool operator==(const RefPtr<U>& o) const {
            return m_ptr == o.get();
        }

        template<typename U>
        bool operator!=(const RefPtr<U>& o) const {
            return m_ptr != o.get();
        }

        template<typename U>
        bool operator<(const RefPtr<U>& o) const {
            return m_ptr < o.get();
        }

        template<typename U>
        bool operator>(const RefPtr<U>& o) const {
            return m_ptr > o.get();
        }

        template<typename U>
        bool operator<=(const RefPtr<U>& o) const {
            return m_ptr <= o.get();
        }

        template<typename U>
        bool operator>=(const RefPtr<U>& o) const {
            return m_ptr >= o.get();
        }

        friend RefPtr adoptPtr<T>(T*);

    private:
        RefPtr(T* ptr, std::nullptr_t) : m_ptr(ptr) {}
        T* m_ptr{nullptr};
    };

    template<typename T>
    inline RefPtr<T> adoptPtr(T* ptr) {
        return RefPtr<T>(ptr, nullptr);
    }

    template<class T>
    inline void swap(RefPtr<T>& a, RefPtr<T>& b) {
        a.swap(b);
    }

    template<typename T, typename U>
    inline bool operator==(const RefPtr<T>& a, const U* b) {
        return a.get() == b;
    }

    template<typename T, typename U>
    inline bool operator==(const T* a, const RefPtr<U>& b) {
        return a == b.get();
    }

    template<typename T>
    inline bool operator==(const RefPtr<T>& a, std::nullptr_t) {
        return a.get() == nullptr;
    }

    template<typename T>
    inline bool operator==(std::nullptr_t, const RefPtr<T>& a) {
        return nullptr == a.get();
    }

    template<typename T, typename U>
    inline bool operator!=(const RefPtr<T>& a, const U* b) {
        return a.get() != b;
    }

    template<typename T, typename U>
    inline bool operator!=(const T* a, const RefPtr<U>& b) {
        return a != b.get();
    }

    template<typename T>
    inline bool operator!=(const RefPtr<T>& a, std::nullptr_t) {
        return a.get() != nullptr;
    }

    template<typename T>
    inline bool operator!=(std::nullptr_t, const RefPtr<T>& a) {
        return nullptr != a.get();
    }

    template<typename T>
    inline bool is(const typename T::ClassRoot& value) {
        if constexpr (std::is_final_v<T>) {
            return T::classKind == value.type();
        } else {
            struct Checker {
                bool operator()(T* p) const noexcept { return true; }
                bool operator()(void*) const noexcept { return false; }
            };
            return visit(const_cast<typename T::ClassRoot*>(&value), Checker{});
        }
    }

    template<typename T, typename U>
    inline bool is(U* value) {
        return value && is<T>(*value);
    }

    template<typename T, typename U>
    inline bool is(const RefPtr<U>& value) {
        return value && is<T>(*value);
    }

    template<typename T, typename U>
    inline T& to(U& value) {
        assert(is<T>(value));
        return static_cast<T&>(value);
    }

    template<typename T, typename U>
    inline const T& to(const U& value) {
        assert(is<T>(value));
        return static_cast<const T&>(value);
    }

    template<typename T, typename U>
    inline T* to(U* value) {
        if (!is<T>(value))
            return nullptr;
        return static_cast<T*>(value);
    }

    template<typename T, typename U>
    inline const T* to(const U* value) {
        if (!is<T>(value))
            return nullptr;
        return static_cast<const T*>(value);
    }

    template<typename T, typename U>
    inline RefPtr<T> to(RefPtr<U>& value) {
        if (!is<T>(value))
            return nullptr;
        return static_cast<T&>(*value);
    }

    template<typename T, typename U>
    inline RefPtr<T> to(const RefPtr<U>& value) {
        if (!is<T>(value))
            return nullptr;
        return static_cast<T&>(*value);
    }
} // namespace plutobook