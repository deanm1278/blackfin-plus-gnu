/*
 * File: elf.c
 *
 * Copyright 2006-2008 Analog Devices Inc.
 * Licensed under the GPL-2, see the file COPYING in this dir
 *
 * Description:
 * Abstract away ELF details here
 *
 * TODO: this only handles 32bit ELFs
 * TODO: this only handles 1 ELF at a time (do_reverse_endian is per-process, not per-elf)
 */

#include "headers.h"
#include "helpers.h"
#include "ldr_elf.h"

#ifndef HAVE_MMAP
#define PROT_READ 0x1
#define MAP_SHARED 0x001
#define MAP_FAILED ((void *) -1)
static void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
	void *ret = xmalloc(length);

	assert(start == 0);
	assert(prot == PROT_READ);

	if (read_retry(fd, ret, length) != length) {
		free(ret);
		ret = MAP_FAILED;
	}

	return ret;
}
static int munmap(void *start, size_t length)
{
	free(start);
	return 0;
}
#endif

char do_reverse_endian;

/* valid elf buffer check */
#define IS_ELF_BUFFER(buff) \
	(buff[EI_MAG0] == ELFMAG0 && \
	 buff[EI_MAG1] == ELFMAG1 && \
	 buff[EI_MAG2] == ELFMAG2 && \
	 buff[EI_MAG3] == ELFMAG3)
/* for now, only handle 32bit little endian as it
 * simplifies the input code greatly */
#define DO_WE_LIKE_ELF(buff) \
	((buff[EI_CLASS] == ELFCLASS32 /*|| buff[EI_CLASS] == ELFCLASS64*/) && \
	 (buff[EI_DATA] == ELFDATA2LSB /*|| buff[EI_DATA] == ELFDATA2MSB*/) && \
	 (buff[EI_VERSION] == EV_CURRENT))

elfobj *elf_open(const char *filename)
{
	elfobj *elf;
	struct stat st;

	elf = xmalloc(sizeof(*elf));

	elf->fd = open(filename, O_RDONLY | O_BINARY);
	if (elf->fd < 0)
		goto err_free;

	if (fstat(elf->fd, &st) == -1 || st.st_size < EI_NIDENT)
		goto err_close;
	elf->len = st.st_size;

	elf->data = mmap(0, elf->len, PROT_READ, MAP_SHARED, elf->fd, 0);
	if (elf->data == MAP_FAILED)
		goto err_close;
	elf->data_end = elf->data + elf->len;

	if (!IS_ELF_BUFFER(elf->data) || !DO_WE_LIKE_ELF(elf->data))
		goto err_munmap;

	elf->filename = filename;
	elf->elf_class = elf->data[EI_CLASS];
	do_reverse_endian = (ELF_DATA != elf->data[EI_DATA]);
	elf->ehdr = (void*)elf->data;

	/* lookup the program and section header tables */
	{
		Elf32_Ehdr *ehdr = EHDR32(elf->ehdr);

		if (EGET(ehdr->e_machine) != EM_BLACKFIN)
			goto err_munmap;

		if (EGET(ehdr->e_phnum) <= 0)
			elf->phdr = NULL;
		else
			elf->phdr = elf->data + EGET(ehdr->e_phoff);
		if (EGET(ehdr->e_shnum) <= 0)
			elf->shdr = NULL;
		else
			elf->shdr = elf->data + EGET(ehdr->e_shoff);
	}

	return elf;

 err_munmap:
	munmap(elf->data, elf->len);
 err_close:
	close(elf->fd);
 err_free:
	free(elf);
	return NULL;
}

void elf_close(elfobj *elf)
{
	munmap(elf->data, elf->len);
	close(elf->fd);
	free(elf);
	return;
}

void *elf_lookup_section(const elfobj *elf, const char *name)
{
	unsigned int i;
	char *shdr_name;

	if (elf->shdr == NULL)
		return NULL;

	{
	Elf32_Ehdr *ehdr = EHDR32(elf->ehdr);
	Elf32_Shdr *shdr = SHDR32(elf->shdr);
	Elf32_Shdr *strtbl;
	Elf32_Off offset;
	uint16_t shstrndx = EGET(ehdr->e_shstrndx);
	uint16_t shnum = EGET(ehdr->e_shnum);

	if (shstrndx >= shnum)
		return NULL;

	strtbl = &(shdr[shstrndx]);
	for (i = 0; i < shnum; ++i) {
		offset = EGET(strtbl->sh_offset) + EGET(shdr[i].sh_name);
		shdr_name = (char*)(elf->data + offset);
		if (!strcmp(shdr_name, name))
			return &(shdr[i]);
	}
	}

	return NULL;
}

void *elf_lookup_symbol(const elfobj *elf, const char *found_sym)
{
	unsigned long i;
	void *symtab_void, *strtab_void;

	symtab_void = elf_lookup_section(elf, ".symtab");
	strtab_void = elf_lookup_section(elf, ".strtab");
	if (!symtab_void || !strtab_void)
		return NULL;

	{
	Elf32_Shdr *shdr = SHDR32(elf->shdr);
	Elf32_Shdr *symtab = SHDR32(symtab_void);
	Elf32_Shdr *strtab = SHDR32(strtab_void);
	Elf32_Sym *sym = SYM32(elf->data + EGET(symtab->sh_offset));
	unsigned long cnt = EGET(symtab->sh_entsize);
	char *symname;
	if (cnt)
		cnt = EGET(symtab->sh_size) / cnt;
	for (i = 0; i < cnt; ++i) {
		symname = (char *)(elf->data + EGET(strtab->sh_offset) + EGET(sym->st_name));
		if (!strcmp(symname, found_sym)) {
			shdr = shdr + EGET(sym->st_shndx);
			return elf->data + EGET(shdr->sh_offset) + (EGET(sym->st_value) - EGET(shdr->sh_addr));
		}
		++sym;
	}
	}

	return NULL;
}
