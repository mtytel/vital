//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/auv3wrapper/docAUv3.h
// Created by  : Steinberg, 07/2017.
// Description : VST 3 AUv3Wrapper
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
\page AUv3Wrapper VST 3 - Audio Unit v3 Wrapper
*******************************************

Helper Class wrapping a VST 3 Plug-in to a Audio Unit v3 Plug-in

***************************
\section AUv3Introduction Introduction
***************************
The VST 3 SDK comes with a helper class which wraps one VST 3 Audio Processor and Edit Controller to
a AUv3 Plug-in.
\n\n

***************************
\section howtoAUv3 How does it work?
***************************
- it works with VSTGUI on iOS

- Structure:
	- App          (container app which initializes the AU through small Playback Engine)
	- Extension    (extension which is loaded by hosts, also initializes the AU)
	- Library      (main wrapper library)

- How-To use the VST3->AUv3 Wrapper:
    
	- remove the AGain Source files from this project and include/link all your necessary VST3
		 Plugin source files (add to AUv3WrappermacOSLib and AUv3WrapperiOSLib target) and resource files
		 (add to AUv3WrappermacOS, AUv3WrappermacOSExtension, AUv3WrapperiOS, AUv3WrapperiOSExtension
		 targets)

	- edit the audiounitconfig.h file (see comment "AUWRAPPER_CHANGE" there)

	- rename the targets and bundle identifiers according to your needs

	- make sure you have correct code signing set up
*/
