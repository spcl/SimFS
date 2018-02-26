//
// 01/2018: SDG
//

#ifndef DV_SERVER_PREFETCHCONTEXT_H_
#define DV_SERVER_PREFETCHCONTEXT_H_

#include "../DVBasicTypes.h"
#include "../DVForwardDeclarations.h"

namespace dv {

    class PrefetchContext {
    public:
        PrefetchContext(ClientDescriptor *client, dv::id_type appid);
        void checkForPrefetch(const std::string &filename);
        void reset();  
    
    public:
        enum State { DISABLED, TRANSIENT, STEADY };
    
    private:
        DV * dv_;
        ClientDescriptor * client_;
        dv::id_type appid_;

        /* prefetch info */
        State state_=DISABLED;
        dv::id_type stride_=0; /* >0: fw direction; <0: bw direction */
        dv::id_type last_access_;
        dv::id_type last_nr_=-1;
        dv::id_type parsims_=1; 
          
    private:
        void forward_prefetch(dv::id_type nr);
        void backward_prefetch(dv::id_type nr);
    
    };
}

#endif //DV_SERVER_PREFETCHCONTEXT_H_

