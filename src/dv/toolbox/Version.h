/*------------------------------------------------------------------------------
 * CppToolbox: Version
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#ifndef TOOLBOX_VERSION_H_
#define TOOLBOX_VERSION_H_

#include <ostream>
#include <string>

namespace toolbox {

	class Version {
	public:
		Version(const std::string &name, int major, int minor, int patch, const std::string &date) :
				name_(name), major_(major), minor_(minor), patch_(patch), date_(date) {}

		void print(std::ostream *out);

	private:
		std::string name_;
		int major_;
		int minor_;
		int patch_;
		std::string date_;
	};

}

#endif //TOOLBOX_VERSION_H_
