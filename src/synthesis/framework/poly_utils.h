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

#include "matrix.h"
#include "utils.h"

#include <cmath>

namespace vital {

  namespace utils {
    constexpr mono_float kPhaseEncodingMultiplier = 0.9f;
    constexpr unsigned int kNotePressedMask = 0xf;

    static const poly_float kStereoSplit(1.0f, -1.0f);
    static const poly_float kLagrangeOne(0.0f, 1.0f, 0.0f, 0.0f);
    static const poly_float kLagrangeTwo(-1.0f, -1.0f, 1.0f, 1.0f);
    static const poly_float kLagrangeThree(-2.0f, -2.0f, -2.0f, -1.0f);
    static const poly_float kLagrangeMult(-1.0f / 6.0f, 1.0f / 2.0f, -1.0f / 2.0f, 1.0f / 6.0f);

    static const poly_float kOptimalOne(0.00224072707074864375f, 0.20184198969656244725f,
                                        0.59244492420272312725f, 0.20345744715566445625f);
    static const poly_float kOptimalTwo(-0.0059513775678254975f, -0.456633315206820491f,
                                        -0.035736698832993691f, 0.4982319203618311775f);
    static const poly_float kOptimalThree(0.093515484757265265f, 0.294278871937834749f,
                                          -0.786648885977648931f, 0.398765058036740415f);
    static const poly_float kOptimalFour(-0.10174985775982505f, 0.36030925263849456f,
                                         -0.36030925263849456f, 0.10174985775982505f);

    force_inline poly_float mulAdd(poly_float a, poly_float b, poly_float c) {
      return poly_float::mulAdd(a, b, c);
    }

    force_inline poly_float mulSub(poly_float a, poly_float b, poly_float c) {
      return poly_float::mulSub(a, b, c);
    }

    template<mono_float(*func)(mono_float)>
    force_inline poly_float map(poly_float value) {
      poly_float result;
      for (int i = 0; i < poly_float::kSize; ++i)
        result.set(i, func(value[i]));
      return result;
    }

    force_inline poly_float centsToRatio(poly_float value) {
      return map<centsToRatio>(value);
    }

    force_inline poly_float noteOffsetToRatio(poly_float value) {
      return map<noteOffsetToRatio>(value);
    }

    force_inline poly_float ratioToMidiTranspose(poly_float value) {
      return map<ratioToMidiTranspose>(value);
    }

    force_inline poly_float midiCentsToFrequency(poly_float value) {
      return map<midiCentsToFrequency>(value);
    }

    force_inline poly_float midiNoteToFrequency(poly_float value) {
      return map<midiNoteToFrequency>(value);
    }

    force_inline poly_float frequencyToMidiNote(poly_float value) {
      return map<frequencyToMidiNote>(value);
    }

    force_inline poly_float frequencyToMidiCents(poly_float value) {
      return map<frequencyToMidiCents>(value);
    }

    force_inline poly_float magnitudeToDb(poly_float value) { return map<magnitudeToDb>(value); }
    force_inline poly_float dbToMagnitude(poly_float value) { return map<dbToMagnitude>(value); }
    force_inline poly_float tan(poly_float value) { return map<tanf>(value); }
    force_inline poly_float sin(poly_float value) { return map<sinf>(value); }
    force_inline poly_float cos(poly_float value) { return map<cosf>(value); }

    force_inline poly_float sqrt(poly_float value) {
    #if VITAL_AVX2
      return _mm256_sqrt_ps(value.value);
    #elif VITAL_SSE2
      return _mm_sqrt_ps(value.value);
    #elif VITAL_NEON
      return map<sqrtf>(value);
    #endif
    }

    force_inline poly_float interpolate(poly_float from, poly_float to, mono_float t) {
      return mulAdd(from, to - from, t);
    }

    force_inline poly_float getCubicInterpolationValues(mono_float mono_t) {
      poly_float t = mono_t;
      return kLagrangeMult * (t + kLagrangeOne) * (t + kLagrangeTwo) * (t + kLagrangeThree);
    }

    force_inline poly_float getOptimalInterpolationValues(mono_float mono_t) {
      poly_float t = mono_t;
      return ((kOptimalFour * t + kOptimalThree) * t + kOptimalTwo) * t + kOptimalOne;
    }

