/* hdf5_bind.c
   
   Read currently maps the areas to netcdf compatible areas to
   be compatible with the netcdf calls

   Put/Write is not yet implemented.

   v0.4 2017-04-27 Pirmin Schmid
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>

#include "hdf5_bind.h"

#include "../dvl.h"
#include "../dvl_internal.h"
#include "dvl_hdf5.h"


//--- F: from file interface (H5F) ---------------------------------------------

// hid_t H5Fcreate(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id)
// see dvl_hdf5_create.c

// hid_t H5Fopen(const char *filename, unsigned flags, hid_t access_plist)
// see dvl_hdf5_open.c



/*

// TODO: this function is just printing at the moment.
hid_t H5Freopen(hid_t file_id) {
    oh5f_reopen_t original = dlsym(RTLD_NEXT, "H5Freopen");

    char filename[MAX_FILE_NAME + 1];

    hid_t res = original(file_id);
#ifdef __HDF5_1_10__
    printf("HDF5_FILE_REOPEN:%li:%s:%li\n", file_id, dvl_H5_filename_associated_with_id(file_id, filename, MAX_FILE_NAME), res);
#else
    printf("HDF5_FILE_REOPEN:%i:%s:%i\n", file_id, dvl_H5_filename_associated_with_id(file_id, filename, MAX_FILE_NAME), res);
#endif
    // res == file_id for later reference
    return res;
}

*/


// herr_t H5Fclose(hid_t file_id)
// see dvl_hdf5_close.c


//--- D: from dataset interface (H5D) ------------------------------------------

/*
herr_t H5Dread(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id,
            hid_t file_space_id, hid_t plist_id, void *buf) {
    oh5d_read_t original = dlsym(RTLD_NEXT, "H5Dread");
    hid_t res = dvl_H5Dread(dset_id, mem_type_id, mem_space_id, file_space_id, plist_id, buf);
    if (res >= 0) return (*original)(dset_id, mem_type_id, mem_space_id, file_space_id, plist_id, buf);
    // default: negative error
    return -1; 
}
*/

/*
herr_t H5Dwrite(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id,
             hid_t file_space_id, hid_t plist_id, const void *buf) {
    oh5d_write_t original = dlsym(RTLD_NEXT, "H5Dwrite");

    hid_t res = dvl_H5Dwrite(dset_id, mem_type_id, mem_space_id, file_space_id, plist_id, buf);
    if (res >= 0) return (*original)(dset_id, mem_type_id, mem_space_id, file_space_id, plist_id, buf)
    // default: negative error
    return -1; 
}
*/


//--- I: identifier interface --------------------------------------------------


static const char *type_names[] = {
    "ZERO",
    "FILE",
    "GROUP",
    "DATATYPE",
    "DATASPACE",
    "DATASET",
    "ATTR",
    "REFERENCE",
    "VFL",
    "GENPROP_CLS",
    "GENPROP_LST",
    "ERROR_CLASS",
    "ERROR_MSG",
    "ERROR_STACK"
};

// this is for security only. this value should be == H5I_NTYPES
static const H5I_type_t n_types = sizeof(type_names) / sizeof(type_names[0]);

static const char *bad_id = "BAD_ID";

__attribute__((unused))
static const char *getTypeName(H5I_type_t nr) {
    if (nr < 0 || n_types <= nr) {
        return bad_id;
    }

    return type_names[nr];
}

/*

// TODO: just printing at the moment
int H5Iinc_ref(hid_t id) {
    oh5i_inc_ref_t original = dlsym(RTLD_NEXT, "H5Iinc_ref");

    int res = original(id);

    char filename[MAX_FILE_NAME + 1];
    char varname[MAX_VAR_NAME + 1];

    // defaults
    hid_t var_id = -1;
    hid_t file_id = id;

    H5I_type_t type = H5Iget_type(id);
    char *type_str = getTypeName(type);
    char *fname;    
    if (H5I_DATASET == type) {
        var_id = id;
        H5Iget_name(var_id, varname, MAX_VAR_NAME);

        file_id = H5Iget_file_id(id); // note: needs an H5Fclose afterwards
        fname = dvl_H5_filename_associated_with_id(file_id, filename, MAX_FILE_NAME);
        dvl_H5Fclose_internal(file_id);
    } else if (H5I_FILE == type) {
        fname = dvl_H5_filename_associated_with_id(file_id, filename, MAX_FILE_NAME);
    } else if (0 < type) {
        file_id = H5Iget_file_id(id); // note: needs an H5Fclose afterwards
        fname = dvl_H5_filename_associated_with_id(file_id, filename, MAX_FILE_NAME);
        dvl_H5Fclose_internal(file_id);
    } else {
        fname = "";
    }

#ifdef __HDF5_1_10__
    printf("HDF5_%s_INC_REF:%li:%s:%li:%s:refcount=%i\n", type_str,
        file_id, fname, var_id, var_id == -1 ? "" : varname, res);
#else
    printf("HDF5_%s_INC_REF:%i:%s:%i:%s:refcount=%i\n", type_str,
        file_id, fname, var_id, var_id == -1 ? "" : varname, res);
#endif
    return res;
}

*/


// note: closing of files and datasets may happen just by calling this function
// we only need to react on files at the moment
// otherwise, just the original function is called
int H5Idec_ref(hid_t id) {
    DVL_CHECK_BENCH_ID_FOLLOWS;

    H5I_type_t type = H5Iget_type(id);
    //char *type_str = getTypeName(type); 

    // during testing
    //DVLPRINT("H5Idec_ref: type %s\n", type_str);
    // deactivated: this function is called very frequently


    // lookup of meta information
    /* // not needed at the moment
    hid_t file_id = id; // default; may change depending on type
    char filename[MAX_FILE_NAME];
    filename[0] = 0;
    char *fname = filename;
    hid_t var_id = -1;
    char varname[MAX_VAR_NAME];
    varname[0] = 0;
    //*/


    if (H5I_FILE == type) {
        return dvl_hdf5_handle_file_close(id, CLOSE_BY_H5IDEC_REF);

    } else if (H5I_DATASET == type) {
        /* // not needed at the moment
        var_id = id;
        H5Iget_name(var_id, varname, MAX_VAR_NAME - 1);

        file_id = H5Iget_file_id(id); // note: needs an H5Fclose afterwards
        if (0 <= file_id) {
            fname = dvl_H5_filename_associated_with_id(file_id, filename, MAX_FILE_NAME);
            dvl.h5originals.h5fclose(file_id);
        }
        //*/
    } else {
        // no need for lookups in other situations
        // default is already ok
    }

    return dvl.h5originals.h5idec_ref(id);
}


//--- O: object interface ------------------------------------------------------

// hid_t H5Oopen(hid_t loc_id, const char *name, hid_t lapl_id)
// see dvl_hdf5_obj_open.c
