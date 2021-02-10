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

#include "JuceHeader.h"
#include "border_bounds_constrainer.h"
#include "full_interface.h"
#include "load_save.h"
#include "startup.h"
#include "synth_editor.h"
#include "tuning.h"

#if JUCE_MODULE_AVAILABLE_juce_graphics
#include "wavetable_edit_section.h"
#include "wavetable_3d.h"
#endif

void handleVitalCrash(void* data) {
  LoadSave::writeCrashLog(SystemStats::getStackBacktrace());
}

String getArgumentValue(const StringArray& args, const String& flag, const String& full_flag) {
  int index = 0;
  for (String arg : args) {
    index++;
    if (arg == flag || arg == full_flag)
      return args[index];
  }

  return "";
}

int loadAudioFile(AudioSampleBuffer& destination, InputStream* audio_stream) {
  AudioFormatManager format_manager;
  format_manager.registerBasicFormats();
  
  audio_stream->setPosition(0);
  std::unique_ptr<AudioFormatReader> format_reader(
      format_manager.createReaderFor(std::unique_ptr<InputStream>(audio_stream)));
  if (format_reader == nullptr)
    return 0;

  int num_samples = static_cast<int>(format_reader->lengthInSamples);
  destination.setSize(format_reader->numChannels, num_samples);
  format_reader->read(&destination, 0, num_samples, 0, true, true);
  return format_reader->sampleRate;
}

FileSource::FadeStyle getFadeStyleFromWavetableString(String data) {
  if (data.isEmpty())
    return FileSource::kFreqInterpolate;

  if (data.substring(0, 3) == "<!>") {
    StringArray tokens;
    tokens.addTokens(data.substring(3), " ", "");
    if (tokens.size() < 2 || tokens[1].isEmpty())
      return FileSource::kFreqInterpolate;

    char fade_character = tokens[1][0];
    if (fade_character == '0')
      return FileSource::kNoInterpolate;
    if (fade_character == '1')
      return FileSource::kTimeInterpolate;

    return FileSource::kFreqInterpolate;
  }

  return FileSource::kFreqInterpolate;
}

class SynthApplication : public JUCEApplication {
  public:
    class MainWindow : public DocumentWindow, public ApplicationCommandTarget, private AsyncUpdater {
      public:
        enum PresetCommand {
          kSave = 0x5001,
          kSaveAs,
          kOpen,
          kToggleVideo,
        };

        MainWindow(const String& name, bool visible) :
            DocumentWindow(name, Colours::lightgrey, DocumentWindow::allButtons, visible), editor_(nullptr) {
          if (!Startup::isComputerCompatible()) {
            String error = String(ProjectInfo::projectName) +
                           " requires SSE2, NEON or AVX2 compatible processor. Exiting.";
            AlertWindow::showNativeDialogBox("Computer not supported", error, false);
            quit();
          }

          SystemStats::setApplicationCrashHandler(handleVitalCrash);
              
          if (visible) {
            setUsingNativeTitleBar(true);
            setResizable(true, true);
          }

          editor_ = new SynthEditor(visible);
          constrainer_.setGui(editor_->getGui());
          if (visible) {
            editor_->animate(true);
            setContentOwned(editor_, true);

            constrainer_.setMinimumSize(vital::kMinWindowWidth, vital::kMinWindowHeight);
            constrainer_.setBorder(getPeer()->getFrameSize());
            float ratio = (1.0f * vital::kDefaultWindowWidth) / vital::kDefaultWindowHeight;

            constrainer_.setFixedAspectRatio(ratio);
            setConstrainer(&constrainer_);

            centreWithSize(getWidth(), getHeight());
            setVisible(visible);
            triggerAsyncUpdate();
          }
          else
            editor_->animate(false);
        }

        void closeButtonPressed() override {
          JUCEApplication::getInstance()->systemRequestedQuit();
        }

        void loadFile(const File& file) {
          file_to_load_ = file;
          triggerAsyncUpdate();
        }

