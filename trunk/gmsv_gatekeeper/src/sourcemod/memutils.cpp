#include "memutils.h"

#ifdef _LINUX
#include <fcntl.h>
#include <link.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

CVector<LibSymbolTable *> g_SymTables;
#else
#include <windows.h>
#endif

void *ResolveSymbol(void *handle, const char *symbol)
{
#ifdef _WIN32
	return GetProcAddress((HMODULE)handle, symbol);
#elif defined _LINUX
	struct link_map *dlmap;
	struct stat dlstat;
	int dlfile;
	uintptr_t map_base;
	Elf32_Ehdr *file_hdr;
	Elf32_Shdr *sections, *shstrtab_hdr, *symtab_hdr, *strtab_hdr;
	Elf32_Sym *symtab;
	const char *shstrtab, *strtab;
	uint16_t section_count;
	uint32_t symbol_count;
	LibSymbolTable *libtable;
	SymbolTable *table;
	Symbol *symbol_entry;

	dlmap = (struct link_map *)handle;
	symtab_hdr = NULL;
	strtab_hdr = NULL;
	table = NULL;
	
	/* See if we already have a symbol table for this library */
	for (size_t i = 0; i < g_SymTables.size(); i++)
	{
		libtable = g_SymTables[i];
		if (libtable->lib_base == dlmap->l_addr)
		{
			table = &libtable->table;
			break;
		}
	}

	/* If we don't have a symbol table for this library, then create one */
	if (table == NULL)
	{
		libtable = new LibSymbolTable();
		libtable->table.Initialize();
		libtable->lib_base = dlmap->l_addr;
		libtable->last_pos = 0;
		table = &libtable->table;
		g_SymTables.push_back(libtable);
	}

	/* See if the symbol is already cached in our table */
	symbol_entry = table->FindSymbol(symbol, strlen(symbol));
	if (symbol_entry != NULL)
	{
		return symbol_entry->address;
	}

	/* If symbol isn't in our table, then we have open the actual library */
	dlfile = open(dlmap->l_name, O_RDONLY);
	if (dlfile == -1 || fstat(dlfile, &dlstat) == -1)
	{
		close(dlfile);
		return NULL;
	}

	/* Map library file into memory */
	file_hdr = (Elf32_Ehdr *)mmap(NULL, dlstat.st_size, PROT_READ, MAP_PRIVATE, dlfile, 0);
	map_base = (uintptr_t)file_hdr;
	if (file_hdr == MAP_FAILED)
	{
		close(dlfile);
		return NULL;
	}
	close(dlfile);

	if (file_hdr->e_shoff == 0 || file_hdr->e_shstrndx == SHN_UNDEF)
	{
		munmap(file_hdr, dlstat.st_size);
		return NULL;
	}

	sections = (Elf32_Shdr *)(map_base + file_hdr->e_shoff);
	section_count = file_hdr->e_shnum;
	/* Get ELF section header string table */
	shstrtab_hdr = &sections[file_hdr->e_shstrndx];
	shstrtab = (const char *)(map_base + shstrtab_hdr->sh_offset);

	/* Iterate sections while looking for ELF symbol table and string table */
	for (uint16_t i = 0; i < section_count; i++)
	{
		Elf32_Shdr &hdr = sections[i];
		const char *section_name = shstrtab + hdr.sh_name;

		if (strcmp(section_name, ".symtab") == 0)
		{
			symtab_hdr = &hdr;
		}
		else if (strcmp(section_name, ".strtab") == 0)
		{
			strtab_hdr = &hdr;
		}
	}

	/* Uh oh, we don't have a symbol table or a string table */
	if (symtab_hdr == NULL || strtab_hdr == NULL)
	{
		munmap(file_hdr, dlstat.st_size);
		return NULL;
	}

	symtab = (Elf32_Sym *)(map_base + symtab_hdr->sh_offset);
	strtab = (const char *)(map_base + strtab_hdr->sh_offset);
	symbol_count = symtab_hdr->sh_size / symtab_hdr->sh_entsize;

	/* Iterate symbol table starting from the position we were at last time */
	for (uint32_t i = libtable->last_pos; i < symbol_count; i++)
	{
		Elf32_Sym &sym = symtab[i];
		unsigned char sym_type = ELF32_ST_TYPE(sym.st_info);
		const char *sym_name = strtab + sym.st_name;
		Symbol *cur_sym;

		/* Skip symbols that are undefined or do not refer to functions or objects */
		if (sym.st_shndx == SHN_UNDEF || (sym_type != STT_FUNC && sym_type != STT_OBJECT))
		{
			continue;
		}

		/* Caching symbols as we go along */
		cur_sym = table->InternSymbol(sym_name, strlen(sym_name), (void *)(dlmap->l_addr + sym.st_value));
		if (strcmp(symbol, sym_name) == 0)
		{
			symbol_entry = cur_sym;
			libtable->last_pos = ++i;
			break;
		}
	}

	munmap(file_hdr, dlstat.st_size);
	return symbol_entry ? symbol_entry->address : NULL;
#endif
}
