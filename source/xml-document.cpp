#include "xml-document.h"
#include "xml-parser.h"

namespace plutobook {

std::unique_ptr<XmlDocument> XmlDocument::create(Context* context, ResourceFetcher* fetcher, Url baseUrl)
{
    return std::unique_ptr<XmlDocument>(new XmlDocument(ClassKind::XmlDocument, context, fetcher, std::move(baseUrl)));
}

bool XmlDocument::parse(std::string_view content)
{
    return XmlParser(this).parse(content);
}

XmlDocument::XmlDocument(ClassKind type, Context* context, ResourceFetcher* fetcher, Url baseUrl)
    : Document(type, context, fetcher, std::move(baseUrl))
{
}

} // namespace plutobook
