
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <type_traits>
#include <unordered_map>
#include <iostream>
#include <iosfwd>
#include <ostream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#ifdef _MSC_VER
#define RTLD_NOW 0
void* dlopen(const char* filename, int flag) { return nullptr; }
void* dlsym(void* handle, const char* symbol) { return nullptr; }
#else
#include <dlfcn.h>
#endif
#include "../all.h"

#define __isl_give
#define __isl_keep



template<typename T>
struct Argprinter;



template<typename T>
struct ObjShortnameBase {
	static int getNextNum() {
		static int cur = 0;
		return ++cur;
	}
};


template<typename T> struct ObjShortname : public  ObjShortnameBase<void> {
	static constexpr const char *name = "val";
};


template<typename T>
static std::unordered_map<T*, std::string >& getResultMap() {
	static std::unordered_map< T*,std::string> map;
	return map;
}



namespace {
	template<typename T>
	struct ObjPrinter {
	//	static std::unordered_map<T*, int>& getTheMap() {
	//		static std::unordered_map<T*, int> Map;
	//		return Map;
	//	}


		//static void registerResult(T* t, int c) {
	//		getTheMap()[t] = c;
	//	}

		void print(std::ostream &OS,  const char* SName, T* t) {
			if (t == nullptr) {
				OS << "NULL";
				return;
			}

			auto& Map = getResultMap<T>();
			auto It = Map.find(t);
			if (It == Map.end()) {
				OS << "(" << SName << "*)" << (void*)t;
				return;
			}
			OS << It->second;
		}
	};
}


// TODO: better if the generator emits which object have a _cow function
template<typename T>
struct ObjCow {
	static T* cow(T*obj) { return obj; }
};
extern "C" {
	isl_ctx* isl_ctx_cow(isl_ctx* ctx) { return ctx; }
	isl_id* isl_id_cow(isl_id* id) { return id; }
	isl_union_access_info* isl_union_access_info_cow(isl_union_access_info* obj) { return obj; }
	isl_union_flow* isl_union_flow_cow(isl_union_flow* obj) { return obj; }
	isl_schedule_constraints* isl_schedule_constraints_cow(isl_schedule_constraints* obj) { return obj; }
	isl_printer* isl_printer_cow(isl_printer* obj) { return obj; }

	isl_union_map* isl_union_map_cow(isl_union_map* umap);
	isl_union_set* isl_union_set_cow(isl_union_set* uset) { return (isl_union_set*)isl_union_map_cow((isl_union_map*)uset); }
}




#define STR_(V) #V
#define STR(V) STR_(V)

#define CONCAT2_(A,B) A ## B
#define CONCAT2(A,B) CONCAT2_(A,B)

#define CONCAT3_(A,B,C) A ## B ## C
#define CONCAT3(A,B,C) CONCAT3_(A,B,C)


#define ISL_STRUCT(sname, shortname)            \
	struct sname;                                 \
  extern "C" sname *CONCAT2(sname,_cow) (sname *obj);  \
template<> struct ObjCow<sname> { static sname* cow(sname*obj) { return CONCAT2(sname,_cow)(obj); } }; \
template<> struct ObjShortname<sname*> : public ObjShortnameBase<sname*> { static constexpr const char *name = STR(shortname);  }; \
template<>                                      \
struct Argprinter<sname*> {                     \
void print(std::ostream &OS,   sname* t) {                 \
static ObjPrinter<sname> prn;                   \
		prn.print(OS, STR(sname), t);                \
  }                                             \
};




