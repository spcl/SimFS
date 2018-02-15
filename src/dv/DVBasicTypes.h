//
// Created by Pirmin Schmid on 17.04.17.
//

#ifndef DV_TYPES_H_
#define DV_TYPES_H_


#include <cstdint>
#include <string>
#include <utility>


namespace dv {
	//--- used for DV *normalized* ids and counters
	typedef int64_t id_type;

	/**
	 * note: throws the same ia exception as std::stoi or std::stoll
	 * function here is just to have it match the type
	 */
	inline id_type stoid(const std::string &s) {
		return std::stoll(s);
	}

	typedef id_type counter_type;
	typedef id_type cost_type;
	typedef id_type size_type;


	//--- used by variable/dataset descriptions (dimensions and offsets/ranges)
	typedef int64_t dimension_type;

	/**
 	 * note: throws the same ia exception as std::stoi or std::stoll
 	 * function here is just to have it match the type
 	 */
	inline dimension_type stodim(const std::string &s) {
		return std::stoll(s);
	}


	typedef int64_t offset_type;

	/**
 	 * note: throws the same ia exception as std::stoi or std::stoll
 	 * function here is just to have it match the type
 	 */
	inline offset_type stooffset(const std::string &s) {
		return std::stoll(s);
	}

}

#endif //DV_TYPES_H_
