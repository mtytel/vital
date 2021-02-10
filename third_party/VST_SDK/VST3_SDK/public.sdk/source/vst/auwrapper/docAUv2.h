//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/auwrapper/docAUv2.h
// Created by  : Steinberg, 07/2017
// Description : VST 3 -> AU Wrapper
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2018, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

/**

*******************************************
\page AUWrapper VST 3 - Audio Unit Wrapper
*******************************************

Helper Class wrapping a VST 3 Plug-in to a Audio Unit v2 Plug-in

***************************
\section AUIntroduction Introduction
***************************
The VST 3 SDK comes with an AudioUnit wrapper, which can wrap one VST 3 Audio Processor and Edit Controller as an AudioUnit effect/instrument.

The wrapper is a small dynamic library which loads the VST 3 Plug-in.
As AudioUnits store some important information in their resource fork, this library must be compiled for every VST 3 Plug-in.
\n\n

***************************
\section AUhowdoesitwork How does it work?
***************************
- build the auwrapper project (public.sdk/source/vst/auwrapper/auwrapper.xcodeproj)
- create a copy of the again AU wrapper example project directory (public.sdk/source/vst/auwrapper/again/)
- rename the copy to your needs
- edit the target settings of the project and change
	- Product Name
	- Library search path so that it points to the directory where libauwrapper.a exists
	- architecture setting so that it only includes architectures the VST 3 Plug-in supports

- search in the project for AUWRAPPER_CHANGE and change the settings to your needs, especially in :
	- edit audiounitconfig.h see comments there
	- edit Info.plist see comments there
- edit the "Make Links Script" for easier debugging/development
- build your project
- done... that is all!

For the release version, you must place a copy or an alias of your VST 3 Plug-in in the resource folder of the bundle named "plugin.vst3"

*/
