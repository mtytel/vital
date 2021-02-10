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
#include "synth_constants.h"
#include "utils.h"

namespace vital {

  template <poly_float (*saturate)(poly_float) = utils::pass<poly_float>>
  class OnePoleFilter {
    public:
      OnePoleFilter() {
        reset(constants::kFullMask);
      }

      force_inline void reset(poly_mask reset_mask) {
        current_state_ = utils::maskLoad(current_state_, 0.0f, reset_mask);
        filter_state_ = utils::maskLoad(filter_state_, 0.0f, reset_mask);
        sat_filter_state_ = utils::maskLoad(sat_filter_state_, 0.0f, reset_mask);
      }

      virtual ~OnePoleFilter() { }

      force_inline poly_float tickBasic(poly_float audio_in, poly_float coefficient) {
        poly_float delta = coefficient * (audio_in - filter_state_);
        filter_state_ += delta;
        current_state_ = filter_state_;
        filter_state_ += delta;
        return current_state_;
      }

      force_inline poly_float tick(poly_float audio_in, poly_float coefficient) {
        poly_float delta = coefficient * (audio_in - sat_filter_state_);
        filter_state_ += delta;
        current_state_ = saturate(filter_state_);
        filter_state_ += delta;
        sat_filter_state_ = saturate(filter_state_);
        return current_state_;
      }

      force_inline poly_float tickDerivative(poly_float audio_in, poly_float coefficient) {
        poly_float delta = coefficient * (audio_in - filter_state_);
        filter_state_ = utils::mulAdd(filter_state_, saturate(filter_state_ + delta), delta);
        current_state_ = filter_state_;
        filter_state_ = utils::mulAdd(filter_state_, saturate(filter_state_ + delta), delta);
        sat_filter_state_ = filter_state_;
        return current_state_;
      }

      force_inline poly_float getCurrentState() { return current_state_; }
      force_inline poly_float getNextSatState() { return sat_filter_state_; }
      force_inline poly_float getNextState() { return filter_state_; }

      static force_inline poly_float computeCoefficient(poly_float cutoff_frequency, int sample_rate) {
        poly_float delta_phase = cutoff_frequency * (vital::kPi / sample_rate);
        return utils::tan(delta_phase / (delta_phase + 1.0f));
      }

    private:
      poly_float current_state_;
      poly_float filter_state_;
      poly_float sat_filter_state_;
  };
} // namespace vital