ISL_STRUCT(isl_access_info, access)
ISL_STRUCT(isl_flow, flow)
ISL_STRUCT(isl_ctx, ctx)
ISL_STRUCT(isl_restriction, restriction)
ISL_STRUCT(isl_map, map)
ISL_STRUCT(isl_set, set)
ISL_STRUCT(isl_local_space, ls)
ISL_STRUCT(isl_ast_expr_list, aelist)
ISL_STRUCT(isl_printer, printer)
ISL_STRUCT(isl_id_to_ast_expr, itamap)
ISL_STRUCT(isl_ast_print_options, astprintoptions)
ISL_STRUCT(isl_mat, map)
ISL_STRUCT(isl_basic_map_list, bmaplist)
ISL_STRUCT(isl_basic_set_list, bsetlist)
ISL_STRUCT(isl_stride_info, stride)
ISL_STRUCT(isl_map_list, maplist)
ISL_STRUCT(isl_vec, vec)
ISL_STRUCT(isl_set_list, setlist)
ISL_STRUCT(isl_union_map_list, umaplist)
ISL_STRUCT(isl_union_pw_multi_aff_list, upwmalust)
ISL_STRUCT(isl_ast_build, astbuild)
ISL_STRUCT(isl_ast_expr, astexpr)
ISL_STRUCT(isl_ast_node, astnode)
ISL_STRUCT(isl_ast_node_list, nodelist)
ISL_STRUCT(isl_basic_map, bmap)
ISL_STRUCT(isl_basic_set, bset)
ISL_STRUCT(isl_fixed_box, fixedbox)
ISL_STRUCT(isl_id, id)
ISL_STRUCT(isl_id_list, idlist)
ISL_STRUCT(isl_aff, aff)
ISL_STRUCT(isl_aff_list, afflist)
ISL_STRUCT(isl_multi_aff, maff)
ISL_STRUCT(isl_multi_id, mid)
ISL_STRUCT(isl_multi_pw_aff, mpwa)
ISL_STRUCT(isl_multi_union_pw_aff, mupwa)
ISL_STRUCT(isl_multi_val, mval)
ISL_STRUCT(isl_point, point)
ISL_STRUCT(isl_pw_aff, pwaff)
ISL_STRUCT(isl_pw_aff_list, pwafflist)
ISL_STRUCT(isl_pw_multi_aff, pwma)
ISL_STRUCT(isl_pw_multi_aff_list, pwmalist)
ISL_STRUCT(isl_union_pw_aff, upwa)
ISL_STRUCT(isl_union_pw_aff_list, upwafflist)
ISL_STRUCT(isl_union_pw_multi_aff, upwma)
ISL_STRUCT(isl_schedule, sched)
ISL_STRUCT(isl_schedule_constraints, schedconstraints)
ISL_STRUCT(isl_schedule_node, schednode)
ISL_STRUCT(isl_space, space)
ISL_STRUCT(isl_union_access_info, uaccess)
ISL_STRUCT(isl_union_flow, uflow)
ISL_STRUCT(isl_union_map, umap)
ISL_STRUCT(isl_union_set, uset)
ISL_STRUCT(isl_union_set_list, usetlist)
ISL_STRUCT(isl_val, val)
ISL_STRUCT(isl_val_list, vallist)
ISL_STRUCT(isl_args, args)
ISL_STRUCT(isl_constraint, constr)


template<>
struct Argprinter<isl_error> {
	void print(std::ostream &OS, isl_error t) {
		OS << "(isl_error)" << t;
	}
};


template<>
struct Argprinter<isl_stat> {
	void print(std::ostream &OS,isl_stat t) {
		switch (t) {
		case isl_stat_error :
			OS << "isl_stat_error";
			return;
		case isl_stat_ok:
			OS << "isl_stat_ok";
			return;
		}
		OS << "(isl_stat)" << t;
	}
};


template<>
struct Argprinter<isl_bool> {
	void print(std::ostream &OS, isl_bool t) {
		switch (t) {
		case 	isl_bool_error :
			OS << "isl_bool_error";
			return;
		case 		isl_bool_false :
			OS << "isl_bool_false";
			return;
		case 	isl_bool_true :
			OS << "isl_bool_true";
			return;
		}

		OS << "(isl_bool)" << t;
	}
};
template<>
struct ObjShortname<isl_bool>: public ObjShortnameBase<isl_bool> {
	static constexpr const char* name = "b";
};

template<>
struct Argprinter<isl_dim_type> {
	void print(std::ostream &OS, isl_dim_type t) {
		switch (t) {
		case	isl_dim_cst:
			OS << "isl_dim_cst";
			return;
		case		isl_dim_param:
			OS << "isl_dim_param";
			return;
		case		isl_dim_in:
			OS << "isl_dim_in";
			return;
		case		isl_dim_out:
			OS << "isl_dim_out";
			return;
		case		isl_dim_div:
			OS << "isl_dim_div";
			return;
		case		isl_dim_all:
			OS << "isl_dim_all";
			return;
		}

		OS << "(isl_dim_type)" << t;
	}
};

