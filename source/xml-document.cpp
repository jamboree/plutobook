#include "xml-document.h"
#include "xml-parser.h"

namespace plutobook {

std::unique_ptr<XmlDocument> XmlDocument::create(Book* book, ResourceFetcher* fetcher, Url baseUrl)
{
    return std::unique_ptr<XmlDocument>(new XmlDocument(ClassKind::XmlDocument, book, fetcher, std::move(baseUrl)));
}

bool XmlDocument::parse(const std::string_view& content)
{
    return XmlParser(this).parse(content);
}

XmlDocument::XmlDocument(ClassKind type, Book* book, ResourceFetcher* fetcher, Url baseUrl)
    : Document(type, book, fetcher, std::move(baseUrl))
{
}

} // namespace plutobook
