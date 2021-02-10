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

#include "formant_filter.h"

#include "digital_svf.h"
#include "formant_manager.h"
#include "operators.h"
#include "synth_constants.h"

namespace vital {

  namespace {
    struct FormantValues {
      cr::Value gain;
      cr::Value resonance;
      cr::Value midi_cutoff;
    };

    const FormantValues formant_a[kNumFormants] = {
      {cr::Value(-2), cr::Value(0.66f), cr::Value(75.7552343327f)},
      {cr::Value(-8), cr::Value(0.75f), cr::Value(84.5454706023f)},
      {cr::Value(-9), cr::Value(1.0f), cr::Value(100.08500317f)},
      {cr::Value(-10), cr::Value(1.0f), cr::Value(101.645729657f)},
    };

    const FormantValues formant_e[kNumFormants] = {
      {cr::Value(0), cr::Value(0.66f), cr::Value(67.349957715f)},
      {cr::Value(-14), cr::Value(0.75f), cr::Value(92.39951181f)},
      {cr::Value(-4), cr::Value(1.0f), cr::Value(99.7552343327f)},
      {cr::Value(-14), cr::Value(1.0f), cr::Value(103.349957715f)},
    };

    const FormantValues formant_i[kNumFormants] = {
      {cr::Value(0), cr::Value(0.8f), cr::Value(61.7825925179f)},
      {cr::Value(-15), cr::Value(0.75f), cr::Value(94.049554095f)},
      {cr::Value(-17), cr::Value(1.0f), cr::Value(101.03821678f)},
      {cr::Value(-20), cr::Value(1.0f), cr::Value(103.618371471f)},
    };

    const FormantValues formant_o[kNumFormants] = {
      {cr::Value(-2), cr::Value(0.7f), cr::Value(67.349957715f)},
      {cr::Value(-6), cr::Value(0.75f), cr::Value(79.349957715f)},
      {cr::Value(-14), cr::Value(1.0f), cr::Value(99.7552343327f)},
      {cr::Value(-14), cr::Value(1.0f), cr::Value(101.03821678f)},
    };

    const FormantValues formant_u[kNumFormants] = {
      {cr::Value(0), cr::Value(0.7f), cr::Value(65.0382167797f)},
      {cr::Value(-20), cr::Value(0.75f), cr::Value(74.3695077237f)},
      {cr::Value(-17), cr::Value(1.0f), cr::Value(100.408607741f)},
      {cr::Value(-14), cr::Value(1.0f), cr::Value(101.645729657f)},
    };

    enum FormantPosition {
      kBottomLeft,
      kBottomRight,
      kTopLeft,
      kTopRight,
      kNumFormantPositions
    };

    const FormantValues* formant_style1[kNumFormantPositions] = {
      formant_a, formant_o, formant_i, formant_e
    };

    const FormantValues* formant_style2[kNumFormantPositions] = {
      formant_a, formant_i, formant_u, formant_o
    };

    const FormantValues** formant_styles[FormantFilter::kNumFormantStyles] = {
      formant_style2,
      formant_style1,
    };
  } // namespace

  namespace {
    poly_float bilinearInterpolate(poly_float top_left, poly_float top_right,
                                   poly_float bot_left, poly_float bot_right,
                                   poly_float x, poly_float y) {
      poly_float top = vital::utils::interpolate(top_left, top_right, x);
      poly_float bot = vital::utils::interpolate(bot_left, bot_right, x);
      return vital::utils::interpolate(bot, top, y);
    }

    SynthFilter::FilterState interpolateFormants(const FormantValues& top_left,
                                                 const FormantValues& top_right,
                                                 const FormantValues& bot_left,
                                                 const FormantValues& bot_right,
                                                 poly_float formant_x, poly_float formant_y) {
      SynthFilter::FilterState filter_state;
      filter_state.midi_cutoff = bilinearInterpolate(top_left.midi_cutoff.value(),
                                                     top_right.midi_cutoff.value(),
                                                     bot_left.midi_cutoff.value(),
                                                     bot_right.midi_cutoff.value(),
                                                     formant_x, formant_y);
      filter_state.resonance_percent = bilinearInterpolate(top_left.resonance.value(),
                                                           top_right.resonance.value(),
                                                           bot_left.resonance.value(),
                                                           bot_right.resonance.value(),
                                                           formant_x, formant_y);
      filter_state.gain = bilinearInterpolate(top_left.gain.value(),
                                              top_right.gain.value(),
                                              bot_left.gain.value(),
                                              bot_right.gain.value(),
                                              formant_x, formant_y);
      return filter_state;
    }
  } // namespace

  FormantFilter::FormantFilter(int style) :
      ProcessorRouter(kNumInputs, 1), formant_manager_(nullptr), style_(style) {
    formant_manager_ = new FormantManager(kNumFormants);
    addProcessor(formant_manager_);
  }

