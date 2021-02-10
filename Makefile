
ifndef CONFIG
	CONFIG=Release
endif

ifndef LIBDIR
	LIBDIR=/usr/lib/
endif

BUILD_DATE="$(shell date +'%Y %m %d %H %M')"

PAID := 1


VERSION := $(shell sh -c 'grep -oh -m 1 "VERSION=[0-9\.]*" standalone/builds/linux/Makefile | cut -d "=" -f 2')

MACHINE := $(shell sh -c 'uname -m 2> /dev/null || echo not')
ifneq (,$(findstring aarch,$(MACHINE)))
	SIMDFLAGS := -march=armv8-a -mtune=cortex-a53
  GLFLAGS := -DOPENGL_ES=1
else
ifneq (,$(findstring arm,$(MACHINE)))
	SIMDFLAGS := -march=armv8-a -mtune=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard
  GLFLAGS := -DOPENGL_ES=1
else
	SIMDFLAGS := -msse2
endif
endif

PROGRAM = vital
LIB_PROGRAM = Vital
LIB_PROGRAM_FX = VitalFX
BIN = $(DESTDIR)/usr/bin
BINFILE = $(BIN)/$(PROGRAM)
LV2 = $(DESTDIR)/$(LIBDIR)/lv2/$(LIB_PROGRAM).lv2
EFFECTS_LV2 = $(DESTDIR)/$(LIBDIR)/lv2/$(LIB_PROGRAM_FX).lv2
VSTDIR = $(DESTDIR)/$(LIBDIR)/vst
VST = $(VSTDIR)/$(LIB_PROGRAM).so
VST3DIR = $(DESTDIR)/$(LIBDIR)/vst3
VST3 = $(VST3DIR)/$(LIB_PROGRAM).vst3
VST3SUBDIR = Contents/x86_64-linux
EFFECTS_VST = $(VSTDIR)/$(LIB_PROGRAM_FX).so
EFFECTS_VST3 = $(VST3DIR)/$(LIB_PROGRAM_FX).vst3
SYSDATA = $(DESTDIR)/usr/share/$(PROGRAM)
MAN = $(DESTDIR)/usr/share/man/man1/
CHANGES = $(DESTDIR)/usr/share/doc/$(PROGRAM)/
DESKTOP = $(DESTDIR)/usr/share/applications/
ZIP_FOLDER = $(LIB_PROGRAM)Binaries

ICONS      = $(DESTDIR)/usr/share/icons/hicolor/
ICON16     = images/vital_icon_16.png
ICON22     = images/vital_icon_22.png
ICON24     = images/vital_icon_24.png
ICON32     = images/vital_icon_32.png
ICON48     = images/vital_icon_48.png
ICON64     = images/vital_icon_64.png
ICON128    = images/vital_icon_128.png
ICON256    = images/vital_icon_256.png
XPMDEST    = $(DESTDIR)/usr/share/pixmaps
ICONXPM    = images/vital.xpm

ICONDEST16 = $(ICONS)/16x16/apps
ICONDEST22 = $(ICONS)/22x22/apps
ICONDEST24 = $(ICONS)/24x24/apps
ICONDEST32 = $(ICONS)/32x32/apps
ICONDEST48 = $(ICONS)/48x48/apps
ICONDEST64 = $(ICONS)/64x64/apps
ICONDEST128 = $(ICONS)/128x128/apps
ICONDEST256 = $(ICONS)/256x256/apps

all: standalone vst vst3 lv2

