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

#include "interface_test.h"
#include "filter_section.h"
#include "default_look_and_feel.h"
#include "full_interface.h"
#include "preset_selector.h"
#include "sound_engine.h"
#include "synth_base.h"
#include "synth_gui_interface.h"
#include "synth_slider.h"
#include "skin.h"

namespace {
  template<class T>
  std::vector<T*> getAllComponentsOfType(Component* component) {
    std::vector<T*> results;
    Array<Component*> children = component->getChildren();
    for (Component* child : children) {
      T* result = dynamic_cast<T*>(child);
      if (result)
        results.push_back(result);
      else {
        std::vector<T*> sub_results = getAllComponentsOfType<T>(child);
        results.insert(results.end(), sub_results.begin(), sub_results.end());
      }
    }
    return results;
  }

  class TestFullInterface : public FullInterface {
    public:
      TestFullInterface() { }

      void resized() override {
        SynthSection::resized();
        redoBackground();
      }

      void paintBackground(Graphics& g) override {
        paintChildrenBackgrounds(g);
      }
  };

  class TestTopComponent : public Component, public Timer {
    public:
      static constexpr float kMsBetweenUpdates = 10.0f;
      static constexpr float kSliderRatioChangesPerUpdate = 0.2f;
      static constexpr int kButtonTries = 32;

      TestTopComponent(FullInterface* full_interface) : Component("Test Top Component"), test_section_(nullptr) {
        if (full_interface == nullptr)
          full_interface_ = std::make_unique<TestFullInterface>();
        else {
          full_interface->reset();
          full_interface_.reset(full_interface);
        }
        addAndMakeVisible(full_interface_.get());

        startTimer(kMsBetweenUpdates);
      }

      TestTopComponent() : TestTopComponent(nullptr) { }

      ~TestTopComponent() {
        stopTimer();
      }

      FullInterface* full_interface() {
        return full_interface_.get();
      }

      void addTestSection(SynthSection* section) {
        test_section_ = section;

        if (full_interface_.get() != section)
          full_interface_->addSubSection(section);

        full_interface_->redoBackground();
        startTimer(kMsBetweenUpdates);
      }

      void resized() override {
        Component::resized();
        full_interface_->setBounds(getLocalBounds());
        startTimer(kMsBetweenUpdates);
      }

      void doSliderChanges() {
        auto sliders = test_section_->getAllSliders();

        if (sliders.size()) {
          int num_changes = std::ceil(kSliderRatioChangesPerUpdate * sliders.size());
          for (int i = 0; i < num_changes; ++i) {
            int index = rand() % sliders.size();
            auto iter = sliders.begin();
            std::advance(iter, index);
            SynthSlider* slider = iter->second;
            if (!slider->isShowing())
              continue;

            float min = slider->getMinimum();
            float max = slider->getMaximum();

            int decision = rand() % 6;
            if (decision == 0)
              slider->setValue(min, NotificationType::sendNotification);
            else if (decision == 1)
              slider->setValue(max, NotificationType::sendNotification);
            else
              slider->setValue((rand() * (max - min)) / RAND_MAX + min, NotificationType::sendNotification);
          }
        }
      }

      void doButtonChanges() {
        auto buttons = getAllComponentsOfType<ToggleButton>(test_section_);

        int num_changes = static_cast<int>(buttons.size());
        for (int i = 0; i < num_changes; ++i) {
          Button* button = buttons[i];
          if (rand() % kButtonTries == 0 && button->isShowing())
            button->setToggleState(!button->getToggleState(), NotificationType::sendNotification);
        }
      }

      void doPresetChanges() {
        auto preset_selectors = getAllComponentsOfType<PresetSelector>(test_section_);
        int num_changes = static_cast<int>(preset_selectors.size());
        for (int i = 0; i < num_changes; ++i) {
          PresetSelector* preset_selector = preset_selectors[i];
          if (rand() % kButtonTries == 0 && preset_selector->isShowing())
            preset_selector->clickNext();
        }
      }

      void timerCallback() override {
        if (test_section_ == nullptr)
          return;

        doSliderChanges();
        doButtonChanges();
        doPresetChanges();

        PopupMenu::dismissAllActiveMenus();
      }

    private:
      std::unique_ptr<FullInterface> full_interface_;
      SynthSection* test_section_;
  };

  class TestAudioComponentBase : public AudioAppComponent {
    public:
      TestAudioComponentBase(TestSynthBase* synth_base, FullInterface* full_interface = nullptr) :
        synth_base_(synth_base), top_component_(full_interface) {
        addAndMakeVisible(&top_component_);

        setAudioChannels(0, vital::kNumChannels);

        AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup(setup);
        setup.sampleRate = vital::kDefaultSampleRate;
        deviceManager.initialise(0, vital::kNumChannels, nullptr, true, "", &setup);

        if (deviceManager.getCurrentAudioDevice() == nullptr) {
          const OwnedArray<AudioIODeviceType>& device_types = deviceManager.getAvailableDeviceTypes();

          for (AudioIODeviceType* device_type : device_types) {
            deviceManager.setCurrentAudioDeviceType(device_type->getTypeName(), true);
            if (deviceManager.getCurrentAudioDevice())
              break;
          }
        }
      }

