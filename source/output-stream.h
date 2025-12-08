#pragma once

#include <charconv>
#include <concepts>
#include <string_view>

namespace plutobook {
    /**
     * @brief The OutputStream is an abstract base class for writing data to an
     * output stream.
     */
    class OutputStream {
    public:
        /**
         * @brief Writes data to the output stream.
         * @param data A pointer to the buffer containing the data to be
         * written.
         * @param length The length of the data to be written, in bytes.
         * @return the length of the data that was written successfully.
         */
        virtual size_t write(const char* data, size_t length) = 0;

        OutputStream& operator<<(char c) {
            write(&c, 1);
            return *this;
        }

        template<class T>
        OutputStream& operator<<(T val)
            requires std::integral<T> || std::floating_point<T>
        {
            char buf[64];
            const auto e = std::to_chars(buf, std::end(buf), val).ptr;
            write(buf, e - buf);
            return *this;
        }

        OutputStream& operator<<(std::string_view str) {
            write(str.data(), str.size());
            return *this;
        }
    };
} // namespace plutobook