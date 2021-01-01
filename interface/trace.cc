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
#include <llvm/ADT/StringSet.h>
#include <llvm/ADT/SmallBitVector.h>




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
		{"isl_basic_map", "bmap"},
		{"isl_basic_set", "bset"},
		{"isl_union_map", "umap"},
		{"isl_union_set", "uset"},
		{"isl_schedule", "sched"},
		{"isl_aff", "aff"},
		{"isl_multi_aff", "ma"},
	  {"isl_multi_pw_aff", "mpwa"},
	  {"isl_multi_union_pw_aff", "mupwa"},
  	{"isl_pw_aff", "pwaff"},
	  {"isl_pw_multi_aff", "pwma"},
	  {"isl_union_pw_aff", "upwa"},
	  {"isl_union_pw_multi_aff", "upwma"},
		{"isl_val", "val"},
		{"isl_multi_val", "mval"},
		{"isl_point", "point"},
		{"isl_int", "i"},
		{"isl_constraint", "constr"},
		{"isl_id", "id"},
		{"isl_multi_id", "mid"},
		{"isl_union_access_info", "accesses"},
		{"isl_union_flow", "flow"},
		{"isl_schedule_node", "node"},
		{"isl_ast_node", "node"},
		{"isl_ast_build", "build"},
		{"isl_vec", "vec"},
		{"isl_mat", "mat"},
	};
	auto It = map.find(longname);
	if (It != map.end())
		return It->second;
	if (longname.endswith("_list"))
		return "list";
	return "obj";
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


static auto getTyList(FunctionDecl *FD, bool PrintType, bool PrintName) -> std::string {
	PrintingPolicy Policy(FD->getASTContext().getLangOpts());
	llvm::SmallString<128> buf;
	llvm::raw_svector_ostream sstream(buf);
	bool First = true;
	for (auto p : llvm::enumerate(FD->parameters())) {
		ParmVarDecl	*Parm = p.value();
		auto i = p.index();
		QualType ParmTy = Parm->getType();

		if (!First)
			sstream << ", ";
		if (PrintType && PrintType) {
			ParmTy.print(sstream, Policy, Parm->getName());
		} else if (PrintType) {
			ParmTy.print(sstream, Policy);
		} else if (PrintName) {
			sstream << Parm->getName();
		}
		First = false;
	}
	return sstream.str().str();
};


static auto getParamTyList(ASTContext&ASTCtx,const FunctionProtoType *FTy, bool PrintType, bool PrintName) -> std::string {
	PrintingPolicy Policy(ASTCtx.getLangOpts());
	llvm::SmallString<128> buf;
	llvm::raw_svector_ostream sstream(buf);
	bool First = true;
	for (auto p : llvm::enumerate(FTy->param_types())) {
		auto i = p.index();
		QualType ParmTy = p.value();

		if (!First)
			sstream << ", ";
		if (PrintType && PrintName) {
			ParmTy.print(sstream, Policy, Twine("param") + Twine(i));
		} else if (PrintType) {
			ParmTy.print(sstream, Policy);
		} else if (PrintName) {
			sstream << "param" << i;
		}
		First = false;
	}
	return sstream.str().str();
};



