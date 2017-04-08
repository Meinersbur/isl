#include "generator.h"

using namespace std;
using namespace clang;

class cpp_generator : public generator {
public:
	cpp_generator(set<RecordDecl *> &exported_types,
		set<FunctionDecl *> functions) :
		generator(exported_types, {}, functions) {}

	virtual void generate();
private:
	void print_forward_declarations(ostream &os);
	void print_declarations(ostream &os);
	void print_class(ostream &os, const isl_class &clazz);
	void print_class_forward_decl(ostream &os, const isl_class &clazz);
	void print_class_factory_decl(ostream &os, const isl_class &clazz);
	void print_private_constructors_decl(ostream &os,
		const isl_class &clazz);
	void print_copy_assignment_decl(ostream &os, const isl_class &clazz);
	void print_public_constructors_decl(ostream &os,
		const isl_class &clazz);
	void print_destructor_decl(ostream &os, const isl_class &clazz);
	void print_ptr_decl(ostream &os, const isl_class &clazz);
	void print_implementations(ostream &os);
	void print_class_impl(ostream &os, const isl_class &clazz);
	void print_class_factory_impl(ostream &os, const isl_class &clazz);
	void print_private_constructors_impl(ostream &os,
		const isl_class &clazz);
	void print_public_constructors_impl(ostream &os,
		const isl_class &clazz);
	void print_copy_assignment_impl(ostream &os, const isl_class &clazz);
	void print_destructor_impl(ostream &os, const isl_class &clazz);
	void print_ptr_impl(ostream &os, const isl_class &clazz);
	string type2cpp(const isl_class &clazz);
	string type2cpp(string type_string);
};
