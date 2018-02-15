/* hdf5 obj / dataset open following the netcdf blocking get version
 * see adjustment of arguments
 * and needed changes to bring info into nc compatible format of DVL
 *
 * v1.1 2017-04-27 Pirmin Schmid
*/

#ifndef __HDF5__
#define __HDF5__
#endif

#include <assert.h>
#include <stdio.h>

#include "../dvl.h"
#include "../dvl_internal.h"
#include "../dvl_proxy.h"

#include "dvl_hdf5.h"

hid_t H5Oopen(hid_t loc_id, const char *name, hid_t lapl_id) {
    char buff[BUFFER_SIZE];        
    
    DVL_CHECK(DVL_NC_GET_ID);   

    if (dvl.is_simulator) {
        return dvl.h5originals.h5oopen(loc_id, name, lapl_id);
    }

    //--- all here: only client side -------------------------------------------

    // any object has to go thru this blocking mechanism, not just datasets

    // lookup of file_id
    //printf("DVL open object lookup %s at loc id %li\n", name, (long) loc_id);

    char filename[MAX_FILE_NAME + 1];
    hid_t file_id = H5Iget_file_id(loc_id); // note: needs an H5Fclose afterwards
    if (file_id < 0)  {
        printf("lookup of file id not possible, return original function");

        hid_t res = dvl.h5originals.h5oopen(loc_id, name, lapl_id);

        // during testing
        DVLPRINT("DVL open object file is open, returning object id %li for name %s\n", (long) res, name);
        return res;
    }

    // successful lookup
    H5Fget_name(file_id, filename, MAX_FILE_NAME);
    dvl.h5originals.h5fclose(file_id);

    // for testing
    DVLPRINT("DVL open object named %s at loc id %li -> file id %li filename %s\n", name, (long) loc_id, (long) file_id, filename);

    // client side
    dvl_file_t * dfile = NULL;
    HASH_FIND_INT(dvl.open_files_idx, &file_id, dfile);

    if (dfile == NULL) {
        return dvl.h5originals.h5oopen(loc_id, name, lapl_id);
    }

    // for testing                                                                                            
    DVLPRINT("DVL read hash lookup: %s (is_meta: %d)\n", dfile->path, dfile->is_meta);

    // adjusted; we do not need the type information at the moment
    // lookup will differ from netcdf, see HDF5 profiler in tests


    /* if the file is already open, just get the data */
    if (dfile->state == DVL_FILE_OPEN) {
        hid_t res = dvl.h5originals.h5oopen(dfile->fid_to_use, name, lapl_id);

        // during testing
        DVLPRINT("DVL HDF5 open object: file is open, returning object id %li for name %s\n", (long) res, name);
        return res;
    }


    //--- here, we handle the case of files in the waiting list that need re-simulation
    //    thus: file_id refers to the meta file.
    //
    // TODO: this system may need some adjustments when the transfer protocol of variable
    //       names / ids, and data ranges (dimensions, offsets, and counts) are fixed.
    //       currently, only the file name / path is used

    if (!dfile->is_meta) {
        fprintf(stderr, "DVL: ERROR: dfile for %s should be a meta file at this point!\n", dfile->path);
    }

    // note: for files that were "opened" first as meta files, open object calls will come
    // in for the entire life-time of this open file thru the id handle that is associated
    // with the meta file.

    // thus: if the information is already known that the actual file is available
    // -> no need to send a message again to the DVL server
    if (0 <= dfile->other_key) {
        hid_t res = dvl.h5originals.h5oopen(dfile->fid_to_use, name, lapl_id);

        // during testing
        DVLPRINT("DVL HDF5 open object: file is open but still accessed thru meta file id (as expected), returning id %li for name %s (in file id %li)\n",
            (long) res, name, (long) dfile->fid_to_use);
        return res;
    } else {
        DVLPRINT("File not yet available. Going through DVL server messaging.\n");
    }


    //--- here we start building up the connection with the DVL server ---------

    // note: we cannot just block for variables/datasets. It must be blocked for any object that
    // is accessed by the application. Otherwise, there will be a mixup of data structures / objects
    // of different files!

    // here we open it to get the id for DVL
    // we may close it again, if not needed
    hid_t obj_res = dvl.h5originals.h5oopen(loc_id, name, lapl_id);

    /* ask the dvl for this data, communicate the rank. 
       It can reply with AVAIL or SIMULATING */
    int msgsize = BUFFER_SIZE;

    MAKE_MESSAGE(buff, msgsize, "%c:%s:%li:%i:", DVL_MSG_VGET, dfile->path, (long) obj_res, dvl.gni.myrank);

    if (msgsize < 0) {
        return DVL_ERROR;
    }

    dvl_send_message(buff, msgsize, 0);
    
    // recv response -> which is blocking!
    dvl_recv_message(buff, BUFFER_SIZE, 1);
  
    // testing
    DVLPRINT("DVL read message received: %c\n", buff[0]);
 
    // at the moment, we need to close obj_res for all following
    // code paths. This may change
    H5Oclose(obj_res);
    // in case of success, a new object will be opened below
    // associated with the new file ID as loc_id

    if (buff[0] == DVL_REPLY_FILE_OPEN){
        /* if AVAIL just open the file and read from it */
        assert(dfile->state==DVL_FILE_SIM);

        snprintf(buff, BUFFER_SIZE, "%s%s", dvl.respath, dfile->path);

        //hid_t plist_res = H5Pcopy(dfile->access_plist); 
        // note: copying of the property list does not work
        // go with the default one.
        hid_t  plist_res = H5P_DEFAULT;

        //DVLPRINT("DVL open %s with prop list %li copied to %li\n", buff, (long) dfile->access_plist, (long) plist_res);

        // note: here, we open the file and not the object: thus fopen and not oopen.
        hid_t file_res = dvl.h5originals.h5fopen(buff, dfile->flags, plist_res);

        DVLPRINT("DVL file is freshly opened, returning file id %li for name %s\n", (long) file_res, buff);

        if (file_res < 0) {
            fprintf(stderr, "DVL HDF5 object open: ERROR: Could not open actual file %s\n", buff);
            return -1;
        }

        // note: we need to create a duplicate of the file descriptor and add it to the hash table
        // old key is referring to .meta fileid
        // new key is referring to actual fileid

        /* get the next free file descriptor */
        int fileid = dvl.free_files->id;
        dvl.free_files = dvl.free_files->trash_next;
        dvl_file_t * new_dfile = &(dvl.open_files[fileid]);

        dfile->other_key = file_res;
        dfile->fid_to_use = file_res;
        new_dfile->other_key = dfile->key;
        new_dfile->key = file_res;
        new_dfile->fid_to_use = file_res;
        new_dfile->is_meta = 0;
        new_dfile->flags = dfile->flags;
        new_dfile->access_plist = plist_res;
        new_dfile->state = DVL_FILE_OPEN;
        size_t pathlen = strlen(dfile->path);
        memcpy(new_dfile->path, dfile->path, pathlen + 1);

        // add the new file_id to hashmap for the next lookup
        HASH_ADD_INT(dvl.open_files_idx, key, &(dvl.open_files[new_dfile->id]));

        DVLPRINT("DVL said the file is avail: opening object %s in file  %s (fid %li)\n", name, buff, (long) new_dfile->fid_to_use);

        // here, we open the object for real
        obj_res = dvl.h5originals.h5oopen(new_dfile->fid_to_use, name, lapl_id);

        DVLPRINT("DVL open object file is freshly opened, returning object id %li for name %s\n", (long) obj_res, name);
        return obj_res;

    } else if (buff[0] == DVL_REPLY_RDMA){
        /* make RDMA get */

        return -1; /* do not call the original get */
    }

    printf("#####Going to fail! Message: %s\n", buff);
    assert(1!=1);
    return -1; /* shouldn't be reached */
}
