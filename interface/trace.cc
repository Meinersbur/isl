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
		{"isl_map", "map"},
		{"isl_set", "set"},
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


		std::string arglist;
		std::string paramlist;
		std::string tylist;
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

				OSarg << Param->getName();

				auto Ty = Param->getType();
				Ty.print(OSparam, Policy, Param->getName());

				Ty.print(OSty, Policy);

				First = false;
			}
			arglist = OSarg.str().str();
			paramlist = OSparam.str().str();
			tylist = OSty.str().str();
		}
		OS << getTy(	FD->getReturnType() ) << " " << FName << "(" << paramlist << ") {\n";


		if (FName == "isl_access_info_alloc" || FName == "isl_basic_set_multiplicative_call" || FName == "isl_id_set_free_user") {
			OS << "  ISL_CALL_UNSUPPORTED(" << FName << ")\n";
			OS << "}\n\n";
			continue;
		}



		auto RetTy = FD->getReturnType();
		bool hasRetVal = !RetTy->isVoidType();
		bool hasRetPtr = RetTy->isPointerType();

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
		OS << "  pushInternalLevel();\n";
		OS << "\n";
		OS << "  std::ostream &OS = lvl->getOutput();\n";
		if (hasRetPtr) {
			OS << "  std::string retname = trace_new_result(\"" << getShortname(RetTy) << "\");\n";
			OS << "  OS << \"  RetTy \" << retname << \" = " << FName << "(\";\n";
		} else {
			OS << "  OS << \"  " << FName << "(\";\n";
		}
		bool First = true;
		for (ParmVarDecl* Param : FD->parameters()) {
			auto PName = Param->getName();
			if (!First)
				OS << "  OS << \", \";\n";
			OS << "  trace_arg(OS, " <<PName<< ");\n";
		}
		OS << "  OS << \");\\n\";\n";
		OS << "  lvl->flush();\n";

		OS << "\n";

		if (hasRetVal) {
			OS << "  RetTy retval = (*orig)(" << arglist << ");\n";
		}	else {
			OS << "  (*orig)(" << arglist << ");\n";
		}

		if (hasRetPtr) {
			OS << "\n";
			OS << "  trace_register_result<RetObjTy>(retname, retval);\n";
		}

		OS << "  popInternalLevel();\n";

		if (hasRetVal) {
			OS << "\n";
			OS << "  return retval;\n";
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




