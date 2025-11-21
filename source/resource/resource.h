#pragma once

#include "pointer.h"
#include "heap-string.h"

namespace plutobook {
    class Resource : public HeapMember, public RefCounted<Resource> {
    public:
        enum class Type { Text, Image, Font };
        using ClassRoot = Resource;
        using ClassKind = Type;

        virtual ~Resource() = default;
        ClassKind type() const noexcept { return m_type; }

    protected:
        explicit Resource(ClassKind type) : m_type(type) {}

        ClassKind m_type;
    };

    class Url;
    class ResourceData;
    class ResourceFetcher;

    class ResourceLoader {
    public:
        static ResourceData loadUrl(const Url& url,
                                    ResourceFetcher* customFetcher = nullptr);
        static Url completeUrl(const std::string_view& value);
        static Url baseUrl();
    };
} // namespace plutobook