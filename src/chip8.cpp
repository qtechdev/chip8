#include <array>
#include <cstdio>
#include <functional>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

#include "chip8.hpp"

std::array<uint8_t, 2> chip8::split_xnn(const opcode &op) {
  uint8_t x = (op & 0x0f00) >> 8;
  uint8_t nn = (op & 0x00ff);

  return {x, nn};
}

void chip8::panic(machine &m, const opcode &op) { // PANIC
  char buf[7];
  snprintf(buf,  7, "%#4x", op);

  std::cerr << "Unknown Instruction! [" << buf << "]\n";
  m.quit = true;
}

void chip8::halt(machine &m, const opcode &op) { // HALT
  m.quit = true;
}

void chip8::f_6xnn(chip8::machine &m, const chip8::opcode &op) { // mov [r] [v]
  auto [r, v] = split_xnn(op);
  m.reg[r] = v;
  m.pc += 2;
}

chip8::opcode chip8::fetch_opcode(const machine &m) {
  return opcode(m.mem.at(m.pc) << 8) | m.mem.at(m.pc + 1);
}

chip8::func_t chip8::decode_opcode(const opcode &op) {
  if (op == 0x0000) {
    return halt;
  } else if ((op & 0xf000) == 0x6000) {
    return f_6xnn;
  }

  return panic;
}

std::string chip8::dump_registers(const machine &m, bool ascii) {
  std::stringstream ss;

  for (int i = 0; i < 16; i++) {
    char buf[5];
    snprintf(buf, 5, "  V%X", i);

    ss << buf << " ";
  }

  ss << "\n";

  for (const auto &c : m.reg) {
    char buf[5];
    snprintf(buf, 5, (ascii ? "%4c" : "%#4x"), c);

    ss << buf << " ";
  }

  return ss.str();
}
