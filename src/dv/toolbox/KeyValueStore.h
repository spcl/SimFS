/*------------------------------------------------------------------------------
 * CppToolbox: KeyValueStore
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#ifndef TOOLBOX_KEYVALUESTORE_H_
#define TOOLBOX_KEYVALUESTORE_H_

#include <functional>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>


namespace toolbox {

	/**
	 * Implements a simple key value store. Features
	 * - reading/writing files
	 * - reading/writing strings
	 * - lookup for strings, int types, double incl. range check default value, and error message
	 *
	 * the import functions append to the store; use clear() before, if a fresh store is needed
	 *
	 * keys are case sensitive
	 *
	 * bool replies: true for success, false for failure
	 *
	 * note: any self-defined error message handler can be used as long as it matches
	 * the defined type.
	 *
	 * v1.0 2017-05-01 Pirmin Schmid
	 */
	class KeyValueStore {
	public:
		typedef std::function<void(const std::string &)> error_message_handler_type;

		KeyValueStore() {}

		void clear();

		const std::unordered_map<std::string, std::string> &getStoreMap() const;

		//--- import/export ----------------------------------------------------

		void extendMap(const std::unordered_map<std::string, std::string> &map);

		void fromString(const std::string &str,
						const std::string &delimiter = ";",
						const std::string &binder = "=");

		std::string toString(const std::string &delimiter = ";",
							 const std::string &binder = "=") const;

		bool fromFile(const std::string &filename,
					  const std::string &delimiter = "\n",
					  const std::string &binder = "=");

		bool toFile(const std::string &filename,
					const std::string &delimiter = "\n",
					const std::string &binder = "=") const;


		//--- access -----------------------------------------------------------

		bool hasKey(const std::string &key) const;

		std::string getString(const std::string &key, error_message_handler_type emh = emDoNothing) const;

		void setString(const std::string &key, const std::string &value);

		double getDouble(const std::string &key,
						 double min = std::numeric_limits<double>::min(),
						 double max = std::numeric_limits<double>::max(),
						 double default_value = 0.0,
						 error_message_handler_type emh = emDoNothing) const;

		void setDouble(const std::string &key, double value);

		template <typename T>
		T getInt(const std::string &key,
				 T min = std::numeric_limits<T>::min(),
				 T max = std::numeric_limits<T>::max(),
				 T default_value = 0,
				 error_message_handler_type emh = emDoNothing) const {

			auto it = store_.find(key);
			if (it == store_.end()) {
				emh("key '" + key + "'not found");
				return default_value;
			}

			T r = default_value;
			try {
				r = static_cast<T>(std::stoll(it->second));
			}   catch (const std::invalid_argument& ia) {
				emh("value of '" + key + "' is not of integer type");
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

		template <typename T>
		void setInt(const std::string &key, T value) {
			store_[key] = std::to_string(value);
		}

		//--- pre-defined error message handlers -------------------------------

		static void emDoNothing(const std::string &message);
		static void emToCout(const std::string &message);
		static void emToCerr(const std::string &message);
		static void emToCoutAndExit(const std::string &message);
		static void emToCerrAndExit(const std::string &message);

	private:
		std::unordered_map<std::string, std::string> store_;
	};

}


#endif //TOOLBOX_KEYVALUESTORE_H_
