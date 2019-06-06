#ifdef HAS_TYPE
#define OPT_TYPE_PARAM		, enum isl_fold type
#define OPT_TYPE_ARG(loc)	, loc type
#define OPT_SET_TYPE(loc,val)	loc type = (val);
#else
#define OPT_TYPE_PARAM
#define OPT_TYPE_ARG(loc)
#define OPT_SET_TYPE(loc,val)
#endif
