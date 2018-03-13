#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE  4096

typedef enum {
  kNotInToken,
  kInToken,
} TokenizerState;

typedef enum {
  kUnknown,
  kValue,
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
  "retq",
  NULL
};

const char *op_name[] = {
  "=",
  NULL
};

const char *register_name[] = {
  "eax",
  "rbp",
  "rsp",
  NULL
};

char buf[BUF_SIZE + 16];
char *token_buf[1024];
TokenType token_buf_type[1024];
int token_buf_count;

int find(const char **list, const char *token)
{
  for(int i = 0; list[i]; i++){
    if(strcmp(token, list[i]) == 0) return i;
  }
  return -1;
}

int getTypeOfToken(const char *token){
  int idx;
  if(~(idx = find(mnemonic_name, token))) return kMnemonic;
  if(~(idx = find(op_name, token))) return kOp;
  if(~(idx = find(register_name, token))) return kRegister;
  return kUnknown;
}

void PrintHeader(){
puts("cf fa ed fe 07 00 00 01 03 00 00 00 01 00 00 00");
puts("04 00 00 00 10 01 00 00 00 00 00 00 00 00 00 00");
puts("19 00 00 00 98 00 00 00 00 00 00 00 00 00 00 00");
puts("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00");
puts("08 00 00 00 00 00 00 00 30 01 00 00 00 00 00 00");
puts("08 00 00 00 00 00 00 00 07 00 00 00 07 00 00 00");
puts("01 00 00 00 00 00 00 00 5f 5f 74 65 78 74 00 00");
puts("00 00 00 00 00 00 00 00 5f 5f 54 45 58 54 00 00");
puts("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00");
puts("08 00 00 00 00 00 00 00 30 01 00 00 00 00 00 00");
puts("00 00 00 00 00 00 00 00 00 04 00 80 00 00 00 00");
puts("00 00 00 00 00 00 00 00 24 00 00 00 10 00 00 00");
puts("00 0c 0a 00 00 00 00 00 02 00 00 00 18 00 00 00");
puts("38 01 00 00 01 00 00 00 48 01 00 00 08 00 00 00");
puts("0b 00 00 00 50 00 00 00 00 00 00 00 00 00 00 00");
puts("00 00 00 00 01 00 00 00 01 00 00 00 00 00 00 00");
puts("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00");
puts("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00");
puts("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00");
}

void PrintFooter(){
puts("01 00 00 00 0f 01 00 00");
puts("00 00 00 00 00 00 00 00 00 5f 6d 61 69 6e 00 00");
}

int main(int argc, char *argv[])
{
  if(argc < 2) return 1;
  FILE *fp = fopen(argv[1], "rb");

  if(!fp) return 0;

  int size = fread(buf, 1, BUF_SIZE, fp);
  
  TokenizerState state = kNotInToken;
  char *p;

  PrintHeader();

  int token_buf_count = 0;
  for(int i = 0; i <= size; i++){
    if(i >= size || buf[i] <= ' '){
      buf[i] = 0;
      if(state == kInToken){
        state = kNotInToken;
        token_buf[token_buf_count] = p;
        token_buf_type[token_buf_count] = getTypeOfToken(p);
        token_buf_count++;
      }
      if(i >= size) break;
      continue;
    }
    if(state == kNotInToken){
      state = kInToken;
      p = &buf[i];
    }
    //printf("%02X ", buf[i]);
  }
  for(int i = 0; i < token_buf_count; ){
    //printf("%s %d\n", token_buf[i], token_buf_type[i]);
    if(strcmp(token_buf[i], "push") == 0){
      i++;
      if(strcmp(token_buf[i], "rbp") == 0){
        i++;
        puts("55");
      } else{
        return 1;
      }
    } else if(strcmp(token_buf[i], "mov") == 0){
      i++;
      i++;
      i++;
      puts("48 89 e5");
    } else if(strcmp(token_buf[i], "xor") == 0){
      i++;
      i++;
      i++;
      puts("31 c0");
    } else if(strcmp(token_buf[i], "pop") == 0){
      i++;
      i++;
      puts("5d");
    } else if(strcmp(token_buf[i], "retq") == 0){
      i++;
      i++;
      puts("c3");
    }
  }

  PrintFooter();

  return 0;
}
