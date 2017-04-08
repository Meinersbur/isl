/*
 * Copyright 2016, 2017 Tobias Grosser. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY TOBIAS GROSSER ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SVEN VERDOOLAEGE OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as
 * representing official policies, either expressed or implied, of
 * Tobias Grosser.
 */

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <map>
#include <vector>

#include "cpp.h"
#include "isl_config.h"

/* Print string formatted according to "fmt" to ostream "os".
 *
 * This osprintf method allows us to use printf style formatting constructs when
 * writing to an ostream.
 */
static void osprintf(ostream &os, const char *format, ...)
{
	va_list arguments;
	FILE *string_stream;
	char *string_pointer;
	size_t size;

	va_start(arguments, format);

	string_stream = open_memstream(&string_pointer, &size);

	if (!string_stream) {
		fprintf(stderr, "open_memstream failed -- aborting!\n");
		exit(1);
	}

	vfprintf(string_stream, format, arguments);
	fclose(string_stream);
	os << string_pointer;
	free(string_pointer);
}

/* Generate a cpp interface based on the extracted types and functions.
 *
 * Print first a set of forward declarations for all isl wrapper
 * classes, then the declarations of the classes, and at the end all
 * implementations.
 */
void cpp_generator::generate()
{
	ostream &os = cout;

	osprintf(os, "\n");
	osprintf(os, "namespace isl {\n\n");
	osprintf(os, "inline namespace noexceptions {\n\n");

	print_forward_declarations(os);
	osprintf(os, "\n");
	print_declarations(os);
	osprintf(os, "\n");
	print_implementations(os);

	osprintf(os, "} // namespace noexceptions\n");
	osprintf(os, "} // namespace isl\n\n");
	osprintf(os, "#endif /* ISL_CPP_NOEXCEPTIONS */\n");
}

/* Print forward declarations for all classes to "os".
*/
void cpp_generator::print_forward_declarations(ostream &os)
{
	map<string, isl_class>::iterator ci;

	osprintf(os, "// forward declarations\n");

	for (ci = classes.begin(); ci != classes.end(); ++ci)
		print_class_forward_decl(os, ci->second);
}

/* Print all declarations to "os".
 */
void cpp_generator::print_declarations(ostream &os)
{
	map<string, isl_class>::iterator ci;
	bool first = true;

	for (ci = classes.begin(); ci != classes.end(); ++ci) {
		if (first)
			first = false;
		else
			osprintf(os, "\n");

		print_class(os, ci->second);
	}
}

/* Print all implementations to "os".
 */
void cpp_generator::print_implementations(ostream &os)
{
	map<string, isl_class>::iterator ci;
	bool first = true;

	for (ci = classes.begin(); ci != classes.end(); ++ci) {
		if (first)
			first = false;
		else
			osprintf(os, "\n");

		print_class_impl(os, ci->second);
	}
}

/* Print declarations for class "clazz" to "os".
 */
void cpp_generator::print_class(ostream &os, const isl_class &clazz)
{
	const char *name = clazz.name.c_str();
	std::string cppstring = type2cpp(clazz);
	const char *cppname = cppstring.c_str();

	osprintf(os, "// declarations for isl::%s\n", cppname);

	print_class_factory_decl(os, clazz);
	osprintf(os, "\n");
	osprintf(os, "class %s {\n", cppname);
	osprintf(os, "  friend ");
	print_class_factory_decl(os, clazz);
	osprintf(os, "\n");
	osprintf(os, "  %s *ptr = nullptr;\n", name);
	osprintf(os, "\n");
	print_private_constructors_decl(os, clazz);
	osprintf(os, "\n");
	osprintf(os, "public:\n");
	print_public_constructors_decl(os, clazz);
	print_copy_assignment_decl(os, clazz);
	print_destructor_decl(os, clazz);
	print_ptr_decl(os, clazz);

	osprintf(os, "};\n");
}

/* Print forward declaration of class "clazz" to "os".
 */
void cpp_generator::print_class_forward_decl(ostream &os,
	const isl_class &clazz)
{
	std::string cppstring = type2cpp(clazz);
	const char *cppname = cppstring.c_str();

	osprintf(os, "class %s;\n", cppname);
}

/* Print global factory function to "os".
 *
 * Each class has one global factory function:
 *
 * 	isl::set manage(__isl_take isl_set *ptr);
 *
 * The only public way to construct isl C++ objects from a raw pointer is
 * through this global factory function. This ensures isl object creation
 * is very explicit and pointers are not converted by accident. Due to
 * overloading, manage() can be called on any isl raw pointer and the
 * corresponding object is automatically created, without the user having
 * to choose the right isl object type.
 */
