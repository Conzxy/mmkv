# In other platform(eg. windows), generating base library only.
if (UNIX AND NOT APPLE)
  message(STATUS "Mmkv on unix/linux/solaris/... platform")
  set(MMKV_ON_UNIX ON)
else ()
  set(MMKV_ON_UNIX OFF)
endif ()

if (WIN32)
  message(STATUS "Mmkv on windows")
  set(MMKV_ON_WIN ON)
endif ()