    force_inline matrix getPolynomialInterpolationMatrix(poly_float t_from) {
      static constexpr mono_float kMultPrev = -1.0f / 6.0f;
      static constexpr mono_float kMultFrom = 1.0f / 2.0f;
      static constexpr mono_float kMultTo = -1.0f / 2.0f;
      static constexpr mono_float kMultNext = 1.0f / 6.0f;

      poly_float t_prev = t_from + 1.0f;
      poly_float t_to = t_from - 1.0f;
      poly_float t_next = t_from - 2.0f;

      poly_float t_prev_from = t_prev * t_from;
      poly_float t_to_next = t_to * t_next;

      return matrix(t_from * t_to_next * kMultPrev,
                    t_prev * t_to_next * kMultFrom,
                    t_prev_from * t_next * kMultTo,
                    t_prev_from * t_to * kMultNext);
    }

    force_inline matrix getCatmullInterpolationMatrix(poly_float t) {
      poly_float half_t = t * 0.5f;
      poly_float half_t2 = t * half_t;
      poly_float half_t3 = half_t2 * t;
      poly_float half_three_t3 = half_t3 * 3.0f;

      return matrix(half_t2 * 2.0f - half_t3 - half_t,
                    mulSub(half_three_t3, half_t2, 5.0f) + 1.0f,
                    mulAdd(half_t, half_t2, 4.0f) - half_three_t3,
                    half_t3 - half_t2);
    }

    force_inline matrix getLinearInterpolationMatrix(poly_float t) {
      return matrix(0.0f, poly_float(1.0f) - t, t, 0.0f);
    }

    force_inline poly_float toPolyFloatFromUnaligned(const mono_float* unaligned) {
    #if VITAL_AVX2
      return _mm256_loadu_ps(unaligned);
    #elif VITAL_SSE2
      return _mm_loadu_ps(unaligned);
    #elif VITAL_NEON
      return vld1q_f32(unaligned);
    #endif
    }

    force_inline matrix getValueMatrix(const mono_float* buffer, poly_int indices) {
      return matrix(toPolyFloatFromUnaligned(buffer + indices[0]),
                    toPolyFloatFromUnaligned(buffer + indices[1]),
                    toPolyFloatFromUnaligned(buffer + indices[2]),
                    toPolyFloatFromUnaligned(buffer + indices[3]));
    }

    force_inline matrix getValueMatrix(const mono_float* const* buffers, poly_int indices) {
      return matrix(toPolyFloatFromUnaligned(buffers[0] + indices[0]),
                    toPolyFloatFromUnaligned(buffers[1] + indices[1]),
                    toPolyFloatFromUnaligned(buffers[2] + indices[2]),
                    toPolyFloatFromUnaligned(buffers[3] + indices[3]));
    }

    force_inline poly_float interpolate(poly_float from, poly_float to, poly_float t) {
      return mulAdd(from, to - from, t);
    }

    force_inline poly_float interpolate(mono_float from, mono_float to, poly_float t) {
      return mulAdd(from, to - from, t);
    }

    force_inline poly_float perlinInterpolate(poly_float from, poly_float to, poly_float t) {
      poly_float interpolate_from = from * t;
      poly_float interpolate_to = to * (t - 1.0f);
      poly_float interpolate_t = t * t * (t * -2.0f + 3.0f);
      return interpolate(interpolate_from, interpolate_to, interpolate_t) * 2.0f;
    }

    force_inline poly_float clamp(poly_float value, mono_float min, mono_float max) {
      return poly_float::max(poly_float::min(value, max), min);
    }

    force_inline poly_float clamp(poly_float value, poly_float min, poly_float max) {
      return poly_float::max(poly_float::min(value, max), min);
    }

    force_inline poly_int clamp(poly_int value, poly_int min, poly_int max) {
      return poly_int::max(poly_int::min(value, max), min);
    }

    force_inline poly_float max(poly_float left, poly_float right) {
      return poly_float::max(left, right);
    }

    force_inline poly_float min(poly_float left, poly_float right) {
      return poly_float::min(left, right);
    }

    force_inline bool equal(poly_float left, poly_float right) {
      return poly_float::notEqual(left, right).sum() == 0;
    }

    force_inline poly_float maskLoad(poly_float zero_value, poly_float one_value, poly_mask reset_mask) {
      poly_float old_values = zero_value & ~reset_mask;
      poly_float new_values = one_value & reset_mask;

      return old_values + new_values;
    }

