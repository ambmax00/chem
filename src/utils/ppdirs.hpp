#ifndef UTILS_PPDIRS_H
#define UTILS_PPDIRS_H

#ifndef TEST_MACRO
  #include <string_view>
  #include "utils/optional.hpp"
#endif

#define CAT(a, ...) PRIMITIVE_CAT(a, __VA_ARGS__)
#define PRIMITIVE_CAT(a, ...) a##__VA_ARGS__

#define PPDIRS_COMMA ,
#define PPDIRS_COMMA2() ,
#define PPDIRS_OP (
#define PPDIRS_CP )
#define PPDIRS_SC ;
#define PPDIRS_OB {
#define PPDIRS_CB }

#define PASTE(x, ...) x##__VA_ARGS__
#define EVALUATING_PASTE(x, ...) PASTE(x, __VA_ARGS__)
#define UNPAREN_IF(x) EVALUATING_PASTE(NOTHING_, EXTRACT x)

#define UNPAREN(...) __VA_ARGS__
#define WRAP(FUNC, ...)

/***********************************************************************
 *                   PYTHON GENERATED FUNCTIONS
 **********************************************************************/

#define NARGS_SEQ( \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, \
    _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, \
    _32, N, ...) \
  N
#define NARGS(...) \
  NARGS_SEQ( \
      0, ##__VA_ARGS__, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
      19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define ITERATE_LIST(FUNC, DELIM, SUFFIX, list) \
  ITERATE(FUNC, DELIM, SUFFIX, UNPAREN list)
#define ITERATE(FUNC, DELIM, SUFFIX, ...) \
  CAT(_ITERATE_, NARGS(__VA_ARGS__))(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_0(FUNC, DELIM, SUFFIX, x)
#define _ITERATE_1(FUNC, DELIM, SUFFIX, x) FUNC(x) UNPAREN SUFFIX
#define _ITERATE_2(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_1(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_3(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_2(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_4(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_3(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_5(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_4(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_6(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_5(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_7(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_6(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_8(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_7(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_9(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_8(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_10(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_9(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_11(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_10(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_12(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_11(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_13(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_12(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_14(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_13(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_15(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_14(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_16(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_15(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_17(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_16(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_18(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_17(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_19(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_18(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_20(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_19(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_21(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_20(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_22(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_21(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_23(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_22(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_24(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_23(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_25(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_24(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_26(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_25(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_27(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_26(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_28(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_27(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_29(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_28(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_30(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_29(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_31(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_30(FUNC, DELIM, SUFFIX, __VA_ARGS__)
#define _ITERATE_32(FUNC, DELIM, SUFFIX, x, ...) \
  FUNC(x) UNPAREN DELIM _ITERATE_31(FUNC, DELIM, SUFFIX, __VA_ARGS__)

#define REPEAT_FIRST(FUNC, VAR, NSTART, N, DELIM, SUFFIX) \
  CAT(_REPEAT_FIRST_, N)(FUNC, VAR, NSTART, DELIM, SUFFIX)
#define _REPEAT_FIRST_0(FUNC, VAR, NSTART, DELIM, SUFFIX)
#define _REPEAT_FIRST_1(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) UNPAREN SUFFIX
#define _REPEAT_FIRST_2(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_1(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_3(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_2(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_4(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_3(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_5(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_4(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_6(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_5(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_7(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_6(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_8(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_7(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_9(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_8(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_10(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_9(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_11(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_10(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_12(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_11(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_13(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_12(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_14(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_13(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_15(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_14(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_16(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_15(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_17(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_16(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_18(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_17(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_19(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_18(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_20(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_19(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_21(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_20(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_22(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_21(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_23(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_22(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_24(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_23(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_25(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_24(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_26(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_25(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_27(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_26(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_28(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_27(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_29(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_28(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_30(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_29(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_31(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_30(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_FIRST_32(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_FIRST_31(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)

#define REPEAT_SECOND(FUNC, VAR, NSTART, N, DELIM, SUFFIX) \
  CAT(_REPEAT_SECOND_, N)(FUNC, VAR, NSTART, DELIM, SUFFIX)
#define _REPEAT_SECOND_0(FUNC, VAR, NSTART, DELIM, SUFFIX)
#define _REPEAT_SECOND_1(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) UNPAREN SUFFIX
#define _REPEAT_SECOND_2(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_1(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_3(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_2(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_4(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_3(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_5(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_4(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_6(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_5(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_7(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_6(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_8(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_7(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_9(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_8(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_10(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_9(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_11(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_10(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_12(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_11(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_13(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_12(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_14(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_13(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_15(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_14(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_16(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_15(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_17(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_16(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_18(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_17(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_19(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_18(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_20(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_19(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_21(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_20(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_22(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_21(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_23(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_22(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_24(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_23(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_25(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_24(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_26(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_25(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_27(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_26(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_28(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_27(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_29(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_28(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_30(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_29(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_31(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_30(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_SECOND_32(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_SECOND_31(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)

#define REPEAT_THIRD(FUNC, VAR, NSTART, N, DELIM, SUFFIX) \
  CAT(_REPEAT_THIRD_, N)(FUNC, VAR, NSTART, DELIM, SUFFIX)
#define _REPEAT_THIRD_0(FUNC, VAR, NSTART, DELIM, SUFFIX)
#define _REPEAT_THIRD_1(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) UNPAREN SUFFIX
#define _REPEAT_THIRD_2(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_1(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_3(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_2(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_4(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_3(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_5(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_4(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_6(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_5(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_7(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_6(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_8(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_7(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_9(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_8(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_10(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_9(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_11(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_10(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_12(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_11(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_13(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_12(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_14(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_13(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_15(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_14(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_16(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_15(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_17(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_16(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_18(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_17(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_19(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_18(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_20(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_19(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_21(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_20(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_22(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_21(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_23(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_22(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_24(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_23(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_25(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_24(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_26(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_25(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_27(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_26(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_28(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_27(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_29(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_28(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_30(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_29(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_31(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_30(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)
#define _REPEAT_THIRD_32(FUNC, VAR, NSTART, DELIM, SUFFIX) \
  FUNC(VAR, NSTART) \
  UNPAREN DELIM _REPEAT_THIRD_31(FUNC, VAR, INC(NSTART), DELIM, SUFFIX)

#define DEC(x) CAT(_DEC_, x)
#define _DEC_1 0
#define _DEC_2 1
#define _DEC_3 2
#define _DEC_4 3
#define _DEC_5 4
#define _DEC_6 5
#define _DEC_7 6
#define _DEC_8 7
#define _DEC_9 8
#define _DEC_10 9
#define _DEC_11 10
#define _DEC_12 11
#define _DEC_13 12
#define _DEC_14 13
#define _DEC_15 14
#define _DEC_16 15
#define _DEC_17 16
#define _DEC_18 17
#define _DEC_19 18
#define _DEC_20 19
#define _DEC_21 20
#define _DEC_22 21
#define _DEC_23 22
#define _DEC_24 23
#define _DEC_25 24
#define _DEC_26 25
#define _DEC_27 26
#define _DEC_28 27
#define _DEC_29 28
#define _DEC_30 29
#define _DEC_31 30
#define _DEC_32 31

#define INC(x) CAT(_INC_, x)
#define _INC_0 1
#define _INC_1 2
#define _INC_2 3
#define _INC_3 4
#define _INC_4 5
#define _INC_5 6
#define _INC_6 7
#define _INC_7 8
#define _INC_8 9
#define _INC_9 10
#define _INC_10 11
#define _INC_11 12
#define _INC_12 13
#define _INC_13 14
#define _INC_14 15
#define _INC_15 16
#define _INC_16 17
#define _INC_17 18
#define _INC_18 19
#define _INC_19 20
#define _INC_20 21
#define _INC_21 22
#define _INC_22 23
#define _INC_23 24
#define _INC_24 25
#define _INC_25 26
#define _INC_26 27
#define _INC_27 28
#define _INC_28 29
#define _INC_29 30
#define _INC_30 31
#define _INC_31 32
#define _INC_32 33

#define GET(list, n) CAT(GET_, n) list
#define GET_1(a_1, ...) a_1
#define GET_2(a_1, a_2, ...) a_2
#define GET_3(a_1, a_2, a_3, ...) a_3
#define GET_4(a_1, a_2, a_3, a_4, ...) a_4
#define GET_5(a_1, a_2, a_3, a_4, a_5, ...) a_5
#define GET_6(a_1, a_2, a_3, a_4, a_5, a_6, ...) a_6
#define GET_7(a_1, a_2, a_3, a_4, a_5, a_6, a_7, ...) a_7
#define GET_8(a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, ...) a_8
#define GET_9(a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, ...) a_9
#define GET_10(a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, ...) a_10
#define GET_11(a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, ...) \
  a_11
#define GET_12( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, ...) \
  a_12
#define GET_13( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, ...) \
  a_13
#define GET_14( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    ...) \
  a_14
#define GET_15( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, ...) \
  a_15
#define GET_16( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, ...) \
  a_16
#define GET_17( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, ...) \
  a_17
#define GET_18( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, a_18, ...) \
  a_18
#define GET_19( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, a_18, a_19, ...) \
  a_19
#define GET_20( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, a_18, a_19, a_20, ...) \
  a_20
#define GET_21( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, a_18, a_19, a_20, a_21, ...) \
  a_21
#define GET_22( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, a_18, a_19, a_20, a_21, a_22, ...) \
  a_22
#define GET_23( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, a_18, a_19, a_20, a_21, a_22, a_23, ...) \
  a_23
#define GET_24( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, a_18, a_19, a_20, a_21, a_22, a_23, a_24, ...) \
  a_24
#define GET_25( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, a_18, a_19, a_20, a_21, a_22, a_23, a_24, a_25, ...) \
  a_25
#define GET_26( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, a_18, a_19, a_20, a_21, a_22, a_23, a_24, a_25, a_26, \
    ...) \
  a_26
#define GET_27( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, a_18, a_19, a_20, a_21, a_22, a_23, a_24, a_25, a_26, \
    a_27, ...) \
  a_27
