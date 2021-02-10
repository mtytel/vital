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

#include <cstdint>
#include <climits>
#include <cstdlib>

#if VITAL_AVX2
  #define VITAL_AVX2 1
  static_assert(false, "AVX2 is not supported yet.");
#elif __SSE2__
  #define VITAL_SSE2 1
#elif defined(__ARM_NEON__) || defined(__ARM_NEON)
  #define VITAL_NEON 1
#else
  static_assert(false, "No SIMD Intrinsics found which are necessary for compilation");
#endif

#if VITAL_SSE2
  #include <immintrin.h>
#elif VITAL_NEON
  #include <arm_neon.h>
#endif

#if !defined(force_inline)
#if defined (_MSC_VER)
#define force_inline __forceinline
  #define vector_call __vectorcall
#else
  #define force_inline inline __attribute__((always_inline))
  #define vector_call
#endif
#endif

namespace vital {
  struct poly_int {
#if VITAL_AVX2
    static constexpr size_t kSize = 8;
    typedef __m256i simd_type;
#elif VITAL_SSE2
    static constexpr size_t kSize = 4;
    typedef __m128i simd_type;
#elif VITAL_NEON
    static constexpr size_t kSize = 4;
    typedef uint32x4_t simd_type;
#endif

    union scalar_simd_union {
      int32_t scalar[kSize];
      simd_type simd;
    };

    union simd_scalar_union {
      simd_type simd;
      int32_t scalar[kSize];
    };

    static constexpr uint32_t kFullMask = (unsigned int)-1;
    static constexpr uint32_t kSignMask = 0x80000000;
    static constexpr uint32_t kNotSignMask = kFullMask ^ kSignMask;

    static force_inline simd_type vector_call init(uint32_t scalar) {
#if VITAL_AVX2
      return _mm256_set1_epi32((int32_t)scalar);
#elif VITAL_SSE2
      return _mm_set1_epi32((int32_t)scalar);
#elif VITAL_NEON
      return vdupq_n_u32(scalar);
#endif
    }

    static force_inline simd_type vector_call load(const uint32_t* memory) {
#if VITAL_AVX2
      return _mm256_loadu_si256((const __m256i*)scalar);
#elif VITAL_SSE2
      return _mm_loadu_si128((const __m128i*)memory);
#elif VITAL_NEON
      return vld1q_u32(memory);
#endif
    }

    static force_inline simd_type vector_call add(simd_type one, simd_type two) {
#if VITAL_AVX2
      return _mm256_add_epi32(one, two);
#elif VITAL_SSE2
      return _mm_add_epi32(one, two);
#elif VITAL_NEON
      return vaddq_u32(one, two);
#endif
    }

    static force_inline simd_type vector_call sub(simd_type one, simd_type two) {
#if VITAL_AVX2
      return _mm256_sub_epi32(one, two);
#elif VITAL_SSE2
      return _mm_sub_epi32(one, two);
#elif VITAL_NEON
      return vsubq_u32(one, two);
#endif
    }

    static force_inline simd_type vector_call neg(simd_type value) {
#if VITAL_AVX2
      return _mm256_sub_epi32(_mm256_set1_epi32(0), value);
#elif VITAL_SSE2
      return _mm_sub_epi32(_mm_set1_epi32(0), value);
#elif VITAL_NEON
      return vmulq_n_u32(value, -1);
#endif
    }

    static force_inline simd_type vector_call mul(simd_type one, simd_type two) {
#if VITAL_AVX2
      return _mm256_mul_epi32(one, two);
#elif VITAL_SSE2
      simd_type mul0_2 = _mm_mul_epu32(one, two);
      simd_type mul1_3 = _mm_mul_epu32(_mm_shuffle_epi32(one, _MM_SHUFFLE(2, 3, 0, 1)),
                                       _mm_shuffle_epi32(two, _MM_SHUFFLE(2, 3, 0, 1)));
      return _mm_unpacklo_epi32(_mm_shuffle_epi32(mul0_2, _MM_SHUFFLE (0, 0, 2, 0)),
                                _mm_shuffle_epi32(mul1_3, _MM_SHUFFLE (0, 0, 2, 0)));
#elif VITAL_NEON
      return vmulq_u32(one, two);
#endif
    }

