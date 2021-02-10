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

#include "processor.h"
#include "futils.h"

namespace vital {

  class Distortion : public Processor {
    public:
      static constexpr mono_float kMaxDrive = 30.0f;
      static constexpr mono_float kMinDrive = -30.0f;
      static constexpr float kPeriodScale = 1.0f / 88200.0f;
      static constexpr mono_float kMinDistortionMult = 32.0f / INT_MAX;

      enum {
        kAudio,
        kType,
        kDrive,
        kNumInputs
      };

      enum {
        kAudioOut,
        kDriveOut,
        kNumOutputs
      };

      enum Type {
        kSoftClip,
        kHardClip,
        kLinearFold,
        kSinFold,
        kBitCrush,
        kDownSample,
        kNumTypes
      };

      static force_inline poly_float driveDbScale(poly_float db) {
        return futils::dbToMagnitude(utils::clamp(db, kMinDrive, kMaxDrive));
      }

      static force_inline poly_float bitCrushScale(poly_float db) {
        constexpr mono_float kDriveScale = 1.0f / (Distortion::kMaxDrive - Distortion::kMinDrive);

        poly_float drive = utils::max(db - kMinDrive, 0.0f) * kDriveScale;
        return utils::clamp(drive * drive, kMinDistortionMult, 1.0f);
      }

      static force_inline poly_float downSampleScale(poly_float db) {
        constexpr mono_float kDriveScale = 1.0f / (Distortion::kMaxDrive - Distortion::kMinDrive);

        poly_float drive = utils::max(db - Distortion::kMinDrive, 0.0f) * kDriveScale;
        drive = -drive + 1.0f;
        drive = poly_float(1.0f) / utils::clamp(drive * drive, Distortion::kMinDistortionMult, 1.0f);
        return utils::max(drive * 0.99f, 1.0f) * Distortion::kPeriodScale;
      }

      static poly_float getDriveValue(int type, poly_float input_drive) {
        if (type == kBitCrush)
          return bitCrushScale(input_drive);
        if (type == kDownSample)
          return downSampleScale(input_drive);
        return driveDbScale(input_drive);
      }

      static poly_float getDrivenValue(int type, poly_float value, poly_float drive);

      Distortion();
      virtual ~Distortion() { }

      virtual Processor* clone() const override {
        VITAL_ASSERT(false);
        return nullptr;
      }

      virtual void process(int num_samples) override;
      virtual void processWithInput(const poly_float* audio_in, int num_samples) override;

      template<poly_float(*distort)(poly_float, poly_float), poly_float(*scale)(poly_float)>
      void processTimeInvariant(int num_samples, const poly_float* audio_in, const poly_float* drive, 
                                poly_float* audio_out);

      void processDownSample(int num_samples, const poly_float* audio_in, const poly_float* drive,
                             poly_float* audio_out);

    private:
      poly_float last_distorted_value_;
      poly_float current_samples_;
      int type_;

      JUCE_LEAK_DETECTOR(Distortion)
  };
} // namespace vital

