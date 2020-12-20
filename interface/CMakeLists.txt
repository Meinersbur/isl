
add_executable(extract_interface extract_interface.cc generator.cc python.cc cpp.cc cpp_conversion.cc)

target_include_directories(extract_interface PRIVATE "${ISL_SOURCE_DIR}" "${ISL_BINARY_DIR}" ${CLANG_INCLUDE_DIRS})
target_compile_options(extract_interface PUBLIC -fno-rtti)
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


add_custom_target(interface-python ALL
  COMMAND extract_interface --language=python "-I${ISL_BINARY_DIR}/include" "-I${ISL_SOURCE_DIR}/include" "${ISL_SOURCE_DIR}/all.h" > python.h
)

add_custom_target(interface-cpp ALL
  COMMAND extract_interface --language=cpp "-I${ISL_BINARY_DIR}/include" "-I${ISL_SOURCE_DIR}/include" "${ISL_SOURCE_DIR}/all.h" > cpp.h
)

add_custom_target(interface-cpp-checked ALL
  COMMAND extract_interface --language=cpp-checked "-I${ISL_BINARY_DIR}/include" "-I${ISL_SOURCE_DIR}/include" "${ISL_SOURCE_DIR}/all.h" > cpp-checked.h
)

add_custom_target(interface-cpp-checked-conversion ALL
  COMMAND extract_interface --language=cpp-checked-conversion "-I${ISL_BINARY_DIR}/include" "-I${ISL_SOURCE_DIR}/include" "${ISL_SOURCE_DIR}/all.h" > cpp-checked-conversion.h
)