#define GET_28( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, a_18, a_19, a_20, a_21, a_22, a_23, a_24, a_25, a_26, \
    a_27, a_28, ...) \
  a_28
#define GET_29( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, a_18, a_19, a_20, a_21, a_22, a_23, a_24, a_25, a_26, \
    a_27, a_28, a_29, ...) \
  a_29
#define GET_30( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, a_18, a_19, a_20, a_21, a_22, a_23, a_24, a_25, a_26, \
    a_27, a_28, a_29, a_30, ...) \
  a_30
#define GET_31( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, a_18, a_19, a_20, a_21, a_22, a_23, a_24, a_25, a_26, \
    a_27, a_28, a_29, a_30, a_31, ...) \
  a_31
#define GET_32( \
    a_1, a_2, a_3, a_4, a_5, a_6, a_7, a_8, a_9, a_10, a_11, a_12, a_13, a_14, \
    a_15, a_16, a_17, a_18, a_19, a_20, a_21, a_22, a_23, a_24, a_25, a_26, \
    a_27, a_28, a_29, a_30, a_31, a_32, ...) \
  a_32

#define RECURSIVE(FUNC, NSTEPS, ARGS) CAT(_RECURSIVE_, NSTEPS)(FUNC, ARGS)
#define _RECURSIVE_0(FUNC, ARGS)
#define _RECURSIVE_1(FUNC, ARGS) FUNC(ARGS)
#define _RECURSIVE_2(FUNC, X) FUNC(_RECURSIVE_1(FUNC, X))
#define _RECURSIVE_3(FUNC, X) FUNC(_RECURSIVE_2(FUNC, X))
#define _RECURSIVE_4(FUNC, X) FUNC(_RECURSIVE_3(FUNC, X))
#define _RECURSIVE_5(FUNC, X) FUNC(_RECURSIVE_4(FUNC, X))
#define _RECURSIVE_6(FUNC, X) FUNC(_RECURSIVE_5(FUNC, X))
#define _RECURSIVE_7(FUNC, X) FUNC(_RECURSIVE_6(FUNC, X))
#define _RECURSIVE_8(FUNC, X) FUNC(_RECURSIVE_7(FUNC, X))
#define _RECURSIVE_9(FUNC, X) FUNC(_RECURSIVE_8(FUNC, X))
#define _RECURSIVE_10(FUNC, X) FUNC(_RECURSIVE_9(FUNC, X))
#define _RECURSIVE_11(FUNC, X) FUNC(_RECURSIVE_10(FUNC, X))
#define _RECURSIVE_12(FUNC, X) FUNC(_RECURSIVE_11(FUNC, X))
#define _RECURSIVE_13(FUNC, X) FUNC(_RECURSIVE_12(FUNC, X))
#define _RECURSIVE_14(FUNC, X) FUNC(_RECURSIVE_13(FUNC, X))
#define _RECURSIVE_15(FUNC, X) FUNC(_RECURSIVE_14(FUNC, X))
#define _RECURSIVE_16(FUNC, X) FUNC(_RECURSIVE_15(FUNC, X))
#define _RECURSIVE_17(FUNC, X) FUNC(_RECURSIVE_16(FUNC, X))
#define _RECURSIVE_18(FUNC, X) FUNC(_RECURSIVE_17(FUNC, X))
#define _RECURSIVE_19(FUNC, X) FUNC(_RECURSIVE_18(FUNC, X))
#define _RECURSIVE_20(FUNC, X) FUNC(_RECURSIVE_19(FUNC, X))
#define _RECURSIVE_21(FUNC, X) FUNC(_RECURSIVE_20(FUNC, X))
#define _RECURSIVE_22(FUNC, X) FUNC(_RECURSIVE_21(FUNC, X))
#define _RECURSIVE_23(FUNC, X) FUNC(_RECURSIVE_22(FUNC, X))
#define _RECURSIVE_24(FUNC, X) FUNC(_RECURSIVE_23(FUNC, X))
#define _RECURSIVE_25(FUNC, X) FUNC(_RECURSIVE_24(FUNC, X))
#define _RECURSIVE_26(FUNC, X) FUNC(_RECURSIVE_25(FUNC, X))
#define _RECURSIVE_27(FUNC, X) FUNC(_RECURSIVE_26(FUNC, X))
#define _RECURSIVE_28(FUNC, X) FUNC(_RECURSIVE_27(FUNC, X))
#define _RECURSIVE_29(FUNC, X) FUNC(_RECURSIVE_28(FUNC, X))
#define _RECURSIVE_30(FUNC, X) FUNC(_RECURSIVE_29(FUNC, X))
#define _RECURSIVE_31(FUNC, X) FUNC(_RECURSIVE_30(FUNC, X))
#define _RECURSIVE_32(FUNC, X) FUNC(_RECURSIVE_31(FUNC, X))

