
#ifndef __MOCK_TEST__
#ifdef RDMA

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

//#include "pmi.h"

#include "dvl_rdma.h"

int cookie;
int modes        = 0;

#define TRANS_LEN          128
#define TRANS_LEN_IN_BYTES ((TRANS_LEN)*sizeof(uint64_t))
#define NTRANS             20
#define CACHELINE_MASK     0x3F    /* 64 byte cacheline */

#define CQENTRIES 	   1024

#define GNICHECK(X) \
{ \
    gni_return_t status; \
    if ((status = X) != GNI_RC_SUCCESS) { \
        printf("GNI error: %u\n", status); \
        return -1; \
    } \
}

typedef struct {
    gni_mem_handle_t mdh;
    uint64_t addr;
} mdh_addr_t;




uint32_t dvl_get_gni_nic_address(int32_t device_id){
    unsigned int address, cpu_id;
    gni_return_t status;
    int i,alps_dev_id=-1,alps_address=-1;
    char *token,*p_ptr;

    p_ptr = getenv("PMI_GNI_DEV_ID");
    if (!p_ptr) {
        status = GNI_CdmGetNicAddress(device_id, &address, &cpu_id);
        if(status != GNI_RC_SUCCESS) {
            fprintf(stderr,"FAILED:GNI_CdmGetNicAddress returned error %d\n",status);
            abort();
        }
    } else {
        while ((token = strtok(p_ptr,":")) != NULL) {
            alps_dev_id = atoi(token);
            if (alps_dev_id == device_id) {
                break;
            }
            p_ptr = NULL;
        }

        assert(alps_dev_id != -1);
        p_ptr = getenv("PMI_GNI_LOC_ADDR");
        assert(p_ptr != NULL);
        i = 0;
        while ((token = strtok(p_ptr,":")) != NULL) {
            if (i == alps_dev_id) {
                alps_address = atoi(token);
                break;
             }
             p_ptr = NULL;
             ++i;
        }
        assert(alps_address != -1);
        address = alps_address;
        address = alps_address;
    }

    return address;
}


static uint8_t get_ptag(void){
    char *p_ptr,*token;
    uint8_t ptag;

    p_ptr = getenv("PMI_GNI_PTAG");
    assert(p_ptr != NULL);  /* something wrong like we haven't called PMI_Init */
    token = strtok(p_ptr,":");
    ptag = (uint8_t)atoi(token);
    return ptag;
}


static uint32_t get_cookie(void){
    uint32_t cookie;
    char *p_ptr,*token;

    p_ptr = getenv("PMI_GNI_COOKIE");
    assert(p_ptr != NULL);
    token = strtok(p_ptr,":");
    cookie = (uint32_t)atoi(token);

    return cookie;
}


uint32_t dvl_gni_init(gni_conn_t * conn){
    uint8_t ptag;
    gni_cdm_handle_t cdm_hndl;
    gni_nic_handle_t nic_hndl;
    int device_id = 0;
    unsigned int local_addr;

    conn->ccount = 0;
    
    ptag = get_ptag();
    cookie = get_cookie();

    /* Create and attach to the communication domain. */
    GNICHECK(GNI_CdmCreate(conn->myrank, ptag, cookie, modes, &cdm_hndl));
    GNICHECK(GNI_CdmAttach(cdm_hndl, device_id, &local_addr, &nic_hndl));

    /* Create the local completion queue */
    GNICHECK(GNI_CqCreate(nic_hndl, CQENTRIES, 0, GNI_CQ_NOBLOCK, NULL, NULL, &(conn->cq)));

    for (int i=0; i<MAX_CONN; i++){
        GNICHECK(GNI_EpCreate(nic_hndl, conn->cq, &(conn->eps[i])));
    }

    return 0;
}

uint32_t dvl_gni_connect(gni_conn_t * conn, unsigned int peer_addr){

    if (conn->ccount >= MAX_CONN) {
        printf("Too many connections!!! (%i)\n", MAX_CONN);
        return -1;
    }
    
    int i = conn->ccount++;
    GNICHECK(GNI_EpBind(conn->eps[i], peer_addr, i));
   
    return 0;
}

uint32_t dvl_gni_disconnect(gni_conn_t * conn, unsigned int peer_addr){
    /* need an hash table here */
    return 0;
}

#endif // ifdef RDMA
#endif // idnfdef __MOCK_TEST__

