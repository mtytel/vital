//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/aaxwrapper/aaxentry.h
// Created by  : Steinberg, 08/2017
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
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

/// \cond ignore

/**
 *	plugin entry from AAX_Exports.cpp
 */
/*================================================================================================*/

#include "pluginterfaces/base/fplatform.h"

// change names to avoid different linkage
#define ACFRegisterPlugin ACFRegisterPlugin_
#define ACFRegisterComponent ACFRegisterComponent_
#define ACFGetClassFactory ACFGetClassFactory_

#define ACFCanUnloadNow ACFCanUnloadNow_
#define ACFStartup ACFStartup_
#define ACFShutdown ACFShutdown_

//#define INITACFIDS // Make sure all of the AVX2 uids are defined.

#include "AAX.h"
#include "AAX_Init.h"
#include "acfresult.h"
#include "acfunknown.h"

#undef ACFRegisterPlugin
#undef ACFRegisterComponent
#undef ACFGetClassFactory

#undef ACFCanUnloadNow
#undef ACFStartup
#undef ACFShutdown

#if SMTG_OS_MACOS
#include <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>
#endif

extern "C" {
#if SMTG_OS_MACOS
bool bundleEntry (CFBundleRef);
bool bundleExit (void);
#else
bool InitDll (); // { return true; }
bool ExitDll (); // { return true; }
#endif
}

int AAXWrapper_linkAnchor; // reference this in the plugin to force inclusion of the wrapper in the
                           // link

//------------------------------------------------------------------------
#if defined(__GNUC__)
#define AAX_EXPORT extern "C" __attribute__ ((visibility ("default"))) ACFRESULT
#else
#define AAX_EXPORT extern "C" __declspec (dllexport) ACFRESULT __stdcall
#endif

AAX_EXPORT ACFRegisterPlugin (IACFUnknown* pUnkHost, IACFPluginDefinition** ppPluginDefinition);
AAX_EXPORT ACFRegisterComponent (IACFUnknown* pUnkHost, acfUInt32 index,
                                 IACFComponentDefinition** ppComponentDefinition);
AAX_EXPORT ACFGetClassFactory (IACFUnknown* pUnkHost, const acfCLSID& clsid, const acfIID& iid,
                               void** ppOut);

AAX_EXPORT ACFCanUnloadNow (IACFUnknown* pUnkHost);
AAX_EXPORT ACFStartup (IACFUnknown* pUnkHost);
AAX_EXPORT ACFShutdown (IACFUnknown* pUnkHost);
AAX_EXPORT ACFGetSDKVersion (acfUInt64* oSDKVersion);

//------------------------------------------------------------------------
// \func ACFRegisterPlugin
// \brief Determines the number of components defined in the dll.
//
ACFAPI ACFRegisterPlugin (IACFUnknown* pUnkHostVoid, IACFPluginDefinition** ppPluginDefinitionVoid)
{
	ACFRESULT result = ACF_OK;

	try
	{
		result = AAXRegisterPlugin (pUnkHostVoid, ppPluginDefinitionVoid);
	}
	catch (...)
	{
		result = ACF_E_UNEXPECTED;
	}

	return result;
}

//------------------------------------------------------------------------
// \func ACFRegisterComponent
// \brief Registers a specific component in the DLL.
//
ACFAPI ACFRegisterComponent (IACFUnknown* pUnkHost, acfUInt32 index,
                             IACFComponentDefinition** ppComponentDefinition)
{
	ACFRESULT result = ACF_OK;

	try
	{
		result = AAXRegisterComponent (pUnkHost, index, ppComponentDefinition);
	}
	catch (...)
	{
		result = ACF_E_UNEXPECTED;
	}

	return result;
}

//------------------------------------------------------------------------
// \func ACFGetClassFactory
// \brief Gets the factory for a given class ID.
//
ACFAPI ACFGetClassFactory (IACFUnknown* pUnkHost, const acfCLSID& clsid, const acfIID& iid,
                           void** ppOut)
{
	ACFRESULT result = ACF_OK;

	try
	{
		result = AAXGetClassFactory (pUnkHost, clsid, iid, ppOut);
	}
	catch (...)
	{
		result = ACF_E_UNEXPECTED;
	}

	return result;
}

//------------------------------------------------------------------------
// \func ACFCanUnloadNow
// \brief Figures out if all objects are released so we can unload.
//
ACFAPI ACFCanUnloadNow (IACFUnknown* pUnkHost)
{
	ACFRESULT result = ACF_OK;

	try
	{
		result = AAXCanUnloadNow (pUnkHost);
	}
	catch (...)
	{
		result = ACF_E_UNEXPECTED;
	}

	return result;
}

#if SMTG_OS_MACOS
//------------------------------------------------------------------------
static CFBundleRef GetBundleFromExecutable (const char* filepath)
{
	// AutoreleasePool ap;
	char* fname = strdup (filepath);
	int pos = strlen (fname);
	int level = 3;
	while (level > 0 && --pos >= 0)
	{
		if (fname[pos] == '/')
			level--;
	}
	if (level > 0)
		return 0;

	fname[pos] = 0;
	CFURLRef url = CFURLCreateFromFileSystemRepresentation (0, (const UInt8*)fname, pos, true);
	CFBundleRef bundle = CFBundleCreate (0, url);
	return bundle;
}

//------------------------------------------------------------------------
static CFBundleRef GetCurrentBundle ()
{
	Dl_info info;
	if (dladdr ((const void*)GetCurrentBundle, &info))
	{
		if (info.dli_fname)
		{
			return GetBundleFromExecutable (info.dli_fname);
		}
	}
	return 0;
}
#endif

//------------------------------------------------------------------------
// \func ACFStartup
// \brief Called once at init time.
//
ACFAPI ACFStartup (IACFUnknown* pUnkHost)
{
	ACFRESULT result = ACF_OK;

	try
	{
		result = AAXStartup (pUnkHost);
		if (result == ACF_OK)
		{
#if SMTG_OS_MACOS
			bool rc = bundleEntry (GetCurrentBundle ());
#else
			bool rc = InitDll ();
#endif
			if (!rc)
			{
				AAXShutdown (pUnkHost);
				result = ACF_E_UNEXPECTED;
			}
		}
	}
	catch (...)
	{
		result = ACF_E_UNEXPECTED;
	}

	return result;
}

//------------------------------------------------------------------------
// \func ACFShutdown
// \brief Called once at termination of dll.
//
ACFAPI ACFShutdown (IACFUnknown* pUnkHost)
{
	ACFRESULT result = ACF_OK;

	try
	{
#if SMTG_OS_MACOS
		bundleExit ();
#else
		ExitDll ();
#endif
		result = AAXShutdown (pUnkHost);
	}
	catch (...)
	{
		result = ACF_E_UNEXPECTED;
	}

	return result;
}

//------------------------------------------------------------------------
ACFAPI ACFGetSDKVersion (acfUInt64* oSDKVersion)
{
	return AAXGetSDKVersion (oSDKVersion);
}
