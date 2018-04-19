#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "asmium.h"

void PrintTokenStr(const TokenStr *ts){
  write(STDOUT_FILENO, ts->str, ts->len);
}
void DebugPrintTokenStr(const TokenStr *ts){
  //printf("%s: ", TokenStrTypeStr[ts->type]);
  if(ts->type == kWord || ts->type == kInteger || ts->type == kSymbol){
    putchar('[');
    fflush(stdout);
    write(STDOUT_FILENO, ts->str, ts->len);
    putchar(']');
  } else if(ts->type == kNewLine){
    putchar('\n');
  } else if(ts->type == kString){
    putchar('"');
    fflush(stdout);
    write(STDOUT_FILENO, ts->str, ts->len);
    putchar('"');
  } else{
    putchar('<');
    printf("%02X", ts->str[0]);
    putchar('>');
  }
}

void DebugPrintTokens(const TokenStr *toke_str_list, int used){
  int line = 0;
  for(int i = 0; i < used; i++){
    if(line != toke_str_list[i].line){
      line = toke_str_list[i].line;
      printf("%d\t", line);
    }
    DebugPrintTokenStr(&toke_str_list[i]);
  }
}

#define IS_ALPHABET(c)  (('A' <= c && c <='Z') || ('a' <= c && c <='z'))
#define IS_DIGIT(c)  (('0' <= c && c <='9'))
#define IS_BINDIGIT(c)  (('0' <= c && c <='1'))
#define IS_HEXDIGIT(c)  (('0' <= c && c <='9') || ('A' <= c && c <='F') || ('a' <= c && c <='f'))
void Tokenize(TokenStr *toke_str_list, int size, int *used, const char *s)
{
  int line_count = 1;
  while(*s){
    if(*s != '\n' && (*s <= 0x20 || *s == 0x7f || (uint8_t)*s == 0xff)){
      // Skip non printable
      s++;
    } else if(*s == '#' || (s[0] == '/' && s[1] == '/')){
      // Line comment
      while(*s && *s != '\n'){
        s++;
      }
    } else if((s[0] == '/' && s[1] == '*')){
      // Block comment
      int nest_count = 0;
      while(*s){
        if((s[0] == '/' && s[1] == '*')){
          s += 2;
          nest_count++;
        } else if((s[0] == '*' && s[1] == '/')){
          s += 2;
          nest_count--;
          if(!nest_count) break;
        } else{
          if(*s == '\n') line_count++;
          s++;
        }
      }
    } else {
      // Token cases
      if(*used >= size){
        fputs("No more space for token. Abort.\n", stderr);
        exit(EXIT_FAILURE);
      }
      TokenStr *ts = &toke_str_list[(*used)++];
      ts->line = line_count;

      if(IS_ALPHABET(*s)){
        ts->type = kWord;
        ts->str = s;
        while(IS_ALPHABET(*s) || IS_DIGIT(*s)){
          ts->len++;
          s++;
        }
      } else if(IS_DIGIT(*s)){
        ts->type = kInteger;
        ts->str = s;
        if(s[0] == '0' && s[1] == 'x'){
          ts->len = 2;
          s += 2;
          while(IS_HEXDIGIT(*s)){
            ts->len++;
            s++;
          }
        } else if(s[0] == '0' && s[1] == 'b'){
          ts->len = 2;
          s += 2;
          while(IS_BINDIGIT(*s)){
            ts->len++;
            s++;
          }
        } else{
          while(IS_DIGIT(*s)){
            ts->len++;
            s++;
          }
        }
      } else if(*s == '\n'){
        ts->type = kNewLine;
        ts->str = s++;
        ts->len = 1;
        line_count++;
      } else if(*s == '"'){
        ts->type = kString;
        ts->str = ++s;
        while(*s != '"' || s[-1] == '\\'){
          if(!*s){
            fputs("Unexpected NULL character in string literal\n", stderr);
            exit(EXIT_FAILURE);
          }
          ts->len++;
          s++;
        }
        s++;
      } else{
        ts->type = kSymbol;
        ts->str = s++;
        ts->len = 1;
      }
    }
  }
}

