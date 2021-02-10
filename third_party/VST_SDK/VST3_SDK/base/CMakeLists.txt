
set(base_sources
	source/baseiids.cpp
	source/classfactoryhelpers.h
	source/fbuffer.cpp
	source/fbuffer.h
	source/fcleanup.h
	source/fcommandline.h
	source/fdebug.cpp
	source/fdebug.h
	source/fdynlib.cpp
	source/fdynlib.h
	source/fobject.cpp
	source/fobject.h
	source/fstreamer.cpp
	source/fstreamer.h
	source/fstring.cpp
	source/fstring.h
	source/timer.cpp
	source/timer.h
	source/updatehandler.cpp
	source/updatehandler.h
	thread/include/fcondition.h
	thread/include/flock.h
	thread/source/fcondition.cpp
	thread/source/flock.cpp
)

set(base_sources_ext
	source/basefwd.h
	source/classfactory.cpp
	source/classfactory.h
	source/fbitset.cpp
	source/fbitset.h
	source/fcontainer.h
	source/fcpu.cpp
	source/fcpu.h
	source/fcriticalperformance.cpp
	source/fcriticalperformance.h
	source/finitializer.cpp
	source/finitializer.h
	source/fmemory.cpp
	source/fmemory.h
	source/fpoint.cpp
	source/fpoint.h
	source/frect.cpp
	source/frect.h
	source/fregion.cpp
	source/fregion.h
	source/fstdmethods.h
	source/fstringmethods.h
	source/fstringstream.h
	source/funknownfactory.h
	source/hexbinary.h
	source/istreamwrapper.cpp
	source/istreamwrapper.h
	source/tringbuffer.h	
	thread/include/fatomic.h
	thread/include/forwarddeclarations.h
	thread/include/frwlock_generic.h
	thread/include/frwlock_macosx.h
	thread/include/frwlock_windows.h
	thread/include/fthread.h
	thread/source/fatomic.cpp
	thread/source/fthread.cpp
)
if (VST_SDK)
	add_library(base STATIC ${base_sources})
else()
	add_library(base STATIC ${base_sources} ${base_sources_ext})
endif()

if(MSVC)
	set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /IGNORE:4221")
endif()

smtg_setup_universal_binary(base)

# iOS target
if(MAC AND XCODE AND SMTG_IOS_DEVELOPMENT_TEAM)
	add_library(base_ios STATIC ${base_sources})
	smtg_set_platform_ios(base_ios)
endif()
