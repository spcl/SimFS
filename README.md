**Dependencies:**
  - CMake
  - Clang
  - LibLua (>=5.3.4): https://www.lua.org/ftp/lua-5.3.4.tar.gz

**Install:**
  1) Install depenencies

  2) Build DV 
    - `./build_dv.sh` to install in current_dir/build. Add build/bin/ to your $PATH to have the `simfs` command available.
    - `sudo ./build_dv.sh install` to install in /usr/bin
    
  3) Build DVLib
    - `./build_dvlib_x86.sh [--netcdf <netcdf path>] [--hdf5_108 <HDF5 1.8 path>] [--hdf5_110 <HDF5 1.10 path>] [--liblsb <liblsb path>]`
    - Note: need only netcdf for the heat equation example

**Configure the heat equation example:**

  1) Create restart files
    - `cd examples/heatequation/simulator`
    - Use `create_restart_files.py` to create the restart files. E.g., 
      `python ./create_restart_files.py -i 2000 -s 10` creates restart files every 10 
       output step until the output step 2000.

  2) Initialize `heat` environment:
    - `cd examples/heatequation/simulator` 
    - `simfs heat init ../../../dv_config_files/heatequation.dv`
    - Note the heatequation.dv file is created by the DV build process.


**Run the heat equation example:**

  1) Run DV
    - `simfs heat start`
    - If you get a binding error, then specify different ports with: `simfs heat start -C <client_port> -S <server_port>

  2) Run heat equation visulizer 
    - `cd examples/heatequation/client`
    - `simfs heat run python reader2.py`

 **Debug:**
   - `export SIMFS_LOG_LEVEL=3`
   - `export SIMFS_LOG="ERROR,INFO,CLIENT,SIMULATOR,CACHE,PREFETCHER"`

    
