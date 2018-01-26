/* Copyright 2016-2017 Tobias Grosser
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Tobias Grosser, Weststrasse 47, CH-8003, Zurich
 */

#include <vector>
#include <string>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <isl/options.h>
#include "interface/isl-noexceptions.h"

static void assert_impl(bool condition, const char *file, int line,
	const char *message)
{
	if (condition)
		return;

	fprintf(stderr, "Assertion failed in %s:%d %s\n", file, line, message);
	exit(EXIT_FAILURE);
}

static void assert_impl(isl::boolean condition, const char *file, int line,
	const char *message)
{
	assert_impl(bool(condition), file, line, message);
}

#define assert(exp) assert_impl(exp, __FILE__, __LINE__, #exp)

/* Test the pointer interface for interaction between isl C and C++ types.
 *
 * This tests:
 * - construction from an isl C object
 * - check that constructed objects are non-null
 * - get a non-owned C pointer from an isl C++ object usable in __isl_keep
 *   methods
 * - use copy to get an owned C pointer from an isl C++ object which is usable
 *   in __isl_take methods. Verify that the original C++ object retains a valid
 *   pointer.
 * - use release to get an owned C pointer from an isl C++ object which is
 *   usable in __isl_take methods. Verify that the original C++ object gave up
 *   its pointer and now is null.
 */
void test_pointer(isl::ctx ctx)
{
	isl_set *c_empty = isl_set_read_from_str(ctx.get(), "{ : false }");
	isl::set empty = isl::manage(c_empty);
	assert(empty.is_empty());
	assert(isl_set_is_empty(empty.get()));

	assert(!empty.is_null());
	isl_set_free(empty.copy());
	assert(!empty.is_null());
	isl_set_free(empty.release());
	assert(empty.is_null());
}

/* Test that isl objects can be constructed.
 *
 * This tests:
 *  - construction of a null object
 *  - construction from a string
 *  - construction from an integer
 *  - static constructor without a parameter
 *  - conversion construction (implicit)
 *  - conversion construction (explicit)
 *
 *  The tests to construct from integers and strings cover functionality that
 *  is also tested in the parameter type tests, but here we verify that
 *  multiple overloaded constructors are available and that overload resolution
 *  works as expected.
 *
 *  Construction from an isl C pointer is tested in test_pointer.
 */
void test_constructors(isl::ctx ctx)
{
	isl::val null;
	assert(null.is_null());

	isl::val zero_from_str = isl::val(ctx, "0");
	assert(zero_from_str.is_zero());

	isl::val zero_int_con = isl::val(ctx, 0);
	assert(zero_int_con.is_zero());

	isl::val zero_static_con = isl::val::zero(ctx);
	assert(zero_static_con.is_zero());

	isl::basic_set bs(ctx, "{ [1] }");
	isl::set result(ctx, "{ [1] }");
	isl::set s = bs;
	assert(s.is_equal(result));
	isl::set s2(bs);
	assert(s.unite(s2).is_equal(result));
}

/* Test integer function parameters.
 *
 * Verify that extreme values and zero work.
 */
void test_parameters_int(isl::ctx ctx)
{
	isl::val long_max_str(ctx, std::to_string(LONG_MAX));
	isl::val long_max_int(ctx, LONG_MAX);
	assert(long_max_str.eq(long_max_int));

	isl::val long_min_str(ctx, std::to_string(LONG_MIN));
	isl::val long_min_int(ctx, LONG_MIN);
	assert(long_min_str.eq(long_min_int));

	isl::val long_zero_str = isl::val(ctx, std::to_string(0));
	isl::val long_zero_int = isl::val(ctx, 0);
	assert(long_zero_str.eq(long_zero_int));
}

/* Test isl objects parameters.
 *
 * Verify that isl objects can be passed as lvalue and rvalue parameters.
 * Also verify that isl object parameters are automatically type converted if
 * there is an inheritance relation. Finally, test function calls without
 * any additional parameters, apart from the isl object on which
 * the method is called.
 */
void test_parameters_obj(isl::ctx ctx)
{
	isl::set a(ctx, "{ [0] }");
	isl::set b(ctx, "{ [1] }");
	isl::set c(ctx, "{ [2] }");
	isl::set expected(ctx, "{ [i] : 0 <= i <= 2 }");

	isl::set tmp = a.unite(b);
	isl::set res_lvalue_param = tmp.unite(c);
	assert(res_lvalue_param.is_equal(expected));

	isl::set res_rvalue_param = a.unite(b).unite(c);
	assert(res_rvalue_param.is_equal(expected));

	isl::basic_set a2(ctx, "{ [0] }");
	assert(a.is_equal(a2));

	isl::val two(ctx, 2);
	isl::val half(ctx, "1/2");
	isl::val res_only_this_param = two.inv();
	assert(res_only_this_param.eq(half));
}

/* Test different kinds of parameters to be passed to functions.
 *
 * This includes integer and isl C++ object parameters.
 */
void test_parameters(isl::ctx ctx)
{
	test_parameters_int(ctx);
	test_parameters_obj(ctx);
}

/* Test that isl objects are returned correctly.
 *
 * This only tests that after combining two objects, the result is successfully
 * returned.
 */
void test_return_obj(isl::ctx ctx)
{
	isl::val one(ctx, "1");
	isl::val two(ctx, "2");
	isl::val three(ctx, "3");

	isl::val res = one.add(two);

	assert(res.eq(three));
}