    force_inline poly_int maskLoad(poly_int zero_value, poly_int one_value, poly_mask reset_mask) {
      poly_int old_values = zero_value & ~reset_mask;
      poly_int new_values = one_value & reset_mask;

      return old_values | new_values;
    }

    force_inline poly_float modOnce(poly_float value) {
      poly_mask less_mask = poly_float::lessThan(value, 1.0f);
      poly_float lower = value - 1.0f;
      return maskLoad(lower, value, less_mask);
    }

    force_inline poly_mask closeToZeroMask(poly_float value) {
      return poly_float::lessThan(poly_float::abs(value), kEpsilon);
    }

    force_inline poly_float pow(poly_float base, poly_float exponent) {
      poly_float result;
      int size = poly_float::kSize;
      for (int i = 0; i < size; ++i)
        result.set(i, powf(base[i], exponent[i]));
      return result;
    }

    force_inline poly_mask getSilentMask(const poly_float* buffer, int length) {
      poly_mask silent_mask = poly_float::equal(0.0f, 0.0f);
      for (int i = 0; i < length; ++i) {
        poly_mask zero_mask = closeToZeroMask(buffer[i]);
        silent_mask &= zero_mask;
      }
      return silent_mask;
    }

    force_inline poly_float swapStereo(poly_float value) {
    #if VITAL_AVX2
      return _mm256_shuffle_ps(value.value, value.value, _MM_SHUFFLE(2, 3, 0, 1));
    #elif VITAL_SSE2
      return _mm_shuffle_ps(value.value, value.value, _MM_SHUFFLE(2, 3, 0, 1));
    #elif VITAL_NEON
      return vrev64q_f32(value.value);
    #endif
    }

    force_inline poly_int swapStereo(poly_int value) {
    #if VITAL_AVX2
      return _mm256_shuffle_epi32(value.value, _MM_SHUFFLE(2, 3, 0, 1));
    #elif VITAL_SSE2
      return _mm_shuffle_epi32(value.value, _MM_SHUFFLE(2, 3, 0, 1));
    #elif VITAL_NEON
      return vrev64q_u32(value.value);
    #endif
    }

    force_inline poly_float swapVoices(poly_float value) {
    #if VITAL_AVX2
      return _mm256_shuffle_ps(value.value, value.value, _MM_SHUFFLE(1, 0, 3, 2));
    #elif VITAL_SSE2
      return _mm_shuffle_ps(value.value, value.value, _MM_SHUFFLE(1, 0, 3, 2));
    #elif VITAL_NEON
      return vextq_f32(value.value, value.value, 2);
    #endif
    }

    force_inline poly_int swapVoices(poly_int value) {
    #if VITAL_AVX2
      return _mm256_shuffle_epi32(value.value, value.value, _MM_SHUFFLE(1, 0, 3, 2));
    #elif VITAL_SSE2
      return _mm_shuffle_epi32(value.value, _MM_SHUFFLE(1, 0, 3, 2));
    #elif VITAL_NEON
      return vextq_u32(value.value, value.value, 2);
    #endif
    }

    force_inline poly_float swapInner(poly_float value) {
    #if VITAL_AVX2
      return _mm256_shuffle_ps(value.value, value.value, _MM_SHUFFLE(3, 1, 2, 0));
    #elif VITAL_SSE2
      return _mm_shuffle_ps(value.value, value.value, _MM_SHUFFLE(3, 1, 2, 0));
    #elif VITAL_NEON
      float32x4_t rotated = vextq_f32(value.value, value.value, 2);
      float32x4x2_t zipped = vzipq_f32(value.value, rotated);
      return zipped.val[0];
    #endif
    }

    force_inline poly_float reverse(poly_float value) {
    #if VITAL_AVX2
      return _mm256_shuffle_ps(value.value, value.value, _MM_SHUFFLE(0, 1, 2, 3));
    #elif VITAL_SSE2
      return _mm_shuffle_ps(value.value, value.value, _MM_SHUFFLE(0, 1, 2, 3));
    #elif VITAL_NEON
      return swapVoices(swapStereo(value));
    #endif
    }

    force_inline poly_float consolidateAudio(poly_float one, poly_float two) {
    #if VITAL_AVX2
      return _mm256_unpacklo_ps(one.value, two.value);
    #elif VITAL_SSE2
      return _mm_unpacklo_ps(one.value, two.value);
    #elif VITAL_NEON
      return vzipq_f32(one.value, two.value).val[0];
    #endif
    }

