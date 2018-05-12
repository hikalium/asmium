typedef enum {
  kIdentifier,
  kInteger,
  kString,
  kOperator,
  kLabel,
  kMemOfsBegin,
  kMemOfsEnd,
} TokenStrType;

typedef struct TOKEN_STR TokenStr;
struct TOKEN_STR {
  int len;
  int line;
  TokenStrType type;
  const char *str;
};

typedef enum {
  kReg8,
  kReg16,
  kReg32,
  kReg64Legacy,
  kReg64Low,
  kReg64Hi,
  kSegReg,
} RegisterType;

typedef struct {
  RegisterType category;
  int number;
} RegisterInfo;

typedef struct {
  int number;
} OperatorInfo;

typedef enum {
  kImm,
  kReg,
  kMem,
  kLabelName,
} OperandType;

typedef struct {
  OperandType type;
  const TokenStr *token;  // kLabelName
  RegisterInfo reg_info;  // kReg
  int64_t imm;  // kImm
  RegisterInfo reg_index;  // kMem
} Operand;

typedef struct {
  const char *name;
  int (*parse)(const Operand *left, const Operand *right);
} OpEntry;

typedef struct {
  const char *mnemonic;
  int (*parse)(const TokenStr *tokens, int num_of_tokens, int index);
} MnemonicEntry;

void DebugPrintTokens(const TokenStr *toke_str_list, int used);
void Tokenize(TokenStr *toke_str_list, int size, int *used, const char *s);

// gen_macho.c
extern uint8_t mach_o_header[0x130];
extern uint8_t mach_o_footer[0x18];

