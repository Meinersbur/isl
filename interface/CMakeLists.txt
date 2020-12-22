
add_executable(extract_interface extract_interface.cc generator.cc python.cc cpp.cc cpp_conversion.cc trace.cc)
target_include_directories(extract_interface PRIVATE "${ISL_SOURCE_DIR}" "${ISL_BINARY_DIR}" ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
if (MSVC)
else ()
  target_compile_options(extract_interface PUBLIC -fno-rtti)
endif ()
target_compile_definitions(extract_interface PRIVATE 
  ADDPATH_TAKES_4_ARGUMENTS
  "CLANG_PREFIX=\"${CLANG_INSTALL_PREFIX}\"" 
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
  if (Python_FOUND)
    add_custom_command(
      OUTPUT "${OUTFILE}"
      COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/redirect_to_file.py" "${OUTFILE}" $<TARGET_FILE:extract_interface> --language=${GENERATOR} "-I${ISL_BINARY_DIR}/include" "-I${ISL_SOURCE_DIR}/include" "${ISL_SOURCE_DIR}/all.h"
      DEPENDS extract_interface
      IMPLICIT_DEPENDS C "${ISL_SOURCE_DIR}/all.h"
    )
  else ()
    add_custom_command(
      OUTPUT "${OUTFILE}"
      COMMAND extract_interface --language=${GENERATOR} "-I${ISL_BINARY_DIR}/include" "-I${ISL_SOURCE_DIR}/include" "${ISL_SOURCE_DIR}/all.h" > "${OUTFILE}"
      DEPENDS extract_interface
      IMPLICIT_DEPENDS C "${ISL_SOURCE_DIR}/all.h"
    )
  endif ()
  add_custom_target(${TARGET} ALL
    DEPENDS "${OUTFILE}"
  )
endfunction()





gen_interface(interface-python python python.h)
gen_interface(interface-cpp cpp cpp.h)
gen_interface(interface-cpp-checked cpp-checked cpp-checked.h)
gen_interface(interface-cpp-checked-conversion cpp-checked-conversion cpp-checked-conversion.h)
gen_interface(interface-trace trace libtrace.inc.cpp)


add_library(trace SHARED libtrace.cpp)
set_property(TARGET trace PROPERTY CXX_STANDARD 17)
add_dependencies(trace interface-trace)
if (MSVC)
  target_compile_options(trace PUBLIC /bigobj)
else ()
  target_compile_options(trace PUBLIC -Wno-deprecated-declarations)
endif ()
target_include_directories(trace PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${ISL_SOURCE_DIR}/include" "${ISL_BINARY_DIR}/include")









