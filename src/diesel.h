#ifndef DIESEL_H
#define DIESEL_H

// Spitfire headers
#include <spitfire/util/string.h>

#define DIESEL_WEB_SERVER_PORT 38002

#define INVALID_PHOTO nullptr

namespace diesel
{
  using spitfire::char_t;
  using spitfire::string_t;

  enum class IMAGE_SIZE {
    THUMBNAIL,
    FULL
  };
}

#endif // DIESEL_H
