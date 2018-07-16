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

#include <assert.h>
#include <iostream>
#include <stdlib.h>
#ifdef HAVE_ADT_OWNINGPTR_H
#include <llvm/ADT/OwningPtr.h>
#else
#include <memory>
#endif
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/ManagedStatic.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTConsumer.h>
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
#ifdef HAVE_BASIC_DIAGNOSTICOPTIONS_H
#include <clang/Basic/DiagnosticOptions.h>
#else
#include <clang/Frontend/DiagnosticOptions.h>
#endif
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/Utils.h>
#include <clang/Lex/HeaderSearch.h>
#ifdef HAVE_LEX_PREPROCESSOROPTIONS_H
#include <clang/Lex/PreprocessorOptions.h>
#else
#include <clang/Frontend/PreprocessorOptions.h>
#endif
#include <clang/Lex/Preprocessor.h>
#include <clang/Parse/ParseAST.h>
#include <clang/Sema/Sema.h>

#include "extract_interface.h"
#include "generator.h"
#include "python.h"
#include "cpp.h"
#include "cpp_conversion.h"

using namespace std;
using namespace clang;
using namespace clang::driver;

#ifdef HAVE_ADT_OWNINGPTR_H
#define unique_ptr	llvm::OwningPtr
#endif

static llvm::cl::opt<string> InputFilename(llvm::cl::Positional,
			llvm::cl::Required, llvm::cl::desc("<input file>"));
static llvm::cl::list<string> Includes("I",
			llvm::cl::desc("Header search path"),
			llvm::cl::value_desc("path"), llvm::cl::Prefix);

static llvm::cl::opt<string> Language(llvm::cl::Required,
	llvm::cl::ValueRequired, "language",
	llvm::cl::desc("Bindings to generate"),
	llvm::cl::value_desc("name"));

