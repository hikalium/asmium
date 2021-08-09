#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "asmium.h"

void Put16(uint16_t v, FILE *fp) {
  for (int i = 0; i < 2; i++) {
    fputc((v >> (i * 8)) & 0xff, fp);
  }
}

void Put32(uint32_t v, FILE *fp) {
  for (int i = 0; i < 4; i++) {
    fputc((v >> (i * 8)) & 0xff, fp);
  }
}

void Put64(uint64_t v, FILE *fp) {
  for (int i = 0; i < 8; i++) {
    fputc((v >> (i * 8)) & 0xff, fp);
  }
}

typedef enum {
  kLocalNoType = 0x00,
  kLocalSection = 0x03,
  kGlobalNoType = 0x10,
} SymbolType;

typedef struct {
  uint32_t name_idx; // Ofs in .strtab
  uint16_t type;
  uint16_t index; // Index of Shdr
  uint64_t size;  // ofs?
  uint64_t value; // ofs?
} SymbolTableEntry;

typedef enum {
  kProgBits = 1,
  kSymTable = 2,
  kStrTable = 3,
  kNoBits = 8,
} SectionType;

typedef enum {
  kWritable = 1,
  kAllocated = 2,
  kExecutable = 4,
} SectionFlags;

typedef struct {
  uint32_t name_idx;
  uint32_t type;
  uint64_t flags;
  uint64_t addr;
  uint64_t offset;
  uint64_t size_in_file;
  uint64_t size_in_mem;
  uint64_t align;
  uint64_t entsize;
} SectionHeaderEntry;

void PutSectionHeaderEntry(const SectionHeaderEntry *shdr, FILE *fp) {
  const uint8_t *data = (const uint8_t *)shdr;
  for (int i = 0; i < sizeof(SectionHeaderEntry); i++) {
    fputc(data[i], fp);
  }
}

void PutSymbolTableEntry(const SymbolTableEntry *symbol, FILE *fp) {
  const uint8_t *data = (const uint8_t *)symbol;
  for (int i = 0; i < sizeof(SymbolTableEntry); i++) {
    fputc(data[i], fp);
  }
}

#define STRBUF_SIZE 128
char shstrtab_buf[STRBUF_SIZE];
int shstrtab_buf_used = 0;

SectionHeaderEntry shdr_list[10];
int shdr_list_used = 0;

char strtab_buf[STRBUF_SIZE];
int strtab_buf_used = 0;

SymbolTableEntry symbol_list[10];
int symbol_list_used = 0;

void AddStrToBuf(char *buf, int *buf_used, const char *s) {
  int len = strlen(s);
  if (*buf_used + len + 1 >= STRBUF_SIZE) {
    Error("exceeded STRBUF_SIZE");
  }
  strcpy(&buf[*buf_used], s);
  *buf_used += (len + 1);
}

SectionHeaderEntry *AddSection(const char *name, uint32_t type, uint64_t flags,
                               uint64_t addr, uint64_t size_in_file,
                               uint64_t size_in_mem) {
  int name_idx = shstrtab_buf_used;
  AddStrToBuf(shstrtab_buf, &shstrtab_buf_used, name);

  shdr_list[shdr_list_used].name_idx = name_idx;
  shdr_list[shdr_list_used].type = type;
  shdr_list[shdr_list_used].flags = flags;
  shdr_list[shdr_list_used].addr = addr;
  shdr_list[shdr_list_used].size_in_file = size_in_file;
  shdr_list[shdr_list_used].size_in_mem = size_in_mem;
  shdr_list[shdr_list_used].align = 1;
  shdr_list[shdr_list_used].entsize = 0;
  return &shdr_list[shdr_list_used++];
}

SymbolTableEntry *AddSymbol(const char *name, uint16_t type, uint16_t index,
                            uint64_t value) {
  int name_idx = strtab_buf_used;
  AddStrToBuf(strtab_buf, &strtab_buf_used, name);

  symbol_list[symbol_list_used].name_idx = name_idx;
  symbol_list[symbol_list_used].type = type;
  symbol_list[symbol_list_used].index = index;
  symbol_list[symbol_list_used].value = value;
  return &symbol_list[symbol_list_used++];
}