    static force_inline simd_type vector_call bitAnd(simd_type value, simd_type mask) {
#if VITAL_AVX2
      return _mm256_and_si256(value, mask);
#elif VITAL_SSE2
      return _mm_and_si128(value, mask);
#elif VITAL_NEON
      return vandq_u32(value, mask);
#endif
    }

    static force_inline simd_type vector_call bitOr(simd_type value, simd_type mask) {
#if VITAL_AVX2
      return _mm256_or_si256(value, mask);
#elif VITAL_SSE2
      return _mm_or_si128(value, mask);
#elif VITAL_NEON
      return vorrq_u32(value, mask);
#endif
    }

    static force_inline simd_type vector_call bitXor(simd_type value, simd_type mask) {
#if VITAL_AVX2
      return _mm256_xor_si256(value, mask);
#elif VITAL_SSE2
      return _mm_xor_si128(value, mask);
#elif VITAL_NEON
      return veorq_u32(value, mask);
#endif
    }

    static force_inline simd_type vector_call bitNot(simd_type value) {
      return bitXor(value, init(-1));
    }

    static force_inline simd_type vector_call max(simd_type one, simd_type two) {
#if VITAL_AVX2
      return _mm256_max_epi32(one, two);
#elif VITAL_SSE2
      simd_type greater_than_mask = greaterThan(one, two);
      return _mm_or_si128(_mm_and_si128(greater_than_mask, one), _mm_andnot_si128(greater_than_mask, two));
#elif VITAL_NEON
      return vmaxq_u32(one, two);
#endif
    }

    static force_inline simd_type vector_call min(simd_type one, simd_type two) {
#if VITAL_AVX2
      return _mm256_min_epi32(one, two);
#elif VITAL_SSE2
      simd_type less_than_mask = _mm_cmpgt_epi32(two, one);
      return _mm_or_si128(_mm_and_si128(less_than_mask, one), _mm_andnot_si128(less_than_mask, two));
#elif VITAL_NEON
      return vminq_u32(one, two);
#endif
    }

    static force_inline simd_type vector_call equal(simd_type one, simd_type two) {
#if VITAL_AVX2
      return _mm256_cmpeq_epi32(one, two);
#elif VITAL_SSE2
      return _mm_cmpeq_epi32(one, two);
#elif VITAL_NEON
      return vceqq_u32(one, two);
#endif
    }

    static force_inline simd_type vector_call greaterThan(simd_type one, simd_type two) {
#if VITAL_AVX2
      return _mm256_cmpgt_epi32(_mm256_xor_si256(one, init(kSignMask)), _mm256_xor_si256(two, init(kSignMask)));
#elif VITAL_SSE2
      return _mm_cmpgt_epi32(_mm_xor_si128(one, init(kSignMask)), _mm_xor_si128(two, init(kSignMask)));
#elif VITAL_NEON
      return vcgtq_u32(one, two);
#endif
    }

    static force_inline uint32_t vector_call sum(simd_type value) {
#if VITAL_AVX2
      simd_type flip = _mm256_permute4x64_epi64(value, _MM_SHUFFLE(1, 0, 3, 2))
      simd_type sum = _mm256_hadd_epi32(value, flip);
      sum = _mm256_hadd_epi32(sum, sum);
      return _mm256_cvtsi256_si32(sum);
#elif VITAL_SSE2
      simd_scalar_union union_value { value };
      uint32_t total = 0;
      for (int i = 0; i < kSize; ++i)
        total += union_value.scalar[i];
      return total;
#elif VITAL_NEON
      uint32x2_t sum = vpadd_u32(vget_low_u32(value), vget_high_u32(value));
      sum = vpadd_u32(sum, sum);
      return vget_lane_u32(sum, 0);
#endif
    }

