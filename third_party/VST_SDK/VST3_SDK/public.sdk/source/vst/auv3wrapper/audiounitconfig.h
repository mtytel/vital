//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    :
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

// AUWRAPPER_CHANGE: change all corresponding entries according to your plugin

// The specific variant of the Audio Unit app extension.
// The four possible types and their values are:
// Effect (aufx), Generator (augn), Instrument (aumu), and Music Effect (aufm)
#define kAUcomponentType 'aufx'
#define kAUcomponentType1 aufx

// A subtype code (unique ID) for the audio unit, such as gav3.
// This value must be exactly 4 alphanumeric characters
#define kAUcomponentSubType 'gav3'
#define kAUcomponentSubType1 gav3

// A manufacturer code for the audio unit, such as Aaud.
// This value must be exactly 4 alphanumeric characters
#define kAUcomponentManufacturer 'Stgb'
#define kAUcomponentManufacturer1 Stgb

// A product name for the audio unit
#define kAUcomponentDescription AUv3WrapperExtension

// The full name of the audio unit.
// This is derived from the manufacturer and description key values
#define kAUcomponentName Steinberg: AGainv3

// Displayed Tags
#define kAUcomponentTag Effects

// A version number for the Audio Unit app extension (decimal value of hexadecimal representation with zeros between subversions)
// Hexadecimal indexes representing: [0] = main version, [1] = 0 = dot, [2] = sub version, [3] = 0 = dot, [4] = sub-sub version,
// e.g. 1.0.0 == 0x10000 == 65536, 1.2.3 = 0x10203 = 66051
#define kAUcomponentVersion 65536

// Supported number of channels of your audio unit.
// Integer indexes representing: [0] = input count, [1] = output count, [2] = 2nd input count,
// [3]=2nd output count, etc.
// e.g. 1122 == config1: [mono input, mono output], config2: [stereo input, stereo output]
// see channelCapabilities for discussion
#define kSupportedNumChannels 1122

// The preview audio file name.
// To add your own custom audio file (for standalone effects), add an audio file to the project (AUv3WrappermacOS and AUv3WrapperiOS targets) and
// enter the file name here
#define kAudioFileName "drumLoop"

// The preview audio file format.
// To add your own custom audio file (for standalone effects), add an audio file to the project (AUv3WrappermacOS and AUv3WrapperiOS targets) and
// enter the file format here
#define kAudioFileFormat "wav"

// componentFlags (leave at 0)
#define kAUcomponentFlags 0

// componentFlagsMask (leave at 0)
#define kAUcomponentFlagsMask 0