static std::string emitCallbackFunc(llvm::raw_ostream &OS, FunctionDecl* FD, int CallbackFnArg, int UserPtrArg, const FunctionProtoType *&CbTy) {
	PrintingPolicy Policy(FD->getASTContext().getLangOpts());
	auto getTy = [&](QualType ty, const Twine& placeholder = {}) -> std::string {
		llvm::SmallString<128> buf;
		llvm::raw_svector_ostream sstream(buf);
		ty.print(sstream, Policy, placeholder);
		return sstream.str().str();
	};
	auto getParamTyList = [&](const FunctionProtoType * FTy, bool PrintType, bool PrintName) -> std::string {
		return ::getParamTyList(FD->getASTContext(), FTy, PrintType, PrintName);
	};


	auto FName = FD->getName();
	auto FuncParm = FD->getParamDecl(CallbackFnArg);
	ParmVarDecl* UserParm = nullptr;
	if (UserPtrArg >= 0) {
		UserParm = FD->getParamDecl(UserPtrArg);
	}

	CbTy = FuncParm->getType()->getPointeeType()->castAs<FunctionProtoType>();
	auto CbRetTy = CbTy->getReturnType();
	auto CbHasRet = !CbRetTy->isVoidType();
	// Assume the last parameter of the callback is the user pointer.
	auto CbUserArg =CbTy->getNumParams()-1 ;
	auto CbUserArgTy = CbTy->getParamType(CbUserArg);
	assert(CbUserArgTy->isVoidPointerType());
	//assert(Preargs[i].empty());

	//ParmVarDecl* UserParm = nullptr;
	//if (FName != "isl_id_set_free_user") {
	//	UserParm = FD->getParamDecl(UserPtrArg);
	//	auto UserName = UserParm->getName();
	//	assert(UserParm->getType()->isVoidPointerType());
	//	//assert(Preargs[i + 1].empty());
	//}


	auto cbname = (Twine(FName) + "_cb" + Twine(CallbackFnArg)).str();

	OS << "static " << getTy(CbRetTy) << " " << cbname << "(";
	{
		bool First = true;
		for (auto p : llvm::enumerate(CbTy->param_types())) {
			auto CbParamTy = p.value();
			auto i = p.index();
			if (!First) {
				OS << ", ";
			}
			OS << getTy(CbParamTy, "param" + Twine(i));
			First = false;
		}
	}
	OS << ") {\n";

#if 0
		SmallVector<std::string, 4> ForwardArgs;
		ForwardArgs.resize(CbTy->getNumParams());
		for (auto p : llvm::enumerate(CbTy->param_types())) {
			auto CbParamTy = p.value();
			auto i = p.index();
			ForwardArgs[i] = ("param" + Twine(i)).str();
		}
		//ForwardArgs[CbUserArg]
		//ForwardArgs[CbUserArg] = "Cb->replCb";
#endif

		//std::tuple<ParamTy...> args{Params...};
		//auto* user = std::get<N>(args);
		OS << "  using CbTy = " << getTy(CbRetTy) << "(" << getParamTyList(CbTy, true, false)  << ");\n";
		//OS << "  typedef " << getTy(CbRetTy, );
		OS << "  using CallbackerTy = Callbacker<CbTy>;\n";
		if (CbHasRet) {
			OS << "  using RetTy = " << getTy(CbRetTy) << ";\n";
		}
		OS << "  CallbackerTy* Cb = (CallbackerTy*)(param" << CbUserArg << ");\n";


		OS << "\n";
		OS << "  CbCall<"<< CbUserArg << ", CbTy> call(Cb);\n";

	//	OS << "  Cb->newInvocation();\n";
	//	OS << "  pushCallbackLevel(Cb);\n";
		//OS << "  Level* lvl = getTopmostLevel();\n";
		//std::get<N>(args) = Cb->origUser;

		//OS << "  std::ostream& OS1 = lvl->getOutput();\n";
		//OS << "  std::ostream& OSvars = getVarfile();\n";
		//OS << "\n";

	//	if (CbUserArg>=0)
	//	OS << "  call.trace_user<" << CbUserArg << ">(param"<< CbUserArg <<");\n";

		for (auto p : llvm::enumerate(CbTy->param_types())) {
			auto CbParamTy = p.value();
			auto i = p.index();

			if (i == CbUserArg)
				continue;

			OS << "  call.trace_arg<" << i << ">(param"<< i <<");\n";

				//OS << "  param" << i << " = Cb->origUser;\n";
			//} else if (isIslObjTy(CbParamTy)) {
			//	OS << "  std::string param" << i << "name = trace_new_result(\"" << getShortname(CbParamTy) << "\");\n";
				//OS << "  OSvars << \"  " << getTy(CbParamTy) << "\" << param" << i << "name;\n";
				//OS << "  OS1 << \"  \" << param" << i << "name << \"= param" << i << "\";\n";
				//OS << "  trace_register_result(param" << i << "name, param" << i << ");\n";
			//} else
			//	continue;
			//OS << "\n";
		}

	//	if (CbHasRet) {
		//	OS << "  RetTy retval = (*Cb->origCb)(" << getParamTyList(CbTy, false, true) << ");\n";
			//OS << "  std::ostream& OS2 = lvl->getOutput();\n";
			//OS << "  OS2 << \"  return \";\n";
			//OS << "  trace_arg(OS2, retval);\n";
			//OS << "  OS2 << \"\\n\";\n";
	//	} else {
	//		OS << "  (*Cb->origCb)(" << getParamTyList(CbTy,false, true) << ");\n";
	//	}

	//	OS << "  Cb->doneInvocation();\n";
		//OS << "  popCallbackLevel();\n";

		OS << "  ";
		if (CbHasRet)
			OS << "return ";
		OS << "call.apply();\n";

		OS << "}\n\n";
		return cbname;
}



/* Generate conversion functions for converting objects between
 * the default and the checked C++ bindings.
 * Do this for each exported class.
 */