    static force_inline uint32_t vector_call anyMask(simd_type value) {
#if VITAL_AVX2
      return _mm256_movemask_epi8(value);
#elif VITAL_SSE2
      return _mm_movemask_epi8(value);
#elif VITAL_NEON
      uint32x2_t max = vpmax_u32(vget_low_u32(value), vget_high_u32(value));
      max = vpmax_u32(max, max);
      return vget_lane_u32(max, 0);
#endif
    }

    static force_inline poly_int vector_call max(poly_int one, poly_int two) {
      return max(one.value, two.value);
    }

    static force_inline poly_int vector_call min(poly_int one, poly_int two) {
      return min(one.value, two.value);
    }

    static force_inline poly_int vector_call equal(poly_int one, poly_int two) {
      return equal(one.value, two.value);
    }

    static force_inline poly_int vector_call greaterThan(poly_int one, poly_int two) {
      return greaterThan(one.value, two.value);
    }

    static force_inline poly_int vector_call lessThan(poly_int one, poly_int two) {
      return greaterThan(two.value, one.value);
    }

    static force_inline uint32_t vector_call sum(poly_int value) {
      return sum(value.value);
    }

    simd_type value;

    force_inline poly_int() noexcept { value = init(0); }
    force_inline poly_int(simd_type initial_value) noexcept : value(initial_value) { }
    force_inline poly_int(uint32_t initial_value) noexcept {
      value = init(initial_value);
    }

    force_inline poly_int(uint32_t first, uint32_t second, uint32_t third, uint32_t fourth) noexcept {
      scalar_simd_union union_value { (int32_t)first, (int32_t)second, (int32_t)third, (int32_t)fourth };
      value = union_value.simd;
    }

    force_inline poly_int(uint32_t first, uint32_t second) noexcept : poly_int(first, second, first, second) { }

    force_inline ~poly_int() noexcept { }

    force_inline uint32_t vector_call access(size_t index) const noexcept {
#if VITAL_AVX2
      simd_union union_value { value };
      return union_value.scalar[index];
#elif VITAL_SSE2
      simd_scalar_union union_value { value };
      return union_value.scalar[index];
#elif VITAL_NEON
      return value[index];
#endif
    }

    force_inline void vector_call set(size_t index, uint32_t new_value) noexcept {
#if VITAL_AVX2
      simd_union union_value { value };
      union_value.scalar[index] = new_value;
      value = union_value.simd;
#elif VITAL_SSE2
      simd_scalar_union union_value { value };
      union_value.scalar[index] = new_value;
      value = union_value.simd;
#elif VITAL_NEON
      value[index] = new_value;
#endif
    }

    force_inline uint32_t vector_call operator[](size_t index) const noexcept {
      return access(index);
    }

    force_inline poly_int& vector_call operator+=(poly_int other) noexcept {
      value = add(value, other.value);
      return *this;
    }

    force_inline poly_int& vector_call operator-=(poly_int other) noexcept {
      value = sub(value, other.value);
      return *this;
    }

    force_inline poly_int& vector_call operator*=(poly_int other) noexcept {
      value = mul(value, other.value);
      return *this;
    }

    force_inline poly_int& vector_call operator&=(poly_int other) noexcept {
      value = bitAnd(value, other.value);
      return *this;
    }

    force_inline poly_int& vector_call operator|=(poly_int other) noexcept {
      value = bitOr(value, other.value);
      return *this;
    }

    force_inline poly_int& vector_call operator^=(poly_int other) noexcept {
      value = bitXor(value, other.value);
      return *this;
    }

    force_inline poly_int& vector_call operator+=(simd_type other) noexcept {
      value = add(value, other);
      return *this;
    }

    force_inline poly_int& vector_call operator-=(simd_type other) noexcept {
      value = sub(value, other);
      return *this;
    }

    force_inline poly_int& vector_call operator*=(simd_type other) noexcept {
      value = mul(value, other);
      return *this;
    }

    force_inline poly_int& vector_call operator&=(simd_type other) noexcept {
      value = bitAnd(value, other);
      return *this;
    }

    force_inline poly_int& vector_call operator|=(simd_type other) noexcept {
      value = bitOr(value, other);
      return *this;
    }

