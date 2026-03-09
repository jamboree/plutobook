#pragma once

#include "document.h"

namespace plutobook {
    class XmlDocument : public Document {
    public:
        static std::unique_ptr<XmlDocument>
        create(Context* context, ResourceFetcher* fetcher, Url baseUrl);

        bool parse(std::string_view content) override;

    protected:
        XmlDocument(ClassKind type, Context* context, ResourceFetcher* fetcher,
                    Url baseUrl);
    };

    extern template bool is<XmlDocument>(const Node& value);
} // namespace plutobook