/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "poly_values_test.h"
#include "poly_values.h"

#define EPSILON 0.0000001f

void PolyValuesTest::runTest() {
  runFloatTests();
  runIntTests();
}

void PolyValuesTest::runFloatTests() {
  vital::poly_float one(1.0f, 2.0f, 3.0f, 4.0f);
  vital::poly_float two(9.0f, 11.0f, 13.0f, 15.0f);

  beginTest("Floats Add");
  vital::poly_float add = one + two;
  expect(add[0] == 10.0f);
  expect(add[1] == 13.0f);
  expect(add[2] == 16.0f);
  expect(add[3] == 19.0f);
  add += two;
  expect(add[0] == 19.0f);
  expect(add[1] == 24.0f);
  expect(add[2] == 29.0f);
  expect(add[3] == 34.0f);

  beginTest("Floats Subtract");
  vital::poly_float subtract = one - two;
  expect(subtract[0] == -8.0f);
  expect(subtract[1] == -9.0f);
  expect(subtract[2] == -10.0f);
  expect(subtract[3] == -11.0f);
  subtract -= two;
  expect(subtract[0] == -17.0f);
  expect(subtract[1] == -20.0f);
  expect(subtract[2] == -23.0f);
  expect(subtract[3] == -26.0f);

  beginTest("Floats Multiply");
  vital::poly_float multiply = one * two;
  expect(multiply[0] == 9.0f);
  expect(multiply[1] == 22.0f);
  expect(multiply[2] == 39.0f);
  expect(multiply[3] == 60.0f);
  multiply *= one;
  expect(multiply[0] == 9.0f);
  expect(multiply[1] == 44.0f);
  expect(multiply[2] == 117.0f);
  expect(multiply[3] == 240.0f);

  beginTest("Floats Compare");
  vital::poly_mask compare_mask = vital::poly_float::greaterThan(one, one);
  expect(compare_mask[0] == 0);
  expect(compare_mask[1] == 0);
  expect(compare_mask[2] == 0);
  expect(compare_mask[3] == 0);
  compare_mask = vital::poly_float::lessThan(one, one);
  expect(compare_mask[0] == 0);
  expect(compare_mask[1] == 0);
  expect(compare_mask[2] == 0);
  expect(compare_mask[3] == 0);

  compare_mask = vital::poly_float::equal(one, one);
  expect(compare_mask[0] == (unsigned int)-1);
  expect(compare_mask[1] == (unsigned int)-1);
  expect(compare_mask[2] == (unsigned int)-1);
  expect(compare_mask[3] == (unsigned int)-1);

  vital::poly_float one_plus1 = one + 1;
  expect(one_plus1[0] == 2.0f);
  expect(one_plus1[1] == 3.0f);
  expect(one_plus1[2] == 4.0f);
  expect(one_plus1[3] == 5.0f);
  vital::poly_mask greater_mask1 = vital::poly_float::greaterThan(one_plus1, one);
  expect(greater_mask1[0] == (unsigned int)-1);
  expect(greater_mask1[1] == (unsigned int)-1);
  expect(greater_mask1[2] == (unsigned int)-1);
  expect(greater_mask1[3] == (unsigned int)-1);

  vital::poly_float neg0(-5, -5, -2, 0);
  vital::poly_float neg1(-10, -5, 1, -1);
  vital::poly_mask greater_mask2 = vital::poly_float::greaterThan(neg0, neg1);
  expect(greater_mask2[0] == (unsigned int)-1);
  expect(greater_mask2[1] == 0);
  expect(greater_mask2[2] == 0);
  expect(greater_mask2[3] == (unsigned int)-1);

  beginTest("Floats Sum");
  vital::poly_float to_sum(1.0f, -2.0f, 3.0f, -4.0f);
  expect(to_sum.sum() == -2.0f);
}

