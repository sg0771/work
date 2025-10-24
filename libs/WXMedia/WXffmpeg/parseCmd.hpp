#include <cstdio>
#include <string>
#include <vector>

bool ParseChar(const std::wstring& buf, std::size_t& pos, wchar_t& ch, bool& escape)
{
    wchar_t c;

    if (pos == buf.length())
        return false;

    // Get the character to parse
    c = buf.at(pos++);

    if (c == L'\\')
    {
        // Parse the escape character

        if (pos != buf.length())
        {
            // Get the character to escape
            c = buf.at(pos++);

            if (c == L'\\' || c == L'"')
            {
                ch = c;
                escape = true;
            }
            else
            {
                // Does not support the character, just hold the '\\' character
                //  We need move the POS back to prepare for the character parsing
                pos--;
                ch = L'\\';
                escape = false;
            }
        }
        else
        {
            // We can't get the character to escape
            //  Just hold the '\\' character
            ch = c;
            escape = false;
        }
    }
    else
    {
        // Copy the character

        ch = c;
        escape = false;
    }

    return true;
}

bool ParseToken(const std::wstring& buf, std::size_t& pos, std::wstring& token)
{
    wchar_t c{};
    bool escape{};
    bool quote{};        // True if parsing a string
    bool doing{};        // True if parsing has started

    // Skip blank characters, if any
    while (pos != buf.length())
    {
        c = buf.at(pos);

        if (c != L' ' && c != L'\t')
            break;

        pos++;
    }
    // Clean up the token
    token.clear();

    while (ParseChar(buf, pos, c, escape))
    {
        if (!doing)
        {
            // Parse the first character

            if (c == L'"' && !escape)
            {
                // Just mark the beginning of the string, don't copy it
                quote = true;
            }
            else
            {
                // Copy the first character of the token
                token.push_back(c);
            }

            // '\n' is a single character token
            if (c == L'\n')
                return true;

            // We have parsed any one character, the parsing has started
            doing = true;
        }
        else
        {
            if (quote)
            {
                // Copying the character of the string here

                if (c == L'"' && !escape)
                {
                    // Mark the ending of a string
                    return true;
                }
                else
                {
                    // Copy the character of the string
                    token.push_back(c);
                }
            }
            else
            {
                // Copying the character of the token here

                if (c == L'"' && !escape)
                {
                    // We accidentally encounter a string beginning mark before the token finished
                    //  We need to finish the token and move the POS back to prepare for the string parsing
                    pos--;
                    return true;
                }

                if (c == L'\n')
                {
                    // We accidentally encounter a '\n' before the token finished
                    //  We need to finish the token and move the POS back to prepare for the '\n' parsing
                    pos--;
                    return true;
                }

                if (c == L' ' || c == L'\t')
                {
                    // Mark the ending of a string
                    return true;
                }
                else
                {
                    // Copy the character of the token
                    token.push_back(c);
                }
            }
        }
    }

    // If no any characters are parsed, we are at the end of the buffer
    //  returns 'false' to finish the parsing
    return doing;
}