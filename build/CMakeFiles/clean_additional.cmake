# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  [[CMakeFiles\ReplayQt_autogen.dir\AutogenUsed.txt]]
  [[CMakeFiles\ReplayQt_autogen.dir\ParseCache.txt]]
  "ReplayQt_autogen"
  )
endif()
