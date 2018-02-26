// 
// 01/2018: Prefetching (SDG)
// 09/2017: Adding support for range queries (only stubs) (PS)
// 04/2017: Porting/rewriting from SDG's python version (PS)
//



#ifndef DV_SERVER_CLIENTDESCRIPTOR_H_
#define DV_SERVER_CLIENTDESCRIPTOR_H_

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "../DVBasicTypes.h"
#include "../DVForwardDeclarations.h"
#include "Profiler.h"
#include "PrefetchContext.h"



namespace dv {

	class ClientDescriptor {
        friend class PrefetchContext;

	public:
		static constexpr dv::id_type kRequestRangeFlagFirst = 1;
		static constexpr dv::id_type kRequestRangeFlagLast = 2;

		class RangeRequest {
		public:
			RangeRequest(const std::string &begin, const std::string &end, dv::id_type stride)
					: begin_(begin), end_(end), stride_(stride) {}

			std::string begin_;
			std::string end_;
			dv::id_type stride_;
		};

		ClientDescriptor(DV *dv, dv::id_type appid);

		bool handleOpen(const std::string &filename,
						const std::vector<std::string> &parameters);

		void handleNotification(SimJob *simjob);

		double computeHotspot(dv::id_type nr);

		dv::id_type updateDirection(dv::id_type nr, bool & direction_changed);

        dv::id_type horizontal_prefetch(dv::id_type &simstart, dv::id_type &simstop);

		void handleRangeRequest(dv::id_type flag, std::unique_ptr<ClientDescriptor::RangeRequest> request);

        bool newSimulation(dv::id_type target_nr, dv::id_type simstop, std::string strparams);


	private:
		DV *dv_;

		dv::id_type appid_;

		dv::id_type direction_ = 0;
		dv::counter_type openop_ = 0;

		dv::id_type last_open_nr_ = -1;

        PrefetchContext prefetcher_;

		const dv::id_type max_prefetching_intervals_;

		bool notified_ = false;

		Profiler sim_profiler_;
		Profiler cli_profiler_;


		std::unordered_set<dv::id_type> known_sims_;

		std::vector<std::unique_ptr<ClientDescriptor::RangeRequest>> range_requests_;

		dv::id_type next_target_nr(dv::id_type current, dv::id_type direction);
        
        void profile(FileDescriptor * cache_entry);
	};

}

#endif //DV_SERVER_CLIENTDESCRIPTOR_H_