install_icons:
	install -d $(ICONDEST16) $(ICONDEST22) $(ICONDEST24) $(ICONDEST32)
	install -d $(ICONDEST48) $(ICONDEST64) $(ICONDEST128) $(ICONDEST256)
	install -d $(XPMDEST)
	cp $(ICONXPM) $(XPMDEST)/$(PROGRAM).xpm
	cp $(ICON16) $(ICONDEST16)/$(PROGRAM).png
	cp $(ICON22) $(ICONDEST22)/$(PROGRAM).png
	cp $(ICON24) $(ICONDEST24)/$(PROGRAM).png
	cp $(ICON32) $(ICONDEST32)/$(PROGRAM).png
	cp $(ICON48) $(ICONDEST48)/$(PROGRAM).png
	cp $(ICON64) $(ICONDEST64)/$(PROGRAM).png
	cp $(ICON128) $(ICONDEST128)/$(PROGRAM).png
	cp $(ICON256) $(ICONDEST256)/$(PROGRAM).png

standalone:
	$(MAKE) -C standalone/builds/linux CONFIG=$(CONFIG) SIMDFLAGS="$(SIMDFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE)

lv2:
	$(MAKE) -C plugin/builds/linux_lv2 CONFIG=$(CONFIG) AR=gcc-ar SIMDFLAGS="$(SIMDFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE)

effects_lv2:
	$(MAKE) -C effects/builds/linux_lv2 CONFIG=$(CONFIG) AR=gcc-ar SIMDFLAGS="$(SIMDFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE)

vst:
	$(MAKE) -C plugin/builds/linux_vst VST CONFIG=$(CONFIG) AR=gcc-ar SIMDFLAGS="$(SIMDFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE)

vst3:
	$(MAKE) -C plugin/builds/linux_vst VST3 CONFIG=$(CONFIG) AR=gcc-ar SIMDFLAGS="$(SIMDFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE)

effects_vst:
	$(MAKE) -C effects/builds/linux_vst VST CONFIG=$(CONFIG) AR=gcc-ar SIMDFLAGS="$(SIMDFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE)

effects_vst3:
	$(MAKE) -C effects/builds/linux_vst VST3 CONFIG=$(CONFIG) AR=gcc-ar SIMDFLAGS="$(SIMDFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE)

headless_server:
	$(MAKE) -C headless/builds/linux CONFIG=$(CONFIG) SIMDFLAGS="$(SIMDFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE)

test:
	$(MAKE) -C tests/builds/linux CONFIG=$(CONFIG) SIMDFLAGS="$(SIMDFLAGS)" GLFLAGS="$(GLFLAGS)" BUILD_DATE=$(BUILD_DATE)

clean:
	$(MAKE) clean -C standalone/builds/linux CONFIG=$(CONFIG)
	$(MAKE) clean -C plugin/builds/linux_vst CONFIG=$(CONFIG)
	$(MAKE) clean -C plugin/builds/linux_lv2 CONFIG=$(CONFIG)
	$(MAKE) clean -C effects/builds/linux_vst CONFIG=$(CONFIG)
	$(MAKE) clean -C effects/builds/linux_lv2 CONFIG=$(CONFIG)
	$(MAKE) clean -C headless/builds/linux CONFIG=$(CONFIG)
	$(MAKE) clean -C tests/builds/linux CONFIG=$(CONFIG)

install_standalone: standalone install_icons
	install -d $(BIN) $(MAN) $(CHANGES) $(DESKTOP)
	install standalone/builds/linux/build/$(PROGRAM) $(BIN)
	install -m644 standalone/vital.desktop $(DESKTOP)/vital.desktop

