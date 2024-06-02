find_library(BE be)
find_library(MEDIA media)
find_package(LibUUID REQUIRED)

set(UUID_TEST_SOURCE "#include<uuid/uuid.h>\nint main(){return 0;}")
check_c_source_compiles("${UUID_TEST_SOURCE}" HAVE_UUID_HEADER)

if(NOT HAVE_UUID_HEADER)
  message(FATAL_ERROR "Required system header <uuid/uuid.h> not found.")
endif()

target_link_libraries(
  libobs
  PRIVATE # cmake-format: sortable
          ${BE}
		  ${MEDIA}
          LibUUID::LibUUID)

target_sources(
    libobs
    PRIVATE # cmake-format: sortable
            obs-haiku.c
            util/threading-posix.c
            util/threading-posix.h
            util/pipe-posix.c
            util/platform-nix.c
			audio-monitoring/haiku/haiku-enum-devices.cpp
            audio-monitoring/haiku/haiku-monitoring-available.c
			audio-monitoring/haiku/haiku-output.cpp)

target_compile_definitions(
  libobs PRIVATE OBS_INSTALL_PREFIX="${OBS_INSTALL_PREFIX}" $<$<COMPILE_LANG_AND_ID:C,GNU>:ENABLE_DARRAY_TYPE_TEST>
                 $<$<COMPILE_LANG_AND_ID:CXX,GNU>:ENABLE_DARRAY_TYPE_TEST>)

set_target_properties(libobs PROPERTIES OUTPUT_NAME obs)
