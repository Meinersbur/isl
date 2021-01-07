list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
cmake_minimum_required(VERSION 3.12)

project("isl" C CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/isl/isl_ctx.c")
  set(ISL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/isl")
elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/isl_ctx.c")
  set(ISL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
else ()
  message(FATAL_ERROR "Cannot find ISL source dir")
endif ()
set(ISL_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")

#find_package(LLVM REQUIRED NO_DEFAULT_PATH)
find_package(Clang QUIET)

set(IMATH_SOURCE_DIR "${ISL_SOURCE_DIR}/imath")
set(IMATH_BINARY_DIR "${ISL_BINARY_DIR}/imath")

## Configure options
set(ISL_INT "imath" CACHE STRING "Which package to use to represent multi-precision integers (gmp|imath)")
option(ISL_SMALL_INT_OPT "Use small integer optimization" ON)


# Process configure options
if (NOT ISL_INT MATCHES "^imath|gmp$")
  message(FATAL_ERROR "Unsupported option for ISL_INT: ${ISL_INT}")
endif ()

set(USE_GMP_FOR_MP OFF)
if (ISL_INT STREQUAL "gmp")
  set(USE_GMP_FOR_MP ON)
endif ()

set(USE_IMATH_FOR_MP OFF)
if (ISL_INT STREQUAL "imath")
  set(USE_IMATH_FOR_MP ON)
endif ()

if (ISL_SMALL_INT_OPT AND (NOT USE_GMP_FOR_MP))
  set(USE_SMALL_INT_OPT ON)
else ()
  set(USE_SMALL_INT_OPT OFF)
endif ()


## Search for libraries
if (USE_GMP_FOR_MP)
  find_package(Gmp REQUIRED)
endif ()
find_package(PipLib QUIET)


## Platform introspection
# Determine version of isl
if (EXISTS "${ISL_SOURCE_DIR}/GIT_HEAD_ID")
  # The source comes from a 'make dist' archive
  file(READ "${ISL_SOURCE_DIR}/GIT_HEAD_ID" ISL_GIT_HEAD_ID)
  string(STRIP "${ISL_GIT_HEAD_ID}" ISL_GIT_HEAD_ID)
elseif (EXISTS "${ISL_SOURCE_DIR}/gitversion.h")
  # The source directory is preconfigured
  file(READ "${ISL_SOURCE_DIR}/gitversion.h" GITVERSION_H)
  string(REGEX REPLACE ".*\\\"([^\\\"]*)\\\".*" "\\1" ISL_GIT_HEAD_ID "${GITVERSION_H}")
elseif (EXISTS "${ISL_SOURCE_DIR}/.git" AND "${ISL_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  # This is the GIT repository itself
  # GetGitRevisionDescription uses CMAKE_CURRENT_SOURCE_DIR to look for the .git directory, so it must refer to ISL's source directory
  include(GetGitRevisionDescription)
  git_describe(ISL_GIT_HEAD_ID)
elseif ()
  # Unknown revision
  # TODO: We could look for a .git and get the revision from HEAD
  set(ISL_GIT_HEAD_ID "UNKNOWN")
endif ()

message(STATUS "ISL version: ${GIT_HEAD_ID}")

# Determine compiler characteristics
if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_COMPILER_IS_CLANGCC ON)
endif()

include(CheckCSourceCompiles)

# Like check_c_source_compiles, but sets the result to either
# 0 (error while compiling) or 1 (compiled successfully)
# Required for compatibility with autotool's AC_CHECK_DECLS
function (check_c_source_compiles_numeric _prog _var)
  check_c_source_compiles("${_prog}" "${_var}")
  if ("${${_var}}")
    set("${_var}" 1 PARENT_SCOPE)
  else ()
    set("${_var}" 0 PARENT_SCOPE)
  endif ()
endfunction ()

# Check for the existance of a type
function (check_c_type_exists _type _files _variable)
  set(_includes "")
  foreach (file_name ${_files})
     set(_includes "${_includes}#include<${file_name}>\n")
  endforeach()
  check_c_source_compiles("
    ${_includes}
    ${_type} typeVar;
    int main() {
        return 0;
    }
    " ${_variable})
endfunction ()


check_c_source_compiles("
  int func(void) __attribute__((__warn_unused_result__));
  int main() { return 0; }
  " HAS_ATTRIBUTE_WARN_UNUSED_RESULT)
set(GCC_WARN_UNUSED_RESULT)
if (HAS_ATTRIBUTE_WARN_UNUSED_RESULT)
  set(GCC_WARN_UNUSED_RESULT "__attribute__((__warn_unused_result__))")
endif ()

check_c_source_compiles("
  static void foo(void) __attribute__ ((unused));
  int main() { return 0; }
  " HAVE___ATTRIBUTE__)


check_c_source_compiles_numeric("
  #include <strings.h>
  int main() { (void)ffs(0); return 0; }
  " HAVE_DECL_FFS)

check_c_source_compiles_numeric("
  int main() { __builtin_ffs(0); return 0; }
  " HAVE_DECL___BUILTIN_FFS)

check_c_source_compiles_numeric("
  #include <intrin.h>
  int main() { _BitScanForward(NULL, 0); return 0; }
  " HAVE_DECL__BITSCANFORWARD)

if (NOT HAVE_DECL_FFS AND
    NOT HAVE_DECL___BUILTIN_FFS AND
    NOT HAVE_DECL__BITSCANFORWARD)
  message(FATAL_ERROR "No ffs implementation found")
endif ()


check_c_source_compiles_numeric("
  #include <strings.h>
  int main() { (void)strcasecmp(\"\", \"\"); return 0; }
  " HAVE_DECL_STRCASECMP)

check_c_source_compiles_numeric("
  #include <string.h>
  int main() { _stricmp(\"\", \"\"); return 0; }
  " HAVE_DECL__STRICMP)

if (NOT HAVE_DECL_STRCASECMP AND NOT HAVE_DECL__STRICMP)
  message(FATAL_ERROR "No strcasecmp implementation found")
endif ()


check_c_source_compiles_numeric("
  #include <strings.h>
  int main() { (void)strncasecmp(\"\", \"\", 0); return 0; }
  " HAVE_DECL_STRNCASECMP)

check_c_source_compiles_numeric("
  #include <string.h>
  int main() { _strnicmp(\"\", \"\", 0); return 0; }
  " HAVE_DECL__STRNICMP)

if (NOT HAVE_DECL_STRNCASECMP AND NOT HAVE_DECL__STRNICMP)
  message(FATAL_ERROR "No strncasecmp implementation found")
endif ()


check_c_source_compiles_numeric("
  #include <stdio.h>
  int main() { snprintf((void*)0, 0, \"\"); return 0; }
  " HAVE_DECL_SNPRINTF)

check_c_source_compiles_numeric("
  #include <stdio.h>
  int main() { _snprintf((void*)0, 0, \"\"); return 0; }
  " HAVE_DECL__SNPRINTF)

if (NOT HAVE_DECL_SNPRINTF AND NOT HAVE_DECL__SNPRINTF)
  message(FATAL_ERROR "No snprintf implementation found")
endif ()


check_c_type_exists(uint8_t "" HAVE_UINT8T)
check_c_type_exists(uint8_t "stdint.h" HAVE_STDINT_H)
check_c_type_exists(uint8_t "inttypes.h" HAVE_INTTYPES_H)
check_c_type_exists(uint8_t "sys/types.h" HAVE_SYS_INTTYPES_H)
if (HAVE_UINT8T)
  set(INCLUDE_STDINT_H "")
elseif (HAVE_STDINT_H)
  set(INCLUDE_STDINT_H "#include <stdint.h>")
elseif (HAVE_INTTYPES_H)
  set(INCLUDE_STDINT_H "#include <inttypes.h>")
elseif (HAVE_SYS_INTTYPES_H)
  set(INCLUDE_STDINT_H "#include <sys/inttypes.h>")
else ()
  message(FATAL_ERROR "No stdint.h or compatible found")
endif ()

# Write configure result
# configure_file(... COPYONLY) avoids that the time stamp changes if the file is identical
file(WRITE "${ISL_BINARY_DIR}/gitversion.h.tmp"
     "#define GIT_HEAD_ID \"${ISL_GIT_HEAD_ID}\"")
configure_file("${ISL_BINARY_DIR}/gitversion.h.tmp"
               "${ISL_BINARY_DIR}/gitversion.h" COPYONLY)

file(WRITE "${ISL_BINARY_DIR}/include/isl/stdint.h.tmp"
     "${INCLUDE_STDINT_H}\n")
configure_file("${ISL_BINARY_DIR}/include/isl/stdint.h.tmp"
               "${ISL_BINARY_DIR}/include/isl/stdint.h" COPYONLY)

configure_file("isl_config.h.cmake" "${ISL_BINARY_DIR}/isl_config.h")
configure_file("isl_srcdir.c.cmake" "${ISL_BINARY_DIR}/isl_srcdir.c")

set(EXEEXT "${CMAKE_EXECUTABLE_SUFFIX}")
set(srcdir "${ISL_SOURCE_DIR}")
configure_file("codegen_test.sh.in" "${ISL_BINARY_DIR}/codegen_test.sh" @ONLY)

# ISL files to compile
set (libisl_common_sources
	isl_aff.c
	isl_aff_map.c
	isl_affine_hull.c
	isl_arg.c
	isl_ast.c
	isl_ast_build.c
	isl_ast_build_expr.c
	isl_ast_codegen.c
	isl_ast_graft.c
	basis_reduction_tab.c
	isl_bernstein.c
	isl_blk.c
	isl_bound.c
	isl_box.c
	isl_coalesce.c
	isl_constraint.c
	isl_convex_hull.c
	isl_ctx.c
	isl_deprecated.c
	isl_dim_map.c
	isl_equalities.c
	isl_factorization.c
	isl_farkas.c
	isl_ffs.c
	isl_flow.c
	isl_fold.c
	isl_hash.c
	isl_id_to_ast_expr.c
	isl_id_to_id.c
	isl_id_to_pw_aff.c
	isl_ilp.c
	isl_input.c
	isl_local.c
	isl_local_space.c
	isl_lp.c
	isl_map.c
	isl_map_list.c
	isl_map_simplify.c
	isl_map_subtract.c
	isl_map_to_basic_set.c
	isl_mat.c
	isl_morph.c
	isl_id.c
	isl_obj.c
	isl_options.c
	isl_output.c
	isl_point.c
	isl_polynomial.c
	isl_printer.c
	print.c
	isl_range.c
	isl_reordering.c
	isl_sample.c
	isl_scan.c
	isl_schedule.c
	isl_schedule_band.c
	isl_schedule_node.c
	isl_schedule_read.c
	isl_schedule_tree.c
	isl_schedule_constraints.c
	isl_scheduler.c
	isl_set_list.c
	isl_sort.c
	isl_space.c
	isl_stream.c
	isl_seq.c
	isl_set_to_ast_graft_list.c
	isl_stride.c
	isl_tab.c
	isl_tab_pip.c
	isl_tarjan.c
	isl_transitive_closure.c
	isl_union_map.c
	isl_val.c
	isl_vec.c
	isl_version.c
	isl_vertices.c
)

set(libisl_gmp_sources
    ${ISL_SOURCE_DIR}/isl_gmp.c
    ${ISL_SOURCE_DIR}/isl_val_gmp.c
)

set(libisl_imath_sources
    ${ISL_SOURCE_DIR}/imath/gmp_compat.c
    ${ISL_SOURCE_DIR}/imath/imath.c
    ${ISL_SOURCE_DIR}/imath/imrat.c
    ${ISL_SOURCE_DIR}/isl_imath.c
)

set(libisl_imath32_sources
    ${ISL_SOURCE_DIR}/imath/gmp_compat.c
    ${ISL_SOURCE_DIR}/imath/imath.c
    ${ISL_SOURCE_DIR}/imath/imrat.c
    ${ISL_SOURCE_DIR}/isl_imath.c
    ${ISL_SOURCE_DIR}/isl_int_sioimath.c
    ${ISL_SOURCE_DIR}/isl_val_sioimath.c
)

set(libisl_sources ${libisl_common_sources})
if (USE_GMP_FOR_MP)
  message(STATUS "Integer implementation: GNU MP")
  list(APPEND libisl_sources ${libisl_gmp_sources})
elseif (USE_IMATH_FOR_MP)
  if (USE_SMALL_INT_OPT)
    message(STATUS "Integer implementation: imath with sio32")
    list(APPEND libisl_sources ${libisl_imath32_sources})
  else ()
    message(STATUS "Integer implementation: imath")
    list(APPEND libisl_sources ${libisl_imath_sources})
  endif ()
endif ()


add_library(isl ${libisl_sources})
set_target_properties(isl PROPERTIES FOLDER "Integer Set Library")

target_include_directories(isl PRIVATE "${ISL_BINARY_DIR}" "${ISL_SOURCE_DIR}" ${PIPLIB_INCLUDE_DIRS})
target_include_directories(isl PUBLIC "${ISL_BINARY_DIR}/include" "${ISL_SOURCE_DIR}/include")
if (USE_GMP_FOR_MP)
  target_include_directories(isl PUBLIC ${GMP_INCLUDE_DIRS})
  target_link_libraries(isl PUBLIC ${GMP_LIBRARIES})
endif ()
if (USE_IMATH_FOR_MP)
  target_include_directories(isl PUBLIC "${ISL_SOURCE_DIR}/imath")
endif ()

# General compiler options
if (CMAKE_COMPILER_IS_CLANGCC OR CMAKE_COMPILER_IS_GNUCC)
  target_compile_options(isl PRIVATE -Wall)
endif ()


# Auxiliary executables
macro (add_isl_executable _name)
  set(_sources ${ARGN})
  add_executable("${_name}" ${_sources})
  set_target_properties("${_name}" PROPERTIES FOLDER "Integer Set Library")
  target_include_directories("${_name}" PRIVATE "${ISL_SOURCE_DIR}" "${ISL_BINARY_DIR}" ${GMP_INCLUDE_DIRS} ${PIPLIB_INCLUDE_DIRS})
  target_link_libraries("${_name}" isl)
endmacro ()


add_isl_executable(isl_test "${ISL_SOURCE_DIR}/isl_test.c")
add_isl_executable(isl_test_int "${ISL_SOURCE_DIR}/isl_test_int.c")
if (USE_IMATH_FOR_MP)
  add_isl_executable(isl_test_imath "${ISL_SOURCE_DIR}/isl_test_imath.c")
endif ()
add_isl_executable(isl_polyhedron_sample "${ISL_SOURCE_DIR}/polyhedron_sample.c")
add_isl_executable(isl_pip "${ISL_SOURCE_DIR}/pip.c")
add_isl_executable(isl_polyhedron_minimize "${ISL_SOURCE_DIR}/polyhedron_minimize.c")
add_isl_executable(isl_polytope_scan "${ISL_SOURCE_DIR}/polytope_scan.c")
add_isl_executable(isl_polyhedron_detect_equalities "${ISL_SOURCE_DIR}/polyhedron_detect_equalities.c")
add_isl_executable(isl_cat "${ISL_SOURCE_DIR}/cat.c")
add_isl_executable(isl_closure "${ISL_SOURCE_DIR}/closure.c")
add_isl_executable(isl_bound "${ISL_SOURCE_DIR}/bound.c")
add_isl_executable(isl_codegen "${ISL_SOURCE_DIR}/codegen.c")

if (Clang_FOUND)
  add_subdirectory(interface)
endif ()


# Testing
enable_testing()

add_test(NAME isl_test_int
  COMMAND "$<TARGET_FILE:isl_test_int>"
)
if (USE_IMATH_FOR_MP)
	add_test(NAME isl_test_imath
		COMMAND "$<TARGET_FILE:isl_test_imath>"
	)
endif ()
add_test(NAME isl_test
  COMMAND "$<TARGET_FILE:isl_test>"
)
add_test(NAME codegen_test
  COMMAND "${ISL_BINARY_DIR}/codegen_test.sh"
)

include(GNUInstallDirs)
install(TARGETS isl
  EXPORT islTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/isl
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)