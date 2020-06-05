#ifndef __CHIP8_HPP__
#define __CHIP8_HPP__
#include <array>
#include <cstdint>
#include <functional>
#include <ostream>
#include <string>

namespace chip8 {
  using opcode = uint16_t;

  struct machine {
    std::array<uint8_t, 16> reg = {0};
    std::array<uint8_t, 4096> mem = {0};
    std::array<uint8_t, 64*32> gfx = {0};
    std::array<uint16_t, 16> stack = {0};
    std::array<bool, 16> keys = {0};
    uint16_t pc = 0x200;
    uint16_t I = 0;
    uint8_t sp = 0;
    uint8_t delay_timer = 0;
    uint8_t sound_timer = 0;

    bool quit=false;
    bool draw=false;
  };

  using func_t = std::function<void(machine &m, const opcode &op)>;
  opcode fetch_opcode(const machine &m);
  func_t decode_opcode(const opcode &op);

  std::string dump_registers(const machine &m, bool ascii=false);

  std::array<uint8_t, 2> split_xnn(const opcode &op);

  void panic(machine &m, const opcode &op);
  void halt(machine &m, const opcode &op);
  void f_6xnn(machine &m, const opcode &op);
};

#endif // __CHIP8_HPP__