    force_inline poly_float compactFirstVoices(poly_float one, poly_float two) {
    #if VITAL_AVX2
      return _mm256_shuffle_ps(one.value, two.value, _MM_SHUFFLE(1, 0, 1, 0));
    #elif VITAL_SSE2
      return _mm_shuffle_ps(one.value, two.value, _MM_SHUFFLE(1, 0, 1, 0));
    #elif VITAL_NEON
      return vcombine_f32(vget_low_f32(one.value), vget_low_f32(two.value));
    #endif
    }

    force_inline poly_float sumSplitAudio(poly_float sum) {
      poly_float totals = sum + utils::swapStereo(sum);
      return utils::swapInner(totals);
    }

    force_inline mono_float maxFloat(poly_float values) {
      poly_float swap_voices = swapVoices(values);
      poly_float max_voice = utils::max(values, swap_voices);
      return utils::max(max_voice, utils::swapStereo(max_voice))[0];
    }

    force_inline mono_float minFloat(poly_float values) {
      poly_float swap_voices = swapVoices(values);
      poly_float min_voice = utils::min(values, swap_voices);
      return utils::min(min_voice, utils::swapStereo(min_voice))[0];
    }

    force_inline poly_float encodeMidSide(poly_float value) {
      return (value + kStereoSplit * swapStereo(value)) * 0.5f;
    }

    force_inline poly_float decodeMidSide(poly_float value) {
      return (value + swapStereo(kStereoSplit * value));
    }

    force_inline poly_float peak(const poly_float* buffer, int num, int skip = 1) {
      poly_float peak = 0.0f;
      for (int i = 0; i < num; i += skip) {
        peak = poly_float::max(peak, buffer[i]);
        peak = poly_float::max(peak, -buffer[i]);
      }

      return peak;
    }

    force_inline void zeroBuffer(mono_float* buffer, int size) {
      for (int i = 0; i < size; ++i)
        buffer[i] = 0.0f;
    }

    force_inline void zeroBuffer(poly_float* buffer, int size) {
      for (int i = 0; i < size; ++i)
        buffer[i] = 0.0f;
    }

    force_inline void copyBuffer(mono_float* dest, const mono_float* source, int size) {
      for (int i = 0; i < size; ++i)
        dest[i] = source[i];
    }
    
    force_inline void copyBuffer(poly_float* dest, const poly_float* source, int size) {
      for (int i = 0; i < size; ++i)
        dest[i] = source[i];
    }

    force_inline void addBuffers(poly_float* dest, const poly_float* b1, const poly_float* b2, int size) {
      for (int i = 0; i < size; ++i)
        dest[i] = b1[i] + b2[i];
    }

    force_inline bool isFinite(poly_float value) {
      for (int i = 0; i < poly_float::kSize; ++i) {
        mono_float val = value[i];
        if (!std::isfinite(val))
          return false;
      }
      return true;
    }

    force_inline bool isInRange(poly_float value, mono_float min, mono_float max) {
      poly_mask greater_mask = poly_float::greaterThan(value, max);
      poly_mask less_than_mask = poly_float::greaterThan(min, value);
      return (greater_mask.sum() + less_than_mask.sum()) == 0;
    }

    force_inline bool isContained(poly_float value) {
      static constexpr mono_float kRange = 8000.0f;
      return isInRange(value, -kRange, kRange);
    }

    force_inline bool isFinite(const poly_float* buffer, int size) {
      for (int i = 0; i < size; ++i) {
        if (!isFinite(buffer[i]))
          return false;
      }
      return true;
    }

    force_inline bool isInRange(const poly_float* buffer, int size, mono_float min, mono_float max) {
      for (int i = 0; i < size; ++i) {
        if (!isInRange(buffer[i], min, max))
          return false;
      }
      return true;
    }

    force_inline bool isContained(const poly_float* buffer, int size) {
      static constexpr mono_float kRange = 8000.0f;
      return isInRange(buffer, size, -kRange, kRange);
    }

    force_inline bool isSilent(const poly_float* buffer, int size) {
      const mono_float* mono_buffer = reinterpret_cast<const mono_float*>(buffer);
      return isSilent(mono_buffer, size * poly_float::kSize);
    }

