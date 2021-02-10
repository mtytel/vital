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

#import "AUv3AudioEngine.h"
#import "../audiounitconfig.h"

@implementation AUv3AudioEngine
{
	AVAudioEngine* audioEngine;
	AVAudioFile* audioFile;
	AVAudioPCMBuffer* audioPCMBuffer;
	AVAudioPlayerNode* playerNode;
	UInt32 componentType;
	BOOL playing;
	BOOL isDone;
	MIDIPortRef* inputPort;
	MIDIClientRef* midiClient;
}

//------------------------------------------------------------------------
- (instancetype)initWithComponentType: (uint32_t) unitComponentType
{
	self = [super init];
	
	isDone = false;
	
	if (self) {
		audioEngine = [[AVAudioEngine alloc] init];
		componentType = unitComponentType;
	}
	
	NSError *error = nil;
	if (componentType == kAudioUnitType_Effect)
	{
		playerNode = [[AVAudioPlayerNode alloc] init];
		
		NSString *fileName = @kAudioFileName;
		NSString *fileFormat = @kAudioFileFormat;
		
		NSURL *fileURL = [[NSBundle mainBundle] URLForResource:fileName withExtension:fileFormat];
		audioFile = [[AVAudioFile alloc] initForReading:fileURL error:&error];
	}
	
	if (error)
		NSLog(@"Error setting up audio or midi file");
	
#if TARGET_OS_IPHONE
	BOOL success = [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:&error];
	if(NO == success)
	{
		NSLog(@"Error setting category: %@", [error localizedDescription]);
	}
#endif
	
	playing = false;
	
	return self;
}

//------------------------------------------------------------------------
- (void) loadAudioUnitWithComponentDescription: (AudioComponentDescription) desc completion:(void (^)(void))completionBlock
{
	[AVAudioUnit instantiateWithComponentDescription:desc options:0 completionHandler:^(AVAudioUnit * audioUnit, NSError *error) {
		if(audioUnit == nil)
			return;
		
		_currentAudioUnit = audioUnit.AUAudioUnit;
		[audioEngine attachNode:audioUnit];
		[audioEngine connect:audioUnit to:audioEngine.outputNode format:audioFile.processingFormat];
		
		if (componentType == kAudioUnitType_Effect)
		{
			[audioEngine attachNode:playerNode];
			[audioEngine connect:playerNode to:audioUnit format:audioFile.processingFormat];
		}
		
		completionBlock();
	}];
}

//------------------------------------------------------------------------
- (NSError*)loadAudioFile:(NSURL*) url
{
	NSError *error = nil;
	
	if (playing)
	{
		[self startStop];
		[playerNode stop];
		audioFile = [[AVAudioFile alloc] initForReading:url error:&error];
		[self startStop];
	}
	else
	{
		[playerNode stop];
		audioFile = [[AVAudioFile alloc] initForReading:url error:&error];
	}
	
	return error;
}

//------------------------------------------------------------------------
- (BOOL) startStop
{
	playing = !playing;
	
	playing ? ([self startPlaying]) : ([self stopPlaying]);
	
	return playing;
}

//------------------------------------------------------------------------
- (void) startPlaying
{
	[self activateSession:true];
	NSError *error = nil;
	
	if (![audioEngine startAndReturnError:&error]) {
		NSLog(@"engine failed to start: %@", error);
		return;
	}
	
	if (componentType == kAudioUnitType_Effect)
	{
		[self loopAudioFile];
		[playerNode play];
	}
	else if (componentType == kAudioUnitType_MusicDevice)
	{
		isDone = false;
		[self loopMIDIsequence];
	}
}

//------------------------------------------------------------------------
- (void) stopPlaying
{
	if (componentType == kAudioUnitType_Effect)
	{
		[playerNode stop];
	}
	else if (componentType == kAudioUnitType_MusicDevice)
	{
		// stop the instrument
		@synchronized (self) {}
	}
	
	[audioEngine stop];
	[self activateSession:false];
}

//------------------------------------------------------------------------
- (void) loopAudioFile
{
	[playerNode scheduleFile:audioFile atTime:nil completionHandler:^{
		if (playerNode.playing)
			[self loopAudioFile];
	}];
}

//------------------------------------------------------------------------
- (void) loopMIDIsequence
{
	UInt8 cbytes[3], *cbytesPtr;
	
	cbytesPtr = cbytes;
	
	dispatch_async (dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		cbytesPtr[0] = 0xB0;
		cbytesPtr[1] = 123;
		cbytesPtr[2] = 0;
		
		if (_currentAudioUnit.scheduleMIDIEventBlock == nil)
			return;
		
		_currentAudioUnit.scheduleMIDIEventBlock(AUEventSampleTimeImmediate, 0, 3, cbytesPtr);
		usleep(useconds_t(0.1 * 1e6));
		
		float releaseTime = 0.05;
		
		usleep(useconds_t(0.1 * 1e6));
		
		int i = 0;
		
		@synchronized(self) {
			while (playing)
			{
				if (releaseTime < 10.0)
					releaseTime = (releaseTime * 1.05) > 10.0 ? (releaseTime * 1.05) : 10.0;
				
				cbytesPtr[0] = 0x90;
				cbytesPtr[1] = UInt8(60 + i);
				cbytesPtr[2] = UInt8(64); // note on
				_currentAudioUnit.scheduleMIDIEventBlock(AUEventSampleTimeImmediate, 0, 3, cbytesPtr);
				
				usleep(useconds_t(0.2 * 1e6));
				
				cbytesPtr[0] = 0x80;
				cbytesPtr[1] = UInt8(60 + i);
				cbytesPtr[2] = UInt8(0);    // note off
				_currentAudioUnit.scheduleMIDIEventBlock(AUEventSampleTimeImmediate, 0, 3, cbytesPtr);
				
				i += 2;
				if (i >= 24) {
					i = -12;
				}
			}
			
			cbytesPtr[0] = 0xB0;
			cbytesPtr[1] = 123;
			cbytesPtr[2] = 0;
			_currentAudioUnit.scheduleMIDIEventBlock(AUEventSampleTimeImmediate, 0, 3, cbytesPtr);
			
			isDone = true;
		}
	});
}

//------------------------------------------------------------------------
- (void) activateSession: (BOOL) active
{
#if TARGET_OS_IPHONE
	NSError * error = nil;
	BOOL success = [[AVAudioSession sharedInstance] setActive:active error:nil];
	if(NO == success)
	{
		NSLog(@"Error setting category: %@", [error localizedDescription]);
	}
#endif
}

@end
