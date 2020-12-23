/*
 * Copyright 2018      Sven Verdoolaege
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege.
 */

#include <stdio.h>
#include <map>
#include <string>

#include "trace.h"
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/ASTContext.h>
//#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/StringSet.h>





static bool isIslObjTy(QualType ty) {
	if (!ty->isPointerType())
		return false;
	auto pyty =  ty->getPointeeType();
	if (!pyty->isRecordType())
		return false;
	auto structty = pyty->getAsRecordDecl();
	auto SName = structty->getName();
	return SName.startswith("isl_");
}


static const char* getShortname(StringRef longname) {
	static llvm::DenseMap<StringRef, const char*> map{
		{"isl_ctx", "ctx"},
		{"isl_space", "space"},
		{"isl_local_space", "ls"},
		{"isl_map", "map"},
		{"isl_set", "set"},
		{"isl_union_map", "umap"},
		{"isl_union_set", "uset"},
		{"isl_schedule", "sched"},
		{"isl_aff", "aff"},
		{"isl_pw_aff", "pa"},
		{"isl_val", "val"},
		{"isl_constraint", "constr"},
		{"isl_id", "id"},
		{"isl_union_access_info", "accesses"},
		{"isl_union_flow", "flow"},
	};
	auto It = map.find(longname);
	if (It == map.end())
		return "obj";
	return It->second;
}

static const char* getShortname(QualType ty) {
	if (!ty->isPointerType())
		return "val";
	auto pyty =  ty->getPointeeType();
	if (!pyty->isRecordType())
		return "obj";
	auto structty = pyty->getAsRecordDecl();
	auto SName = structty->getName();
	return getShortname(SName);
}



/* Generate conversion functions for converting objects between
 * the default and the checked C++ bindings.
 * Do this for each exported class.
 */
