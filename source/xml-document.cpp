#include "xml-document.h"
#include "xml-parser.h"

namespace plutobook {

std::unique_ptr<XmlDocument> XmlDocument::create(Book* book, Heap* heap, ResourceFetcher* fetcher, Url baseUrl)
{
    return std::unique_ptr<XmlDocument>(new (heap) XmlDocument(ClassKind::XmlDocument, book, heap, fetcher, std::move(baseUrl)));
}

bool XmlDocument::parse(const std::string_view& content)
{
    return XmlParser(this).parse(content);
}

XmlDocument::XmlDocument(ClassKind type, Book* book, Heap* heap, ResourceFetcher* fetcher, Url baseUrl)
    : Document(type, book, heap, fetcher, std::move(baseUrl))
{
}

} // namespace plutobook
