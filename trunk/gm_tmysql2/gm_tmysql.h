#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <winsock2.h>
#include <mysql.h>
#include <errmsg.h>
#include <ctime>

#define NO_MALLOC_OVERRIDE
#define _RETAIL
#include "common/GMLuaModule.h"
#include "mutex.h"
#include "list.h"
#include "database.h"