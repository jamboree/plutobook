#pragma once

#include "output-stream.h"

#include <string>
#include <boost/container_hash/hash.hpp>

namespace plutobook {
    class Url {
    public:
        Url() = default;
        explicit Url(const std::string_view& input);

        Url complete(std::string_view input) const;

        bool protocolIs(const std::string_view& protocol) const;
        bool isHierarchical() const {
            return m_schemeEnd < m_userBegin && m_value[m_schemeEnd + 1] == '/';
        }
        bool isEmpty() const { return m_value.empty(); }

        const std::string& value() const { return m_value; }

        std::string_view base() const { return componentString(0, m_baseEnd); }
        std::string_view path() const {
            return componentString(m_portEnd, m_pathEnd);
        }
        std::string_view query() const {
            return componentString(m_pathEnd, m_queryEnd);
        }
        std::string_view fragment() const {
            return componentString(m_queryEnd, m_fragmentEnd);
        }

        friend std::size_t hash_value(const Url& key) {
            return boost::hash<std::string_view>{}(key.m_value);
        }

        bool operator==(const Url& other) const {
            return m_value == other.m_value;
        }

        auto operator<=>(const Url& other) const {
            return m_value <=> other.m_value;
        }

    private:
        std::string_view componentString(size_t begin, size_t end) const;
        std::string m_value;
        unsigned m_schemeEnd{0};
        unsigned m_userBegin{0};
        unsigned m_userEnd{0};
        unsigned m_passwordEnd{0};
        unsigned m_hostEnd{0};
        unsigned m_portEnd{0};
        unsigned m_baseEnd{0};
        unsigned m_pathEnd{0};
        unsigned m_queryEnd{0};
        unsigned m_fragmentEnd{0};
    };

    inline std::string_view Url::componentString(size_t begin,
                                                 size_t end) const {
        return std::string_view(m_value).substr(begin, end - begin);
    }

    inline OutputStream& operator<<(OutputStream& o, const Url& in) {
        return o << in.value();
    }
} // namespace plutobook