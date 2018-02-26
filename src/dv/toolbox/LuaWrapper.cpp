/*------------------------------------------------------------------------------
 * CppToolbox: LuaWrapper
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#include <ctime>
#include <iostream>
#include "LuaWrapper.h"

namespace lua {

LuaWrapper::LuaWrapper() {
    L_ = luaL_newstate();
    luaL_openlibs(L_);

    // make callback functions available in Lua
    lua_register(L_, "cb_datestr2seconds", callback_datestr2seconds);
    lua_register(L_, "cb_seconds2datestr", callback_seconds2datestr);
}

LuaWrapper::~LuaWrapper() {
    if (L_ != nullptr) {
        lua_close(L_);
        L_ = nullptr;
    }
}

bool LuaWrapper::loadFile(const std::string &file_name) {
    int result = luaL_loadfile(L_, file_name.c_str());
    lua_pcall(L_, 0, 0, 0);
    return result == LUA_OK;
}

std::string LuaWrapper::getErrorString() {
    std::string r = lua_tostring(L_, -1);
    return r;
}

bool LuaWrapper::check(LuaWrapper::ApiType expected_type, const std::string &name) {
    lua_getglobal(L_, name.c_str());
    bool ok = false;
    switch(expected_type) {
    case kFunction:
        ok = lua_isfunction(L_, -1) != 0;
        break;

    case kInt:
        ok = lua_isinteger(L_, -1) != 0;
        break;

    case kDouble:
        ok = lua_isnumber(L_, -1) != 0;
        break;

    case kString:
        ok = lua_isstring(L_, -1) != 0;
        break;
    }

    lua_pop(L_, 1);
    return ok;
}

//--- variable access ------------------------------------------------------

std::string LuaWrapper::getString(const std::string &variable_name) {
    lua_getglobal(L_, variable_name.c_str());
    if (!lua_isstring(L_, -1)) {
        std::cerr << "LuaWrapper ERROR: string variable " << variable_name << " not found" << std::endl;
        return std::string("");
    }

    std::string r = lua_tostring(L_, -1);
    lua_pop(L_, 1);
    return r;
}

void LuaWrapper::setString(const std::string &variable_name, const std::string &value) {
    lua_pushstring(L_, value.c_str());
    lua_setglobal(L_, variable_name.c_str());
}

double LuaWrapper::getDouble(const std::string &variable_name) {
    lua_getglobal(L_, variable_name.c_str());
    if (!lua_isnumber(L_, -1)) {
        std::cerr << "LuaWrapper ERROR: double variable " << variable_name << " not found" << std::endl;
        return 0.0;
    }

    double r = static_cast<double>(lua_tonumber(L_, -1));
    lua_pop(L_, 1);
    return r;
}

void LuaWrapper::setDouble(const std::string &variable_name, double value) {
    lua_pushnumber(L_, value);
    lua_setglobal(L_, variable_name.c_str());
}

LuaWrapper::int_type LuaWrapper::getInt(const std::string &variable_name) {
    lua_getglobal(L_, variable_name.c_str());
    if (!lua_isinteger(L_, -1)) {
        std::cerr << "LuaWrapper ERROR: integer variable " << variable_name << " not found" << std::endl;
        return 0;
    }

    int_type r = static_cast<int_type>(lua_tointeger(L_, -1));
    lua_pop(L_, 1);
    return r;
}

void LuaWrapper::setInt(const std::string &variable_name, LuaWrapper::int_type value) {
    lua_pushinteger(L_, value);
    lua_setglobal(L_, variable_name.c_str());
}


//--- function calls -------------------------------------------------------

LuaWrapper::int_type LuaWrapper::callInt2Int(const std::string &function_name, LuaWrapper::int_type arg1) {
    lua_getglobal(L_, function_name.c_str());
    if (!lua_isfunction(L_, -1)) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << " not found" << std::endl;
        return 0;
    }

    lua_pushinteger(L_, arg1);
    if (lua_pcall(L_, 1, 1, 0) != 0) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << ": error during execution" << std::endl;
        return 0;
    }

    if (!lua_isinteger(L_, -1)) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << " did not return an integer value" << std::endl;
        return 0;
    }

    int_type r = lua_tointeger(L_, -1);
    lua_pop(L_, 1);

    return r;
}

std::string LuaWrapper::callString2String(const std::string &function_name, const std::string &arg1) {
    lua_getglobal(L_, function_name.c_str());
    if (!lua_isfunction(L_, -1)) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << " not found" << std::endl;
        return std::string("");
    }

    lua_pushstring(L_, arg1.c_str());
    if (lua_pcall(L_, 1, 1, 0) != 0) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << ": error during execution" << std::endl;
        return std::string("");
    }

    if (!lua_isstring(L_, -1)) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << " did not return a string value" << std::endl;
        return std::string("");
    }

    std::string r = lua_tostring(L_, -1);
    lua_pop(L_, 1);

    return r;
}

LuaWrapper::int_type LuaWrapper::callString2Int(const std::string &function_name, const std::string &arg1) {
    lua_getglobal(L_, function_name.c_str());
    if (!lua_isfunction(L_, -1)) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << " not found" << std::endl;
        return 0;
    }

    lua_pushstring(L_, arg1.c_str());
    if (lua_pcall(L_, 1, 1, 0) != 0) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << ": error during execution" << std::endl;
        return 0;
    }

    if (!lua_isinteger(L_, -1)) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << " did not return an integer value" << std::endl;
        return 0;
    }

    int_type r = lua_tointeger(L_, -1);
    lua_pop(L_, 1);

    return r;
}

LuaWrapper::int_type LuaWrapper::callStringInt2Int(const std::string &function_name, const std::string &arg1,
        LuaWrapper::int_type arg2) {
    lua_getglobal(L_, function_name.c_str());
    if (!lua_isfunction(L_, -1)) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << " not found" << std::endl;
        return 0;
    }

    lua_pushstring(L_, arg1.c_str());
    lua_pushinteger(L_, arg2);
    if (lua_pcall(L_, 2, 1, 0) != 0) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << ": error during execution" << std::endl;
        return 0;
    }

    if (!lua_isinteger(L_, -1)) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << " did not return an integer value" << std::endl;
        return 0;
    }

    int_type r = lua_tointeger(L_, -1);
    lua_pop(L_, 1);

    return r;
}

std::string LuaWrapper::callStringString2String(const std::string &function_name, const std::string &arg1,
        const std::string &arg2) {
    lua_getglobal(L_, function_name.c_str());
    if (!lua_isfunction(L_, -1)) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << " not found" << std::endl;
        return std::string("");
    }

    lua_pushstring(L_, arg1.c_str());
    lua_pushstring(L_, arg2.c_str());
    if (lua_pcall(L_, 2, 1, 0) != 0) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << ": error during execution" << std::endl;
        return std::string("");
    }

    if (!lua_isstring(L_, -1)) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << " did not return a string value" << std::endl;
        return std::string("");
    }

    std::string r = lua_tostring(L_, -1);
    lua_pop(L_, 1);

    return r;
}

std::string LuaWrapper::callStringIntInt2String(const std::string &function_name, const std::string &arg1,
        LuaWrapper::int_type arg2, LuaWrapper::int_type arg3) {
    lua_getglobal(L_, function_name.c_str());
    if (!lua_isfunction(L_, -1)) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << " not found" << std::endl;
        return std::string("");
    }

    lua_pushstring(L_, arg1.c_str());
    lua_pushinteger(L_, arg2);
    lua_pushinteger(L_, arg3);
    if (lua_pcall(L_, 3, 1, 0) != 0) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << ": error during execution" << std::endl;
        return std::string("");
    }

    if (!lua_isstring(L_, -1)) {
        std::cerr << "LuaWrapper ERROR: function " << function_name << " did not return a string value" << std::endl;
        return std::string("");
    }

    std::string r = lua_tostring(L_, -1);
    lua_pop(L_, 1);

    return r;
}

int callback_datestr2seconds(lua_State *state) {
    int args = lua_gettop(state);
    if (args != 2) {
        std::cerr << "callback_datestr2seconds(): ERROR: Need 2 arguments";
        lua_pushinteger(state, 0);
        return 1;
    }

    std::tm tm = {};
    char *r = strptime(lua_tostring(state, 1), lua_tostring(state, 2), &tm);
    if (r == nullptr) {
        std::cerr << "callback_datestr2seconds(): ERROR. cannot convert the date using given format string" << std::endl;
        lua_pushstring(state, "");
        return 1;
    }
    std::time_t t = std::mktime(&tm);
    // may be a float on some systems

    lua_pushinteger(state, static_cast<int64_t >(t));
    return 1; // number of arguments
}

int callback_seconds2datestr(lua_State *state) {
    int args = lua_gettop(state);
    if (args != 2) {
        std::cerr << "callback_seconds2datestr(): ERROR: Need 2 arguments" << std::endl;
        lua_pushstring(state, "");
        return 1;
    }

    std::time_t t = static_cast<std::time_t>(lua_tointeger(state, 1));
    std::tm *tm = std::localtime(&t);
    char buffer[256];
    std::size_t r = strftime(buffer, sizeof(buffer), lua_tostring(state, 2), tm);
    if (r == 0) {
        std::cerr << "callback_seconds2datestr(): ERROR. cannot convert the date using given format string" << std::endl;
        lua_pushstring(state, "");
        return 1;
    }

    lua_pushstring(state, buffer);
    return 1; // number of arguments
}

}