template<>
struct Argprinter<isl_ast_expr_op_type> {
	void print(std::ostream &OS,isl_ast_expr_op_type t) {
		OS << "(isl_ast_expr_op_type)" << t;
	}
};

template<>
struct Argprinter<isl_fold> {
	void print(std::ostream &OS, isl_fold t) {
		OS << "(isl_fold)" << t;
	}
};

template<>
struct Argprinter<isl_ast_loop_type> {
	void print(std::ostream &OS,isl_ast_loop_type t) {
		OS << "(isl_ast_loop_type)" << t;
	}
};

template<>
struct Argprinter<double> {
	void print(std::ostream &OS, double t) {
		OS << t;
	}
};

template<>
struct Argprinter<int> {
	void print(std::ostream &OS, int t) {
		OS << t;
	}
};
template<>
struct ObjShortname<int> : public ObjShortnameBase<int> {
	static constexpr const char* name = "i";
};


template<>
struct Argprinter< unsigned int> {
	void print(std::ostream &OS, unsigned int t) {
		OS << t << "u";
	}
};


template<>
struct Argprinter<long  > {
	void print(std::ostream &OS, long t) {
		OS << t;
	}
};


template<>
struct Argprinter<long unsigned int> {
	void print(std::ostream &OS, long unsigned int t) {
		OS << t << "u";
	}
};


template<>
struct Argprinter<long long unsigned int> {
	void print(std::ostream &OS, long long unsigned int t) {
		OS << t << "u";
	}
};


template<>
struct Argprinter<const char*> {
	void print(std::ostream &OS, const char * t) {
		// TODO: escape
		OS << "\"" << t << "\"";
	}
};
template<>
struct ObjShortname<const char*>: public ObjShortnameBase<const char*> {
	static constexpr const char* name = "str";
};




template<>
struct Argprinter<char**> {
	void print(std::ostream &OS, char** t) {
		// TODO: print before, probably invalid syntax
		OS << "(char*[]){";
		bool First = true;
		while (t[0]) {
			if (!First)
				OS << ", ";
			OS <<  "\"" << t[0] << "\"";
			++t;
			First = false;
		}
		OS << "}";
	}
};


// Any other pointer
template<typename T>
struct Argprinter<T*> {
	void print(std::ostream &OS, T* t) {
		//OS << "(void*)" << t;

		if (t == nullptr) {
			OS << "NULL";
			return;
		}

		auto& Map = getResultMap<T>();
		auto It = Map.find(t);
		if (It == Map.end()) {
			OS << "(void*)" << t;
			return;
		}
		OS <<  It->second;
	}
};




struct CallbackBase;


static std::vector<CallbackBase*>& getCallbackerList() {
	static std::vector<CallbackBase*> list;
	return list;
}





struct CallbackBase {
	std::string name;
	void* origUser;
	std::string cbretty;
	std::string cbparamty;
	int callbackidx;
	std::vector<std::ostringstream*> code;
	int inprogress = -1;

	CallbackBase(const std::string &name, void *origUser, const std::string &cbretty, const std::string &cbparamty) : name(name), origUser(origUser), cbretty(cbretty), cbparamty(cbparamty) {
	}

	std::string getName()const {
		return name;
	}

	std::ostringstream& getLastCode() {
		return *code.back();
	}
};


template<int N, typename CallbackT>
struct Callbacker;


template<int N, typename RetTy, typename ...ParamTy>
struct Callbacker<N, RetTy(ParamTy...)> : public CallbackBase {
	using CallbackT = RetTy(ParamTy...);
	CallbackT* origCb;

	explicit Callbacker(const std::string& name,CallbackT* origCb, const std::string &cbretty, const std::string &cbparamty, void *origUser) : CallbackBase(name, origUser,cbretty,cbparamty), origCb(origCb) {}

	static RetTy callback(ParamTy... Params);

	CallbackT* getAddr() {
		return &callback;
	}

	void newInvocation() {
		assert(inprogress==-1);
		inprogress = code.size();
		code.push_back(new std::ostringstream());
	}

	void doneInvocation() {
		assert(inprogress>=0);
		assert(inprogress + 1 == code.size());
		inprogress = -1;
	}
};






