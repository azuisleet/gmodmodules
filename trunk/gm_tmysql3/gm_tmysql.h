#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <winsock2.h>
#include <mysql.h>
#include <errmsg.h>
#include <ctime>

#include "memdbgon.h"
#include "utlvector.h"
#include "utlstack.h"
#include "jobthread.h"

#include "common/GMLuaModule.h"
#include "database.h"