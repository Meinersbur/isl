#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isl_ctx.h"
#include "isl_options.h"

struct isl_arg_choice isl_lp_solver_choice[] = {
	{"tab",		ISL_LP_TAB},
#ifdef ISL_PIPLIB
	{"pip",		ISL_LP_PIP},
#endif
	{0}
};

struct isl_arg_choice isl_ilp_solver_choice[] = {
	{"gbr",		ISL_ILP_GBR},
#ifdef ISL_PIPLIB
	{"pip",		ISL_ILP_PIP},
#endif
	{0}
};

struct isl_arg_choice isl_pip_solver_choice[] = {
	{"tab",		ISL_PIP_TAB},
#ifdef ISL_PIPLIB
	{"pip",		ISL_PIP_PIP},
#endif
	{0}
};

struct isl_arg_choice isl_pip_context_choice[] = {
	{"gbr",		ISL_CONTEXT_GBR},
	{"lexmin",	ISL_CONTEXT_LEXMIN},
	{0}
};

struct isl_arg_choice isl_gbr_choice[] = {
	{"never",	ISL_GBR_NEVER},
	{"once",	ISL_GBR_ONCE},
	{"always",	ISL_GBR_ALWAYS},
	{0}
};

struct isl_arg isl_options_arg[] = {
ISL_ARG_CHOICE(struct isl_options, lp_solver, 0, "lp-solver", \
	isl_lp_solver_choice,	ISL_LP_TAB)
ISL_ARG_CHOICE(struct isl_options, ilp_solver, 0, "ilp-solver", \
	isl_ilp_solver_choice,	ISL_ILP_GBR)
ISL_ARG_CHOICE(struct isl_options, pip, 0, "pip", \
	isl_pip_solver_choice,	ISL_PIP_TAB)
ISL_ARG_CHOICE(struct isl_options, context, 0, "context", \
	isl_pip_context_choice,	ISL_CONTEXT_GBR)
ISL_ARG_CHOICE(struct isl_options, gbr, 0, "gbr", \
	isl_gbr_choice,	ISL_GBR_ONCE)
ISL_ARG_BOOL(struct isl_options, gbr_only_first, 0, "gbr-only-first", 0)
ISL_ARG_END
};

ISL_ARG_DEF(isl_options, struct isl_options, isl_options_arg)
