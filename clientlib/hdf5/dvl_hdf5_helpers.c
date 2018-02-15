/* helper functions
   v1.1 2017-04-27 Pirmin Schmid
*/

#ifndef __HDF5__
#define __HDF5__
#endif

#define _GNU_SOURCE

#include <dlfcn.h>

#include "../dvl.h"
#include "../dvl_internal.h"
#include "../dvl_proxy.h"

#include "dvl_hdf5.h"


void dvl_H5_fill_originals_table(dvl_hdf5_originals_t *originals) {
    originals->h5fcreate = dlsym(RTLD_NEXT, "H5Fcreate");
    originals->h5fopen = dlsym(RTLD_NEXT, "H5Fopen");
    originals->h5fclose = dlsym(RTLD_NEXT, "H5Fclose");

    originals->h5iinc_ref = dlsym(RTLD_NEXT, "H5Iinc_ref");
    originals->h5idec_ref = dlsym(RTLD_NEXT, "H5Idec_ref");

    originals->h5oopen = dlsym(RTLD_NEXT, "H5Oopen");
}

// returns name associated with id, but only for valid id (i.e. >= 0)
// returns "" for invalid ids
// note: filename_buffer must be large enough for the name associated with the id
static char *empty_string = ""; 
char *dvl_H5_filename_associated_with_id(hid_t file_id, char *buffer, size_t len) {
    if (file_id < 0) {
        return empty_string;
    }

    H5Fget_name(file_id, buffer, len - 1); // to be on the save side
    return buffer; 
}
