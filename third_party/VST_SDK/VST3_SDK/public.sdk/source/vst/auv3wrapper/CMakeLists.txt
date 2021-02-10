
# iOS target
if(MAC AND SMTG_ADD_VSTGUI)
    if(XCODE AND SMTG_IOS_DEVELOPMENT_TEAM)
        function(add_vst_resources varName)
            smtg_add_vst3_resource(${varName} "${SDK_ROOT}/public.sdk/samples/vst/again/resource/again.rc")
            smtg_add_vst3_resource(${varName} "${SDK_ROOT}/public.sdk/samples/vst/again/resource/again.uidesc")
            smtg_add_vst3_resource(${varName} "${SDK_ROOT}/public.sdk/samples/vst/again/resource/background.png")
            smtg_add_vst3_resource(${varName} "${SDK_ROOT}/public.sdk/samples/vst/again/resource/slider_background.png")
            smtg_add_vst3_resource(${varName} "${SDK_ROOT}/public.sdk/samples/vst/again/resource/slider_handle.png")
            smtg_add_vst3_resource(${varName} "${SDK_ROOT}/public.sdk/samples/vst/again/resource/slider_handle_2.0x.png")
            smtg_add_vst3_resource(${varName} "${SDK_ROOT}/public.sdk/samples/vst/again/resource/vu_off.png")
            smtg_add_vst3_resource(${varName} "${SDK_ROOT}/public.sdk/samples/vst/again/resource/vu_on.png")
        endfunction()

        set(vst_sources
            ${SDK_ROOT}/public.sdk/samples/vst/again/source/again.cpp
            ${SDK_ROOT}/public.sdk/samples/vst/again/source/againcontroller.cpp
            ${SDK_ROOT}/public.sdk/samples/vst/again/source/againentry.cpp
            ${SDK_ROOT}/public.sdk/samples/vst/again/source/againsidechain.cpp
            ${SDK_ROOT}/public.sdk/samples/vst/again/source/againsimple.cpp
        )

        set(auv3wrappermacos_sources
            AUv3WrappermacOS/main.mm
            AUv3WrappermacOS/ViewController.m
            AUv3WrappermacOS/AppDelegate.m
            Shared/AUv3AudioEngine.mm
            Shared/AUv3AudioEngine.h
        )

        set(auv3wrappermacos_xib_resources
            AUv3WrappermacOS/Base.lproj/Main.storyboard
            AUv3WrappermacOS/Assets.xcassets
        )

        set(auv3wrappermacosextension_sources
            Shared/AUv3WrapperFactory.mm
        )

        set(auv3wrappermacoslib_sources
            Shared/AUv3Wrapper.mm
            ${SDK_ROOT}/base/source/baseiids.cpp
            ${SDK_ROOT}/base/source/fbuffer.cpp
            ${SDK_ROOT}/base/source/fdebug.cpp
            ${SDK_ROOT}/base/source/fobject.cpp
            ${SDK_ROOT}/base/source/fstreamer.cpp
            ${SDK_ROOT}/base/source/fstring.cpp
            ${SDK_ROOT}/base/source/timer.cpp
            ${SDK_ROOT}/base/source/updatehandler.cpp
            ${SDK_ROOT}/base/thread/source/flock.cpp
            ${SDK_ROOT}/pluginterfaces/base/conststringtable.cpp
            ${SDK_ROOT}/pluginterfaces/base/coreiids.cpp
            ${SDK_ROOT}/pluginterfaces/base/funknown.cpp
            ${SDK_ROOT}/pluginterfaces/base/ustring.cpp
            ${SDK_ROOT}/public.sdk/source/common/pluginview.cpp
            ${SDK_ROOT}/public.sdk/source/main/macmain.cpp
            ${SDK_ROOT}/public.sdk/source/main/pluginfactoryvst3.cpp
            ${SDK_ROOT}/public.sdk/source/vst/auwrapper/NSDataIBStream.mm
            ${SDK_ROOT}/public.sdk/source/vst/hosting/eventlist.cpp
            ${SDK_ROOT}/public.sdk/source/vst/hosting/hostclasses.cpp
            ${SDK_ROOT}/public.sdk/source/vst/hosting/parameterchanges.cpp
            ${SDK_ROOT}/public.sdk/source/vst/hosting/processdata.cpp
            ${SDK_ROOT}/public.sdk/source/vst/vstguieditor.cpp
            ${SDK_ROOT}/public.sdk/source/vst/vstinitiids.cpp
            ${SDK_ROOT}/public.sdk/source/vst/vstnoteexpressiontypes.cpp
            ${SDK_ROOT}/public.sdk/source/vst/vstpresetfile.cpp
            ${SDK_ROOT}/public.sdk/source/vst3stdsdk.cpp
            ${VSTGUI_ROOT}/vstgui4/vstgui/plugin-bindings/vst3editor.cpp
            ${VSTGUI_ROOT}/vstgui4/vstgui/plugin-bindings/vst3groupcontroller.cpp
            ${VSTGUI_ROOT}/vstgui4/vstgui/plugin-bindings/vst3padcontroller.cpp
            ${VSTGUI_ROOT}/vstgui4/vstgui/uidescription/xmlparser.cpp
        )

        set(auv3wrapperios_sources
            ${VSTGUI_ROOT}/vstgui4/vstgui/vstgui_uidescription.cpp
            AUv3WrapperiOS/AppDelegate.m
            AUv3WrapperiOS/ViewController.m
            AUv3WrapperiOS/main.m
            Shared/AUv3AudioEngine.h
            Shared/AUv3AudioEngine.mm
        )

        set(auv3wrapperios_xib_resources
            AUv3WrapperiOS/Base.lproj/LaunchScreen.storyboard
            AUv3WrapperiOS/Base.lproj/Main.storyboard
            AUv3WrappermacOS/Assets.xcassets
        )

        set(auv3wrapperiosextension_sources
            Shared/AUv3WrapperFactory.mm
            ${VSTGUI_ROOT}/vstgui4/vstgui/vstgui_uidescription.cpp
        )

        set(auv3wrapperioslib_sources
            ${VSTGUI_ROOT}/vstgui4/vstgui/vstgui_ios.mm
            ${SDK_ROOT}/base/thread/source/fcondition.cpp
        )

        function(add_shared_functions varName macOS)
            set_target_properties(${varName} PROPERTIES ${SDK_IDE_PLUGIN_EXAMPLES_FOLDER})
            get_filename_component(AudioUnitConfig "audiounitconfig.h" ABSOLUTE)
            set_target_properties(${varName} PROPERTIES XCODE_ATTRIBUTE_INFOPLIST_PREFIX_HEADER ${AudioUnitConfig})
            smtg_set_bundle(${varName} INFOPLIST "${InfoPlistFile}" PREPROCESS)
            smtg_add_vst3_resource(${varName} "Shared/drumLoop.wav")
            add_vst_resources(${varName})
            if (macOS)
                set_target_properties(${target} PROPERTIES MACOSX_BUNDLE TRUE)
                target_link_libraries(${varName} PRIVATE 
                    auv3wrappermacoslib 
                    "-framework Accelerate" 
                    "-framework AppKit" 
                    "-framework AudioToolbox" 
                    "-framework AVFoundation" 
                    "-framework CoreAudioKit" 
                    "-framework OpenGL" 
                    "-framework QuartzCore"
                    )
            else()
                set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
                set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${SMTG_IOS_DEVELOPMENT_TEAM})
                set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_ENABLE_BITCODE "NO") # prevent warning
                smtg_set_platform_ios(${target})
                target_link_libraries(${varName} PRIVATE 
                    auv3wrapperioslib 
                    "-framework UIKit" 
                    "-framework CoreAudioKit" 
                    "-framework AudioToolbox" 
                    "-framework CoreGraphics" 
                    "-framework QuartzCore" 
                    "-framework CoreText" 
                    "-framework Accelerate" 
                    "-framework AVFoundation" 
                    "-framework ImageIO" 
                    "-framework MobileCoreServices" 
                    "-framework MediaPlayer"
                    )
            endif(macOS)
            set_target_properties(${varName} PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
        endfunction()

        #--------------------------------------------------------------------------------------------------------
        set(target auv3wrappermacoslib)
        add_library(${target} STATIC ${auv3wrappermacoslib_sources} ${vst_sources})
        set_target_properties(${target} PROPERTIES ${SDK_IDE_LIBS_FOLDER} XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
        target_include_directories(${target} PUBLIC ${ROOT}/vstgui4)
        target_link_libraries(${target} PRIVATE base sdk vstgui_support)

        #--------------------------------------------------------------------------------------------------------
        set(target auv3wrappermacosextension)
        add_library(${target} MODULE ${${target}_sources})
        get_filename_component(InfoPlistFile "Shared/Info.plist" ABSOLUTE)
        set_target_properties(${target} PROPERTIES BUNDLE YES XCODE_PRODUCT_TYPE com.apple.product-type.app-extension)
        #set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_LIST_DIR}/AUv3WrappermacOSExtension/AUv3WrappermacOSExtension.entitlements")
        #set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Mac Developer")
        #set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${SMTG_IOS_DEVELOPMENT_TEAM})
        add_shared_functions(${target} TRUE)

        #--------------------------------------------------------------------------------------------------------
        set(target auv3wrappermacos)
        add_executable(${target} ${${target}_sources} ${${target}_xib_resources})
        get_filename_component(InfoPlistFile "AUv3WrappermacOS/Info.plist" ABSOLUTE)
        add_dependencies(${target} "auv3wrappermacosextension")
        set_target_properties(${target} PROPERTIES RESOURCE "${${target}_xib_resources}")
        add_shared_functions(${target} TRUE)

        #--------------------------------------------------------------------------------------------------------
        set(target auv3wrapperioslib)
        add_library(${target} STATIC ${auv3wrappermacoslib_sources} ${auv3wrapperioslib_sources} ${vst_sources})
        set_target_properties(${target} PROPERTIES ${SDK_IDE_LIBS_FOLDER} XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
        set_target_properties(${target} PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
        smtg_set_platform_ios(${target})
        target_include_directories(${target} PUBLIC ${ROOT}/vstgui4)
        target_link_libraries(${target} PRIVATE base_ios sdk_ios "-framework UIKit" "-framework CoreAudioKit" "-framework AudioToolbox" "-framework AVFoundation")
        add_vst_resources(${target})

        #--------------------------------------------------------------------------------------------------------
        set(target auv3wrapperiosextension)
        add_library(${target} MODULE ${${target}_sources})
        get_filename_component(InfoPlistFile "Shared/Info.plist" ABSOLUTE)
        set_target_properties(${target} PROPERTIES BUNDLE YES XCODE_PRODUCT_TYPE com.apple.product-type.app-extension)
        set_target_properties(${target} PROPERTIES MACOSX_BUNDLE TRUE MACOSX_BUNDLE_INFO_PLIST ${InfoPlistFile})
        add_shared_functions(${target} FALSE)

        #--------------------------------------------------------------------------------------------------------
        set(target auv3wrapperios)
        add_executable(${target} ${${target}_sources} ${${target}_xib_resources})
        get_filename_component(InfoPlistFile "AUv3WrapperiOS/Info.plist" ABSOLUTE)
        add_dependencies(${target} "auv3wrapperiosextension")
        set_target_properties(${target} PROPERTIES BUNDLE TRUE)
        set_target_properties(${target} PROPERTIES ${SDK_IDE_LIBS_FOLDER} XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
        set_target_properties(${target} PROPERTIES RESOURCE "${${target}_xib_resources}")
        set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE "YES")
        add_shared_functions(${target} FALSE)
    else()
        message("* To enable building the AUv3 Wrapper example for iOS you need to set the SMTG_IOS_DEVELOPMENT_TEAM and use the Xcode generator")
    endif()
endif()
