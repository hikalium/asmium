#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 4096
#define BIN_BUF_SIZE 4096
#define MAX_TOKENS 1024
#define MAX_LABELS 16

typedef enum {
  kNotInToken,
  kInToken,
} TokenizerState;

typedef enum {
  kUnknown,
  kImmediate,
  kMnemonic,
  kBinaryOperator,
  kLabel,
  kRegister,
  kLineDelimiter,
} TokenType;

const char *mnemonic_name[] = {"push",    "pop", "xor", "mov", "nop", "retq",
                               "syscall", "inc", "cmp", "jne", NULL};

const char *op_name[] = {"=", NULL};

const char *register_name[] = {"eax", "ecx", "edx", "ebx", "esp", "ebp",
                               "esi", "edi", "rax", "rcx", "rdx", "rbx",
                               "rsp", "rbp", "rsi", "rdi", NULL};

typedef struct {
  int offset;
  const char *str;
  TokenType type;
  uint64_t value;
} Token;

typedef struct {
  int offset_in_binary;
  const char *name;
} Label;

char buf[BUF_SIZE + 16];
char *tokens[MAX_TOKENS];
TokenType token_types[MAX_TOKENS];
uint64_t token_values[MAX_TOKENS];
int tokens_count;

uint8_t bin_buf[BIN_BUF_SIZE];
size_t bin_buf_size;

Label labels[MAX_LABELS];
int labels_count;

FILE* dst_fp = NULL;
int is_hex_mode = 0;

void Error(const char *s) {
  fputs(s, stderr);
  putchar('\n');
  exit(EXIT_FAILURE);
}

void AddLabel(const char *name, int offset_in_binary) {
  printf("Label definition %s at ofs +%d\n", name, offset_in_binary);
  if (labels_count >= MAX_LABELS) {
    Error("Label table exceeded");
  }
  // TODO: Add label duplication check
  labels[labels_count].offset_in_binary = offset_in_binary;
  labels[labels_count].name = name;
  labels_count++;
}

int FindLabel(const char *name) {
  for (int i = 0; i < labels_count; i++) {
    if (strcmp(labels[i].name, name) == 0) return i;
  }
  return -1;
}

Token GetToken(int index) {
  printf("%d\n", index);
  printf("%d\n", tokens_count);
  if (index >= tokens_count) {
    Error("Trying to get token out of range");
  }
  Token token = {index, tokens[index], token_types[index], token_values[index]};
  return token;
}

void PrintToken(const Token *token) {
  printf("%s (%x.%llx) ", token->str, token->type, token->value);
}

int find(const char **list, const char *token) {
  for (int i = 0; list[i]; i++) {
    if (strcmp(token, list[i]) == 0) return i;
  }
  return -1;
}

int getTypeOfToken(const char *token, uint64_t *token_value) {
  int idx;
  TokenType type = kUnknown;
  if (~(idx = find(mnemonic_name, token)))
    type = kMnemonic;
  else if (~(idx = find(op_name, token)))
    type = kBinaryOperator;
  else if (~(idx = find(register_name, token)))
    type = kRegister;
  else {
    // check if it is immediate value
    char *p;
    uint64_t value = strtoq(token, &p, 0);
    if (*token != '\0' && *p == '\0') {
      // entire token is valid
      if (token_value) {
        *token_value = value;
      }
      return kImmediate;
    }
    // check if it is label
    int len = strlen(token);
    if (len > 0 && token[len - 1] == ':') {
      // label
      return kLabel;
    }
    return kUnknown;
  }
  if (token_value) {
    *token_value = idx;
  }
  return type;
}

