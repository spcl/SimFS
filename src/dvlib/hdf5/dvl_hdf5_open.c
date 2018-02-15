/* hdf5 open following the netcdf version
 * see adjustment of arguments
 *
 * 2017-03-31 Pirmin Schmid
*/

#ifndef __HDF5__
#define __HDF5__
#endif

#include "../dvl.h"
#include "../dvl_internal.h"
#include "../dvl_proxy.h"

#include "dvl_hdf5.h"

hid_t H5Fopen(const char *opath, unsigned flags, hid_t access_plist) {
    char buff[BUFFER_SIZE];        
    char npath[MAX_FILE_NAME];

    DVL_CHECK(DVL_NC_OPEN_ID);

    DVLPRINT("open!! %s\n", opath);

    /* Check if this file is relevant to us */
    char * path = is_result_file(opath, npath);
    if (path == NULL) {
        return dvl.h5originals.h5fopen(opath, flags, access_plist);
    }


    if (!dvl.is_simulator) {
        DVLPRINT("DVL OPEN!\n");

        /* Check if we have file descriptors available */
        if (dvl.free_files==NULL){
            printf("Error: too many open files!\n");
            DVL_ABORT;
        }        

        size_t pathlen = strlen(path);

        if (pathlen >= MAX_FILE_NAME){
            printf("Error: file name too long: %s\n", path);
            DVL_ABORT;
        }
    
        /* get the next free file descriptor */
        int fileid = dvl.free_files->id;
        dvl.free_files = dvl.free_files->trash_next;

        dvl_file_t * dfile = &(dvl.open_files[fileid]);
#ifdef BENCH
        //LSB_Set_Rparam_str("restart", simstart);
        //LSB_Res();
#endif

        /* send request to the dvl */
        int msgsize = BUFFER_SIZE;

        // here we use the shorter path (without pre-defined respath): path (equal as in nc version)
        MAKE_MESSAGE(buff, msgsize, "%c:%s:%i:file=%s;gni_addr=%u", DVL_MSG_FOPEN, path, dvl.gni.myrank, path, dvl.gni.addr);

        if (msgsize<0) {
            fprintf(stderr, "DVLib H5Fopen: could not create the message.\n");
            return DVL_ERROR;
        }
        dvl_send_message(buff, msgsize, 0);
    
        /* recv response -> this is blocking for a typically short time */
        dvl_recv_message(buff, BUFFER_SIZE, 1);

        hid_t res;

        if (buff[0]==DVL_REPLY_FILE_SIM){
            DVLPRINT("File is not avail. Opening meta: %s\n", buff+1);
            res = dvl.h5originals.h5fopen(buff+1, flags, access_plist);
            if (res < 0) {
                printf("Error opening metadata file\n");
                DVL_ABORT;
            }
            dfile->state = DVL_FILE_SIM;
            dfile->is_meta = 1;     
        } else {
            DVLPRINT("DVL message received: file %s is available\n", opath);
            res = dvl.h5originals.h5fopen(opath, flags, access_plist);
            dfile->state = DVL_FILE_OPEN;
            dfile->is_meta = 0;  
        }

        dfile->key = res;
        dfile->other_key = -1;
        dfile->fid_to_use = res;
        dfile->flags = flags;
        dfile->access_plist = access_plist;

        memcpy(dfile->path, path, pathlen + 1);

        HASH_ADD_INT(dvl.open_files_idx, key, &(dvl.open_files[fileid]));

        DVLPRINT("DVL_NC_OPEN: relative path (used for DVL comm): %s, full path: %s, fileid: %i, idx cnt: %u, is_meta: %i, key: %li\n",
            path, npath, fileid, HASH_COUNT(dvl.open_files_idx), dfile->is_meta, (long) dfile->key);
        return res;

    } else { /* we don't care about the simulator */
        /*int msgsize = BUFFER_SIZE;
        MAKE_MESSAGE(buff, msgsize, "%c:%s", DVL_MSG_FOPEN_SIM, path);
        if (msgsize<0) return DVL_ERROR;

        dvl_send_message(buff, msgsize, 1);
        */
        DVLPRINT("SIM OPEN!\n");
        return dvl.h5originals.h5fopen(opath, flags, access_plist);
    }
}
