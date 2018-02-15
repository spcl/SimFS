/*------------------------------------------------------------------------------
 * CppToolbox: KeyValueStore
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#include "KeyValueStore.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include "StringHelper.h"

namespace toolbox {

	void KeyValueStore::clear() {
		store_.clear();
	}

	const std::unordered_map<std::string, std::string> &KeyValueStore::getStoreMap() const {
		return store_;
	}

	//--- import/export ----------------------------------------------------

	void KeyValueStore::extendMap(const std::unordered_map<std::string, std::string> &map) {
		for (const auto &item : map) {
			store_[item.first] = item.second;
		}
	}

	void KeyValueStore::fromString(const std::string &str,
								   const std::string &delimiter,
								   const std::string &binder) {
		StringHelper::extendParameterMap(&store_, str, delimiter, binder);
	}

	std::string KeyValueStore::toString(const std::string &delimiter,
										const std::string &binder) const {
		std::stringstream s;
		std::string current_delimiter = "";
		for (const auto &kv : store_) {
			s << current_delimiter << kv.first << binder << kv.second;
			current_delimiter = delimiter;
		}
		return s.str();
	}

	bool KeyValueStore::fromFile(const std::string &filename,
								 const std::string &delimiter,
								 const std::string &binder) {
		std::ifstream in;
		try {
			in.open(filename);
			std::stringstream buffer;
			buffer << in.rdbuf();
			StringHelper::extendParameterMap(&store_, buffer.str(), delimiter, binder);
			in.close();
		} catch (std::ifstream::failure e) {
			if (in.is_open()) {
				in.close();
			}
			return false;
		}

		return true;
	}

	bool KeyValueStore::toFile(const std::string &filename,
							   const std::string &delimiter,
							   const std::string &binder) const {

		std::string str = toString(delimiter, binder);
		std::ofstream out;
		try {
			out.open(filename);
			out << str;
			out.close();
		} catch (std::ifstream::failure e) {
			if (out.is_open()) {
				out.close();
			}
			return false;
		}

		return true;
	}


	//--- access -----------------------------------------------------------

	bool KeyValueStore::hasKey(const std::string &key) const {
		auto it = store_.find(key);
		return it != store_.end();
	}

	std::string KeyValueStore::getString(const std::string &key, KeyValueStore::error_message_handler_type emh) const {
		auto it = store_.find(key);
		if (it == store_.end()) {
			emh("key '" + key + "'not found");
			return "";
		}

		return it->second;
	}

	void KeyValueStore::setString(const std::string &key, const std::string &value) {
		store_[key] = value;
	}

	double KeyValueStore::getDouble(const std::string &key, double min, double max, double default_value,
									KeyValueStore::error_message_handler_type emh) const {
		auto it = store_.find(key);
		if (it == store_.end()) {
			emh("key '" + key + "'not found");
			return default_value;
		}

		double r = default_value;
		try {
			r = std::stod(it->second);
		}   catch (const std::invalid_argument& ia) {
			emh("value of '" + key + "' is not of floating point type");
			return default_value;
		}
		if (r < min || max < r) {
			emh("value of '" + key + "' " + std::to_string(r) +
				" is out of range [" + std::to_string(min) +
				"," + std::to_string(max) + "]");
			return default_value;
		}
		return r;
	}

	void KeyValueStore::setDouble(const std::string &key, double value) {
		store_[key] = std::to_string(value);
	}


	//--- pre-defined error message handlers -------------------------------

	void KeyValueStore::emDoNothing(const std::string &message) {}

	void KeyValueStore::emToCout(const std::string &message) {
		std::cout << message << std::endl;
	}

	void KeyValueStore::emToCerr(const std::string &message) {
		std::cerr << message << std::endl;
	}

	void KeyValueStore::emToCoutAndExit(const std::string &message) {
		std::cout << message << std::endl;
		exit(1);
	}

	void KeyValueStore::emToCerrAndExit(const std::string &message) {
		std::cerr << message << std::endl;
		exit(1);
	}
}