void cpp_generator::print_class_factory_decl(ostream &os,
	const isl_class &clazz)
{
	const char *name = clazz.name.c_str();
	std::string cppstring = type2cpp(clazz);
	const char *cppname = cppstring.c_str();

	osprintf(os, "inline isl::%s manage(__isl_take %s *ptr);\n", cppname,
		 name);
}

/* Print declarations of private constructors for class "clazz" to "os".
 *
 * Each class has currently one private constructor:
 *
 * 	1) Constructor from a plain isl_* C pointer
 *
 * Example:
 *
 * 	set(__isl_take isl_set *ptr);
 *
 * The raw pointer constructor is kept private. Object creation is only
 * possible through isl::manage().
 */
void cpp_generator::print_private_constructors_decl(ostream &os,
	const isl_class &clazz)
{
	const char *name = clazz.name.c_str();
	std::string cppstring = type2cpp(clazz);
	const char *cppname = cppstring.c_str();

	osprintf(os, "  inline explicit %s(__isl_take %s *ptr);\n", cppname,
		 name);
}

/* Print declarations of public constructors for class "clazz" to "os".
 *
 * Each class currently has two public constructors:
 *
 * 	1) A default constructor
 * 	2) A copy constructor
 *
 * Example:
 *
 *	set();
 *	set(const isl::set &set);
 */
void cpp_generator::print_public_constructors_decl(ostream &os,
	const isl_class &clazz)
{
	std::string cppstring = type2cpp(clazz);
	const char *cppname = cppstring.c_str();
	osprintf(os, "  inline /* implicit */ %s();\n", cppname);

	osprintf(os, "  inline /* implicit */ %s(const isl::%s &obj);\n",
		 cppname, cppname);
}

/* Print declarations of copy assignment operator for class "clazz"
 * to "os".
 *
 * Each class has one assignment operator.
 *
 * 	isl:set &set::operator=(isl::set obj)
 *
 */
void cpp_generator::print_copy_assignment_decl(ostream &os,
	const isl_class &clazz)
{
	std::string cppstring = type2cpp(clazz);
	const char *cppname = cppstring.c_str();

	osprintf(os, "  inline isl::%s &operator=(isl::%s obj);\n", cppname,
		 cppname);
}

/* Print declaration of destructor for class "clazz" to "os".
 */
void cpp_generator::print_destructor_decl(ostream &os, const isl_class &clazz)
{
	std::string cppstring = type2cpp(clazz);
	const char *cppname = cppstring.c_str();

	osprintf(os, "  inline ~%s();\n", cppname);
}

/* Print declaration of pointer functions for class "clazz" to "os".
 *
 * To obtain a raw pointer three functions are provided:
 *
 * 	1) __isl_give isl_set *copy()
 *
 * 	  Returns a pointer to a _copy_ of the internal object
 *
 * 	2) __isl_keep isl_set *get()
 *
 * 	  Returns a pointer to the internal object
 *
 * 	3) __isl_give isl_set *release()
 *
 * 	  Returns a pointer to the internal object and resets the
 * 	  internal pointer to nullptr.
 *
 * We also provide functionality to explicitly check if a pointer is
 * currently managed by this object.
 *
 * 	4) bool is_null()
 *
 * 	  Check if the current object is a null pointer.
 *
 * The functions get() and release() model the value_ptr proposed in
 * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3339.pdf.
 * The copy() function is an extension to allow the user to explicitly
 * copy the underlying object.
 *
 * Also generate a declaration to delete copy() for r-values, for
 * r-values release() should be used to avoid unnecessary copies.
 */
void cpp_generator::print_ptr_decl(ostream &os, const isl_class &clazz)
{
	const char *name = clazz.name.c_str();

	osprintf(os, "  inline __isl_give %s *copy() const &;\n", name);
	osprintf(os, "  inline __isl_give %s *copy() && = delete;\n", name);
	osprintf(os, "  inline __isl_keep %s *get() const;\n", name);
	osprintf(os, "  inline __isl_give %s *release();\n", name);
	osprintf(os, "  inline bool is_null() const;\n");
}

/* Print implementations for class "clazz" to "os".
 */
