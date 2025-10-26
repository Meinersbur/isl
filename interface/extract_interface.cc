/*
 * Copyright 2011 Sven Verdoolaege. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY SVEN VERDOOLAEGE ''AS IS'' AND ANY
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
 * Sven Verdoolaege.
 */ 

#include "isl_config.h"
#undef PACKAGE

#include <assert.h>
#include <iostream>
#include <stdlib.h>
#include <type_traits>
#include <memory>
#ifdef HAVE_LLVM_OPTION_ARG_H
#include <llvm/Option/Arg.h>
#endif
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/CommandLine.h>
#ifdef HAVE_TARGETPARSER_HOST_H
#include <llvm/TargetParser/Host.h>
#else
#include <llvm/Support/Host.h>
#endif
#include <llvm/Support/ManagedStatic.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/Basic/Builtins.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/FileSystemOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Basic/Version.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/Tool.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/Utils.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Parse/ParseAST.h>
#include <clang/Sema/Sema.h>

#include "extract_interface.h"
#include "generator.h"
#include "python.h"
#include "plain_cpp.h"
#include "cpp_conversion.h"
#include "template_cpp.h"

using namespace std;
using namespace clang;
using namespace clang::driver;
#ifdef HAVE_LLVM_OPTION_ARG_H
using namespace llvm::opt;
#endif

static llvm::cl::opt<string> InputFilename(llvm::cl::Positional,
			llvm::cl::Required, llvm::cl::desc("<input file>"));
static llvm::cl::list<string> Includes("I",
			llvm::cl::desc("Header search path"),
			llvm::cl::value_desc("path"), llvm::cl::Prefix);

static llvm::cl::opt<string> OutputLanguage(llvm::cl::Required,
	llvm::cl::ValueRequired, "language",
	llvm::cl::desc("Bindings to generate"),
	llvm::cl::value_desc("name"));

static const char *ResourceDir =
	ISL_CLANG_PREFIX "/lib/clang/" CLANG_VERSION_STRING;

/* Does decl have an attribute of the following form?
 *
 *	__attribute__((annotate("name")))
 */
bool has_annotation(Decl *decl, const char *name)
{
	if (!decl->hasAttrs())
		return false;

	AttrVec attrs = decl->getAttrs();
	for (AttrVec::const_iterator i = attrs.begin() ; i != attrs.end(); ++i) {
		const AnnotateAttr *ann = dyn_cast<AnnotateAttr>(*i);
		if (!ann)
			continue;
		if (ann->getAnnotation().str() == name)
			return true;
	}

	return false;
}

/* Is decl marked as exported?
 */
static bool is_exported(Decl *decl)
{
	return has_annotation(decl, "isl_export");
}

/* Collect all types and functions that are annotated "isl_export"
 * in "exported_types" and "exported_function".  Collect all function
 * declarations in "functions".
 *
 * We currently only consider single declarations.
 */
struct MyASTConsumer : public ASTConsumer {
	set<RecordDecl *> exported_types;
	set<FunctionDecl *> exported_functions;
	set<FunctionDecl *> functions;

	virtual HandleTopLevelDeclReturn HandleTopLevelDecl(DeclGroupRef D) {
		Decl *decl;

		if (!D.isSingleDecl())
			return HandleTopLevelDeclContinue;
		decl = D.getSingleDecl();
		if (isa<FunctionDecl>(decl))
			functions.insert(cast<FunctionDecl>(decl));
		if (!is_exported(decl))
			return HandleTopLevelDeclContinue;
		switch (decl->getKind()) {
		case Decl::Record:
			exported_types.insert(cast<RecordDecl>(decl));
			break;
		case Decl::Function:
			exported_functions.insert(cast<FunctionDecl>(decl));
			break;
		default:
			break;
		}
		return HandleTopLevelDeclContinue;
	}
};

static Driver *construct_driver(const char *binary, DiagnosticsEngine &Diags)
{
	return new Driver(binary, llvm::sys::getDefaultTargetTriple(), Diags);
}

namespace clang { namespace driver { class Job; } }

/* Clang changed its API from 3.5 to 3.6 and once more in 3.7.
 * We fix this with a simple overloaded function here.
 */
struct ClangAPI {
	static Job *command(Job *J) { return J; }
	static Job *command(Job &J) { return &J; }
	static Command *command(Command &C) { return &C; }
};

#ifdef CREATE_FROM_ARGS_TAKES_ARRAYREF

/* Call CompilerInvocation::CreateFromArgs with the right arguments.
 * In this case, an ArrayRef<const char *>.
 */
static void create_from_args(CompilerInvocation &invocation,
	const ArgStringList *args, DiagnosticsEngine &Diags)
{
	CompilerInvocation::CreateFromArgs(invocation, *args, Diags);
}