void WriteObjFileForELF64(FILE *fp, uint8_t *bin_buf, uint32_t bin_size) {
  int i;
  size_t bin_size_aligned = (bin_size + 0xf) & ~0xf;
  printf("bin_size_aligned = 0x%lX\n", bin_size_aligned);

  AddSymbol("", kLocalNoType, 0, 0);
  AddSymbol("", kLocalSection, 1, 0);
  AddSymbol("", kLocalSection, 2, 0);
  AddSymbol("", kLocalSection, 3, 0);
  AddSymbol("main", kGlobalNoType, 1, 0);

  AddSection("", 0, 0, 0, 0, 0);
  AddSection(".text", kProgBits, kAllocated | kExecutable, 0, bin_size_aligned,
             0);
  AddSection(".data", kProgBits, kAllocated | kWritable, 0, 0, 0);
  AddSection(".bss", kNoBits, kAllocated | kWritable, 0, 0, 0);

  SectionHeaderEntry *shstrtab = AddSection(".shstrtab", kStrTable, 0, 0, 0, 0);
  int idx_of_shstrtab = (shstrtab - shdr_list);

  SectionHeaderEntry *strtab = AddSection(".strtab", kStrTable, 0, 0, 0, 0);
  int idx_of_strtab = (strtab - shdr_list);

  SectionHeaderEntry *symtab = AddSection(".symtab", kSymTable, 0, 0, 0, 0);
  symtab->size_in_file = sizeof(SymbolTableEntry) * symbol_list_used;
  symtab->entsize = 24;
  symtab->align = 8;
  symtab->size_in_mem = ((uint64_t)idx_of_shstrtab << 32) | idx_of_strtab;
  printf("symtab entries = %d\n", symbol_list_used);

  size_t strtab_size_aligned = (strtab_buf_used + 0xf) & ~0xf;
  printf("strtab size = 0x%lX\n", strtab_size_aligned);
  strtab->size_in_file = strtab_size_aligned;

  size_t shstrtab_size_aligned = (shstrtab_buf_used + 0xf) & ~0xf;
  printf("shstrtab size = 0x%lX\n", shstrtab_size_aligned);
  shstrtab->size_in_file = shstrtab_size_aligned;

  // recalc ofsets of shdrs
  uint64_t ofs = 0x40;
  for (int i = 0; i < shdr_list_used; i++) {
    shdr_list[i].offset = ofs;
    ofs += shdr_list[i].size_in_file;
  }

  // +0x00: ELF Header(0x40)
  // data of .symtab (sizeof(SymbolTableEntry) * symbol_list_used)
  // data of .text (bin_size_aligned)
  // data of .data ()
  // data of .bss ()
  // data of .shstrtab (shstrtab_size_aligned)
  // data of .strtab (strtab_size_aligned)
  // data of shdr list (sizeof(SectionHeaderEntry) * shdr_list_used)

  // header
  fputc(0x7f, fp);
  fputc('E', fp);
  fputc('L', fp);
  fputc('F', fp);
  fputc(0x02, fp);
  fputc(0x01, fp);
  fputc(0x01, fp);
  fputc(0x00, fp);
  fputc(0x00, fp);
  for (i = 0; i < 7; i++) {
    fputc(0x00, fp);
  }

  Put16(0x0001, fp);
  Put16(0x003e, fp);
  Put32(0x0001, fp);
  Put64(0x0000, fp);

  Put64(0x0000, fp);
  Put64(ofs, fp); // beginning of shdr array (ofs in file)

  Put32(0x0000, fp);
  Put16(0x0040, fp);
  Put16(0x0000, fp);
  Put16(0x0000, fp);
  Put16(0x0040, fp);          // size of shdr entry (fixed)
  Put16(shdr_list_used, fp);  // number of shdrs
  Put16(idx_of_shstrtab, fp); // index of shstrtab in shdr array

  for (i = 0; i < bin_size_aligned; i++) {
    fputc(bin_buf[i], fp);
  }

  for (i = 0; i < shstrtab_size_aligned; i++) {
    fputc(shstrtab_buf[i], fp);
  }

  for (i = 0; i < strtab_size_aligned; i++) {
    fputc(strtab_buf[i], fp);
  }

  for (i = 0; i < symbol_list_used; i++) {
    PutSymbolTableEntry(&symbol_list[i], fp);
  }

  for (i = 0; i < shdr_list_used; i++) {
    PutSectionHeaderEntry(&shdr_list[i], fp);
  }
}
