//
// Created by Pirmin Schmid on 19.04.17.
//

#ifndef DV_DVFORWARDDECLARATIONS_H_
#define DV_DVFORWARDDECLARATIONS_H_

// this solves some compilation problems (circular dependencies)
// note: where only pointers and references to classes are mentioned
// not a complete class definition is needed; declaration is sufficient there
// thus: only include the full header file where needed, i.e. actual
// class is used or class members are accessed

namespace dv {
	class ClientDescriptor;
	class Cosmo;
	class CosmoConfig;
	class DV;
	class FileCache;
	class FileCollection;
	class FileDescriptor;
	class Flash;
	class FlashConfig;
	class JobQueue;
	class MessageHandler;
	class Profiler;
	class RestartFiles;
	class SimConfig;
	class SimJob;
	class Simulator;
	class VariableDescriptor;
}

#endif //DV_DVFORWARDDECLARATIONS_H_
