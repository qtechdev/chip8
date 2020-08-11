#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__
#include <cstddef>

#include <glad/glad.h>

static GLuint current_texture = 0;

struct Texture {
  GLuint id = 0;
};

Texture create_texture_from_data(
  const std::size_t width, const std::size_t height, const std::size_t channels,
  const unsigned char *data
);

void bindTexture(const Texture &t);

#endif // __TEXTURE_HPP__
