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
#include <cmath>

// These are faster but less accurate versions of utility functions.

namespace vital {

  namespace {
    constexpr mono_float kDbGainConversionMult = 6.02059991329f;
    constexpr mono_float kDbMagnitudeConversionMult = 1.0f / kDbGainConversionMult;
    constexpr mono_float kExpConversionMult = 1.44269504089f;
    constexpr mono_float kLogConversionMult = 0.69314718056f;
  }

  namespace futils {

    force_inline poly_float exp2(poly_float exponent) {
      static constexpr mono_float kCoefficient0 = 1.0f;
      static constexpr mono_float kCoefficient1 = 16970.0 / 24483.0;
      static constexpr mono_float kCoefficient2 = 1960.0 / 8161.0;
      static constexpr mono_float kCoefficient3 = 1360.0 / 24483.0;
      static constexpr mono_float kCoefficient4 = 80.0 / 8161.0;
      static constexpr mono_float kCoefficient5 = 32.0 / 24483.0;

      poly_int integer = utils::roundToInt(exponent);
      poly_float t = exponent - utils::toFloat(integer);
      poly_float int_pow = utils::pow2ToFloat(integer);

      poly_float cubic = t * (t * (t * kCoefficient5 + kCoefficient4) + kCoefficient3) + kCoefficient2;
      poly_float interpolate = t * (t * cubic + kCoefficient1) + kCoefficient0;
      return int_pow * interpolate;
    }

    force_inline poly_float log2(poly_float value) {
      static constexpr mono_float kCoefficient0 = -1819.0 / 651.0;
      static constexpr mono_float kCoefficient1 = 5.0;
      static constexpr mono_float kCoefficient2 = -10.0 / 3.0;
      static constexpr mono_float kCoefficient3 = 10.0 / 7.0;
      static constexpr mono_float kCoefficient4 = -1.0 / 3.0;
      static constexpr mono_float kCoefficient5 = 1.0 / 31.0;

      poly_int floored_log2 = utils::shiftRight<23>(utils::reinterpretToInt(value)) - 0x7f;
      poly_float t = (value & 0x7fffff) | (0x7f << 23);

      poly_float cubic = t * (t * (t * kCoefficient5 + kCoefficient4) + kCoefficient3) + kCoefficient2;
      poly_float interpolate = t * (t * cubic + kCoefficient1) + kCoefficient0;
      return utils::toFloat(floored_log2) + interpolate;
    }

    force_inline poly_float cheapExp2(poly_float exponent) {
      static constexpr mono_float kCoefficient0 = 1.0f;
      static constexpr mono_float kCoefficient1 = 12.0 / 17.0;
      static constexpr mono_float kCoefficient2 = 4.0 / 17.0;

      poly_int integer = utils::roundToInt(exponent);
      poly_float t = exponent - utils::toFloat(integer);
      poly_float int_pow = utils::pow2ToFloat(integer);

      poly_float interpolate = t * (t * kCoefficient2 + kCoefficient1) + kCoefficient0;
      return int_pow * interpolate;
    }

    force_inline poly_float cheapLog2(poly_float value) {
      static constexpr mono_float kCoefficient0 = -5.0 / 3.0;
      static constexpr mono_float kCoefficient1 = 2.0;
      static constexpr mono_float kCoefficient2 = -1.0 / 3.0;

      poly_int floored_log2 = utils::shiftRight<23>(utils::reinterpretToInt(value)) - 0x7f;
      poly_float t = (value & 0x7fffff) | (0x7f << 23);

      poly_float interpolate = t * (t * kCoefficient2 + kCoefficient1) + kCoefficient0;
      return utils::toFloat(floored_log2) + interpolate;
    }

    force_inline mono_float exp2(mono_float value) {
      poly_float input = value;
      poly_float result = exp2(input);
      return result[0];
    }

    force_inline mono_float log2(mono_float value) {
      poly_float input = value;
      poly_float result = log2(input);
      return result[0];
    }

    force_inline mono_float exp(mono_float exponent) {
      return exp2(exponent * kExpConversionMult);
    }

    force_inline mono_float log(mono_float value) {
      return log2(value) * kLogConversionMult;
    }

    force_inline mono_float exp_half(mono_float exponent) {
      return exp2(-exponent);
    }

    force_inline mono_float pow(mono_float base, mono_float exponent) {
      return exp2(log2(base) * exponent);
    }

    template<mono_float(*func)(mono_float)>
    force_inline poly_float map(poly_float value) {
      poly_float result;
      int size = poly_float::kSize;
      for (int i = 0; i < size; ++i)
        result.set(i, func(value[i]));
      return result;
    }

    force_inline poly_float exp(poly_float exponent) {
      return exp2(exponent * kExpConversionMult);
    }