uint8_t mach_o_header[0x130] = {
    0xcf, 0xfa, 0xed, 0xfe, 0x07, 0x00, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00,
    0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x30, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x5f, 0x5f, 0x74, 0x65, 0x78, 0x74, 0x00, 0x00,  // "__text"
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x5f, 0x54, 0x45,
    0x58, 0x54, 0x00, 0x00,  // "__TEXT"
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x30, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
    // +0xd0
    0x38, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x48, 0x01, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

uint8_t mach_o_footer[0x18] = {
    // labe infos?
    // [uint32_t label_name_offset in label names]
    // [uint32_t label_type?]
    // [uint32_t offset_in_binary?]
    // [uint32_t ???(zero filled)]
    //
    // label_type:
    //    0e 01 00 00 : local label?
    //    0f 01 00 00 : global label?
    0x01, 0x00, 0x00, 0x00, 0x0f, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    // label names. each label names should be null terminated.
    // 0x00, ["label name", ...] [padding to align 4-bytes]
    0x00, 0x5f, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00,  // "_main"
};

void PrintHeader(FILE *fp, uint32_t binsize) {
  *((uint32_t *)&mach_o_header[0x40]) = binsize;
  *((uint32_t *)&mach_o_header[0x50]) = binsize;
  *((uint32_t *)&mach_o_header[0x90]) = binsize;

  uint32_t binsize_4b_aligned = (binsize + 0x03) & ~0x03;
  *((uint32_t *)&mach_o_header[0xd0]) = binsize_4b_aligned + 0x130;
  *((uint32_t *)&mach_o_header[0xd8]) = binsize_4b_aligned + 0x140;
  for (int i = 0; i < 0x130; i++) {
    fputc(mach_o_header[i], fp);
  }
}

void PrintBody(FILE *fp) {
  int i;
  for (i = 0; i < bin_buf_size; i++) {
    fputc(bin_buf[i], fp);
  }
  if(!is_hex_mode){
    // 4-byte align
    for (; i & 0x3; i++) {
      fputc(0x00, fp);
    }
  }
}

void PrintFooter(FILE *fp) {
  for (int i = 0; i < 0x18; i++) {
    fputc(mach_o_footer[i], fp);
  }
}

uint8_t ModRM(uint8_t mod, uint8_t r, uint8_t r_m) {
  return (mod << 6) | (r << 3) | r_m;
}

void PutByte(uint8_t byte)
{
  bin_buf[bin_buf_size++] = byte;
  printf("%02X ", byte);
  if(dst_fp && is_hex_mode){
    fprintf(dst_fp, "%02X ", byte);
  }
}

#define PREFIX_REX 0x40
#define PREFIX_REX_BITS_W 0x08
#define PREFIX_REX_BITS_R 0x04
#define PREFIX_REX_BITS_X 0x02
#define PREFIX_REX_BITS_B 0x01

#define OP_Immediate_Grp1_Ev_Ib 0x83
#define OP_MOV_Ev_Gv 0x89
#define OP_MOV_Ev_Iz 0xc7
#define OP_INC_DEC_Grp5 0xff

#define OP_Jcc_BASE 0x70
#define COND_Jcc_NE 0x05

#define REG_rAX 0x0
#define REG_rCX 0x1
#define REG_rDX 0x2
#define REG_rBX 0x3
#define REG_rSP 0x4
#define REG_rBP 0x5
#define REG_rSI 0x6
#define REG_rDI 0x7


int main(int argc, char *argv[]) {
  int src_path_index = 0;
  int dst_path_index = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-o") == 0) {
      i++;
      if (i < argc) {
        dst_path_index = i;
      }
      continue;
    } else if (strcmp(argv[i], "--hex") == 0) {
      is_hex_mode = 1;
    }
    src_path_index = i;
  }
  if (!dst_path_index || !src_path_index) {
    puts("asmium: Human readable assembler");
    printf("Usage: %s -o <dst> <src>\n", argv[0]);
    return 1;
  }

  //printf("src: %s\ndst: %s\n", argv[src_path_index], argv[dst_path_index]);
  FILE *src_fp = fopen(argv[src_path_index], "rb");
  dst_fp = fopen(argv[dst_path_index], "wb");

  if (!src_fp || !dst_fp) return 0;

  int size = fread(buf, 1, BUF_SIZE, src_fp);

  TokenizerState state = kNotInToken;
  char *p;

  tokens_count = 0;
  for (int i = 0; i <= size; i++) {
    if (i >= size || buf[i] <= ' ') {
      char old_char = buf[i];
      buf[i] = 0;
      if (state == kInToken) {
        state = kNotInToken;
        tokens[tokens_count] = p;
        token_types[tokens_count] =
            getTypeOfToken(p, &token_values[tokens_count]);
        tokens_count++;
      }
      if(old_char == '\n'){
        tokens[tokens_count] = NULL;
        token_types[tokens_count] = kLineDelimiter;
        tokens_count++;
      }
      if (i >= size) break;
      continue;
    }
    if (state == kNotInToken) {
      state = kInToken;
      p = &buf[i];
    }
  }
  // <label>
  // <operator> (<reg> | <imm> | <label>)* <option>*
  // <reg> <op> (<reg> | <imm>)
  //printf("%d tokens\n", tokens_count);
  for (int i = 0; i < tokens_count;) {
    TokenType type = token_types[i];
    const char *mn = tokens[i];
    int mn_idx = i++;
    //printf("%d: %s %d\n", i, mn, type);
    if (type == kMnemonic) {
      if (strcmp(mn, "push") == 0) {
        const char *reg = tokens[i++];
        if (strcmp(reg, "rbp") == 0) {
          PutByte(0x55);
        } else {
          Error("Not implemented");
        }
      } else if (strcmp(mn, "mov") == 0) {
        const char *src = tokens[i++];
        const char *dst = tokens[i++];
        //printf("mov %s = %s\n", src, dst);

        PutByte(PREFIX_REX | PREFIX_REX_BITS_W);
        PutByte(OP_MOV_Ev_Gv);
        PutByte(ModRM(0b11, REG_rSP, REG_rBP));
      } else if (strcmp(mn, "inc") == 0) {
        const char *reg = tokens[i];
        uint64_t reg_num = token_values[i];
        i++;
        //printf("inc %s\n", reg);

        // PutByte(PREFIX_REX | PREFIX_REX_BITS_W);
        PutByte(OP_INC_DEC_Grp5);
        PutByte(ModRM(0b11, 0b000, reg_num & 7));
      } else if (strcmp(mn, "cmp") == 0) {
        const Token left = GetToken(i++);
        const Token right = GetToken(i++);
        //
        //printf("cmp ");
        PrintToken(&left);
        PrintToken(&right);
        putchar('\n');
        //
        if (left.type == kImmediate && right.type == kRegister) {
          // PutByte(PREFIX_REX | PREFIX_REX_BITS_W);
          PutByte(OP_Immediate_Grp1_Ev_Ib);
          PutByte(ModRM(0b11, 0b111, right.value & 7));
          if (left.value & ~0xff) {
            Error("Not implemented imm larger than 8bits");
          }
          PutByte(left.value & 0xff);
        } else {
          Error("Not implemented");
        }
      } else if (strcmp(mn, "jne") == 0) {
        const Token target = GetToken(i++);
        //
        //printf("jne ");
        PrintToken(&target);
        putchar('\n');
        //
        if (target.type == kLabel) {
          int label_index = FindLabel(target.str);
          if (label_index == -1) {
            Error("Label not found (not implemented yet)");
          }
          int32_t rel_offset =
              labels[label_index].offset_in_binary - (bin_buf_size + 2);
          if ((rel_offset & ~127) && ~(rel_offset | 127)) {
            Error("Offset out of bound (not impleented yet)");
          }
          PutByte(OP_Jcc_BASE | COND_Jcc_NE);
          PutByte(rel_offset & 0xff);
        } else {
          Error("Not implemented");
        }
      } else if (strcmp(mn, "xor") == 0) {
        const char *src = tokens[i++];
        const char *dst = tokens[i++];
        PutByte(0x31);
        PutByte(0xc0);
      } else if (strcmp(mn, "pop") == 0) {
        const char *reg = tokens[i++];
        PutByte(0x5d);
      } else if (strcmp(mn, "retq") == 0) {
        PutByte(0xc3);
      } else if (strcmp(mn, "nop") == 0) {
        PutByte(0x90);
      } else if (strcmp(mn, "syscall") == 0) {
        PutByte(0x0f);
        PutByte(0x05);
      } else {
        printf("mn: %s\n", mn);
        Error("Not implemented");
      }
      continue;
    } else if (type == kBinaryOperator) {
      if (strcmp(mn, "=") == 0) {
        const char *src = tokens[mn_idx + 1];
        TokenType src_type = token_types[mn_idx + 1];
        uint64_t src_value = token_values[mn_idx + 1];
        const char *dst = tokens[mn_idx - 1];
        TokenType dst_type = token_types[mn_idx - 1];
        uint64_t dst_value = token_values[mn_idx - 1];
        printf("%s(%x.%llx) = %s(%x.%llx)\n", dst, dst_type, dst_value, src,
               src_type, src_value);
        if (dst_type == kRegister && src_type == kRegister) {
          PutByte(PREFIX_REX | PREFIX_REX_BITS_W);
          PutByte(OP_MOV_Ev_Gv);
          PutByte(ModRM(0b11, src_value & 7, dst_value & 7));
        } else if (dst_type == kRegister && src_type == kImmediate) {
          PutByte(PREFIX_REX | PREFIX_REX_BITS_W);
          PutByte(OP_MOV_Ev_Iz);
          PutByte(ModRM(0b11, 0b000, dst_value & 7));
          for (int bi = 0; bi < 4; bi++) {
            PutByte((src_value >> (8 * bi)) & 0xff);
          }
        } else {
          Error("Not implemented");
        }
        i++;
      } else {
        Error("Not implemented");
      }
      continue;
    } else if (type == kLabel) {
      AddLabel(mn, bin_buf_size);
      continue;
    } else if (type == kRegister) {
      // kRegister appears only prior to kBinaryOperator
      if (i < tokens_count && token_types[i] == kBinaryOperator) {
        continue;
      }
    } else if(type == kLineDelimiter) {
      putchar('\n');
      if(dst_fp && is_hex_mode){
        fputc('\n', dst_fp);
      }
      continue;
    }
    printf("> %s (type: %d)\n", mn, type);
    Error("Unexpected token");
  }

  if(!is_hex_mode){
    PrintHeader(dst_fp, bin_buf_size);
    PrintBody(dst_fp);
    PrintFooter(dst_fp);
  }

  return 0;
}