void cpp_generator::print_class_impl(ostream &os, const isl_class &clazz)
{
	std::string cppstring = type2cpp(clazz);
	const char *cppname = cppstring.c_str();

	osprintf(os, "// implementations for isl::%s\n", cppname);

	print_class_factory_impl(os, clazz);
	osprintf(os, "\n");
	print_public_constructors_impl(os, clazz);
	osprintf(os, "\n");
	print_private_constructors_impl(os, clazz);
	osprintf(os, "\n");
	print_copy_assignment_impl(os, clazz);
	osprintf(os, "\n");
	print_destructor_impl(os, clazz);
	osprintf(os, "\n");
	print_ptr_impl(os, clazz);
}

/* Print implementation of global factory function to "os".
 */
void cpp_generator::print_class_factory_impl(ostream &os,
	const isl_class &clazz)
{
	const char *name = clazz.name.c_str();
	std::string cppstring = type2cpp(clazz);
	const char *cppname = cppstring.c_str();

	osprintf(os, "isl::%s manage(__isl_take %s *ptr) {\n", cppname, name);
	osprintf(os, "  return %s(ptr);\n", cppname);
	osprintf(os, "}\n");
}

/* Print implementations of private constructors for class "clazz" to "os".
 */
void cpp_generator::print_private_constructors_impl(ostream &os,
	const isl_class &clazz)
{
	const char *name = clazz.name.c_str();
	std::string cppstring = type2cpp(clazz);
	const char *cppname = cppstring.c_str();

	osprintf(os, "%s::%s(__isl_take %s *ptr)\n    : ptr(ptr) {}\n",
		 cppname, cppname, name);
}

/* Print implementations of public constructors for class "clazz" to "os".
 */
void cpp_generator::print_public_constructors_impl(ostream &os,
	const isl_class &clazz)
{
	const char *name = clazz.name.c_str();
	std::string cppstring = type2cpp(clazz);
	const char *cppname = cppstring.c_str();

	osprintf(os, "%s::%s()\n    : ptr(nullptr) {}\n\n", cppname, cppname);
	osprintf(os, "%s::%s(const isl::%s &obj)\n    : ptr(obj.copy()) {}\n",
		 cppname, cppname, cppname, name);
}

/* Print implementation of copy assignment operator for class "clazz" to "os".
 */
void cpp_generator::print_copy_assignment_impl(ostream &os,
	const isl_class &clazz)
{
	const char *name = clazz.name.c_str();
	std::string cppstring = type2cpp(clazz);
	const char *cppname = cppstring.c_str();

	osprintf(os, "%s &%s::operator=(isl::%s obj) {\n", cppname,
		 cppname, cppname);
	osprintf(os, "  std::swap(this->ptr, obj.ptr);\n", name);
	osprintf(os, "  return *this;\n");
	osprintf(os, "}\n");
}

/* Print implementation of destructor for class "clazz" to "os".
 */
void cpp_generator::print_destructor_impl(ostream &os,
	const isl_class &clazz)
{
	const char *name = clazz.name.c_str();
	std::string cppstring = type2cpp(clazz);
	const char *cppname = cppstring.c_str();

	osprintf(os, "%s::~%s() {\n", cppname, cppname);
	osprintf(os, "  if (ptr)\n");
	osprintf(os, "    %s_free(ptr);\n", name);
	osprintf(os, "}\n");
}

/* Print implementation of ptr() functions for class "clazz" to "os".
 */
void cpp_generator::print_ptr_impl(ostream &os, const isl_class &clazz)
{
	const char *name = clazz.name.c_str();
	std::string cppstring = type2cpp(clazz);
	const char *cppname = cppstring.c_str();

	osprintf(os, "__isl_give %s *%s::copy() const & {\n", name, cppname);
	osprintf(os, "  return %s_copy(ptr);\n", name);
	osprintf(os, "}\n\n");
	osprintf(os, "__isl_keep %s *%s::get() const {\n", name, cppname);
	osprintf(os, "  return ptr;\n");
	osprintf(os, "}\n\n");
	osprintf(os, "__isl_give %s *%s::release() {\n", name, cppname);
	osprintf(os, "  %s *tmp = ptr;\n", name);
	osprintf(os, "  ptr = nullptr;\n");
	osprintf(os, "  return tmp;\n");
	osprintf(os, "}\n\n");
	osprintf(os, "bool %s::is_null() const {\n", cppname);
	osprintf(os, "  return ptr == nullptr;\n");
	osprintf(os, "}\n");
}

/* Translate isl class "clazz" to its corresponding C++ type.
 */
string cpp_generator::type2cpp(const isl_class &clazz)
{
	return type2cpp(clazz.name);
}

/* Translate type string "type_str" to its C++ name counterpart.
*/
string cpp_generator::type2cpp(string type_str)
{
	return type_str.substr(4);
}