    force_inline poly_float gather(const mono_float* buffer, const poly_int& indices) {
      poly_float result;
      for (int i = 0; i < poly_float::kSize; ++i) {
        int index = indices[i];
        result.set(i, buffer[index]);
      }
      return result;
    }

    force_inline void adjacentGather(const mono_float* buffer, const poly_int& indices,
                                     poly_float& value, poly_float& next) {
      for (int i = 0; i < poly_float::kSize; ++i) {
        int index = indices[i];
        value.set(i, buffer[index]);
        next.set(i, buffer[index + 1]);
      }
    }

    force_inline poly_float gatherSeparate(const mono_float* const* buffers, const poly_int& indices) {
      poly_float result;
      for (int i = 0; i < poly_float::kSize; ++i) {
        int index = indices[i];
        result.set(i, buffers[i][index]);
      }
      return result;
    }

    force_inline void adjacentGatherSeparate(const mono_float* const* buffers,
                                             const poly_int& indices,
                                             poly_float& value, poly_float& next) {
      for (int i = 0; i < poly_float::kSize; ++i) {
        int index = indices[i];
        value.set(i, buffers[i][index]);
        next.set(i, buffers[i][index + 1]);
      }
    }

    force_inline poly_float fltScale(poly_float value, poly_float power) {
      return power * value / ((power - 1.0f) * value + 1.0f);
    }

    force_inline poly_float toFloat(poly_int integers) {
      VITAL_ASSERT(poly_float::kSize == poly_int::kSize);

    #if VITAL_AVX2
      return _mm256_cvtepi32_ps(integers.value);
    #elif VITAL_SSE2
      return _mm_cvtepi32_ps(integers.value);
    #elif VITAL_NEON
      return vcvtq_f32_s32(vreinterpretq_s32_u32(integers.value));
    #endif
    }

    force_inline poly_int toInt(poly_float floats) {
      VITAL_ASSERT(poly_float::kSize == poly_int::kSize);

    #if VITAL_AVX2
      return _mm256_cvtps_epi32(floats.value);
    #elif VITAL_SSE2
      return _mm_cvtps_epi32(floats.value);
    #elif VITAL_NEON
      return vreinterpretq_u32_s32(vcvtq_s32_f32(floats.value));
    #endif
    }

    force_inline poly_int truncToInt(poly_float value) {
      return toInt(value);
    }

    force_inline poly_float trunc(poly_float value) {
      return toFloat(truncToInt(value));
    }

    force_inline poly_float floor(poly_float value) {
      poly_float truncated = trunc(value);
      return truncated + (poly_float(-1.0f) & poly_float::greaterThan(truncated, value));
    }

    force_inline poly_int floorToInt(poly_float value) {
      return toInt(floor(value));
    }

    force_inline poly_int roundToInt(poly_float value) {
      return floorToInt(value + 0.5f);
    }

    force_inline poly_float ceil(poly_float value) {
      poly_float truncated = trunc(value);
      return truncated + (poly_float(1.0f) & poly_float::lessThan(truncated, value));
    }

    force_inline poly_float round(poly_float value) {
      return floor(value + 0.5f);
    }

    force_inline poly_float mod(poly_float value) {
      return value - floor(value);
    }

    force_inline poly_float reinterpretToFloat(poly_int value) {
    #if VITAL_AVX2
      return _mm256_castsi256_ps(value.value);
    #elif VITAL_SSE2
      return _mm_castsi128_ps(value.value);
    #elif VITAL_NEON
      return vreinterpretq_f32_u32(value.value);
    #endif
    }

    force_inline poly_int reinterpretToInt(poly_float value) {
    #if VITAL_AVX2
      return _mm256_castps_si256(value.value);
    #elif VITAL_SSE2
      return _mm_castps_si128(value.value);
    #elif VITAL_NEON
      return vreinterpretq_u32_f32(value.value);
    #endif
    }

    template<size_t shift>
    force_inline poly_int shiftRight(poly_int integer) {
    #if VITAL_AVX2
      return _mm256_srli_epi32(integers.value, shift);
    #elif VITAL_SSE2
      return _mm_srli_epi32(integer.value, shift);
    #elif VITAL_NEON
      return vshrq_n_u32(integer.value, shift);
    #endif
    }