#if 0
template<typename RetTy, typename ...ParamTy >
struct Argprinter<RetTy(*)(ParamTy...)> {
	using T = RetTy(ParamTy...);
	void print(std::ostream &OS, T* &t) {
		auto Cb = Callbacker<T>::get();
		OS << Cb->getName();

		// Intercept the callback
		Cb->mostrecent = t;
		t = Cb->getAddr();
	}
};
#endif







static void* openlibisl() {
	static void* handle = nullptr;
	if (!handle) {
		const char* libpath = getenv("ISLTRACE_LIBISL");
		if (!libpath)
			libpath = "/home/meinersbur/build/llvm-project/debug_shared/lib/libPollyISL.so"; // "libisl.so";
		handle = dlopen(libpath, RTLD_NOW);
		if (!handle) {
			fprintf(stderr, "Could not open true libpath at %s\n",libpath);
			exit(1);
		}
		fprintf(stderr, "isltrace loaded\n");
	}
	return handle;
}


static 	const char * prologue = R"(
#include <stdio.h>
#include <isl/aff.h>
#include <isl/aff_type.h>
#include <isl/arg.h>
#include <isl/ast.h>
#include <isl/ast_build.h>
#include <isl/ast_type.h>
#include <isl/constraint.h>
#include <isl/ctx.h>
#include <isl/fixed_box.h>
#include <isl/flow.h>
#include <isl/hash.h>
//#include <isl/hmap.h>
#include <isl/id.h>
#include <isl/id_to_ast_expr.h>
#include <isl/id_to_id.h>
#include <isl/id_to_pw_aff.h>
#include <isl/id_type.h>
#include <isl/ilp.h>
#include <isl/list.h>
#include <isl/local_space.h>
#include <isl/lp.h>
#include <isl/map.h>
#include <isl/map_to_basic_set.h>
#include <isl/map_type.h>
#include <isl/mat.h>
#include <isl/maybe.h>
#include <isl/maybe_ast_expr.h>
#include <isl/maybe_basic_set.h>
#include <isl/maybe_id.h>
#include <isl/maybe_pw_aff.h>
#include <isl/multi.h>
#include <isl/obj.h>
#include <isl/options.h>
#include <isl/point.h>
#include <isl/polynomial.h>
#include <isl/polynomial_type.h>
#include <isl/printer.h>
#include <isl/printer_type.h>
#include <isl/schedule.h>
#include <isl/schedule_node.h>
#include <isl/schedule_type.h>
#include <isl/set.h>
#include <isl/set_type.h>
#include <isl/space.h>
#include <isl/space_type.h>
#include <isl/stream.h>
#include <isl/stride_info.h>
#include <isl/union_map.h>
#include <isl/union_map_type.h>
#include <isl/union_set.h>
#include <isl/union_set_type.h>
#include <isl/val.h>
//#include <isl/val_gmp.h>
#include <isl/val_type.h>
#include <isl/vec.h>
#include <isl/version.h>
#include <isl/vertices.h>

#include "isltrace_vars.inc"
#include "isltrace_callbacks.inc"

int main() {
  const char *dynIslVersion = isl_version();
)";






void writeOutput(FILE *f) {
	// Need to completely close (and re-opened when needed again) the file because fflush does not ensure all data written so far will be in the file if the program crashes.
	fclose(f);
}



#if 0
static bool& get_within_recusion() {
	static bool within_recursion = false;
	return within_recursion;
}


static bool try_enter_recursion() {
	bool& within_recrusion = get_within_recusion();
	if (within_recrusion)
		return false;
	within_recrusion = true;
	return true;
}


static void exit_recursion() {
	bool& within_recrusion = get_within_recusion();
	assert(within_recrusion);
	within_recrusion = false;
}

#endif




template< typename... Rest>
struct Argsprinter;



template<>
struct Argsprinter<> {
	void printArgs(std::ostream &OS) {
		// empty argument list
	}
};


template<typename T>
struct Argsprinter<T> {
	void printArgs(std::ostream &OS, T &t) {
		Argprinter<T>().print(OS, t);
	}
};


template<typename T, typename S,	typename... Rest>
struct Argsprinter<T,S,Rest...> {
	void printArgs(std::ostream &OS,T &t, S &s, Rest &...r) {
		Argsprinter<T>().printArgs(OS, t);
		OS << ", ";
		Argsprinter<S, Rest...>().printArgs(OS, s, r...);
	}
};


