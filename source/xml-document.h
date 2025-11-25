#pragma once

#include "document.h"

namespace plutobook {
    class XmlDocument : public Document {
    public:
        static std::unique_ptr<XmlDocument>
        create(Book* book, ResourceFetcher* fetcher, Url baseUrl);

        bool parse(const std::string_view& content) override;

    protected:
        XmlDocument(ClassKind type, Book* book, ResourceFetcher* fetcher,
                    Url baseUrl);
    };

    extern template bool is<XmlDocument>(const Node& value);
} // namespace plutobook