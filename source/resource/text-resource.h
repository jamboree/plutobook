#pragma once

#include "resource.h"

#include <string>

namespace plutobook {
    class Document;

    class TextResource final : public Resource {
    public:
        static constexpr ClassKind classKind = ClassKind::Text;

        static RefPtr<TextResource> create(Document* document, const Url& url);
        static std::string_view decode(const char* data, size_t length,
                                       const std::string_view& mimeType,
                                       const std::string_view& textEncoding);
        static bool isXmlMimeType(const std::string_view& mimeType);
        const std::string& text() const { return m_text; }

    private:
        TextResource(const std::string_view& text)
            : Resource(classKind), m_text(text) {}
        std::string m_text;
    };
} // namespace plutobook