void PolyValuesTest::runIntTests() {
  vital::poly_int one(1, 2, 3, 4);
  vital::poly_int two(9, 11, 13, 15);

  beginTest("Ints Init");
  expect(one[0] == 1);
  expect(one[1] == 2);
  expect(one[2] == 3);
  expect(one[3] == 4);

  vital::poly_int negative = -5;
  unsigned int negative_uint = -5;
  expect(negative[0] == negative_uint);
  expect(negative[1] == negative_uint);
  expect(negative[2] == negative_uint);
  expect(negative[3] == negative_uint);

  vital::poly_int negatives(-10, 3, -3, -9);
  expect(negatives[0] == (unsigned int)-10);
  expect(negatives[1] == (unsigned int)3);
  expect(negatives[2] == (unsigned int)-3);
  expect(negatives[3] == (unsigned int)-9);

  beginTest("Ints Add");
  vital::poly_int add = one + two;
  expect(add[0] == 10);
  expect(add[1] == 13);
  expect(add[2] == 16);
  expect(add[3] == 19);
  add += two;
  expect(add[0] == 19);
  expect(add[1] == 24);
  expect(add[2] == 29);
  expect(add[3] == 34);

  vital::poly_int wrap = (unsigned int)-2;
  wrap += 5;
  expect(wrap[0] == 3);
  expect(wrap[1] == 3);
  expect(wrap[2] == 3);
  expect(wrap[3] == 3);

  beginTest("Ints Subtract");
  vital::poly_int subtract = two - one;
  expect(subtract[0] == 8);
  expect(subtract[1] == 9);
  expect(subtract[2] == 10);
  expect(subtract[3] == 11);
  subtract -= one;
  expect(subtract[0] == 7);
  expect(subtract[1] == 7);
  expect(subtract[2] == 7);
  expect(subtract[3] == 7);

  beginTest("Ints Multiply");
  vital::poly_int multiply = one * two;
  expect(multiply[0] == 9);
  expect(multiply[1] == 22);
  expect(multiply[2] == 39);
  expect(multiply[3] == 60);
  multiply *= one;
  expect(multiply[0] == 9);
  expect(multiply[1] == 44);
  expect(multiply[2] == 117);
  expect(multiply[3] == 240);

  beginTest("Ints Set");
  multiply = 0;
  expect(multiply[0] == 0);
  expect(multiply[1] == 0);
  expect(multiply[2] == 0);
  expect(multiply[3] == 0);
  subtract = 1;
  expect(subtract[0] == 1);
  expect(subtract[1] == 1);
  expect(subtract[2] == 1);
  expect(subtract[3] == 1);
  add.set(1, 5);
  expect(add[0] == 19);
  expect(add[1] == 5);
  expect(add[2] == 29);
  expect(add[3] == 34);
  add.set(3, 0);
  expect(add[0] == 19);
  expect(add[1] == 5);
  expect(add[2] == 29);
  expect(add[3] == 0);
  add.set(2, (unsigned int)-1);
  expect(add[0] == 19);
  expect(add[1] == 5);
  expect(add[2] == (unsigned int)-1);
  expect(add[3] == 0);

  beginTest("Ints Negate");
  vital::poly_int negate = (unsigned int)-5;
  vital::poly_int negated = -negate;
  expect(negated[0] == 5);
  expect(negated[1] == 5);
  expect(negated[2] == 5);
  expect(negated[3] == 5);

  beginTest("Ints Compare Equal");
  vital::poly_mask compare1 = vital::poly_int::equal(one, two);
  expect(compare1[0] == 0);
  expect(compare1[1] == 0);
  expect(compare1[2] == 0);
  expect(compare1[3] == 0);

  vital::poly_int test_equal0(-1, 5, -10, 5);
  vital::poly_int test_equal1(1, -5, -10, 5);

  vital::poly_mask compare2 = vital::poly_int::equal(test_equal0, test_equal1);
  expect(compare2[0] == 0);
  expect(compare2[1] == 0);
  expect(compare2[2] == (unsigned int)-1);
  expect(compare2[3] == (unsigned int)-1);

  vital::poly_mask compare3 = vital::poly_int::equal(test_equal1, test_equal1);
  expect(compare3[0] == (unsigned int)-1);
  expect(compare3[1] == (unsigned int)-1);
  expect(compare3[2] == (unsigned int)-1);
  expect(compare3[3] == (unsigned int)-1);

  beginTest("Ints Compare Greater");
  vital::poly_mask equal_mask = vital::poly_int::greaterThan(one, one);
  expect(equal_mask[0] == 0);
  expect(equal_mask[1] == 0);
  expect(equal_mask[2] == 0);
  expect(equal_mask[3] == 0);

  vital::poly_int one_plus1 = one + 1;
  expect(one_plus1[0] == 2);
  expect(one_plus1[1] == 3);
  expect(one_plus1[2] == 4);
  expect(one_plus1[3] == 5);
  vital::poly_mask greater_mask1 = vital::poly_int::greaterThan(one_plus1, one);
  expect(greater_mask1[0] == (unsigned int)-1);
  expect(greater_mask1[1] == (unsigned int)-1);
  expect(greater_mask1[2] == (unsigned int)-1);
  expect(greater_mask1[3] == (unsigned int)-1);

  vital::poly_int neg0(-5, -5, -2, 0);
  vital::poly_int neg1(-10, -5, 1, -1);
  vital::poly_mask greater_mask2 = vital::poly_int::greaterThan(neg0, neg1);
  expect(greater_mask2[0] == (unsigned int)-1);
  expect(greater_mask2[1] == 0);
  expect(greater_mask2[2] == (unsigned int)-1);
  expect(greater_mask2[3] == 0);

  beginTest("Ints Sum");
  vital::poly_int to_sum(1, -2, 3, -4);
  expect(to_sum.sum() == (unsigned int)-2);

  beginTest("Detect Mask");
  vital::poly_float compare(1.0f, -2.0f, 3.0f, -4.0f);
  vital::poly_mask compare_mask = vital::poly_float::greaterThan(compare, 2.0f);
  expect(vital::poly_float::greaterThan(compare, 2.0f).anyMask() != 0);
  expect(vital::poly_float::lessThan(compare, -3.5f).anyMask() != 0);
  expect(vital::poly_float::equal(compare, 1.0f).anyMask() != 0);
  expect(vital::poly_float::equal(compare, -2.0f).anyMask() != 0);
  expect(vital::poly_float::equal(compare, 5.0f).anyMask() == 0);
}

static PolyValuesTest poly_values_test;
