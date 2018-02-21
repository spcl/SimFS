
#ifndef TOOLBOX_NETWORKHELPER_H_
#define TOOLBOX_NETWORKHELPER_H_

#include <string>
#include <vector>

namespace toolbox{

    class NetworkHelper {
    public:
        static int getAllIPs(std::vector<std::string> &ips);

    };

}

#endif /* TOOLBOX_NETWORK_HELPER_H_ */
