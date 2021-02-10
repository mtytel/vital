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

#include "matrix_test.h"
#include "matrix.h"

#define EPSILON 0.0000001f

void MatrixTest::runTest() {
  beginTest("Transpose");
  vital::poly_float row0(1.0f, 2.0f, 3.0f, 4.0f);
  vital::poly_float row1(5.0f, 6.0f, 7.0f, 8.0f);
  vital::poly_float row2(9.0f, 10.0f, 11.0f, 12.0f);
  vital::poly_float row3(13.0f, 14.0f, 15.0f, 16.0f);
  vital::matrix matrix(row0, row1, row2, row3);
  matrix.transpose();
  expect(matrix.row0[0] == 1.0f);
  expect(matrix.row1[0] == 2.0f);
  expect(matrix.row2[0] == 3.0f);
  expect(matrix.row3[0] == 4.0f);
  expect(matrix.row0[1] == 5.0f);
  expect(matrix.row1[1] == 6.0f);
  expect(matrix.row2[1] == 7.0f);
  expect(matrix.row3[1] == 8.0f);
  expect(matrix.row0[2] == 9.0f);
  expect(matrix.row1[2] == 10.0f);
  expect(matrix.row2[2] == 11.0f);
  expect(matrix.row3[2] == 12.0f);
  expect(matrix.row0[3] == 13.0f);
  expect(matrix.row1[3] == 14.0f);
  expect(matrix.row2[3] == 15.0f);
  expect(matrix.row3[3] == 16.0f);
}

static MatrixTest matrix_test;
