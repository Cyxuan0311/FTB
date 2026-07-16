cmake_minimum_required(VERSION 3.10)

set(GIFLIB_SRCS
  src/dgif_lib.c
  src/gif_err.c
  src/gif_hash.c
  src/gifalloc.c
)

# reallocarray 检测
include(CheckCSourceCompiles)
check_c_source_compiles("
  #include <stdlib.h>
  int main(void) { return !!reallocarray(NULL, 1, 1); }
" has_reallocarray)
if(has_reallocarray)
  set(GIFLIB_NEED_REALLOCARRAY 0)
else()
  list(APPEND GIFLIB_SRCS src/openbsd-reallocarray.c)
  set(GIFLIB_NEED_REALLOCARRAY 1)
endif()

add_library(giflib STATIC ${GIFLIB_SRCS})

target_include_directories(giflib PUBLIC include)

set_target_properties(giflib PROPERTIES
  C_STANDARD 99
  POSITION_INDEPENDENT_CODE ON
)

if(has_reallocarray)
  target_compile_definitions(giflib PRIVATE HAVE_REALLOCARRAY)
endif()
