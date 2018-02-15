/*------------------------------------------------------------------------------
 * CppToolbox: LuaWrapper
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#ifndef TOOLBOX_LUAWRAPPER_H_
#define TOOLBOX_LUAWRAPPER_H_

#include <cstdint>
#include <string>

#include "../lua/lua.hpp"

namespace lua {

	/**
	 * This class embeds the lua interpreter for easy access of variables and functions defined in Lua.
	 * Goal of this class: A light-weight wrapper around the Lua interpreter. Lua can then be used for
	 * configuration of constants and some functions of a larger C++ application.
	 * Lua code can use pre-defined callback functions written in C++ to give additional flexibility.
	 *
	 * The interface of the call and callback functions is just a starting point.
	 * It can easily be adjusted to the needs of the application.
	 *
	 * embedding follows the code examples of the Lua manual, with modifications.
	 * http://www.lua.org/manual
	 *
	 * v1.2 2017-05-01 / 2017-05-12 Pirmin Schmid
	 */
	class LuaWrapper {
	public:
		typedef int64_t int_type;

		LuaWrapper();

		~LuaWrapper();

		/**
		 * loads source code and byte code files
		 * @returns true in case of success; false otherwise
		 */
		bool loadFile(const std::string &file_name);

		std::string getErrorString();

		/**
		 * allows a restricted form of API validation of a provided
		 * Lua script
		 */
		enum ApiType { kFunction, kInt, kDouble, kString };

		bool check(ApiType expected_type, const std::string &name);

		//--- variable access ------------------------------------------------------

		std::string getString(const std::string &variable_name);

		void setString(const std::string &variable_name, const std::string &value);

		double getDouble(const std::string &variable_name);

		void setDouble(const std::string &variable_name, double value);

		int_type getInt(const std::string &variable_name);

		void setInt(const std::string &variable_name, int_type value);


		//--- various function calls -----------------------------------------------

		// 1 -> 1
		int_type callInt2Int(const std::string &function_name, int_type arg1);
		std::string callString2String(const std::string &function_name, const std::string &arg1);
		int_type callString2Int(const std::string &function_name, const std::string &arg1);

		// 2 -> 1
		int_type callStringInt2Int(const std::string &function_name, const std::string &arg1, int_type arg2);
		std::string callStringString2String(const std::string &function_name, const std::string &arg1,
											const std::string &arg2);

		// 3 -> 1
		std::string callStringIntInt2String(const std::string &function_name, const std::string &arg1,
											int_type arg2, int_type arg3);

		// extend function signatures as needed


	private:
		lua_State *L_ = nullptr;

	};


	//--- callback function calls: extend lua functionality --------------------

	/**
	 * input:  date string; format string
	 * output: integer
	 */
	extern "C" int callback_datestr2seconds(lua_State *L);

	/**
	 * input:  integer; format string
	 * output: date string
	 */
	extern "C" int callback_seconds2datestr(lua_State *L);

}


#endif //TOOLBOX_LUAWRAPPER_H_
