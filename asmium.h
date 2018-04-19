typedef enum {
  kWord,
  kInteger,
  kNewLine,
  kString,
  kSymbol,
} TokenStrType;

typedef struct TOKEN_STR TokenStr;
struct TOKEN_STR {
  int len;
  int line;
  TokenStrType type;
  const char *str;
};

void DebugPrintTokens(const TokenStr *toke_str_list, int used);
void Tokenize(TokenStr *toke_str_list, int size, int *used, const char *s);
