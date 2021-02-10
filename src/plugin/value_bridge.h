/* Copyright 2013-2019 Matt Tytel
 *
 * pylon is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * pylon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with pylon.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "JuceHeader.h"
#include "value.h"
#include "synth_parameters.h"

class ValueBridge : public AudioProcessorParameter {
  public:
    class Listener {
      public:
        virtual ~Listener() { }
        virtual void parameterChanged(std::string name, vital::mono_float value) = 0;
    };

    ValueBridge() = delete;

    ValueBridge(std::string name, vital::Value* value) :
        AudioProcessorParameter(), name_(name), value_(value), listener_(nullptr),
        source_changed_(false) {
      details_ = vital::Parameters::getDetails(name);
      span_ = details_.max - details_.min;
      if (details_.value_scale == vital::ValueDetails::kIndexed)
        span_ = std::round(span_);
    }

    float getValue() const override {
      return convertToPluginValue(value_->value());
    }

    void setValue(float value) override {
      if (listener_ && !source_changed_) {
        source_changed_ = true;
        vital::mono_float synth_value = convertToEngineValue(value);
        listener_->parameterChanged(name_.toStdString(), synth_value);
        source_changed_ = false;
      }
    }

    void setListener(Listener* listener) {
      listener_ = listener;
    }

    float getDefaultValue() const override {
      return convertToPluginValue(details_.default_value);
    }

    String getName(int maximumStringLength) const override {
      return String(details_.display_name).substring(0, maximumStringLength);
    }

    String getLabel() const override {
      return "";
    }

    String getText(float value, int maximumStringLength) const override {
      float adjusted = convertToEngineValue(value);
      String result = "";
      if (details_.string_lookup)
        result = details_.string_lookup[std::max<int>(0, std::min(adjusted, details_.max))];
      else {
        float display_value = details_.display_multiply * skewValue(adjusted) + details_.post_offset;
        result = String(display_value) + details_.display_units;
      }
      return result.substring(0, maximumStringLength).trim();
    }

    float getValueForText(const String &text) const override {
      return unskewValue(text.getFloatValue() / details_.display_multiply);
    }

    bool isAutomatable() const override {
      return true;
    }

    int getNumSteps() const override {
      if (isDiscrete())
        return 1 + (int)span_;
      return AudioProcessorParameter::getNumSteps();
    }

    bool isDiscrete() const override {
      static constexpr int kMaxIndexedSteps = 300;
      return details_.value_scale == vital::ValueDetails::kIndexed && span_ < kMaxIndexedSteps;
    }

    bool isBoolean() const override {
      return isDiscrete() && span_ == 1.0f;
    }

    // Converts internal value to value from 0.0 to 1.0.
    float convertToPluginValue(vital::mono_float synth_value) const {
      return (synth_value - details_.min) / span_;
    }

    // Converts from value from 0.0 to 1.0 to internal engine value.
    float convertToEngineValue(vital::mono_float plugin_value) const {
      float value = plugin_value * span_ + details_.min;

      if (details_.value_scale == vital::ValueDetails::kIndexed)
        return std::round(value);

      return value;
    }

    void setValueNotifyHost(float new_value) {
      if (!source_changed_) {
        source_changed_ = true;
        setValueNotifyingHost(new_value);
        source_changed_ = false;
      }
    }

  private:
    float getSkewedValue() const {
      return skewValue(value_->value());
    }

    float skewValue(float value) const {
      switch (details_.value_scale) {
        case vital::ValueDetails::kQuadratic:
          return value * value;
        case vital::ValueDetails::kCubic:
          return value * value * value;
        case vital::ValueDetails::kQuartic:
          value *= value;
          return value * value;
        case vital::ValueDetails::kExponential:
          if (details_.display_invert)
            return 1.0f / powf(2.0f, value);
          return powf(2.0f, value);
        case vital::ValueDetails::kSquareRoot:
          return sqrtf(value);
        default:
          return value;
      }
    }

    float unskewValue(float value) const {
      switch (details_.value_scale) {
        case vital::ValueDetails::kQuadratic:
          return sqrtf(value);
        case vital::ValueDetails::kCubic:
          return powf(value, 1.0f / 3.0f);
        case vital::ValueDetails::kQuartic:
          return powf(value, 1.0f / 4.0f);
        case vital::ValueDetails::kExponential:
          if (details_.display_invert)
            return log2(1.0f / value);
          return log2(value);
        default:
          return value;
      }
    }

    String name_;
    vital::ValueDetails details_;
    vital::mono_float span_;
    vital::Value* value_;
    Listener* listener_;
    bool source_changed_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ValueBridge)
};

