#define _DEFAULT_SOURCE

#include "dvl.h"
#include "dvl_internal.h"
#include "dvl_proxy.h"
#include <assert.h>

int dvl_nc_get(int id, int varid, const size_t start[], const size_t count[], const void * valuesp){

    char buff[BUFFER_SIZE];        

#ifdef __MT__
    uint32_t mt_rank = 0;
#endif	
    DVL_CHECK(DVL_NC_GET_ID);   

    if (dvl.is_simulator) return id;

    // * Client side is split for default and MT aware libraries to avoid having a read lock applied
    // over the entire function including network I/O access, which may fail and thus render
    // the entire open_files hashmap useless in that case.
    // And actually, the messaging with DV is a blocking operation that can take a while
    // if the file needs to be generated first.
    // Thus, a local copy of the data structure is created for MT aware libraries.
    // Default library continues using the data as before
    // * Additional difference for MT: mt_rank (created inside DVL_CHECK() macro) is used instead
    //   of dvl.gni.myrank in MAKE_MESSAGE.
    // * Finally, for the situation of a state change, a second lock is acquired then (write)
    //   that needs a check whether the data structure is still valid. Thus again some
    //   subtle differences (including the avoiding of assert/fail in case another thread)
    //   was faster.

#ifdef __MT__
    // copy of default code with several modifications as explained above

    if (pthread_rwlock_rdlock(&dvl.open_files_lock) != 0) fatal("dvl_nc_get(): can't get rdlock");
    dvl_file_t * dfile_ptr = NULL;
    HASH_FIND_INT(dvl.open_files_idx, &id, dfile_ptr);
    if (dfile_ptr == NULL) {
        pthread_rwlock_unlock(&dvl.open_files_lock);
        return id;
    }

    // create local copy and release lock
    dvl_file_t dfile = *dfile_ptr;
    pthread_rwlock_unlock(&dvl.open_files_lock);

    /* if the file is already open, just get the data */
    if (dfile.state == DVL_FILE_OPEN) {
        return dfile.ncid;
    }

    nc_type type;
    size_t tsize;
    nc_inq_vartype(dfile.ncid, varid, &type);
    nc_inq_type(dfile.ncid, type, NULL, &tsize);

    //printf("GET (size: %u)!!!\n", (uint32_t) tsize);

    /* ask the dvl for this data, communicate the rank. 
       It can reply with AVAIL or SIMULATING */
    int msgsize = BUFFER_SIZE;

    // note: additional change here: dvl.gni.myrank -> mt_rank
    MAKE_MESSAGE(buff, msgsize, "%c:%s:%i:%i:", DVL_MSG_VGET, dfile.path, varid, mt_rank);

    if (msgsize<0) return DVL_ERROR;

    dvl_send_message(buff, msgsize, 0);
    
    /* recv response (which is blocking) */
    dvl_recv_message(buff, BUFFER_SIZE, 1);
   
    if (buff[0] == DVL_REPLY_FILE_OPEN) {
        /* if AVAIL just open the file and read from it */
        assert(dfile.state==DVL_FILE_SIM);

        // more things need to happen within the write lock here to avoid race conditions to actually open the file
        // note: it is not clear how a client may have made non-thread-safe netcdf working with multiple threads
        // while open and close may be encapsulated in locks, get operations may potentially be free floating
        // -> thus: any race condition between changing states here and actually open file identifiers must be avoided

        snprintf(buff, BUFFER_SIZE, "%s%s", dvl.respath, dfile.path);
        printf("DVL said the file is avail: opening %s\n", buff);

        if (pthread_rwlock_wrlock(&dvl.open_files_lock) != 0) fatal("dvl_nc_get(): can't get wrlock");        
        HASH_FIND_INT(dvl.open_files_idx, &id, dfile_ptr);
        if (dfile_ptr == NULL) {
            pthread_rwlock_unlock(&dvl.open_files_lock);
            fprintf(stderr, "dvl_nc_get(): file data structure removed before it could be updated with new info. Check client code.\n");
            return NC_EBADID; // return error message
        }

        if (strncmp(dfile.path, dfile_ptr->path, MAX_FILE_NAME) == 0) {
            if (dfile_ptr->state == DVL_FILE_SIM) {
                // see assert above: OK to actually open the actual file
                dvl.ncopen(buff, dfile.omode, &(dfile.ncid));
                dfile.state = DVL_FILE_OPEN;

                dfile_ptr->state = dfile.state;
                dfile_ptr->ncid = dfile.ncid;
            } else if (dfile_ptr->state == DVL_FILE_OPEN) {
                // a get() operation from another thread has been faster -> use that id
                dfile.ncid = dfile_ptr->ncid;
            } else {
                fprintf(stderr, "dvl_nc_get(): Unknown state inside of dfile. Check DVLib code.\n");
                // this should not happen
                dfile.ncid = NC_EBADID;
            }
        } else {
            fprintf(stderr, "dvl_nc_get(): file path does not match after lookup. File ID must have been reused by other file. Check client code.\n");
            dfile.ncid = NC_EBADID;
        }

        pthread_rwlock_unlock(&dvl.open_files_lock);

        return dfile.ncid;

    } else if (buff[0] == DVL_REPLY_RDMA) {
        /* make RDMA get */

        return -1; /* do not call the original get */
    }


#else
    // default / master code
    dvl_file_t * dfile = NULL;
    HASH_FIND_INT(dvl.open_files_idx, &id, dfile);
    if (dfile==NULL) return id;

    /* if the file is already open, just get the data */
    if (dfile->state == DVL_FILE_OPEN) {
        return dfile->ncid;
    }

    nc_type type;
    size_t tsize;
    nc_inq_vartype(dfile->ncid, varid, &type);
    nc_inq_type(dfile->ncid, type, NULL, &tsize);

    //printf("GET (size: %u)!!!\n", (uint32_t) tsize);

    /* ask the dvl for this data, communicate the rank. 
       It can reply with AVAIL or SIMULATING */
    int msgsize = BUFFER_SIZE;

    MAKE_MESSAGE(buff, msgsize, "%c:%s:%i:%i:", DVL_MSG_VGET, dfile->path, varid, dvl.gni.myrank);

    if (msgsize<0) return DVL_ERROR;

    dvl_send_message(buff, msgsize, 0);
    
    /* recv response (which is blocking) */
    dvl_recv_message(buff, BUFFER_SIZE, 1);
   
    if (buff[0] == DVL_REPLY_FILE_OPEN) {
        /* if AVAIL just open the file and read from it */
        assert(dfile->state==DVL_FILE_SIM);
    
        dfile->state = DVL_FILE_OPEN;
        snprintf(buff, BUFFER_SIZE, "%s%s", dvl.respath, dfile->path);        

        dvl.ncopen(buff, dfile->omode, &(dfile->ncid));

        printf("DVL said the file is avail: opening %s\n", buff);
        return dfile->ncid;

    } else if (buff[0] == DVL_REPLY_RDMA) {
        /* make RDMA get */

        return -1; /* do not call the original get */
    }

#endif

    printf("#####Going to fail! Message: %s\n", buff);
    assert(1!=1);
    return id; /* shouldn't be reached */
}
