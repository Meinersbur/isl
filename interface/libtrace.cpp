
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
#include <dlfcn.h>
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


namespace {
	template<typename T>
	struct ObjPrinter {
		static std::unordered_map<T*, int>& getTheMap() {
			static std::unordered_map<T*, int> Map;
			return Map;
		}

		static void registerResult(T* t, int c) {
			getTheMap()[t] = c;
		}

		void print(std::ostream &OS,const char* SName, T* t) {
			if (t == nullptr) {
				OS << "NULL";
				return;
			}

			auto& Map = getTheMap();
			auto It = Map.find(t);
			if (It == Map.end()) {
				OS << " " << SName << ")" << t;
				return;
			}


			auto num = It->second;
			OS << ObjShortname<T>::name << num;
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
void print(std::ostream &OS, sname* t) {                 \
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
ISL_STRUCT(isl_aff, aff)
ISL_STRUCT(isl_aff_list, afflist)
ISL_STRUCT(isl_ast_build, astbuild)
ISL_STRUCT(isl_ast_expr, astexpr)
ISL_STRUCT(isl_ast_node, astnode)
ISL_STRUCT(isl_ast_node_list, nodelist)
ISL_STRUCT(isl_basic_map, bmap)
ISL_STRUCT(isl_basic_set, bset)
ISL_STRUCT(isl_fixed_box, fixedbox)
ISL_STRUCT(isl_id, id)
ISL_STRUCT(isl_id_list, idlist)
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
ISL_STRUCT(isl_schedule, sched)
ISL_STRUCT(isl_schedule_constraints, schedconstraints)
ISL_STRUCT(isl_schedule_node, schednode)
ISL_STRUCT(isl_space, space)
ISL_STRUCT(isl_union_access_info, uaccess)
ISL_STRUCT(isl_union_flow, uflow)
ISL_STRUCT(isl_union_map, umap)
ISL_STRUCT(isl_union_pw_aff, upwa)
ISL_STRUCT(isl_union_pw_aff_list, upwafflist)
ISL_STRUCT(isl_union_pw_multi_aff, upwma)
ISL_STRUCT(isl_union_set, uset)
ISL_STRUCT(isl_union_set_list, usetlist)
ISL_STRUCT(isl_val, val)
ISL_STRUCT(isl_val_list, vallist)
ISL_STRUCT(isl_args, args)



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
	void print(std::ostream &OS, long   t) {
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
struct Argprinter<const char*> {
	void print(std::ostream &OS, const char * t) {
		// TODO: escape
		OS << "\n" << t << "\"";
	}
};
template<>
struct ObjShortname<const char*>: public ObjShortnameBase<const char*> {
	static constexpr const char* name = "str";
};



template<typename T>
struct Argprinter<T*> {
	void print(std::ostream &OS, T* t) {
		OS << "(void)" << t;
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
//#include <isl/hmap_templ.c>
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
//#include <isl/maybe_templ.h>
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
#include <isl/val_gmp.h>
#include <isl/val_type.h>
#include <isl/vec.h>
#include <isl/version.h>
#include <isl/vertices.h>

int main() {
)";


#if 0
static FILE* getOutfile() {
	static	const char* path=nullptr;

	FILE* handle;
	if (!path) {
		 path = getenv("ISLTRACE_OUTFILE");
		if (!path)
			path = "libisl.log";
		handle = fopen(path, "w");

		fputs(prologue, handle);
			typedef const char *(*VerFuncTy)(void);
		auto verfunc = (VerFuncTy) dlsym(openlibisl(), "isl_version");
		auto ver = (*verfunc) ();
		fprintf(stderr, "ISL reports version %s\n",ver);
		fprintf(handle, "  // ISL self-reported version: %s\n", ver);

		fprintf(stderr, "Opened file %s\n",path);
	} else {
		handle = fopen(path, "a");
	}
	return handle;
}
#endif



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
	void printArgs(std::ostream &OS, T t) {
		Argprinter<T>().print(f, t);
	}
};


template<typename T, typename S,	typename... Rest>
struct Argsprinter<T,S,Rest...> {
	void printArgs(std::ostream &OS,T t, S s, Rest ...r) {
		Argsprinter<T>().printArgs(f, t);
		fputs(", ", f);
		Argsprinter<S, Rest...>().printArgs(f, s, r...);
	}
};







template<typename... Rest>
static void printArgs(std::ostream &OS, Rest ...r) {
	Argsprinter<Rest...>().printArgs(OS, r...);
}





struct Level {
	bool IsMain = false;
	bool IsInternal = false;

	static Level createMain() {
		Level Result;
		Result.IsMain = true;

		Result.mainpath = getenv("ISLTRACE_OUTFILE");
		if (!Result.mainpath)
			Result.mainpath = "libisl.log";

		return Result;
	}

	static Level createInternal() {
		Level Result;
		Result.IsInternal = true;
		return Result;
	}


	const char* mainpath =nullptr;
	std::ofstream* mainfile;



	std::ostream &getOutput() {
		if (IsMain) {
			if (!mainfile) {
				mainfile = new std::ofstream(mainpath,std:: ios_base::out);
				*mainfile << prologue;
				typedef const char *(*VerFuncTy)(void);
				auto verfunc = (VerFuncTy) dlsym(openlibisl(), "isl_version");
				auto ver = (*verfunc)();
				*mainfile << "  // ISL self-reported version:" << ver << "\n";
			} else if (!mainfile->is_open()) {
				mainfile->open(mainpath, std::ios_base::ate);
			}
			return *mainfile;
		}

		assert(!"No output stream");
	}

	void flush() {
		if (IsMain) {
			// Need to completely close (and re-opened when needed again) the file because fflush does not ensure all data written so far will be in the file if the program crashes.
			mainfile->close();
		}
	}
};




static std::vector<Level> &getLevelStack() {
	std::vector<Level> stack;
	if (stack.empty())
		stack.push_back(Level::createMain());
	return stack;
}


static Level& getLevel() {
	getLevelStack().back();
}




template<typename RetTy, typename FuncTy, typename ...ParamTy>
static RetTy trace(const char *fname, const char *rettyname, void*self, FuncTy *&orig, int dummy, ParamTy... Params) {
	if (!orig) {
		  //fprintf(stderr, "Calling %s for the first time\n", fname);
			void* handle = openlibisl();
			orig = (std::remove_reference_t< decltype(orig)>)dlsym(handle, fname);
			fprintf(stderr, "Address of %s: %p\n", fname, orig);
	}
	assert(orig);
	assert(orig != self);


	auto& lvl = getLevel();
	if (lvl.IsInternal) {
		// fprintf(stderr, "Calling %s during recursion\n", fname);
		return (*orig)(Params...);
	}
//	fprintf(stderr, "Calling %s\n", fname);



	 // Write the line before calling the function to ensure the crashing command appears in the log in case it crashes.
	auto* OS = lvl.getOutput()
		OS << "  ";

		int c;
		if constexpr (!std::is_void<RetTy>()) {
			c = ObjShortname<RetTy>::getNextNum();
			OS << rettyname << " " << ObjShortname<RetTy>::name << c;
		}

		OS << fname << "(";
		printArgs<ParamTy...>(OS, Params...);
		OS << ");\n";
		lvl.flush();


		if constexpr (std::is_void<RetTy>()) {
			(*orig)(Params...);
			exit_recursion();
			return;
		} else {
			auto result = (*orig)(Params...);
			if constexpr (std::is_pointer<RetTy>::value) {
				using BaseTy = typename std::remove_pointer<RetTy>::type;
				result = ObjCow<BaseTy>::cow(result);
				ObjPrinter<BaseTy>::registerResult(result, c);
			}
			exit_recursion();
			return result;
		}
}









#define CALL_ORIG(FNAME, RETTY, PARAMTYS, ...)                         \
  static decltype(FNAME) *orig = nullptr; \
	return trace<RETTY>(STR(FNAME), STR(RETTY), (void*)&FNAME, orig, __VA_ARGS__);

#define CALL_ORIG_VOID(FNAME, RETTY, PARAMTYS, ...)                    \
  static decltype(FNAME) *orig = nullptr; \
	       trace<void >(STR(FNAME), STR(RETTY), (void*)&FNAME, orig, __VA_ARGS__);








#include "libtrace.inc.cpp"



