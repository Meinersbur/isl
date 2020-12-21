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

		//Printer.TraverseDecl(FD);
		PrintingPolicy Policy(FD->getASTContext().getLangOpts());



		std::string FStr;
		llvm::raw_string_ostream FOS(FStr);
		FOS << FName << "(";
		bool First = true;
		for (auto Param : FD->parameters()) {
			if (!First)
				FOS << ", ";

			auto Ty = Param->getType();
			Ty.print(FOS, Policy, Param->getName());
			First = false;
		}
		FOS << ")";
		FD->getReturnType().print(OS, Policy, /* placeholder */FOS.str(), /*Indentation=*/0);
		OS << " {\n";

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

		OS << "}\n\n";
	}
}




