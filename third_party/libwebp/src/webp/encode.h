#ifndef WEBP_WEBP_ENCODE_H_
#define WEBP_WEBP_ENCODE_H_

#include "src/webp/types.h"

typedef struct WebPPicture {
  int use_argb;
  uint32_t* argb;
  int argb_stride;
  int width;
  int height;
} WebPPicture;

#endif
