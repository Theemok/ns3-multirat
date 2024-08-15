add_library(
  scratch-multirat-lib
  lib/multi-rat-client.cc
  lib/multi-rat-fileprotocol-header.cc
  lib/multi-rat-fileprotocol-helper.cc
  lib/multi-rat-fileprotocol.cc
  lib/multi-rat-header.cc
  lib/multi-rat-protocol.cc
  lib/multi-rat-tag.cc
  lib/multi-rat-utility.cc
  lib/packet-counter.cc
)

build_exec(
  EXECNAME example
  EXECNAME_PREFIX scratch_multirat_
  SOURCE_FILES example.cc
  LIBRARIES_TO_LINK scratch-multirat-lib
                    "${ns3-libs}" "${ns3-contrib-libs}"
  EXECUTABLE_DIRECTORY_PATH ${CMAKE_OUTPUT_DIRECTORY}/scratch/multirat
)

build_exec(
  EXECNAME static-example
  EXECNAME_PREFIX scratch_multirat_
  SOURCE_FILES static-example.cc
  LIBRARIES_TO_LINK scratch-multirat-lib
                    "${ns3-libs}" "${ns3-contrib-libs}"
  EXECUTABLE_DIRECTORY_PATH ${CMAKE_OUTPUT_DIRECTORY}/scratch/multirat
)

build_exec(
  EXECNAME experiments
  EXECNAME_PREFIX scratch_multirat_
  SOURCE_FILES experiments.cc
  LIBRARIES_TO_LINK scratch-multirat-lib
                    "${ns3-libs}" "${ns3-contrib-libs}"
  EXECUTABLE_DIRECTORY_PATH ${CMAKE_OUTPUT_DIRECTORY}/scratch/multirat
)

build_exec(
  EXECNAME dynamic-routes
  EXECNAME_PREFIX scratch_multirat_
  SOURCE_FILES dynamic-routes.cc
  LIBRARIES_TO_LINK scratch-multirat-lib
                    "${ns3-libs}" "${ns3-contrib-libs}"
  EXECUTABLE_DIRECTORY_PATH ${CMAKE_OUTPUT_DIRECTORY}/scratch/multirat
)