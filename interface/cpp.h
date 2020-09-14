#ifndef ISL_INTERFACE_CPP_H
#define ISL_INTERFACE_CPP_H

#include "generator.h"

using namespace std;
using namespace clang;

/* A generated C++ method derived from an isl function.
 *
 * "clazz" is the class to which the method belongs.
 * "fd" is the original isl function.
 * "kind" is the type of the method.
 */
struct Method {
	enum Kind {
		static_method,
		member_method,
		constructor,
	};

	Method(const isl_class &clazz, FunctionDecl *fd);

	bool is_subclass_mutator() const;

	const isl_class &clazz;
	FunctionDecl *const fd;
	const enum Kind kind;
};

/* Generator for C++ bindings.
 *
 * "checked" is set if C++ bindings should be generated
 * that rely on the user to check for error conditions.
 */
class cpp_generator : public generator {
	struct class_printer;
	struct decl_printer;
	struct impl_printer;
protected:
	bool checked;
public:
	cpp_generator(SourceManager &SM, set<RecordDecl *> &exported_types,
		set<FunctionDecl *> exported_functions,
		set<FunctionDecl *> functions,
		bool checked = false) :
		generator(SM, exported_types, exported_functions, functions),
		checked(checked) {}

	virtual void generate();
private:
	void print_forward_declarations(ostream &os);
	void print_declarations(ostream &os);
	void print_class(ostream &os, const isl_class &clazz);
	void print_class_forward_decl(ostream &os, const isl_class &clazz);
	void print_implementations(ostream &os);
	void print_class_impl(ostream &os, const isl_class &clazz);
	void print_check_no_persistent_callback(ostream &os,
		const isl_class &clazz, FunctionDecl *fd);
	void print_invalid(ostream &os, int indent, const char *msg,
		const char *checked_code);
	void print_method_param_use(ostream &os, ParmVarDecl *param,
		bool load_from_this_ptr);
	std::string get_return_type(const Method &method);
	string generate_callback_args(QualType type, bool cpp);
	string generate_callback_type(QualType type);
	std::string rename_method(std::string name);
	string isl_bool2cpp();
	string isl_namespace();
	string param2cpp(QualType type);
	bool is_implicit_conversion(const Method &cons);
	bool is_subclass(QualType subclass_type, const isl_class &class_type);
public:
	static string type2cpp(const isl_class &clazz);
	static string type2cpp(string type_string);
};

/* A helper class for printing method declarations and definitions
 * of a class.
 *
 * "os" is the stream onto which the methods are printed.
 * "clazz" describes the methods of the class.
 * "cppstring" is the C++ name of the class.
 * "generator" is the C++ interface generator printing the classes.
 * "declarations" is set if this object is used to print declarations.
 */
struct cpp_generator::class_printer {
	std::ostream &os;
	const isl_class &clazz;
	const std::string cppstring;
	cpp_generator &generator;
	const bool declarations;

	class_printer(std::ostream &os, const isl_class &clazz,
			cpp_generator &generator, bool declarations);

	void print_constructors();
	void print_persistent_callback_prototype(FunctionDecl *method);
	void print_persistent_callback_setter_prototype(FunctionDecl *method);
	void print_methods();
	bool next_variant(FunctionDecl *fd, std::vector<bool> &convert);
	void print_method_variants(FunctionDecl *fd);
	void print_method_group(const function_set &methods);
	virtual void print_method(const Method &method) = 0;
	virtual void print_method(const Method &method,
		const std::vector<bool> &convert) = 0;
	virtual void print_get_method(FunctionDecl *fd) = 0;
	virtual void print_set_enum(const Method &method,
		const string &enum_name, const string &method_name) = 0;
	void print_set_enums(FunctionDecl *fd);
	void print_set_enums();
	ParmVarDecl *get_param(FunctionDecl *fd, int pos,
		const std::vector<bool> &convert);
	void print_method_header(const Method &method, const string &cname,
		int num_params,
		const std::vector<bool> &convert = {});
	void print_named_method_header(const Method &method, string name,
		const std::vector<bool> &convert = {});
	void print_method_header(const Method &method);
	void print_callback_data_decl(ParmVarDecl *param, const string &name);
};

/* A helper class for printing method declarations of a class.
 */
struct cpp_generator::decl_printer : public cpp_generator::class_printer {
	decl_printer(std::ostream &os, const isl_class &clazz,
			cpp_generator &generator) :
		class_printer(os, clazz, generator, true) {}

	void print_subclass_type();
	void print_class_factory(const std::string &prefix = std::string());
	void print_protected_constructors();
	void print_copy_assignment();
	void print_public_constructors();
	void print_destructor();
	void print_ptr();
	void print_isa_type_template(int indent, const isl_class &super);
	void print_downcast();
	void print_ctx();
	void print_persistent_callback_data(FunctionDecl *method);
	void print_persistent_callbacks();
	void print_named_method(const Method &method, const string &name,
		const std::vector<bool> &convert = {});
	virtual void print_method(const Method &method) override;
	virtual void print_method(const Method &method,
		const std::vector<bool> &convert) override;
	virtual void print_get_method(FunctionDecl *fd) override;
	virtual void print_set_enum(const Method &method,
		const string &enum_name, const string &method_name) override;
};

/* A helper class for printing method definitions of a class.
 */
struct cpp_generator::impl_printer : public cpp_generator::class_printer {
	impl_printer(std::ostream &os, const isl_class &clazz,
			cpp_generator &generator) :
		class_printer(os, clazz, generator, false) {}

	virtual void print_method(const Method &method) override;
	virtual void print_method(const Method &method,
		const std::vector<bool> &convert) override;
	virtual void print_get_method(FunctionDecl *fd) override;
	virtual void print_set_enum(const Method &method,
		const string &enum_name, const string &method_name) override;
	void print_check_ptr(const char *ptr);
	void print_check_ptr_start(const char *ptr);
	void print_check_ptr_end(const char *ptr);
	void print_class_factory();
	void print_protected_constructors();
	void print_public_constructors();
	void print_copy_assignment();
	void print_destructor();
	void print_ptr();
	void print_downcast();
	void print_ctx();
	void print_set_persistent_callback(const Method &method);
	void print_persistent_callbacks();
	void print_argument_validity_check(const Method &method);
	void print_save_ctx(const Method &method);
	void print_on_error_continue();
	void print_exceptional_execution_check(const Method &method);
	void print_method_return(const Method &method);
	void print_stream_insertion();
	void print_wrapped_call_checked(int indent, const std::string &call);
	void print_wrapped_call(int indent, const std::string &call,
		QualType rtype);
	void print_callback_body(int indent, ParmVarDecl *param,
		const string &name);
	void print_callback_local(ParmVarDecl *param);
};

#endif