  void FormantFilter::init() {
    static const cr::Value k12Db(DigitalSvf::k12Db);

    formant_manager_->useOutput(output());
    Value* center = new Value(kCenterMidi);
    addIdleProcessor(center);

    for (int i = 0; i < kNumFormants; ++i) {
      cr::BilinearInterpolate* formant_gain = new cr::BilinearInterpolate();
      cr::BilinearInterpolate* formant_q = new cr::BilinearInterpolate();
      BilinearInterpolate* formant_midi = new BilinearInterpolate();

      for (int p = 0; p < kNumFormantPositions; ++p) {
        formant_gain->plug(&formant_styles[style_][p][i].gain, cr::BilinearInterpolate::kPositionStart + p);
        formant_q->plug(&formant_styles[style_][p][i].resonance, cr::BilinearInterpolate::kPositionStart + p);
        formant_midi->plug(&formant_styles[style_][p][i].midi_cutoff, cr::BilinearInterpolate::kPositionStart + p);
      }

      formant_gain->useInput(input(kInterpolateX), cr::BilinearInterpolate::kXPosition);
      formant_q->useInput(input(kInterpolateX), cr::BilinearInterpolate::kXPosition);
      formant_midi->useInput(input(kInterpolateX), BilinearInterpolate::kXPosition);

      formant_gain->useInput(input(kInterpolateY), cr::BilinearInterpolate::kYPosition);
      formant_q->useInput(input(kInterpolateY), cr::BilinearInterpolate::kYPosition);
      formant_midi->useInput(input(kInterpolateY), BilinearInterpolate::kYPosition);

      Interpolate* formant_midi_spread = new Interpolate();
      formant_midi_spread->useInput(input(kSpread), Interpolate::kFractional);
      formant_midi_spread->useInput(input(kReset), Interpolate::kReset);
      formant_midi_spread->plug(center, Interpolate::kTo);
      formant_midi_spread->plug(formant_midi, Interpolate::kFrom);

      Add* formant_midi_adjust = new Add();
      formant_midi_adjust->useInput(input(kTranspose), 0);
      formant_midi_adjust->plug(formant_midi_spread, 1);

      cr::Multiply* formant_q_adjust = new cr::Multiply();
      formant_q_adjust->useInput(input(kResonance), 0);
      formant_q_adjust->plug(formant_q, 1);

      formant_manager_->getFormant(i)->useInput(input(kAudio), DigitalSvf::kAudio);
      formant_manager_->getFormant(i)->useInput(input(kReset), DigitalSvf::kReset);
      formant_manager_->getFormant(i)->plug(&k12Db, DigitalSvf::kStyle);
      formant_manager_->getFormant(i)->plug(&constants::kValueOne, DigitalSvf::kPassBlend);
      formant_manager_->getFormant(i)->plug(formant_gain, DigitalSvf::kGain);
      formant_manager_->getFormant(i)->plug(formant_q_adjust, DigitalSvf::kResonance);
      formant_manager_->getFormant(i)->plug(formant_midi_adjust, DigitalSvf::kMidiCutoff);

      addProcessor(formant_gain);
      addProcessor(formant_q);
      addProcessor(formant_q_adjust);
      addProcessor(formant_midi);
      addProcessor(formant_midi_spread);
      addProcessor(formant_midi_adjust);
    }

    ProcessorRouter::init();
  }

  void FormantFilter::setupFilter(const FilterState& filter_state) {
    int style = std::min(filter_state.style, FormantFilter::kNumFormantStyles - 1);
    for (int i = 0; i < vital::kNumFormants; ++i) {
      FilterState formant_setting = interpolateFormants(formant_styles[style][kTopLeft][i],
                                                        formant_styles[style][kTopRight][i],
                                                        formant_styles[style][kBottomLeft][i],
                                                        formant_styles[style][kBottomRight][i],
                                                        filter_state.interpolate_x,
                                                        filter_state.interpolate_y);

      vital::DigitalSvf* formant = formant_manager_->getFormant(i);
      formant_setting.midi_cutoff = utils::interpolate(formant_setting.midi_cutoff, kCenterMidi,
                                                       filter_state.pass_blend);
      formant_setting.midi_cutoff += filter_state.transpose;
      formant_setting.resonance_percent *= filter_state.resonance_percent;
      formant_setting.style = DigitalSvf::k12Db;
      formant_setting.pass_blend = 1.0f;
      formant->setupFilter(formant_setting);
    }
  }

  void FormantFilter::reset(poly_mask reset_mask) {
    getLocalProcessor(formant_manager_)->reset(reset_mask);
  }

  void FormantFilter::hardReset() {
    getLocalProcessor(formant_manager_)->hardReset();
  }
} // namespace vital
