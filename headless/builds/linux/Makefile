# Automatically generated makefile, created by the Projucer
# Don't edit this file! Your changes will be overwritten when you re-save the Projucer project!

# build with "V=1" for verbose builds
ifeq ($(V), 1)
V_AT =
else
V_AT = @
endif

# (this disables dependency generation if multiple architectures are set)
DEPFLAGS := $(if $(word 2, $(TARGET_ARCH)), , -MMD)

ifndef STRIP
  STRIP=strip
endif

ifndef AR
  AR=ar
endif

ifndef CONFIG
  CONFIG=Debug
endif

JUCE_ARCH_LABEL := $(shell uname -m)

ifeq ($(CONFIG),Debug)
  JUCE_BINDIR := build
  JUCE_LIBDIR := build
  JUCE_OBJDIR := build/intermediate/Debug
  JUCE_OUTDIR := build

  ifeq ($(TARGET_ARCH),)
    TARGET_ARCH := 
  endif

  JUCE_CPPFLAGS := $(DEPFLAGS) "-DLINUX=1" "-DDEBUG=1" "-D_DEBUG=1" "-DBUILD_DATE=$(BUILD_DATE)" "-DJUCE_JACK_CLIENT_NAME=\"Vital\"" "-DJUCE_ALSA_MIDI_INPUT_NAME=\"Vital\"" "-DJUCE_ALSA_MIDI_OUTPUT_NAME=\"Vital\"" "-DJUCE_USE_XRANDR=0" "-DJUCE_DSP_USE_SHARED_FFTW=1" "-DHEADLESS=1" "-DNO_AUTH=1" "-DJUCER_LINUX_MAKE_6B3E762A=1" "-DJUCE_APP_VERSION=99999.9.9" "-DJUCE_APP_VERSION_HEX=0x869f0909" $(shell pkg-config --cflags libcurl) -pthread -I../../JuceLibraryCode -I../../../third_party/JUCE/modules -I../../../src/common -I../../../src/common/wavetable -I../../../src/interface/editor_components -I../../../src/interface/editor_sections -I../../../src/interface/look_and_feel -I../../../src/interface/wavetable -I../../../src/interface/wavetable/editors -I../../../src/interface/wavetable/overlays -I../../../src/standalone -I../../../src/synthesis/synth_engine -I../../../src/synthesis/effects -I../../../src/synthesis/filters -I../../../src/synthesis/framework -I../../../src/synthesis/lookups -I../../../src/synthesis/modulators -I../../../src/synthesis/modules -I../../../src/synthesis/producers -I../../../src/synthesis/utilities -I../../../third_party $(CPPFLAGS)
  JUCE_CPPFLAGS_APP :=  "-DJucePlugin_Build_VST=0" "-DJucePlugin_Build_VST3=0" "-DJucePlugin_Build_AU=0" "-DJucePlugin_Build_AUv3=0" "-DJucePlugin_Build_RTAS=0" "-DJucePlugin_Build_AAX=0" "-DJucePlugin_Build_Standalone=0" "-DJucePlugin_Build_Unity=0"
  JUCE_TARGET_APP := vital

  JUCE_CFLAGS += $(JUCE_CPPFLAGS) $(TARGET_ARCH) -g -ggdb -O0 -ffast-math ${SIMDFLAGS} ${GLFLAGS} -ftree-vectorize -ftree-slp-vectorize -funroll-loops $(CFLAGS)
  JUCE_CXXFLAGS += $(JUCE_CFLAGS) -std=c++14 $(CXXFLAGS)
  JUCE_LDFLAGS += $(TARGET_ARCH) -L$(JUCE_BINDIR) -L$(JUCE_LIBDIR) -L/usr/X11R6/lib/ $(shell pkg-config --libs libcurl) -fvisibility=hidden -ffast-math ${SIMDFLAGS} ${GLFLAGS} -ftree-vectorize -ftree-slp-vectorize -lcurl -lrt -ldl -lpthread $(LDFLAGS)

  CLEANCMD = rm -rf $(JUCE_OUTDIR)/$(TARGET) $(JUCE_OBJDIR)
endif

