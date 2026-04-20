
set(GMP_DEFINITIONS)

find_path(GMP_INCLUDE_DIR gmp.h
  PATHS ENV GMP_INCLUDE ENV GMP_DIR 
  NO_DEFAULT_PATH
)
find_path(GMP_INCLUDE_DIR gmp.h)
mark_as_advanced(GMP_INCLUDE_DIR)
set(GMP_INCLUDE_DIRS ${GMP_INCLUDE_DIR})

find_library(GMP_LIBRARY NAMES gmp mpir
  HINTS ENV GMP_LIB ENV GMP_DIR
  NO_DEFAULT_PATH
)
find_library(GMP_LIBRARY NAMES gmp mpir)
mark_as_advanced(GMP_LIBRARY)
set(GMP_LIBRARIES ${GMP_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gmp DEFAULT_MSG GMP_LIBRARY GMP_INCLUDE_DIR)
