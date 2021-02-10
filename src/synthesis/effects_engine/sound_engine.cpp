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

#include "sound_engine.h"

#include "compressor_module.h"
#include "flanger_module.h"
#include "phaser_module.h"
#include "decimator.h"
#include "modulation_connection_processor.h"
#include "synth_constants.h"
#include "effects_modulation_handler.h"
#include "peak_meter.h"
#include "operators.h"
#include "reorderable_effect_chain.h"
#include "value_switch.h"

namespace vital {

  SoundEngine::SoundEngine() : SynthModule(0, 1), modulation_handler_(nullptr), 
                               last_oversampling_amount_(-1), last_sample_rate_(-1), peak_meter_(nullptr) {
    init();
    bps_ = data_->controls["beats_per_minute"];
    modulation_processors_.reserve(kMaxModulationConnections);
  }

  SoundEngine::~SoundEngine() {
    modulation_handler_->prepareDestroy();
  }

  void SoundEngine::init() {
    createBaseControl("bypass");
    oversampling_ = createBaseControl("oversampling");
    legato_ = createBaseControl("legato");

    Output* stereo_routing = createMonoModControl("stereo_routing");
    Value* stereo_mode = createBaseControl("stereo_mode");
    Output* beats_per_second = createMonoModControl("beats_per_minute");
    cr::LowerBound* beats_per_second_clamped = new cr::LowerBound(0.0f);
    beats_per_second_clamped->plug(beats_per_second);
    addProcessor(beats_per_second_clamped);

    Output* polyphony = createMonoModControl("polyphony");
    Value* voice_priority = createBaseControl("voice_priority");
    Value* voice_override = createBaseControl("voice_override");

    modulation_handler_ = new EffectsModulationHandler(beats_per_second_clamped->output());
    addSubmodule(modulation_handler_);
    modulation_handler_->setPolyphony(vital::kMaxPolyphony);
    modulation_handler_->plug(polyphony, VoiceHandler::kPolyphony);
    modulation_handler_->plug(voice_priority, VoiceHandler::kVoicePriority);
    modulation_handler_->plug(voice_override, VoiceHandler::kVoiceOverride);

    addProcessor(modulation_handler_);

    Value* pitch_wheel = createBaseControl("pitch_wheel");
    modulation_handler_->setPitchWheelControl(pitch_wheel);
    Value* mod_wheel = createBaseControl("mod_wheel");
    modulation_handler_->setModWheelControl(mod_wheel);

    upsampler_ = new Upsampler();
    addIdleProcessor(upsampler_);

    Value* effect_chain_order = createBaseControl("effect_chain_order");
    effect_chain_ = new ReorderableEffectChain(beats_per_second, modulation_handler_->midi_offset_output());
    addSubmodule(effect_chain_);
    addProcessor(effect_chain_);
    effect_chain_->plug(upsampler_, ReorderableEffectChain::kAudio);
    effect_chain_->plug(effect_chain_order, ReorderableEffectChain::kOrder);

    SynthModule* compressor = effect_chain_->getEffect(constants::kCompressor);
    createStatusOutput("compressor_low_input", compressor->output(CompressorModule::kLowInputMeanSquared));
    createStatusOutput("compressor_band_input", compressor->output(CompressorModule::kBandInputMeanSquared));
    createStatusOutput("compressor_high_input", compressor->output(CompressorModule::kHighInputMeanSquared));
    createStatusOutput("compressor_low_output", compressor->output(CompressorModule::kLowOutputMeanSquared));
    createStatusOutput("compressor_band_output", compressor->output(CompressorModule::kBandOutputMeanSquared));
    createStatusOutput("compressor_high_output", compressor->output(CompressorModule::kHighOutputMeanSquared));

    SynthModule* chorus = effect_chain_->getEffect(constants::kChorus);
    for (int i = 0; i < ChorusModule::kMaxDelayPairs; ++i)
      createStatusOutput("chorus_delays" + std::to_string(i + 1), chorus->output(i + 1));

    SynthModule* phaser = effect_chain_->getEffect(constants::kPhaser);
    createStatusOutput("phaser_cutoff", phaser->output(PhaserModule::kCutoffOutput));

    SynthModule* flanger = effect_chain_->getEffect(constants::kFlanger);
    createStatusOutput("flanger_delay_frequency", flanger->output(FlangerModule::kFrequencyOutput));

    Decimator* decimator = new Decimator(3);
    decimator->plug(effect_chain_);
    addProcessor(decimator);

    StereoEncoder* decoder = new StereoEncoder(true);
    decoder->plug(decimator, StereoEncoder::kAudio);
    decoder->plug(stereo_routing, StereoEncoder::kEncodingValue);
    decoder->plug(stereo_mode, StereoEncoder::kMode);
    addProcessor(decoder);

    Output* volume = createMonoModControl("volume");
    SmoothVolume* scaled_audio = new SmoothVolume();
    scaled_audio->plug(decoder, SmoothVolume::kAudioRate);
    scaled_audio->plug(volume, SmoothVolume::kDb);

    peak_meter_ = new PeakMeter();
    peak_meter_->plug(scaled_audio, 0);
    createStatusOutput("peak_meter", peak_meter_->output());

    Clamp* clamp = new Clamp(-2.1f, 2.1f);
    clamp->plug(scaled_audio);
    
    addProcessor(peak_meter_);
    addProcessor(scaled_audio);

    addProcessor(clamp);
    clamp->useOutput(output());

    SynthModule::init();
    disableUnnecessaryModSources();
    setOversamplingAmount(kDefaultOversamplingAmount, kDefaultSampleRate);
  }

