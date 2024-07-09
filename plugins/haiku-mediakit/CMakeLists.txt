cmake_minimum_required(VERSION 3.22...3.25)

if(NOT OS_HAIKU)
  target_disable(haiku-mediakit)
  return()
endif()

add_library(haiku-mediakit MODULE)
add_library(OBS::mediakit ALIAS haiku-mediakit)

target_sources(haiku-mediakit PRIVATE # cmake-format: sortable
                                        haiku-mediakit.cpp haiku-input.cpp)
target_link_libraries(haiku-mediakit PRIVATE OBS::libobs)

set_target_properties_obs(haiku-mediakit PROPERTIES FOLDER plugins PREFIX "")