#else

/* Call CompilerInvocation::CreateFromArgs with the right arguments.
 * In this case, two "const char *" pointers.
 */
static void create_from_args(CompilerInvocation &invocation,
	const ArgStringList *args, DiagnosticsEngine &Diags)
{
	CompilerInvocation::CreateFromArgs(invocation, args->data() + 1,
						args->data() + args->size(),
						Diags);
}

#endif

#ifdef ISL_CLANG_SYSROOT
/* Set sysroot if required.
 *
 * If ISL_CLANG_SYSROOT is defined, then set it to this value.
 */
static void set_sysroot(ArgStringList &args)
{
	args.push_back("-isysroot");
	args.push_back(ISL_CLANG_SYSROOT);
}
#else
/* Set sysroot if required.
 *
 * If ISL_CLANG_SYSROOT is not defined, then it does not need to be set.
 */
static void set_sysroot(ArgStringList &args)
{
}
#endif

/* Create a CompilerInvocation object that stores the command line
 * arguments constructed by the driver.
 * The arguments are mainly useful for setting up the system include
 * paths on newer clangs and on some platforms.
 */
static CompilerInvocation *construct_invocation(const char *filename,
	DiagnosticsEngine &Diags)
{
	const char *binary = ISL_CLANG_PREFIX"/bin/clang";
	const std::unique_ptr<Driver> driver(construct_driver(binary, Diags));
	std::vector<const char *> Argv;
	Argv.push_back(binary);
	Argv.push_back(filename);
	const std::unique_ptr<Compilation> compilation(
		driver->BuildCompilation(llvm::ArrayRef<const char *>(Argv)));
	JobList &Jobs = compilation->getJobs();

	Command *cmd = cast<Command>(ClangAPI::command(*Jobs.begin()));
	if (strcmp(cmd->getCreator().getName(), "clang"))
		return NULL;

	ArgStringList args = cmd->getArguments();
	set_sysroot(args);

	CompilerInvocation *invocation = new CompilerInvocation;
	create_from_args(*invocation, &args, Diags);
	return invocation;
}

#ifdef CREATETARGETINFO_TAKES_SHARED_PTR

static TargetInfo *create_target_info(CompilerInstance *Clang,
	DiagnosticsEngine &Diags)
{
	std::shared_ptr<TargetOptions> TO = Clang->getInvocation().TargetOpts;
	TO->Triple = llvm::sys::getDefaultTargetTriple();
	return TargetInfo::CreateTargetInfo(Diags, TO);
}

#else

static TargetInfo *create_target_info(CompilerInstance *Clang,
	DiagnosticsEngine &Diags)
{
	TargetOptions &TO = Clang->getTargetOpts();
	TO.Triple = llvm::sys::getDefaultTargetTriple();
	return TargetInfo::CreateTargetInfo(Diags, TO);
}

#endif

static void create_diagnostics(CompilerInstance *Clang)
{
	Clang->createDiagnostics();
}

static void create_preprocessor(CompilerInstance *Clang)
{
	Clang->createPreprocessor(TU_Complete);
}

/* Add "Path" to the header search options.
 *
 * Do not take into account sysroot, i.e., set ignoreSysRoot to true.
 */
static void add_path(HeaderSearchOptions &HSO, std::string Path)
{
	HSO.AddPath(Path, frontend::Angled, false, true);
}

template <typename T>
static void create_main_file_id(SourceManager &SM, const T &file)
{
	SM.setMainFileID(SM.createFileID(file, SourceLocation(),
					SrcMgr::C_User));
}

#ifdef SETLANGDEFAULTS_TAKES_5_ARGUMENTS

#include "set_lang_defaults_arg4.h"

static void set_lang_defaults(CompilerInstance *Clang)
{
	PreprocessorOptions &PO = Clang->getPreprocessorOpts();
	TargetOptions &TO = Clang->getTargetOpts();
	llvm::Triple T(TO.Triple);
	SETLANGDEFAULTS::setLangDefaults(Clang->getLangOpts(), IK_C, T,
					    setLangDefaultsArg4(PO),
					    LangStandard::lang_unspecified);
}

#else

static void set_lang_defaults(CompilerInstance *Clang)
{
	CompilerInvocation::setLangDefaults(Clang->getLangOpts(), IK_C,
					    LangStandard::lang_unspecified);
}

#endif

#ifdef SETINVOCATION_TAKES_SHARED_PTR

static void set_invocation(CompilerInstance *Clang,
	CompilerInvocation *invocation)
{
	Clang->setInvocation(std::shared_ptr<CompilerInvocation>(invocation));
}

#else

