#This file is taken from here https://gitlab.ralph.or.at/causal-rt/causal-cpp/, it was GPLv3 license
#Thank you so much, mr. Ralph Alexander Bariz, I hope you don't mind me using your code

# Try to find LMDB headers and library.
#
# Usage of this module as follows:
#
#     find_package(LMDB)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  LMDB_ROOT_DIR          Set this variable to the root installation of
#                            LMDB if the module has problems finding the
#                            proper installation path.
#
# Variables defined by this module:
#
#  LMDB_FOUND               System has LMDB library/headers.
#  LMDB_LIBRARIES           The LMDB library.
#  LMDB_INCLUDE_DIRS        The location of LMDB headers.

find_path(LMDB_ROOT_DIR
    NAMES include/lmdb.h
)

find_library(LMDB_LIBRARIES
    NAMES lmdb
    HINTS ${LMDB_ROOT_DIR}/lib
)

find_path(LMDB_INCLUDE_DIRS
    NAMES lmdb.h
    HINTS ${LMDB_ROOT_DIR}/include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LMDB DEFAULT_MSG
    LMDB_LIBRARIES
    LMDB_INCLUDE_DIRS
)

mark_as_advanced(
    LMDB_ROOT_DIR
    LMDB_LIBRARIES
    LMDB_INCLUDE_DIRS
)
