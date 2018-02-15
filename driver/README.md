
SDaVi: DV Server C++ implementation
====================================

This is the C++ implementation of the initial Python version of the SDaVi DV Server.
It conforms to the full outside API of the initial Python version and can be used as
a direct replacement. However, some internal things have been adjusted to already
incorporate some improvements and to allow better updating of the code in
future versions.

The server can be stopped with the new ```stop_dv``` command (see below),
by ```SIGINT (Ctrl-C)``` and ```SIGTERM (kill pid)```. All three variants lead to the
same graceful shutdown including writing final log messages.

The code is written in C++14 and can be compiled by using CMake. CLion was used as IDE,
but CMakeList.txt should allow other IDEs to import the project as well.



Dependencies
------------
- Current C++14 compiler with standard library. We have tested various versions of gcc
(starting with v4.9.2) and Clang.
- Lua. Version 5.3.4 is already included in the source code here. Lua has MIT license.
https://www.lua.org/
- Lua needs ```readline``` and ```readline-devel``` packages installed on Linux.



Build
-----
A compilation script ```compile_dv_on_x86_and_cray.sh``` is in the ```$SDAVI_DIR``` folder
that handles all compilation of dv including copying of the needed template and config files,
into the folder ```./bin```. For compilation on macOS, use ```mac``` as argument to the
compilation script.



Location of configuration and template files
--------------------------------------------
- configuration files (suffix .dv) are in the folder ```$SDAVI_DIR/dv_config_files```.
They are modified Lua scripts that conform to the specified interface needed by dv.
See examples.
- template files for job generation are in folder ```$SDAVI_DIR/templates```.

Both sets of files are copied to ```$SDAVI_DIR/bin``` during compilation.



Style
-----
The code follows the Google C++ Style Guide with only few modifications
https://google.github.io/styleguide/cppguide.html

The style guide is in particular also applied to the use of references and pointers:
All passed reference types must be const, and types must be passed as pointers
if modifications could happen.



Additional note re pointers / memory management and ownership
-------------------------------------------------------------
Memory management is handled by using RAII, mainly automatic object allocation on the
stack, and ```unique_ptr<>``` smart pointers for dynamic memory allocation on the heap.

Raw pointers are used at various locations on purpose. They **never** interfere with
the life-cycle of the pointee objects. Ownership shall **never** be assumed!

While nullptr is avoided where possible, lookup methods e.g. for simjobs or
file/client descriptors may return nullptr, of course, to indicate that the element was
not found. Test for nullptr **must** be applied before accessing their fields/methods.

On the other hand, nullptr shall **never** be passed as argument to functions.



Produced binaries
-----------------
```dv [options] <config_file>``` starts the DV server. Configuration is based on ```.dv``` type configuration
files, ENV variables (as defined in the script), and optional command line arguments.
Command line arguments override options set by config file or ENV variable.
Use ```./dv --help``` for details on optional arguments.

```stop_dv <DV_IP_address> <DV_port>``` stops the DV server including writing final
log outputs and cleanup.

```dv_status <DV_IP_address> <DV_port>``` retrieves some key information from the running DV server.

```check_dv_config_file <DV config file>``` runs user-defined checks within the
config file as defined in the API.


Original Python implementation
------------------------------
Original Python implementation by Salvatore Di Girolamo
with contributions by Pirmin Schmid (mainly FLASH and Cache variants).



Version
-------
v0.9 May 2017, Pirmin Schmid and Salvatore Di Girolamo