/***********************************************************************
 *                       OTHER FUNCTIONS
 **********************************************************************/

#define ADD(X, Y) RECURSIVE(INC, Y, X)
#define SUB(X, Y) RECURSIVE(DEC, Y, X)

#define ECHO(x) x
#define ECHO_P(x, n) ECHO(x)

#define ECHO_LIST(DELIM, SUFFIX, list) ITERATE_LIST(ECHO, DELIM, SUFFIX, list)

#define PUSH_FRONT(val, args) (val, UNPAREN args)

#define CONCAT(list1, list2) \
  (ECHO_LIST((, ), (, ), list1) ECHO_LIST((, ), (), list2))

#define XSTR(a) STR(a)
#define STR(a) #a

#define STRING_EQUAL(str1, str2) util::string_equal(XSTR(str1), XSTR(str2))

#define _LISTSIZE_DETAIL(UNUSED, ...) NARGS(__VA_ARGS__)
#define LISTSIZE(list) _LISTSIZE_DETAIL(, UNPAREN list)

#define _BUILDER_MEMBER_BASE(ctype, name) \
  typename util::builder_type<UNPAREN ctype>::type CAT(c_, name);

#define _BUILDER_SET_BASE(ctype, name) \
  _create_base& name( \
      util::optional<typename util::base_type<UNPAREN ctype>::type> CAT( \
          i_, name)) \
  { \
    CAT(c_, name) = std::move(CAT(i_, name)); \
    return *this; \
  }

