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
#include "interface/full_interface_test.h"

class SynthTestRunner : public UnitTestRunner {
  protected:
    void logMessage(const String& message) override {
      std::cout << message.toStdString() << std::endl;
    }
};

force_inline int chunkNameToData(const char* chunk_name) {
  return static_cast<int>(ByteOrder::littleEndianInt(chunk_name));
}

String getWavetableDataString(InputStream* input_stream) {
  static constexpr int kDataLength = 27;
  int first_chunk = input_stream->readInt();
  if (first_chunk != chunkNameToData("RIFF"))
    return "";

  int length = input_stream->readInt();
  int data_end = static_cast<int>(input_stream->getPosition()) + length;

  if (input_stream->readInt() != chunkNameToData("WAVE"))
    return "";

  while (!input_stream->isExhausted() && input_stream->getPosition() < data_end) {
    int chunk_label = input_stream->readInt();
    int chunk_length = input_stream->readInt();

    if (chunk_label == chunkNameToData("clm ")) {
      MemoryBlock memory_block;
      input_stream->readIntoMemoryBlock(memory_block, chunk_length);
      return memory_block.toString().substring(0, kDataLength);
    }

    input_stream->setPosition(input_stream->getPosition() + chunk_length);
  }

  return "";
}

void rebrandAllWavs() {
  static constexpr int kWavetableSampleRate = 88200;

  File directory("D:\\dev\\PurchasedWavetables");
  if (!directory.exists())
    return;

  Array<File> wavs;
  directory.findChildFiles(wavs, File::findFiles, true, "*.wav");
  AudioFormatManager format_manager;
  format_manager.registerBasicFormats();

  File converted_directory = directory.getChildFile("Converted");
  converted_directory.createDirectory();
  for (File& file : wavs) {
    FileInputStream* input_stream = new FileInputStream(file);
    String clm_data = getWavetableDataString(input_stream) + "[Matt Tytel]";
    input_stream->setPosition(0);
    std::unique_ptr<AudioFormatReader> format_reader(
        format_manager.createReaderFor(std::unique_ptr<InputStream>(input_stream)));
    if (format_reader == nullptr)
      continue;

    AudioSampleBuffer sample_buffer;
    int num_samples = static_cast<int>(format_reader->lengthInSamples);
    sample_buffer.setSize(1, num_samples);
    format_reader->read(&sample_buffer, 0, num_samples, 0, true, false);

    File output_file = converted_directory.getChildFile(file.getFileName());
    std::unique_ptr<FileOutputStream> file_stream = output_file.createOutputStream();
    WavAudioFormat wav_format;
    StringPairArray meta_data;
    meta_data.set("clm ", clm_data);
    std::unique_ptr<AudioFormatWriter> writer(wav_format.createWriterFor(file_stream.get(), kWavetableSampleRate,
                                              1, 16, meta_data, 0));

    const float* channel = sample_buffer.getReadPointer(0);
    writer->writeFromFloatArrays(&channel, 1, num_samples);
    writer->flush();
    file_stream->flush();
  }
}

int getResult(UnitTestRunner& test_runner) {
  for (int i = 0; i < test_runner.getNumResults(); ++i) {
    if (test_runner.getResult(i)->failures)
      return -1;
  }

  return 0;
}

int runSingleTest(UnitTest* test) {
  SynthTestRunner test_runner;
  test_runner.setAssertOnFailure(true);

  Array<UnitTest*> tests;
  tests.add(test);
  test_runner.runTests(tests);
  return getResult(test_runner);
}

int runSingleTest(String category, String name) {
  StringArray categories = UnitTest::getAllCategories();
  for (const String& category_name : categories) {
    if (category_name == category) {
      Array<UnitTest*> tests = UnitTest::getTestsInCategory(category);

      for (UnitTest* test : tests) {
        if (test->getName() == name)
          return runSingleTest(test);
      }
    }
  }
  return -1;
}

int runNonGraphicalTests() {
  SynthTestRunner test_runner;
  test_runner.setAssertOnFailure(true);

  StringArray categories = UnitTest::getAllCategories();
  for (const String& category : categories) {
    if (category != "Interface")
      test_runner.runTestsInCategory(category);

    for (int i = 0; i < test_runner.getNumResults(); ++i) {
      if (test_runner.getResult(i)->failures)
        return -1;
    }
  }

  return getResult(test_runner);
}

int runAllTests() {
  SynthTestRunner test_runner;
  test_runner.setAssertOnFailure(true);
  test_runner.runAllTests();

  return getResult(test_runner);
}

int runTests(int argc) {
  if (argc > 1)
    return runNonGraphicalTests();

  return runAllTests();
}

int main(int argc, char* argv[]) {
  int result = runTests(argc);

  DeletedAtShutdown::deleteAll();
  MessageManager::deleteInstance();
  return result;
}
