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
#include "sound_engine.h"
#include "synth_base.h"

class FullInterface;
class SynthSection;
class TestSynthBase;

class TestSynthBase : public SynthBase {
  public:
    TestSynthBase() : gui_interface_(nullptr) { }
    void setGuiInterface(SynthGuiInterface* gui_interface) { gui_interface_ = gui_interface; }
    const CriticalSection& getCriticalSection() override { return critical_section_; }
    void pauseProcessing(bool pause) override { 
      if (pause)
        critical_section_.enter();
      else
        critical_section_.exit();
    }
    SynthGuiInterface* getGuiInterface() override { return gui_interface_; }

    void process(AudioSampleBuffer* buffer, int channels, int samples, int offset) {
      ScopedLock lock(getCriticalSection());
      processAudio(buffer, channels, samples, offset);
    }

  private:
    CriticalSection critical_section_;
    SynthGuiInterface* gui_interface_;
};

class InterfaceTest : public UnitTest {
  public:
    InterfaceTest(std::string name) : UnitTest(name, "Interface") { }

    void runStressRandomTest(SynthSection* component, FullInterface* full_interface = nullptr);

    vital::SoundEngine* createSynthEngine() {
      synth_base_ = std::make_unique<TestSynthBase>();
      return synth_base_->getEngine();
    }

    SynthBase* getSynthBase() {
      return synth_base_.get();
    }

    vital::SoundEngine* getSynthEngine() {
      return synth_base_->getEngine();
    }

    void deleteSynthEngine() {
      synth_base_ = nullptr;
    }

  private:
    std::unique_ptr<TestSynthBase> synth_base_;
};

