cmake_minimum_required (VERSION 3.28)

project (src2build)
add_executable (${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE $<IF:$<BOOL:${MSVC}>,c_std_17,c_std_23>)

if (MSVC)
  message(STATUS "MSVC detected")
  target_compile_options(${PROJECT_NAME} PRIVATE /source-charset:utf-8 /execution-charset:shift_jis)
  target_compile_options(${PROJECT_NAME} PRIVATE /J)
else()
  message(STATUS "GCC/Clang detected")
  target_compile_options(${PROJECT_NAME} PRIVATE -funsigned-char -O3 -Wall -Wextra -Werror -pipe)
endif()

target_sources(${PROJECT_NAME} PRIVATE
  src/src2build.c
)