void trace_generator::generate()
{
	std::string InitCode;
	llvm::raw_string_ostream InitOS(InitCode);

	auto &OS = llvm::outs();

#if 0
	OS << "// ISL structs\n";
	llvm::StringSet<> Visited;
	for (auto P : classes) {
		isl_class *C =& P.second;
		auto Name = C->name;
		if (Visited.contains(Name)) continue;

		OS << "ISL_STRUCT(" << Name << ")\n";
		Visited.insert(Name);
	}
	OS << "\n\n";
#endif


	OS << "// ISL functions\n";
	for (auto ci = functions_by_name.begin(); ci != functions_by_name.end(); ++ci) {
		FunctionDecl* FD = ci->second;
		auto FName = FD->getName();
		if (!FName.startswith("isl_"))
			continue;



		PrintingPolicy Policy(FD->getASTContext().getLangOpts());
		auto getTy = [&](QualType ty, const Twine& placeholser = {}) -> std::string {
			llvm::SmallString<128> buf;
			llvm::raw_svector_ostream sstream(buf);
			ty.print(sstream, Policy, placeholser);
			return sstream.str().str();
		};


		llvm::StringSet<> localvars{"orig", "lvl","OS","retname","retval"};
		auto getLocalVarName = [&localvars](const char* base) -> std::string {
			int i = 0;
			while (true) {
				i += 1;
				auto candidate = base + to_string(i);
				if (localvars.contains(candidate))
					continue;
				localvars.insert(candidate);
				return candidate;
			}
		};


		std::string arglist;
		std::string paramlist;
		std::string tylist;
		SmallVector<QualType, 4> ParmTypes;
		{
			llvm::SmallString<128> argbuf;
			llvm::SmallString<128> parambuf;
			llvm::SmallString<128> tybuf;
			llvm::raw_svector_ostream OSarg(argbuf);
			llvm::raw_svector_ostream OSparam(parambuf);
			llvm::raw_svector_ostream OSty(tybuf);
			bool First = true;
			for (auto Param : FD->parameters()) {
				if (!First) {
					OSarg << ", ";
					OSparam << ", ";
					OSty << ", ";
				}
				First = false;

				auto PName = Param->getName();
				QualType ParmTy = Param->getType();
				ParmTy.removeLocalConst();
				ParmTypes.push_back(ParmTy);
				assert(!ParmTy.isConstQualified());

				OSarg << PName;
				ParmTy.print(OSparam, Policy, PName);
				ParmTy.print(OSty, Policy);
			}
			arglist = OSarg.str().str();
			paramlist = OSparam.str().str();
			tylist = OSty.str().str();
		}
		OS << getTy(	FD->getReturnType() ) << " " << FName << "(" << paramlist << ") {\n";
		auto RetTy = FD->getReturnType();



		bool isFreeFunc = FName.endswith("_free");
		bool isUnsupported = FName == "isl_access_info_alloc" || FName == "isl_basic_set_multiplicative_call" || FName == "isl_id_set_free_user";
		bool hasRetVal = !RetTy->isVoidType() ;
		bool hasRetPtr = hasRetVal && RetTy->isPointerType() && !isFreeFunc;


		SmallVector<std::string, 8> Preargs;
		Preargs.clear();
		Preargs.resize(FD->getNumParams());


		OS << "  using FuncTy = " << getTy(FD->getReturnType()) << "(" << tylist << ");\n";
		OS << "  using RetTy = " << getTy(RetTy) << ";\n";
		if (hasRetPtr) {
			OS << "  using RetObjTy = " << getTy(RetTy->getPointeeType()) << ";\n";
		}
		OS << "\n";
		OS << "  static FuncTy *orig = getsym<FuncTy>(\"" << FName << "\");\n";
		OS << "  Level *lvl = getTopmostLevel();\n";
		OS << "  if (lvl->isInternal())\n";
	  OS << "    return (*orig)(" << arglist << ");\n";

		if (isUnsupported) {
			OS << "  ISL_CALL_UNSUPPORTED(" << FName << ")\n";
		}	else {
			OS << "  pushInternalLevel();\n";
			OS << "\n";
			OS << "  std::ostream &OS = lvl->getOutput();\n";
			if (FName == "isl_ctx_parse_options") {
				auto countarg = FD->getParamDecl(1);
				auto ppchararg = FD->getParamDecl(2);
				OS << "  std::string argname = trace_new_result(\"optlist\");\n";
				OS << "  trace_pre_ppchar(OS, argname, " << countarg->getName() << ", " << ppchararg->getName() << ");\n";
				Preargs[2] = "argname";
			} else if (FName == "isl_val_int_from_chunks") {
				auto narg = FD->getParamDecl(1);
				auto sizearg = FD->getParamDecl(2);
				auto chunksarg = FD->getParamDecl(3);
				OS << "  std::string argname = trace_new_result(\"chunks\");\n";
				OS << "  trace_pre_chunks(OS, argname, " << narg->getName() << ", " << sizearg->getName() << ", "  << chunksarg->getName() << ");\n";
				Preargs[3] = "argname";
			} else {
				// Look for callback arguments
				for (auto p: llvm::enumerate(FD->parameters())) {
					ParmVarDecl* Parm = p.value();
					auto i = p.index();
					auto PName = Parm->getName();
					auto ParmTy = Parm->getType();
					auto isCallback = ParmTy->isPointerType() && ParmTy->getPointeeType()->isFunctionType();
					if (!isCallback)
						continue;

					auto UserParm = FD->getParamDecl(i + 1);
					assert(UserParm->getType()->isVoidPointerType());
					assert(Preargs[i].empty());
					assert(Preargs[i+1].empty());
					auto CbTy = Parm->getType()->getPointeeType()->castAs<FunctionProtoType>();
					auto CbRetTy = CbTy->getReturnType();

					auto cbname = getLocalVarName("cb");
					auto username = getLocalVarName("user");

					OS << "  std::string " << cbname << " = trace_new_result(\"cb\");\n";
					OS << "  std::string "<<username<<" = trace_new_result(\"user\");\n";
					OS << "  trace_callback(OS, " << cbname << ", "<<username<<", " << PName << ", " << UserParm->getName() << ");\n";
					Preargs[i] = cbname;
					Preargs[i+1] = username;
				}
			}
			if (hasRetPtr) {
				OS << "  std::string retname = trace_new_result(\"" << getShortname(RetTy) << "\");\n";
				OS << "  OS << \"  " <<  getTy(RetTy) << " \" << retname << \" = " << FName << "(\";\n";
			} else {
				OS << "  OS << \"  " << FName << "(\";\n";
			}
			bool First = true;
			for (auto p: llvm::enumerate(FD->parameters())) {
				ParmVarDecl* Param = p.value();
				auto i = p.index();
				auto PName = Param->getName();
				if (!First)
					OS << "  OS << \", \";\n";
				if (!Preargs[i].empty()) {
					OS << "  OS << " << Preargs[i] << ";\n";
				}	else {
					OS << "  trace_arg(OS, " << PName << ");\n";
				}
				First = false;
			}
			OS << "  OS << \");\\n\";\n";
			OS << "  lvl->flush();\n";

			OS << "\n";

			if (hasRetVal)
				OS << "  " << getTy(RetTy) << " retval = (*orig)(" << arglist << ");\n";
			 else
				OS << "  (*orig)(" << arglist << ");\n";

			if (hasRetPtr) {
				OS << "\n";
				OS << "  trace_register_result<RetObjTy>(retname, retval);\n";
			}

			OS << "\n";
			OS << "  popInternalLevel();\n";

			if (hasRetVal) {
				OS << "\n";
				OS << "  return retval;\n";
			}
		}

		OS << "}\n\n";
		continue;

















		auto FuncCall = [&](const char*MacroName) {
			OS << "  " << MacroName;
			if (FD->getReturnType()->isVoidType()) {
				OS << "_VOID";
			} else {
				OS << "_NONVOID";
			}
			OS << "(" << FName << ", ";
			FD->getReturnType().print(OS, Policy, "");
			OS << ", 0";
			for (auto Parm : FD->parameters()) {
				OS << ", " << Parm->getName();
			}
			OS << ")\n";
		};

		auto ParamCalls = [&](const char* MacroName) {
			int i = 0;
			while (i < FD->getNumParams()) {
				auto Parm = FD->getParamDecl(i);
				auto ParmTy = Parm->getType();
				if (ParmTy->isPointerType() && ParmTy->getPointeeType()->isFunctionType()) {
					// A callback
					auto UserParm = FD->getParamDecl(i + 1);
					assert(UserParm->getType()->isVoidPointerType());
					auto CbTy = Parm->getType()->getPointeeType()->castAs<FunctionProtoType>();
					auto CbRetTy = CbTy->getReturnType();
					OS << "  " << MacroName << "_CB(" << Parm->getName() << ", " << UserParm->getName() << ", ";
					CbRetTy.print(OS, Policy, "");
					for (auto CbParmTy : CbTy->param_types()) {
						OS << ", ";
						CbParmTy.print(OS, Policy, "");
					}
					OS << ")\n";
					i += 2;
					return;
				}

				// A "normal" parameter
				OS << "  " << MacroName << "_ARG(" << Parm->getName(); OS << ", ";  ParmTy.print(OS, Policy, ""); OS << ")\n";
				i += 1;
			}
		};

		// Things that we cannot handle for now
		if (FName == "isl_access_info_alloc" || FName == "isl_basic_set_multiplicative_call" || FName == "isl_id_set_free_user") {
			OS << "  ISL_CALL_UNSUPPORTED(" << FName << ")\n";
		} else {
			FuncCall("ISL_PRECALL");
			ParamCalls("ISL_PREPARE");
			FuncCall("ISL_CALL");
			ParamCalls("ISL_POSTPARE");
			FuncCall("ISL_POSTCALL");
		}

#if 0
		OS << "  ";
		if (!FD->getReturnType()->isVoidType()) {
			OS << "CALL_ORIG";
		} else {
			OS << "CALL_ORIG_VOID";
		}
		OS << "(" << FName << ", ";
		FD->getReturnType().print(OS, Policy, "");
		OS << ", (";
		First = true;
		for (auto Param : FD->parameters()) {
			if (!First)
				OS << ",";
			auto Ty = Param->getType();
			Ty.print(OS, Policy, "", 0);
			First = false;
		}
		OS << "), 0";
		for (auto Param : FD->parameters()) {
			OS << "," << Param->getName();
		}
		OS << ")\n";
#endif

		OS << "}\n\n";
	}
}




