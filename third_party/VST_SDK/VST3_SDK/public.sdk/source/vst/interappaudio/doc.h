//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Documentation
// Filename    : public.sdk/source/vst/interappaudio/doc.h
// Created by  : Steinberg, 10/2005
// Description : Inter-App Audio Documentation of VST 3 SDK
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
\page interappaudio iOS Inter-App Audio support

iOS InterApp-Audio Application out of your VST 3 Plug-in

\section iaa_introduction Introduction

The VST 3 SDK provides an easy way to create an iOS InterApp-Audio Application out of your VST 3 Plug-in.

The SDK comes with an iOS VST 3 host application that can run standalone or as an Inter-App Audio slave.
If your plug-in does not use any specific Windows or Mac OS X API's, it should be reasonably easy to get
your plug-in running on iOS.

If you use VSTGUI4 with the VST3Editor class as your UI, you mainly only have to create a new UI description
for the different device sizes.

\section iaa_howto How does it work?

\subsection iaa_howto1 Create and Setup the Xcode Project
- Create a new iOS Application in Xcode. Use the "Empty Application" Template.
- Use "C++11" as "C++ Language Dialect".
- Activate "Objective-C Automatic Reference Counting".
- Change the Bundle Identifier according to your registered AppID at Apple (see the Apple Documentation).
- Activate the "Inter-App Audio" capability for the project.
- Activate the "Background Modes" capability for the project and check the "Audio and Airplay" mode.
- Change the "Header Search Path" configuration to add a path to the root of the VST 3 SDK.

\subsection iaa_howto2 Add your Audio Component description to the Info.plist
- Add a new Array with the name "AudioComponents".
- Add a new Dictionary to the array.
- Add the following keys to the dictionary: manufacturer, name, type, subtype, version.
- Set the values of these keys:
- The manufacturer value must be the one registered with Apple (the one you use for AudioUnits).
- The type value must be either 'aurx' (Effect), 'aurm' (Effect with MIDI), 'aurg' (Generator) or 'auri' (Instrument).
- The version value must be 1.

\subsection iaa_howto3 Add files to the project
- all the sources from your VST 3 Plug-in project
- all files from public.sdk/source/vst/interappaudio/
- all files from public.sdk/source/vst/hosting
- all files from base/source/ (if not already done previously)
- public.sdk/source/vst/auwrapper/NSDataIBStream.mm (if not already done previously)

\subsection iaa_howto4 If using vstgui
- add vstgui_ios.mm
- add vstgui_uidescription_ios.mm

\section iaa_codechanges Code changes

Xcode created some files for you when you created the project. One of it is the App Delegate.
You need to change the base class from NSResponder<UIApplicationDelegate> to VSTInterAppAudioAppDelegateBase
(you need to import its header file which is here : public.sdk/source/vst/interappaudio/VSTInterAppAudioAppDelegateBase.h)
and then remove all methods from it.
If you want to add some custom behavior to your app, you should do it in your App Delegate class implementation.
Per example, if you want to only allow landscape mode, you have to add this method:
\code
- (NSUInteger)application:(UIApplication *)application supportedInterfaceOrientationsForWindow:(UIWindow *)window
{
  return UIInterfaceOrientationMaskLandscapeLeft|UIInterfaceOrientationMaskLandscapeRight;
}
\endcode
Make sure to call the super method if you override one of the methods implemented by VSTInterAppAudioAppDelegateBase (see the header which one are implemented).

\section iaa_ui UI Handling

\subsection iaa_editor Creating a different UI when running on iOS
To create a different view for iOS, you can check the host context for the IInterAppAudioWrapper interface:
\code
//-----------------------------------------------------------------------------
IPlugView* PLUGIN_API MyEditController::createView (FIDString _name)
{
  ConstString name (_name);
  if (name == ViewType::kEditor)
  {
    FUnknownPtr<IInterAppAudioHost> interAudioApp (getHostContext ());
    if (interAudioApp)
    {
      // create and return the view for iOS
    }
    else
    {
      // create and return the view for Windows/macOSX
    }
  }
  return 0;
}
\endcode

\subsection iaa_ui_vstgui Using VSTGUI
VSTGUI 4.3 or higher includes support for iOS. You can use it the same like you use it for Windows or Mac OS X. See the VSTGUI documentation for some limitations.

\subsection iaa_ui_native Using a native UIView
If you want to create a native UIView as your plug-in editor, you have to create your own IPlugView derivate and attach the UIView in the IPlugView::attached method.
An example of this can be seen in public.sdk/samples/vst/adelay/interappaudio/iosEditor.mm

\subsection iaa_host_ui Host UI Integration
The example project in public.sdk/samples/vst/InterAppAudio uses a custom solution to show the UI for controlling and switching to the host application.
You should take this as an example if you want to add it this way. You can also use the UI Apple provides with the its example source for Inter-App Audio.

But you can also implement the host UI integration into your plug-in view. Your edit controller will get an interface to IInterAppAudioHost via its initialize method
which you can use to get the host icon and send commands or switch to the host. If you use this method to show the host controls you should implement
the IInterAppAudioConnectionNotification interface in your edit controller to conditionally show or hide the controls depending on the connection state.
\n\n
*/
