#include "NetworkHelper.h"
#include <stropts.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netdevice.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define DOMAIN AF_INET

using namespace std;

namespace toolbox{

    int NetworkHelper::getAllIPs(vector<string> &ips){
        int s;
        struct ifconf ifconf;
        struct ifreq ifr[50];
        int ifs;
        int i;
    
        s = socket(DOMAIN, SOCK_STREAM, 0);
        if (s < 0) { return -1; } 

        ifconf.ifc_buf = (char *) ifr;
        ifconf.ifc_len = sizeof ifr;

        if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) { return -1; }

        ifs = ifconf.ifc_len / sizeof(ifr[0]);
    
        char ip[INET_ADDRSTRLEN];

        for (i=0; i<ifs; i++){
            struct sockaddr_in *s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;
            if (!inet_ntop(DOMAIN, &s_in->sin_addr, ip, sizeof(ip))) { return -1; }
   
            ips.push_back(ip);
        }

        close(s);
        return 1;
    }
}
