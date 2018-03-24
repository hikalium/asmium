#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define BUF_SIZE        4096
#define BIN_BUF_SIZE    4096
#define MAX_TOKENS      1024

typedef enum {
  kNotInToken,
  kInToken,
} TokenizerState;

typedef enum {
  kUnknown,
  kImmediate,
  kMnemonic,
  kOp,
  kLabel,
  kRegister,
} TokenType;

const char *mnemonic_name[] = {
  "push",
  "pop",
  "xor",
  "mov",
  "nop",
  "retq",
  "syscall",
  NULL
};

const char *op_name[] = {
  "=",
  NULL
};

const char *register_name[] = {
  "eax",
  "ecx",
  "edx",
  "ebx",
  "esp",
  "ebp",
  "esi",
  "edi",
  "rax",
  "rcx",
  "rdx",
  "rbx",
  "rsp",
  "rbp",
  "rsi",
  "rdi",
  NULL
};

char buf[BUF_SIZE + 16];
char *tokens[MAX_TOKENS];
TokenType token_types[MAX_TOKENS];
uint64_t token_values[MAX_TOKENS];
int tokens_count;

uint8_t bin_buf[BIN_BUF_SIZE];
size_t bin_buf_size;

typedef struct {
  uint8_t op1, op2;
  uint8_t arg1, arg2;
} Instruction;

int find(const char **list, const char *token)
{
  for(int i = 0; list[i]; i++){
    if(strcmp(token, list[i]) == 0) return i;
  }
  return -1;
}

int getTypeOfToken(const char *token, uint64_t *token_value){
  int idx;
  TokenType type = kUnknown;
  if(~(idx = find(mnemonic_name, token))) type = kMnemonic;
  else if(~(idx = find(op_name, token))) type = kOp;
  else if(~(idx = find(register_name, token))) type = kRegister;
  else {
    char *p;
    uint64_t value = strtoq(token, &p, 0);
    if(*token != '\0' && *p == '\0'){
      // entire token is valid
      if(token_value){
        *token_value = value;
      }
      return kImmediate;
    }
    return kUnknown;
  }
  if(token_value){
    *token_value = idx;
  }
  return type;
}