void trace_generator::generate() {
	std::string InitCode;
	llvm::raw_string_ostream InitOS(InitCode);
	auto &OS = llvm::outs();


	OS << "// ISL functions\n";
	for (auto ci = functions_by_name.begin(); ci != functions_by_name.end(); ++ci) {
		FunctionDecl* FD = ci->second;
		auto FName = FD->getName();
		if (!FName.startswith("isl_"))
			continue;

		PrintingPolicy Policy(FD->getASTContext().getLangOpts());
		auto getTy = [&](QualType ty, const Twine& placeholder = {}) -> std::string {
			llvm::SmallString<128> buf;
			llvm::raw_svector_ostream sstream(buf);
			ty.print(sstream, Policy, placeholder);
			return sstream.str().str();
		};
		auto getParamTyList = [&](const FunctionProtoType * FTy, bool PrintType, bool PrintName) -> std::string {
			return ::getParamTyList(FD->getASTContext(), FTy, PrintType, PrintName);
		};


		llvm::StringSet<> localvars{"orig", "lvl", "OS", "retname", "retval"};
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


		bool isFreeFunc = FName.endswith("_free");
		bool isUnsupported = FName == "isl_access_info_alloc" || FName == "isl_basic_set_multiplicative_call";
		QualType RetTy = FD->getReturnType();
		bool hasRetVal = !RetTy->isVoidType() ;
		bool hasRetPtr = hasRetVal && RetTy->isPointerType() && !isFreeFunc;

		SmallVector<std::string, 8> Preargs;
		Preargs.clear();
		Preargs.resize(FD->getNumParams());

		if (!isUnsupported) {
			for (auto p : llvm::enumerate(FD->parameters())) {
				ParmVarDecl* Parm = p.value();
				auto i = p.index();
				auto PName = Parm->getName();
				auto ParmTy = Parm->getType();
				auto isCallback = ParmTy->isPointerType() && ParmTy->getPointeeType()->isFunctionType();
				if (!isCallback)
					continue;

				auto UserParmIdx = -1;
				if (FName != "isl_id_set_free_user") {
					UserParmIdx = i + 1;
				}

				const FunctionProtoType* CbTy=nullptr;
				auto cbname = emitCallbackFunc(OS, FD, i, UserParmIdx,CbTy);
				llvm::SmallString<256> buf;
				llvm::raw_svector_ostream cbOS(buf);
				std::string username = "NULL";
				std::string userty = "NULL";
				if (UserParmIdx >= 0) {
					username = (Twine("\"") + FD->getParamDecl(UserParmIdx)->getName() + "\"").str();
					userty = (Twine("\"") + getTy(FD->getParamDecl(UserParmIdx)->getType()) + Twine("\"")).str();
				}
				std::string cbty =   (Twine("\"") + getTy(ParmTy) + Twine("\"") ).str();

				if (UserParmIdx==-1)
					cbOS << "  call.trace_cb<" << i << "," << UserParmIdx << ">(\"" << FName << "\", \"" << PName <<"\", " << username << ", " <<  cbty  << ", " << userty << ", \"" << getTy(CbTy->getReturnType()) << "\", \"" << getParamTyList(CbTy,true,false) << "\", "<< PName <<", &"<< cbname <<", (void*)nullptr);\n";
				else
					cbOS << "  call.trace_cb<" << i << "," << UserParmIdx << ">(\"" << FName << "\", \"" << PName <<"\", " << username << ", " <<  cbty  << ", " << userty << ", \"" << getTy(CbTy->getReturnType()) << "\", \"" << getParamTyList(CbTy,true,false) << "\", "<< PName <<", &"<< cbname <<", "<< FD->getParamDecl(UserParmIdx)->getName() <<");\n";

				Preargs[i] = cbOS.str().str();
				if (UserParmIdx != -1)
					Preargs[UserParmIdx] = "  /* skip user*/\n"; // Do not have a trace_arg line for the user arg.
			}
		}

		OS << getTy(FD->getReturnType()) << " " << FName << "(" << paramlist << ") {\n";
		OS << "  using FuncTy = " << getTy(FD->getReturnType()) << "(" << tylist << ");\n";
		//OS << "  using RetTy = " << getTy(RetTy) << ";\n";
		if (hasRetPtr) {
		//	OS << "  using RetObjTy = " << getTy(RetTy->getPointeeType()) << ";\n";
		}
		OS << "  static FuncTy *orig = getsym<FuncTy>(\"" << FName << "\");\n";
	//	OS << "  Level *lvl = getTopmostLevel();\n";
		OS << "  if (getIsInternal())\n";
	  OS << "    return (*orig)(" << arglist << ");\n";

		if (isUnsupported) {
			OS << "  ISL_CALL_UNSUPPORTED(" << FName << ")\n";
			OS << "}\n\n";
			continue;
		}

		OS << "\n";
		OS << "  IslCall<FuncTy> call(\""<< FName <<"\", \"" << getTy(RetTy) << "\", orig);\n";

		if (FName == "isl_ctx_parse_options") {
			auto countarg = FD->getParamDecl(1);
			auto ppchararg = FD->getParamDecl(2);
			//OS << "  std::string argname = trace_new_result(\"optlist\");\n";
			//OS << "  trace_pre_ppchar(OS, argname, " << countarg->getName() << ", " << ppchararg->getName() << ");\n";
			//Preargs[2] = "argname";
			Preargs[2] = (Twine( "  call.trace_argv<1,2>(\"") + ppchararg->getName() + "\", \"" +  getTy(ppchararg->getType()) + "\", " + countarg->getName() + ", " + ppchararg->getName() + ");\n" ).str();
		} else if (FName == "isl_val_int_from_chunks") {
			auto narg = FD->getParamDecl(1);
			auto sizearg = FD->getParamDecl(2);
			auto chunksarg = FD->getParamDecl(3);
			//OS << "  std::string argname = trace_new_result(\"chunks\");\n";
			//OS << "  trace_pre_chunks(OS, argname, " << narg->getName() << ", " << sizearg->getName() << ", "  << chunksarg->getName() << ");\n";
			//Preargs[3] = "argname";
			Preargs[3] = (Twine("  call.trace_chunks<1,2,3>(\"")+
				narg->getName()+"\", \""+sizearg->getName()+"\", \""+chunksarg->getName()+"\", \""+
				getTy(narg->getType())+"\", \""+	getTy(sizearg->getType())+"\", \""+	getTy(chunksarg->getType())+"\", "+
				narg->getName()+", "+sizearg->getName()+", "+chunksarg->getName()+");\n").str();
		}
#if 0
		else if (FName == "isl_val_get_abs_num_chunks") {
			auto v = FD->getParamDecl(0);
			auto size =  FD->getParamDecl(1);
			auto chunks = FD->getParamDecl(2);

			// chunks is written to, but we don't care about the result; set to NULL so it's an error
			OS << "  call.trace_arg<2, "<< getTy(chunks->getType()) <<">(\"" << chunks->getName() << "\", \""<< getTy(chunks->getType()) <<"\", "<<chunks->getName()<<");\n";
		}
#endif


		for (auto p : llvm::enumerate(FD->parameters())) {
			auto i = p.index();
			if (!Preargs[i].empty()) {
				OS << Preargs[i];
				continue;
			}
			ParmVarDecl* Parm = p.value();
			QualType ParmTy = Parm->getType();
			auto PName = Parm->getName();

			//bool isCallback = ParmTy->isPointerType() && ParmTy->getPointeeType()->isFunctionType();
			//if (isCallback) {
			//	continue;
			//}
			OS << "  call.trace_arg<" << i << ", "<< getTy(ParmTy) <<">(\"" << PName << "\", \""<< getTy(ParmTy) <<"\", "<<PName<<");\n";
			Preargs[i] = FName.str();
		}

		//OS << "  call->endPrepareIslCall();\n";
		//	OS << "\n";
			// OS << "  std::ostream &OS = lvl->getOutput();\n";
			//if (hasRetPtr) {
			//	OS << "  std::ostream &VarsOS = getVarfile();\n";
			//}

			if (hasRetVal) {
			}

#if 0
			if (hasRetPtr) {
				OS << "  std::string retname = trace_new_result(\"" << getShortname(RetTy) << "\");\n";
				OS << "  VarsOS <<  \"" << getTy(RetTy) << " \" << retname << \";\\n\";\n";
				OS << "  OS << \"  \" << retname << \" = " << FName << "(\";\n";
			} else {
				OS << "  OS << \"  " << FName << "(\";\n";
			}
#endif

#if 0
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
			OS << "  OS << \");\";\n";
			OS << "  lvl->flush();\n";

			OS << "\n";
#endif

			OS << "  ";
			if (hasRetVal)
				OS << "return ";
			OS << "call.apply();\n";
#if 0
			bool First = true;
			for (auto arg : Preargs) {
				if (!First)
					OS << ", ";
				OS << arg;
			}
			OS << ");\n";
#endif

#if 0
			if (hasRetVal)
				OS << "  " << getTy(RetTy) << " retval = (*orig)(" << arglist << ");\n";
			 else
				OS << "  (*orig)(" << arglist << ");\n";


			OS << "  std::ostream &OS2 = lvl->getOutput();\n";
			if (hasRetPtr) {
				OS << "\n";
				OS << "  trace_register_result_uniquify<RetObjTy>(retname, retval);\n";
				OS << "  trace_print_result<RetObjTy>(OS2, retval);\n";
			}
			OS << "  OS2 << \"\\n\";";

			OS << "\n";
#endif

#if 0
			OS << "  popInternalLevel();\n";

			if (hasRetVal)
				OS << "  return retval;\n";
#endif

		OS << "}\n\n";

	}
}