  void SoundEngine::connectModulation(const modulation_change& change) {
    change.modulation_processor->plug(change.source, ModulationConnectionProcessor::kModulationInput);
    change.modulation_processor->setDestinationScale(change.destination_scale);
    VITAL_ASSERT(vital::utils::isFinite(change.destination_scale));

    Processor* destination = change.mono_destination;
    bool polyphonic = change.source->owner->isPolyphonic() && change.poly_destination;
    change.modulation_processor->setPolyphonicModulation(polyphonic);
    if (polyphonic)
      destination = change.poly_destination;

    if (!destination->isControlRate() && !change.source->isControlRate()) {
      change.source->owner->setControlRate(false);
      change.modulation_processor->setControlRate(false);
    }

    change.source->owner->enable(true);
    change.modulation_processor->enable(true);
    destination->plugNext(change.modulation_processor);
    change.modulation_processor->process(1);
    destination->process(1);

    change.mono_modulation_switch->set(1);
    if (change.poly_modulation_switch)
      change.poly_modulation_switch->set(1);

    modulation_processors_.push_back(change.modulation_processor);
  }

  int SoundEngine::getNumPressedNotes() {
    return modulation_handler_->getNumPressedNotes();
  }

  void SoundEngine::disconnectModulation(const modulation_change& change) {
    Processor* destination = change.mono_destination;
    if (change.source->owner->isPolyphonic() && change.poly_destination)
      destination = change.poly_destination;

    destination->unplug(change.modulation_processor);

    if (change.mono_destination->connectedInputs() == 1 &&
        (change.poly_destination == nullptr || change.poly_destination->connectedInputs() == 0)) {
      change.mono_modulation_switch->set(0);

      if (change.poly_modulation_switch)
        change.poly_modulation_switch->set(0);
    }

    change.modulation_processor->enable(false);
    change.modulation_processor->setControlRate(true);
    if (change.num_audio_rate == 0)
      change.source->owner->setControlRate(true);
    modulation_processors_.remove(change.modulation_processor);
  }

  int SoundEngine::getNumActiveVoices() {
    return modulation_handler_->getNumActiveVoices();
  }

  ModulationConnectionBank& SoundEngine::getModulationBank() {
    return modulation_handler_->getModulationBank();
  }

  mono_float SoundEngine::getLastActiveNote() const {
    return modulation_handler_->getLastActiveNote();
  }

  void SoundEngine::setTuning(const Tuning* tuning) {
    modulation_handler_->setTuning(tuning);
  }

  void SoundEngine::checkOversampling() {
    int oversampling_amount = oversampling_->value();
    int sample_rate = getSampleRate();
    if (last_oversampling_amount_ != oversampling_amount || last_sample_rate_ != sample_rate) {
      setOversamplingAmount(1 << oversampling_amount, sample_rate);
      last_oversampling_amount_ = oversampling_amount;
      last_sample_rate_ = sample_rate;
    }
  }

  void SoundEngine::setOversamplingAmount(int oversampling_amount, int sample_rate) {
    static constexpr int kBaseSampleRate = 44100;
    int oversample = oversampling_amount;
    int sample_rate_mult = sample_rate / kBaseSampleRate;
    while (sample_rate_mult > 1 && oversample > 1) {
      sample_rate_mult >>= 1;
      oversample >>= 1;
    }

    upsampler_->setOversampleAmount(oversample);
    modulation_handler_->setOversampleAmount(oversample);
    effect_chain_->setOversampleAmount(oversample);
  }

  void SoundEngine::processWithInput(const poly_float* audio_in, int num_samples) {
    VITAL_ASSERT(num_samples <= output()->buffer_size);

    FloatVectorOperations::disableDenormalisedNumberSupport();
    modulation_handler_->setLegato(legato_->value());

    upsampler_->processWithInput(audio_in, num_samples);
    ProcessorRouter::process(num_samples);

    for (auto& status_source : data_->status_outputs)
      status_source.second->update();
  }

