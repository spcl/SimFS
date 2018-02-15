module switch PrgEnv-cray PrgEnv-gnu 2> /dev/null

current_path=$(pwd)
code_source_path=$current_path/src/dv
target_path=$current_path/bin
build_path=$current_path/build

mkdir -p $target_path
mkdir -p $build_path

if [ -z "$LUA_PATH" ]; then
    LUA_PATH=$current_path/lua
fi

export LUA_LIB_PATH=$LUA_PATH/lib
export LUA_INCLUDE_PATH=$LUA_PATH/include


echo "compilation of SDaVi driver dv from $code_source_path in $build_path"
cd $build_path
cmake $code_source_path
make -j 8 dv
#make -j 8 stop_dv
#make -j 8 dv_status
make -j 8 check_dv_config_file

cd $current_path
#
echo "installing binaries to $target_path"
cp $build_path/dv $target_path/
#cp $build_path/stop_dv $target_path/
#cp $build_path/dv_status $target_path/
cp $build_path/check_dv_config_file $target_path/

