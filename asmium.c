#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asmium.h"

#define BUF_SIZE 4096
#define BIN_BUF_SIZE 8192
#define MAX_TOKENS 1024
#define MAX_LABELS 16

const char *mnemonic_name[] = {"push", "pop",     "xor", "mov", "nop",
                               "retq", "syscall", "inc", "cmp", "jne",
                               "jmp",  "hlt",     "int", NULL};

const char *op_name[] = {"=", "^=", NULL};

const char *line_comment_marker[] = {";", NULL};

const char *register_name[] = {
    "al",  "cl",  "dl",  "bl",  "ah",  "ch",  "dh",  "bh",   // 8bits
    "ax",  "cx",  "dx",  "bx",  "sp",  "bp",  "si",  "di",   // 16bits
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",  // 32bits
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",  // 64bits
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",   // 64bits
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",  // 64bits
    "es",  "cs",  "ss",  "ds",  "fs",  "gs",  "",    "",     // seg
    NULL};

#define REG_rAX 0x0
#define REG_rCX 0x1
#define REG_rDX 0x2
#define REG_rBX 0x3
#define REG_rSP 0x4
#define REG_rBP 0x5
#define REG_rSI 0x6
#define REG_rDI 0x7


const char *segment_register_name[] = {"es", "cs", "ss", "ds",
                                       "fs", "gs", NULL};
typedef struct {
  int offset_in_binary;
  const TokenStr *token;
} Label;

char buf[BUF_SIZE + 16];

uint8_t bin_buf[BIN_BUF_SIZE];
size_t bin_buf_size;

uint8_t current_bits = 64;

Label labels[MAX_LABELS];
int labels_count;

FILE *dst_fp = NULL;
int is_hex_mode = 0;

//
// TokenStr
//

TokenStr token_str_list[MAX_TOKENS];
int token_str_list_used = 0;

int IsEqualTokenStr(const TokenStr *ts, const char *s) {
  int s_len = strlen(s);
  if (s_len != ts->len) return 0;
  return strncmp(ts->str, s, s_len) == 0;
}
int IsEqualTokenStrs(const TokenStr *tsa, const TokenStr *tsb) {
  return (tsa->len == tsb->len && strncmp(tsa->str, tsb->str, tsa->len) == 0);
}

void CopyTokenStr(char *dst, const TokenStr *ts, int size) {
  if (!(ts->len < size)) {
    fprintf(stderr, "Not enough dst.\n");
    exit(EXIT_FAILURE);
  }
  strncpy(dst, ts->str, ts->len);
  dst[ts->len] = 0;
}

#define TmpTokenCStr_tmpstrsize (64 + 1)
const char *TmpTokenCStr(const TokenStr *ts) {
  static char tmpstr[TmpTokenCStr_tmpstrsize];
  CopyTokenStr(tmpstr, ts, TmpTokenCStr_tmpstrsize);
  return tmpstr;
}

void ExpectTokenStrType(const TokenStr *ts, TokenStrType type) {
  if (ts->type != type) {
    fprintf(stderr, "line %d: Expected type %d, got %d\n", ts->line, type,
            ts->type);
    exit(EXIT_FAILURE);
  }
}

#define GetIntegerFromTokenStr_tmpstrsize (64 + 1)
int64_t GetIntegerFromTokenStr(const TokenStr *ts) {
  ExpectTokenStrType(ts, kInteger);
  char tmpstr[GetIntegerFromTokenStr_tmpstrsize];
  CopyTokenStr(tmpstr, ts, GetIntegerFromTokenStr_tmpstrsize);
  char *p;
  int64_t value = strtoll(tmpstr, &p, 0);
  if (!(tmpstr[0] != '\0' && *p == '\0')) {
    fprintf(stderr, "line %d: Not valid integer. '%s'\n", ts->line, tmpstr);
    exit(EXIT_FAILURE);
  }
  // entire token is valid
  return value;
}