/* Test that integer values are returned correctly.
 */
void test_return_int(isl::ctx ctx)
{
	isl::val one(ctx, "1");
	isl::val neg_one(ctx, "-1");
	isl::val zero(ctx, "0");

	assert(one.sgn() > 0);
	assert(neg_one.sgn() < 0);
	assert(zero.sgn() == 0);
}

/* Test that isl_bool values are returned correctly.
 *
 * We check in detail the following parts of the isl::boolean class:
 *  - The is_true, is_false, and is_error functions return true in case they
 *    are called on a true, false, or error instance of isl::boolean,
 *    respectively
 *  - Explicit conversion to 'bool'
 *  - Implicit conversion to 'bool'
 *  - The complement operator
 *  - Explicit construction from 'true' and 'false'
 *  - Explicit construction form isl_bool
 */
void test_return_bool(isl::ctx ctx)
{
	isl::set empty(ctx, "{ : false }");
	isl::set univ(ctx, "{ : }");
	isl::set null;

	isl::boolean b_true = empty.is_empty();
	isl::boolean b_false = univ.is_empty();
	isl::boolean b_error = null.is_empty();

	assert(b_true.is_true());
	assert(!b_true.is_false());
	assert(!b_true.is_error());

	assert(!b_false.is_true());
	assert(b_false.is_false());
	assert(!b_false.is_error());

	assert(!b_error.is_true());
	assert(!b_error.is_false());
	assert(b_error.is_error());

	assert(bool(b_true) == true);
	assert(bool(b_false) == false);

	assert(b_true);

	assert((!b_false).is_true());
	assert((!b_true).is_false());
	assert((!b_error).is_error());

	assert(isl::boolean(true).is_true());
	assert(!isl::boolean(true).is_false());
	assert(!isl::boolean(true).is_error());

	assert(isl::boolean(false).is_false());
	assert(!isl::boolean(false).is_true());
	assert(!isl::boolean(false).is_error());

	assert(isl::manage(isl_bool_true).is_true());
	assert(!isl::manage(isl_bool_true).is_false());
	assert(!isl::manage(isl_bool_true).is_error());

	assert(isl::manage(isl_bool_false).is_false());
	assert(!isl::manage(isl_bool_false).is_true());
	assert(!isl::manage(isl_bool_false).is_error());

	assert(isl::manage(isl_bool_error).is_error());
	assert(!isl::manage(isl_bool_error).is_true());
	assert(!isl::manage(isl_bool_error).is_false());
}

/* Test that strings are returned correctly.
 * Do so by calling overloaded isl::ast_build::from_expr methods.
 */
void test_return_string(isl::ctx ctx)
{
	isl::set context(ctx, "[n] -> { : }");
	isl::ast_build build = isl::ast_build::from_context(context);
	isl::pw_aff pw_aff(ctx, "[n] -> { [n] }");
	isl::set set(ctx, "[n] -> { : n >= 0 }");

	isl::ast_expr expr = build.expr_from(pw_aff);
	const char *expected_string = "n";
	assert(expected_string == expr.to_C_str());

	expr = build.expr_from(set);
	expected_string = "n >= 0";
	assert(expected_string == expr.to_C_str());
}

/* Test that return values are handled correctly.
 *
 * Test that isl C++ objects, integers, boolean values, and strings are
 * returned correctly.
 */
void test_return(isl::ctx ctx)
{
	test_return_obj(ctx);
	test_return_int(ctx);
	test_return_bool(ctx);
	test_return_string(ctx);
}

/* Test that foreach functions are modeled correctly.
 *
 * Verify that lambdas are correctly called as callback of a 'foreach'
 * function and that variables captured by the lambda work correctly. Also
 * check that the foreach function takes account of the return value of the
 * lambda and aborts in case isl::stat::error is returned and then returns
 * isl::stat::error itself.
 */
void test_foreach(isl::ctx ctx)
{
	isl::set s(ctx, "{ [0]; [1]; [2] }");

	std::vector<isl::basic_set> basic_sets;

	auto add_to_vector = [&] (isl::basic_set bs) {
		basic_sets.push_back(bs);
		return isl::stat::ok;
	};

	isl::stat ret1 = s.foreach_basic_set(add_to_vector);

	assert(ret1 == isl::stat::ok);
	assert(basic_sets.size() == 3);
	assert(isl::set(basic_sets[0]).is_subset(s));
	assert(isl::set(basic_sets[1]).is_subset(s));
	assert(isl::set(basic_sets[2]).is_subset(s));
	assert(!basic_sets[0].is_equal(basic_sets[1]));

	auto fail = [&] (isl::basic_set bs) {
		return isl::stat::error;
	};

	isl::stat ret2 = s.foreach_basic_set(fail);

	assert(ret2 == isl::stat::error);
}

/* Test the isl C++ interface
 *
 * This includes:
 *  - The isl C <-> C++ pointer interface
 *  - Object construction
 *  - Different parameter types
 *  - Different return types
 *  - Foreach functions
 */
int main()
{
	isl_ctx *ctx = isl_ctx_alloc();

	isl_options_set_on_error(ctx, ISL_ON_ERROR_ABORT);

	test_pointer(ctx);
	test_constructors(ctx);
	test_parameters(ctx);
	test_return(ctx);
	test_foreach(ctx);

	isl_ctx_free(ctx);
}
