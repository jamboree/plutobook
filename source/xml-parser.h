#pragma once

#include <string_view>

namespace plutobook {
    class XmlDocument;
    class ContainerNode;

    class XmlParser {
    public:
        explicit XmlParser(XmlDocument* document);

        bool parse(const std::string_view& content);

        void handleStartNamespace(const char* prefix, const char* uri);
        void handleEndNamespace(const char* prefix);

        void handleStartElement(const char* name, const char** attrs);
        void handleEndElement(const char* name);
        void handleCharacterData(const char* data, size_t length);

    private:
        XmlDocument* m_document;
        ContainerNode* m_currentNode;
    };
} // namespace plutobook