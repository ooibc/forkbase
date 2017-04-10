
FIND_PATH(GFLAGS_INCLUDE_DIR NAMES gflags/gflags.h PATHS "$ENV{GFLAGS_DIR}/include")
FIND_LIBRARY(GFLAGS_LIBRARIES NAMES gflags)

INCLUDE(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GFLAGS DEFAULT_MSG GFLAGS_INCLUDE_DIR GFLAGS_LIBRARIES)

IF(GFLAGS_FOUND)
    #    MESSAGE(STATUS "Found gflags at ${GFLAGS_INCLUDE_DIR}")
    MARK_AS_ADVANCED(GFLAGS_INCLUDE_DIR GFLAGS_LIBRARIES)
ENDIF()
