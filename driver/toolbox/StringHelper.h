/*------------------------------------------------------------------------------
 * CppToolbox: StringHelper
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#ifndef TOOLBOX_STRINGHELPER_H_
#define TOOLBOX_STRINGHELPER_H_


#include <string>
#include <unordered_map>
#include <vector>

namespace toolbox {

	class StringHelper {
	public:
		static void splitStr(std::vector<std::string> *v, const std::string &s, const std::string &delimiter);

		// note: s gets modified during processing
		static void splitCStr(std::vector<std::string> *v, char *s, const std::string &delimiter);


		static void extendParameterMap(std::unordered_map<std::string, std::string> *p, const std::string &s,
									   const std::string &delimiter = ";", const std::string &binder = "=");


		static std::string joinPath(const std::string &path, const std::string &filename);


		static void replaceAllStringsInPlace(std::string *subject, const std::string& search, const std::string& replace);


		template <typename T>
		static T str2int(const std::string &str, T min, T max, T default_value) {
			T r = 0;
			try {
				r = static_cast<T>(std::stoll(str));
			}   catch (const std::invalid_argument& ia) {
				return default_value;
			}
			if (r < min || max < r) {
				return default_value;
			}
			return r;
		}

		template <typename T>
		static T str2float(const std::string &str, T min, T max, T default_value) {
			T r = 0.0;
			try {
				r = static_cast<T>(std::stod(str));
			}   catch (const std::invalid_argument& ia) {
				return default_value;
			}
			if (r < min || max < r) {
				return default_value;
			}
			return r;
		}
	};

}

#endif //TOOLBOX_STRINGHELPER_H_
