/**
 * Copyright (c) 2022 Alexander Lunsford
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/

#ifndef TEXT_UTIL_H
#define TEXT_UTIL_H

#include "raylib.h"

#include <string>
#include <initializer_list>
#include <filesystem>
namespace fs = std::filesystem;

inline std::string BuildPath(std::initializer_list<std::string> components) {
    std::string output;
    for (const std::string& s : components) {
        output += s;
        if (s != *(components.end()-1) && s[s.length() - 1] != '/') {
            output += '/';
        }
    }
    return output;
}

//Returns the approximate width, in pixels, of a string written in the given font.
//Based off of Raylib's DrawText functions
inline int GetStringWidth(Font font, float fontSize, const std::string &string)
{
    float scaleFactor = fontSize / font.baseSize;
    int maxWidth = 0;
    int lineWidth = 0;
    for (int i = 0; i < string.size();)
    {
        int codepointByteCount = 0;
        int codepoint = GetCodepoint(&string[i], &codepointByteCount);
        int g = GetGlyphIndex(font, codepoint);

        if (codepoint == 0x3f) codepointByteCount = 1;

        if (codepoint == '\n')
        {
            maxWidth = Max(lineWidth, maxWidth);
            lineWidth = 0;
        }
        else 
        {
            if (font.glyphs[g].advanceX == 0) lineWidth += ((float)font.recs[g].width*scaleFactor);
            else lineWidth += ((float)font.glyphs[g].advanceX*scaleFactor);
        }

        i += codepointByteCount;
    }
    maxWidth = Max(lineWidth, maxWidth);
    return maxWidth;
}

#endif
