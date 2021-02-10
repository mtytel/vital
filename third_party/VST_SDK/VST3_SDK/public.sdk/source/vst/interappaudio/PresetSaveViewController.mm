//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/interappaudio/PresetSaveViewController.mm
// Created by  : Steinberg, 09/2013
// Description : VST 3 InterAppAudio
// Flags       : clang-format SMTGSequencer
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

#import "PresetSaveViewController.h"
#import "pluginterfaces/base/funknown.h"

//------------------------------------------------------------------------
@interface PresetSaveViewController ()
//------------------------------------------------------------------------
{
	IBOutlet UIView* containerView;
	IBOutlet UITextField* presetName;

	std::function<void (const char* presetPath)> callback;
	Steinberg::FUID uid;
}
@end

//------------------------------------------------------------------------
@implementation PresetSaveViewController
//------------------------------------------------------------------------

//------------------------------------------------------------------------
- (id)initWithCallback:(std::function<void (const char* presetPath)>)_callback
{
	self = [super initWithNibName:@"PresetSaveView" bundle:nil];
	if (self)
	{
		callback = _callback;

		self.modalPresentationStyle = UIModalPresentationOverCurrentContext;
		self.modalTransitionStyle = UIModalTransitionStyleCrossDissolve;

		UIViewController* rootViewController =
		    [[UIApplication sharedApplication].windows[0] rootViewController];

		[rootViewController presentViewController:self
		                                 animated:YES
		                               completion:^{ [self showKeyboard]; }];
	}
	return self;
}

//------------------------------------------------------------------------
- (void)viewDidLoad
{
	[super viewDidLoad];

	containerView.layer.shadowOpacity = 0.5;
	containerView.layer.shadowOffset = CGSizeMake (5, 5);
	containerView.layer.shadowRadius = 5;
}

//------------------------------------------------------------------------
- (void)showKeyboard
{
	[presetName becomeFirstResponder];
}

//------------------------------------------------------------------------
- (void)removeSelf
{
	[self dismissViewControllerAnimated:YES completion:^{}];
}

//------------------------------------------------------------------------
- (NSURL*)presetURL
{
	NSFileManager* fs = [NSFileManager defaultManager];
	NSURL* documentsUrl = [fs URLForDirectory:NSDocumentDirectory
	                                 inDomain:NSUserDomainMask
	                        appropriateForURL:Nil
	                                   create:YES
	                                    error:NULL];
	if (documentsUrl)
	{
		NSURL* presetPath = [[documentsUrl URLByAppendingPathComponent:presetName.text]
		    URLByAppendingPathExtension:@"vstpreset"];
		return presetPath;
	}
	return nil;
}

//------------------------------------------------------------------------
- (BOOL)textFieldShouldReturn:(UITextField*)textField
{
	if ([textField.text length] > 0)
	{
		[self save:textField];
		return YES;
	}
	return NO;
}

//------------------------------------------------------------------------
- (IBAction)save:(id)sender
{
	if (callback)
	{
		NSURL* presetPath = [self presetURL];
		NSFileManager* fs = [NSFileManager defaultManager];
		if ([fs fileExistsAtPath:[presetPath path]])
		{
			// alert for overwrite
			auto alertController = [UIAlertController
			    alertControllerWithTitle:NSLocalizedString (
			                                 @"A Preset with this name already exists",
			                                 "Alert title")
			                     message:NSLocalizedString (@"Save it anyway ?", "Alert message")
			              preferredStyle:UIAlertControllerStyleAlert];
			[alertController
			    addAction:[UIAlertAction
			                  actionWithTitle:NSLocalizedString (@"Save", "Alert Save Button")
			                            style:UIAlertActionStyleDefault
			                          handler:^(UIAlertAction* _Nonnull action) {
				                        callback ([[[self presetURL] path] UTF8String]);
				                        [self removeSelf];
			                          }]];
			[alertController
			    addAction:[UIAlertAction
			                  actionWithTitle:NSLocalizedString (@"Cancel", "Alert Cancel Button")
			                            style:UIAlertActionStyleCancel
			                          handler:^(UIAlertAction* _Nonnull action) {}]];
			[self presentViewController:alertController animated:YES completion:nil];
			return;
		}
		callback ([[presetPath path] UTF8String]);
	}
	[self removeSelf];
}

//------------------------------------------------------------------------
- (IBAction)cancel:(id)sender
{
	if (callback)
	{
		callback (nullptr);
	}
	[self removeSelf];
}

//------------------------------------------------------------------------
- (BOOL)prefersStatusBarHidden
{
	return YES;
}

@end