#define _BUILDER_SET(params) _BUILDER_SET_BASE(GET_1 params, GET_2 params)

#define _BUILDER_MEMBER(params) _BUILDER_MEMBER_BASE(GET_1 params, GET_2 params)

#define MAKE_BUILDER_MEMBERS(structname, params) \
  ITERATE_LIST(_BUILDER_MEMBER, (), (), params)

#define MAKE_BUILDER_SETS(structname, params) \
  ITERATE_LIST(_BUILDER_SET, (), (), params)

#define _PARAM_MEMBER_BASE(ctype, name) UNPAREN ctype CAT(p_, name);

#define _PARAM_MEMBER(params) _PARAM_MEMBER_BASE(GET_1 params, GET_2 params)

#define MAKE_PARAM_STRUCT(structname, params, initlist) \
  struct CAT(structname, _pack) { \
    ITERATE_LIST(_PARAM_MEMBER, (), (), CONCAT(initlist, params)) \
  };

#define _CHECK_MEMBER_BASE(ctype, name) \
  if constexpr (!util::is_optional<UNPAREN ctype>::value) { \
    if (!CAT(c_, name)) { \
      throw std::runtime_error("Parameter " XSTR(name) " not present!"); \
    } \
  }

#define _CHECK_MEMBER(params) _CHECK_MEMBER_BASE(GET_1 params, GET_2 params)

#define CHECK_REQUIRED(params) ITERATE_LIST(_CHECK_MEMBER, (), (), params)

#define _EVAL_BASE(ctype, name) util::evaluate<UNPAREN ctype>(CAT(c_, name))

