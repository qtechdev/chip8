#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <regex>

#include "glad.h"
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "chip8.hpp"
#include "gl/rect.hpp"
#include "gl/shader_program.hpp"
#include "gl/texture.hpp"
#include "gl/window.hpp"
#include "util/error.hpp"
#include "util/file_io.hpp"
#include "util/timer.hpp"
#include "util/xdg.hpp"

static constexpr int window_width = 640;
static constexpr int window_height = 480;
static constexpr int gl_major_version = 3;
static constexpr int gl_minor_version = 3;

static const std::map<int, uint8_t> key_map = {
  {GLFW_KEY_X, 0x0},
  {GLFW_KEY_1, 0x1},
  {GLFW_KEY_2, 0x2},
  {GLFW_KEY_3, 0x3},
  {GLFW_KEY_Q, 0x4},
  {GLFW_KEY_W, 0x5},
  {GLFW_KEY_E, 0x6},
  {GLFW_KEY_A, 0x7},
  {GLFW_KEY_S, 0x8},
  {GLFW_KEY_D, 0x9},
  {GLFW_KEY_Z, 0xa},
  {GLFW_KEY_C, 0xb},
  {GLFW_KEY_4, 0xc},
  {GLFW_KEY_R, 0xd},
  {GLFW_KEY_F, 0xe},
  {GLFW_KEY_V, 0xf}
};

constexpr timing::seconds update_timestep(1.0/60.0);
static const std::regex program_re(R"re(.*(\.ch8)$)re");

#ifdef DEBUG
namespace xdg {
  std::optional<xdg::path_t> get_data_path(
    const xdg::base &b, const std::string &n, const std::string &p,
    fio::log_stream_f &log_stream, const bool create=false
  );
}
namespace fio {
  std::optional<xdg::path_t> read(
    const xdg::path_t &path, fio::log_stream_f &log_stream
  );
}
#endif

void processInput(GLFWwindow *window, chip8::machine &m);
std::array<glm::mat4, 3> fullscreen_rect_matrices(const int w, const int h);

