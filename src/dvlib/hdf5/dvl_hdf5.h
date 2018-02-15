/* declares all dvl functions provided by HDF5 implementation.
   note: HDF5 is more complex to build a layer than NetCDF / Parallel-NetCDF.

   In contrast to NetCDF, HDF5 offers various options to do the same thing
   (see e.g. how to close a file). Different applications may use different
   paths (e.g. FLASH simulator used H5Fclose, h5py uses H5Idec_ref to close
   files), which all need to be caught and mapped to a unique path
   of interaction with the DVL server and statistics logger. Additionally,
   e.g. H5Idec_ref can be used for closing of datasets and other references, too.

   Additionally, files are not necessarily closed on H5Fclose.
   Only if the reference counter goes to 0, the file is actually closed
   on the file system. And additionally, for some lookup uperations,
   files are automatically reopened again.

   All such details need to be handled properly.

   note: with the current system of storing the original function pointers,
   the dvl functions are directly mapped to the original function signatures
   if possible.

   Comment on printing hid_t values: because this type maps to an int in
   v1.8 and to a long int in v1.10, always a cast to (long) is used.

   The wrapper interface is not yet complete and handles only the
   functions that are currently needed.

   Current feature support (compared to netcdf version):
   - Only create, open and close (and associated functions); no read (get), no write (put).
   - Finalize / stopping the simulator is supported (if simulator accepts it; see SIGINT).
   - The implementation includes result and checkpoint files redirection (if existent),
   but does not include the avoid writing feature (see put and get not implemented).
   - Multithreaded client support is also not supported (yet).

   v0.6 2017-09-20 Pirmin Schmid
*/

#ifndef CLIENTLIB_HDF5_DVL_HDF5_
#define CLIENTLIB_HDF5_DVL_HDF5_

#include <H5Fpublic.h>
#include <H5Dpublic.h>
#include <H5Ipublic.h>
#include <H5Opublic.h>

#include "hdf5_bind.h"

//--- administration & helpers -------------------------------------------------

// defined separately to assure values to always be a valid subset of herr_t
#define DVL_HDF5_SUCCESS 0
#define DVL_HDF5_ERROR (-1)

// official error code value
#define HDF5_FAIL (-1)

typedef struct dvl_hdf5_originals_ {
    oh5f_create_t h5fcreate;
    oh5f_open_t h5fopen;
    oh5f_close_t h5fclose;

    oh5i_inc_ref_t h5iinc_ref;
    oh5i_dec_ref_t h5idec_ref;

    oh5o_open_t h5oopen;
} dvl_hdf5_originals_t;

void dvl_H5_fill_originals_table(dvl_hdf5_originals_t *originals);

char * dvl_H5_filename_associated_with_id(hid_t file_id, char *buffer, size_t len);


//--- entry points & handlers --------------------------------------------------

hid_t H5Fcreate(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id);

hid_t H5Fopen(const char *filename, unsigned flags, hid_t access_plist);


typedef enum dvl_hdf5_file_close_ {
  CLOSE_BY_H5FCLOSE,
  CLOSE_BY_H5IDEC_REF
} dvl_hdf5_file_close_t;

herr_t H5Fclose(hid_t file_id);

// herr_t is defined as int in HDF5 v1.8 and v1.10
// this type here is defined to have a cleaner upgrade path, if needed,
// because some internal functions need to be used in case of herr_t and int
// return values
typedef int dvl_hdf5_herr_or_int_t;

dvl_hdf5_herr_or_int_t dvl_hdf5_handle_file_close(hid_t file_id, dvl_hdf5_file_close_t type);


/*
herr_t dvl_H5Dread(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id,
            hid_t file_space_id, hid_t plist_id, void *buf out);
*/

/*
herr_t dvl_H5Dwrite(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id,
             hid_t file_space_id, hid_t plist_id, const void *buf);
*/

int H5Idec_ref(hid_t id);

hid_t H5Oopen(hid_t loc_id, const char *name, hid_t lapl_id);

#endif // CLIENTLIB_HDF5_DVL_HDF5_
