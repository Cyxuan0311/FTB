cmake_minimum_required(VERSION 3.10)

# --- decoder source files (src/dec/) ---
set(WEBP_DEC_SRCS
  src/dec/alpha_dec.c
  src/dec/buffer_dec.c
  src/dec/frame_dec.c
  src/dec/idec_dec.c
  src/dec/io_dec.c
  src/dec/quant_dec.c
  src/dec/tree_dec.c
  src/dec/vp8_dec.c
  src/dec/vp8l_dec.c
  src/dec/webp_dec.c
)

# --- DSP source files (common, needed by decoder) ---
set(WEBP_DSP_COMMON_SRCS
  src/dsp/alpha_processing.c
  src/dsp/cpu.c
  src/dsp/dec.c
  src/dsp/dec_clip_tables.c
  src/dsp/filters.c
  src/dsp/lossless.c
  src/dsp/rescaler.c
  src/dsp/upsampling.c
  src/dsp/yuv.c
)

# --- utils source files (common, needed by decoder) ---
set(WEBP_UTILS_COMMON_SRCS
  src/utils/bit_reader_utils.c
  src/utils/color_cache_utils.c
  src/utils/filters_utils.c
  src/utils/huffman_utils.c
  src/utils/palette.c
  src/utils/quant_levels_dec_utils.c
  src/utils/random_utils.c
  src/utils/rescaler_utils.c
  src/utils/thread_utils.c
  src/utils/utils.c
)

add_library(webpdecoder STATIC
  ${WEBP_DEC_SRCS}
  ${WEBP_DSP_COMMON_SRCS}
  ${WEBP_UTILS_COMMON_SRCS}
)

target_include_directories(webpdecoder PUBLIC
  include
)

target_include_directories(webpdecoder PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  src
  src/dec
  src/dsp
  src/utils
  src/webp
)

target_compile_definitions(webpdecoder PRIVATE
  HAVE_CONFIG_H
  WEBP_DISABLE_THREADS
)

set_target_properties(webpdecoder PROPERTIES
  C_STANDARD 99
  C_STANDARD_REQUIRED ON
  POSITION_INDEPENDENT_CODE ON
)