static const char *ResourceDir =
	CLANG_PREFIX "/lib/clang/" CLANG_VERSION_STRING;

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
	if (isa<FunctionDecl>(decl)) {
	FunctionDecl *FDecl = cast<FunctionDecl>(decl);
	std::string N = FDecl->getName();
	if (	N.find("fold_get_domain") != std::string::npos ||
		N.find("fold_align_params") != std::string::npos)
		return false;

	if (	N.find("is_disjoint") != std::string::npos ||
		N.find("get_domain") != std::string::npos ||
		N.find("band_member_set_coincident") != std::string::npos ||
		N.find("band_member_get_coincident") != std::string::npos ||
		N.find("insert_partial_schedule") != std::string::npos ||
		N.find("isl_schedule_node_band_set_ast_build_options") != std::string::npos ||
		N.find("align_params") != std::string::npos ||
		N.find("gist_domain_params") != std::string::npos ||
		N.find("from_domain_and_range") != std::string::npos)
		return true;

	return (N.find("dump") == std::string::npos &&
		N.find("get_ctx") == std::string::npos &&
		N.find("stride_info") == std::string::npos &&
		N.find("isl_ast_expr_or") == std::string::npos &&
		N.find("foreach_ast_op_type") == std::string::npos &&
		N.find("foreach_descendant_top_down") == std::string::npos &&
		N.find("foreach_schedule_node_top_down") == std::string::npos &&
		N.find("foreach_descendant_top_down") == std::string::npos &&
		N.find("set_dim_name") == std::string::npos &&
		N.find("set_dim_name") == std::string::npos &&
		N.find("_list_") == std::string::npos &&
		N.find("isl_qpolynomial_substitute") == std::string::npos &&
		N.find("from_constraint_matrices") == std::string::npos &&
		N.find("partial_") == std::string::npos &&
		N.find("total_dim") == std::string::npos &&
		N.find("compare_at") == std::string::npos &&
		N.find("map_descendant_bottom_up") == std::string::npos &&
		N.find("isl_space_drop_outputs") == std::string::npos &&
		N.find("isl_space_extend") == std::string::npos &&
		N.find("isl_space_match") == std::string::npos &&
		N.find("isl_space_tuple_match") == std::string::npos &&
		N.find("isl_union_map_compute_flow") == std::string::npos &&
		N.find("isl_val_gcdext") == std::string::npos &&
		N.find("multiplicative_call") == std::string::npos &&
		N.find("pw_qpolynomial_fold") == std::string::npos &&
		N.find("fold") == std::string::npos &&
		N.find("pw_qpolynomial_bound") == std::string::npos &&
		N.find("fold_get") == std::string::npos &&
		N.find("isl_val_get_d") == std::string::npos &&
		N.find("compute_divs") == std::string::npos &&
		N.find("dims_get_sign") == std::string::npos &&
		N.find("basic_set_add") == std::string::npos &&
		N.find("id_alloc") == std::string::npos &&
		N.find("align_divs") == std::string::npos &&
		N.find("_n_div") == std::string::npos &&
		N.find("schedule_set") == std::string::npos &&
		N.find("_n_in") == std::string::npos &&
		N.find("_n_out") == std::string::npos &&
		N.find("_n_param") == std::string::npos &&
		N.find("fix_input_si") == std::string::npos &&
		N.find("map_power") == std::string::npos &&
		N.find("path_lengths") == std::string::npos &&
		N.find("remove_inputs") == std::string::npos &&
		N.find("isl_mat_col_add") == std::string::npos &&
		N.find("isl_mat_extend") == std::string::npos &&
		N.find("isl_mat_identity") == std::string::npos &&
		N.find("isl_mat_left_hermite") == std::string::npos &&
		N.find("read_from_file") == std::string::npos &&
		N.find("every_descendant") == std::string::npos &&
		N.find("remove_map_if") == std::string::npos &&
		N.find("isl_vec_normalize") == std::string::npos &&
		N.find("every_map") == std::string::npos &&
		N.find("transitive_closure") == std::string::npos &&
		N.find("disjoint") == std::string::npos &&
		N.find("lift") == std::string::npos &&
		N.find("isl_val_get_abs_num_chunks") == std::string::npos &&
		N.find("isl_val_int_from_chunks") == std::string::npos &&
		N.find("isl_ast_build_set") == std::string::npos &&
		N.find("isl_id_get_user") == std::string::npos &&
		N.find("copy") == std::string::npos &&
		N.find("try_get") == std::string::npos &&
		N.find("get_parent_type") == std::string::npos &&
		N.find("isl_basic_set_has_defining_inequalities") == std::string::npos &&
		N.find("isl_basic_map_has_defining_equality") == std::string::npos &&
		N.find("isl_constraint_dim") == std::string::npos &&
		N.find("isl_basic_set_has_defining_equality") == std::string::npos &&
		N.find("isl_constraint_is_equal") == std::string::npos &&
		N.find("isl_constraint_negate") == std::string::npos &&
		N.find("isl_constraint_cow") == std::string::npos &&
		N.find("isl_set_fix_dim_si") == std::string::npos &&
		N.find("isl_set_eliminate_dims") == std::string::npos &&
		N.find("isl_set_get_hash") == std::string::npos &&
		N.find("isl_space_drop_inputs") == std::string::npos &&
		N.find("dim_residue_class_val") == std::string::npos &&
		N.find("map_schedule_node_bottom_up") == std::string::npos &&
		N.find("void") == std::string::npos &&
		N.find("to_str") == std::string::npos &&
		N.find("foreach_scc") == std::string::npos &&
		N.find("set_free_user") == std::string::npos &&
		N.find("and") == std::string::npos &&
		N.find("get_op_type") == std::string::npos &&
		N.find("get_type") == std::string::npos &&
		N.find("apply_multi_aff") == std::string::npos &&
		N.find("free") == std::string::npos &&
		N.find("delete") == std::string::npos &&
		N.find("print") == std::string::npos);
	}
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

#ifdef USE_ARRAYREF

#ifdef HAVE_CXXISPRODUCTION
static Driver *construct_driver(const char *binary, DiagnosticsEngine &Diags)
{
	return new Driver(binary, llvm::sys::getDefaultTargetTriple(),
			    "", false, false, Diags);
}
#elif defined(HAVE_ISPRODUCTION)
static Driver *construct_driver(const char *binary, DiagnosticsEngine &Diags)
{
	return new Driver(binary, llvm::sys::getDefaultTargetTriple(),
			    "", false, Diags);
}
#elif defined(DRIVER_CTOR_TAKES_DEFAULTIMAGENAME)
static Driver *construct_driver(const char *binary, DiagnosticsEngine &Diags)
{
	return new Driver(binary, llvm::sys::getDefaultTargetTriple(),
			    "", Diags);
}
#else
static Driver *construct_driver(const char *binary, DiagnosticsEngine &Diags)
{
	return new Driver(binary, llvm::sys::getDefaultTargetTriple(), Diags);
}
#endif

