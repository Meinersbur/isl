
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
struct Argprinter {
	static void print(std::ostream& OS, T t) { OS << "/*Argprinter*/";  }
};



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


// TODO: better if the generator emits which objects have a _copy/_cow function
template<typename T>
struct ObjCow {
	static T* cow(T*obj) { return obj; }
};
extern "C" {
	isl_ctx* isl_ctx_copy(isl_ctx* ctx) { return ctx; }
	isl_printer* isl_printer_copy(isl_printer* obj) { return obj; }
	isl_access_info* isl_access_info_copy(isl_access_info* obj) { return obj; }
//	isl_union_access_info* isl_union_access_info_copy(isl_union_access_info* obj) { return obj; }
	isl_flow* isl_flow_copy(isl_flow* obj) { return obj; }
	isl_restriction* isl_restriction_copy(isl_restriction* obj) { return obj; }
	isl_args *isl_args_copy(isl_args* obj) { return obj; }
	isl_obj *isl_obj_copy(isl_obj* obj) { return obj; }

	isl_ctx* isl_ctx_cow(isl_ctx* ctx) { return ctx; }
	isl_id* isl_id_cow(isl_id* id) { return id; }
	isl_union_access_info* isl_union_access_info_cow(isl_union_access_info* obj) { return obj; }
	isl_union_flow* isl_union_flow_cow(isl_union_flow* obj) { return obj; }
	isl_schedule_constraints* isl_schedule_constraints_cow(isl_schedule_constraints* obj) { return obj; }
	isl_printer* isl_printer_cow(isl_printer* obj) { return obj; }
	__isl_give isl_union_map* isl_union_map_cow(__isl_take isl_union_map* umap);
	__isl_give isl_union_set* isl_union_set_cow(__isl_take isl_union_set* uset)  { return (isl_union_set*)isl_union_map_cow((isl_union_map*)uset); }
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
template<> struct ObjCow<sname> { static sname* cow(sname*obj) {   /* if(!obj) return NULL; obj = CONCAT2(sname,_copy)(obj);*/ return CONCAT2(sname,_cow)(obj); } }; \
template<> struct ObjShortname<sname*> : public ObjShortnameBase<sname*> { static constexpr const char *name = STR(shortname);  }; \
template<>                                      \
struct Argprinter<sname*> {                     \
static void print(std::ostream &OS,   sname* t) {                 \
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
ISL_STRUCT(isl_ast_expr_list, list)
ISL_STRUCT(isl_printer, printer)
ISL_STRUCT(isl_id_to_ast_expr, itamap)
ISL_STRUCT(isl_ast_print_options, astprintoptions)
ISL_STRUCT(isl_mat, map)
ISL_STRUCT(isl_basic_map_list, bmaplist)
ISL_STRUCT(isl_basic_set_list, bsetlist)
ISL_STRUCT(isl_stride_info, stride)
ISL_STRUCT(isl_map_list, list)
ISL_STRUCT(isl_vec, vec)
ISL_STRUCT(isl_set_list, list)
ISL_STRUCT(isl_union_map_list, list)
ISL_STRUCT(isl_union_pw_multi_aff_list, list)
ISL_STRUCT(isl_ast_build, astbuild)
ISL_STRUCT(isl_ast_expr, astexpr)
ISL_STRUCT(isl_ast_node, astnode)
ISL_STRUCT(isl_ast_node_list, list)
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
ISL_STRUCT(isl_pw_aff_list, list)
ISL_STRUCT(isl_pw_multi_aff, pwma)
ISL_STRUCT(isl_pw_multi_aff_list, list)
ISL_STRUCT(isl_union_pw_aff, upwa)
ISL_STRUCT(isl_union_pw_aff_list, list)
ISL_STRUCT(isl_union_pw_multi_aff, upwma)
ISL_STRUCT(isl_schedule, sched)
ISL_STRUCT(isl_schedule_constraints, schedconstraints)
ISL_STRUCT(isl_schedule_node, schednode)
ISL_STRUCT(isl_space, space)
ISL_STRUCT(isl_union_access_info, uaccess)
ISL_STRUCT(isl_union_flow, uflow)
ISL_STRUCT(isl_union_map, umap)
ISL_STRUCT(isl_union_set, uset)
ISL_STRUCT(isl_union_set_list, list)
ISL_STRUCT(isl_val, val)
ISL_STRUCT(isl_val_list, vallist)
ISL_STRUCT(isl_args, args)
ISL_STRUCT(isl_constraint, constr)
ISL_STRUCT(isl_obj, obj)


template<>
struct Argprinter<isl_error> {
	static void print(std::ostream &OS, isl_error t) {
		OS << "(isl_error)" << t;
	}
};


template<>
struct Argprinter<isl_token_type> {
	static void print(std::ostream &OS, isl_token_type t) {
		OS << "(isl_token_type)" << t;
	}
};


template<>
struct Argprinter<isl_ast_node_type> {
	static void print(std::ostream &OS, isl_ast_node_type t) {
		OS << "(isl_ast_node_type)" << t;
	}
};


template<>
struct Argprinter<isl_ast_expr_type> {
	static void print(std::ostream &OS, isl_ast_expr_type t) {
		OS << "(isl_ast_expr_type)" << t;
	}
};

template<>
struct Argprinter<isl_stat> {
	static void print(std::ostream &OS,isl_stat t) {
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
	static void print(std::ostream &OS, isl_bool t) {
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
	static	void print(std::ostream &OS, isl_dim_type t) {
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
	static void print(std::ostream &OS,isl_ast_expr_op_type t) {
		OS << "(isl_ast_expr_op_type)" << t;
	}
};

template<>
struct Argprinter<isl_fold> {
	static void print(std::ostream &OS, isl_fold t) {
		OS << "(isl_fold)" << t;
	}
};

template<>
struct Argprinter<isl_ast_loop_type> {
static	void print(std::ostream &OS,isl_ast_loop_type t) {
		OS << "(isl_ast_loop_type)" << t;
	}
};

template<>
struct Argprinter<double> {
	static	void print(std::ostream &OS, double t) {
		OS << t;
	}
};

template<>
struct Argprinter<int> {
	static	void print(std::ostream &OS, int t) {
		OS << t;
	}
};
template<>
struct ObjShortname<int> : public ObjShortnameBase<int> {
	static constexpr const char* name = "i";
};


template<>
struct Argprinter< unsigned int> {
	static	void print(std::ostream &OS, unsigned int t) {
		OS << t << "u";
	}
};


template<>
struct Argprinter<long  > {
	static void print(std::ostream &OS, long t) {
		OS << t;
	}
};


template<>
struct Argprinter<long unsigned int> {
	static void print(std::ostream &OS, long unsigned int t) {
		OS << t << "u";
	}
};


template<>
struct Argprinter<long long unsigned int> {
	static	void print(std::ostream &OS, long long unsigned int t) {
		OS << t << "u";
	}
};


template<>
struct Argprinter<const char*> {
	static	void print(std::ostream &OS, const char * t) {
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
	static void print(std::ostream &OS, char** t) {
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
	static	void print(std::ostream &OS, T* t) {
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
	std::string fname;
	std::string pname;
	void* origUser;
	std::string cbretty;
	std::string cbparamty;
	int callbackidx;
	std::vector<std::ostringstream*> code;
	int inprogress = -1;

	CallbackBase(const std::string &fname, const std::string &pname, void *origUser, const std::string &cbretty, const std::string &cbparamty)
		: fname{ fname }, pname(pname), origUser(origUser), cbretty{ cbretty }, cbparamty(cbparamty) {}

	std::string getName()const {
		return pname;
	}

	std::ostringstream& getLastCode() {
		return *code.back();
	}
};


template<typename CallbackT>
struct Callbacker;

static std::ostream& openLogfile();
static void escape(std::ostream &OS, const std::string& name, const std::string& val);
static void escape(std::ostream& OS, const std::string& name, void *val);
static void escape(std::ostream& OS, const std::string& name, int val) ;

template<typename RetTy, typename ...ParamTy>
struct Callbacker<RetTy(ParamTy...)> : public CallbackBase {
	using CallbackT = RetTy(ParamTy...);
	CallbackT* origCb;
	CallbackT* replCb;

	explicit Callbacker(const std::string &fname, const std::string& pname,CallbackT* origCb,CallbackT* replCb, const std::string &cbretty, const std::string &cbparamty, void *origUser)
		: CallbackBase(fname, pname, origUser,cbretty,cbparamty), origCb(origCb), replCb(replCb) {
#if 0
        auto& OS = openLogfile();
        OS << "\ncallbacker_created";
	    escape(OS, "callbacker", (void*)this);
        escape(OS, "fname",  fname);
         escape(OS, "pname",  pname);
				 flushLogfile();
#endif
        }

	constexpr int getNumArgs() {
		return sizeof...(ParamTy);
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

static void escape(std::ostream &OS, const std::string& name, const std::string& val) {
	OS << ' ' << name << '=';
	if (val.find_first_of(" \"\'\t\r\n\\") == std::string::npos) {
		OS << val;
		return;
	}

	OS << '\"';
	for (auto c : val) {
		switch (c) {
		case '\"':
			OS << "\\\""; break;
		case '\'':
			OS << "\\\'"; break;
		case '\t':
			OS << "\\t"; break;
		case '\r':
			OS << "\\r"; break;
		case '\n':
			OS << "\\n"; break;
		case '\\':
			OS << "\\\\"; break;
		default:
			OS << c;
		}
	}
	OS << '\"';
}


static std::string to_hex(const void* ptr) {
	std::ostringstream buf;
	buf << ptr;
	return buf.str();
}



static void escape(std::ostream& OS, const std::string& name, void *val) {
	OS << ' ' << name << '=';
	if (val)
		OS << val;
	else
		OS << "0x0";
}


static void escape(std::ostream& OS, const std::string& name, int val) {
	OS << ' ' << name << '=' << val;
}


static const char* getLogpath() {
	static const char* logpath = nullptr;
	if (!logpath)
		logpath = getenv("ISLTRACE_OUTFILE");
		if (!logpath)
			logpath = "isltrace.log";
	return logpath;
}


static std::ofstream &getLogfile() {
	static std::ofstream *logfile;

	if (!logfile) {
		auto logpath = getLogpath();
		logfile = new std::ofstream(logpath, std::ios_base::out);
		typedef const char *(*VerFuncTy)(void);
		auto verfunc = (VerFuncTy) dlsym(openlibisl(), "isl_version");
		auto ver = (*verfunc)();
		std::string verstr = ver;
		rtrim(verstr);
		*logfile << "isltrace";
		escape(*logfile, "gentime", verstr);

		//*logfile << ""
		//*mainfile << "  const char *genIslVersion = \"" << verstr << "\";\n";
		//*mainfile << "  printf(\"### ISL version: %s (%s at generation)\\n\",  dynIslVersion, genIslVersion);\n\n";
	}
	return *logfile;
}


static std::ostream& openLogfile() {
	auto& logfile = getLogfile();
	if (!logfile.is_open()) {
		auto logpath = getLogpath();
		logfile.open(logpath,std::ios_base::in | std::ios_base::out | std::ios_base::ate);
	}
	return logfile;
}

static void flushLogfile() {
	auto& logfile = getLogfile();
	logfile.close();
}


static bool &getIsInternal() {
	static bool IsInternal = false;
	return IsInternal;
}


#if 0
struct Level {
	bool IsMain = false;
	bool IsInternal = false;
	bool IsCallback = false;

	static Level *createMain() {
		Level* Result = new Level();
		Result->IsMain = true;

#if 0
		Result->mainpath = getenv("ISLTRACE_OUTFILE");
		if (!Result->mainpath)
			Result->mainpath = "isltrace.log";

		//Result->varpath = "isltrace_vars.inc";
#endif

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


#if 0
	const char* mainpath =nullptr;
	const char* varpath = nullptr;
	std::ofstream* mainfile;
	std::ofstream* varfile;

	std::ostream& getVarfile() {
		assert(IsMain);
		if (!varfile){
			varfile = new std::ofstream(varpath,std:: ios_base::out);
			*varfile << "// Global variable declarations\n";
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


	void flush();
#endif
};
#endif



#if 0
static std::vector<Level*> &getLevelStack() {
	static std::vector<Level*> stack;
	if (stack.empty())
		stack.push_back(Level::createMain());
	return stack;
}

static std::ostream& getVarfile() {
	return getLevelStack().front()->getVarfile();
}


void Level::flush() {
	if (IsMain) {
		// Need to completely close (and re-opened when needed again) the file because fflush does not ensure all data written so far will be in the file if the program crashes.
		mainfile->close();
		assert(!mainfile->is_open());
	}		else {
		getLevelStack().front()->flush();
	}

	if (varfile) {
		assert(IsMain);
		varfile->close();
		assert(!varfile->is_open());
	}
}
#endif

#if 0
static Level* getTopmostLevel() {
	return getLevelStack().back();
}
#endif

static void pushInternalLevel() {
	auto& IsInternal = getIsInternal();
	assert(!IsInternal);
	IsInternal = true;
	//getLevelStack().push_back(Level::createInternal());
}

static void popInternalLevel() {
	auto& IsInternal = getIsInternal();
	assert(IsInternal);
	IsInternal = false;
	//assert(getTopmostLevel()->IsInternal);
	//getLevelStack().pop_back();
}


static void pushCallbackLevel(CallbackBase*Cb) {
	auto& IsInternal = getIsInternal();
	assert(IsInternal);
	IsInternal = false;
	//assert(getTopmostLevel()->IsInternal);
	//getLevelStack().push_back(Level::createCallback(Cb));
}

#if 0
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
#endif


static void popCallbackLevel() {
	auto& IsInternal = getIsInternal();
	assert(!IsInternal);
	IsInternal = true;
	//assert(getTopmostLevel()->IsCallback);
	//writeCallbackFileOutput();
	//getLevelStack().pop_back();
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
		OS << "uint64_t " << chunkvname << "[] = {";
		bool First = true;
		for (int i = 0; i < n; i += 1) {
			if (!First)
				OS << ", ";
			OS << "0x" << std::hex << b[i] << std::dec;
			First = false;
		}
		OS << "};";
	} else if (size == 4) {
			const uint32_t* b = (const uint32_t*)p;
			OS << "uint32_t " << chunkvname << "[] = {";
			bool First = true;
			for (int i = 0; i < n; i += 1) {
				if (!First)
					OS << ", ";
				OS << "0x" << std::hex << b[i] << std::dec;
				First = false;
			}
			OS << "};";
	} else if (size == 2) {
		const uint16_t* b = (const uint16_t*)p;
		OS << "uint16_t " << chunkvname << "[] = {";
		bool First = true;
		for (int i = 0; i < n; i += 1) {
			if (!First)
				OS << ", ";
			OS << "0x" << std::hex << b[i] << std::dec;
			First = false;
		}
		OS << "};";
	} else {
		auto c = n * size;
		const char* b = (const char*)p;
		OS << "uint8_t " << chunkvname << "[] = {";
		bool First = true;
		for (int i = 0; i < c; i += 1) {
			if (!First)
				OS << ", ";
			OS << "0x" << std::hex << (int)b[i] << std::dec;
			First = false;
		}
		OS << "};";
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
static void trace_register_result(const std::string& retname, T* retval) {
	auto &map = getResultMap<T>();
	map[retval] = retname;
}


template<typename T>
static void trace_register_result_uniquify(const std::string& retname, T* &retval) {
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
	static void registerArgs(std::ostream& OS,std::ostream& OSvars, int i, T &t, Rest&... r) {
#if 0
		std::string paramstr{ param };
		auto loc = paramstr.find_first_of(',');
		const char *next;
		std::string self;
		if (loc != string::npos) {
			self = paramstr.substr(0, loc);
			next = param + loc+1;
		} else {
			self = paramstr;
			next = "";
		}
#endif

		if constexpr (std::is_pointer_v<T>) {
			// We could uniquify __isl_take parameters, but not __isl_keep parameters (cow removing a reference is indistinguishable from freeing one).
			// Unifortunately, clang does not remember annotations for the function types of callbacks.
			std::string parmname = trace_new_result("p");
			OSvars << "void *" << parmname << ";\n";
			OS << parmname << " = param" << i ;
			trace_register_result_uniquify(parmname, t);
		}
		Argsregister< Rest...>().registerArgs(i+1,r...);
	}
};



#if 0
template< typename RetTy, typename ...ParamTy >
RetTy Callbacker<RetTy(ParamTy...)>::callback(ParamTy... Params) {
	// Calls isl functions (*_dup), so do it in an internal stack context.
	// TODO: actually, use generator::callback_takes_argument
	//Argsregister<ParamTy...>::registerArgs(OS1, OSvars, 0, Params...);


	std::tuple<ParamTy...> args{Params...};
	auto* user = std::get<N>(args);
	Callbacker* Cb = reinterpret_cast<Callbacker*>(const_cast<void*>(user));
	Cb->newInvocation();
	pushCallbackLevel(Cb);
	Level* lvl = getTopmostLevel();
	std::get<N>(args) = Cb->origUser;

	std::ostream& OS1 = lvl->getOutput();
	std::ostream& OSvars = getVarfile();

	if constexpr (!std::is_void<RetTy>()) {
		RetTy result = std::apply(*Cb->origCb, args); // (*Cb->origCb)(Params...);

		std::ostream& OS = lvl->getOutput();
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
#endif




template<typename CallTy>
static CallTy*getsym(const char *fname) {
	void* handle = openlibisl();
	auto* orig = (CallTy*)dlsym(handle, fname);
	fprintf(stderr, "Address of %s: %p\n", fname, orig);
	assert(orig);
	return orig;
}








template<typename CallbackTy, typename VoidTy>
static void trace_callback(std::ostream &OS, const std::string &cbname, const char* cbreturnty, const char*cbparamtys,  CallbackTy *&origCb, CallbackTy *replCb,VoidTy* &user) {
	// VoidTy might be 'void' or 'const void'

	auto& list = getCallbackerList();
	auto callbacker = new Callbacker<CallbackTy>(cbname, origCb, replCb,cbreturnty, cbparamtys, (void*)user);
	list.push_back(callbacker);

	//OS << "  CallbackStruct " << username << "{";
	//trace_arg(OS, user);
	//OS << "}";

	// replace
	origCb = replCb;
	user = callbacker;
}






template<int N, typename CallbackTy>
static void trace_callback(std::ostream& OS, const std::string& cbname, const char* cbreturnty, const char* cbparamtys, CallbackTy*& Cb){
void* dummy;
	trace_callback<N,CallbackTy>(OS, cbname, cbreturnty, cbparamtys, Cb,dummy);
}










#if 0
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
#endif



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
	static void printResult(std::ostream& OS, ObjT obj) {}
};

template<>
struct ResultObjPrinter<int> {
	static void printResult(std::ostream& OS, int val) {
		OS << val;
	}
};

template<>
struct ResultObjPrinter<unsigned> {
	static void printResult(std::ostream& OS, unsigned  val) {
		OS << val;
	}
};

template<>
struct ResultObjPrinter<long> {
	static void printResult(std::ostream& OS, long  val) {
		OS << val;
	}
};

template<>
struct ResultObjPrinter<unsigned long> {
	static void printResult(std::ostream& OS, unsigned long val) {
		OS << val;
	}
};

template<>
struct ResultObjPrinter<char*> {
	static void printResult(std::ostream& OS, char* val) {
		OS << "\"" << val << "\"";
	}
};

template<>
struct ResultObjPrinter<const char*> {
	static void printResult(std::ostream& OS,const char* val) {
		OS << "\"" << val << "\"";
	}
};

template<>
struct ResultObjPrinter<isl_stat> {
	static void printResult(std::ostream& OS,isl_stat val) {
		Argprinter<isl_stat>::print(OS,val);
	}
};

template<>
struct ResultObjPrinter<isl_bool> {
	static void printResult(std::ostream& OS,isl_bool val) {
		Argprinter<isl_bool>::print(OS,val);
	}
};

template<>
struct ResultObjPrinter<isl_dim_type> {
	static void printResult(std::ostream& OS,isl_dim_type val) {
		Argprinter<isl_dim_type>::print(OS,val);
	}
};

template<>
struct ResultObjPrinter<isl_constraint*> {
	static void printResult(std::ostream& OS, isl_constraint* obj) {
		auto p = isl_printer_to_str(isl_constraint_get_ctx(obj));
		p = isl_printer_print_constraint(p, obj);
		auto c=	isl_printer_get_str(p);
		OS << c;
		free(c);
		isl_printer_free(p);
	}
};

template<>
struct ResultObjPrinter<isl_basic_map*> {
	static void printResult(std::ostream& OS, isl_basic_map* obj) {
		auto p = isl_printer_to_str(isl_basic_map_get_ctx(obj));
		p = isl_printer_print_basic_map(p, obj);
		auto c=	isl_printer_get_str(p);
		OS << c;
		free(c);
		isl_printer_free(p);
	}
};

template<>
struct ResultObjPrinter<isl_map*> {
	static void printResult(std::ostream& OS, isl_map* obj) {
		auto p = isl_printer_to_str(isl_map_get_ctx(obj));
		p = isl_printer_print_map(p, obj);
		auto c=	isl_printer_get_str(p);
		OS << c;
		free(c);
		isl_printer_free(p);
	}
};

template<>
struct ResultObjPrinter<isl_union_map*> {
	static void printResult(std::ostream& OS, isl_union_map* obj) {
		auto p = isl_printer_to_str(isl_union_map_get_ctx(obj));
		p = isl_printer_print_union_map(p, obj);
		auto c=	isl_printer_get_str(p);
		OS << c;
		free(c);
		isl_printer_free(p);
	}
};

template<>
struct ResultObjPrinter<isl_basic_set*> {
	static void printResult(std::ostream& OS, isl_basic_set* obj) {
		auto p = isl_printer_to_str(isl_basic_set_get_ctx(obj));
		p = isl_printer_print_basic_set(p, obj);
		auto c=	isl_printer_get_str(p);
		OS << c;
		free(c);
		isl_printer_free(p);
	}
};

template<>
struct ResultObjPrinter<isl_set*> {
	static void printResult(std::ostream& OS, isl_set* obj) {
		auto p = isl_printer_to_str(isl_set_get_ctx(obj));
		p = isl_printer_print_set(p, obj);
		auto c=	isl_printer_get_str(p);
		OS << c;
		free(c);
		isl_printer_free(p);
	}
};

template<>
struct ResultObjPrinter<isl_union_set*> {
	static void printResult(std::ostream& OS, isl_union_set* obj) {
		auto p = isl_printer_to_str(isl_union_set_get_ctx(obj));
		p = isl_printer_print_union_set(p, obj);
		auto c=	isl_printer_get_str(p);
		OS << c;
		free(c);
		isl_printer_free(p);
	}
};

template<>
struct ResultObjPrinter<isl_aff*> {
	static void printResult(std::ostream& OS, isl_aff* obj) {
		auto p = isl_printer_to_str(isl_aff_get_ctx(obj));
		p = isl_printer_print_aff(p, obj);
		auto c=	isl_printer_get_str(p);
		OS << c;
		free(c);
		isl_printer_free(p);
	}
};

template<>
struct ResultObjPrinter<isl_pw_aff*> {
	static void printResult(std::ostream& OS, isl_pw_aff* obj) {
		auto p = isl_printer_to_str(isl_pw_aff_get_ctx(obj));
		p = isl_printer_print_pw_aff(p, obj);
		auto c=	isl_printer_get_str(p);
		OS << c;
		free(c);
		isl_printer_free(p);
	}
};


template<>
struct ResultObjPrinter<isl_space*> {
	static void printResult(std::ostream& OS, isl_space* obj) {
		auto p = isl_printer_to_str(isl_space_get_ctx(obj));
		p = isl_printer_print_space(p, obj);
		auto c=	isl_printer_get_str(p);
		OS << c;
		free(c);
		isl_printer_free(p);
	}
};

template<>
struct ResultObjPrinter<isl_val*> {
	static void printResult(std::ostream& OS, isl_val* obj) {
		auto p = isl_printer_to_str(isl_val_get_ctx(obj));
		p = isl_printer_print_val(p, obj);
		auto c=	isl_printer_get_str(p);
		OS << c;
		free(c);
		isl_printer_free(p);
	}
};


template<>
struct ResultObjPrinter<isl_id*> {
	static void printResult(std::ostream& OS, isl_id* obj) {
		auto p = isl_printer_to_str(isl_id_get_ctx(obj));
		p = isl_printer_print_id(p, obj);
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




template<typename T>
struct PredefArg {
static	bool predef(std::string& parmname, std::string& parmty, std::string& valArg, std::string& ptrArg,  T &val) { return false; }
};


#if 0
template<>
struct PredefArg<char**> {
static	bool predef(std::string& parmname, std::string& parmty, std::string& valArg, std::string& ptrArg, char** &val) {
		auto& OS = openLogfile();

		OS << "\npredef";
		escape(OS, "type", parmty);

		std::ostringstream buf;
		buf << "char* %name%[] = {";
		auto t = val;
		bool First = true;
		while (t[0]) {
			if (!First)
				buf << ", ";
			buf <<  "\"" << t[0] << "\""; // TODO: escape
			++t;
			First = false;
		}
		buf << "}";
		escape(OS, "code", buf.str());
		return true;
	}
};
#endif



namespace {

struct IslCallBase {
	const char* name;
	const char* rettystr;
	std::vector<std::string> argsParmname;
	std::vector<std::string> argsTystr;
	std::vector<std::string> valArgs;
	std::vector<std::string> ptrArgs;
	std::vector<CallbackBase*> cbArgs;
	//std::vector<std::string> predefArgs;

	explicit IslCallBase(const char* name, const char* rettystr, int nargs) : rettystr{rettystr}, name{name}  {
		argsParmname.resize(nargs);
		argsTystr.resize(nargs);
		valArgs.resize(nargs);
		ptrArgs.resize(nargs);
	  // predefArgs.resize(nargs);
		cbArgs.resize(nargs);
		pushInternalLevel();
	}

	~IslCallBase() {
		popInternalLevel();
	}

	int getNumArgs() {
		return (int)valArgs.size();
	}

	void beforeCall(const char *cmd) {
		auto &OS = openLogfile();
		OS << "\n" << cmd;
		escape(OS, "fname", name);
		if (rettystr)
			escape(OS, "rettype", rettystr);
		escape(OS, "numargs", getNumArgs());
		for (int i = 0; i < getNumArgs(); i += 1) {
			escape(OS, "parm" + std::to_string(i) + "name", argsParmname[i]);
			escape(OS, "parm" + std::to_string(i) + "type", argsTystr[i]);
			//if (!predefArgs[i].empty()) {
			//	escape(OS, "parm" + std::to_string(i) + "predef", predefArgs[i]);
			//}
			if (cbArgs[i]) {
				escape(OS, "parm" + std::to_string(i) + "cb", cbArgs[i]);
			} else if (!valArgs[i].empty()) {
				escape(OS, "parm" + std::to_string(i) + "val", valArgs[i]);
			} else if (!ptrArgs[i].empty()) {
				escape(OS, "parm" +std:: to_string(i) + "ptr", ptrArgs[i]);
			}
		}
		flushLogfile();
	}
};





template<typename T>
struct IslCallImpl;


template<typename RetTy, typename... ParmTy>
struct IslCallImpl<RetTy(ParmTy...)> : public IslCallBase {
	using FuncTy = RetTy(ParmTy...);

	FuncTy* orig;
	std::tuple<ParmTy...> realArgs;
	std::tuple<ParmTy...> args;

	explicit IslCallImpl(const char* name, const char* rettystr, FuncTy* orig) : IslCallBase(name, rettystr, sizeof...(ParmTy)), orig{orig} {}

	template<int N, typename ArgT>
	void trace_arg(const char *parmname, const char *parmtystr, ArgT arg) {
		argsParmname[N] = parmname;
		argsTystr[N] = parmtystr;

		std::get<N>(realArgs) = arg;
		if (PredefArg<ArgT>::predef(argsParmname[N], argsTystr[N], valArgs[N], ptrArgs[N], arg)) {
			// has been handled
		} else if constexpr (std::is_pointer_v<ArgT> && !std::is_same_v<ArgT,char*>&& !std::is_same_v<ArgT,const char*>) {
			using PointerT = std::remove_pointer_t<ArgT>;
			//arg = ObjCow<PointerT>::cow(arg);

			std::ostringstream OS;
			OS << arg;
			//auto uniqued = ObjCow<PointerT>::cow(arg);
			ptrArgs[N] = OS.str();
		} else {
			std::ostringstream OS;
			Argprinter<ArgT>::print(OS, arg);
			valArgs[N] = OS.str();
		}
		std::get<N>(args) = arg;
	}

	template<int ArgcIdx, int ArgvIdx>
	void trace_argv(const char *argvname, const char *argvtype, int argc, char *argv[]) {
		argsParmname[ArgvIdx] = argvname;
		argsTystr[ArgvIdx] = argvtype;

		auto name = trace_new_result("argv");
		auto& OS = openLogfile();
		OS << "\npredef";
		escape(OS, "name", name);
		escape(OS, "ty", argvtype);
		escape(OS, "ptr", argv);
		std::ostringstream buf;
		buf << "char* " << name << "[] = { ";
		for (int i = 0; i < argc; i += 1) {
			if (i)
				buf << ", ";
			buf << "\"" << argv[i] << "\""; // TODO: escape
		}
		buf << " };";
		escape(OS, "code", buf.str());
		ptrArgs[ArgvIdx] = to_hex(argv);
		valArgs[ArgcIdx] = std::to_string(argc);

		std::get<ArgcIdx>(args) = argc;
		std::get<ArgcIdx>(realArgs) = argc;

		std::get<ArgvIdx>(args) = argv;
		std::get<ArgvIdx>(realArgs) = argv;
	}

	template<int CountIdx, int SizeIdx, int ChunksIdx>
	void trace_chunks(const char* countname, const char* sizename, const char* chunksname,
		const char* countty, const char* sizety, const char* chunkty,
		size_t count, size_t size, const void* chunks) {
		auto name = trace_new_result("chunks");

		auto& OS = openLogfile();
		OS << "\npredef";
		escape(OS, "name", name);
		escape(OS, "ty", chunkty);
		escape(OS, "ptr", (void*)chunks);
		std::ostringstream buf;
		trace_pre_chunks(buf, name, count, size, chunks);
		escape(OS, "code", buf.str());
		valArgs[CountIdx] = std::to_string(count);
		valArgs[SizeIdx] = std::to_string(size);
		ptrArgs[ChunksIdx] = to_hex(chunks);

		std::get<CountIdx>(args) = count;
		std::get<SizeIdx>(args) = size;
		std::get<ChunksIdx>(args) = chunks;
		argsTystr[CountIdx] = countty;
		argsTystr[SizeIdx] = sizety;
		argsTystr[ChunksIdx] = chunkty;
		argsParmname[CountIdx] = countname;
		argsParmname[SizeIdx] = sizename;
		argsParmname[ChunksIdx] = chunksname;
	}

	template<int CbIdx, int UserIdx, typename CallbackT, typename VoidT>
	void trace_cb(const char *fname, const char* parmname, const char* username, const char*cbty, const char *userty, const char *retty, const char* paramtys, CallbackT *origFn, CallbackT *replFn, VoidT *origUser) {
		auto* callbacker = new Callbacker<CallbackT>(fname, parmname, origFn, replFn, retty, paramtys, (void*)origUser);

		std::get<CbIdx>(args) = replFn;
		std::get<CbIdx>(realArgs) = origFn;
		argsParmname[CbIdx] = parmname;
		argsTystr[CbIdx] = cbty;
		//ptrArgs[CbIdx] =(void*) replFn;
		//valArgs[CbIdx] = std::string{ "&" }  + fname;
		//ptrArgs[CbIdx] = to_hex(callbacker);
		cbArgs[CbIdx] = callbacker;

	//	static_assert(UserIdx >= 0);
		if constexpr (UserIdx >= 0) {
			std::get<UserIdx>(args) = callbacker;
			std::get<UserIdx>(realArgs) = origUser;
			argsParmname[UserIdx] = username;
			argsTystr[UserIdx] = userty;
			valArgs[UserIdx] = std::string("(") + userty + ")" + to_hex(origUser);
		}

		auto& OS = openLogfile();
		OS << "\ncallback";
		escape(OS,"callbacker",(void*)callbacker);
		escape(OS,"fname",fname);
		escape(OS,"pname",parmname);
		escape(OS,"retty",retty);
		escape(OS,"numargs", callbacker->getNumArgs());
		escape(OS,"paramtys",paramtys);
	}
};









template<typename T>
struct IslCall;

template<typename... ParmTy>
struct IslCall<void(ParmTy...)>: public IslCallImpl<void(ParmTy...)> {
	using FuncTy = void(ParmTy...);
	IslCallImpl<FuncTy>& getBase() { return *this;  }

	explicit IslCall(const char* name, const char *tystr, FuncTy* orig) : IslCallImpl<void(ParmTy...)>(name,tystr,orig) {}

	virtual void execOrig() {
		std::apply(getBase().orig, getBase().args);
	}

	void apply() {
		getBase().beforeCall("call_proc");
		execOrig();
	}
};





template<typename RetTy, typename... ParmTy>
struct IslCall<RetTy(ParmTy...)> : public IslCallImpl<RetTy(ParmTy...)> {
	using FuncTy = RetTy(ParmTy...);
	IslCallImpl<FuncTy>& getBase() { return *this;  }

	explicit IslCall(const char* name, const char *tystr, FuncTy* orig) : IslCallImpl<RetTy(ParmTy...)>(name,tystr,orig) {}

	virtual RetTy execOrig()  {
		return std::apply(getBase().orig, getBase().args);
	}

	RetTy apply() {
	  getBase().beforeCall("call_func");
		RetTy retval = execOrig();

		if constexpr (std::is_pointer_v<RetTy>) {
			using PointerT = std::remove_pointer_t<RetTy>;
			retval = ObjCow<PointerT>::cow(retval);
		}

			auto& OS = openLogfile();
			OS << "\nreturn";
			escape(OS, "fname", getBase().name);
			escape(OS, "rettype", getBase().rettystr);
			if constexpr (std::is_pointer_v<RetTy> && !std::is_same_v<RetTy,char*>&& !std::is_same_v<RetTy,const char*>) {
				escape(OS, "retptr", retval);
			}	else {
				std::ostringstream kOS;
				Argprinter<RetTy>::print(kOS, retval);
				escape(OS, "retval", kOS.str());
			}

			//if constexpr (std::is_pointer_v<RetTy>  && !std::is_same_v<RetTy,char*>&& !std::is_same_v<RetTy,const char*>) {
			//	using PointerT = std::remove_pointer_t<RetTy>;
				std::ostringstream dOS;
				ResultObjPrinter<RetTy>::printResult(dOS, retval);
				auto desc = dOS.str();
				if (!desc.empty())
					escape(OS, "desc", desc);
			//}

			return retval;
	}
};



template<typename RetTy, typename... ParmTy>
struct IslIdAllocCall;

template<>
struct IslIdAllocCall<isl_id *(isl_ctx *, const char *, void *)> : public IslCall<isl_id *(isl_ctx *, const char *, void *)> {
	typedef   void (CallbackT)(void*);
    using FuncTy = isl_id *(isl_ctx *, const char *, void *);
    using BaseTy = IslCall<FuncTy>;
		using RetTy = isl_id*;
		BaseTy& getBase() { return *this;  }

    explicit IslIdAllocCall(const char* name, const char* rettystr, FuncTy* orig) : IslCall(name, rettystr, orig) {}


		RetTy execOrig() override {
			// Wrap the user pointer into a dummy callbacker (should never be called, requires isl_id_set_free_user to be called first)
			auto Id = std::get<0>(getBase().args);
			auto Name = std::get<1>(getBase().args);
			auto origUser = std::get<2>(getBase().args);
			if (origUser) {
				auto DummyCallbacker = new Callbacker<CallbackT>("isl_id_alloc", "free_func", NULL, NULL, "void*", "void*", origUser);
				auto CallbackerRef = new decltype(DummyCallbacker)[1];
				CallbackerRef[0] = DummyCallbacker;
				assert(CallbackerRef[0]);  assert(&CallbackerRef[0]);  assert(CallbackerRef[0]->origUser == origUser);
				origUser = &CallbackerRef[0];
			}
			return getBase().orig(Id,Name,origUser);
		}
};




//extern Callbacker<void (*)(void*)> pp;




template<typename RetTy, typename... ParmTy>
struct IslIdGetUserCall;

template<>
struct IslIdGetUserCall<void *(isl_id *)> : public IslCall<void *(isl_id *)> {
	typedef   void (CallbackT)(void*);
    using FuncTy = void *(isl_id *);
		using RetTy = void *;
    explicit IslIdGetUserCall(const char* name, const char* rettystr, FuncTy* orig) : IslCall(name, rettystr, orig) {}

		Callbacker<CallbackT>**getCallbacker() {
			auto Id = std::get<0>(getBase().args);
			using GetUserFuncTy = void *(isl_id *);
			static GetUserFuncTy* origGetUser = getsym<GetUserFuncTy>("isl_id_get_user");
			Callbacker<CallbackT>** refCallbacker = (Callbacker<CallbackT>**)(*origGetUser)(Id);
			return refCallbacker;
		}

		RetTy execOrig() override {
			auto refCallbacker = getCallbacker();
			if (!refCallbacker)
				return nullptr;
			return (*refCallbacker)->origUser;
		}
};





template<typename RetTy, typename... ParmTy>
struct IslIdSetFreeUserCall;

template<>
struct IslIdSetFreeUserCall<isl_id *(isl_id *, void (*)(void *))> : public IslCall<isl_id *(isl_id *, void (*)(void *))> {
	typedef   void (CallbackT)(void*);
    using FuncTy = isl_id *(isl_id *,CallbackT);
		using RetTy = isl_id *;
		using BaseTy = IslCall<isl_id* (isl_id*, void (*)(void*))>;

    explicit IslIdSetFreeUserCall(const char* name, const char* rettystr, FuncTy* orig) : IslCall(name, rettystr, orig) {}


		Callbacker<CallbackT>* *getCallbacker() {
			auto Id = std::get<0>(getBase().args);
			using GetUserFuncTy = void *(isl_id *);
			static auto origGetUser = getsym<GetUserFuncTy>("isl_id_get_user");
			Callbacker<CallbackT>** refCallbacker = (Callbacker<CallbackT>**)(*origGetUser)(Id);
			return refCallbacker;
		}

		template<int CbIdx, int UserIdx, typename CallbackT, typename VoidT>
		void trace_cb(const char *fname, const char* parmname, const char* username, const char*cbty, const char *userty, const char *retty, const char* paramtys, CallbackT *origFn, CallbackT *replFn, VoidT *origUser) {
			static_assert(CbIdx == 1);	static_assert(UserIdx == -1); assert(origUser==NULL);

			// Get the origUser if any
			auto refCallbacker = getCallbacker();
			if (refCallbacker)
				origUser = (*refCallbacker)->origUser;

			return getBase().trace_cb<CbIdx, UserIdx>(fname,parmname,username,cbty,userty,retty,paramtys,origFn,replFn,origUser);
		}

		RetTy execOrig() override {
			Callbacker<CallbackT>* newCallbacker = (Callbacker<CallbackT>*)cbArgs[1];

			// In addition executing the original function, update the CallbackerRef
			// If there is no callbacker, the user pointer must have been NULL and there is nothing to free
			auto refCallbacker = getCallbacker();
			if (!refCallbacker)
				return std::get<0>(getBase().args);
			(*refCallbacker) = newCallbacker;

			return BaseTy::execOrig();
		 }
};











struct CbCallBase {
	CallbackBase* callbacker;
	std::vector<std::string> ptrArgs;

	explicit CbCallBase(CallbackBase* callbacker, int numargs) : callbacker{callbacker} {
		ptrArgs.resize(numargs);
		pushCallbackLevel(callbacker);

	}
	~CbCallBase() {
		popCallbackLevel();
	}
};




template<int UserIdx, typename FuncTy>
struct CbCallImpl;

template<int UserIdx, typename RetTy, typename... ParmTy>
struct CbCallImpl<UserIdx, RetTy(ParmTy...)> : public CbCallBase {
	using FuncTy = RetTy(ParmTy...);
	CbCallBase& getBase() { return *this;  }

	Callbacker<FuncTy>* callbacker;
	std::tuple<ParmTy...> args;

	explicit CbCallImpl(Callbacker<FuncTy>* callbacker) : CbCallBase(callbacker, sizeof...(ParmTy)),callbacker{callbacker} {
#if 0
    auto& OS = openLogfile();
    OS << "\ncbcall_created";
	  escape(OS, "callbacker", (void*)callbacker);
		flushLogfile();
#endif
    	}

	template<int N, typename ArgT>
	void trace_arg(ArgT arg) {
		if constexpr (std::is_pointer_v<ArgT> && !std::is_same_v<ArgT,char*>&& !std::is_same_v<ArgT,const char*>) {
			using PointerT = std::remove_pointer_t<ArgT>;
			std::ostringstream OS;
			OS << arg;
			ptrArgs[N] = OS.str();
		} else {
			// not interesting
		}
		std::get<N>(args) = arg;
	}

	void beforeApply() {
		std::get<UserIdx>(args) = callbacker->origUser;
		auto& OS = openLogfile();
		OS << "\ncallback_enter";
	    escape(OS, "callbacker", (void*)callbacker);
		escape(OS, "numargs", sizeof...(ParmTy));
		for (int i = 0; i < sizeof...(ParmTy); i += 1) {
			if (!ptrArgs[i].empty())
				escape(OS, "arg" + std::to_string(i) + "ptr", ptrArgs[i]);
		}
		flushLogfile();
	}
};





template<int UserIdx, typename FuncTy>
struct CbCall;

template<int UserIdx, typename... ParmTy>
struct CbCall<UserIdx, void(ParmTy...)> : public CbCallImpl<UserIdx, void(ParmTy...)>  {
	using FuncTy = void(ParmTy...);
	CbCallImpl<UserIdx, void(ParmTy...)>& getBase() { return *this;  }

	 CbCall(Callbacker<FuncTy>* callbacker) : CbCallImpl<UserIdx, void(ParmTy...)>(callbacker) {}

	void apply() {
		getBase().beforeApply();
		std::apply(getBase().callbacker->origCb,getBase().args);
		{
			auto& OS = openLogfile();
			OS << "\ncallback_exit";
			escape(OS, "callbacker", (void*)getBase().callbacker);
			flushLogfile();
		}
	}
};


template<int UserIdx, typename RetTy, typename... ParmTy>
struct CbCall<UserIdx, RetTy(ParmTy...)> : public CbCallImpl<UserIdx, RetTy(ParmTy...)>  {
	using FuncTy = RetTy(ParmTy...);
	CbCallImpl<UserIdx, RetTy(ParmTy...)>& getBase() { return *this;  }

	 CbCall(Callbacker<FuncTy>* callbacker) : CbCallImpl<UserIdx, RetTy(ParmTy...)>(callbacker) {}

	RetTy apply() {
		getBase().	beforeApply();
		auto retval = std::apply(getBase().callbacker->origCb,getBase(). args);

		if constexpr (std::is_pointer_v<RetTy>) {
			using PointerT = std::remove_pointer_t<RetTy>;
			retval = ObjCow<PointerT>::cow(retval);
		}


			auto& OS = openLogfile();
			OS << "\ncallback_exit";
			escape(OS, "callbacker", (void*)getBase().callbacker);
			escape(OS, "retty", getBase().callbacker->cbretty);
			if constexpr (std::is_pointer_v<RetTy> && !std::is_same_v<RetTy, char*> && !std::is_same_v<RetTy, const char*>) {
				using PointerT = std::remove_pointer_t<RetTy>;
				escape(OS, "retptr", (void*)retval);
			}	else {
				std::ostringstream buf;
				Argprinter<RetTy>::print(buf, retval);
				escape(OS, "retval" , buf.str() );
			}
			flushLogfile();

		return retval;
	}
};

}




#include "libtrace.inc.cpp"








