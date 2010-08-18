cmake_minimum_required(VERSION 2.6)

project(serverplugin_serversecure2)

find_path(SDK_DIR NAMES)

if(MSVC)
	link_directories("${SDK_DIR}/lib/public")
else()
	link_directories("${SDK_DIR}/lib/linux")
	
	add_definitions(-D_LINUX -DLINUX -mtune=i686 -march=pentium3 -mmmx -m32 -DVPROF_LEVEL=1 -DSWDS -D_finite=finite -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -Usprintf -Ustrncpy -UPROTECTED_THINGS_ENABLE)
endif()

include_directories("${SDK_DIR}/public" "${SDK_DIR}/public/tier1" "${SDK_DIR}/public/tier0" "${TBB_DIR}/include")

add_library(serverplugin_serversecure2 MODULE filecheck.cpp netfilter.cpp Serverplugin_serversecure2.cpp validation.cpp)


if(MSVC)
	target_link_libraries(serverplugin_serversecure2 tier0 tier1 mathlib)
else()
	target_link_libraries(serverplugin_serversecure2 tier0)
	target_link_libraries(serverplugin_serversecure2 ${SDK_DIR}/lib/linux/tier1_i486.a ${SDK_DIR}/lib/linux/mathlib_i486.a)
endif()