        void shutdownAudio() {
          editor_->shutdownAudio();
        }

        ApplicationCommandTarget* getNextCommandTarget() override {
          return findFirstTargetParentComponent();
        }

        void getAllCommands(Array<CommandID>& commands) override {
          commands.add(kSave);
          commands.add(kSaveAs);
          commands.add(kOpen);
        }

        void getCommandInfo(const CommandID commandID, ApplicationCommandInfo& result) override {
          if (commandID == kSave) {
            result.setInfo(TRANS("Save"), TRANS("Save the current preset"), "Application", 0);
            result.defaultKeypresses.add(KeyPress('s', ModifierKeys::commandModifier, 0));
          }
          else if (commandID == kSaveAs) {
            result.setInfo(TRANS("Save As"), TRANS("Save preset to a new file"), "Application", 0);
            ModifierKeys modifier = ModifierKeys::commandModifier | ModifierKeys::shiftModifier;
            result.defaultKeypresses.add(KeyPress('s', modifier, 0));
          }
          else if (commandID == kOpen) {
            result.setInfo(TRANS("Open"), TRANS("Open a preset"), "Application", 0);
            result.defaultKeypresses.add(KeyPress('o', ModifierKeys::commandModifier, 0));
          }
          else if (commandID == kToggleVideo) {
            result.setInfo(TRANS("Toggle Zoom"), TRANS("Toggle zoom for recording"), "Application", 0);
            ModifierKeys modifier = ModifierKeys::commandModifier | ModifierKeys::shiftModifier;
            result.defaultKeypresses.add(KeyPress('t', modifier, 0));
          }
        }

        bool perform(const InvocationInfo& info) override {
          if (info.commandID == kSave) {
            if (!editor_->saveToActiveFile())
              editor_->openSaveDialog();
            else {
              grabKeyboardFocus();
              editor_->setFocus();
            }
            return true;
          }
          else if (info.commandID == kSaveAs) {
            File active_file = editor_->getActiveFile();
            FileChooser save_box("Export Preset", File(), String("*.") + vital::kPresetExtension);
            if (save_box.browseForFileToSave(true))
              editor_->saveToFile(save_box.getResult().withFileExtension(vital::kPresetExtension));
            grabKeyboardFocus();
            editor_->setFocus();
            return true;
          }
          else if (info.commandID == kOpen) {
            File active_file = editor_->getActiveFile();
            FileChooser open_box("Open Preset", active_file, String("*.") + vital::kPresetExtension);
            if (!open_box.browseForFileToOpen())
              return true;
            
            File choice = open_box.getResult();
            if (!choice.exists())
              return true;

            std::string error;
            if (!editor_->loadFromFile(choice, error)) {
              error = "There was an error open the preset. " + error;
              AlertWindow::showNativeDialogBox("Error opening preset", error, false);
            }
            else
              editor_->externalPresetLoaded(choice);
            grabKeyboardFocus();
            editor_->setFocus();
            return true;
          }
          else if (info.commandID == kToggleVideo)
            editor_->getGui()->toggleFilter1Zoom();

          return false;
        }

        void handleAsyncUpdate() override {
          if (command_manager_ == nullptr) {
            command_manager_ = std::make_unique<ApplicationCommandManager>();
            command_manager_->registerAllCommandsForTarget(JUCEApplication::getInstance());
            command_manager_->registerAllCommandsForTarget(this);
            addKeyListener(command_manager_->getKeyMappings());
          }
          
          if (file_to_load_.exists()) {
            loadFileAsyncUpdate();
            file_to_load_ = File();
          }
          
          editor_->setFocus();
        }

      private:
        void loadFileAsyncUpdate() {
          std::string error;
          bool success = editor_->loadFromFile(file_to_load_, error);
          if (success)
            editor_->externalPresetLoaded(file_to_load_);
        }
      
        File file_to_load_;
        SynthEditor* editor_;
        std::unique_ptr<ApplicationCommandManager> command_manager_;
        BorderBoundsConstrainer constrainer_;
      
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

