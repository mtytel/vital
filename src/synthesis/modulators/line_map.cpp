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

#include "line_map.h"

#include "line_generator.h"
#include "utils.h"

namespace vital {
  LineMap::LineMap(LineGenerator* source) : Processor(1, kNumOutputs, true), source_(source) { }

  void LineMap::process(int num_samples) {
    process(input()->at(0));
  }

  void LineMap::process(poly_float phase) {
    mono_float* buffer = source_->getCubicInterpolationBuffer();
    int resolution = source_->resolution();
    poly_float boost = utils::clamp(phase * resolution, 0.0f, resolution);
    poly_int indices = utils::clamp(utils::toInt(boost), 0, resolution - 1);
    poly_float t = boost - utils::toFloat(indices);

    matrix interpolation_matrix = utils::getPolynomialInterpolationMatrix(t);
    matrix value_matrix = utils::getValueMatrix(buffer, indices);

    value_matrix.transpose();

    poly_float result = utils::clamp(interpolation_matrix.multiplyAndSumRows(value_matrix), -1.0f, 1.0f);
    output(kValue)->buffer[0] = result;
    output(kPhase)->buffer[0] = phase;
  }
} // namespace vital
