//
// Created by Pirmin Schmid on 12.04.17.
//

#ifndef DV_SIMULATORS_SIMULATOR_H_
#define DV_SIMULATORS_SIMULATOR_H_


#include <functional>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../DVBasicTypes.h"
#include "../DVForwardDeclarations.h"
#include "../toolbox/KeyValueStore.h"


namespace dv {

	class Simulator {
	public:
		Simulator(DV *dv_ptr);

		void incFilesCount(dv::counter_type increment);
		dv::counter_type getFilesCount() const;

		void incSimulationsCount();
		dv::counter_type getSimulationsCount() const;

		void scanRestartFiles();


		//--- file predicates & file/id_type conversion functions ------------------

		/**
		 * type == 0 -> file is not a checkpoint/result file
		 */
		dv::id_type getCheckpointFileType(const std::string &filename) const;
		dv::id_type getResultFileType(const std::string &filename) const;


		/**
		 * if file_type == 0 -> type will be determined first
		 * if type is known -> small performance advantage
		 */
		dv::id_type checkpoint2nr(const std::string &name, dv::id_type file_type = 0) const;
		dv::id_type result2nr(const std::string &name, dv::id_type file_type = 0) const;


		/**
		 * compares result files
		 */
		dv::id_type compare(const std::string &filename1, const std::string &filename2) const;

		/**
		 * used for new prefetching calculation
		 */
		dv::id_type getCheckpointNr(dv::id_type nr) const;
		dv::id_type getNextCheckpointNr(dv::id_type nr) const;

		/**
		 * for hotspot detection at the moment
		 */
		dv::id_type getPrevRestartDiff(const std::string &filename) const;
		dv::id_type getNextRestartDiff(const std::string &filename) const;


		/**
		 * returns the max. id value
		 */
		dv::id_type convertTrace(std::vector<dv::id_type> *output,
								  const std::vector<std::string> &input) const;

		dv::id_type partitionKey(const std::string &filename) const;

		dv::cost_type getCost(const std::string &filename) const;


		std::string getMetadataFilename(const std::string &filename) const;


		//--- job generation ---------------------------------------------------

		std::unique_ptr<SimJob> generateSimJob(dv::id_type appid,
											   dv::id_type target_nr,
											   std::unique_ptr<toolbox::KeyValueStore> parameters);

		std::unique_ptr<SimJob> generateSimJobForRange(dv::id_type appid,
													   dv::id_type target_begin_nr,
													   dv::id_type target_stop_nr,
													   std::unique_ptr<toolbox::KeyValueStore> parameters);

		//--- alpha and taus ---------------------------------------------------
		// TODO: may need adjustment after discussing how to handle
		//       these summary values; note: principal calculation
		//       is done in sim profiler in client descriptor

		void addAlphaAndTau(double alpha, double tau);

		void addAlpha(double alpha);
		void extendAlphas(const std::vector<double> &alphas);
		const std::vector<double> &getAlphas() const;

		/**
		 * returns the median
		 */
		double getAlpha() const;

		void addTau(double tau);
		void extendTaus(const std::vector<double> &taus);
		const std::vector<double> &getTaus() const;

		/**
		 * returns the median
		 */
		double getTau() const;

		const toolbox::KeyValueStore &getStatusSummary();

	private:
		DV *dv_ptr_;

		std::set<dv::id_type, std::greater<dv::id_type>> restarts_;
		// sets are implemented as balanced red/black tree in C++ std lib
		// however, bounds can only be got as
		// - lower bound (not <)
		// - upper bound (>)
		// for default comparator std::less.
		// and since we need to find the closest one
		// less than given value, we use the
		// set with greater comparator function
		// and then use the "lower bound",
		// whose (not >) then matches what we need: <=
		// use 0 in case nr cannot be found
		// see ../testing/test_set_boundaries.cpp


		std::set<dv::id_type> restarts_ceil_;
		// and we use a second set for the ceiling values
		// also here, lower_bound can be used for lookup for the Simulator::getNextRestartDiff function
		// but upper_bound has to be used for simstop lookup (to assure running for at least one full interval)
		// use the lookup nr in case it cannot be found


		std::vector<double> alphas_;
		std::vector<double> taus_;

		dv::counter_type files_ = 0;
		dv::counter_type simulations_ = 0;

		toolbox::KeyValueStore statusSummary_;
	};

}

#endif //DV_SIMULATORS_SIMULATOR_H_
