cmake_minimum_required(VERSION 2.6)

project(gm_tmysql)

# haza's prefix/suffix
set(CMAKE_SHARED_LIBRARY_PREFIX "")

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(CMAKE_SHARED_LIBRARY_SUFFIX "_win32.dll")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(CMAKE_SHARED_LIBRARY_SUFFIX "_darwin.dll")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(CMAKE_SHARED_LIBRARY_SUFFIX "_linux.dll")
endif(CMAKE_SYSTEM_NAME MATCHES "Windows")

add_definitions(-DGMOD_BETA -DNO_SDK -D_LINUX -DBOOST_DATE_TIME_NO_LIB -DBOOST_REGEX_NO_LIB -std=c++0x)

find_package(Boost 1.44)
find_path(MYSQL_INCLUDE_DIR NAMES mysql.h)
find_path(ASIO_INCLUDE_DIR NAMES asio.h)

if(MSVC)
	find_path(MYSQL_LINK_DIR NAMES libmysql.lib)
else()
	find_path(MYSQL_LINK_DIR NAMES libmysqlclient.a)

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
endif()

include_directories("${MYSQL_INCLUDE_DIR}" "${ASIO_INCLUDE_DIR}" "common")
link_directories("${MYSQL_LINK_DIR}")

add_library(gm_tmysql MODULE database.cpp gm_tmysql.cpp ILuaInterface.cpp ILuaModuleManager.cpp ILuaObject.cpp)


if(MSVC)
	target_link_libraries(gm_tmysql libmysql)
else()
	target_link_libraries(gm_tmysql mysqlclient.a dl z pthread)
endif()
