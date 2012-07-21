cmake_minimum_required(VERSION 2.6)

project(gm_tmysql)

find_path(SDK_DIR notes.txt)
find_path(MYSQL_INCLUDE_DIR NAMES mysql.h)

find_package( Boost 1.44 COMPONENTS thread )

if(MSVC)
	find_path(MYSQL_LINK_DIR NAMES libmysql.lib)
else()
	find_path(MYSQL_LINK_DIR NAMES libmysqlclient.a)
endif()

include_directories("common/threadpool")
include_directories("${MYSQL_INCLUDE_DIR}")
link_directories("${MYSQL_LINK_DIR}")

add_library(gm_tmysql MODULE database.cpp gm_tmysql.cpp)

#target_link_libraries(gm_tmysql ${Boost_LIBRARIES})
if(MSVC)
	target_link_libraries(gm_tmysql libmysql)
else()	
	target_link_libraries(gm_tmysql mysqlclient)
endif()
