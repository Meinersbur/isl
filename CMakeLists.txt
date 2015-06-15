list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
cmake_minimum_required(VERSION 3.2)

project("isl" C)
set(ISL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ISL_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})


option(ISL_BUILD_OBJECT_LIBRARY "Make object libraries" OFF)
set(ISL_INT "gmp" CACHE STRING "Which package to use to represent multi-precision integers (gmp|imath)")


set(USE_GMP_FOR_MP OFF)
if (ISL_INT STREQUAL "gmp")
  set(USE_GMP_FOR_MP ON)
endif ()

set(USE_IMATH_FOR_MP OFF)
if (ISL_INT STREQUAL "imath")
  set(USE_IMATH_FOR_MP ON)
endif ()


if (USE_GMP_FOR_MP)
  find_package(Gmp REQUIRED)
endif ()
find_package(PipLib QUIET)



include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)

#if (MSVC_IDE)
include(CheckCXXSourceCompiles)
function (CHECK_CXX_TYPE_EXISTS _type _files _variable)
  set(_includes "")
  foreach (file_name ${_files})
     set(_includes "${_includes}#include<${file_name}>\n")
  endforeach()
  CHECK_CXX_SOURCE_COMPILES("
    ${_includes} ${_type} typeVar;
    int main() {
        return 0;
    }
    " ${_variable})
endfunction ()
#else ()
include(CheckCSourceCompiles)
function (CHECK_C_TYPE_EXISTS _type _files _variable)
  set(_includes "")
  foreach (file_name ${_files})
     set(_includes "${_includes}#include<${file_name}>\n")
  endforeach()
  CHECK_C_SOURCE_COMPILES("
    ${_includes} ${_type} typeVar;
    int main() {
        return 0;
    }
    " ${_variable})
endfunction ()
#endif ()

function (CHECK_TYPE_EXISTS _type _files _variable)
  if (MSVC_IDE)
    CHECK_CXX_TYPE_EXISTS(${_type} "${_files}" ${_variable} ${ARGN})
  else ()
    CHECK_C_TYPE_EXISTS(${_type} "${_files}" ${_variable} ${ARGN})
  endif ()
endfunction ()

CHECK_TYPE_EXISTS(uint8_t "" HAVE_UINT8T)
CHECK_TYPE_EXISTS(uint8_t "stdint.h" HAVE_STDINT_H)
CHECK_TYPE_EXISTS(uint8_t "inttypes.h" HAVE_INTTYPES_H)
CHECK_TYPE_EXISTS(uint8_t "sys/types.h" HAVE_SYS_INTTYPES_H)
if (HAVE_UINT8T)
  set(INCLUDE_STDINT_H)
elseif (HAVE_STDINT_H)
  set(INCLUDE_STDINT_H "#include <stdint.h>")
elseif (HAVE_INTTYPES_H)
  set(INCLUDE_STDINT_H "#include <inttypes.h>")
elseif (HAVE_SYS_INTTYPES_H)
  set(INCLUDE_STDINT_H "#include <sys/inttypes.h>")
else ()
  message(FATAL_ERROR "No stdint.h or compatible found")
endif ()
#configure_file("${ISL_SOURCE_DIR}/include/isl/stdint.h.cmake" "${ISL_BINARY_DIR}/include/isl/stdint.h")
#configure_file("${ISL_SOURCE_DIR}/include/isl/config.h.cmake" "${ISL_BINARY_DIR}/include/isl/config.h")

include_directories("${ISL_BINARY_DIR}/include" "${ISL_SOURCE_DIR}" "${ISL_SOURCE_DIR}/include" "${ISL_SOURCE_DIR}/imath" ${GMP_INCLUDE_DIRS} ${PIPLIB_INCLUDE_DIRS})
if (MSVC)
  add_definitions("/FI\"${ISL_SOURCE_DIR}/unicompat.h\"")
endif ()


set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_COMPILER_IS_CLANGCC ON)
endif()

if (CMAKE_COMPILER_IS_CLANGCC OR CMAKE_COMPILER_IS_GNUCC)
   add_definitions(-Wall)
endif ()

# Workaround: cmake removes /usr/include from include_directories(...)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${GMP_INCLUDE_DIR}")

#Workaround
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -fgnu89-inline")


if (USE_IMATH_FOR_MP)
  add_subdirectory(imath)
endif ()



configure_file(${ISL_SOURCE_DIR}/isl_config.h.cmake ${ISL_BINARY_DIR}/include/isl_config.h)
configure_file(${ISL_SOURCE_DIR}/gitversion.h.cmake ${ISL_BINARY_DIR}/include/gitversion.h)



set(libisl_sources
  isl_aff.c
	isl_affine_hull.c
	isl_arg.c
	isl_ast.c
	isl_ast_build.c
	isl_ast_build_expr.c
	isl_ast_codegen.c
	isl_ast_graft.c
	isl_band.c
	basis_reduction_tab.c
	isl_bernstein.c
	isl_blk.c
	isl_bound.c
	isl_coalesce.c
	isl_constraint.c
	isl_convex_hull.c
	isl_ctx.c
	isl_deprecated.c
	isl_dim_map.c
	isl_equalities.c
	isl_factorization.c
	isl_farkas.c
	isl_flow.c
	isl_fold.c
	isl_hash.c
	isl_ilp.c
	isl_input.c
	isl_local_space.c
	isl_lp.c
	isl_map.c
	isl_map_simplify.c
	isl_map_subtract.c
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
	isl_set_list.c
	isl_sort.c
	isl_space.c
	isl_stream.c
	isl_seq.c
	isl_tab.c
	isl_tab.h
	isl_tab_pip.c
	isl_tarjan.c
	isl_transitive_closure.c
	isl_union_map.c
	isl_val.c
	isl_vec.c
	isl_version.c
	isl_vertices.c
	isl_ast_int.c
	isl_id_to_ast_expr.c
	isl_id_to_pw_aff.c
	isl_map_to_basic_set.c
	isl_arg.c
	isl_map_list.c
	isl_schedule_node.c
	isl_schedule_read.c
	isl_schedule_tree.c
	isl_schedule_band.c
	isl_scheduler.c
	isl_gmp.c
	isl_val_gmp.c
)


file(GLOB libisl_headers ${ISL_SOURCE_DIR}/*.h ${ISL_SOURCE_DIR}/include/isl/*.h)
if (MSVC_IDE)
  SET_SOURCE_FILES_PROPERTIES( ${libisl_sources} PROPERTIES LANGUAGE CXX )
endif ()

set(_outheaders)
foreach( _headerpath ${libisl_headers} )
  set( _src ${_headerpath} )
  get_filename_component(_headerfile "${_headerpath}" NAME)
  set( _dst ${ISL_BINARY_DIR}/include/${_headerfile} )
  add_custom_command(OUTPUT ${_dst}
    DEPENDS ${_src}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_src}" "${_dst}"
    COMMENT "Copying ${_headerfile}...")
  list(APPEND _outheaders "${_dst}")
endforeach ()
add_custom_target(isl-headers DEPENDS ${_outheaders})
set_target_properties(isl-headers PROPERTIES FOLDER "Isl")

#if (ISL_BUILD_OBJECT_LIBRARY)
#  add_library(isl OBJECT ${libisl_sources} ${libisl_headers})
#  set(ISL_LIBRARY CACHE INTERNAL "")
#  set(ISL_OBJECTS "$<TARGET_OBJECTS:isl>" CACHE INTERNAL "")
#else ()
#  add_library(isl ${libisl_sources} ${libisl_headers})
#  target_link_libraries(isl ${GMP_LIBRARIES} ${PIPLIB_LIBRARIES})
#  add_dependencies(isl isl-headers)
#  set(ISL_LIBRARY isl CACHE INTERNAL "")
#  set(ISL_OBJECTS CACHE INTERNAL "")
#endif ()

#set(CMAKE_C_FLAGS "-std=c99 ${CMAKE_C_FLAGS}")

 
add_library(isl ${libisl_sources} ${libisl_headers})
target_include_directories(isl PUBLIC "${ISL_SOURCE_DIR}/include")
if (USE_GMP_FOR_MP)
  target_link_libraries(isl PUBLIC ${GMP_LIBRARIES})
endif()
set_target_properties(isl PROPERTIES FOLDER "Isl")


set(isl_libs "${ISL_LIBRARY}" ${PIPLIB_LIBRARIES})
if (USE_GMP_FOR_MP OR USE_LONG_FOR_MP)
  list(APPEND isl_libs ${GMP_LIBRARIES})
endif()
if (USE_IMATH_FOR_MP)
  list(APPEND isl_libs "imath")
endif()
set(ISL_LIBRARIES isl ${isl_libs} CACHE INTERNAL "")
set(ISL_INCLUDE_DIR "${ISL_SOURCE_DIR}/include" CACHE INTERNAL "")
set(ISL_INCLUDE_DIRS "${ISL_INCLUDE_DIR}" "${ISL_BINARY_DIR}/include" ${PIPLIB_INCLUDE_DIRS} ${GMP_INCLUDE_DIRS} CACHE INTERNAL "")
set(ISL_TARGET "isl" CACHE INTERNAL "")
set(ISL_FOUND TRUE CACHE INTERNAL "")
function(add_isl_executable _name)
  set(_sources ${ARGN})
  if (MSVC_IDE)
    SET_SOURCE_FILES_PROPERTIES( ${_sources} PROPERTIES LANGUAGE CXX )
  endif ()
  add_executable(${_name} ${_sources} ${ISL_OBJECTS})
  set_target_properties(${_name} PROPERTIES FOLDER "Isl")
  target_link_libraries(${_name} isl ${ISL_LIBRARIES})
endfunction()

#add_definitions("-DISL_TEST_SRCDIR=\\\"${ISL_SOURCE_DIR}\\\"")
#add_definitions("-DISL_TEST_SRCDIR=\\"${ISL_SOURCE_DIR}\\"")
#add_definitions("-DISL_TEST_SRCDIR='${ISL_SOURCE_DIR}'")



add_isl_executable(isl_test isl_test.c)
#add_isl_executable(isl_inttest isl_inttest.c)
add_isl_executable(isl_polyhedron_sample polyhedron_sample.c)
add_isl_executable(isl_pip pip.c)
add_isl_executable(isl_polyhedron_minimize polyhedron_minimize.c)
add_isl_executable(isl_polytope_scan polytope_scan.c)
add_isl_executable(isl_polyhedron_detect_equalities polyhedron_detect_equalities.c)
add_isl_executable(isl_cat cat.c)
add_isl_executable(isl_closure closure.c)
add_isl_executable(isl_bound bound.c)
add_isl_executable(isl_codegen codegen.c)

# TODO: Should not be here, but handled by LLVM
#set_property(GLOBAL APPEND PROPERTY LLVM_EXPORTS "isl")

add_custom_target(run-isl_test
  COMMAND srcdir=${ISL_SOURCE_DIR} "$<TARGET_FILE:isl_test>"
  COMMENT "Running isl_test"
)
#isl_test codegen_test.sh pip_test.sh bound_test.sh

add_custom_target(run-codegen_test
  COMMAND srcdir=${ISL_SOURCE_DIR} "${ISL_SOURCE_DIR}/codegen_test.sh"
  COMMENT "Running codegen_test"
)
