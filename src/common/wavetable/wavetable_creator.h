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

#include "JuceHeader.h"
#include "wave_frame.h"
#include "wavetable_group.h"
#include "file_source.h"
#include "json/json.h"
#include "wavetable.h"

using json = nlohmann::json;

class LineGenerator;

class WavetableCreator {
  public:
    enum AudioFileLoadStyle {
      kNone,
      kWavetableSplice,
      kVocoded,
      kTtwt,
      kPitched,
      kNumDragLoadStyles
    };

    WavetableCreator(vital::Wavetable* wavetable) : wavetable_(wavetable),
                                                    full_normalize_(true), remove_all_dc_(true) { }
  
    int getGroupIndex(WavetableGroup* group);
    void addGroup(WavetableGroup* group) { groups_.push_back(std::unique_ptr<WavetableGroup>(group)); }
    void removeGroup(int index);
    void moveUp(int index);
    void moveDown(int index);

    int numGroups() const { return static_cast<int>(groups_.size()); }
    WavetableGroup* getGroup(int index) const { return groups_[index].get(); }
    float render(int position);
    void render();
    void postRender(float max_span);
    void renderToBuffer(float* buffer, int num_frames, int frame_size);
    void init();
    void clear();
    void loadDefaultCreator();

    void initPredefinedWaves();
    void initFromAudioFile(const float* audio_buffer, int num_samples, int sample_rate,
                           AudioFileLoadStyle load_style, FileSource::FadeStyle fade_style);

    void setName(const std::string& name) { wavetable_->setName(name); }
    void setAuthor(const std::string& author) { wavetable_->setAuthor(author); }
    void setFileLoaded(const std::string& path) { last_file_loaded_ = path; }
    std::string getName() const { return wavetable_->getName(); }
    std::string getAuthor() const { return wavetable_->getAuthor(); }
    std::string getLastFileLoaded() { return last_file_loaded_; }

    static bool isValidJson(json data);
    json updateJson(json data);
    json stateToJson();
    void jsonToState(json data);

    vital::Wavetable* getWavetable() { return wavetable_; }

  protected:
    void initFromSplicedAudioFile(const float* audio_buffer, int num_samples, int sample_rate,
                                  FileSource::FadeStyle fade_style);
    void initFromVocodedAudioFile(const float* audio_buffer, int num_samples, int sample_rate, bool ttwt);
    void initFromPitchedAudioFile(const float* audio_buffer, int num_samples, int sample_rate);
    void initFromLineGenerator(LineGenerator* line_generator);

    vital::WaveFrame compute_frame_combine_;
    vital::WaveFrame compute_frame_;
    std::vector<std::unique_ptr<WavetableGroup>> groups_;

    std::string last_file_loaded_;
    vital::Wavetable* wavetable_;
    bool full_normalize_;
    bool remove_all_dc_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableCreator)
};

