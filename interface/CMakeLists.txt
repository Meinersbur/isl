add_executable(extract_interface
  extract_interface.cc
  generator.cc
  python.cc
  cpp.cc
  cpp_conversion.cc
  plain_cpp.cc
  template_cpp.cc
)

# I think extract_interface is supposed to have its own header config.h separate from  ${ISL_BINARY_DIR}/isl_config.h, but that seems to have been applied inconsistently
# These include paths are such as mess
configure_file("${ISL_SOURCE_DIR}/isl_config.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/include/isl-interface/config.h")
target_include_directories(extract_interface PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include" #include "isl-interface/clang_wrap.h"
  "${CMAKE_CURRENT_BINARY_DIR}/include" #include <isl-interface/config.h>
  "${ISL_BINARY_DIR}"                   #include "isl_config.h"
  "${ISL_SOURCE_DIR}"                   #include <isl_config_post.h>
  ${CLANG_INCLUDE_DIRS}
)


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


set_property(GLOBAL
  PROPERTY ISL_INTERFACES ""
)

# Note that some header files have the same name:
#
#  * ${CMAKE_CURRENT_SOURCE_DIR}/python.h used by extract_interface and the generated ${ISL_BINARY_DIR}/include/isl/python.h
#  * ${CMAKE_CURRENT_SOURCE_DIR}/cpp.h used by extract_interface and the generated ${ISL_BINARY_DIR}/include/isl/cpp.h
#
# Totally not confusing!
function(gen_interface TARGET GENERATOR OUTFILE)
  cmake_parse_arguments(ARG "" "" "PREPEND;APPEND" ${ARGN})

  cmake_path(SET _outpath "${ISL_BINARY_DIR}/include/isl/")
  cmake_path(APPEND _outpath "${OUTFILE}")

  set(_more)
  foreach (_prepend IN LISTS  ARG_PREPEND)
    list(APPEND _more --prepend ${_prepend})
  endforeach ()
  foreach (_append IN LISTS  ARG_APPEND)
    list(APPEND _more --append ${_append})
  endforeach ()
  add_custom_command(
    OUTPUT "${_outpath}"
    COMMAND "${Python_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/redirect_to_file.py" "${_outpath}" ${_more} "$<TARGET_FILE:extract_interface>" --language=${GENERATOR} "-I${ISL_BINARY_DIR}/include" "-I${ISL_SOURCE_DIR}/include" "${ISL_SOURCE_DIR}/all.h"
    DEPENDS redirect_to_file.py
    IMPLICIT_DEPENDS "${ISL_SOURCE_DIR}/all.h"
  )

  add_custom_target(${TARGET}
    DEPENDS "${_outpath}"
  )
  add_dependencies(${TARGET} extract_interface)
  add_dependencies(interface ${TARGET})

  set_property(GLOBAL
    APPEND PROPERTY ISL_INTERFACES "${GENERATOR}"
  )
endfunction()


add_custom_target(interface ALL)
gen_interface(interface-python                 python                 "python.h")
gen_interface(interface-cpp                    cpp                    "cpp.h"                    PREPEND "${ISL_SOURCE_DIR}/cpp/cpp.h.top"         "${ISL_SOURCE_DIR}/all.h" APPEND "${ISL_SOURCE_DIR}/cpp/cpp.h.bot")
gen_interface(interface-cpp-checked            cpp-checked            "cpp-checked.h"            PREPEND "${ISL_SOURCE_DIR}/cpp/cpp-checked.h.top" "${ISL_SOURCE_DIR}/all.h" APPEND "${ISL_SOURCE_DIR}/cpp/cpp-checked.h.bot")
gen_interface(interface-cpp-checked-conversion cpp-checked-conversion "cpp-checked-conversion.h" PREPEND "${ISL_SOURCE_DIR}/cpp/cpp-checked-conversion.h.top"                APPEND "${ISL_SOURCE_DIR}/cpp/cpp-checked-conversion.h.bot")
gen_interface(interface-template-cpp           template-cpp           "typed_cpp.h"              PREPEND "${ISL_SOURCE_DIR}/cpp/typed_cpp.h.top"                             APPEND "${ISL_SOURCE_DIR}/cpp/typed_cpp.h.bot")

