#pragma once

#include <limits>
#include <optional>

namespace plutobook {
    template<class T>
    struct OptionalTrait;

    template<class T>
    struct OptionalTrait<T*> {
        static constexpr T* null = nullptr;
        static constexpr bool isNull(T* value) { return value == null; }
    };

    template<std::floating_point T>
    struct OptionalTrait<T> {
        static constexpr float null = std::numeric_limits<T>::quiet_NaN();

        static constexpr bool isNull(float value) { return value != value; }
    };

    template<class T>
    struct ExtremeTrait;

    template<std::signed_integral T>
    struct ExtremeTrait<T> {
        static constexpr T value = std::numeric_limits<T>::min();
    };

    template<std::unsigned_integral T>
    struct ExtremeTrait<T> {
        static constexpr T value = std::numeric_limits<T>::max();
    };

    template<std::integral T>
    struct OptionalTrait<T> {
        static constexpr T null = ExtremeTrait<T>::value;
        static constexpr bool isNull(T value) { return value == null; }
    };

    template<class T>
    concept Enum = std::is_enum_v<T>;

    template<Enum T>
    struct OptionalTrait<T> {
        static constexpr T null =
            T(ExtremeTrait<std::underlying_type_t<T>>::value);
        static constexpr bool isNull(T value) { return value == null; }
    };

    template<class T>
    struct Optional {
        Optional() = default;

        constexpr Optional(std::nullopt_t) {}

        template<class U = std::remove_cv_t<T>>
        constexpr Optional(U&& value) : m_value(std::forward<U>(value)) {}

        constexpr explicit operator bool() const noexcept {
            return !Trait::isNull(m_value);
        }

        constexpr bool has_value() const noexcept {
            return !Trait::isNull(m_value);
        }

        constexpr T& value() {
            if (Trait::isNull(m_value)) {
                throw std::bad_optional_access();
            }
            return m_value;
        }

        constexpr const T& value() const {
            if (Trait::isNull(m_value)) {
                throw std::bad_optional_access();
            }
            return m_value;
        }

        template<class U = std::remove_cv_t<T>>
        constexpr T value_or(U&& fallback) && {
            return Trait::isNull(m_value)
                       ? static_cast<T>(std::forward<U>(fallback))
                       : std::move(m_value);
        }

        template<class U = std::remove_cv_t<T>>
        constexpr T value_or(U&& fallback) const& {
            return Trait::isNull(m_value)
                       ? static_cast<T>(std::forward<U>(fallback))
                       : m_value;
        }

        constexpr T* operator->() noexcept {
            assert(has_value());
            return &m_value;
        }

        constexpr const T* operator->() const noexcept {
            assert(has_value());
            return &m_value;
        }

        constexpr T& operator*() noexcept {
            assert(has_value());
            return m_value;
        }

        constexpr T& operator*() const noexcept {
            assert(has_value());
            return m_value;
        }

        constexpr void reset() noexcept { m_value = Trait::null; }

        bool operator==(const Optional&) const = default;
        auto operator<=>(const Optional&) const = default;

    private:
        using Trait = OptionalTrait<T>;

        T m_value = Trait::null;
    };
} // namespace plutobook