    force_inline poly_float log(poly_float value) {
      return log2(value) * kLogConversionMult;
    }

    force_inline poly_float exp_half(poly_float exponent) {
      return exp2(-exponent);
    }

    force_inline poly_float pow(poly_float base, poly_float exponent) {
      return exp2(log2(base) * exponent);
    }

    force_inline poly_float cheapPow(poly_float base, poly_float exponent) {
      return cheapExp2(cheapLog2(base) * exponent);
    }
    
    force_inline poly_float midiOffsetToRatio(poly_float note_offset) {
      return exp2(note_offset * (1.0f / kNotesPerOctave));
    }

    force_inline poly_float midiNoteToFrequency(poly_float note) {
      return midiOffsetToRatio(note) * kMidi0Frequency;
    }

    force_inline mono_float magnitudeToDb(mono_float magnitude) {
      return log2(magnitude) * kDbGainConversionMult;
    }

    force_inline poly_float magnitudeToDb(poly_float magnitude) {
      return log2(magnitude) * kDbGainConversionMult;
    }

    force_inline mono_float dbToMagnitude(mono_float decibels) {
      return exp(decibels * kDbMagnitudeConversionMult);
    }

    force_inline poly_float dbToMagnitude(poly_float decibels) {
      return exp2(decibels * kDbMagnitudeConversionMult);
    }

    force_inline poly_float mulAdd(poly_float a, poly_float b, poly_float c) {
      return poly_float::mulAdd(a, b, c);
    }

    force_inline mono_float quickTanh(mono_float value) {
      mono_float square = value * value;
      return value / (square / (3.0f + square * 0.2f) + 1.0f);
    }

    force_inline poly_float quickTanh(poly_float value) {
      poly_float square = value * value;
      return value / (square / mulAdd(3.0f, square, 0.2f) + 1.0f);
    }

    force_inline poly_float quickTanhDerivative(poly_float value) {
      poly_float square = value * value;
      poly_float fourth = square * square;
      poly_float part_den = square + 2.5f;
      poly_float num = mulAdd(mulAdd(6.25f, fourth, 0.166667f), square, -1.25f);
      poly_float den = part_den * part_den;
      return num / den;
    }

    force_inline mono_float quickTanhDerivative(mono_float value) {
      mono_float square = value * value;
      mono_float fourth = square * square;
      mono_float part_den = square + 2.5f;
      mono_float num = square * -1.25f + fourth * 0.166667f + 6.25f;
      mono_float den = part_den * part_den;
      return num / den;
    }

    // Using 1/x function to approximate tanh saturation.
    // Not smooth second derivative.
    force_inline mono_float reciprocalSat(mono_float value) {
      mono_float sign = copysignf(1.0f, value);
      return -1.42f * (1.0f / (value + sign) - sign);
    }

    // Using basic algebra to approximate tanh saturation.
    // Doesn't clamp at infinity but grows slowly.
    force_inline mono_float algebraicSat(mono_float value) {
      mono_float square = value * value;
      return value - value * square * 0.9f / (square + 3.0f);
    }

    force_inline poly_float algebraicSat(poly_float value) {
      poly_float square = value * value;
      return value * square * -0.9f / (square + 3.0f) + value;
    }

    force_inline poly_float quadraticInvSat(poly_float value) {
      return value / (value * value * 0.25f + 1.0f);
    }

    force_inline poly_float bumpSat(poly_float value) {
      poly_float square = value * value;
      poly_float pow_four = square * square;
      return value / (pow_four * 0.1f + 1.0f);
    }

    force_inline poly_float bumpSat2(poly_float value) {
      poly_float square = value * value;
      poly_float pow_four = square * square;
      return (value + square * value * 3.0f) / (pow_four * 20.0f + 1.0f);
    }

    force_inline mono_float algebraicSatDerivative(mono_float value) {
      mono_float square = value * value;
      mono_float fourth = square * square;
      mono_float num = fourth * 0.1f + square * -2.1f + 9.0f;
      mono_float part_den = square + 3.0f;
      mono_float den = part_den * part_den;
      return num / den;
    }

    force_inline poly_float algebraicSatDerivative(poly_float value) {
      poly_float square = value * value;
      poly_float fourth = square * square;
      poly_float part_num = square * -2.1f + 9.0f;
      poly_float num = fourth * 0.1f + part_num;
      poly_float part_den = square + 3.0f;
      poly_float den = part_den * part_den;
      return num / den;
    }