void Error(const char *s) {
  fputs(s, stderr);
  putchar('\n');
  exit(EXIT_FAILURE);
}

void ErrorWithLine(const TokenStr *ts, const char *fmt, ...) {
  fprintf(stderr, "line %d: ", ts->line);
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  exit(EXIT_FAILURE);
}

void AddLabel(const TokenStr *token, int offset_in_binary) {
  if (labels_count >= MAX_LABELS) {
    Error("Label table exceeded");
  }
  printf("Label[%d] definition %s at ofs +%d\n", labels_count,
         TmpTokenCStr(token), offset_in_binary);
  // TODO: Add label duplication check
  labels[labels_count].offset_in_binary = offset_in_binary;
  labels[labels_count].token = token;
  labels_count++;
}

int FindLabel(const TokenStr *token) {
  printf("Search LabelName %s\n", TmpTokenCStr(token));
  for (int i = 0; i < labels_count; i++) {
    printf("LabelName[%d] = %s\n", i, TmpTokenCStr(labels[i].token));
    if (IsEqualTokenStrs(labels[i].token, token)) return i;
  }
  return -1;
}

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
  if (!is_hex_mode) {
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

void PutByte(uint8_t byte) {
  bin_buf[bin_buf_size++] = byte;
  printf("%02X ", byte);
  if (dst_fp && is_hex_mode) {
    fprintf(dst_fp, "%02X ", byte);
  }
}
void PutEndOfInstr() {
  if (dst_fp && is_hex_mode) {
    fprintf(dst_fp, "\n");
  }
}

#define PREFIX_REX 0x40
#define PREFIX_REX_BITS_W 0x08
#define PREFIX_REX_BITS_R 0x04
#define PREFIX_REX_BITS_X 0x02
#define PREFIX_REX_BITS_B 0x01

#define OP_Immediate_Grp1_Ev_Ib 0x83
#define OP_MOV_Ev_Gv 0x89
#define OP_MOV_Gb_Eb 0x8a
#define OP_MOV_Ev_Iz 0xc7
#define OP_INC_DEC_Grp5 0xff
#define OP_POP_GReg 0x58 /* 0101 1rrr*/

#define OP_Jcc_BASE 0x70
#define COND_Jcc_NE 0x05

int ReadRegisterToken(const TokenStr *tokens, int num_of_tokens, int index,
                      RegisterInfo *reg_info) {
  // retv: Next index or -1 if There is no register name at index.
  if (index >= num_of_tokens) return -1;
  for (int i = 0; register_name[i]; i++) {
    if (IsEqualTokenStr(&tokens[index], register_name[i])) {
      reg_info->number = i & 7;
      reg_info->category = i >> 3;
      return index + 1;
    }
  }
  return -1;
}

Operand *ReadOperand(const TokenStr *tokens, int num_of_tokens, int *index,
                     Operand *ope) {
  if (*index >= num_of_tokens) {
    Error("Trying to read an operand beyond the end of tokens.");
  }
  switch (tokens[*index].type) {
    case kIdentifier:
      ope->token = &tokens[*index];
      if (~ReadRegisterToken(tokens, num_of_tokens, *index, &ope->reg_info)) {
        (*index)++;
        ope->type = kReg;
        return ope;
      }
      break;
    case kInteger:
      ope->token = &tokens[*index];
      ope->imm = GetIntegerFromTokenStr(&tokens[*index]);
      (*index)++;
      ope->type = kImm;
      return ope;
    case kLabel:
      ope->token = &tokens[*index];
      (*index)++;
      ope->type = kLabelName;
      return ope;
    case kMemOfsBegin:
      ope->type = kMem;
      (*index)++;
      if (ReadRegisterToken(tokens, num_of_tokens, *index, &ope->reg_index)) {
        printf("Index Reg found: %s\n", TmpTokenCStr(&tokens[*index]));
        (*index)++;
      }
      if(tokens[*index].type != kMemOfsEnd){
        ErrorWithLine(&tokens[*index], "Expected ] but got %s", TmpTokenCStr(&tokens[*index]));
      }
      (*index)++;
      return ope;
    case kOperator:
      if(IsEqualTokenStr(&tokens[*index], "-")){
         (*index)++;
         if(ReadOperand(tokens, num_of_tokens, index, ope)){
           if(ope->type == kImm){
              ope->imm = -ope->imm;
              return ope;
           }
         }
      }
      break;
    default:
      // do nothing -> Error
      break;
  }
  return NULL;
}

//
// Operator
//

int ParseOpAssign(const Operand *left, const Operand *right) {
  if (left->type == kReg && right->type == kReg) {
    if (left->reg_info.category == kReg64Legacy &&
        right->reg_info.category == kReg64Legacy) {
      // greg64 = greg64
      PutByte(PREFIX_REX | PREFIX_REX_BITS_W);
      PutByte(OP_MOV_Ev_Gv);
      PutByte(ModRM(3, right->reg_info.number, left->reg_info.number));
    } else if (left->reg_info.category == kSegReg &&
               right->reg_info.category == kReg16) {
      // sreg = greg16
      PutByte(0x8e);
      PutByte(ModRM(3, left->reg_info.number, right->reg_info.number));
    }
    return 0;
  } else if (left->type == kReg && right->type == kImm) {
    switch (left->reg_info.category) {
      case kReg64Legacy:
        // case kReg64Low:
        // case kReg64Hi:
        PutByte(PREFIX_REX | PREFIX_REX_BITS_W);
        PutByte(OP_MOV_Ev_Iz);
        PutByte(ModRM(3, 0, left->reg_info.number));
        // imm32
        for (int bi = 0; bi < 4; bi++) {
          PutByte((right->imm >> (8 * bi)) & 0xff);
        }
        return 0;
      case kReg32:
        PutByte(OP_MOV_Ev_Iz);
        PutByte(ModRM(3, 0, left->reg_info.number));
        // imm32
        for (int bi = 0; bi < 4; bi++) {
          PutByte((right->imm >> (8 * bi)) & 0xff);
        }
        return 0;
      case kReg16:
        PutByte(0xb8 | left->reg_info.number);
        // imm16
        for (int bi = 0; bi < 2; bi++) {
          PutByte((right->imm >> (8 * bi)) & 0xff);
        }
        return 0;
      default:
        break;
    }
  } else if (left->type == kReg && right->type == kMem) {
    // reg = [mem location]
    if(left->reg_info.category == kReg8){
      // 2.1.5 Table 2-1. 16-Bit Addressing Forms with the ModR/M Byte
      if(right->reg_index.number == REG_rSI){
        PutByte(OP_MOV_Gb_Eb);
        PutByte(ModRM(0, left->reg_info.number, 4));
        return 0;
      }
    } 
  }
  Error("Not implemented");
  return 1;
}

int ParseOpXor(const Operand *left, const Operand *right) {
  if (left->type == kReg && right->type == kReg) {
    PutByte(0x31);
    PutByte(ModRM(3 /* 0b11 */, right->reg_info.number & 7,
                  left->reg_info.number & 7));
    return 0;
  } else {
    Error("Not implemented");
  }
  return 1;
}

int ParseOpCmp(const Operand *left, const Operand *right) {
  /*
  if (left->type == kReg && right->type == kReg) {
    PutByte(PREFIX_REX | PREFIX_REX_BITS_W);
    PutByte(OP_MOV_Ev_Gv);
    PutByte(ModRM(3, right->reg_info.number, left->reg_info.number));
    return 0;
  } else if (left->type == kSegReg && right->type == kReg) {
    PutByte(0x8e);
    PutByte(ModRM(3, left->reg_info.number, right->reg_info.number));
    return 0;
  } else */
  if (left->type == kImm && right->type == kReg) {
    switch (right->reg_info.category) {
      case kReg32:
        PutByte(OP_Immediate_Grp1_Ev_Ib);
        PutByte(ModRM(3, 7, right->reg_info.number));
        if (left->imm & ~0xff) {
          Error("Not implemented imm larger than 8bits");
        }
        PutByte(left->imm & 0xff);
        return 0;
      default:
        break;
    }
    Error("Not implemented imm");
  } else {
    Error("Not implemented");
  }
  return 1;
}

const OpEntry op_table[] = {
    {"=", ParseOpAssign}, {"^=", ParseOpXor}, {"?", ParseOpCmp}, {NULL, NULL}};

const OpEntry *FindOp(const TokenStr *tokenstr) {
  const OpEntry *op;
  for (op = op_table; op->name; op++) {
    if (IsEqualTokenStr(tokenstr, op->name)) return op;
  }
  return NULL;
}

//
// Mnemonic
//

int ParseMnemonicJMP(const TokenStr *tokens, int num_of_tokens, int index) {
  index++;  // skip mnemonic

  Operand jmp_target;
  if (!ReadOperand(tokens, num_of_tokens, &index, &jmp_target)) {
    ErrorWithLine(&tokens[index], "Expected operand, got %s",
                  TmpTokenCStr(&tokens[index]));
  }
  if(jmp_target.type == kImm){
    int64_t rel_offset = jmp_target.imm;
    if ((rel_offset & ~127) && ~(rel_offset | 127)) {
      Error("Offset out of bound (not impleented yet)");
    }
    PutByte(0xeb);
    PutByte(rel_offset & 0xff);
  } else {
    ErrorWithLine(&tokens[index], "Unexpected type of operand");
  }
  return index;
}

int ParseMnemonicINT(const TokenStr *tokens, int num_of_tokens, int index) {
  index++;  // skip mnemonic
  if (tokens[index].type == kInteger) {
    int64_t int_num = GetIntegerFromTokenStr(&tokens[index]);
    index++;
    if (int_num < 0 || 0xff < int_num) {
      Error("Invalid int number");
    }
    PutByte(0xcd);
    PutByte(int_num & 0xff);
  } else {
    ErrorWithLine(&tokens[index], "Unexpected operand");
  }
  return index;
}

int ParseMnemonicNOP(const TokenStr *tokens, int num_of_tokens, int index) {
  index++;  // skip mnemonic
  PutByte(0x90);
  return index;
}

int ParseMnemonicRETQ(const TokenStr *tokens, int num_of_tokens, int index) {
  index++;  // skip mnemonic
  PutByte(0xc3);
  return index;
}

int ParseMnemonicHLT(const TokenStr *tokens, int num_of_tokens, int index) {
  index++;  // skip mnemonic
  PutByte(0xf4);
  return index;
}

int ParseMnemonicSYSCALL(const TokenStr *tokens, int num_of_tokens, int index) {
  index++;  // skip mnemonic
  PutByte(0x0f);
  PutByte(0x05);
  return index;
}

int ParseMnemonicINC(const TokenStr *tokens, int num_of_tokens, int index) {
  int mn_index = index;
  index++;  // skip mnemonic
  Operand ope;
  if (ReadOperand(tokens, num_of_tokens, &index, &ope)) {
    if (ope.reg_info.category == kReg32) {
      if (current_bits == 64) {
        PutByte(OP_INC_DEC_Grp5);
        PutByte(ModRM(3, 0, ope.reg_info.number));
      } else {
        ErrorWithLine(&tokens[mn_index], "Not implemented in current bits");
      }
    } else {
      ErrorWithLine(&tokens[mn_index], "Not implemented");
    }
  } else {
    ErrorWithLine(&tokens[index], "Unexpected token %s",
                  TmpTokenCStr(&tokens[index]));
  }
  return index;
}
int ParseMnemonicPUSH(const TokenStr *tokens, int num_of_tokens, int index) {
  int mn_index = index;
  index++;  // skip mnemonic
  Operand ope;
  if (ReadOperand(tokens, num_of_tokens, &index, &ope)) {
    if (ope.reg_info.category == kReg64Legacy ||
        ope.reg_info.category == kReg64Low) {
      if (current_bits == 64) {
        PutByte(0x50 + ope.reg_info.number);
      } else {
        ErrorWithLine(&tokens[mn_index], "Not implemented in current bits");
      }
    } else {
      ErrorWithLine(&tokens[mn_index], "Not implemented");
    }
  } else {
    ErrorWithLine(&tokens[index], "Unexpected token %s",
                  TmpTokenCStr(&tokens[index]));
  }
  return index;
}

int ParseMnemonicPOP(const TokenStr *tokens, int num_of_tokens, int index) {
  int mn_index = index;
  index++;  // skip mnemonic
  Operand ope;
  if (ReadOperand(tokens, num_of_tokens, &index, &ope)) {
    if (ope.reg_info.category == kReg64Legacy ||
        ope.reg_info.category == kReg64Low) {
      if (current_bits == 64) {
        PutByte(0x58 + ope.reg_info.number);
      } else {
        ErrorWithLine(&tokens[mn_index], "Not implemented in current bits");
      }
    } else {
      ErrorWithLine(&tokens[mn_index], "Not implemented");
    }
  } else {
    ErrorWithLine(&tokens[index], "Unexpected token %s",
                  TmpTokenCStr(&tokens[index]));
  }
  return index;
}

int ParseMnemonicJNE(const TokenStr *tokens, int num_of_tokens, int index) {
  int mn_index = index;
  index++;  // skip mnemonic
  Operand ope;
  if (ReadOperand(tokens, num_of_tokens, &index, &ope)) {
    if (ope.type == kLabelName) {
      int label_index = FindLabel(ope.token);
      if (label_index == -1) {
        Error("Label not found (not implemented yet)");
      }
      printf("Label[%d] %s found (ofs=%d)\n", label_index,
             TmpTokenCStr(labels[label_index].token),
             labels[label_index].offset_in_binary);
      int32_t rel_offset =
          labels[label_index].offset_in_binary - (bin_buf_size + 2);
      if ((rel_offset & ~127) && ~(rel_offset | 127)) {
        Error("Offset out of bound (not impleented yet)");
      }
      PutByte(OP_Jcc_BASE | COND_Jcc_NE);
      PutByte(rel_offset & 0xff);
    } else {
      ErrorWithLine(&tokens[mn_index], "Not implemented jmp target");
    }
  } else {
    ErrorWithLine(&tokens[index], "Unexpected token %s",
                  TmpTokenCStr(&tokens[index]));
  }
  return index;
}

const MnemonicEntry mnemonic_table[] = {{"jmp", ParseMnemonicJMP},
                                        {"nop", ParseMnemonicNOP},
                                        {"push", ParseMnemonicPUSH},
                                        {"pop", ParseMnemonicPOP},
                                        {"retq", ParseMnemonicRETQ},
                                        {"syscall", ParseMnemonicSYSCALL},
                                        {"++", ParseMnemonicINC},
                                        {"jne", ParseMnemonicJNE},
                                        {"int", ParseMnemonicINT},
                                        {"hlt", ParseMnemonicHLT},
                                        {NULL, NULL}};

const MnemonicEntry *FindMnemonic(const TokenStr *tokenstr) {
  const MnemonicEntry *mne;
  for (mne = mnemonic_table; mne->mnemonic; mne++) {
    if (IsEqualTokenStr(tokenstr, mne->mnemonic)) return mne;
  }
  return NULL;
}

//
// Parser
//

int Parse(const TokenStr *tokens, int num_of_tokens, int index) {
  const MnemonicEntry *mne;
  while (index < num_of_tokens) {
    if (tokens[index].type == kLabel) {
      AddLabel(&tokens[index++], bin_buf_size);
    } else if (IsEqualTokenStr(&tokens[index], ".")) {
      // directive
      index++;
      if (IsEqualTokenStr(&tokens[index], "bits")) {
        index++;
        int64_t bits = GetIntegerFromTokenStr(&tokens[index]);
        if (bits == 64) {
          puts(".bits 64");
          current_bits = 64;
        } else if (bits == 16) {
          puts(".bits 16");
          current_bits = 16;
        } else {
          ErrorWithLine(&tokens[index], "Invalid bits for .bits");
        }
        index++;
      } else if (IsEqualTokenStr(&tokens[index], "asciinz")) {
        index++;
        const TokenStr *string_token = &tokens[index++];
        ExpectTokenStrType(string_token, kString);
        for (int i = 0; i < string_token->len; i++) {
          PutByte(string_token->str[i]);
        }
      } else if (IsEqualTokenStr(&tokens[index], "data32")) {
        index++;
        const TokenStr *int_token;
        for (int_token = &tokens[index];
             int_token && int_token->type == kInteger;
             int_token = &tokens[++index]) {
          int64_t v = GetIntegerFromTokenStr(int_token);
          PutByte(v & 0xff);
          PutByte((v >> 8) & 0xff);
          PutByte((v >> 16) & 0xff);
          PutByte((v >> 24) & 0xff);
        }
      } else if (IsEqualTokenStr(&tokens[index], "data16")) {
        index++;
        const TokenStr *int_token;
        for (int_token = &tokens[index];
             int_token && int_token->type == kInteger;
             int_token = &tokens[++index]) {
          int64_t v = GetIntegerFromTokenStr(int_token);
          PutByte(v & 0xff);
          PutByte((v >> 8) & 0xff);
        }
      } else if (IsEqualTokenStr(&tokens[index], "data8")) {
        index++;
        const TokenStr *int_token;
        for (int_token = &tokens[index];
             int_token && int_token->type == kInteger;
             int_token = &tokens[++index]) {
          int64_t v = GetIntegerFromTokenStr(int_token);
          PutByte(v & 0xff);
        }
      } else if (IsEqualTokenStr(&tokens[index], "offset")) {
        index++;
        const TokenStr *ofs_token = &tokens[index++];
        int64_t ofs = GetIntegerFromTokenStr(ofs_token);
        if (bin_buf_size > ofs) {
          ErrorWithLine(&tokens[index],
                        "Current offset is greater than %s (%d)",
                        TmpTokenCStr(ofs_token), bin_buf_size);
        }
        while (bin_buf_size < ofs) {
          PutByte(0x00);
        }
        printf("@+0x%zX\n", bin_buf_size);
      } else {
        ErrorWithLine(&tokens[index], "No directive named %s found.",
                      TmpTokenCStr(&tokens[index]));
      }
      PutEndOfInstr();
    } else if ((mne = FindMnemonic(&tokens[index]))) {
      printf("MN_EXPR\n");
      index = mne->parse(tokens, num_of_tokens, index);
      PutEndOfInstr();
    } else {
      printf("BIN_EXPR\n");
      // <op_sentence> = <operand> <operator> <operand>
      // <operand> = <register> | <immediate> | <memory_location>
      // <memory_location> = <sib> | <segment_register><sib>
      // <sib> =  [ <scale> * <index> + <base> ] |
      //          [ <index> + <base> ] |
      //          [ <base> ]
      // <scale> = 1 | 2 | 4 | 8
      Operand left_ope;
      if (!ReadOperand(tokens, num_of_tokens, &index, &left_ope)) {
        ErrorWithLine(&tokens[index], "Expected operand, got %s",
                      TmpTokenCStr(&tokens[index]));
      }

      const OpEntry *op;
      if (!(op = FindOp(&tokens[index]))) {
        ErrorWithLine(&tokens[index], "Expected operator, got %s",
                      TmpTokenCStr(&tokens[index]));
      }
      int op_index = index;
      index++;

      Operand right_ope;
      if (!ReadOperand(tokens, num_of_tokens, &index, &right_ope)) {
        ErrorWithLine(&tokens[index], "Expected operand, got %s",
                      TmpTokenCStr(&tokens[index]));
      }
      if (op->parse(&left_ope, &right_ope)) {
        ErrorWithLine(&tokens[op_index], "Failed to parse operator %s",
                      TmpTokenCStr(&tokens[op_index]));
      }
      PutEndOfInstr();
    }
  }
  return 0;
}

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

  // printf("src: %s\ndst: %s\n", argv[src_path_index], argv[dst_path_index]);
  FILE *src_fp = fopen(argv[src_path_index], "rb");
  dst_fp = fopen(argv[dst_path_index], "wb");

  if (!src_fp || !dst_fp) return 0;

  int size = fread(buf, 1, BUF_SIZE, src_fp);
  if (size >= BUF_SIZE) {
    fputs("Too large input. Abort.", stderr);
    exit(EXIT_FAILURE);
  }
  buf[size] = 0;

  Tokenize(token_str_list, MAX_TOKENS, &token_str_list_used, buf);
  DebugPrintTokens(token_str_list, token_str_list_used);

  Parse(token_str_list, token_str_list_used, 0);

  if (!is_hex_mode) {
    PrintHeader(dst_fp, bin_buf_size);
    PrintBody(dst_fp);
    PrintFooter(dst_fp);
  }

  return 0;
  // <label>
  // <operator> (<reg> | <imm> | <label>)* <option>*
  // <reg> <op> (<reg> | <imm>)
  // printf("%d tokens\n", tokens_count);
#if 0
  for (int i = 0; i < token_str_list_used;) {
    TokenType type = token_types[i];
    const char *mn = tokens[i];
    int mn_idx = i++;
    // printf("%d: %s %d\n", i, mn, type);
    if (type == kMnemonic) {
      } else if (strcmp(mn, "mov") == 0) {
        const Token src = GetToken(i++);
        const Token dst = GetToken(i++);
        // printf("mov %s = %s\n", src, dst);

        PutByte(PREFIX_REX | PREFIX_REX_BITS_W);
        PutByte(OP_MOV_Ev_Gv);
        PutByte(ModRM(3 /* 0b11 */, ToRegNum(&src), ToRegNum(&dst)));
      } else if (strcmp(mn, "jmp") == 0) {
        const Token target = GetToken(i++);
        //
        // printf("jne ");
        PrintToken(&target);
        putchar('\n');
        //
      } else if (strcmp(mn, "xor") == 0) {
        const Token src = GetToken(i++);
        const Token dst = GetToken(i++);
        PutByte(0x31);
        PutByte(ModRM(3 /* 0b11 */, ToRegNum(&src), ToRegNum(&dst)));
      } else if (strcmp(mn, "pop") == 0) {
        const Token reg = GetToken(i++);
        PutByte(OP_POP_GReg | ToRegNum(&reg));
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
      } else if (strcmp(mn, "^=") == 0) {
        const Token dst = GetToken(i - 2);
        const Token src = GetToken(i++);
        PutByte(0x31);
        PutByte(ModRM(3 /* 0b11 */, ToRegNum(&src), ToRegNum(&dst)));
      } else {
        Error("Not implemented");
      }
      continue;
    } else if (type == kRegister || type == kSegmentRegister) {
      // kRegister appears only prior to kBinaryOperator
      if (i < tokens_count && token_types[i] == kBinaryOperator) {
        continue;
      }
    } else if (type == kLineDelimiter) {
      putchar('\n');
      if (dst_fp && is_hex_mode) {
        fputc('\n', dst_fp);
      }
      continue;
    } else if (type == kDirective) {
      printf("DIRECTIVE: %s\n", mn);
    } else if (type == kLineCommentMarker) {
      for (;;) {
        const Token bt = GetToken(i++);
        if (bt.type == kLineDelimiter) break;
      }
      continue;
    }
    printf("> %s (type: %d)\n", mn, type);
    Error("Unexpected token");
  }

  return 0;
#endif
}
