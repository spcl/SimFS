#ifndef __DVL_RDMA_H__
#define __DVL_RDMA_H__

#ifndef __MOCK_TEST__
#ifdef RDMA
#include <gni_pub.h>
#endif
#endif

#define MAX_CONN           10


typedef struct {
    uint32_t myrank;
    uint32_t ccount;
    uint32_t addr;
#ifndef __MOCK_TEST__ 
#ifdef RDMA
    gni_cq_handle_t cq;
    gni_ep_handle_t eps[MAX_CONN];
#endif
#endif
} gni_conn_t;


#ifndef __MOCK_TEST__
#ifdef RDMA

uint32_t dvl_gni_init(gni_conn_t * conn);
uint32_t dvl_gni_connect(gni_conn_t * conn, unsigned int peer_addr);
uint32_t dvl_gni_disconnect(gni_conn_t * conn, unsigned int peer_addr);
uint32_t dvl_get_gni_nic_address(int32_t device_id);

#endif  // ifdef RDMA
#endif  // ifndef __MOCK_TEST__ 



#endif /* __DVL_RDMA_H__ */
