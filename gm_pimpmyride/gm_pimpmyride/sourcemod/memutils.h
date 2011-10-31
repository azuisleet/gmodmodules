#ifndef _INCLUDE_MEMUTILS_H_
#define _INCLUDE_MEMUTILS_H_

#ifndef NULL
#define NULL (void*)0
#endif

#ifdef _LINUX

#include <sys/mman.h>

#include "sh_vector.h"
#include "sm_symtable.h"

typedef uint32_t Elf32_Addr;

struct LibSymbolTable
{
	SymbolTable table;
	Elf32_Addr lib_base;
	uint32_t last_pos;
};

extern CVector<LibSymbolTable *> g_SymTables;

#endif // _LINUX

void *ResolveSymbol(void *handle, const char *symbol);

#endif//_INCLUDE_MEMUTILS_H_