#define EVAL(params) _EVAL_BASE(GET_1 params, GET_2 params)

#define _ECHO_DEF_BASE(ctype, name) UNPAREN ctype CAT(i_, name)
#define ECHO_DEF(params) _ECHO_DEF_BASE(GET_1 params, GET_2 params)

#define _ECHO_INIT_BASE(name) CAT(c_, name)(CAT(i_, name))
#define ECHO_INIT(params) _ECHO_INIT_BASE(GET_2 params)

#define ECHO_NONE(...)

#define MAKE_BUILDER_CONSTRUCTOR(structname, initlist) \
  CAT(structname, _base) \
  (ITERATE_LIST(ECHO_DEF, (, ), (), initlist)) ITERATE_LIST( \
      ECHO_NONE, (), \
      ( \
          :), \
      initlist) ITERATE_LIST(ECHO_INIT, (, ), (), initlist) \
  { \
  }

#define MAKE_BUILDER_CLASS(classname, structname, params, initlist) \
  class CAT(structname, _base) { \
    typedef CAT(structname, _base) _create_base; \
\
   private: \
    MAKE_BUILDER_MEMBERS(structname, CONCAT(initlist, params)) \
\
   public: \
    MAKE_BUILDER_SETS(structname, params) \
    MAKE_BUILDER_CONSTRUCTOR(structname, initlist) \
\
    std::shared_ptr<classname> build() \
    { \
      CHECK_REQUIRED(params) \
      CAT(structname, _pack) \
      p = {ITERATE_LIST(EVAL, (, ), (), CONCAT(initlist, params))}; \
      return std::make_shared<classname>(std::move(p)); \
    } \
  }; \
\
  template <typename... Types> \
  static CAT(structname, _base) structname(Types&&... p) \
  { \
    return CAT(structname, _base)(std::forward<Types>(p)...); \
  }

#define MAKE_INIT_BASE(name, value) \
  CAT(m_, name)((CAT(p.p_, name)) ? *(CAT(p.p_, name)) : value)

#define MAKE_INIT(param) MAKE_INIT_BASE(GET_2 param, GET_3 param)

#define MAKE_INIT_LIST_OPT(list) ITERATE_LIST(MAKE_INIT, (, ), (), list)

#define CAT_M_BASE(ctype, name) \
  util::base_type<UNPAREN ctype>::type CAT(m_, name)

#define CAT_M(params) CAT_M_BASE(GET_1 params, GET_2 params)

#define MAKE_MEMBER_VARS(list) ITERATE_LIST(CAT_M, (;), (;), list)

#define ENUM_STRINGS_IF(val) \
  if (str == #val) \
    return enumtype::val;

#define ENUM_STRINGS(name, list) \
  enum class name { ITERATE_LIST(ECHO, (, ), (), list) }; \
  inline name CAT(str2, name)(std::string str) \
  { \
    using enumtype = name; \
    ITERATE_LIST(ENUM_STRINGS_IF, (), (), list) \
    throw std::runtime_error("Invalid string for " #name); \
  }

/***********************************************************************
 *                          CPP FUNCTIONS
 **********************************************************************/

namespace util {

inline constexpr bool string_equal(const char* str1, const char* str2)
{
  return std::string_view(str1) == str2;
}

template <typename T>
struct is_optional : public std::false_type {
};

template <typename T>
struct is_optional<util::optional<T>> : public std::true_type {
};

template <typename T, typename T2 = void>
struct builder_type;

template <typename T>
struct builder_type<
    T,
    typename std::enable_if<!util::is_optional<T>::value>::type> {
  typedef typename util::optional<T> type;
};

template <typename T>
struct builder_type<
    T,
    typename std::enable_if<util::is_optional<T>::value>::type> {
  typedef T type;
};

template <class T, typename T2 = void>
struct base_type;

template <class T>
struct base_type<
    T,
    typename std::enable_if<util::is_optional<T>::value>::type> {
  typedef typename T::value_type type;
};

template <class T>
struct base_type<
    T,
    typename std::enable_if<!util::is_optional<T>::value>::type> {
  typedef T type;
};

template <
    typename T,
    typename = typename std::enable_if<!is_optional<T>::value>::type>
inline T evaluate(util::optional<T>& val)
{
  return *val;
}

template <
    typename T,
    typename = typename std::enable_if<is_optional<T>::value>::type>
inline T evaluate(T& val)
{
  return (val) ? val : util::nullopt;
}

}  // namespace util

#endif
