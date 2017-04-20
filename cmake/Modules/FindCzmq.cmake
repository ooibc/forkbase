
FIND_PATH(Czmq_INCLUDE_DIR NAMES czmq.h PATHS "$ENV{Czmq_ROOT_DIR}/include")
FIND_LIBRARY(Czmq_LIBRARIES NAMES czmq PATHS "$ENV{Czmq_ROOT_DIR}/lib")

INCLUDE(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Czmq DEFAULT_MSG Czmq_INCLUDE_DIR Czmq_LIBRARIES)

IF(CZMQ_FOUND)
    MESSAGE(STATUS "Found czmq at ${Czmq_LIBRARIES}")
    MARK_AS_ADVANCED(Czmq_INCLUDE_DIR Czmq_LIBRARIES)
ELSE()
    MESSAGE(FATAL_ERROR "Czmq NOT FOUND")
ENDIF()