template<typename... Rest>
static void printArgs(std::ostream &OS, Rest& ...r) {
	Argsprinter<Rest...>().printArgs(OS, r...);
}



static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

struct Level {
	bool IsMain = false;
	bool IsInternal = false;
	bool IsCallback = false;

	static Level *createMain() {
		Level* Result = new Level();
		Result->IsMain = true;

		Result->mainpath = getenv("ISLTRACE_OUTFILE");
		if (!Result->mainpath)
			Result->mainpath = "isltrace.c";

		Result->mainpath = "isltrace_vars.inc";

		return Result;
	}

	static Level *createInternal() {
		Level* Result = new Level();
		Result->IsInternal = true;
		return Result;
	}

	bool isInternal() {
		return IsInternal;
	}

	CallbackBase* callbacker = nullptr;
	static Level *createCallback(CallbackBase *Callbacker) {
		Level* Result = new Level();
		Result->IsCallback = true;
		Result->callbacker = Callbacker;
		return Result;
	}



	const char* mainpath =nullptr;
	const char* varpath = nullptr;
	std::ofstream* mainfile;
	std::ofstream* varfile;

	std::ostream& getVarfile() {
		assert(IsMain);
		if (!varfile){
			varfile = new std::ofstream(varpath,std:: ios_base::out);
			*varfile << "// Global variable declarions\n";
		} else if (!varfile->is_open()) {
			varfile->open(varpath,std::ios_base::in | std::ios_base::out | std::ios_base::ate);
		}

		assert(varfile);
		assert(varfile->is_open());

		return *varfile;
	}

	std::ostream &getOutput() {
		if (IsMain) {
			if (!mainfile) {
				mainfile = new std::ofstream(mainpath,std:: ios_base::out);
				*mainfile << prologue;
				typedef const char *(*VerFuncTy)(void);
				auto verfunc = (VerFuncTy) dlsym(openlibisl(), "isl_version");
				auto ver = (*verfunc)();
				std::string verstr = ver;
				rtrim(verstr);
				*mainfile << "  const char *genIslVersion = \"" << verstr << "\";\n";
				*mainfile << "  printf(\"### ISL version: %s (%s at generation)\\n\",  dynIslVersion, genIslVersion);\n\n";
				//*mainfile << "  // ISL self-reported version when generation:" << ver << "\n";
			} else if (!mainfile->is_open()) {
				mainfile->open(mainpath,std::ios_base::in | std::ios_base::out | std::ios_base::ate);
			}

			assert(mainfile);
			assert(mainfile->is_open());
			return *mainfile;
		} else if (IsCallback) {
			return callbacker->getLastCode();
		}

		assert(!"No output stream");
		abort();
	}

	void flush() {
		if (IsMain) {
			// Need to completely close (and re-opened when needed again) the file because fflush does not ensure all data written so far will be in the file if the program crashes.
			mainfile->close();
			assert(!mainfile->is_open());

			varfile->close();
			assert(!varfile->is_open());
		}

	}
};




static std::vector<Level*> &getLevelStack() {
	static std::vector<Level*> stack;
	if (stack.empty())
		stack.push_back(Level::createMain());
	return stack;
}

static std::ostream& getVarfile() {
	return getLevelStack().front()->getVarfile();
}


static Level* getTopmostLevel() {
	return getLevelStack().back();
}

static void pushInternalLevel() {
	getLevelStack().push_back(Level::createInternal());
}

static void popInternalLevel() {
	assert(getTopmostLevel()->IsInternal);
	getLevelStack().pop_back();
}


static void pushCallbackLevel(CallbackBase*Cb) {
	assert(getTopmostLevel()->IsInternal);
	getLevelStack().push_back(Level::createCallback(Cb));
}


