//
// Created by Pirmin Schmid on 12.04.17.
//

#ifndef DV_CACHES_FILECACHES_FILEDESCRIPTOR_H
#define DV_CACHES_FILECACHES_FILEDESCRIPTOR_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../DVBasicTypes.h"
#include "../../DVForwardDeclarations.h"
#include "VariableDescriptor.h"


namespace dv {

	class FileDescriptor {
	public:
		FileDescriptor(const std::string &name, const std::string &file_name);

		const std::string &getName() const;

		const std::string &getFileName() const;

		void setPartitionKey(dv::id_type partition_key);
		dv::id_type getPartitionKey() const;

		void setNr(dv::id_type id_nr);
		dv::id_type  getNr() const;

		dv::counter_type getUseCount() const;

		dv::counter_type getLockCount() const;

		bool isFileAvailable() const;
		void setFileAvailable(bool available);

		/**
		 * note: while these status functions work with booleans,
		 * internally, a simulator use count is used.
		 * Thus, in the bad case scenario that 2 simulators would report working on the file
		 * (which is a bad case and should be avoided by cache sizes large enough
		 * to accomodate for the productions of at least the number of simulators that
		 * can be run in parallel), at least this status function will not allow
		 * a file to be evicted by falsely assuming that no simulator is currently
		 * using the file).
		 */
		bool isFileUsedBySimulator() const;
		void setFileUsedBySimulator(bool used);

		bool isFilePrefetched() const;
		void setFilePrefetched(bool prefetched);

		void setSize(dv::size_type size);
		dv::size_type getSize() const;

		void setCost(dv::cost_type cost);
		void setUnitCost();
		void applyPenaltyFactor(double penalty_factor);
		dv::cost_type getActualCost() const;

		void lock();

		void unlock();

		/**
		 * note:
		 * notification sockets and waiting clients must be hold separately,
		 * since multi-threaded clients may be waiting for various files in parallel
		 * and we cannot just send a notification to all of these waiting sockets.
		 */

		void appendNotificationSocket(int socket);
		void removeAllNotificationSockets();
		const std::unordered_set<int> &getNotificationSockets() const;
		
		void appendWaitingClientPtr(ClientDescriptor *client_descriptor_ptr);
		void removeAllWaitingClientPtrs();
		const std::unordered_set<ClientDescriptor *> &getWaitingClientPtrs() const;

		void addVariable(const std::string &id, size_t offset, size_t count);

		std::string toString() const;

	private:
		static constexpr dv::cost_type kUnitCost = 100;
		// note: assure that this cost > 0 if reduced by getActualCost() in case for not-used/requested files

		std::string name_;
		std::string file_name_;
		dv::id_type partition_key_ = 0;
		dv::id_type id_nr_ = 0;

		dv::counter_type use_count_ = 0;
		dv::counter_type lock_count_ = 0;
		bool file_available_ = false;
		dv::counter_type file_used_by_simulator_count_ = 0;
		bool is_prefetched_ = false;

		dv::size_type size_ = 0;

		dv::cost_type cost_ = 0;

		std::unordered_set<int> notification_sockets_;
		
		std::unordered_set<ClientDescriptor *> waiting_clients_ptrs_;

		std::unordered_map<std::string, VariableDescriptor> variables_;
	};

}

#endif //DV_CACHES_FILECACHES_FILEDESCRIPTOR_H
