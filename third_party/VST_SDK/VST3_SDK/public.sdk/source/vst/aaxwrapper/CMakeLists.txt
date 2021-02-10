
set(target aaxwrapper)
set(${target}_sources
	aaxentry.cpp
	aaxlibrary.cpp
	aaxwrapper.cpp
	aaxwrapper.h
	aaxwrapper_description.h
	aaxwrapper_gui.cpp
	aaxwrapper_gui.h
	aaxwrapper_parameters.cpp
	aaxwrapper_parameters.h
	docaax.h
	resource/PlugIn.ico
)
add_library(${target} STATIC ${${target}_sources})

target_include_directories(${target} PRIVATE "${SMTG_AAX_SDK_PATH}/Interfaces")
target_include_directories(${target} PRIVATE "${SMTG_AAX_SDK_PATH}/Interfaces/ACF")
target_include_directories(${target} PRIVATE "${SMTG_AAX_SDK_PATH}/Libs/AAXLibrary/Include")
smtg_setup_universal_binary(${target})

if(XCODE)
	add_compile_options(-Wno-incompatible-ms-struct)
elseif(WIN)
	add_compile_options(/wd4996)
	add_compile_options(/GR)
	# too much warnings in the AAX SDK!!
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()
