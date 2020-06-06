#include <array>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

#include "chip8.hpp"

chip8::machine::machine() {
  for (int i = 0; i < char_data.size(); i++) {
    mem[font_index + i] = char_lines[char_data[i]];
  }

  engine.seed(std::random_device{}());
};

chip8::opcode chip8::fetch_opcode(const machine &m) {
  return opcode(m.mem.at(m.pc) << 8) | m.mem.at(m.pc + 1);
}

chip8::func_t chip8::decode_opcode(const opcode &op) {
  if (op == 0x0000) { return halt; }
  if (op == 0x00e0) { return f_00e0; }
  if (op == 0x00ee) { return f_00ee; }
  if ((op & 0xf000) == 0x1000) { return f_1nnn; }
  if ((op & 0xf000) == 0x2000) { return f_2nnn; }
  if ((op & 0xf000) == 0x3000) { return f_3xnn; }
  if ((op & 0xf000) == 0x4000) { return f_4xnn; }
  if ((op & 0xf000) == 0x5000) { return f_5xy0; }
  if ((op & 0xf000) == 0x6000) { return f_6xnn; }
  if ((op & 0xf000) == 0x7000) { return f_7xnn; }
  if ((op & 0xf00f) == 0x8000) { return f_8xy0; }
  if ((op & 0xf00f) == 0x8001) { return f_8xy1; }
  if ((op & 0xf00f) == 0x8002) { return f_8xy2; }
  if ((op & 0xf00f) == 0x8003) { return f_8xy3; }
  if ((op & 0xf00f) == 0x8004) { return f_8xy4; }
  if ((op & 0xf00f) == 0x8005) { return f_8xy5; }
  if ((op & 0xf00f) == 0x8006) { return f_8xy6; }
  if ((op & 0xf00f) == 0x8007) { return f_8xy7; }
  if ((op & 0xf00f) == 0x800e) { return f_8xye; }
  if ((op & 0xf00f) == 0x9000) { return f_9xy0; }
  if ((op & 0xf000) == 0xa000) { return f_annn; }
  if ((op & 0xf000) == 0xb000) { return f_bnnn; }
  if ((op & 0xf000) == 0xc000) { return f_cxnn; }
  if ((op & 0xf000) == 0xd000) { return f_dxyn; }
  if ((op & 0xf0ff) == 0xe09e) { return f_ex9e; }
  if ((op & 0xf0ff) == 0xe0a1) { return f_exa1; }
  if ((op & 0xf0ff) == 0xf007) { return f_fx07; }
  if ((op & 0xf0ff) == 0xf00a) { return f_fx0a; }
  if ((op & 0xf0ff) == 0xf015) { return f_fx15; }
  if ((op & 0xf0ff) == 0xf018) { return f_fx18; }
  if ((op & 0xf0ff) == 0xf01e) { return f_fx1e; }
  if ((op & 0xf0ff) == 0xf029) { return f_fx29; }
  if ((op & 0xf0ff) == 0xf033) { return f_fx33; }
  if ((op & 0xf0ff) == 0xf055) { return f_fx55; }
  if ((op & 0xf0ff) == 0xf065) { return f_fx65; }

  return panic;
}

uint8_t chip8::sprite_address(const uint8_t index) {
  return font_index + (index * 5);
}

std::string chip8::dump_registers(const machine &m, bool ascii) {
  std::stringstream ss;
  char buf[13];
  ss << "| ";
  for (int i = 0; i < 8; i++) {
    snprintf(buf, 5, "  V%X", i);
    ss << buf << " |";
  }
  ss << "\n| ";
  for (int i = 0; i < 8; i++) {
    snprintf(buf, 5, (ascii ? "%4c" : "%#4x"), m.reg[i]);
    ss << buf << " |";
  }
  ss << "\n\n| ";

  for (int i = 8; i < 16; i++) {
    snprintf(buf, 5, "  V%X", i);
    ss << buf << " |";
  }
  ss << "\n| ";
  for (int i = 8; i < 16; i++) {
    snprintf(buf, 5, (ascii ? "%4c" : "%#4x"), m.reg[i]);
    ss << buf << " |";
  }
  ss << "\n\n| ";


  ss << "         I |        pc |  sp |  dt |  st |\n| ";
  snprintf(buf, 13, "    %#6x", m.I);
  ss << buf << " |";
  snprintf(buf, 13, "    %#6x", m.pc);
  ss << buf << " |";
  snprintf(buf, 5, "%#4x", m.sp);
  ss << buf << " |";
  snprintf(buf, 5, "%#4x", m.delay_timer);
  ss << buf << " |";
  snprintf(buf, 5, "%#4x", m.sound_timer);
  ss << buf << " |";

  return ss.str();
}