    template<size_t shift>
    force_inline poly_int shiftLeft(poly_int integer) {
    #if VITAL_AVX2
      return _mm256_slli_epi32(integers.value, shift);
    #elif VITAL_SSE2
      return _mm_slli_epi32(integer.value, shift);
    #elif VITAL_NEON
      return vshlq_n_u32(integer.value, shift);
    #endif
    }

    force_inline poly_float pow2ToFloat(poly_int value) {
      return reinterpretToFloat(shiftLeft<23>(value + 127));
    }

    force_inline poly_float triangleWave(poly_float t) {
      poly_float adjust = t + 0.75f;
      poly_float range = utils::mod(adjust);
      return poly_float::abs(mulAdd(-1.0f, range, 2.0f));
    }

    force_inline poly_float getCycleOffsetFromSeconds(double seconds, poly_float frequency) {
      poly_float offset;
      for (int i = 0; i < poly_float::kSize; ++i) {
        double freq = frequency[i];
        double cycles = freq * seconds;
        offset.set(i, cycles - ::floor(cycles));
      }

      return offset;
    }

    force_inline poly_float getCycleOffsetFromSamples(long long samples, poly_float frequency,
                                                      int sample_rate, int oversample_amount) {
      double tick_time = (1.0 * oversample_amount) / sample_rate;
      double seconds_passed = tick_time * samples;
      return getCycleOffsetFromSeconds(seconds_passed, frequency);
    }

    force_inline poly_float snapTranspose(poly_float transpose, int quantize) {
      poly_float octave_floored = utils::floor(transpose * (1.0f / kNotesPerOctave)) * kNotesPerOctave;
      poly_float tranpose_from_octave = transpose - octave_floored;
      poly_float min_distance = kNotesPerOctave;
      poly_float transpose_in_octave = tranpose_from_octave;
      for (int i = 0; i <= kNotesPerOctave; ++i) {
        if ((quantize >> (i % kNotesPerOctave)) & 1) {
          poly_float distance = poly_float::abs(tranpose_from_octave - i);
          poly_mask best_mask = poly_float::lessThan(distance, min_distance);
          min_distance = utils::maskLoad(min_distance, distance, best_mask);
          transpose_in_octave = utils::maskLoad(transpose_in_octave, i, best_mask);
        }
      }

      return octave_floored + transpose_in_octave;
    }

    force_inline void fillSnapBuffer(int transpose_quantize, float* snap_buffer) {
      float min_snap = 0.0f;
      float max_snap = 0.0f;
      for (int i = 0; i < kNotesPerOctave; ++i) {
        if ((transpose_quantize >> i) & 1) {
          max_snap = i;
          if (min_snap == 0.0f)
            min_snap = i;
        }
      }

      float offset = kNotesPerOctave - max_snap;
      for (int i = 0; i <= kNotesPerOctave; ++i) {
        if ((transpose_quantize >> (i % kNotesPerOctave)) & 1)
          offset = 0.0f;

        snap_buffer[i] = offset;
        offset += 1.0f;
      }
      offset = min_snap;
      for (int i = kNotesPerOctave; i >= 0; --i) {
        if (offset < snap_buffer[i])
          snap_buffer[i] = i + offset;
        else if (snap_buffer[i] != 0.0f)
          snap_buffer[i] = i - snap_buffer[i];
        else {
          snap_buffer[i] = i;
          offset = 0.0f;
        }
        offset += 1.0f;
      }
    }

    force_inline bool isTransposeQuantizeGlobal(int quantize) {
      return quantize >> kNotesPerOctave;
    }

    force_inline bool isTransposeSnapping(int quantize) {
      static constexpr int kTransposeMask = (1 << kNotesPerOctave) - 1;
      return quantize & kTransposeMask;
    }

    force_inline poly_float encodePhaseAndVoice(poly_float phase, poly_float voice) {
      poly_float voice_float = toFloat((toInt(voice) & poly_int(kNotePressedMask)) + 1);
      return voice_float + phase * kPhaseEncodingMultiplier;
    }

    force_inline std::pair<poly_float, poly_float> decodePhaseAndVoice(poly_float encoded) {
      poly_float modulo = mod(encoded);
      poly_float voice = encoded - modulo;
      poly_float phase = modulo * (1.0f / kPhaseEncodingMultiplier);
      return { phase, voice };
    }
  } // namespace utils
} // namespace vital