static void set_invocation(CompilerInstance *Clang,
	CompilerInvocation *invocation)
{
	Clang->setInvocation(invocation);
}

#endif

/* Helper function for ignore_error that only gets enabled if T
 * (which is either const FileEntry * or llvm::ErrorOr<const FileEntry *>)
 * has getError method, i.e., if it is llvm::ErrorOr<const FileEntry *>.
 */
template <class T>
static const FileEntry *ignore_error_helper(const T obj, int,
	int[1][sizeof(obj.getError())])
{
	return *obj;
}

/* Helper function for ignore_error that is always enabled,
 * but that only gets selected if the variant above is not enabled,
 * i.e., if T is const FileEntry *.
 */
template <class T>
static const FileEntry *ignore_error_helper(const T obj, long, void *)
{
	return obj;
}

/* Given either a const FileEntry * or a llvm::ErrorOr<const FileEntry *>,
 * extract out the const FileEntry *.
 */
template <class T>
static const FileEntry *ignore_error(const T obj)
{
	return ignore_error_helper(obj, 0, NULL);
}

/* A template class with value true if "T" has a getFileRef method.
 */
template <typename T>
struct HasGetFileRef {
private:
    template <typename U>
    static auto test(int) ->
	decltype(std::declval<U>().
	    getFileRef(std::declval<const std::string &>()), std::true_type());

    template <typename>
    static std::false_type test(...);

public:
    using type = decltype(test<T>(0));
    static constexpr bool value = type::value;
};

/* Return the FileEntryRef/FileEntry corresponding to the given file name or
 * an error if anything went wrong.
 *
 * If T (= FileManager) has a getFileRef method, then call that and
 * return a FileEntryRef.
 * Otherwise, call getFile and return a FileEntry (pointer).
 */
template <typename T,
	typename std::enable_if<HasGetFileRef<T>::value, bool>::type = true>
static auto getFile(T& obj, const std::string &filename)
	-> llvm::ErrorOr<decltype(*obj.getFileRef(filename))>
{
	auto file = obj.getFileRef(filename);
	if (!file)
	    return std::error_code();
	return *file;
}
template <typename T,
	typename std::enable_if<!HasGetFileRef<T>::value, bool>::type = true>
static llvm::ErrorOr<const FileEntry *> getFile(T& obj,
	const std::string &filename)
{
	const FileEntry *file = ignore_error(obj.getFile(filename));
	if (!file)
	    return std::error_code();
	return file;
}

/* A helper class handling the invocation of clang on a file and
 * allowing a derived class to perform the actual parsing
 * in an overridden handle() method.
 * Other virtual methods allow different kinds of configuration.
 */
struct Wrap {
	/* Construct a TextDiagnosticPrinter. */
	virtual TextDiagnosticPrinter *construct_printer() = 0;
	/* Suppress any errors, if needed. */
	virtual void suppress_errors(DiagnosticsEngine &Diags) = 0;
	/* Add required search paths to "HSO". */
	virtual void add_paths(HeaderSearchOptions &HSO) = 0;
	/* Add required macro definitions to "PO". */
	virtual void add_macros(PreprocessorOptions &PO) = 0;
	/* Handle an error opening the file. */
	virtual void handle_error() = 0;
	/* Parse the file, returning true if no error was encountered. */
	virtual bool handle(CompilerInstance *Clang) = 0;

	/* Invoke clang on "filename", passing control to the handle() method
	 * for parsing.
	 */
	bool invoke(const char *filename) {
		CompilerInstance *Clang = new CompilerInstance();
		create_diagnostics(Clang);
		DiagnosticsEngine &Diags = Clang->getDiagnostics();
		Diags.setSuppressSystemWarnings(true);
		suppress_errors(Diags);
		TargetInfo *target = create_target_info(Clang, Diags);
		Clang->setTarget(target);
		set_lang_defaults(Clang);
		CompilerInvocation *invocation =
			construct_invocation(filename, Diags);
		if (invocation)
			set_invocation(Clang, invocation);
		Diags.setClient(construct_printer());
		Clang->createFileManager();
		Clang->createSourceManager(Clang->getFileManager());
		HeaderSearchOptions &HSO = Clang->getHeaderSearchOpts();
		LangOptions &LO = Clang->getLangOpts();
		PreprocessorOptions &PO = Clang->getPreprocessorOpts();
		HSO.ResourceDir = ResourceDir;

		add_paths(HSO);
		add_macros(PO);

		create_preprocessor(Clang);
		Preprocessor &PP = Clang->getPreprocessor();

		PP.getBuiltinInfo().initializeBuiltins(PP.getIdentifierTable(),
							LO);
		auto file = getFile(Clang->getFileManager(), filename);
		if (!file) {
			handle_error();
			delete Clang;
			return false;
		}
		create_main_file_id(Clang->getSourceManager(), *file);

		Clang->createASTContext();
		bool ok = handle(Clang);
		delete Clang;

		return ok;
	}
};

