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

#include "wave_frame_test.h"
#include "wave_frame.h"

void WaveFrameTest::runTest() {
  testRandomTimeFrequencyConversion();
}

void WaveFrameTest::testRandomTimeFrequencyConversion() {
  static constexpr float kMaxError = 0.00001f;

  beginTest("Test Random Wave Frame Time Frequency Conversion");

  vital::WaveFrame wave_frame;
  vital::mono_float original[vital::WaveFrame::kWaveformSize];
  for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i) {
    original[i] = (2.0f * rand()) / RAND_MAX - 1.0f;
    wave_frame.time_domain[i] = original[i];
  }

  wave_frame.toFrequencyDomain();
  wave_frame.toTimeDomain();

  for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i) {
    float error = wave_frame.time_domain[i] - original[i];
    expect(error < kMaxError && -error < kMaxError, "Fourier Inverse gave big error.");
  }
}

static WaveFrameTest wave_frame_test;
