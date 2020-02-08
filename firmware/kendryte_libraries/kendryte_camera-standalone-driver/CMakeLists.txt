
# DO NOT MODIFY THIS FILE, IT WILL BE OVERRIDE!!!

# [section] Head
##### include(reset) #####
### reset basic var
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

# set(CMAKE_SYSTEM "Generic")
set(CMAKE_SYSTEM_NAME "Generic")

# set(CMAKE_HOST_SYSTEM "Generic")
# set(CMAKE_HOST_SYSTEM_NAME "Generic")
#
set(CMAKE_CROSSCOMPILING TRUE)
# set(CMAKE_C_COMPILER_FORCED TRUE)
# set(CMAKE_CXX_COMPILER_FORCED TRUE)
# set(CMAKE_ASM_COMPILER_FORCED TRUE)
#

set(SOURCE_FILES "")
set(LDFLAGS "")
set(CMAKE_EXE_LINKER_FLAGS "")
set(CMAKE_SHARED_LINKER_FLAGS "")
set(CMAKE_MODULE_LINKER_FLAGS "")


cmake_minimum_required(VERSION 3.0.0)
set(PROJECT_NAME "kendryte_camera-standalone-driver")
# [/section] Head

# [section] Init
# not need in sub project
# debug mode enabled
set(-DCMAKE_VERBOSE_MAKEFILE TRUE)
set_property(GLOBAL PROPERTY JOB_POOLS single_debug=1)
# [/section] Init

# [section] Project
##### include(fix9985) #####
### http://www.cmake.org/Bug/view.php?id=9985
string(REPLACE "-rdynamic" "" _C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS}")
string(REPLACE "-rdynamic" "" _CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS}")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${_C_FLAGS}")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${_CXX_FLAGS}")


message("======== PROJECT:${PROJECT_NAME} ========")
project(${PROJECT_NAME})

## [section] Header
### from project local
include_directories(
  "${CMAKE_CURRENT_LIST_DIR}/src"
)### from ide
include_directories("${CMAKE_CURRENT_LIST_DIR}/config")
## from project public
include_directories(
  "${CMAKE_CURRENT_LIST_DIR}/include"
  "${CMAKE_CURRENT_LIST_DIR}/../kendryte_standalone-sdk/lib/bsp/include"
  "${CMAKE_CURRENT_LIST_DIR}/../kendryte_standalone-sdk/lib/drivers/include"
  "${CMAKE_CURRENT_LIST_DIR}/../kendryte_standalone-sdk/lib/utils/include"
  "${CMAKE_CURRENT_LIST_DIR}/../kendryte_standalone-sdk/third_party/xtl/include"
  "${CMAKE_CURRENT_LIST_DIR}/../kendryte_standalone-sdk/lib/nncase/include"
)
## [/section] Header
## [section] Source
## add source from config json (2 files matched)
add_source_file(src/camera.c)
add_source_file(src/ov5640.c)
## [/section] Source
# [/section] Project

# [section] Custom

# [/section] Custom
# [section] Target
## final create kendryte_camera-standalone-driver library
add_library(${PROJECT_NAME} SHARED STATIC ${SOURCE_FILES})
target_compile_definitions(${PROJECT_NAME} PRIVATE "PROJECT_PATH=${CMAKE_CURRENT_LIST_DIR}/")
### [section] Custom2

### [/section] Custom2
# debug mode enabled
set_property(TARGET ${PROJECT_NAME} PROPERTY JOB_POOL_COMPILE single_debug)
set_property(TARGET ${PROJECT_NAME} PROPERTY JOB_POOL_LINK single_debug)
## no properties
## set definitions
add_compile_definitions(TCB_SPAN_NO_EXCEPTIONS)
add_compile_definitions(TCB_SPAN_NO_CONTRACT_CHECKING)
add_compile_definitions(NNCASE_TARGET=k210)
add_compile_definitions(KENDRYTE_SDK_TYPE=1)
add_compile_definitions(KENDRYTE_SDK_TYPE_STANDALONE=1)
add_compile_definitions(KENDRYTE_SDK_TYPE_FREERTOS=2)
add_compile_definitions(KENDRYTE_SDK_RELEASE_DATE=20190906195432)
add_compile_definitions(SPI_DEVICE_INDEX=SPI_DEVICE_1)
# [/section] Target

# [section] Dependency
cmake_policy(SET CMP0079 NEW)
## add simple folder dependency
# [/section] Dependency


# [section] C/C++ compiler flags

##### flags from config json #####
message("config flags for ${PROJECT_NAME}")

# [/section] C/C++ compiler flags

# [section] Finish
##### include(afterProject) #####
removeDuplicateSubstring("${CMAKE_C_FLAGS}" CMAKE_C_FLAGS)
removeDuplicateSubstring("${CMAKE_CXX_FLAGS}" CMAKE_CXX_FLAGS)


##### include(fix9985) #####
### http://www.cmake.org/Bug/view.php?id=9985
string(REPLACE "-rdynamic" "" _C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS}")
string(REPLACE "-rdynamic" "" _CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS}")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${_C_FLAGS}")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${_CXX_FLAGS}")



# [/section] Finish

# [section] Dump Setting
##### include(dumpConfig) #####
set(CMAKE_SYSTEM_NAME "Generic")

message("")
message("Project: ${PROJECT_NAME}")
message("  LIST_FILE=${CMAKE_PARENT_LIST_FILE}")
message("  TOOLCHAIN=${TOOLCHAIN}")
message("  KENDRYTE_IDE=${KENDRYTE_IDE}")
message("  BUILDING_SDK=${BUILDING_SDK}")
message("  _CMAKE_TOOLCHAIN_PREFIX=${_CMAKE_TOOLCHAIN_PREFIX}")
message("  _CMAKE_TOOLCHAIN_LOCATION=${_CMAKE_TOOLCHAIN_LOCATION}")


message("")
message("  CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
message("  CMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
message("  CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
message("  CMAKE_LINKER=${CMAKE_LINKER}")
message("  CMAKE_OBJCOPY=${CMAKE_OBJCOPY}")
message("  CMAKE_OBJDUMP=${CMAKE_OBJDUMP}")
message("  CMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}")

message("")
message("  CMAKE_C_FLAGS=${CMAKE_C_FLAGS}")
message("  CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")
message("  LDFLAGS=${LDFLAGS}")
message("  LINK_FLAGS=${LINK_FLAGS}")
message("  CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")

message("Makefile created.")
message("")
message("")


message("
  ${PROJECT_NAME} :: SOURCE_FILES=${SOURCE_FILES}")
# [/section] Dump Setting