    force_inline poly_int& vector_call operator^=(simd_type other) noexcept {
      value = bitXor(value, other);
      return *this;
    }

    force_inline poly_int& vector_call operator+=(uint32_t scalar) noexcept {
      value = add(value, init(scalar));
      return *this;
    }

    force_inline poly_int& vector_call operator-=(uint32_t scalar) noexcept {
      value = sub(value, init(scalar));
      return *this;
    }

    force_inline poly_int& vector_call operator*=(uint32_t scalar) noexcept {
      value = mul(value, init(scalar));
      return *this;
    }

    force_inline poly_int vector_call operator+(poly_int other) const noexcept {
      return add(value, other.value);
    }

    force_inline poly_int vector_call operator-(poly_int other) const noexcept {
      return sub(value, other.value);
    }

    force_inline poly_int vector_call operator*(poly_int other) const noexcept {
      return mul(value, other.value);
    }

    force_inline poly_int vector_call operator&(poly_int other) const noexcept {
      return bitAnd(value, other.value);
    }

    force_inline poly_int vector_call operator|(poly_int other) const noexcept {
      return bitOr(value, other.value);
    }

    force_inline poly_int vector_call operator^(poly_int other) const noexcept {
      return bitXor(value, other.value);
    }

    force_inline poly_int vector_call operator-() const noexcept {
      return neg(value);
    }

    force_inline poly_int vector_call operator~() const noexcept {
      return bitNot(value);
    }

    force_inline uint32_t vector_call sum() const noexcept {
      return sum(value);
    }

    force_inline uint32_t vector_call anyMask() const noexcept {
      return anyMask(value);
    }
  };

  typedef poly_int poly_mask;

  struct poly_float {
#if VITAL_AVX2
    static constexpr size_t kSize = 8;
    typedef __m256 simd_type;
    typedef __m256i mask_simd_type;
#elif VITAL_SSE2
    static constexpr size_t kSize = 4;
    typedef __m128 simd_type;
    typedef __m128i mask_simd_type;
#elif VITAL_NEON
    static constexpr size_t kSize = 4;
    typedef float32x4_t simd_type;
    typedef uint32x4_t mask_simd_type;
#endif

    union simd_scalar_union {
      simd_type simd;
      float scalar[kSize];
    };

    union scalar_simd_union {
      float scalar[kSize];
      simd_type simd;
    };

    static force_inline mask_simd_type vector_call toMask(simd_type value) {
#if VITAL_AVX2
      return _mm256_castps_si256(value);
#elif VITAL_SSE2
      return _mm_castps_si128(value);
#elif VITAL_NEON
      return vreinterpretq_u32_f32(value);
#endif
    }

    static force_inline simd_type vector_call toSimd(mask_simd_type mask) {
#if VITAL_AVX2
      return _mm256_castsi256_ps(mask);
#elif VITAL_SSE2
      return _mm_castsi128_ps(mask);
#elif VITAL_NEON
      return vreinterpretq_f32_u32(mask);
#endif
    }

    static force_inline simd_type vector_call init(float scalar) {
#if VITAL_AVX2
      return _mm256_broadcast_ss(&scalar);
#elif VITAL_SSE2
      return _mm_set1_ps(scalar);
#elif VITAL_NEON
      return vdupq_n_f32(scalar);
#endif
    }

    static force_inline simd_type vector_call load(const float* memory) {
#if VITAL_AVX2
      return _mm256_loadu_ps(&scalar);
#elif VITAL_SSE2
      return _mm_loadu_ps(memory);
#elif VITAL_NEON
      return vld1q_f32(memory);
#endif
    }

    static force_inline simd_type vector_call add(simd_type one, simd_type two) {
#if VITAL_AVX2
      return _mm256_add_ps(one, two);
#elif VITAL_SSE2
      return _mm_add_ps(one, two);
#elif VITAL_NEON
      return vaddq_f32(one, two);
#endif
    }

