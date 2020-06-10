#ifndef __CHIP8_HPP__
#define __CHIP8_HPP__
#include <array>
#include <cstdint>
#include <functional>
#include <ostream>
#include <random>
#include <string>

namespace chip8 {
  constexpr uint16_t entry_point = 0x200;
  constexpr uint16_t font_index = 0x50;
  using opcode = uint16_t;

  constexpr std::array<uint8_t, 4> char_lines = {
    0xf0, // ****----
    0x90, // *--*----
    0x10, // ---*----
    0x80  // *-------
  };

  constexpr std::array<uint8_t, 5*16> char_data = {
    0, 1, 1, 1, 0, // 0
    2, 2, 2, 2, 2, // 1
    0, 2, 0, 3, 0, // 2
    0, 2, 0, 2, 0, // 3
    1, 1, 0, 2, 2, // 4
    0, 3, 0, 2, 0, // 5
    0, 3, 0, 1, 0, // 6
    0, 2, 2, 2, 2, // 7
    0, 1, 0, 1, 0, // 8
    0, 1, 0, 2, 2, // 9
    0, 1, 0, 1, 1, // a
    3, 3, 0, 1, 0, // b
    0, 3, 3, 3, 0, // c
    2, 2, 0, 1, 0, // d
    0, 1, 0, 3, 0, // e
    0, 3, 0, 3, 3  // f
  };

  struct machine {
    machine();

    std::uniform_int_distribution<uint8_t> distribution;
    std::mt19937 engine;

    static constexpr std::size_t display_width = 64;
    static constexpr std::size_t display_height = 32;
    std::array<uint8_t, 16> reg = {0};
    std::array<uint8_t, 4096> mem = {0};
    std::array<uint8_t, display_width*display_height> gfx = {0};
    std::array<uint16_t, 16> stack = {0};
    std::array<bool, 16> keys = {0};
    uint16_t pc = entry_point;
    uint16_t I = 0;
    uint8_t sp = 0;
    uint8_t delay_timer = 0;
    uint8_t sound_timer = 0;

    bool quit=false;
    bool draw=false;
    constexpr static int debug_out_size = 100;
    char debug_out[debug_out_size];
  };

  using func_t = std::function<void(machine &m, const opcode &op)>;
  opcode fetch_opcode(const machine &m);
  func_t decode_opcode(const opcode &op);
  uint8_t sprite_address(const uint8_t index);

  std::string dump_registers(const machine &m, bool ascii=false);
  std::string dump_memory(const machine &m);

  uint8_t split_x(const opcode &op);
  uint16_t split_addr(const opcode &op);
  std::array<uint8_t, 2> split_xy(const opcode &op);
  std::array<uint8_t, 2> split_rv(const opcode &op);
  std::array<uint8_t, 3> split_xyn(const opcode &op);

  void panic(machine &m, const opcode &op);
  void halt(machine &m, const opcode &op);
  // void f_0nnn(machine &m, const opcode &op);
  void f_00e0(machine &m, const opcode &op);
  void f_00ee(machine &m, const opcode &op);
  void f_1nnn(machine &m, const opcode &op);
  void f_2nnn(machine &m, const opcode &op);
  void f_3xnn(machine &m, const opcode &op);
  void f_4xnn(machine &m, const opcode &op);
  void f_5xy0(machine &m, const opcode &op);
  void f_6xnn(machine &m, const opcode &op);
  void f_7xnn(machine &m, const opcode &op);
  void f_8xy0(machine &m, const opcode &op);
  void f_8xy1(machine &m, const opcode &op);
  void f_8xy2(machine &m, const opcode &op);
  void f_8xy3(machine &m, const opcode &op);
  void f_8xy4(machine &m, const opcode &op);
  void f_8xy5(machine &m, const opcode &op);
  void f_8xy6(machine &m, const opcode &op);
  void f_8xy7(machine &m, const opcode &op);
  void f_8xye(machine &m, const opcode &op);
  void f_9xy0(machine &m, const opcode &op);
  void f_annn(machine &m, const opcode &op);
  void f_bnnn(machine &m, const opcode &op);
  void f_cxnn(machine &m, const opcode &op);
  void f_dxyn(machine &m, const opcode &op);
  void f_ex9e(machine &m, const opcode &op);
  void f_exa1(machine &m, const opcode &op);
  void f_fx07(machine &m, const opcode &op);
  void f_fx0a(machine &m, const opcode &op);
  void f_fx15(machine &m, const opcode &op);
  void f_fx18(machine &m, const opcode &op);
  void f_fx1e(machine &m, const opcode &op);
  void f_fx29(machine &m, const opcode &op);
  void f_fx33(machine &m, const opcode &op);
  void f_fx55(machine &m, const opcode &op);
  void f_fx65(machine &m, const opcode &op);
};

#endif // __CHIP8_HPP__
