#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>

using namespace std;

class Imm {
public:
  Imm(const vector<uint8_t> &bin, vector<uint8_t>::const_iterator &it,
      int size) {
    assert(size == 1 || size == 2 || size == 4 || size == 8);
    size_ = size;
    value_ = 0;
    for (int i = 0; i < size; i++) {
      assert(it != bin.end());
      value_ |= (static_cast<uint64_t>(*it) << (i * 8));
      ++it;
    }
  }
  void Print() {
    if (size_ == 1) {
      printf("0x%02llX", value_);
    } else if (size_ == 2) {
      printf("0x%04llX", value_);
    } else if (size_ == 4) {
      printf("0x%08llX", value_);
    } else if (size_ == 8) {
      printf("0x%016llX", value_);
    }
  }
  uint64_t GetByteSize() { return size_; }

private:
  uint64_t value_;
  uint64_t size_;
};

class ModRM {
public:
  ModRM(uint8_t byte) {
    mod_ = (byte >> 6) & 0b11;
    reg_ = (byte >> 3) & 0b111;
    r_m_ = byte & 0b111;
  }
  void Print() { printf("ModRM(%01X %01X %01X) ", mod_, reg_, r_m_); }
  int GetNumOfDispBytes() {
    // Table 2-2. 32-Bit Addressing Forms with the ModR/M Byte
    if (mod_ == 0b00 && r_m_ == 0b101) {
      return 4;
    }
    if (mod_ == 0b01) {
      return 1;
    }
    if (mod_ == 0b10) {
      return 4;
    }
    return 0;
  }
  const char *GetRegName32() {
    static const char *kRegName32[8] = {
        "EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI",
    };
    assert(reg_ < 8);
    return kRegName32[reg_];
  }
  void PrintEffectiveAddress(Imm *disp) {
    if (mod_ == 0b00 && r_m_ == 0b101) {
      // Table 2-7. RIP-Relative Addressing
      // In protected / compatibility mode: [Disp32]
      // In 64-bit mode: [RIP + Disp32]
      assert(disp);
      assert(disp->GetByteSize() == 4);
      printf("[");
      disp->Print();
      printf("]");
      return;
    }
    assert(false);
  };

private:
  uint8_t mod_;
  uint8_t reg_;
  uint8_t r_m_;
};

class Op {
public:
  Op(const vector<uint8_t> &bin, vector<uint8_t>::const_iterator &it) {
    assert(it != bin.end());
    begin_ = it;
    op0_ = static_cast<Opcode0>(*it);
    if (op0_ == Opcode0::kADDGvEv) {
      // ADD Gv, Ev
      ++it;
      mod_rm_ = new ModRM(*it);
      ++it;
      int disp_size = mod_rm_->GetNumOfDispBytes();
      if (disp_size) {
        disp_ = new Imm(bin, it, disp_size);
      }
      length_ = it - begin_;
      return;
    }
    ++it;
    length_ = it - begin_;
  }
  void Print() {
    for (int i = 0; i < length_; i++) {
      printf("%02X%c", *(begin_ + i), (i < length_ - 1) ? ' ' : '\t');
    }
    printf("%s ", GetMnemonic());
    if (op0_ == Opcode0::kADDGvEv) {
      assert(mod_rm_);
      printf("%s, ", mod_rm_->GetRegName32());
      mod_rm_->PrintEffectiveAddress(disp_);
    }
    printf("\n");
  }
  const char *GetMnemonic() {
    if (op0_ == Opcode0::kADDGvEv) {
      return "ADD";
    }
    return "(Unknown)";
  }

private:
  enum class Opcode0 : uint8_t {
    kADDGvEv = 0x03,
  };
  int length_;
  vector<uint8_t>::const_iterator begin_;
  Opcode0 op0_;
  ModRM *mod_rm_ = nullptr;
  Imm *disp_ = nullptr;
};

void Disassemble(const vector<uint8_t> &bin) {
  auto begin = bin.begin();
  auto it = begin;
  while (it != bin.end()) {
    Op op(bin, it);
    op.Print();
  }
}

int main(int argc, char *argv[]) {
  // Example A-1. Look-up Example for 1-Byte Opcodes
  // ADD EAX, [0x0000'0000]
  vector<uint8_t> bin{
      0x03, 0x05, 0x00, 0x00, 0x00, 0x00, // ADD EAX, [0x00000000]
      0x03, 0x05, 0x78, 0x56, 0x34, 0x12, // ADD EAX, [0x12345678]
      0x03, 0x15, 0x58, 0x0e, 0x03, 0x00, // ADD EDX, [0x00030E58]
  };
  Disassemble(bin);
  return 0;
}