    static force_inline simd_type vector_call sub(simd_type one, simd_type two) {
#if VITAL_AVX2
      return _mm256_sub_ps(one, two);
#elif VITAL_SSE2
      return _mm_sub_ps(one, two);
#elif VITAL_NEON
      return vsubq_f32(one, two);
#endif
    }

    static force_inline simd_type vector_call neg(simd_type value) {
#if VITAL_AVX2
      return _mm256_xor_ps(value, _mm256_set1_ps(-0.f));
#elif VITAL_SSE2
      return _mm_xor_ps(value, _mm_set1_ps(-0.f));
#elif VITAL_NEON
      return vmulq_n_f32(value, -1.0f);
#endif
    }

    static force_inline simd_type vector_call mul(simd_type one, simd_type two) {
#if VITAL_AVX2
      return _mm256_mul_ps(one, two);
#elif VITAL_SSE2
      return _mm_mul_ps(one, two);
#elif VITAL_NEON
      return vmulq_f32(one, two);
#endif
    }

    static force_inline simd_type vector_call mulScalar(simd_type value, float scalar) {
#if VITAL_AVX2
      return _mm256_mul_ps(value, _mm_set1_ps(scalar));
#elif VITAL_SSE2
      return _mm_mul_ps(value, _mm_set1_ps(scalar));
#elif VITAL_NEON
      return vmulq_n_f32(value, scalar);
#endif
    }

    static force_inline simd_type vector_call mulAdd(simd_type one, simd_type two, simd_type three) {
#if VITAL_AVX2
      return _mm256_fmadd_ps(two, three, one);
#elif VITAL_SSE2
      return _mm_add_ps(one, _mm_mul_ps(two, three));
#elif VITAL_NEON
#if defined(NEON_VFP_V3)
      return vaddq_f32(one, vmulq_f32(two, three));
#else
      return vmlaq_f32(one, two, three);
#endif
#endif
    }

    static force_inline simd_type vector_call mulSub(simd_type one, simd_type two, simd_type three) {
#if VITAL_AVX2
      return _mm256_fsub_ps(two, three, one);
#elif VITAL_SSE2
      return _mm_sub_ps(one, _mm_mul_ps(two, three));
#elif VITAL_NEON
#if defined(NEON_VFP_V3)
      return vsubq_f32(one, vmulq_f32(two, three));
#else
      return vmlsq_f32(one, two, three);
#endif
#endif
    }

    static force_inline simd_type vector_call div(simd_type one, simd_type two) {
#if VITAL_AVX2
      return _mm256_div_ps(one, two);
#elif VITAL_SSE2
      return _mm_div_ps(one, two);
#elif VITAL_NEON
#if defined(NEON_ARM32)
      simd_type reciprocal = vrecpeq_f32(two);
      reciprocal = vmulq_f32(vrecpsq_f32(two, reciprocal), reciprocal);
      reciprocal = vmulq_f32(vrecpsq_f32(two, reciprocal), reciprocal);
      return vmulq_f32(one, reciprocal);
#else
      return vdivq_f32(one, two);
#endif
#endif
    }

    static force_inline simd_type vector_call bitAnd(simd_type value, mask_simd_type mask) {
#if VITAL_AVX2
      return _mm256_and_ps(value, toSimd(mask));
#elif VITAL_SSE2
      return _mm_and_ps(value, toSimd(mask));
#elif VITAL_NEON
      return toSimd(vandq_u32(toMask(value), mask));
#endif
    }

    static force_inline simd_type vector_call bitOr(simd_type value, mask_simd_type mask) {
#if VITAL_AVX2
      return _mm256_or_ps(value, toSimd(mask));
#elif VITAL_SSE2
      return _mm_or_ps(value, toSimd(mask));
#elif VITAL_NEON
      return toSimd(vorrq_u32(toMask(value), mask));
#endif
    }

    static force_inline simd_type vector_call bitXor(simd_type value, mask_simd_type mask) {
#if VITAL_AVX2
      return _mm256_xor_ps(value, toSimd(mask));
#elif VITAL_SSE2
      return _mm_xor_ps(value, toSimd(mask));
#elif VITAL_NEON
      return toSimd(veorq_u32(toMask(value), mask));
#endif
    }