/* A class specializing the Wrap helper class for
 * extracting the isl interface.
 */
struct Extractor : public Wrap {
	virtual TextDiagnosticPrinter *construct_printer() override;
	virtual void suppress_errors(DiagnosticsEngine &Diags) override;
	virtual void add_paths(HeaderSearchOptions &HSO) override;
	virtual void add_macros(PreprocessorOptions &PO) override;
	virtual void handle_error() override;
	virtual bool handle(CompilerInstance *Clang) override;
};

/* Construct a TextDiagnosticPrinter.
 */
TextDiagnosticPrinter *Extractor::construct_printer(void)
{
	return new TextDiagnosticPrinter(llvm::errs(), new DiagnosticOptions());
}

/* Suppress any errors, if needed.
 */
void Extractor::suppress_errors(DiagnosticsEngine &Diags)
{
}

/* Add required search paths to "HSO".
 */
void Extractor::add_paths(HeaderSearchOptions &HSO)
{
	for (llvm::cl::list<string>::size_type i = 0; i < Includes.size(); ++i)
		add_path(HSO, Includes[i]);
}

/* Add required macro definitions to "PO".
 */
void Extractor::add_macros(PreprocessorOptions &PO)
{
	PO.addMacroDef("__isl_give=__attribute__((annotate(\"isl_give\")))");
	PO.addMacroDef("__isl_keep=__attribute__((annotate(\"isl_keep\")))");
	PO.addMacroDef("__isl_take=__attribute__((annotate(\"isl_take\")))");
	PO.addMacroDef("__isl_export=__attribute__((annotate(\"isl_export\")))");
	PO.addMacroDef("__isl_overload="
	    "__attribute__((annotate(\"isl_overload\"))) "
	    "__attribute__((annotate(\"isl_export\")))");
	PO.addMacroDef("__isl_constructor=__attribute__((annotate(\"isl_constructor\"))) __attribute__((annotate(\"isl_export\")))");
	PO.addMacroDef("__isl_subclass(super)=__attribute__((annotate(\"isl_subclass(\" #super \")\"))) __attribute__((annotate(\"isl_export\")))");
}

/* Handle an error opening the file.
 */
void Extractor::handle_error()
{
	assert(false);
}

/* Create an interface generator for the selected language and
 * then use it to generate the interface.
 */
static void generate(MyASTConsumer &consumer, SourceManager &SM)
{
	generator *gen;

	if (OutputLanguage.compare("python") == 0) {
		gen = new python_generator(SM, consumer.exported_types,
			consumer.exported_functions, consumer.functions);
	} else if (OutputLanguage.compare("cpp") == 0) {
		gen = new plain_cpp_generator(SM, consumer.exported_types,
			consumer.exported_functions, consumer.functions);
	} else if (OutputLanguage.compare("cpp-checked") == 0) {
		gen = new plain_cpp_generator(SM, consumer.exported_types,
			consumer.exported_functions, consumer.functions, true);
	} else if (OutputLanguage.compare("cpp-checked-conversion") == 0) {
		gen = new cpp_conversion_generator(SM, consumer.exported_types,
			consumer.exported_functions, consumer.functions);
	} else if (OutputLanguage.compare("template-cpp") == 0) {
		gen = new template_cpp_generator(SM, consumer.exported_types,
			consumer.exported_functions, consumer.functions);
	} else {
		cerr << "Language '" << OutputLanguage
		     << "' not recognized." << endl
		     << "Not generating bindings." << endl;
		exit(EXIT_FAILURE);
	}

	gen->generate();
}

/* Parse the current source file, returning true if no error was encountered.
 */
bool Extractor::handle(CompilerInstance *Clang)
{
	Preprocessor &PP = Clang->getPreprocessor();
	MyASTConsumer consumer;
	Sema *sema = new Sema(PP, Clang->getASTContext(), consumer);

	DiagnosticsEngine &Diags = Clang->getDiagnostics();
	Diags.getClient()->BeginSourceFile(Clang->getLangOpts(), &PP);
	ParseAST(*sema);
	Diags.getClient()->EndSourceFile();

	generate(consumer, Clang->getSourceManager());

	delete sema;

	return !Diags.hasErrorOccurred();
}

int main(int argc, char *argv[])
{
	llvm::cl::ParseCommandLineOptions(argc, argv);

	Extractor extractor;
	bool ok = extractor.invoke(InputFilename.c_str());

	llvm::llvm_shutdown();

	if (!ok)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
