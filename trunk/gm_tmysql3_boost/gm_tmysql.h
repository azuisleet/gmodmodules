#ifdef WIN32
#include <winsock2.h>
#endif

#include <mysql.h>
#include <threadpool.hpp>
#include <boost/format.hpp>

using namespace boost::threadpool;
using namespace boost::threadpool::detail;
using namespace boost;

#include "common/GMLuaModule.h"
#include "database.h"