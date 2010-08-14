cmake_minimum_required(VERSION 2.6)

project(gm_tmysql)

# haza's prefix/suffix
set(CMAKE_SHARED_LIBRARY_PREFIX "")

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
set(CMAKE_SHARED_LIBRARY_SUFFIX "_darwin.dll")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
set(CMAKE_SHARED_LIBRARY_SUFFIX "_linux.dll")
endif(CMAKE_SYSTEM_NAME MATCHES "Windows")


find_path(SDK_DIR notes.txt)
find_path(MYSQL_INCLUDE_DIR NAMES mysql.h)

if(MSVC)
	link_directories("${SDK_DIR}/lib/public")
	
	find_path(MYSQL_LINK_DIR NAMES libmysql.lib)
else()
	link_directories("${SDK_DIR}/lib/linux")
	add_definitions(-D_LINUX -DLINUX -mtune=i686 -march=pentium3 -mmmx -m32 -DVPROF_LEVEL=1 -DSWDS -D_finite=finite -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -Usprintf -Ustrncpy -UPROTECTED_THINGS_ENABLE)

	find_path(MYSQL_LINK_DIR NAMES libmysqlclient.a)
endif()

include_directories("${SDK_DIR}/public" "${SDK_DIR}/public/tier1" "${SDK_DIR}/public/tier0" "${SDK_DIR}/public/vstdlib")
include_directories("${MYSQL_INCLUDE_DIR}")
link_directories("${MYSQL_LINK_DIR}")

add_library(gm_tmysql MODULE database.cpp gm_tmysql.cpp)


if(MSVC)
	target_link_libraries(gm_tmysql tier0 tier1 vstdlib)
	
	target_link_libraries(gm_tmysql libmysql)
else()
	target_link_libraries(gm_tmysql tier0 vstdlib)
	target_link_libraries(gm_tmysql ${SDK_DIR}/lib/linux/tier1_i486.a)
	
	target_link_libraries(gm_tmysql mysqlclient)
endif()