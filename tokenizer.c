#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "asmium.h"

void PrintTokenStr(const TokenStr *ts) {
  write(STDOUT_FILENO, ts->str, ts->len);
}
void DebugPrintTokenStr(const TokenStr *ts) {
  if (ts->type == kIdentifier || ts->type == kInteger ||
      ts->type == kOperator || ts->type == kMemOfsBegin || ts->type == kMemOfsEnd) {
    fflush(stdout);
    write(STDOUT_FILENO, ts->str, ts->len);
    putchar(' ');
  } else if (ts->type == kString) {
    putchar('"');
    fflush(stdout);
    write(STDOUT_FILENO, ts->str, ts->len);
    putchar('"');
  } else if (ts->type == kLabel) {
    putchar(':');
    fflush(stdout);
    write(STDOUT_FILENO, ts->str, ts->len);
  } else {
    putchar('<');
    printf("%02X", ts->str[0]);
    putchar('>');
  }
}

void DebugPrintTokens(const TokenStr *toke_str_list, int used) {
  int line = 0;
  for (int i = 0; i < used; i++) {
    if (line != toke_str_list[i].line) {
      line = toke_str_list[i].line;
      printf("\n%d\t", line);
    }
    DebugPrintTokenStr(&toke_str_list[i]);
  }
  putchar('\n');
}

#define IS_TOKEN_CHAR(c) \
  (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || (c == '_'))
#define IS_DIGIT(c) (('0' <= c && c <= '9'))
#define IS_BINDIGIT(c) (('0' <= c && c <= '1'))
#define IS_HEXDIGIT(c) \
  (('0' <= c && c <= '9') || ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f'))
void Tokenize(TokenStr *toke_str_list, int size, int *used, const char *s) {
  int line_count = 1;
  while (*s) {
    if ((*s <= 0x20 || *s == 0x7f || (uint8_t)*s == 0xff)) {
      // Skip non printable
      if (*s == '\n') line_count++;
      s++;
    } else if (*s == '#' || (s[0] == '/' && s[1] == '/')) {
      // Line comment
      while (*s && *s != '\n') {
        s++;
      }
    } else if ((s[0] == '/' && s[1] == '*')) {
      // Block comment
      int nest_count = 0;
      while (*s) {
        if ((s[0] == '/' && s[1] == '*')) {
          s += 2;
          nest_count++;
        } else if ((s[0] == '*' && s[1] == '/')) {
          s += 2;
          nest_count--;
          if (!nest_count) break;
        } else {
          if (*s == '\n') line_count++;
          s++;
        }
      }
      if (nest_count) {
        fputs("Block comment marker is not balanced.\n", stderr);
        exit(EXIT_FAILURE);
      }
    } else {
      // Token cases
      if (*used >= size) {
        fputs("No more space for token. Abort.\n", stderr);
        exit(EXIT_FAILURE);
      }
      TokenStr *ts = &toke_str_list[(*used)++];
      ts->line = line_count;

      if (IS_TOKEN_CHAR(*s)) {
        ts->type = kIdentifier;
        ts->str = s;
        while (IS_TOKEN_CHAR(*s) || IS_DIGIT(*s)) {
          ts->len++;
          s++;
        }
      } else if (*s == ':') {
        ts->type = kLabel;
        ts->str = ++s;  // skip ':'
        ts->len = 0;
        while (IS_TOKEN_CHAR(*s) || IS_DIGIT(*s)) {
          ts->len++;
          s++;
        }
      } else if (IS_DIGIT(*s)) {
        ts->type = kInteger;
        ts->str = s;
        if (s[0] == '0' && s[1] == 'x') {
          ts->len = 2;
          s += 2;
          while (IS_HEXDIGIT(*s)) {
            ts->len++;
            s++;
          }
        } else if (s[0] == '0' && s[1] == 'b') {
          ts->len = 2;
          s += 2;
          while (IS_BINDIGIT(*s)) {
            ts->len++;
            s++;
          }
        } else {
          while (IS_DIGIT(*s)) {
            ts->len++;
            s++;
          }
        }
      } else if (*s == '"') {
        ts->type = kString;
        ts->str = ++s;
        while (*s != '"' || s[-1] == '\\') {
          if (!*s) {
            fputs("Unexpected NULL character in string literal\n", stderr);
            exit(EXIT_FAILURE);
          }
          ts->len++;
          s++;
        }
        s++;
      } else if(*s == '[') {
        ts->type = kMemOfsBegin;
        ts->str = s++;
        ts->len = 1;
      } else if(*s == ']') {
        ts->type = kMemOfsEnd;
        ts->str = s++;
        ts->len = 1;
      } else {
        ts->type = kOperator;
        ts->str = s++;
        ts->len = 1;
        if ((ts->str[0] == '^' && ts->str[1] == '=') ||
            (ts->str[0] == '+' && ts->str[1] == '+')) {
          s++;
          ts->len++;
        }
      }
    }
  }
}
