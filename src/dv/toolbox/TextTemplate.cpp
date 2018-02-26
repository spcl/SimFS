/*------------------------------------------------------------------------------
 * CppToolbox: TextTemplate
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#include "TextTemplate.h"

#include <fstream>
#include <sstream>

#include "StringHelper.h"

namespace toolbox {

TextTemplate::TextTemplate(const TextTemplate::Style style) : style_(style) {}

bool TextTemplate::readFile(const std::string &filename) {
    std::ifstream in;
    try {
        in.open(filename, std::ifstream::in);
        if (!in.good()) return false;
        std::stringstream buffer;
        buffer << in.rdbuf();
        content_ = buffer.str();
        in.close();
    } catch (std::ifstream::failure e) {
        if (in.is_open()) {
            in.close();
        }
        return false;
    }

    return true;
}

bool TextTemplate::writeFile(const std::string &filename) const {
    std::ofstream out;
    try {
        out.open(filename);
        out << content_;
        out.close();
    } catch (std::ifstream::failure e) {
        if (out.is_open()) {
            out.close();
        }
        return false;
    }

    return true;
}

void TextTemplate::setContent(const std::string &content) {
    content_ = content;
}

const std::string &TextTemplate::getContent() const {
    return content_;
}

void TextTemplate::print(std::ostream *out) const {
    *out << content_;
}

void TextTemplate::substituteWith(const toolbox::KeyValueStore &substitutions) {
    switch (style_) {
    case kDoubleBracketsStyle:
        substituteWith_DoubleBracketsStyle(substitutions);
        break;
    case kDollarSignStyle:
        substituteWith_PythonDollarStyle(substitutions);
        break;
    }
}

void TextTemplate::substituteWith_DoubleBracketsStyle(
    const toolbox::KeyValueStore &substitutions) {

    for (const auto &r : substitutions.getStoreMap()) {
        if (r.first.empty()) {
            continue;
        }

        StringHelper::replaceAllStringsInPlace(&content_, "{{" + r.first + "}}", r.second);
    }
}

void TextTemplate::substituteWith_PythonDollarStyle(const toolbox::KeyValueStore &substitutions) {

    // handle ${identifier} type
    for (const auto &r : substitutions.getStoreMap()) {
        if (r.first.empty()) {
            continue;
        }

        StringHelper::replaceAllStringsInPlace(&content_, "${" + r.first + "}", r.second);
    }

    // handle $identifier type
    for (const auto &r : substitutions.getStoreMap()) {
        if (r.first.empty()) {
            continue;
        }

        StringHelper::replaceAllStringsInPlace(&content_, "$" + r.first, r.second);
    }

    // additional replacement of $$ escape
    StringHelper::replaceAllStringsInPlace(&content_, "$$", "$");
}

}
