/* hdf5_bind.h

   note: not all methods of the interfaces are caught,
   but only the ones of interest at the moment.
   
   v0.4 2017-04-27 Pirmin Schmid
*/


#ifndef CLIENTLIB_HDF5_HDF5_BIND_H_
#define CLIENTLIB_HDF5_HDF5_BIND_H_

#include <H5Fpublic.h>
#include <H5Dpublic.h>
#include <H5Ipublic.h>
#include <H5Opublic.h>


// F: from file interface (H5F)
typedef hid_t (*oh5f_create_t)(const char *, unsigned, hid_t, hid_t);
typedef hid_t (*oh5f_open_t)(const char *, unsigned, hid_t);
typedef hid_t (*oh5f_reopen_t)(hid_t); 
typedef herr_t (*oh5f_close_t)(hid_t);


// D: from dataset interface (H5D)
typedef hid_t (*oh5d_open1_t)(hid_t, const char *);
typedef hid_t (*oh5d_open2_t)(hid_t, const char *, hid_t);
typedef herr_t (*oh5d_read_t)(hid_t, hid_t, hid_t, hid_t, hid_t, void *);
typedef herr_t (*oh5d_write_t)(hid_t, hid_t, hid_t, hid_t, hid_t, const void *);
typedef herr_t (*oh5d_close_t)(hid_t);


// I: from identifier interface (H5I) 
typedef int (*oh5i_inc_ref_t)(hid_t id);
typedef int (*oh5i_dec_ref_t)(hid_t id);


// O: from object interface (H5O) -- incomplete at the moment
typedef hid_t (*oh5o_open_t)(hid_t, const char *, hid_t);
typedef hid_t (*oh5o_open_by_idx_t)(hid_t, const char *, H5_index_t, H5_iter_order_t, hsize_t, hid_t);
typedef hid_t (*oh5o_open_by_addr_t)(hid_t, haddr_t);
typedef herr_t (*oh5o_close_t)(hid_t);

#endif  // CLIENTLIB_HDF5_HDF5_BIND_H_
