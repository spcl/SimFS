#define _DEFAULT_SOURCE

#include "dvl.h"
#include "dvl_internal.h"
#include "dvl_proxy.h"
#include <assert.h>


void stringify_size_array(char * buff, size_t bsize, const size_t * arr, size_t asize){
    int off = 0;
    int res;
    for (int i=0; i<asize; i++){
        res = snprintf(buff + off, bsize-off, "%lu,", arr[i]);

        //printf("stringify: ndims: %lu, res: %i, write: %lu; buff: %s\n", asize, res, arr[i], buff);
        if (res>bsize-off) { printf("BUFFER TO SMALL!\n"); DVL_ABORT;}
        off += res;
    }
    printf("\n");
    buff[off-1] = '\0';
}


int dvl_nc_put(int id, int varid, const size_t start[], const size_t count[], const void * valuesp){
    // nothing is done here at the moment (see: the macro is not defined)
    // when it gets activated, various things need to be checked again against the latest version of the code base

#ifdef SEND_PUT_MESSAGE
    char buff[BUFFER_SIZE];
    char pathbuff[MAX_FILE_NAME];
    char npath[MAX_FILE_NAME];
    char countstr[BUFFER_SIZE];
    char startstr[BUFFER_SIZE];

    int bsize = BUFFER_SIZE;

    countstr[0] = '\0';
    startstr[0] = '\0';

#ifdef __MT__
    uint32_t mt_rank = 0;
#endif
    DVL_CHECK(DVL_NC_PUT_ID);


    if (dvl.is_simulator) {
#ifdef __WRITE_NO_DATA_DURING_REDIRECT__
        // here the implementation if SEND_PUT_MESSAGE is on
        // below follows the implementation if it is off
        dvl_redirected_file_t *rfile = NULL;
        HASH_FIND_INT(dvl.open_redirected_files_idx, &id, rfile);
        if (rfile != NULL) {
            return DVL_PREVENT_WRITING_DATA_DURING_REDIRECT;
        }
#endif

        size_t pathlen;
        int ndims;

        if (dvl.finalized){
            fprintf(stderr, "Warning: this simulation is suppose to terminate but it's trying to write to a file.\n");
            return NC_ENOMEM;
        }

        nc_inq_path(id, &pathlen, NULL);
        nc_inq_var(id, varid, NULL, NULL, &ndims, NULL, NULL);
        
        if (pathlen >= MAX_FILE_NAME){
            printf("Not enough space to build the PUT message\n");
            DVL_ABORT;
        }

        nc_inq_path(id, NULL, pathbuff);
        char * path = is_result_file(pathbuff, npath);
        if (path==NULL) return id;

        stringify_size_array(startstr, BUFFER_SIZE, start, ndims);
        stringify_size_array(countstr, BUFFER_SIZE, count, ndims);

        MAKE_MESSAGE(buff, bsize, "%c:%i:%i:%s:%i:%s:%s", DVL_MSG_VPUT, dvl.gni.myrank, varid, path, ndims, startstr, countstr);

        if (bsize<0) return DVL_ERROR;
    
        //printf("sending: %s\n", buff);
        dvl_send_message(buff, bsize, 1);
        
        return id;
    }
    printf("put from client applications is not supported yet\n");


#else
    // inactive SEND_PUT_MESSAGE
    // only code for testing of __WRITE_NO_DATA_DURING_REDIRECT__

#ifdef __WRITE_NO_DATA_DURING_REDIRECT__

#ifdef __MT__
    uint32_t mt_rank = 0;
#endif
    DVL_CHECK(DVL_NC_PUT_ID);

    if (dvl.is_simulator) {
        dvl_redirected_file_t *rfile = NULL;
        HASH_FIND_INT(dvl.open_redirected_files_idx, &id, rfile);
        if (rfile != NULL) {
            return DVL_PREVENT_WRITING_DATA_DURING_REDIRECT;
        }
    }
#endif // __WRITE_NO_DATA_DURING_REDIRECT__

#endif // SEND_PUT_MESSAGE


    return id;
}