    static force_inline simd_type vector_call bitNot(simd_type value) {
      return bitXor(value, poly_mask::init(-1));
    }

    static force_inline simd_type vector_call max(simd_type one, simd_type two) {
#if VITAL_AVX2
      return _mm256_max_ps(one, two);
#elif VITAL_SSE2
      return _mm_max_ps(one, two);
#elif VITAL_NEON
      return vmaxq_f32(one, two);
#endif
    }

    static force_inline simd_type vector_call min(simd_type one, simd_type two) {
#if VITAL_AVX2
      return _mm256_min_ps(one, two);
#elif VITAL_SSE2
      return _mm_min_ps(one, two);
#elif VITAL_NEON
      return vminq_f32(one, two);
#endif
    }

    static force_inline simd_type vector_call abs(simd_type value) {
      return bitAnd(value, poly_mask::init(poly_mask::kNotSignMask));
    }

    static force_inline mask_simd_type vector_call sign_mask(simd_type value) {
      return toMask(bitAnd(value, poly_mask::init(poly_mask::kSignMask)));
    }

    static force_inline mask_simd_type vector_call equal(simd_type one, simd_type two) {
#if VITAL_AVX2
      return toMask(_mm256_cmpeq_ps(one, two, _CMP_EQ_OQ));
#elif VITAL_SSE2
      return toMask(_mm_cmpeq_ps(one, two));
#elif VITAL_NEON
      return vceqq_f32(one, two);
#endif
    }

    static force_inline mask_simd_type vector_call greaterThan(simd_type one, simd_type two) {
#if VITAL_AVX2
      return toMask(_mm256_cmp_ps(one, two, _CMP_GT_OQ));
#elif VITAL_SSE2
      return toMask(_mm_cmpgt_ps(one, two));
#elif VITAL_NEON
      return vcgtq_f32(one, two);
#endif
    }

    static force_inline mask_simd_type vector_call greaterThanOrEqual(simd_type one, simd_type two) {
#if VITAL_AVX2
      return toMask(_mm256_cmp_ps(one, two, _CMP_GE_OQ));
#elif VITAL_SSE2
      return toMask(_mm_cmpge_ps(one, two));
#elif VITAL_NEON
      return vcgeq_f32(one, two);
#endif
    }

    static force_inline mask_simd_type vector_call notEqual(simd_type one, simd_type two) {
#if VITAL_AVX2
      return toMask(_mm256_cmp_ps(one, two, _CMP_NEQ_OQ));
#elif VITAL_SSE2
      return toMask(_mm_cmpneq_ps(one, two));
#elif VITAL_NEON
      poly_mask greater = greaterThan(one, two);
      poly_mask less = lessThan(one, two);
      return poly_mask::bitOr(greater.value, less.value);
#endif
    }

    static force_inline float vector_call sum(simd_type value) {
#if VITAL_AVX2
      simd_type flip = _mm256_permute2f128_ps(value, value, 1)
      simd_type sum = _mm256_hadd_ps(value, flip);
      sum = _mm256_hadd_ps(sum, sum);
      return _mm256_cvtss_f32(sum);
#elif VITAL_SSE2
      simd_type flip = _mm_shuffle_ps(value, value, _MM_SHUFFLE(1, 0, 3, 2));
      simd_type sum = _mm_add_ps(value, flip);
      simd_type swap = _mm_shuffle_ps(sum, sum, _MM_SHUFFLE(2, 3, 0, 1));
      return _mm_cvtss_f32(_mm_add_ps(sum, swap));
#elif VITAL_NEON
      float32x2_t sum = vpadd_f32(vget_low_f32(value), vget_high_f32(value));
      sum = vpadd_f32(sum, sum);
      return vget_lane_f32(sum, 0);
#endif
    }

