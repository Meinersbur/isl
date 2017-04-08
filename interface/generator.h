#ifndef ISL_INTERFACE_GENERATOR_H
#define ISL_INTERFACE_GENERATOR_H

#include <map>
#include <set>
#include <string>

#include <clang/AST/Decl.h>

using namespace std;
using namespace clang;

void die(const char *msg) __attribute__((noreturn));
string drop_type_suffix(string name, FunctionDecl *method);
vector<string> find_superclasses(RecordDecl *decl);
bool is_overload(Decl *decl);
bool is_constructor(Decl *decl);
bool takes(Decl *decl);
bool gives(Decl *decl);
bool is_isl_ctx(QualType type);
bool first_arg_is_isl_ctx(FunctionDecl *fd);
bool is_isl_type(QualType type);
bool is_isl_bool(QualType type);
bool is_callback(QualType type);
bool is_string(QualType type);
string extract_type(QualType type);

#endif /* ISL_INTERFACE_GENERATOR_H */