std::string chip8::dump_memory(const machine &m) {
  std::stringstream ss;

  for (int i = 0; i < 4096; i += 16) {
    std::stringstream line;
    char buf[5];
    snprintf(buf, 5, "%04x", i);

    line << buf << " ";

    for (int j = 0; j < 16; j++) {
      snprintf(buf, 3, "%02x", m.mem[i + j]);
      line << buf << " ";
    }

    line << " : ";

    for (int j = 0; j < 16; j++) {
      char c = m.mem[i + j];
      snprintf(buf, 2, ((c >= ' ' && c <= '~') ? "%c" : "."), c);
      line << buf;
    }

    ss << line.str() << "\n";
  }

  return ss.str();
}

uint8_t chip8::split_x(const opcode &op) {
  return (op & 0x0f00) >> 8;
}

uint16_t chip8::split_nnn(const opcode &op) {
  return op & 0x0fff;
}

std::array<uint8_t, 2> chip8::split_xy(const opcode &op) {
  uint8_t x = (op & 0x0f00) >> 8;
  uint8_t y = (op & 0x00f0) >> 4;

  return {x, y};
}

std::array<uint8_t, 2> chip8::split_xnn(const opcode &op) {
  uint8_t x = (op & 0x0f00) >> 8;
  uint8_t nn = (op & 0x00ff);

  return {x, nn};
}