install_lv2: lv2
	install -d $(LV2)
	install -m644 plugin/builds/linux_lv2/Vital.lv2/* $(LV2)

install_effects_lv2: effects_lv2
	install -d $(EFFECTS_LV2)
	install -m644 effects/builds/linux_lv2/VitalFX.lv2/* $(EFFECTS_LV2)

install_vst: vst
	install -d $(VSTDIR)
	install plugin/builds/linux_vst/build/Vital.so $(VST)

install_effects_vst: effects_vst
	install -d $(VSTDIR)
	install effects/builds/linux_vst/build/VitalFX.so $(EFFECTS_VST)

install_vst3: vst3
	install -d $(VST3)/$(VST3SUBDIR)
	install -m644 plugin/builds/linux_vst/build/Vital.vst3/$(VST3SUBDIR)/* $(VST3)/$(VST3SUBDIR)

install_effects_vst3: effects_vst3
	install -d $(EFFECTS_VST3)/$(VST3SUBDIR)
	install -m644 plugin/builds/linux_vst/build/VitalFX.vst3/$(VST3SUBDIR)/* $(EFFECTS_VST3)/$(VST3SUBDIR)

install: install_standalone install_lv2 install_vst install_vst3
install_effects: install_effects_lv2 install_effects_vst install_effects_vst3

dist:
	rm -rf $(PROGRAM)
	mkdir $(PROGRAM)
	-cp -rf * $(PROGRAM)/
	$(MAKE) clean -C $(PROGRAM) CONFIG=Release
	$(MAKE) clean -C $(PROGRAM) CONFIG=Debug
	rm -rf $(PROGRAM)/.git
	rm -rf $(PROGRAM)/plugin/builds/CLion
	rm -rf $(PROGRAM)/plugin/builds/iOS
	rm -rf $(PROGRAM)/plugin/builds/osx
	rm -rf $(PROGRAM)/plugin/builds/vs17
	rm -rf $(PROGRAM)/plugin/builds/vs19
	rm -rf $(PROGRAM)/effects/builds/CLion
	rm -rf $(PROGRAM)/effects/builds/iOS
	rm -rf $(PROGRAM)/effects/builds/osx
	rm -rf $(PROGRAM)/effects/builds/vs17
	rm -rf $(PROGRAM)/effects/builds/vs19
	rm -rf $(PROGRAM)/standalone/builds/CLion
	rm -rf $(PROGRAM)/standalone/builds/iOS
	rm -rf $(PROGRAM)/standalone/builds/osx
	rm -rf $(PROGRAM)/standalone/builds/vs17
	rm -rf $(PROGRAM)/standalone/builds/vs19
	rm -rf $(PROGRAM)/tests/builds/CLion
	rm -rf $(PROGRAM)/tests/builds/iOS
	rm -rf $(PROGRAM)/tests/builds/osx
	rm -rf $(PROGRAM)/tests/builds/vs17
	rm -rf $(PROGRAM)/tests/builds/vs19
	mv $(PROGRAM) $(PROGRAM)-$(VERSION)
	tar -cvzf $(PROGRAM)_$(VERSION).orig.tar.gz $(PROGRAM)-$(VERSION)
	rm -rf $(PROGRAM)-$(VERSION)

zip_binaries:
	mkdir $(ZIP_FOLDER)
	cp -r plugin/builds/linux_lv2/Vital.lv2 $(ZIP_FOLDER)
	cp -r plugin/builds/linux_vst/build/Vital.so $(ZIP_FOLDER)
	cp -r plugin/builds/linux_vst/build/Vital.vst3 $(ZIP_FOLDER)
	cp -r standalone/builds/linux/build/$(PROGRAM) $(ZIP_FOLDER)
	zip -r $(ZIP_FOLDER) $(ZIP_FOLDER)

uninstall:
	rm -rf $(LV2)
	rm -rf $(VST)
	rm -rf $(SYSDATA)
	rm -rf $(BINFILE)
	rm $(ICONDEST16)/$(PROGRAM).png
	rm $(ICONDEST22)/$(PROGRAM).png
	rm $(ICONDEST24)/$(PROGRAM).png
	rm $(ICONDEST32)/$(PROGRAM).png
	rm $(ICONDEST48)/$(PROGRAM).png
	rm $(ICONDEST64)/$(PROGRAM).png
	rm $(ICONDEST128)/$(PROGRAM).png
	rm $(ICONDEST256)/$(PROGRAM).png
	rm $(XPMDEST)/$(PROGRAM).xpm

.PHONY: standalone