    static force_inline void vector_call transpose(simd_type& row0, simd_type& row1,
                                                   simd_type& row2, simd_type& row3) {
#if VITAL_AVX2
      static_assert(false, "AVX2 transpose not supported yet");
#elif VITAL_SSE2
      __m128 low0 = _mm_unpacklo_ps(row0, row1);
      __m128 low1 = _mm_unpacklo_ps(row2, row3);
      __m128 high0 = _mm_unpackhi_ps(row0, row1);
      __m128 high1 = _mm_unpackhi_ps(row2, row3);
      row0 = _mm_movelh_ps(low0, low1);
      row1 = _mm_movehl_ps(low1, low0);
      row2 = _mm_movelh_ps(high0, high1);
      row3 = _mm_movehl_ps(high1, high0);
#elif VITAL_NEON
      float32x4x2_t swap_low = vtrnq_f32(row0, row1);
      float32x4x2_t swap_high = vtrnq_f32(row2, row3);
      row0 = vextq_f32(vextq_f32(swap_low.val[0], swap_low.val[0], 2), swap_high.val[0], 2);
      row1 = vextq_f32(vextq_f32(swap_low.val[1], swap_low.val[1], 2), swap_high.val[1], 2);
      row2 = vextq_f32(swap_low.val[0], vextq_f32(swap_high.val[0], swap_high.val[0], 2), 2);
      row3 = vextq_f32(swap_low.val[1], vextq_f32(swap_high.val[1], swap_high.val[1], 2), 2);
#else
#endif
    }

    static force_inline poly_float vector_call mulAdd(poly_float one, poly_float two, poly_float three) {
      return mulAdd(one.value, two.value, three.value);
    }

    static force_inline poly_float vector_call mulSub(poly_float one, poly_float two, poly_float three) {
      return mulSub(one.value, two.value, three.value);
    }

    static force_inline poly_float vector_call max(poly_float one, poly_float two) {
      return max(one.value, two.value);
    }

    static force_inline poly_float vector_call min(poly_float one, poly_float two) {
      return min(one.value, two.value);
    }

    static force_inline poly_float vector_call abs(poly_float value) {
      return abs(value.value);
    }

    static force_inline poly_mask vector_call sign_mask(poly_float value) {
      return sign_mask(value.value);
    }

    static force_inline poly_mask vector_call equal(poly_float one, poly_float two) {
      return equal(one.value, two.value);
    }

    static force_inline poly_mask vector_call notEqual(poly_float one, poly_float two) {
      return notEqual(one.value, two.value);
    }

    static force_inline poly_mask vector_call greaterThan(poly_float one, poly_float two) {
      return greaterThan(one.value, two.value);
    }

    static force_inline poly_mask vector_call greaterThanOrEqual(poly_float one, poly_float two) {
      return greaterThanOrEqual(one.value, two.value);
    }

    static force_inline poly_mask vector_call lessThan(poly_float one, poly_float two) {
      return greaterThan(two.value, one.value);
    }

    static force_inline poly_mask vector_call lessThanOrEqual(poly_float one, poly_float two) {
      return greaterThanOrEqual(two.value, one.value);
    }

    simd_type value;

    force_inline poly_float() noexcept { value = init(0.0f); }
    force_inline poly_float(simd_type initial_value) noexcept : value(initial_value) { }
    force_inline poly_float(float initial_value) noexcept { value = init(initial_value); }

    force_inline poly_float(float initial_value1, float initial_value2) noexcept {
      scalar_simd_union union_value { initial_value1, initial_value2, initial_value1, initial_value2 };
      value = union_value.simd;
    }

    force_inline poly_float(float first, float second, float third, float fourth) noexcept {
      scalar_simd_union union_value { first, second, third, fourth };
      value = union_value.simd;
    }

    force_inline ~poly_float() noexcept { }

    force_inline float vector_call access(size_t index) const noexcept {
#if VITAL_AVX2
      simd_union union_value { value };
      return union_value.scalar[index];
#elif VITAL_SSE2
      simd_scalar_union union_value { value };
      return union_value.scalar[index];
#elif VITAL_NEON
      return value[index];
#endif
    }

