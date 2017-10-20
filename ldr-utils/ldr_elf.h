/*
 * File: elf.h
 *
 * Copyright 2006-2007 Analog Devices Inc.
 * Licensed under the GPL-2, see the file COPYING in this dir
 *
 * Description:
 * Defines for internal ELF handling routines
 */

#ifndef __LDR_ELF_H__
#define __LDR_ELF_H__

#include "headers.h"

#ifndef EM_BLACKFIN
# define EM_BLACKFIN 106
#endif

/* only support 1 ELF at a time for now ... */
extern char do_reverse_endian;

typedef struct {
	void *ehdr;
	void *phdr;
	void *shdr;
	char *data, *data_end;
	char elf_class;
	off_t len;
	int fd;
	const char *filename;
} elfobj;

#define EGET(X) \
	(__extension__ ({ \
		uint32_t __res; \
		if (!do_reverse_endian) {    __res = (X); \
		} else if (sizeof(X) == 1) { __res = (X); \
		} else if (sizeof(X) == 2) { __res = bswap_16((X)); \
		} else if (sizeof(X) == 4) { __res = bswap_32((X)); \
		} else if (sizeof(X) == 8) { printf("64bit types not supported\n"); exit(1); \
		} else { printf("Unknown type size %i\n", (int)sizeof(X)); exit(1); } \
		__res; \
	}))

#define EHDR32(ptr) ((Elf32_Ehdr *)(ptr))
#define EHDR64(ptr) ((Elf64_Ehdr *)(ptr))
#define PHDR32(ptr) ((Elf32_Phdr *)(ptr))
#define PHDR64(ptr) ((Elf64_Phdr *)(ptr))
#define SHDR32(ptr) ((Elf32_Shdr *)(ptr))
#define SHDR64(ptr) ((Elf64_Shdr *)(ptr))
#define RELA32(ptr) ((Elf32_Rela *)(ptr))
#define RELA64(ptr) ((Elf64_Rela *)(ptr))
#define REL32(ptr) ((Elf32_Rel *)(ptr))
#define REL64(ptr) ((Elf64_Rel *)(ptr))
#define DYN32(ptr) ((Elf32_Dyn *)(ptr))
#define DYN64(ptr) ((Elf64_Dyn *)(ptr))
#define SYM32(ptr) ((Elf32_Sym *)(ptr))
#define SYM64(ptr) ((Elf64_Sym *)(ptr))

extern elfobj *elf_open(const char *filename);
extern void elf_close(elfobj *elf);
extern void *elf_lookup_section(const elfobj *elf, const char *name);
extern void *elf_lookup_symbol(const elfobj *elf, const char *found_sym);

#endif
