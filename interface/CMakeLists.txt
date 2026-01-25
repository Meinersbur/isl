
add_executable(extract_interface extract_interface.cc generator.cc python.cc cpp.cc cpp_conversion.cc plain_cpp.cc template_cpp.cc)

configure_file("../isl_config.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/include/isl-interface/config.h")

target_include_directories(extract_interface PRIVATE "${ISL_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/include" "${ISL_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/include" ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
if (MSVC)
else ()
  target_compile_options(extract_interface PUBLIC -fno-rtti)
endif ()
target_compile_definitions(extract_interface PRIVATE
  ADDPATH_TAKES_4_ARGUMENTS
  "ISL_CLANG_PREFIX=\"${CLANG_INSTALL_PREFIX}\""
  CREATEPREPROCESSOR_TAKES_TUKIND
  CREATETARGETINFO_TAKES_SHARED_PTR
  CREATE_FROM_ARGS_TAKES_ARRAYREF
  HAVE_BASIC_DIAGNOSTICOPTIONS_H
  HAVE_DLFCN_H=1
  HAVE_INTTYPES_H=1
  HAVE_LEX_PREPROCESSOROPTIONS_H
  HAVE_LLVM_OPTION_ARG_H
  HAVE_MEMORY_H=1
  HAVE_SETMAINFILEID
  HAVE_STDINT_H=1
  HAVE_STDLIB_H=1
  HAVE_STRINGS_H=1
  HAVE_STRING_H=1
  HAVE_SYS_STAT_H=1
  HAVE_SYS_TYPES_H=1
  HAVE_UNISTD_H=1
  HAVE_TARGETPARSER_HOST_H=1
  SETLANGDEFAULTS=LangOptions
  HandleTopLevelDeclContinue=true
  HandleTopLevelDeclReturn=bool
  "IK_C=Language::C"
  SETINVOCATION_TAKES_SHARED_PTR
  SETLANGDEFAULTS_TAKES_5_ARGUMENTS
  STDC_HEADERS=1
  USE_ARRAYREF
  getArgType=getParamType
  getNumArgs=getNumParams
)
target_link_libraries(extract_interface PRIVATE clangBasic clangAnalysis clangAST clangLex clangEdit clangParse clangSema clangFrontend clangSerialization)



function(gen_interface TARGET GENERATOR OUTFILE)
  cmake_parse_arguments(ARG "" "" "PREPEND;APPEND" ${ARGN})
  set(_outpath "${ISL_BINARY_DIR}/include/isl/${OUTFILE}")

  set(_more)
  foreach (_prepend IN LISTS  ARG_PREPEND)
    list(APPEND _more --prepend ${_prepend})
  endforeach ()
  foreach (_append IN LISTS  ARG_APPEND)
    list(APPEND _more --append ${_append})
  endforeach ()
  add_custom_command(
    OUTPUT "${_outpath}"
    COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/redirect_to_file.py" "${_outpath}" ${_more} $<TARGET_FILE:extract_interface> --language=${GENERATOR} "-I${ISL_BINARY_DIR}/include" "-I${ISL_SOURCE_DIR}/include" "${ISL_SOURCE_DIR}/all.h"
    DEPENDS  redirect_to_file.py
    IMPLICIT_DEPENDS "${ISL_SOURCE_DIR}/all.h"
  )

  add_custom_target(${TARGET}
    DEPENDS "${_outpath}" 
  )
  add_dependencies(${TARGET} extract_interface)
  add_dependencies(interface ${TARGET} )
endfunction()

add_custom_target(interface ALL)
gen_interface(interface-python python python.h)
gen_interface(interface-cpp cpp cpp.h PREPEND "${ISL_SOURCE_DIR}/cpp/cpp.h.top" "${ISL_SOURCE_DIR}/all.h" APPEND "${ISL_SOURCE_DIR}/cpp/cpp.h.bot") 
gen_interface(interface-cpp-checked cpp-checked cpp-checked.h PREPEND "${ISL_SOURCE_DIR}/cpp/cpp-checked.h.top" "${ISL_SOURCE_DIR}/all.h" APPEND "${ISL_SOURCE_DIR}/cpp/cpp-checked.h.bot")
gen_interface(interface-cpp-checked-conversion cpp-checked-conversion cpp-checked-conversion.h PREPEND "${ISL_SOURCE_DIR}/cpp/cpp-checked-conversion.h.top" APPEND "${ISL_SOURCE_DIR}/cpp/cpp-checked-conversion.h.bot")
gen_interface(interface-template-cpp template-cpp typed_cpp.h PREPEND "${ISL_SOURCE_DIR}/cpp/typed_cpp.h.top" APPEND "${ISL_SOURCE_DIR}/cpp/typed_cpp.h.bot")

