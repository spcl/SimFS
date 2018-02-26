/*------------------------------------------------------------------------------
 * CppToolbox: StringHelper
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#include "StringHelper.h"

#include <cstring>
#include <iostream>

namespace toolbox {

void StringHelper::splitStr(std::vector<std::string> *v, const std::string &s, const std::string &delimiter) {
    // mutable char[]
    unsigned long len = s.length() + 1;
    char buf[len + 1];
    char *next = &buf[0];
    strncpy(buf, s.c_str(), len);
    buf[len] = 0;

    splitCStr(v, next, delimiter);
}

void StringHelper::splitCStr(std::vector<std::string> *v, char *s, const std::string &delimiter) {
    char *next = s;
    while (next != nullptr) {
        char *current = strsep(&next, delimiter.c_str());
        std::string token(current);
        v->push_back(token);
    }
}

void StringHelper::extendParameterMap(std::unordered_map<std::string, std::string> *p, const std::string &s,
                                      const std::string &delimiter, const std::string &binder) {
    std::vector<std::string> splits;
    splitStr(&splits, s, delimiter);
    for (const auto &item : splits) {
        std::vector<std::string> def;
        splitStr(&def, item, binder);
        if (def.size() == 2) {
            (*p)[def[0]] = def[1];
        }
    }
}

std::string StringHelper::joinPath(const std::string &path, const std::string &filename) {
    if (path.back() == '/') {
        return path + filename;
    } else {
        return path + "/" + filename;
    }
}


// this method was implemented by following http://stackoverflow.com/a/14678946 by Czarek Tomczak
// and empty check for search string as mentioned in comment by iOS-programmer
// with additional modifications
void StringHelper::replaceAllStringsInPlace(std::string *subject, const std::string& search, const std::string& replace) {
    if (subject == nullptr || (*subject).empty() || search.empty()) {
        return;
    }

    size_t pos = 0;
    while( (pos = (*subject).find(search, pos)) != std::string::npos) {
        (*subject).replace(pos, search.length(), replace);
        pos += replace.length();
    }
}
}