      ~TestAudioComponentBase() {
        shutdownAudio();
      }

      void setSizes() {
        top_component_.setSize(vital::kDefaultWindowWidth, vital::kDefaultWindowHeight);
      }

      TestTopComponent* top_component() { return &top_component_; }

      void prepareToPlay(int buffer_size, double sample_rate) override {
        synth_base_->getEngine()->setSampleRate(sample_rate);
        synth_base_->getEngine()->updateAllModulationSwitches();
      }

      void getNextAudioBlock(const AudioSourceChannelInfo& buffer) override {
        int num_samples = buffer.buffer->getNumSamples();
        int synth_samples = std::min(num_samples, vital::kMaxBufferSize);

        MidiBuffer midi_messages;

        for (int b = 0; b < num_samples; b += synth_samples) {
          int current_samples = std::min<int>(synth_samples, num_samples - b);

          synth_base_->process(buffer.buffer, vital::kNumChannels, current_samples, b);
        }
      }

      void releaseResources() override {
      }

    private:
      TestSynthBase* synth_base_;
      TestTopComponent top_component_;
  };

  class TestWindow : public DocumentWindow, public SynthGuiInterface, public Timer {
    public:
      static constexpr int kTestMs = 8000;

      TestWindow(TestSynthBase* synth_base, FullInterface* full_interface = nullptr) :
          DocumentWindow("Interface Test", Colours::lightgrey, DocumentWindow::allButtons, true),
          SynthGuiInterface(synth_base, false) {
        synth_base->setGuiInterface(this);
        top_audio_component_ = std::make_unique<TestAudioComponentBase>(synth_base, full_interface);
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        top_audio_component_->setSize(vital::kDefaultWindowWidth, vital::kDefaultWindowHeight);
        setContentOwned(top_audio_component_.get(), true);
        top_audio_component_->setSizes();
        setLookAndFeel(DefaultLookAndFeel::instance());
        startTimer(kTestMs);
      }

      TestTopComponent* top_component() { return top_audio_component_->top_component(); }

      void closeButtonPressed() override {
        JUCEApplication::getInstance()->systemRequestedQuit();
      }

      void timerCallback() override {
        closeButtonPressed();
      }

    private:
      std::unique_ptr<TestAudioComponentBase> top_audio_component_;
  };

  class TestApp : public JUCEApplication {
    public:
      TestApp(TestSynthBase* synth_base, FullInterface* full_interface = nullptr) {
        main_window_ = std::make_unique<TestWindow>(synth_base, full_interface);
        main_window_->centreWithSize(vital::kDefaultWindowWidth, vital::kDefaultWindowHeight);
        main_window_->setVisible(true);
      }

      const String getApplicationName() override { return ProjectInfo::projectName; }
      const String getApplicationVersion() override { return ProjectInfo::versionString; }
      bool moreThanOneInstanceAllowed() override { return true; }

      void initialise(const String& command_line) override {
      }

      void systemRequestedQuit() override {
        MessageManager::getInstance()->stopDispatchLoop();
        main_window_ = nullptr;
      }

      void shutdown() override {
        main_window_ = nullptr;
      }

      TestWindow* window() { return main_window_.get(); }

    protected:
      std::unique_ptr<TestWindow> main_window_;
  };
}

namespace {
  JUCEApplicationBase* createNullApplication() { return nullptr; }
}

void InterfaceTest::runStressRandomTest(SynthSection* component, FullInterface* full_interface) {
  beginTest("Stress Random Controls");
  MessageManager::getInstance();

  ScopedJuceInitialiser_GUI library_initializer;
  juce::JUCEApplicationBase::createInstance = &createNullApplication;

  if (synth_base_ == nullptr)
    createSynthEngine();

  vital::SoundEngine* engine = getSynthEngine();
  engine->noteOn(30, 0, 0, 0);
  engine->noteOn(37, 0, 0, 0);
  engine->noteOn(42, 0, 0, 0);

  TestApp test_app(synth_base_.get(), full_interface);
  test_app.window()->top_component()->addTestSection(component);
  component->setSize(vital::kDefaultWindowWidth, vital::kDefaultWindowHeight);
  test_app.window()->resized();
  vital::control_map controls = engine->getControls();
  test_app.window()->top_component()->full_interface()->setAllValues(controls);
  test_app.window()->top_component()->full_interface()->reset();

  JUCE_TRY {
    MessageManager::getInstance()->runDispatchLoop();
  }
  JUCE_CATCH_EXCEPTION

  engine->noteOff(30, 0, 0, 0);
  engine->noteOff(37, 0, 0, 0);
  engine->noteOff(42, 0, 0, 0);
}
