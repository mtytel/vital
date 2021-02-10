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
#include "poly_utils.h"

namespace vital {

  template<mono_float(*function)(mono_float), size_t resolution>
  class OneDimLookup {
    static constexpr int kExtraValues = 4;
    public:
      OneDimLookup(float scale = 1.0f) {
        scale_ = resolution / scale;
        for (int i = 0; i < resolution + kExtraValues; ++i) {
          mono_float t = (i - 1.0f) / (resolution - 1.0f);
          lookup_[i] = function(t * scale);
        }
      }

      ~OneDimLookup() { }

      force_inline poly_float cubicLookup(poly_float value) const {
        poly_float boost = value * scale_;
        poly_int indices = utils::clamp(utils::toInt(boost), 0, resolution);
        poly_float t = boost - utils::toFloat(indices);

        matrix interpolation_matrix = utils::getCatmullInterpolationMatrix(t);
        matrix value_matrix = utils::getValueMatrix(lookup_, indices);
        value_matrix.transpose();

        return interpolation_matrix.multiplyAndSumRows(value_matrix);
      }

    private:
      mono_float lookup_[resolution + kExtraValues];
      mono_float scale_;

      JUCE_LEAK_DETECTOR(OneDimLookup)
  };
} // namespace vital

