cmake_minimum_required(VERSION 3.0)
project(dv)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_CXX_FLAGS_DEBUG "-std=c++14 -g -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-std=c++14 -Wall -O2")

link_directories($ENV{LUA_LIB_PATH})
include_directories($ENV{LUA_INCLUDE_PATH})

set(LUA_INCLUDES lua/lua.hpp lua/lua.h lua/lualib.h lua/lauxlib.h lua/luaconf.h)
set(TOOLBOX toolbox/FileSystemHelper.cpp toolbox/FileSystemHelper.h toolbox/KeyValueStore.cpp toolbox/KeyValueStore.h toolbox/LinkedMap.cpp toolbox/LinkedMap.h toolbox/LuaWrapper.cpp toolbox/LuaWrapper.h ${LUA_INCLUDES} toolbox/StatisticsHelper.cpp toolbox/StatisticsHelper.h toolbox/StringHelper.cpp toolbox/StringHelper.h toolbox/TextTemplate.cpp toolbox/TextTemplate.h toolbox/TimeHelper.cpp toolbox/TimeHelper.h toolbox/Version.cpp toolbox/Version.h toolbox/Logger.h toolbox/Logger.cpp toolbox/NetworkHelper.h toolbox/NetworkHelper.cpp)
add_library(toolbox ${TOOLBOX})

set(BLOCK_CACHES )
set(FILE_CACHES caches/filecaches/FileCache.cpp caches/filecaches/FileCache.h caches/filecaches/FileDescriptor.cpp caches/filecaches/FileDescriptor.h caches/filecaches/VariableDescriptor.cpp caches/filecaches/VariableDescriptor.h caches/filecaches/FileCacheUnlimited.cpp caches/filecaches/FileCacheUnlimited.h caches/filecaches/FileCacheLRU.cpp caches/filecaches/FileCacheLRU.h caches/filecaches/FileCacheBCL.cpp caches/filecaches/FileCacheBCL.h caches/filecaches/FileCacheDCL.cpp caches/filecaches/FileCacheDCL.h caches/filecaches/FileCachePartitionAwareBase.cpp caches/filecaches/FileCachePartitionAwareBase.h caches/filecaches/FileCachePBCL.cpp caches/filecaches/FileCachePBCL.h caches/filecaches/FileCachePDCL.cpp caches/filecaches/FileCachePDCL.h caches/filecaches/FileCachePLRU.cpp caches/filecaches/FileCachePLRU.h caches/filecaches/FileCacheLIRS.cpp caches/filecaches/FileCacheLIRS.h caches/filecaches/FileCacheARC.cpp caches/filecaches/FileCacheARC.h caches/filecaches/FileCacheFifoWrapper.cpp caches/filecaches/FileCacheFifoWrapper.h)
set(CACHES caches/FileCollection.cpp caches/FileCollection.h caches/RestartFiles.cpp caches/RestartFiles.h ${BLOCK_CACHES} ${FILE_CACHES})
add_library(caches ${CACHES})

set(SIMULATOR simulator/Simulator.cpp simulator/Simulator.h simulator/SimJob.cpp simulator/SimJob.h)
add_library(simulator ${SIMULATOR})

set(COMMON_LISTENERS server/common_listeners/HelloMessageHandler.cpp server/common_listeners/HelloMessageHandler.h server/common_listeners/StopServerMessageHandler.cpp server/common_listeners/StopServerMessageHandler.h server/common_listeners/StatusRequestMessageHandler.cpp server/common_listeners/StatusRequestMessageHandler.h server/common_listeners/ExtendedApiMessageHandler.cpp server/common_listeners/ExtendedApiMessageHandler.h)
set(CLIENT_LISTENERS server/client_listeners/ClientFileOpenMessageHandler.cpp server/client_listeners/ClientFileOpenMessageHandler.h server/client_listeners/ClientFileCloseMessageHandler.cpp server/client_listeners/ClientFileCloseMessageHandler.h server/client_listeners/ClientVariableGetMessageHandler.cpp server/client_listeners/ClientVariableGetMessageHandler.h)
set(SIMULATOR_LISTENERS server/simulator_listeners/SimulatorFileCreateMessageHandler.cpp server/simulator_listeners/SimulatorFileCreateMessageHandler.h server/simulator_listeners/SimulatorFileCloseMessageHandler.cpp server/simulator_listeners/SimulatorFileCloseMessageHandler.h server/simulator_listeners/SimulatorVariablePutMessageHandler.cpp server/simulator_listeners/SimulatorVariablePutMessageHandler.h server/simulator_listeners/SimulatorFinalizeMessageHandler.cpp server/simulator_listeners/SimulatorFinalizeMessageHandler.h server/simulator_listeners/SimulatorCheckpointCreateMessageHandler.cpp server/simulator_listeners/SimulatorCheckpointCreateMessageHandler.h)
set(SERVER server/DV.cpp server/DV.h server/JobQueue.cpp server/JobQueue.h server/MessageHandler.cpp server/MessageHandler.h server/MessageHandlerFactory.cpp server/MessageHandlerFactory.h server/Profiler.cpp server/Profiler.h server/ClientDescriptor.cpp server/ClientDescriptor.h server/PrefetchContext.cpp server/PrefetchContext.h server/DVConfig.cpp server/DVConfig.h server/DVStats.cpp server/DVStats.h ${COMMON_LISTENERS} ${CLIENT_LISTENERS} ${SIMULATOR_LISTENERS})
add_library(server ${SERVER})

set(GETOPT getopt/dv_cmdline_wrapper.cpp dv.h)

set(DV simfs.cpp dv.cpp dv.h DVLog.h DVBasicTypes.h DVForwardDeclarations.h ${SERVER} ${CACHES} ${SIMULATOR} ${TOOLBOX} ${GETOPT})
add_executable(simfs ${DV})
target_link_libraries(simfs lua)
target_link_libraries(simfs dl)

set(STOP_DV stop_dv.cpp toolbox/StringHelper.cpp toolbox/StringHelper.h toolbox/Version.cpp toolbox/Version.h)
add_executable(stop_dv ${STOP_DV})

set(DV_STATUS dv_status.cpp toolbox/KeyValueStore.cpp toolbox/KeyValueStore.h toolbox/StringHelper.cpp toolbox/StringHelper.h toolbox/Version.cpp toolbox/Version.h)
add_executable(dv_status ${DV_STATUS})

set(CHECK_DV_CONFIG_FILE check_dv_config_file.cpp server/DVConfig.cpp server/DVConfig.h ${TOOLBOX})
add_executable(check_dv_config_file ${CHECK_DV_CONFIG_FILE})
target_link_libraries(check_dv_config_file lua)
target_link_libraries(check_dv_config_file dl)


set(SIMFS_WORKSPACE ${SIMFS_WORKSPACE_PATH})
set(SIMFS_INSTALL_PATH ${SIMFS_INSTALL_PATH})
set(SIMFS_DVLIB ${SIMFS_DVLIB_PATH})

configure_file(${PROJECT_SOURCE_DIR}/config.h.in ${PROJECT_BINARY_DIR}/config.h)
include_directories(${PROJECT_BINARY_DIR})
