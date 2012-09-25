#ifdef WIN32
#include <winsock2.h>
#endif

#include <mysql.h>

#include "utlvector.h"
#include "utlstack.h"
#include "jobthread.h"

#ifndef GMOD_BETA
#include "common/GMLuaModule.h"
#else
#include <ILuaModuleManager.h>
#endif
#include "database.h"

#include "memdbgon.h"