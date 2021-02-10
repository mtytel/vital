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

#include "distortion.h"

#include "synth_constants.h"

#include <climits>

namespace vital {

  namespace {
    force_inline poly_float linearFold(poly_float value, poly_float drive) {
      poly_float adjust = value * drive * 0.25f + 0.75f;
      poly_float range = utils::mod(adjust);
      return poly_float::abs(range * -4.0f + 2.0f) - 1.0f;
    }

    force_inline poly_float sinFold(poly_float value, poly_float drive) {
      poly_float adjust = value * drive * -0.25f + 0.5f;
      poly_float range = utils::mod(adjust);
      return futils::sin1(range);
    }

    force_inline poly_float softClip(poly_float value, poly_float drive) {
      return futils::tanh(value * drive);
    }

    force_inline poly_float hardClip(poly_float value, poly_float drive) {
      return utils::clamp(value * drive, -1.0f, 1.0f);
    }

    force_inline poly_float bitCrush(poly_float value, poly_float drive) {
      return utils::round(value / drive) * drive;
    }

    force_inline int compactAudio(poly_float* audio_out, const poly_float* audio_in, int num_samples) {
      int num_full = num_samples / 2;
      for (int i = 0; i < num_full; ++i) {
        int in_index = 2 * i;
        audio_out[i] = utils::compactFirstVoices(audio_in[in_index], audio_in[in_index + 1]);
      }

      int num_remaining = num_samples % 2;

      if (num_remaining)
        audio_out[num_full] = audio_in[num_samples - 1];

      return num_full + num_remaining;
    }

    force_inline void expandAudio(poly_float* audio_out, const poly_float* audio_in, int num_samples) {
      int num_full = num_samples / 2;
      if (num_samples % 2)
        audio_out[num_samples - 1] = audio_in[num_full];

      for (int i = num_full - 1; i >= 0; --i) {
        int out_index = 2 * i;
        audio_out[out_index] = audio_in[i];
        audio_out[out_index + 1] = utils::swapVoices(audio_in[i]);
      }
    }
  } // namespace

  poly_float Distortion::getDrivenValue(int type, poly_float value, poly_float drive) {
    switch(type) {
      case kSoftClip:
        return softClip(value, drive);
      case kHardClip:
        return hardClip(value, drive);
      case kLinearFold:
        return linearFold(value, drive);
      case kSinFold:
        return sinFold(value, drive);
      case kBitCrush:
        return bitCrush(value, drive);
      case kDownSample:
        return bitCrush(value, poly_float(1.001f) - poly_float(kPeriodScale) / drive);
      default:
        return value;
    }
  }

  Distortion::Distortion() : Processor(kNumInputs, kNumOutputs),
                             last_distorted_value_(0.0f), current_samples_(0.0f), type_(kNumTypes) { }

  template<poly_float(*distort)(poly_float, poly_float), poly_float(*scale)(poly_float)>
  void Distortion::processTimeInvariant(int num_samples, const poly_float* audio_in, const poly_float* drive,
                                        poly_float* audio_out) {
    for (int i = 0; i < num_samples; ++i) {
      poly_float current_drive = scale(drive[i]);
      poly_float sample = audio_in[i];
      audio_out[i] = distort(sample, current_drive);
      VITAL_ASSERT(utils::isContained(audio_out[i]));
    }
  }

  void Distortion::processDownSample(int num_samples, const poly_float* audio_in, const poly_float* drive,
                                     poly_float* audio_out) {
    mono_float sample_rate = getSampleRate();
    poly_float current_samples = current_samples_;

    for (int i = 0; i < num_samples; ++i) {
      poly_float current_period = downSampleScale(drive[i]) * sample_rate; 
      current_samples += 1.0f;

      poly_float current_sample = audio_in[i];
      poly_float current_downsample = current_sample & constants::kFirstMask;
      current_downsample += utils::swapVoices(current_downsample);

      poly_mask update = poly_float::greaterThanOrEqual(current_samples, current_period);
      last_distorted_value_ = utils::maskLoad(last_distorted_value_, current_downsample, update);
      current_samples = utils::maskLoad(current_samples, current_samples - current_period, update);
      audio_out[i] = last_distorted_value_;
    }

    current_samples_ = current_samples;
  }

  void Distortion::processWithInput(const poly_float* audio_in, int num_samples) {
    VITAL_ASSERT(checkInputAndOutputSize(num_samples));

    int type = static_cast<int>(input(kType)->at(0)[0]);
    poly_float* audio_out = output(kAudioOut)->buffer;
    poly_float* drive_out = output(kDriveOut)->buffer;

    int compact_samples = compactAudio(audio_out, audio_in, num_samples);
    compactAudio(drive_out, input(kDrive)->source->buffer, num_samples);

    if (type != type_) {
      type_ = type;
      last_distorted_value_ = 0.0f;
      current_samples_ = 0.0f;
    }

    switch(type) {
      case kSoftClip:
        processTimeInvariant<softClip, driveDbScale>(compact_samples, audio_out, drive_out, audio_out);
        break;
      case kHardClip:
        processTimeInvariant<hardClip, driveDbScale>(compact_samples, audio_out, drive_out, audio_out);
        break;
      case kLinearFold:
        processTimeInvariant<linearFold, driveDbScale>(compact_samples, audio_out, drive_out, audio_out);
        break;
      case kSinFold:
        processTimeInvariant<sinFold, driveDbScale>(compact_samples, audio_out, drive_out, audio_out);
        break;
      case kBitCrush:
        processTimeInvariant<bitCrush, bitCrushScale>(compact_samples, audio_out, drive_out, audio_out);
        break;
      case kDownSample:
        processDownSample(compact_samples, audio_out, drive_out, audio_out);
        break;
      default:
        utils::copyBuffer(audio_out, audio_in, num_samples);
        return;
    }

    expandAudio(audio_out, audio_out, num_samples);
  }

  void Distortion::process(int num_samples) {
    VITAL_ASSERT(inputMatchesBufferSize(kAudio));
    processWithInput(input(kAudio)->source->buffer, num_samples);
  }
} // namespace vital
