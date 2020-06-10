#include <array>
#include <cstdint>
#include <iostream>

#include "chip8.hpp"
#include "util/file_io.hpp"
#include "util/timer.hpp"
#include "util/xdg.hpp"

constexpr timing::seconds update_timestep(1.0/60.0);
constexpr auto program = "disp.hex";

std::ostream &operator<<(std::ostream &os, const std::vector<uint8_t> &v) {
  for (const auto &x : v) {
    os << x;
  }
  return os;
}

int main(int argc, const char *argv[]) {
  xdg::base base_dirs = xdg::get_base_directories();

  auto log_path = xdg::get_data_path(base_dirs, "qch8", "logs/qch8.log", true);
  fio::log_stream_f log_stream(*log_path);

  auto prog_path = xdg::get_data_path(base_dirs, "qch8", program);
  if (!prog_path) { log_stream << "program not found: " << program << "\n"; }
  auto prog_data = fio::readb(*prog_path);
  if (!prog_data) { log_stream << "could not read file"; }
  log_stream << "loading program ...\n--> " << *prog_path << "\n";
  log_stream << "--> " << prog_data->size() << " bytes read" << "\n";
  log_stream << "--> " << *prog_data << "\n";

  chip8::machine m;
  // laod rom
  std::copy(prog_data->begin(), prog_data->end(), &m.mem[0x200]);

  timing::Clock clock;
  timing::Timer loop_timer;
  timing::seconds time_accumulator(0.0);

  while (!m.quit) {
    time_accumulator += loop_timer.getDelta();

    loop_timer.tick(clock.get());

    while (time_accumulator >= update_timestep) {
      chip8::opcode op = chip8::fetch_opcode(m);
      chip8::func_t f = chip8::decode_opcode(op);
      f(m, op);

      #ifdef DEBUG
      std::cout << m.debug_out << "\n";
      #endif

      // reduce timers
      --m.delay_timer;
      --m.sound_timer;

      if (m.draw) {
        for (int i = 0; i < m.display_width; i++) {
          std::cout << '-';
        }
        std::cout << '\n';

        for (std::size_t row = 0; row < m.display_height; row++) {
          for (std::size_t col = 0; col < m.display_width; col++) {
            std::size_t index = (row * m.display_width) + col;
            uint8_t pixel = m.gfx[index];
            std::cout << (pixel == 0 ? " " : "█");
          }
          std::cout << '\n';
        }

        for (int i = 0; i < m.display_width; i++) {
          std::cout << '-';
        }
        std::cout << '\n';

        m.draw = false;
      }

      //process input
      time_accumulator -= update_timestep;
    }
  }

  // std::cout << dump_memory(m) << "\n";
  // std::cout << dump_registers(m) << "\n";

  return 0;
}