static void writeCallbackFileOutput() {
	const char* path = getenv("ISLTRACE_CBFILE");
	if (!path)
		path = "isltrace_callbacks.inc";

	std::ofstream OS(path);

	for (auto Cb : getCallbackerList()) {
		auto cbName = Cb->getName();

		OS << "\n";
		OS << Cb->cbretty << " " << cbName << "(" << Cb->cbparamty << ") {\n";
		OS << "static int invoke_num=0;\n";
		OS << "switch(invoke_num++) {\n";
		for (int i = 0; i < Cb->code.size(); i += 1) {
			OS << "case " << i << ": {\n";
			OS << Cb->code[i]->str();
			OS << "} break;\n";
		}
		OS << "default:\n";
		OS << "  abort();\n";
		OS << "};\n";
		OS << "}\n";
	}

	OS.close();
}


static void popCallbackLevel() {
	assert(getTopmostLevel()->IsCallback);
	writeCallbackFileOutput();
	getLevelStack().pop_back();
}


template<typename T>
static void trace_arg(std::ostream& OS,  T t) {
	Argprinter<T>().print(OS,  t);
}


static void trace_pre_ppchar(std::ostream &OS, const  std::string &vname, size_t c, char**p) {
	OS << "  char* " << vname << "[] = {";
	bool First = true;
	for (int i = 0; i < c; i += 1) {
		if (!First)
			OS << ", ";
		OS << "\"" << p[i] << "\"";
		First = false;
	}
	OS << "};\n";
}


static void trace_pre_chunks(std::ostream &OS, const  std::string &chunkvname, size_t n, size_t size, const void *p) {
	if (size == 8) {
		const uint64_t* b = (const uint64_t*)p;
		OS << "  uint64_t " << chunkvname << "[] = {";
		bool First = true;
		for (int i = 0; i < n; i += 1) {
			if (!First)
				OS << ", ";
			OS << "0x" << std::hex << b[i] << std::dec;
			First = false;
		}
		OS << "};\n";
	} else if (size == 4) {
			const uint32_t* b = (const uint32_t*)p;
			OS << "  uint32_t " << chunkvname << "[] = {";
			bool First = true;
			for (int i = 0; i < n; i += 1) {
				if (!First)
					OS << ", ";
				OS << "0x" << std::hex << b[i] << std::dec;
				First = false;
			}
			OS << "};\n";
	} else if (size == 2) {
		const uint16_t* b = (const uint16_t*)p;
		OS << "  uint16_t " << chunkvname << "[] = {";
		bool First = true;
		for (int i = 0; i < n; i += 1) {
			if (!First)
				OS << ", ";
			OS << "0x" << std::hex << b[i] << std::dec;
			First = false;
		}
		OS << "};\n";
	} else {
		auto c = n * size;
		const char* b = (const char*)p;
		OS << "  char " << chunkvname << "[] = {";
		bool First = true;
		for (int i = 0; i < c; i += 1) {
			if (!First)
				OS << ", ";
			OS << "0x" << std::hex << (int)b[i] << std::dec;
			First = false;
		}
		OS << "};\n";
	}
}




static std::unordered_map<std::string, int>& getCountMap() {
	static std::unordered_map<std::string, int> map;
	return map;
}

//template<typename RetTy>
static std::string trace_new_result(const char* shortname) {
	auto &map = getCountMap();
	auto c = ++map[shortname];
	return shortname + std::to_string(c);
}




template<typename T>
static void trace_register_result(const std::string& retname, T* &retval, bool uniquify) {
	if (uniquify)
		retval = ObjCow<T>::cow(retval);

	auto &map = getResultMap<T>();
	map[retval] = retname;
}



template< typename... Rest>
struct Argsregister;

template<>
struct Argsregister<> {
	static void registerArgs(int i) {
		// empty argument list
	}
};

template<typename T, 	typename... Rest>
struct Argsregister<T,Rest...> {
	static void registerArgs( int i, T &t, Rest&... r) {
		if constexpr (std::is_pointer_v<T>) {
			// We could uniquify __isl_take paramters, but not __isl_keep parameters (cow removing a reference is indistinguishable from freeing one).
			// Unifortunately, clang does not remember annotations for the function types of callbacks.
			trace_register_result("param" + std::to_string(i), t, false);
		}
		Argsregister< Rest...>().registerArgs(i+1,r...);
	}
};




