//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/source/vst/axwrapper/docaax.h
// Created by  : Steinberg, 09/2017
// Description : VST 3 -> AAX Wrapper
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
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

/**

*******************************************
\page AAXWrapper VST 3 - AAX Wrapper
*******************************************

Helper Class wrapping a VST 3 Plug-in to a AAX Plug-in

***************************
\section AAXIntroduction Introduction
***************************
The VST 3 SDK comes with a helper class which wraps one VST 3 Audio Processor and Edit Controller to
a AAX Plug-in.
\n\n

***************************
\section howtoAAX How does it work? 
***************************
- Check the AGainAAX example
	
- AAX SDK 2.3 or higher is expected in folder external.avid.aax (located at the same level than the folder public.sdk)
	
- here the step based on the AgainAAX example:
	- in the cpp file againaax.cpp you can define the plugin properties : IO audio, product ID, ...

	- on Windows copy built linker output again_aax.aaxplugin to 
		"c:\Program Files\Common Files\Avid\Audio\Plug-Ins\again_aax.aaxplugin\Contents\x64" 
		(the debug build does this automatically, but needs appropriate access rights (Administrator rights of your visual))

	- on OSX copy built bundle build/Debug/again.aaxplugin to 
		"/Library/Application Support/Avid/Audio/Plug-Ins" 

- a developer version of Pro Tools is needed to load the plugin, the release version of Pro Tools requires
plugins to be Pace-signed (PACE Anti-Piracy Inc.: https://www.ilok.com)

*/
