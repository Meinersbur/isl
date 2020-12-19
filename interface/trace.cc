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

	//ASTPrinter Printer(llvm::outs(), ASTPrinter::Print, ADOF_Default,  "");
	auto &OS = llvm::outs();

	OS << "// ISL structs\n";
	llvm::StringSet<> Visited;
	for (auto P : classes) {
		isl_class *C =& P.second;
		auto Name = C->name;
		if (Visited.contains(Name)) continue;

		#if 1
		OS << "ISL_STRUCT(" << Name  << ")\n";
		#else
		OS << "typdef struct " << Name << " " <<  Name << ";\n";
		#endif
		Visited.insert(Name);
	}
	OS << "\n\n";



	OS << "// ISL functions\n";
	for (auto ci = functions_by_name.begin(); ci != functions_by_name.end(); ++ci) {
		FunctionDecl *FD = ci->second;
		auto FName = FD->getName();
		if (!FName.startswith("isl_"))
			continue;

		//Printer.TraverseDecl(FD);
		PrintingPolicy Policy(FD->getASTContext().getLangOpts());

	OS << "ISL_FUNC(" << FName << ", (";
		bool First = true;
		for (auto Param : FD->parameters()) {
			if (!First)
				OS << ", ";
			Param->print(OS);
			First = false;		
		}
		OS << "), (";
		First = true;
				for (auto Param : FD->parameters()) {
			if (!First)
				OS << ", ";
			OS << Param->getName();
First = false;
				}
		OS << "))\n";


	#if 1

	#else
		std::string FStr;
		llvm::raw_string_ostream FOS(FStr);
		FOS << FName << "(";
		bool First = true;
		for (auto Param : FD->parameters()) {
			if (!First)
				FOS << ", ";
			Param->print(FOS);
			First = false;		
		}
		FOS << ")";

		FD->getReturnType().print(OS, Policy, /* placeholder */FOS.str(), /*Indentation=*/0);

		
	

		//FD->print(llvm::outs(), Policy, /*Indentation=*/0, /*PrintInstantiation=*/true);
		OS << " {\n";
		OS << "static void *Orig  = loadISLFunc(\"" << FName << "\");\n";
		OS << "  if (!_recursive) {\n";
		OS << "  _recursive = 1;\n";
		OS << "    (*Orig)(";;
		First = true;
		for (auto Param : FD->parameters()) {
			if (!First)
				OS << ", ";
			OS << Param->getName();
		}
		OS << ");\n";
		// <<  << "\n";
		OS << "  }\n";
		OS << "}\n\n";		
		#endif
	}
}