    force_inline poly_float tanh(poly_float value) {
      poly_float abs_value = poly_float::abs(value);
      poly_float square = value * value;

      poly_float part_num1 = abs_value * 0.821226666969744f + 0.893229853513558f;
      poly_float part_num2 = square * part_num1 + 2.45550750702956f;
      poly_float num = value * (abs_value *  2.45550750702956f + part_num2);

      poly_float part_den = poly_float::abs(abs_value * 0.814642734961073f * value + value);
      poly_float den = part_den * (square + 2.44506634652299f) + 2.44506634652299f;
      return num / den;
    }

    force_inline mono_float tanh(mono_float value) {
      mono_float abs_value = fabsf(value);
      mono_float square = value * value;

      mono_float num = value * (2.45550750702956f + 2.45550750702956f * abs_value +
                                square * (0.893229853513558f + 0.821226666969744f * abs_value));
      mono_float den = 2.44506634652299f + (2.44506634652299f + square) *
                       fabsf(value + 0.814642734961073f * value * abs_value);
      return num / den;
    }

    force_inline poly_float hardTanh(poly_float value) {
      static constexpr mono_float kHardnessConstant = 0.66f;
      static constexpr mono_float kHardnessConstantInv = 1.0f - kHardnessConstant;
      static constexpr mono_float kHardnessConstantInvRec = 1.0f / kHardnessConstantInv;

      poly_float clamped = poly_float::max(poly_float::min(value, kHardnessConstant), -kHardnessConstant);
      return clamped + tanh((value - clamped) * kHardnessConstantInvRec) * (1.0f - kHardnessConstant);
    }

    force_inline poly_float tanhDerivativeFast(poly_float value) {
      poly_float square = value * value;
      return poly_float(1.0f) / mulAdd(2.0f, square, 1.8f);
    }

    // Version of fast sin where phase is is [-0.5, 0.5]
    force_inline mono_float quickSin(mono_float phase) {
      return phase * (8.0f - 16.0f * fabsf(phase));
    }

    force_inline poly_float quickSin(poly_float phase) {
      return phase * mulAdd(8.0f, poly_float::abs(phase), -16.0f);
    }

    force_inline mono_float sin(mono_float phase) {
      mono_float approx = quickSin(phase);
      return approx * (0.776f + 0.224f * fabsf(approx));
    }

    force_inline poly_float sin(poly_float phase) {
      poly_float approx = quickSin(phase);
      return approx * mulAdd(0.776f, poly_float::abs(approx), 0.224f);
    }

    force_inline poly_float sinInterpolate(poly_float from, poly_float to, poly_float t) {
      poly_float sin_value = sin(t * 0.5f - 0.25f);
      poly_float sin_t = sin_value * 0.5f + 0.5f;
      return from + (to - from) * sin_t;
    }

    // Version of fast sin where phase is is [0, 1]
    force_inline mono_float quickSin1(mono_float phase) {
      phase = 0.5f - phase;
      return phase * (8.0f - 16.0f * fabsf(phase));
    }

    force_inline poly_float quickSin1(poly_float phase) {
      poly_float adjusted_phase = poly_float(0.5f) - phase;
      return adjusted_phase * mulAdd(8.0f, poly_float::abs(adjusted_phase), -16.0f);
    }

    force_inline mono_float sin1(mono_float phase) {
      mono_float approx = quickSin1(phase);
      return approx * (0.776f + 0.224f * fabsf(approx));
    }

    force_inline poly_float sin1(poly_float phase) {
      poly_float approx = quickSin1(phase);
      return approx * mulAdd(0.776f, poly_float::abs(approx), 0.224f);
    }

    force_inline poly_float equalPowerFade(poly_float t) {
      return sin1(t * 0.25f);
    }

    force_inline poly_float panAmplitude(poly_float pan) {
      static constexpr float kScale = 1.41421356237f;
      poly_float eighth_phase = 0.125f;
      return sin1(eighth_phase - utils::kStereoSplit * pan * eighth_phase) * kScale;
    }

    force_inline poly_float equalPowerFadeInverse(poly_float t) {
      return sin1((t + 1.0f) * 0.25f);
    }

    force_inline mono_float powerScale(mono_float value, mono_float power) {
      static constexpr mono_float kMinPower = 0.01f;

      if (fabsf(power) < kMinPower)
        return value;

      mono_float numerator = exp(power * value) - 1.0f;
      mono_float denominator = exp(power) - 1.0f;
      return numerator / denominator;
    }

    force_inline poly_float powerScale(poly_float value, poly_float power) {
      static constexpr mono_float kMinPowerMag = 0.005f;
      poly_mask zero_mask = poly_float::lessThan(power, kMinPowerMag) & poly_float::lessThan(-power, kMinPowerMag);
      poly_float numerator = exp(power * value) - 1.0f;
      poly_float denominator = exp(power) - 1.0f;
      poly_float result = numerator / denominator;
      return utils::maskLoad(result, value, zero_mask);
    }
  } // namespace futils
} // namespace vital