uint8_t mach_o_header[0x130] = {
0xcf, 0xfa, 0xed, 0xfe, 0x07, 0x00, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
0x04, 0x00, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x19, 0x00, 0x00, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x5f, 0x74, 0x65, 0x78, 0x74, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x5f, 0x54, 0x45, 0x58, 0x54, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
0x00, 0x0c, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
0x38, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x48, 0x01, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
0x0b, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

uint8_t mach_o_footer[0x18] = {
0x01,0x00,0x00,0x00,0x0f,0x01,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x5f,0x6d,0x61,0x69,0x6e,0x00,0x00,
};

void PrintHeader(FILE* fp, uint32_t binsize){
  *((uint32_t *)&mach_o_header[0x40]) = binsize;
  *((uint32_t *)&mach_o_header[0x50]) = binsize;
  *((uint32_t *)&mach_o_header[0x90]) = binsize;

  uint32_t binsize_4b_aligned = (binsize + 0x03) & ~0x03;
  *((uint32_t *)&mach_o_header[0xd0]) = binsize_4b_aligned + 0x130;
  *((uint32_t *)&mach_o_header[0xd8]) = binsize_4b_aligned + 0x140;
  for(int i = 0; i < 0x130; i++){
    fputc(mach_o_header[i], fp);
  }
}

void PrintBody(FILE* fp){
  int i;
  for(i = 0; i < bin_buf_size; i++){
    fputc(bin_buf[i], fp);
  }
  // 4-byte align
  for(; i & 0x3; i++){
    fputc(0x00, fp);
  }
}

void PrintFooter(FILE* fp){
  for(int i = 0; i < 0x18; i++){
    fputc(mach_o_footer[i], fp);
  }
}

void Error(const char *s){
  fputs(s, stderr);
  putchar('\n');
  exit(EXIT_FAILURE);
}

uint8_t ModRM(uint8_t mod, uint8_t r, uint8_t r_m){
  return (mod << 6) | (r << 3) | r_m;
}
#define PREFIX_REX          0x40
#define PREFIX_REX_BITS_W   0x08
#define PREFIX_REX_BITS_R   0x04
#define PREFIX_REX_BITS_X   0x02
#define PREFIX_REX_BITS_B   0x01

#define OP_MOV_Ev_Gv        0x89
#define OP_MOV_Ev_Iz        0xc7

#define REG_eAX 0x0
#define REG_eCX 0x1
#define REG_eDX 0x2
#define REG_eBX 0x3
#define REG_eSP 0x4
#define REG_eBP 0x5
#define REG_eSI 0x6
#define REG_eDI 0x7

int main(int argc, char *argv[])
{
  int src_path_index = 0;
  int dst_path_index = 0;
  for(int i = 1; i < argc; i++){
    if(strcmp(argv[i], "-o") == 0){
      i++;
      if(i < argc){
        dst_path_index = i;
      }
      continue;
    }
    src_path_index = i;
  }
  if(!dst_path_index || !src_path_index){
    puts("asmium: Human readable assembler");
    printf("Usage: %s -o <dst> <src>\n", argv[0]);
    return 1;
  }

  printf("src: %s\ndst: %s\n", argv[src_path_index], argv[dst_path_index]);
  FILE *src_fp = fopen(argv[src_path_index], "rb");
  FILE *dst_fp = fopen(argv[dst_path_index], "wb");

  if(!src_fp || !dst_fp) return 0;

  int size = fread(buf, 1, BUF_SIZE, src_fp);
  
  TokenizerState state = kNotInToken;
  char *p;


  int tokens_count = 0;
  for(int i = 0; i <= size; i++){
    if(i >= size || buf[i] <= ' '){
      buf[i] = 0;
      if(state == kInToken){
        state = kNotInToken;
        tokens[tokens_count] = p;
        token_types[tokens_count] = getTypeOfToken(p, &token_values[tokens_count]);
        tokens_count++;
      }
      if(i >= size) break;
      continue;
    }
    if(state == kNotInToken){
      state = kInToken;
      p = &buf[i];
    }
  }
  for(int i = 0; i < tokens_count; ){
    TokenType type = token_types[i];
    const char *mn = tokens[i];
    int mn_idx = i++;
    //printf("%s %d\n", mn, type);
    if(type == kMnemonic){
      if(strcmp(mn, "push") == 0){
        const char *reg = tokens[i++];
        if(strcmp(reg, "rbp") == 0){
          bin_buf[bin_buf_size++] = 0x55;
        } else{
          Error("Not implemented");
        }
        puts("push");
      } else if(strcmp(mn, "mov") == 0){
        const char *src = tokens[i++];
        const char *dst = tokens[i++];
        printf("mov %s = %s\n", src, dst);
        
        bin_buf[bin_buf_size++] = PREFIX_REX | PREFIX_REX_BITS_W;
        bin_buf[bin_buf_size++] = OP_MOV_Ev_Gv;
        bin_buf[bin_buf_size++] = ModRM(0b11, REG_eSP, REG_eBP);
      } else if(strcmp(mn, "xor") == 0){
        puts("xor");
        const char *src = tokens[i++];
        const char *dst = tokens[i++];
        bin_buf[bin_buf_size++] = 0x31;
        bin_buf[bin_buf_size++] = 0xc0;
      } else if(strcmp(mn, "pop") == 0){
        puts("pop");
        const char *reg = tokens[i++];
        bin_buf[bin_buf_size++] = 0x5d;
      } else if(strcmp(mn, "retq") == 0){
        puts("retq");
        bin_buf[bin_buf_size++] = 0xc3;
      } else if(strcmp(mn, "nop") == 0){
        puts("nop");
        bin_buf[bin_buf_size++] = 0x90;
      } else if(strcmp(mn, "syscall") == 0){
        puts("syscall");
        bin_buf[bin_buf_size++] = 0x0f;
        bin_buf[bin_buf_size++] = 0x05;
      } else{
        printf("mn: %s\n", mn);
        Error("Not implemented");
      }
    } else if(type == kOp){
      if(strcmp(mn, "=") == 0){
        const char *src = tokens[mn_idx + 1];
        TokenType src_type = token_types[mn_idx + 1];
        uint64_t src_value = token_values[mn_idx + 1];
        const char *dst = tokens[mn_idx - 1];
        TokenType dst_type = token_types[mn_idx - 1];
        uint64_t dst_value = token_values[mn_idx - 1];
        printf("%s(%x.%llx) = %s(%x.%llx)\n",
              dst, dst_type, dst_value, src, src_type, src_value);
        if(dst_type == kRegister && src_type == kRegister){
          bin_buf[bin_buf_size++] = PREFIX_REX | PREFIX_REX_BITS_W;
          bin_buf[bin_buf_size++] = OP_MOV_Ev_Gv;
          bin_buf[bin_buf_size++] = ModRM(0b11, src_value & 7, dst_value & 7);
        } else if(dst_type == kRegister && src_type == kImmediate){
          bin_buf[bin_buf_size++] = PREFIX_REX | PREFIX_REX_BITS_W;
          bin_buf[bin_buf_size++] = OP_MOV_Ev_Iz;
          bin_buf[bin_buf_size++] = ModRM(0b11, 0b000, dst_value & 7);
          for(int bi = 0; bi < 4; bi++){
            bin_buf[bin_buf_size++] = (src_value >> (8 * bi)) & 0xff;
          }
        } else{
          Error("Not implemented");
        }
      } else{
        Error("Not implemented");
      }
    }
  }

  PrintHeader(dst_fp, bin_buf_size);
  PrintBody(dst_fp);
  PrintFooter(dst_fp);
  puts("done");

  return 0;
}