ifeq ($(CONFIG),Release)
  JUCE_BINDIR := build
  JUCE_LIBDIR := build
  JUCE_OBJDIR := build/intermediate/Release
  JUCE_OUTDIR := build

  ifeq ($(TARGET_ARCH),)
    TARGET_ARCH := 
  endif

  JUCE_CPPFLAGS := $(DEPFLAGS) "-DLINUX=1" "-DNDEBUG=1" "-DBUILD_DATE=$(BUILD_DATE)" "-DJUCE_JACK_CLIENT_NAME=\"Vital\"" "-DJUCE_ALSA_MIDI_INPUT_NAME=\"Vital\"" "-DJUCE_ALSA_MIDI_OUTPUT_NAME=\"Vital\"" "-DJUCE_USE_XRANDR=0" "-DJUCE_DSP_USE_SHARED_FFTW=1" "-DHEADLESS=1" "-DNO_AUTH=1" "-DJUCER_LINUX_MAKE_6B3E762A=1" "-DJUCE_APP_VERSION=99999.9.9" "-DJUCE_APP_VERSION_HEX=0x869f0909" $(shell pkg-config --cflags libcurl) -pthread -I../../JuceLibraryCode -I../../../third_party/JUCE/modules -I../../../src/common -I../../../src/common/wavetable -I../../../src/interface/editor_components -I../../../src/interface/editor_sections -I../../../src/interface/look_and_feel -I../../../src/interface/wavetable -I../../../src/interface/wavetable/editors -I../../../src/interface/wavetable/overlays -I../../../src/standalone -I../../../src/synthesis/synth_engine -I../../../src/synthesis/effects -I../../../src/synthesis/filters -I../../../src/synthesis/framework -I../../../src/synthesis/lookups -I../../../src/synthesis/modulators -I../../../src/synthesis/modules -I../../../src/synthesis/producers -I../../../src/synthesis/utilities -I../../../third_party $(CPPFLAGS)
  JUCE_CPPFLAGS_APP :=  "-DJucePlugin_Build_VST=0" "-DJucePlugin_Build_VST3=0" "-DJucePlugin_Build_AU=0" "-DJucePlugin_Build_AUv3=0" "-DJucePlugin_Build_RTAS=0" "-DJucePlugin_Build_AAX=0" "-DJucePlugin_Build_Standalone=0" "-DJucePlugin_Build_Unity=0"
  JUCE_TARGET_APP := vital

  JUCE_CFLAGS += $(JUCE_CPPFLAGS) $(TARGET_ARCH) -Ofast -flto -ffast-math ${SIMDFLAGS} ${GLFLAGS} -ftree-vectorize -ftree-slp-vectorize -funroll-loops $(CFLAGS)
  JUCE_CXXFLAGS += $(JUCE_CFLAGS) -std=c++14 $(CXXFLAGS)
  JUCE_LDFLAGS += $(TARGET_ARCH) -L$(JUCE_BINDIR) -L$(JUCE_LIBDIR) -L/usr/X11R6/lib/ $(shell pkg-config --libs libcurl) -fvisibility=hidden -flto -ffast-math ${SIMDFLAGS} ${GLFLAGS} -ftree-vectorize -ftree-slp-vectorize -lcurl -lrt -ldl -lpthread $(LDFLAGS)

  CLEANCMD = rm -rf $(JUCE_OUTDIR)/$(TARGET) $(JUCE_OBJDIR)
endif

OBJECTS_APP := \
  $(JUCE_OBJDIR)/main_f0db04ea.o \
  $(JUCE_OBJDIR)/common_24cbed85.o \
  $(JUCE_OBJDIR)/synthesis_1ee447c4.o \
  $(JUCE_OBJDIR)/include_juce_audio_basics_8a4e984a.o \
  $(JUCE_OBJDIR)/include_juce_audio_formats_15f82001.o \
  $(JUCE_OBJDIR)/include_juce_core_f26d17db.o \
  $(JUCE_OBJDIR)/include_juce_data_structures_7471b1e3.o \
  $(JUCE_OBJDIR)/include_juce_dsp_aeb2060f.o \
  $(JUCE_OBJDIR)/include_juce_events_fd7d695.o \

.PHONY: clean all strip

all : $(JUCE_OUTDIR)/$(JUCE_TARGET_APP)

$(JUCE_OUTDIR)/$(JUCE_TARGET_APP) : $(OBJECTS_APP) $(RESOURCES)
	@command -v pkg-config >/dev/null 2>&1 || { echo >&2 "pkg-config not installed. Please, install it."; exit 1; }
	@pkg-config --print-errors libcurl
	@echo Linking "Vital - App"
	-$(V_AT)mkdir -p $(JUCE_BINDIR)
	-$(V_AT)mkdir -p $(JUCE_LIBDIR)
	-$(V_AT)mkdir -p $(JUCE_OUTDIR)
	$(V_AT)$(CXX) -o $(JUCE_OUTDIR)/$(JUCE_TARGET_APP) $(OBJECTS_APP) $(JUCE_LDFLAGS) $(JUCE_LDFLAGS_APP) $(RESOURCES) $(TARGET_ARCH)

$(JUCE_OBJDIR)/main_f0db04ea.o: ../../../src/headless/main.cpp
	-$(V_AT)mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling main.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/common_24cbed85.o: ../../../src/unity_build/common.cpp
	-$(V_AT)mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling common.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/synthesis_1ee447c4.o: ../../../src/unity_build/synthesis.cpp
	-$(V_AT)mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling synthesis.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_audio_basics_8a4e984a.o: ../../JuceLibraryCode/include_juce_audio_basics.cpp
	-$(V_AT)mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling include_juce_audio_basics.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_audio_formats_15f82001.o: ../../JuceLibraryCode/include_juce_audio_formats.cpp
	-$(V_AT)mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling include_juce_audio_formats.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_core_f26d17db.o: ../../JuceLibraryCode/include_juce_core.cpp
	-$(V_AT)mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling include_juce_core.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_data_structures_7471b1e3.o: ../../JuceLibraryCode/include_juce_data_structures.cpp
	-$(V_AT)mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling include_juce_data_structures.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_dsp_aeb2060f.o: ../../JuceLibraryCode/include_juce_dsp.cpp
	-$(V_AT)mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling include_juce_dsp.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_events_fd7d695.o: ../../JuceLibraryCode/include_juce_events.cpp
	-$(V_AT)mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling include_juce_events.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

clean:
	@echo Cleaning Vital
	$(V_AT)$(CLEANCMD)

strip:
	@echo Stripping Vital
	-$(V_AT)$(STRIP) --strip-unneeded $(JUCE_OUTDIR)/$(TARGET)

-include $(OBJECTS_APP:%.o=%.d)