namespace clang { namespace driver { class Job; } }

/* Clang changed its API from 3.5 to 3.6 and once more in 3.7.
 * We fix this with a simple overloaded function here.
 */
struct ClangAPI {
	static Job *command(Job *J) { return J; }
	static Job *command(Job &J) { return &J; }
	static Command *command(Command &C) { return &C; }
};

/* Create a CompilerInvocation object that stores the command line
 * arguments constructed by the driver.
 * The arguments are mainly useful for setting up the system include
 * paths on newer clangs and on some platforms.
 */
static CompilerInvocation *construct_invocation(const char *filename,
	DiagnosticsEngine &Diags)
{
	const char *binary = CLANG_PREFIX"/bin/clang";
	const unique_ptr<Driver> driver(construct_driver(binary, Diags));
	std::vector<const char *> Argv;
	Argv.push_back(binary);
	Argv.push_back(filename);
	const unique_ptr<Compilation> compilation(
		driver->BuildCompilation(llvm::ArrayRef<const char *>(Argv)));
	JobList &Jobs = compilation->getJobs();

	Command *cmd = cast<Command>(ClangAPI::command(*Jobs.begin()));
	if (strcmp(cmd->getCreator().getName(), "clang"))
		return NULL;

	const ArgStringList *args = &cmd->getArguments();

	CompilerInvocation *invocation = new CompilerInvocation;
	CompilerInvocation::CreateFromArgs(*invocation, args->data() + 1,
						args->data() + args->size(),
						Diags);
	return invocation;
}

#else

static CompilerInvocation *construct_invocation(const char *filename,
	DiagnosticsEngine &Diags)
{
	return NULL;
}

#endif

#ifdef HAVE_BASIC_DIAGNOSTICOPTIONS_H

static TextDiagnosticPrinter *construct_printer(void)
{
	return new TextDiagnosticPrinter(llvm::errs(), new DiagnosticOptions());
}

#else

static TextDiagnosticPrinter *construct_printer(void)
{
	DiagnosticOptions DO;
	return new TextDiagnosticPrinter(llvm::errs(), DO);
}

#endif

#ifdef CREATETARGETINFO_TAKES_SHARED_PTR

static TargetInfo *create_target_info(CompilerInstance *Clang,
	DiagnosticsEngine &Diags)
{
	shared_ptr<TargetOptions> TO = Clang->getInvocation().TargetOpts;
	TO->Triple = llvm::sys::getDefaultTargetTriple();
	return TargetInfo::CreateTargetInfo(Diags, TO);
}

#elif defined(CREATETARGETINFO_TAKES_POINTER)