template<int N, typename RetTy, typename ...ParamTy >
RetTy Callbacker<N, RetTy(ParamTy...)>::callback(ParamTy... Params) {
	// Calls isl functions (*_dup), so do it in an internal stack context.
	// TODO: actually, use generator::callback_takes_argument
	Argsregister<ParamTy...>::registerArgs(0, Params...);

	std::tuple<ParamTy...> args{Params...};
	auto* user = std::get<N>(args);
	Callbacker* Cb = reinterpret_cast<Callbacker*>(const_cast<void*>(user));
	Cb->newInvocation();
	pushCallbackLevel( Cb );
	auto* lvl = getTopmostLevel();
	std::get<N>(args) = Cb->origUser;

	if constexpr (!std::is_void<RetTy>()) {
		RetTy result = std::apply(*Cb->origCb, args); //(*Cb->origCb)(Params...);

		auto& OS = lvl->getOutput();
		OS << "  return ";
		trace_arg(OS, result);
		OS << ";\n";

		Cb->doneInvocation();
		popCallbackLevel();
		return result;
	} else {
		std::apply(*Cb->origCb, args);

		Cb->doneInvocation();
		popCallbackLevel();
		return ;
	}
}




template<typename CallTy>
static CallTy*  getsym(const char *fname) {
	void* handle = openlibisl();
	auto* orig = (CallTy*)dlsym(handle, fname);
	fprintf(stderr, "Address of %s: %p\n", fname, orig);
	assert(orig);
	return orig;
}








template<int N, typename CallbackTy, typename VoidTy>
static void trace_callback(std::ostream &OS, const std::string &cbname, const char* cbreturnty, const char*cbparamtys,  CallbackTy *&Cb, VoidTy* &user) {
	// VoidTy might be 'void' or 'const void'

	auto& list = getCallbackerList();
	auto callbacker = new Callbacker<N, CallbackTy>(cbname, Cb,  cbreturnty, cbparamtys, (void*)user);
	list.push_back(callbacker);

	//OS << "  CallbackStruct " << username << "{";
	//trace_arg(OS, user);
	//OS << "}";

	// replace
	Cb = callbacker->callback;
	user = callbacker;
}






template<int N, typename CallbackTy>
static void trace_callback(std::ostream& OS, const std::string& cbname, const char* cbreturnty, const char* cbparamtys, CallbackTy*& Cb){
void* dummy;
	trace_callback<N,CallbackTy>(OS, cbname, cbreturnty, cbparamtys, Cb,dummy);
}












template<typename RetTy, typename FuncTy, typename ...ParamTy>
static void trace_precall1(const char* fname, const char* rettyname, void* self, FuncTy*& orig, int dummy, ParamTy... Params) {
	if (!orig) {
		// fprintf(stderr, "Calling %s for the first time\n", fname);
		void* handle = openlibisl();
		orig = (std::remove_reference_t<decltype(orig)>)dlsym(handle, fname);
		fprintf(stderr, "Address of %s: %p\n", fname, orig);
	}
	assert(orig);
	assert(orig != self);
}


template<typename RetTy, typename FuncTy, typename ...ParamTy>
static void trace_precall2(const char* fname, const char* rettyname, void* self, FuncTy* orig, Level *lvl, std::ostream *OS,  int &c, int dummy, ParamTy... Params) {
	pushInternalLevel();
	OS = &lvl->getOutput();
	*OS << "  ";
	if constexpr (!std::is_void<RetTy>()) {
		c = ObjShortname<RetTy>::getNextNum();
		*OS << rettyname << " " << ObjShortname<RetTy>::name << c << " = ";
	}
	*OS << fname << "(";
}


template<typename RetTy, typename FuncTy, typename ...ParamTy>
static RetTy direct_call(const char *fname, const char *rettyname, void*self, FuncTy *&orig, int dummy, ParamTy... Params) {
	return (*orig)(Params...);
}



template<typename RetTy, typename FuncTy, typename ...ParamTy>
static RetTy trace_call(const char *fname, const char *rettyname, void*self, FuncTy *&orig,  std::ostream &OS,  int dummy, ParamTy... Params) {
	printArgs<ParamTy...>(OS, Params...);
	return (*orig)(Params...);
}



template<typename RetTy, typename FuncTy, typename ...ParamTy>
static void trace_postcall(const char* fname, const char* rettyname, void* self, FuncTy* orig, Level* lvl, std::ostream& OS, int dummy, ParamTy... Params) {
	popInternalLevel();
}