    force_inline void vector_call set(size_t index, float new_value) noexcept {
#if VITAL_AVX2
      simd_union union_value { value };
      union_value.scalar[index] = new_value;
      value = union_value.simd;
#elif VITAL_SSE2
      simd_scalar_union union_value { value };
      union_value.scalar[index] = new_value;
      value = union_value.simd;
#elif VITAL_NEON
      value[index] = new_value;
#endif
    }

    force_inline float vector_call operator[](size_t index) const noexcept {
      return access(index);
    }

    force_inline poly_float& vector_call operator+=(poly_float other) noexcept {
      value = add(value, other.value);
      return *this;
    }

    force_inline poly_float& vector_call operator-=(poly_float other) noexcept {
      value = sub(value, other.value);
      return *this;
    }

    force_inline poly_float& vector_call operator*=(poly_float other) noexcept {
      value = mul(value, other.value);
      return *this;
    }

    force_inline poly_float& vector_call operator/=(poly_float other) noexcept {
      value = div(value, other.value);
      return *this;
    }

    force_inline poly_float& vector_call operator&=(poly_mask other) noexcept {
      value = bitAnd(value, other.value);
      return *this;
    }

    force_inline poly_float& vector_call operator|=(poly_mask other) noexcept {
      value = bitOr(value, other.value);
      return *this;
    }

    force_inline poly_float& vector_call operator^=(poly_mask other) noexcept {
      value = bitXor(value, other.value);
      return *this;
    }

    force_inline poly_float& vector_call operator+=(simd_type other) noexcept {
      value = add(value, other);
      return *this;
    }

    force_inline poly_float& vector_call operator-=(simd_type other) noexcept {
      value = sub(value, other);
      return *this;
    }

    force_inline poly_float& vector_call operator*=(simd_type other) noexcept {
      value = mul(value, other);
      return *this;
    }

    force_inline poly_float& vector_call operator/=(simd_type other) noexcept {
      value = div(value, other);
      return *this;
    }

    force_inline poly_float& vector_call operator&=(mask_simd_type other) noexcept {
      value = bitAnd(value, other);
      return *this;
    }

    force_inline poly_float& vector_call operator|=(mask_simd_type other) noexcept {
      value = bitOr(value, other);
      return *this;
    }

    force_inline poly_float& vector_call operator^=(mask_simd_type other) noexcept {
      value = bitXor(value, other);
      return *this;
    }

    force_inline poly_float& vector_call operator+=(float scalar) noexcept {
      value = add(value, init(scalar));
      return *this;
    }

    force_inline poly_float& vector_call operator-=(float scalar) noexcept {
      value = sub(value, init(scalar));
      return *this;
    }

    force_inline poly_float& vector_call operator*=(float scalar) noexcept {
      value = mulScalar(value, scalar);
      return *this;
    }

    force_inline poly_float& vector_call operator/=(float scalar) noexcept {
      value = div(value, init(scalar));
      return *this;
    }

    force_inline poly_float vector_call operator+(poly_float other) const noexcept {
      return add(value, other.value);
    }

    force_inline poly_float vector_call operator-(poly_float other) const noexcept {
      return sub(value, other.value);
    }

    force_inline poly_float vector_call operator*(poly_float other) const noexcept {
      return mul(value, other.value);
    }

    force_inline poly_float vector_call operator/(poly_float other) const noexcept {
      return div(value, other.value);
    }

    force_inline poly_float vector_call operator*(float scalar) const noexcept {
      return mulScalar(value, scalar);
    }

    force_inline poly_float vector_call operator&(poly_mask other) const noexcept {
      return bitAnd(value, other.value);
    }

    force_inline poly_float vector_call operator|(poly_mask other) const noexcept {
      return bitOr(value, other.value);
    }

    force_inline poly_float vector_call operator^(poly_mask other) const noexcept {
      return bitXor(value, other.value);
    }

    force_inline poly_float vector_call operator-() const noexcept {
      return neg(value);
    }

    force_inline poly_float vector_call operator~() const noexcept {
      return bitNot(value);
    }

    force_inline float vector_call sum() const noexcept {
      return sum(value);
    }
  };
} // namespace vital

