#ifndef PLUTOBOOK_HtmlENTITYPARSER_H
#define PLUTOBOOK_HtmlENTITYPARSER_H

#include <string>
#include <cstdint>

namespace plutobook {

class HtmlEntityParser {
public:
    HtmlEntityParser(const std::string_view& input, std::string& output, bool inAttributeValue)
        : m_input(input), m_output(output), m_inAttributeValue(inAttributeValue)
    {}

    size_t offset() const { return m_offset; }

    bool parse();

private:
    bool handleNamed(char cc);
    bool handleNumber(char cc);
    bool handleDecimal(char cc);
    bool handleMaybeHex(char cc);
    bool handleHex(char cc);
    void append(uint32_t cp);

    char currentInputCharacter() const;
    char nextInputCharacter();

    std::string_view m_input;
    std::string& m_output;
    bool m_inAttributeValue;
    size_t m_offset{0};
};

inline char HtmlEntityParser::currentInputCharacter() const
{
    if(m_offset < m_input.length())
        return m_input[m_offset];
    return 0;
}

inline char HtmlEntityParser::nextInputCharacter()
{
    m_offset += 1;
    if(m_offset < m_input.length())
        return m_input[m_offset];
    return 0;
}

} // namespace plutobook

#endif // PLUTOBOOK_HtmlENTITYPARSER_H