static TargetInfo *create_target_info(CompilerInstance *Clang,
	DiagnosticsEngine &Diags)
{
	TargetOptions &TO = Clang->getTargetOpts();
	TO.Triple = llvm::sys::getDefaultTargetTriple();
	return TargetInfo::CreateTargetInfo(Diags, &TO);
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

#ifdef CREATEDIAGNOSTICS_TAKES_ARG

static void create_diagnostics(CompilerInstance *Clang)
{
	Clang->createDiagnostics(0, NULL, construct_printer());
}

#else

static void create_diagnostics(CompilerInstance *Clang)
{
	Clang->createDiagnostics(construct_printer());
}

#endif

#ifdef CREATEPREPROCESSOR_TAKES_TUKIND

static void create_preprocessor(CompilerInstance *Clang)
{
	Clang->createPreprocessor(TU_Complete);
}

#else

static void create_preprocessor(CompilerInstance *Clang)
{
	Clang->createPreprocessor();
}

#endif

#ifdef ADDPATH_TAKES_4_ARGUMENTS

void add_path(HeaderSearchOptions &HSO, string Path)
{
	HSO.AddPath(Path, frontend::Angled, false, false);
}

#else

void add_path(HeaderSearchOptions &HSO, string Path)
{
	HSO.AddPath(Path, frontend::Angled, true, false, false);
}

#endif

#ifdef HAVE_SETMAINFILEID

static void create_main_file_id(SourceManager &SM, const FileEntry *file)
{
	SM.setMainFileID(SM.createFileID(file, SourceLocation(),
					SrcMgr::C_User));
}

#else

static void create_main_file_id(SourceManager &SM, const FileEntry *file)
{
	SM.createMainFileID(file);
}

#endif

#ifdef SETLANGDEFAULTS_TAKES_5_ARGUMENTS

static void set_lang_defaults(CompilerInstance *Clang)
{
	PreprocessorOptions &PO = Clang->getPreprocessorOpts();
	TargetOptions &TO = Clang->getTargetOpts();
	llvm::Triple T(TO.Triple);
	CompilerInvocation::setLangDefaults(Clang->getLangOpts(), IK_C, T, PO,
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
	Clang->setInvocation(std::make_shared<CompilerInvocation>(*invocation));
}

#else

static void set_invocation(CompilerInstance *Clang,
	CompilerInvocation *invocation)
{
	Clang->setInvocation(invocation);
}

#endif

/* Create an interface generator for the selected language and
 * then use it to generate the interface.
 */
static void generate(MyASTConsumer &consumer, SourceManager &SM)
{
	generator *gen;

	if (Language.compare("python") == 0) {
		gen = new python_generator(SM, consumer.exported_types,
			consumer.exported_functions, consumer.functions);
	} else if (Language.compare("cpp") == 0) {
		gen = new cpp_generator(SM, consumer.exported_types,
			consumer.exported_functions, consumer.functions);
	} else if (Language.compare("cpp-checked") == 0) {
		gen = new cpp_generator(SM, consumer.exported_types,
			consumer.exported_functions, consumer.functions, true);
	} else if (Language.compare("cpp-checked-conversion") == 0) {
		gen = new cpp_conversion_generator(SM, consumer.exported_types,
			consumer.exported_functions, consumer.functions);
	} else {
		cerr << "Language '" << Language << "' not recognized." << endl
		     << "Not generating bindings." << endl;
		exit(EXIT_FAILURE);
	}

	gen->generate();
}

int main(int argc, char *argv[])
{
	llvm::cl::ParseCommandLineOptions(argc, argv);

	CompilerInstance *Clang = new CompilerInstance();
	create_diagnostics(Clang);
	DiagnosticsEngine &Diags = Clang->getDiagnostics();
	Diags.setSuppressSystemWarnings(true);
	CompilerInvocation *invocation =
		construct_invocation(InputFilename.c_str(), Diags);
	if (invocation)
		set_invocation(Clang, invocation);
	Clang->createFileManager();
	Clang->createSourceManager(Clang->getFileManager());
	TargetInfo *target = create_target_info(Clang, Diags);
	Clang->setTarget(target);
	set_lang_defaults(Clang);
	HeaderSearchOptions &HSO = Clang->getHeaderSearchOpts();
	LangOptions &LO = Clang->getLangOpts();
	PreprocessorOptions &PO = Clang->getPreprocessorOpts();
	HSO.ResourceDir = ResourceDir;

	for (llvm::cl::list<string>::size_type i = 0; i < Includes.size(); ++i)
		add_path(HSO, Includes[i]);

	PO.addMacroDef("__isl_give=__attribute__((annotate(\"isl_give\")))");
	PO.addMacroDef("__isl_keep=__attribute__((annotate(\"isl_keep\")))");
	PO.addMacroDef("__isl_take=__attribute__((annotate(\"isl_take\")))");
	PO.addMacroDef("__isl_export=__attribute__((annotate(\"isl_export\")))");
	PO.addMacroDef("__isl_overload="
	    "__attribute__((annotate(\"isl_overload\"))) "
	    "__attribute__((annotate(\"isl_export\")))");
	PO.addMacroDef("__isl_constructor=__attribute__((annotate(\"isl_constructor\"))) __attribute__((annotate(\"isl_export\")))");
	PO.addMacroDef("__isl_subclass(super)=__attribute__((annotate(\"isl_subclass(\" #super \")\"))) __attribute__((annotate(\"isl_export\")))");

	create_preprocessor(Clang);
	Preprocessor &PP = Clang->getPreprocessor();

	PP.getBuiltinInfo().initializeBuiltins(PP.getIdentifierTable(), LO);

	const FileEntry *file = Clang->getFileManager().getFile(InputFilename);
	assert(file);
	create_main_file_id(Clang->getSourceManager(), file);

	Clang->createASTContext();
	MyASTConsumer consumer;
	Sema *sema = new Sema(PP, Clang->getASTContext(), consumer);

	Diags.getClient()->BeginSourceFile(LO, &PP);
	ParseAST(*sema);
	Diags.getClient()->EndSourceFile();

	generate(consumer, Clang->getSourceManager());

	delete sema;
	delete Clang;
	llvm::llvm_shutdown();

	return EXIT_SUCCESS;
}
