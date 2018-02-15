#!/bin/bash

# get the defined paths
. PATHS.in

mkdir lib logs 2> /dev/null
NETCDFI="-I $SDAVI_NETCDF_PATH/include/"
NETCDFL="-L $SDAVI_NETCDF_PATH/lib/"

HDF5_8I="-I $SDAVI_HDF5_1_8_PATH/include/"
HDF5_8L="-L $SDAVI_HDF5_1_8_PATH/lib/"

HDF5_10I="-I $SDAVI_HDF5_1_10_PATH/include/"
HDF5_10L="-L $SDAVI_HDF5_1_10_PATH/lib/"

if [ "$1" != "NOBENCH" ]; then
    benchflag="-DBENCH"
else
    echo "Note: compiling without libLSB"
    benchflag=""
fi

if [ "$SDAVI_LIBLSB_PATH" = "" ]; then
	LIBLSBI=
	LIBLSBL=
else
	LIBLSBI="$benchflag -I $SDAVI_LIBLSB_PATH/include/"
	LIBLSBL="-L $SDAVI_LIBLSB_PATH/lib/ -llsb -Wl,-rpath,$SDAVI_LIBLSB_PATH/lib/"
fi

# currently only for testing: deactivate actual put/write operation for redirected files
# uncomment the -D compile option flag, if desired
# only available for netcdf at the moment
no_data_during_redirect=""
#no_data_during_redirect="-D__WRITE_NO_DATA_DURING_REDIRECT__"

# DVLib for NetCDF
if [ "$SDAVI_BUILD_FOR_NETCDF" = "YES" ]; then
	echo "Building lib/libdvl.so for netcdf"
	gcc $NETCDFI $LIBLSBI $no_data_during_redirect -Wall -fPIC -shared -std=c99 -O2 -o lib/libdvl.so clientlib/*.c clientlib/extended_api/*.c -ldl $NETCDFL $LIBLSBL -lnetcdf
	echo "Building lib/libdvlmt.so for netcdf (multithreading-aware for multithreaded clients; note: netcdf itself is *not* thread-safe; client application must handle that)"
	gcc $NETCDFI $LIBLSBI $no_data_during_redirect -Wall -Wno-unused-variable -Wno-unused-but-set-variable -fPIC -shared -std=c99 -O2 -D__MT__ -o lib/libdvlmt.so clientlib/*.c clientlib/extended_api/*.c -ldl $NETCDFL $LIBLSBL -lnetcdf -pthread
fi

# DVLib for HDF5
if [ "$SDAVI_BUILD_FOR_HDF5" = "YES" ]; then
	echo "Building lib/libdvlh8.so for HDF5 v1.8.16 for FLASH simulator"
	gcc $HDF5_8I $LIBLSBI -Wall -fPIC -shared -std=c99 -O2 -D__HDF5__ -D__HDF5_1_8__ -D__FLASH__ -o lib/libdvlh8.so clientlib/hdf5/*.c clientlib/dvl.c clientlib/dvl_proxy.c clientlib/dvl_rdma.c clientlib/extended_api/*.c -ldl $HDF5_8L $LIBLSBL -lhdf5
	echo "Building lib/libdvlh10.so for HDF5 v1.10 for h5py"
	gcc $HDF5_10I $LIBLSBI -Wall -fPIC -shared -std=c99 -O2 -D__HDF5__ -D__HDF5_1_10__ -D__FLASH__ -o lib/libdvlh10.so clientlib/hdf5/*.c clientlib/dvl.c clientlib/dvl_proxy.c clientlib/dvl_rdma.c clientlib/extended_api/*.c -ldl $HDF5_10L $LIBLSBL -lhdf5
    echo "Building lib/libhdf5profiler8.so for HDF5 v1.8.16 for FLASH simulator"
    gcc $HDF5_8I $LIBLSBI -Wall -fPIC -shared -std=c99 -O2 -D__HDF5_1_8__ -o lib/libhdf5profiler8.so hdf5_profiler/hdf5_bind.c -ldl $HDF5_8L $LIBLSBL -lhdf5
    echo "Building lib/libhdf5profiler10.so for HDF5 v1.10 for h5py"
    gcc $HDF5_10I $LIBLSBI -Wall -fPIC -shared -std=c99 -O2 -D__HDF5_1_10__ -o lib/libhdf5profiler10.so hdf5_profiler/hdf5_bind.c -ldl $HDF5_10L $LIBLSBL -lhdf5
fi

# copy the python module
echo "Installing the extended API python module"
cp clientlib/extended_api/dvl_extended_api.py lib/
# note: we will also copy it to other locations where python programs are using it
# -> easier found if locally in the same folder

. distribute_extended_api_python_module.sh

#
