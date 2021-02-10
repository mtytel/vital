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

#import "ViewController.h"
#import <CoreAudioKit/AUViewController.h>
#import "../audiounitconfig.h"
#import "../Shared/AUv3AudioEngine.h"
#import "../Shared/AUv3Wrapper.h"

@class AUv3WrapperViewController;

@interface ViewController () {
	// Button for playback
	IBOutlet UIButton *playButton;

	// Container for our custom view.
	__weak IBOutlet UIView *auContainerView;
	
	// Audio playback engine.
	AUv3AudioEngine *audioEngine;
	
	// Container for the custom view.
	AUv3WrapperViewController *auV3ViewController;
}

-(IBAction)togglePlay:(id)sender;
-(IBAction)loadFile:(id)sender;

@end

@implementation ViewController

//------------------------------------------------------------------------
- (void)viewDidLoad {
	[super viewDidLoad];
	
	// Do any additional setup after loading the view.
	[self embedPlugInView];

	AudioComponentDescription desc;

	desc.componentType = kAUcomponentType;
	desc.componentSubType = kAUcomponentSubType;
	desc.componentManufacturer = kAUcomponentManufacturer;
	desc.componentFlags = kAUcomponentFlags;
	desc.componentFlagsMask = kAUcomponentFlagsMask;

	[AUAudioUnit registerSubclass: AUv3Wrapper.class asComponentDescription:desc name:@"Local AUv3" version: UINT32_MAX];
	
	audioEngine = [[AUv3AudioEngine alloc] initWithComponentType:desc.componentType];
	
	[audioEngine loadAudioUnitWithComponentDescription:desc completion:^{
		auV3ViewController.audioUnit = (AUv3Wrapper*)audioEngine.currentAudioUnit;
	}];
}

//------------------------------------------------------------------------
- (void)embedPlugInView {
	NSURL *builtInPlugInURL = [[NSBundle mainBundle] builtInPlugInsURL];
	NSURL *pluginURL = [builtInPlugInURL URLByAppendingPathComponent: @"AUv3WrapperiOSExtension.appex"];
	NSBundle *appExtensionBundle = [NSBundle bundleWithURL: pluginURL];
	
	auV3ViewController = [[AUv3WrapperViewController alloc] initWithNibName: @"AUv3WrapperViewController" bundle: appExtensionBundle];
	
	// Present the view controller's view.
	UIView *view = auV3ViewController.view;
	view.frame = auContainerView.bounds;
	
	[auContainerView addSubview: view];
	
	view.translatesAutoresizingMaskIntoConstraints = NO;
	
	NSArray *constraints = [NSLayoutConstraint constraintsWithVisualFormat: @"H:|-[view]-|" options:0 metrics:nil views:NSDictionaryOfVariableBindings(view)];
	[auContainerView addConstraints: constraints];
	
	constraints = [NSLayoutConstraint constraintsWithVisualFormat: @"V:|-[view]-|" options:0 metrics:nil views:NSDictionaryOfVariableBindings(view)];
	[auContainerView addConstraints: constraints];
}

//------------------------------------------------------------------------
- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
	// Dispose of any resources that can be recreated.
}

//------------------------------------------------------------------------
-(IBAction)loadFile:(id)sender {
	MPMediaPickerController *soundPicker=[[MPMediaPickerController alloc]
										  initWithMediaTypes:MPMediaTypeAnyAudio];
	soundPicker.delegate=self;
	soundPicker.allowsPickingMultipleItems=NO; // You can set it to yes for multiple selection
	[self presentViewController:soundPicker animated:YES completion:nil];
}

//------------------------------------------------------------------------
-(void)mediaPicker:(MPMediaPickerController *)mediaPicker didPickMediaItems:
(MPMediaItemCollection *)mediaItemCollection
{
	MPMediaItem *item = [[mediaItemCollection items] objectAtIndex:0]; // For multiple you can iterate iTems array
	NSURL *url = [item valueForProperty:MPMediaItemPropertyAssetURL];
	[mediaPicker dismissViewControllerAnimated:YES completion:nil];
	NSError* error = [audioEngine loadAudioFile:url];
	if (error != nil)
	{
		NSLog(@"something went wrong");
	}
}

- (void) mediaPickerDidCancel: (MPMediaPickerController *) mediaPicker
{
	[mediaPicker dismissViewControllerAnimated:YES completion:nil];
}

//------------------------------------------------------------------------
-(IBAction)togglePlay:(id)sender {	
	BOOL isPlaying = [audioEngine startStop];
	
	[playButton setTitle: isPlaying ? @"Stop" : @"Play" forState: UIControlStateNormal];
}

@end