std::array<uint8_t, 3> chip8::split_xyn(const opcode &op) {
  uint8_t x = (op & 0x0f00) >> 8;
  uint8_t y = (op & 0x00f0) >> 4;
  uint8_t n = (op & 0x000f);

  return {x, y, n};
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
void chip8::f_00e0(machine &m, const opcode &op) { // clear
  m.gfx.fill(0);
  m.pc += 2;
}
void chip8::f_00ee(machine &m, const opcode &op) { // ret
  m.pc = m.stack[m.sp--];
}

void chip8::f_1nnn(machine &m, const opcode &op) { // jmp [addr]
  m.pc = (op & 0x0fff);
}

void chip8::f_2nnn(machine &m, const opcode &op) { // call [addr]
  m.stack[m.sp++] = m.pc;
  m.pc = op & 0x0fff;
}

void chip8::f_3xnn(machine &m, const opcode &op) { // beq [r] [v]
  auto [r, v] = split_xnn(op);
  if (m.reg[r] == v) {
    m.pc += 2;
  }
  m.pc += 2;
}

void chip8::f_4xnn(machine &m, const opcode &op) { // bne [r] [v]
  auto [r, v] = split_xnn(op);
  if (m.reg[r] != v) {
    m.pc += 2;
  }
  m.pc += 2;
}

void chip8::f_5xy0(machine &m, const opcode &op) { // beqr [x] [y]
  auto [x, y] = split_xy(op);
  if (m.reg[x] == m.reg[y]) {
    m.pc += 2;
  }
  m.pc += 2;
}

void chip8::f_6xnn(chip8::machine &m, const chip8::opcode &op) { // mov [r] [v]
  auto [r, v] = split_xnn(op);
  m.reg[r] = v;
  m.pc += 2;
}

void chip8::f_7xnn(machine &m, const opcode &op) { // add [r] [v]
  auto [r, v] = split_xnn(op);
  m.reg[r] += v;
  m.pc += 2;
}

void chip8::f_8xy0(machine &m, const opcode &op) { // movr [x] [y]
  auto [x, y] = split_xy(op);
  m.reg[x] = m.reg[y];
  m.pc += 2;
}

void chip8::f_8xy1(machine &m, const opcode &op) { // or [x] [y]
  auto [x, y] = split_xy(op);
  m.reg[x] |= m.reg[y];
  m.pc += 2;
}

void chip8::f_8xy2(machine &m, const opcode &op) { // and [x] [y]
  auto [x, y] = split_xy(op);
  m.reg[x] &= m.reg[y];
  m.pc += 2;
}

void chip8::f_8xy3(machine &m, const opcode &op) { // xor [x] [y]
  auto [x, y] = split_xy(op);
  m.reg[x] ^= m.reg[y];
  m.pc += 2;
}

void chip8::f_8xy4(machine &m, const opcode &op) { // add [x] [y]
  auto [x, y] = split_xy(op);
  uint16_t tmp = m.reg[x] + m.reg[y];
  // set flag if carry (i.e. 9th bit is set)
  m.reg[0xf] = (tmp & 0x100) >> 8;
  m.reg[x] = tmp & 0xff;
  m.pc += 2;
}

void chip8::f_8xy5(machine &m, const opcode &op) { // sub [x] [y]
  auto [x, y] = split_xy(op);
  uint16_t tmp = m.reg[x] - m.reg[y];
  // set flag if borrow (i.e. 9th bit is not set)
  m.reg[0xf] =  !((tmp & 0x100) >> 8);
  m.reg[x] = tmp & 0xff;
  m.pc += 2;
}

void chip8::f_8xy6(machine &m, const opcode &op) { // slr [x]
  auto x = split_x(op);
  m.reg[0xf] =  m.reg[x] & 0x01;
  m.reg[x] >>= 1;
  m.pc += 2;
}

void chip8::f_8xy7(machine &m, const opcode &op) { // rsub [x] [y]
  auto [x, y] = split_xy(op);
  uint16_t tmp = m.reg[y] - m.reg[x];
  // set flag if borrow (i.e. 9th bit is not set)
  m.reg[0xf] =  !((tmp & 0x100) >> 8);
  m.reg[x] = tmp & 0xff;
}

void chip8::f_8xye(machine &m, const opcode &op) { // sll [x]
  auto x = split_x(op);
  m.reg[0xf] =  (m.reg[x] & 0x70) >> 7;
  m.reg[x] <<= 1;
  m.pc += 2;
}

void chip8::f_9xy0(machine &m, const opcode &op) { // bner [x] [y]
  auto [x, y] = split_xy(op);
  if (m.reg[x] != m.reg[y]) {
    m.pc += 2;
  }
  m.pc += 2;
}

void chip8::f_annn(machine &m, const opcode &op) { // movi [addr]
  auto n = split_nnn(op);
  m.I = n;
  m.pc += 2;
}

void chip8::f_bnnn(machine &m, const opcode &op) { // jmpv [addr]
  m.pc = (op & 0x0fff) + m.reg[0];
}

void chip8::f_cxnn(machine &m, const opcode &op) { // rand [r] [v]
  uint8_t rng = m.distribution(m.engine);
  auto [r, v] = split_xnn(op);
  m.reg[r] = (rng % 256) & v;
  m.pc += 2;
}
void chip8::f_dxyn(machine &m, const opcode &op) { /**/ m.pc += 2; }
void chip8::f_ex9e(machine &m, const opcode &op) { /**/ m.pc += 2; }
void chip8::f_exa1(machine &m, const opcode &op) { /**/ m.pc += 2; }
void chip8::f_fx07(machine &m, const opcode &op) { // std [r]
  auto x = split_x(op);
  m.reg[x] = m.delay_timer;
  m.pc += 2;
}

void chip8::f_fx0a(machine &m, const opcode &op) { /**/ m.pc += 2; }
void chip8::f_fx15(machine &m, const opcode &op) { // ldd [r]
  auto x = split_x(op);
  m.delay_timer = m.reg[x];
  m.pc += 2;
}

void chip8::f_fx18(machine &m, const opcode &op) { // lds [r]
  auto x = split_x(op);
  m.sound_timer = m.reg[x];
  m.pc += 2;
}

void chip8::f_fx1e(machine &m, const opcode &op) { // addi [x]
  auto x = split_x(op);
  uint16_t tmp = m.reg[x] + m.I;
  // set flag if carry (i.e. 9th bit is set)
  m.reg[0xf] = (tmp & 0x100) >> 8;
  m.I = tmp & 0xff;
  m.pc += 2;
}

void chip8::f_fx29(machine &m, const opcode &op) { // addr [x]
  auto x = split_x(op);
  m.I = sprite_address(x);
  m.pc += 2;
}

void chip8::f_fx33(machine &m, const opcode &op) { // bcd [x]
  auto x = split_x(op);
  m.mem[m.I] = m.reg[x] / 100;
  m.mem[m.I + 1] = (m.reg[x] / 10) % 10;
  m.mem[m.I + 2] = m.reg[x] % 10;
  m.pc += 2;
}

void chip8::f_fx55(machine &m, const opcode &op) { // str [x]
  auto x = split_x(op);
  for (int i = 0; i < x; i++) {
    m.mem[m.I + i] = m.reg[i];
  }
  m.pc += 2;
}

void chip8::f_fx65(machine &m, const opcode &op) { // ldr [x]
  auto x = split_x(op);
  for (int i = 0; i < x; i++) {
    m.reg[i] = m.mem[m.I + i];
  }
  m.pc += 2;
}