  void SoundEngine::correctToTime(double seconds) {
    modulation_handler_->correctToTime(seconds);
    effect_chain_->correctToTime(seconds);
  }

  void SoundEngine::allSoundsOff() {
    effect_chain_->hardReset();
  }

  void SoundEngine::allNotesOff(int sample) {
    modulation_handler_->allNotesOff(sample);
  }

  void SoundEngine::allNotesOff(int sample, int channel) {
    modulation_handler_->allNotesOff(channel);
  }

  void SoundEngine::allNotesOffRange(int sample, int from_channel, int to_channel) {
    modulation_handler_->allNotesOffRange(sample, from_channel, to_channel);
  }

  void SoundEngine::noteOn(int note, mono_float velocity, int sample, int channel) {
    modulation_handler_->noteOn(note, velocity, sample, channel);
  }

  void SoundEngine::noteOff(int note, mono_float lift, int sample, int channel) {
    modulation_handler_->noteOff(note, lift, sample, channel);
  }

  void SoundEngine::setModWheel(mono_float value, int channel) {
    modulation_handler_->setModWheel(value, channel);
  }

  void SoundEngine::setModWheelAllChannels(mono_float value) {
    modulation_handler_->setModWheelAllChannels(value);
  }
  
  void SoundEngine::setPitchWheel(mono_float value, int channel) {
    modulation_handler_->setPitchWheel(value, channel);
  }

  void SoundEngine::setZonedPitchWheel(mono_float value, int from_channel, int to_channel) {
    modulation_handler_->setZonedPitchWheel(value, from_channel, to_channel);
  }

  void SoundEngine::disableUnnecessaryModSources() {
    modulation_handler_->disableUnnecessaryModSources();
  }

  void SoundEngine::enableModSource(const std::string& source) {
    getModulationSource(source)->owner->enable(true);
  }

  void SoundEngine::disableModSource(const std::string& source) {
    modulation_handler_->disableModSource(source);
  }

  bool SoundEngine::isModSourceEnabled(const std::string& source) {
    return getModulationSource(source)->owner->enabled();
  }

  const StereoMemory* SoundEngine::getEqualizerMemory() {
    return effect_chain_->getEqualizerMemory();
  }

  void SoundEngine::setAftertouch(mono_float note, mono_float value, int sample, int channel) {
    modulation_handler_->setAftertouch(note, value, sample, channel);
  }

  void SoundEngine::setChannelAftertouch(int channel, mono_float value, int sample) {
    modulation_handler_->setChannelAftertouch(channel, value, sample);
  }

  void SoundEngine::setChannelRangeAftertouch(int from_channel, int to_channel, mono_float value, int sample) {
    modulation_handler_->setChannelRangeAftertouch(from_channel, to_channel, value, sample);
  }

  void SoundEngine::setChannelSlide(int channel, mono_float value, int sample) {
    modulation_handler_->setChannelSlide(channel, value, sample);
  }

  void SoundEngine::setChannelRangeSlide(int from_channel, int to_channel, mono_float value, int sample) {
    modulation_handler_->setChannelRangeSlide(from_channel, to_channel, value, sample);
  }

  void SoundEngine::setBpm(mono_float bpm) {
    mono_float bps = bpm / 60.0f;
    if (bps_->value() != bps)
      bps_->set(bps);
  }

  Wavetable* SoundEngine::getWavetable(int index) {
    return nullptr;
  }

  Sample* SoundEngine::getSample() {
    return nullptr;
  }

  LineGenerator* SoundEngine::getLfoSource(int index) {
    return modulation_handler_->getLfoSource(index);
  }

  void SoundEngine::sustainOn(int channel) {
    modulation_handler_->sustainOn(channel);
  }

  void SoundEngine::sustainOff(int sample, int channel) {
    modulation_handler_->sustainOff(sample, channel);
  }

  void SoundEngine::sostenutoOn(int channel) {
    modulation_handler_->sostenutoOn(channel);
  }

  void SoundEngine::sostenutoOff(int sample, int channel) {
    modulation_handler_->sostenutoOff(sample, channel);
  }

  void SoundEngine::sustainOnRange(int from_channel, int to_channel) {
    modulation_handler_->sustainOnRange(from_channel, to_channel);
  }

  void SoundEngine::sustainOffRange(int sample, int from_channel, int to_channel) {
    modulation_handler_->sustainOffRange(sample, from_channel, to_channel);
  }

  void SoundEngine::sostenutoOnRange(int from_channel, int to_channel) {
    modulation_handler_->sostenutoOnRange(from_channel, to_channel);
  }

  void SoundEngine::sostenutoOffRange(int sample, int from_channel, int to_channel) {
    modulation_handler_->sostenutoOffRange(sample, from_channel, to_channel);
  }
} // namespace vital
