# - Try to find EUTelescope
# Once done this will define
#  EUTelescope_FOUND - System has EUTelescope
#  EUTelescope_INCLUDE_DIRS - The EUTelescope include directories
#  EUTelescope_LIBRARIES - The libraries needed to use EUTelescope

find_path(EUTelescope_INCLUDE_DIR EUTELESCOPE.h HINTS $ENV{EUTELESCOPE}/include)
find_library(EUTelescope_LIBRARY NAMES libEutelescope.so HINTS $ENV{EUTELESCOPE}/lib $ENV{EUTELESCOPE}/build/lib)
include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBXML2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(EUTelescope DEFAULT_MSG
	                          EUTelescope_LIBRARY EUTelescope_INCLUDE_DIR)
mark_as_advanced(EUTelescope_LIBRARY EUTelescope_INCLUDE_DIR)

set(EUTelescope_LIBRARIES ${EUTelescope_LIBRARY})
set(EUTelescope_INCLUDE_DIRS ${EUTelescope_INCLUDE_DIR})