int main(int argc, const char *argv[]) {
  // get base directories and init logger
  xdg::base base_dirs = xdg::get_base_directories();
  auto log_path = xdg::get_data_path(base_dirs, "qchip", "logs/qchip.log", true);
  fio::log_stream_f log_stream(*log_path);
  log_stream << "GLFW Version: " << glfwGetVersionString() << "\n";

  auto program_files = xdg::search_data_dirs(base_dirs, "qchip", program_re);

  int index = 0;
  while ((index < 1) || (index > program_files.size())) {
    std::cout << "Choose program!\n";
    for (int i = 0; i < program_files.size(); i++) {
      std::cout << (i + 1) << ") " << program_files[i] << "\n";
    }

    std::cin >> index;
    if (std::cin.fail()) {
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
  }

  std::string program_path = program_files[index - 1];

  // create opengl window and context
  GLFWwindow *window = createWindow(
    gl_major_version, gl_minor_version, true, window_width, window_height,
    "qChip8"
  );

  if (window == nullptr) {
    #ifdef DEBUG
    log_stream << "failed to create window\n";
    #endif

    glfwDestroyWindow(window);
    return to_underlying(error_code_t::window_failed);
  }

  glfwMakeContextCurrent(window);

  // load opengl functions
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    #ifdef DEBUG
    log_stream << "failed to initialise GLAD\n";
    #endif

    return to_underlying(error_code_t::glad_failed);
  }

  glViewport(0, 0, window_width, window_height);
  glClearColor(0.1, 0.1, 0.2, 1.0);

  log_stream << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";

  // load shaders
  auto v_shader_path = xdg::get_data_path(
    base_dirs, "qchip", "shaders/tex/vshader.glsl"
    #ifdef DEBUG
    , log_stream
    #endif
  );
  auto v_shader_string = fio::read(*v_shader_path);

  auto f_shader_path = xdg::get_data_path(
    base_dirs, "qchip", "shaders/tex/fshader.glsl"
    #ifdef DEBUG
    , log_stream
    #endif
  );
  auto f_shader_string = fio::read(*f_shader_path);

  GLuint v_shader = createShader(GL_VERTEX_SHADER, *v_shader_string);
  GLuint f_shader = createShader(GL_FRAGMENT_SHADER, *f_shader_string);
  #ifdef DEBUG
  const auto v_compile_error = getCompileStatus(v_shader);
  if (v_compile_error) {
    log_stream << "vertex shader compilation failed\n";
    log_stream << *v_compile_error << "\n";
  }
  const auto f_compile_error = getCompileStatus(f_shader);
  if (f_compile_error) {
    log_stream << "fragment shader compilation failed\n";
    log_stream << *f_compile_error << "\n";
  }
  #endif

  GLuint shader_program = createProgram(v_shader, f_shader, true);
  #ifdef DEBUG
  const auto link_error = getLinkStatus(shader_program);
  if (link_error) {
    log_stream << "shader program link failed\n";
    log_stream << *link_error << "\n";
  }
  #endif

  // chip8 virtual machine
  chip8::machine m;
  m.draw = true; // force screen refresh at program start

  // initialise texture
  constexpr std::size_t num_cols = m.display_width;
  constexpr std::size_t num_rows = m.display_height;
  constexpr std::size_t num_channels = 1;
  constexpr std::size_t image_stride = num_cols * num_channels;
  constexpr std::size_t data_size = num_cols*num_rows*num_channels;
  unsigned char data[num_cols*num_rows*num_channels];

  for (int i = 0; i < data_size; i++) {
    data[i] = 0;
  }

  Texture texture = create_texture_from_data(
    num_cols, num_rows, num_channels, data
  );
  // create screen rect
  Rect rect = createRect();

  auto [projection, view, model] = fullscreen_rect_matrices(
    window_width, window_height
  );

  uniformMatrix4fv(shader_program, "projection", glm::value_ptr(projection));
  uniformMatrix4fv(shader_program, "view", glm::value_ptr(view));
  uniformMatrix4fv(shader_program, "model", glm::value_ptr(model));

  // load program from file
  auto program_data = fio::readb(program_path);
  if (!program_data) { log_stream << "could not read file"; }
  log_stream << "loading program ...\n--> " << program_path << "\n";
  log_stream << "--> " << program_data->size() << " bytes read" << "\n";
  std::copy(program_data->begin(), program_data->end(), &m.mem[0x200]);

  timing::Clock clock;
  timing::Timer loop_timer;
  timing::seconds time_accumulator(0.0);

  while (!m.quit && !glfwWindowShouldClose(window)) {
    time_accumulator += loop_timer.getDelta();
    loop_timer.tick(clock.get());

    //process input
    glfwPollEvents();
    processInput(window, m);

    while (time_accumulator >= update_timestep) {
      if (!m.blocking) {
        if (m.halted) {
          time_accumulator -= update_timestep;
          continue;
        }

        chip8::opcode op = chip8::fetch_opcode(m);
        chip8::func_t f = chip8::decode_opcode(op);
        f(m, op);

        #ifdef DEBUG
        std::cout << m.debug_out << "\n";
        #endif

        // reduce timers
        --m.delay_timer;
        --m.sound_timer;
      } else {
        chip8::get_key(m);
      }

      if (m.draw) {
        bindTexture(texture);
        glTexSubImage2D(
          GL_TEXTURE_2D, 0, 0, 0, m.display_width, m.display_height,
          GL_RED, GL_UNSIGNED_BYTE, m.gfx.data()
        );
        bindTexture({0});
        m.draw = false;
      }

      time_accumulator -= update_timestep;
    }

    // draw screen texture
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program);
    bindTexture(texture);
    drawRect(rect);
    glfwSwapBuffers(window);
  }

  #ifdef DEBUG
  // std::cout << dump_memory(m) << "\n";
  // std::cout << dump_graphics_data(m) << "\n";
  std::cout << dump_registers(m) << "\n";
  #endif

  return 0;
}

#ifdef DEBUG
std::optional<xdg::path_t> xdg::get_data_path(
  const xdg::base &b, const std::string &n, const std::string &p,
  fio::log_stream_f &log_stream, const bool create
) {
  log_stream << "Fetching path: " << p << "\n";
  auto path = xdg::get_data_path(b, n, p);
  if (!path) {
    log_stream << "[w] `" << p << "` not found...\n";
  } else {
    log_stream << "--> " << *path << "\n";
  }

  return path;
}

std::optional<xdg::path_t> fio::read(
  const xdg::path_t &path, fio::log_stream_f &log_stream
) {
  log_stream << "Loading file: " << path << "\n";
  auto data = fio::read(path);
  if (!data) {
    log_stream << "[w] Could not read file...\n";
  }

  return data;
}
#endif

void processInput(GLFWwindow *window, chip8::machine &m) {
  if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }

  for (auto &[k, v] : key_map) {
    int state = glfwGetKey(window, k);

    if (state == GLFW_PRESS) {
      m.keys[v] = true;
    } else if (state == GLFW_RELEASE) {
      m.keys[v] = false;
    }
  }
}

std::array<glm::mat4, 3> fullscreen_rect_matrices(const int w, const int h) {
  glm::mat4 projection = glm::ortho<double>(0, w, 0, h, 0.1, 100.0);

  glm::mat4 view = glm::mat4(1.0);
  view = glm::translate(view, glm::vec3(0.0, 0.0, -1.0));

  glm::mat4 model = glm::mat4(1.0);
  model = glm::scale(model, glm::vec3(w, h, 1));

  return {projection, view, model};
}
