
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
set(PROJECT_NAME "kendryte_standalone-sdk")
# [/section] Head

# [section] Init
# not need in sub project
# debug mode disabled
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
### from ide
include_directories("${CMAKE_CURRENT_LIST_DIR}/config")
## from project public
include_directories(
  "${CMAKE_CURRENT_LIST_DIR}/lib/bsp/include"
  "${CMAKE_CURRENT_LIST_DIR}/lib/drivers/include"
  "${CMAKE_CURRENT_LIST_DIR}/lib/utils/include"
  "${CMAKE_CURRENT_LIST_DIR}/third_party/xtl/include"
  "${CMAKE_CURRENT_LIST_DIR}/lib/nncase/include"
)
## [/section] Header
## [section] Source
## add source from config json (38 files matched)
add_source_file(lib/bsp/crt.S)
add_source_file(lib/bsp/entry.c)
add_source_file(lib/bsp/interrupt.c)
add_source_file(lib/bsp/entry_user.c)
add_source_file(lib/bsp/locks.c)
add_source_file(lib/bsp/sleep.c)
add_source_file(lib/bsp/printf.c)
add_source_file(lib/bsp/syscalls.c)
add_source_file(lib/drivers/aes.c)
add_source_file(lib/drivers/apu.c)
add_source_file(lib/drivers/dmac.c)
add_source_file(lib/drivers/clint.c)
add_source_file(lib/drivers/dvp.c)
add_source_file(lib/drivers/fft.c)
add_source_file(lib/drivers/fpioa.c)
add_source_file(lib/drivers/gpio.c)
add_source_file(lib/drivers/gpiohs.c)
add_source_file(lib/drivers/i2c.c)
add_source_file(lib/drivers/i2s.c)
add_source_file(lib/drivers/kpu.c)
add_source_file(lib/drivers/plic.c)
add_source_file(lib/drivers/pwm.c)
add_source_file(lib/drivers/rtc.c)
add_source_file(lib/drivers/sha256.c)
add_source_file(lib/drivers/spi.c)
add_source_file(lib/drivers/sysctl.c)
add_source_file(lib/drivers/uart.c)
add_source_file(lib/drivers/timer.c)
add_source_file(lib/drivers/uarths.c)
add_source_file(lib/drivers/wdt.c)
add_source_file(lib/drivers/utils.c)
add_source_file(lib/nncase/nncase.cpp)
add_source_file(lib/nncase/runtime/interpreter.cpp)
add_source_file(lib/nncase/runtime/kernel_registry.cpp)
add_source_file(lib/nncase/runtime/cpu/cpu_ops.cpp)
add_source_file(lib/nncase/runtime/k210/interpreter.cpp)
add_source_file(lib/nncase/runtime/k210/k210_ops.cpp)
add_source_file(lib/nncase/runtime/neutral/neutral_ops.cpp)
## [/section] Source
# [/section] Project

# [section] Custom
FILE(GLOB_RECURSE ASSEMBLY_FILES
        "${CMAKE_CURRENT_LIST_DIR}/*.s"
        "${CMAKE_CURRENT_LIST_DIR}/*.S"
)

SET_PROPERTY(SOURCE ${ASSEMBLY_FILES} PROPERTY LANGUAGE C)
SET_SOURCE_FILES_PROPERTIES(${ASSEMBLY_FILES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp -D __riscv64")

# [/section] Custom
# [section] Target
## final create kendryte_standalone-sdk library
add_library(${PROJECT_NAME} SHARED STATIC ${SOURCE_FILES})
target_compile_definitions(${PROJECT_NAME} PRIVATE "PROJECT_PATH=${CMAKE_CURRENT_LIST_DIR}/")
### [section] Custom2

### [/section] Custom2
# debug mode disabled
## set properties
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE "C")
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
# [/section] Dependency


# [section] C/C++ compiler flags

##### flags from config json #####
message("config flags for ${PROJECT_NAME}")
set(FLAGS_FOR_C
  "-std=gnu11"
  "-Wno-pointer-to-int-cast"
  "-Wno-old-style-declaration"
  "-mcmodel=medany"
  "-mabi=lp64f"
  "-march=rv64imafc"
  "-fno-common"
  "-ffunction-sections"
  "-fdata-sections"
  "-fstrict-volatile-bitfields"
  "-fno-zero-initialized-in-bss"
  "-ffast-math"
  "-fno-math-errno"
  "-fsingle-precision-constant"
  "-Os"
  "-ggdb"
  "-Wall"
  "-Werror=all"
  "-Wno-error=unused-function"
  "-Wno-error=unused-but-set-variable"
  "-Wno-error=unused-variable"
  "-Wno-error=deprecated-declarations"
  "-Wno-multichar"
  "-Wextra"
  "-Werror=frame-larger-than=32768"
  "-Wno-unused-parameter"
  "-Wno-sign-compare"
  "-Wno-error=missing-braces"
  "-Wno-error=return-type"
  "-Wno-error=pointer-sign"
  "-Wno-missing-braces"
  "-Wno-strict-aliasing"
  "-Wno-implicit-fallthrough"
  "-Wno-missing-field-initializers"
  "-Wno-int-to-pointer-cast"
  "-Wno-error=comment"
  "-Wno-error=logical-not-parentheses"
  "-Wno-error=duplicate-decl-specifier"
  "-Wno-error=parentheses"
)
set(FLAGS_FOR_CXX
  "-std=gnu++17"
  "-mcmodel=medany"
  "-mabi=lp64f"
  "-march=rv64imafc"
  "-fno-common"
  "-ffunction-sections"
  "-fdata-sections"
  "-fstrict-volatile-bitfields"
  "-fno-zero-initialized-in-bss"
  "-ffast-math"
  "-fno-math-errno"
  "-fsingle-precision-constant"
  "-Os"
  "-ggdb"
  "-Wall"
  "-Werror=all"
  "-Wno-error=unused-function"
  "-Wno-error=unused-but-set-variable"
  "-Wno-error=unused-variable"
  "-Wno-error=deprecated-declarations"
  "-Wno-multichar"
  "-Wextra"
  "-Werror=frame-larger-than=32768"
  "-Wno-unused-parameter"
  "-Wno-sign-compare"
  "-Wno-error=missing-braces"
  "-Wno-error=return-type"
  "-Wno-error=pointer-sign"
  "-Wno-missing-braces"
  "-Wno-strict-aliasing"
  "-Wno-implicit-fallthrough"
  "-Wno-missing-field-initializers"
  "-Wno-int-to-pointer-cast"
  "-Wno-error=comment"
  "-Wno-error=logical-not-parentheses"
  "-Wno-error=duplicate-decl-specifier"
  "-Wno-error=parentheses"
)
target_compile_options(${PROJECT_NAME} PRIVATE
	$<$<COMPILE_LANGUAGE:C>:${FLAGS_FOR_C}>
	$<$<COMPILE_LANGUAGE:CXX>:${FLAGS_FOR_CXX}>
)

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



# [/section] Dump Setting