#if 0
#define CALL_ORIG(FNAME, RETTY, PARAMTYS, ...)                         \
  static decltype(FNAME) *orig = nullptr; \
	return trace<RETTY>(STR(FNAME), STR(RETTY), (void*)&FNAME, orig, __VA_ARGS__);

#define CALL_ORIG_VOID(FNAME, RETTY, PARAMTYS, ...)                    \
  static decltype(FNAME) *orig = nullptr; \
	       trace<void >(STR(FNAME), STR(RETTY), (void*)&FNAME, orig, __VA_ARGS__);
#endif






#define ISL_PRECALL_NONVOID(FNAME, RETTY, ...)     ISL_PRECALL(FNAME, RETTY, __VA_ARGS__)
#define ISL_PRECALL_VOID(FNAME, RETTY, ...)        ISL_PRECALL(FNAME, RETTY, __VA_ARGS__)
#define ISL_PRECALL(FNAME, RETTY, ...)                                                           \
	  static decltype(FNAME) *orig = nullptr;                                                      \
    int c = -1;                                                                                  \
    std::ostream *OS=nullptr;                                                                    \
		trace_precall1<RETTY>(STR(FNAME), STR(RETTY), (void*)&FNAME, orig, __VA_ARGS__);             \
		Level *lvl = getTopmostLevel();                                                                     \
		if (lvl->IsInternal)                                                                         \
			return direct_call<RETTY>(STR(FNAME), STR(RETTY), (void*)&FNAME, orig, __VA_ARGS__);       \
	  trace_precall2<RETTY>(STR(FNAME), STR(RETTY), (void*)&FNAME, orig, lvl, OS, c, __VA_ARGS__);




#define ISL_PREPARE_ARG(PNAME, PTY)                 \
	Argprinter<decltype(PNAME)>().print(*OS, PNAME);
#define ISL_PREPARE_CB(...)


#define ISL_CALL_NONVOID(FNAME, RETTY, ...) \
  *OS << ");" ;                               \
  lvl->flush();                              \
	auto retval = trace_call<RETTY>(STR(FNAME), STR(RETTY), (void*)&FNAME, orig,*OS, __VA_ARGS__); \
  *OS << "\n";

#define ISL_CALL_VOID(FNAME, RETTY, ...) \
  *OS << ");" ;                           \
  lvl->flush();                           \
	trace_call<RETTY>(STR(FNAME), STR(RETTY), (void*)&FNAME, orig, *OS, __VA_ARGS__); \
  *OS << "\n";


#define ISL_POSTPARE_ARG(PNAME, PTY)
#define ISL_POSTPARE_CB(...)


#define ISL_POSTCALL_NONVOID(FNAME, RETTY, ...)                                             \
if constexpr (std::is_pointer<RETTY>::value) {  \
  	using BaseTy = typename std::remove_pointer<RETTY>::type;                                 \
  retval = ObjCow<BaseTy>::cow(retval);                                                     \
  ObjPrinter<BaseTy>::registerResult(retval, c);                                            \
} \
  trace_postcall<RETTY>(STR(FNAME), STR(RETTY), (void*)&FNAME, orig, lvl, *OS, __VA_ARGS__); \
	return retval;
#define ISL_POSTCALL_VOID(FNAME, RETTY, ...)                                                \
  trace_postcall<RETTY>(STR(FNAME), STR(RETTY), (void*)&FNAME, orig, lvl, *OS, __VA_ARGS__); \
	return;






#define ISL_CALL_UNSUPPORTED(FNAME, ...) \
	fprintf(stderr, "Unsupported call %s\n", STR(FNAME)); \
	abort();



template<typename ObjT>
struct ResultObjPrinter {
	static void printResult(std::ostream& OS, ObjT* obj) {}
};

template<>
struct ResultObjPrinter<isl_map> {
	static void printResult(std::ostream& OS, isl_map* obj) {
		OS << " // ";
		auto p = isl_printer_to_str(isl_map_get_ctx(obj));
		p = isl_printer_print_map(p, obj);
	auto c=	isl_printer_get_str(p);
	OS << c;
	free(c);
	isl_printer_free(p);
	}
};


template<typename T>
static void trace_print_result(std::ostream &OS, T* obj) {
	if (!obj) return;
	ResultObjPrinter<T>::printResult(OS,obj);
}


#include "libtrace.inc.cpp"








