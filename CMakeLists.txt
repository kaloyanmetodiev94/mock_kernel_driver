cmake_minimum_required(VERSION 3.10 FATAL_ERROR) #not really tested, but how different can it be (i am using 3.17.5)

project(mock_kernel_driver VERSION 1.0.0 LANGUAGES C)
add_executable(client client/client.c)
add_executable(server server/server.c)

if(CMAKE_C_COMPILER_VERSION VERSION_LESS 9.3.1) #it is what I was using
	message(FATAL_ERROR "Insufficient gcc version. Found ${CMAKE_C_COMPILER_VERSION}")
endif()

add_definitions(-D__KERNEL__ -DMODULE)

execute_process(
        COMMAND uname -r
        OUTPUT_VARIABLE KERNEL_RELEASE
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Find the kernel devel files
find_path(
        KERNELHEADERS_DIR
        /Makefile
        PATHS /lib/modules/${KERNEL_RELEASE}/build
)

message(STATUS "Kernel release: ${KERNEL_RELEASE}")
message(STATUS "Kernel devel files: ${KERNELHEADERS_DIR}")


set(DRIVER_FILE chardev_enduro.ko)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/kernel_chardev/chardev_enduro.c ${CMAKE_CURRENT_BINARY_DIR}/kernel_chardev/chardev_enduro.c COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/kernel_chardev/Makefile ${CMAKE_CURRENT_BINARY_DIR}/kernel_chardev/Makefile COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/kernel_chardev/load_driver ${CMAKE_CURRENT_BINARY_DIR}/load_driver COPYONLY)

add_custom_command(OUTPUT ${DRIVER_FILE}
        COMMAND make all
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/kernel_chardev
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/kernel_chardev/chardev_enduro.c )

add_custom_target(driver ALL DEPENDS ${DRIVER_FILE})

