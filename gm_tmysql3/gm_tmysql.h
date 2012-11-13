#ifdef WIN32
#include <winsock2.h>
#endif

#include <mysql.h>

#ifndef GMOD_BETA
#include "common/GMLuaModule.h"
#else
#include <ILuaModuleManager.h>
#endif
#include "database.h"