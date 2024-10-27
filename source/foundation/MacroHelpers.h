#pragma once

//=========================================================================
// https://www.scs.stanford.edu/~dm/blog/va-opt.html
//=========================================================================
#ifndef PARENS
	#define PARENS ()
#endif

#ifndef FOR_EACH
	#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
	#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
	#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
	#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
	#define EXPAND1(...) __VA_ARGS__

	#define FOR_EACH(macro, ...)                                    \
	  __VA_OPT__(EXPAND(FOR_EACH_HELPER(macro, __VA_ARGS__)))

	#define FOR_EACH_HELPER(macro, a1, ...)                         \
	  macro(a1)                                                     \
	  __VA_OPT__(FOR_EACH_AGAIN PARENS (macro, __VA_ARGS__))

	#define FOR_EACH_AGAIN() FOR_EACH_HELPER
#endif

//=========================================================================
// Macro Prefix Variant
//=========================================================================
#ifndef FOR_EACH_MACRO
	#define FOR_EACH_MACRO(macro, ...)                                    \
	  __VA_OPT__(EXPAND(FOR_EACH_MACRO_HELPER(macro, __VA_ARGS__)))

	#define FOR_EACH_MACRO_HELPER(macro, a1, ...)                         \
	  macro##a1                                                           \
	  __VA_OPT__(FOR_EACH_MACRO_AGAIN PARENS (macro, __VA_ARGS__))

	#define FOR_EACH_MACRO_AGAIN() FOR_EACH_MACRO_HELPER
#endif

//=========================================================================
// https://stackoverflow.com/a/77894709
//=========================================================================
#ifndef VALUE_IFNOT
	#define VALUE_IFNOT_TEST(...) __VA_ARGS__
	#define VALUE_IFNOT_TEST0(...) __VA_ARGS__
	#define VALUE_IFNOT_TEST1(...)
	#define VALUE_IFNOT(COND, ...) VALUE_IFNOT_TEST ## COND ( __VA_ARGS__ )
#endif

//=========================================================================