    SynthApplication() = default;

    const String getApplicationName() override { return ProjectInfo::projectName; }
    const String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const String& command_line) override {
      String command = " " + command_line + " ";
      if (command.contains(" --version ") || command.contains(" -v ")) {
        std::cout << getApplicationName() << " " << getApplicationVersion() << newLine;
        quit();
      }
      else if (command.contains(" --help ") || command.contains(" -h ")) {
        std::cout << "Usage:" << newLine;
        std::cout << "  " << getApplicationName().toLowerCase() << " [OPTION...]" << newLine << newLine;
        std::cout << getApplicationName() << " polyphonic wavetable synthesizer." << newLine << newLine;
        std::cout << "Help Options:" << newLine;
        std::cout << "  -h, --help                          Show help options" << newLine << newLine;
        std::cout << "Application Options:" << newLine;
        std::cout << "  -v, --version                       Show version information and exit" << newLine;
        std::cout << "  --headless                          Run without graphical interface." << newLine;
        std::cout << "  --tabletowav                        Converts a vitaltable to wav file." << newLine;
        std::cout << "  --tableimages                       Renders an image for the table." << newLine;
        std::cout << "  --render                            Render to an audio file." << newLine;
        std::cout << "  -m, --midi                          Note to play (with --render)." << newLine;
        std::cout << "  -l, --length                        Not length to play (with --render)." << newLine;
        std::cout << "  -b, --bpm                           BPM to play (with --render)." << newLine;
        std::cout << "  --images                            Render oscilloscope images (with --render)." << newLine << newLine;
        quit();
      }
      else if (command.contains(" --tabletowav ")) {
        static constexpr int kSampleRate = 88200;
        
        vital::Wavetable wavetable(vital::kNumOscillatorWaveFrames);
        WavetableCreator wavetable_creator(&wavetable);
        File output_file;

        bool last_arg_was_option = false;
        StringArray args = getCommandLineParameterArray();
        for (const String& arg : args) {
          File file;
          if (arg[0] == '/')
            file = File(arg);
          else
            file = File("./" + arg);
          
          if (arg != "" && arg[0] != '-' && !last_arg_was_option && file.exists()) {
            if (file.getFileExtension() == String(".") + String(vital::kWavetableExtension)) {
              output_file = File::getCurrentWorkingDirectory().getChildFile(file.getFileNameWithoutExtension() + ".wav");

              try {
                json parsed_json_state = json::parse(file.loadFileAsString().toStdString(), nullptr, false);
                wavetable_creator.jsonToState(parsed_json_state);
              }
              catch (const json::exception& e) {
                std::cout << "Error loading wavetable" << std::endl;
                quit();
                return;
              }
            }
            else {
              String new_name = file.getFileNameWithoutExtension() + "_converted.wav";
              output_file = File::getCurrentWorkingDirectory().getChildFile(new_name);

              FileInputStream* audio_stream = new FileInputStream(file);
              AudioSampleBuffer sample_buffer;
              String wavetable_string = WavetableEditSection::getWavetableDataString(audio_stream);
              int sample_rate = loadAudioFile(sample_buffer, audio_stream);
              if (sample_rate == 0) {
                std::cout << "Error loading wav as wavetable" << std::endl;
                quit();
                return;
              }

              FileSource::FadeStyle fade_style = getFadeStyleFromWavetableString(wavetable_string);
              wavetable_creator.initFromAudioFile(sample_buffer.getReadPointer(0), sample_buffer.getNumSamples(),
                                                  sample_rate, WavetableCreator::kWavetableSplice, fade_style);
            }
          }

          last_arg_was_option = arg[0] == '-' && arg != "--headless" && arg != "--tabletowav";
        }
        
        std::unique_ptr<FileOutputStream> file_stream = output_file.createOutputStream();
        WavAudioFormat wav_format;
        StringPairArray meta_data;
        meta_data.set("clm ", "<!>2048 20000000 wavetable (vital.audio)");
        std::unique_ptr<AudioFormatWriter> writer(wav_format.createWriterFor(file_stream.get(), kSampleRate,
                                                                             1, 16, meta_data, 0));

        int total_samples = vital::WaveFrame::kWaveformSize * vital::kNumOscillatorWaveFrames;
        std::unique_ptr<float[]> buffer = std::make_unique<float[]>(total_samples);
        wavetable_creator.renderToBuffer(buffer.get(), vital::kNumOscillatorWaveFrames, vital::WaveFrame::kWaveformSize);

        float* channel = buffer.get();
        writer->writeFromFloatArrays(&channel, 1, total_samples);
        writer->flush();
        file_stream->flush();

        writer = nullptr;
        file_stream.release();
        quit();
      }
      else if (command.contains(" --tableimages ")) {
      #if JUCE_MODULE_AVAILABLE_juce_graphics
        static constexpr int kSampleRate = 88200;
        static constexpr int kFrameRate = 30;
        static constexpr int kSamplesPerFrame = kSampleRate / kFrameRate;

        vital::Wavetable wavetable(vital::kNumOscillatorWaveFrames);
        WavetableCreator wavetable_creator(&wavetable);

        bool last_arg_was_option = false;
        StringArray args = getCommandLineParameterArray();
        for (const String& arg : args) {
          File file;
          if (arg[0] == '/')
            file = File(arg);
          else
            file = File("./" + arg);
          if (arg != "" && arg[0] != '-' && !last_arg_was_option && file.exists()) {
            if (file.getFileExtension() == String(".") + String(vital::kWavetableExtension)) {
              try {
                json parsed_json_state = json::parse(file.loadFileAsString().toStdString(), nullptr, false);
                wavetable_creator.jsonToState(parsed_json_state);
              }
              catch (const json::exception& e) {
                std::cout << "Error loading wavetable" << std::endl;
                quit();
                return;
              }
            }
            else {
              FileInputStream* audio_stream = new FileInputStream(file);
              AudioSampleBuffer sample_buffer;
              String wavetable_string = WavetableEditSection::getWavetableDataString(audio_stream);
              int sample_rate = loadAudioFile(sample_buffer, audio_stream);
              if (sample_rate == 0) {
                quit();
                return;
              }

              FileSource::FadeStyle fade_style = getFadeStyleFromWavetableString(wavetable_string);
              wavetable_creator.initFromAudioFile(sample_buffer.getReadPointer(0), sample_buffer.getNumSamples(),
                                                  sample_rate, WavetableCreator::kWavetableSplice, fade_style);
            }
          }
          
          last_arg_was_option = arg[0] == '-' && arg != "--headless" && arg != "--tableimages";
        }
        
        static constexpr float kWaveHeightPercent = 0.1f;
        static constexpr float kWaveRangeX = 0.6993633f;
        static constexpr float kFrameRangeX = 0.1711459f;
        static constexpr float kWaveRangeY = 0.11762711f;
        static constexpr float kFrameRangeY = -0.480666f;
        static constexpr float kStartX = 0.064745374f;
        static constexpr float kStartY = 0.731519639f;
        static constexpr float kOffsetX = -0.248793f;
        static constexpr float kOffsetY = 0.147922352f;
        
        static constexpr int kImageWidth = 500;
        static constexpr int kImageHeight = 250;
        static constexpr int kImageNumberPlaces = 3;

        Colour background(0xff4c4f52);
        Colour selected_color(0xffaa88ff);
        Colour color(0x19aa88ff);
        
        File images_folder = File::getCurrentWorkingDirectory().getChildFile("images");
        if (!images_folder.exists())
          images_folder.createDirectory();
        
        Image base_image(Image::RGB, kImageWidth, kImageHeight, true);
        Graphics base_g(base_image);
        base_g.fillAll(Colour(0xff1d2125));
        Wavetable3d::paint3dBackground(base_g, &wavetable, true, background, color, color, kImageWidth, kImageHeight,
                                       kWaveHeightPercent, kWaveRangeX, kFrameRangeX, kWaveRangeY, kFrameRangeY,
                                       kStartX, kStartY, kOffsetX, kOffsetY);
        int total_samples = vital::WaveFrame::kWaveformSize * vital::kNumOscillatorWaveFrames;
        int frame = 0;
        for (int i = 0; i < total_samples; i += kSamplesPerFrame) {
          int index = std::min((i * vital::kNumOscillatorWaveFrames) / total_samples, vital::kNumOscillatorWaveFrames - 1);
          Image image = base_image.createCopy();
          Graphics g(image);
          Wavetable3d::paint3dLine(g, &wavetable, index, selected_color, kImageWidth, kImageHeight,
                                   kWaveHeightPercent, kWaveRangeX, kFrameRangeX, kWaveRangeY, kFrameRangeY,
                                   kStartX, kStartY, kOffsetX, kOffsetY);
          
          String number(frame);
          while (number.length() < kImageNumberPlaces)
            number = "0" + number;

          File image_file = images_folder.getChildFile("rendered_image" + number + ".png");
          FileOutputStream image_file_stream(image_file);
          PNGImageFormat png;
          png.writeImageToStream(image, image_file_stream);
          frame++;
        }
        
        quit();
      #endif
      }
      else if (command.contains(" --render ")) {
        bool images = command.contains(" --images ");
        HeadlessSynth synth;
        File output_file;
        StringArray args = getCommandLineParameterArray();
        bool last_arg_was_option = false;
        for (const String& arg : args) {
          File file;
          if (arg[0] == '/')
            file = File(arg);
          else
            file = File("./" + arg);
          if (arg != "" && arg[0] != '-' && !last_arg_was_option && file.exists()) {
            std::string error;
            synth.loadFromFile(file, error);
            output_file = File::getCurrentWorkingDirectory().getChildFile(file.getFileNameWithoutExtension() + ".wav");
            break;
          }

          last_arg_was_option = arg[0] == '-' && arg != "--headless" && arg != "--render" && arg != "--images";
        }

        int note = 48;
        int length = 6;
        float bpm = 120.0f;
        String note_setting = getArgumentValue(args, "-m", "--midi");
        if (!note_setting.isEmpty())
          note = note_setting.getIntValue();

        String length_setting = getArgumentValue(args, "-l", "--length");
        if (!length_setting.isEmpty())
          length = length_setting.getIntValue();

        String bpm_setting = getArgumentValue(args, "-b", "--bpm");
        if (!bpm_setting.isEmpty())
          bpm = bpm_setting.getIntValue();

        synth.renderAudioToFile(output_file, length, bpm, { note }, images);
        quit();
      }
      else {
        bool visible = !command.contains(" --headless ");
        main_window_ = std::make_unique<MainWindow>(getApplicationName(), visible);

        StringArray args = getCommandLineParameterArray();
        bool last_arg_was_option = false;
        for (const String& arg : args) {
          if (arg != "" && arg[0] != '-' && !last_arg_was_option && loadFromCommandLine(arg))
            break;

          last_arg_was_option = arg[0] == '-' && arg != "--headless";
        }
      }
    }

    bool loadFromCommandLine(const String& command_line) {
      String file_path = command_line;
      if (file_path[0] == '"' && file_path[file_path.length() - 1] == '"')
        file_path = command_line.substring(1, command_line.length() - 1);
      File file = File::getCurrentWorkingDirectory().getChildFile(file_path);
      if (!file.exists())
        return false;

      main_window_->loadFile(file);
      return true;
    }

    void shutdown() override {
      main_window_ = nullptr;
    }

    void systemRequestedQuit() override {
      quit();
    }

    void anotherInstanceStarted(const String& command_line) override {
      loadFromCommandLine(command_line);
    }

  private:
    std::unique_ptr<MainWindow> main_window_;
};

START_JUCE_APPLICATION(SynthApplication)
