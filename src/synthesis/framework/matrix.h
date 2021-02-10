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

#pragma once

#include "common.h"

namespace vital {
  struct matrix {
    poly_float row0, row1, row2, row3;

    matrix() { }
    matrix(poly_float r0, poly_float r1, poly_float r2, poly_float r3) :
      row0(r0), row1(r1), row2(r2), row3(r3) { }

    force_inline void transpose() {
      poly_float::transpose(row0.value, row1.value, row2.value, row3.value);
    }

    force_inline void interpolateColumns(const matrix& other, poly_float t) {
      row0 = poly_float::mulAdd(row0, other.row0 - row0, t);
      row1 = poly_float::mulAdd(row1, other.row1 - row1, t);
      row2 = poly_float::mulAdd(row2, other.row2 - row2, t);
      row3 = poly_float::mulAdd(row3, other.row3 - row3, t);
    }

    force_inline void interpolateRows(const matrix& other, poly_float t) {
      row0 = poly_float::mulAdd(row0, other.row0 - row0, t[0]);
      row1 = poly_float::mulAdd(row1, other.row1 - row1, t[1]);
      row2 = poly_float::mulAdd(row2, other.row2 - row2, t[2]);
      row3 = poly_float::mulAdd(row3, other.row3 - row3, t[3]);
    }

    force_inline poly_float sumRows() {
      return row0 + row1 + row2 + row3;
    }

    force_inline poly_float multiplyAndSumRows(const matrix& other) {
      poly_float row01 = poly_float::mulAdd(row0 * other.row0, row1, other.row1);
      poly_float row012 = poly_float::mulAdd(row01, row2, other.row2);
      return poly_float::mulAdd(row012, row3, other.row3);
    }
  };
} // namespace vital

