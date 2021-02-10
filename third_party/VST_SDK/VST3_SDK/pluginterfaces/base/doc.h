//------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : Basic Host Service Interfaces
// Filename    : pluginterfaces/base/doc.h
// Created by  : Steinberg, 01/2004
// Description : doc for doxygen
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------


/** \mainpage VST Module Architecture
********************************************************************
********************************************************************
\section piVstMa Introduction
********************************************************************
\b VST-MA is a component model system which is used in any <a href="http://www.steinberg.net" target=_blank>Steinberg</a> host application 
as the basic layer for Plug-in support as well as for internal application components.\n
It is object-oriented, cross-platform and (almost) compiler-independent. \n
The basics are very much like Microsoft(R) COM, so if you are familiar with
this technology, understanding VST-MA should be quite easy. 

\b VST-MA currently is provided in C++ only. Interfaces in C++ are expressed as pure virtual class
(which is a class with nothing but abstract methods). Unlike COM there is no support for C or other
languages yet - simply because there has been no need for this so far. But all \b VST-MA interfaces
can be transformed into different representations in case this should be inevitable some day. \n
It is currently available for Windows and Mac OS X. 

The C++ files belonging to \b VST-MA are located in the following folders:
- <a href="../../pluginterfaces/base" target=_blank>pluginterfaces/base</a>
- <a href="../../pluginterfaces/gui" target=_blank>pluginterfaces/gui</a>

\b Note: The name 'VST Module Architecture' has only little relation to the 'Virtual Studio Technology' itself. \n
It describes the basic layer for any Plug-in category supported in <a href="http://www.steinberg.net" target=_blank>Steinberg</a> hosts. \b VST-MA has been
existing long before it was used as base for VST 3 indeed. The 'VST'-part of the name was introduced only
for reasons of misleading advertising...
\n
\n
********************************************************************
\section piInterfaces Interfaces
********************************************************************

**********************************
\subsection funknown FUnknown
**********************************
  Steinberg::FUnknown is the basic interface of \b VST-MA. All other interfaces are directly or 
  indirectly derived from it. 
\n
\n
**********************************
\subsection iid IID/CID
**********************************
  Each interface has a unique identifier (IID) of type Steinberg::FUID. It is used to retrieve a new 
  interface from another one (Steinberg::FUnknown::queryInterface). It is important to understand the 
  difference between interface identifier and component identifier.\n
  A component-ID or class-ID (CID) is used to identify a concrete implementation class and is usually passed to a class factory in order 
  to create the accordant component. \n
  So a lot of different classes (with different class identifiers) can implement the same interfaces. 
\n 
\n
**********************************
\subsection direction Direction
**********************************
An interface may have a \b direction, meaning that the interface is expected to be 
  implemented either in the Plug-in or in the host. The nature of an interface is documented like this:
  \n \n
  - <b>[host imp] </b>: the host implements the interface
  - <b>[plug imp] </b>: the Plug-in implements the interface
  .
  \n
  When neither of them is specified, the interface can be used in both ways.
\n
\n
**********************************
\subsection version Versioning and inheritance
**********************************
  Unlike C++ classes, interfaces do not use inheritance to express specializations of objects. 
  Inheritance is used for versioning only. One of the strict rules is, that once an interface has been 
  released, it must never change again. Adding new functionality to an interface requires a new version
  (usually an ordinal number is added to its name in this case).\n
  A new version inherits the old version(s)
  of the interface, so the old and the new methods are combined in one interface. This is why specializations 
  need to be modelled as separate interface! If a specialized interface would inherit from the basic interface
  as well, an implementation class that needs to implement all these interfaces would inherit the base 
  interface twice and the compiler will start to complain about ambiguities. So the specialization relation 
  to a basic interface can only be expressed in the documentation: \n \n
  - ISpecialInterface [\b extends IBaseInterface] => means IBaseInterface::queryInterface (ISpecialInterface::iid, ...) can be used to retrieve the derived 
	interface.
  . 
  \n
  You can find some example code here: \ref versionInheritance 
\n
\n
**********************************
\subsection com COM Compatibility
**********************************
  The first layer of \b VST-MA is binary compatible to \b COM. The Vtable and Interface Identifier of 
  FUnknown match with the corresponding COM interface IUnknown. The main difference is the organization 
  and creation of components by a host application. \b VST-MA does not require any Microsoft(R) COM source file. 
  You can find information about \b COM on pages like: \n
	- \htmlonly <a href="http://www.microsoft.com/Com/resources/comdocs.asp" target="_blank">http://www.microsoft.com/Com/resources/comdocs.asp</a>\endhtmlonly.
\n
\n
**********************************
\subsection basic Basic Interfaces
**********************************
  - Steinberg::FUnknown
  - Steinberg::IPluginBase 
  - Steinberg::IPluginFactory 
\n
\n
**********************************
\subsection helper Helper Classes
**********************************
  - Steinberg::FUID
  - Steinberg::FUnknownPtr
.
\n
\n
\see \ref howtoClass
\n
\n
********************************************************************
\section piPlugins Plug-ins
********************************************************************

**********************************
\subsection module Module Factory:
**********************************
  A module (Windows: Dynamic Link Library, MAC: Mach-O Bundle) contains the implementation 
  of one or more components (e.g. VST Effects). A \b VST-MA module must contain a class factory
  where meta-data and create-methods for the components are registered. \n
  The host has access to this factory through the Steinberg::IPluginFactory interface. This is the anchor point to the module
  and it is realized as C-style export function named GetPluginFactory. You can find an export 
  definition file in folder - <a href="../../public.sdk/win/stdplug.def" target=_blank>public.sdk/win/stdplug.def</a> 
  which can be used to export this function. \n
  GetPluginFactory is declared as follows:

\code
IPluginFactory* PLUGIN_API GetPluginFactory ()
\endcode
\n
\n
**********************************
\subsection Locations
**********************************
  Component modules don't require registration like DirectX. The host application expects component modules to be located in predefined folders of the file system.
  These folders and their subfolders are scanned at application startup for \b VST-MA modules. Each folder serves a special purpose:
	- The application's \c Components subfolder (e.g. "C:\Program Files\Steinberg\Cubase SX\Components") is used for components tightly bound to the application. No other application should use it.
	- Components that are shared between all <a href="http://www.steinberg.net" target=_blank>Steinberg</a> hosts are located at:
		- Win: "/Program Files/Common Files/Steinberg/shared components"
		- Mac: "/Library/Application Support/Steinberg/Components/"
	- For special purpose Plug-in types, additional locations can be defined. Please refer to the corresponding documentation to find out if additional folders are used and where they are.
\n
\n
**********************************
\subsection Categories
**********************************
  Any class that the factory can create is assigned to a category. It is this category that tells 
  the host the purpose of the class (and gives a hint of wich interfaces it might implement). \n
  A class is also described with a name and it has a unique id.
  - The category for import/export filters is <b>"Project Filter"</b> and for VST 3 Audio Plug-ins <b>"Audio Module Class"</b> for example. 
  - A special category is "Service". The purpose of a class of this category is completely 
    unknown to the host. It will be loaded automatically at the program start 
	(if the user did not deactivate it).
  - Since the factory can create any number of classes, one component library can contain 
    multiple components of any type.
\n
\n
**********************************
\subsection IPluginBase
**********************************
  The entry-point interface for any component class is Steinberg::IPluginBase. The host uses this 
  interface to initialize and to terminate the Plug-in component. When the host initializes the 
  Plug-in, it passes a so called context. This context contains any interface to the host that 
  the Plug-in will need to work.
\n
\n
**********************************
\subsection purpose Purpose-specific interfaces
**********************************
Each Plug-in category (VST 3 Effects, Project import/export Filters, Audio Codecs, etc...) defines its own set of purpose-specific interfaces. These are not part of the basic VST-MA layer.
\n
\n
\see \ref loadPlugin
\n
\n
********************************************************************
\section Unicode Unicode
********************************************************************

Beginning with version 5 of Cubase and Nuendo, the internal structure of the host was modified to
better support internationalization. Therefore the string handling was changed to utilize
<A HREF="http://en.wikipedia.org/wiki/Unicode">Unicode</A> strings whenever strings are passed
around. Consequently all the interfaces to Plug-ins have changed from using ASCI to Unicode strings
as call and return parameters as well. So in turn also any Plug-in has to be adapted to support
Unicode. This has major consequences in that:
- Unicode hosts (Cubase5 or later) will only work with Unicode Plug-ins. While loading a Plug-in
a Unicode host will check the Plug-in's type and refuse to load any Plug-in that is non-Unicode.
\n
- Unicode Plug-ins will <b>not</b> load into non-Unicode hosts. While loading a Unicode Plug-in
will request information from the host and refuse to load itself, if no Unicode host is detected.
Therefore, if a Plug-in is supposed to work on both older and newer hosts, it is best to provide
two versions of the Plug-in. 
\n
\n
**********************************
\subsection plugunicode Plug-ins for only Unicode hosts
**********************************
Writing Plug-ins that are supposed to work only on Unicode hosts is easy. Use a current version
of this SDK and develop a Plug-in as usual. Make sure that you only ever pass Unicode 
<a href="http://en.wikipedia.org/wiki/UTF-16">UTF16</a> strings to interfaces that
have strings as call parameters and also be prepared that interface return strings are
always <a href="http://en.wikipedia.org/wiki/UTF-16">UTF16</a>. Therefore, to make things easier, it is recommended that throughout the
Plug-in's implementation Unicode strings are used, in order to avoid back and forth conversions.
To make things even more easy, use the Steinberg::String and Steinberg::ConstString classes from
the \ref Base module, they have been designed to work universially on both, Mac and Win. 
\n
\n
**********************************
\subsection migrating Migrating from non-Unicode to Unicode
**********************************
In <a href="http://www.steinberg.net" target="_blank">Steinberg</a> SDKs released before Cubase5 the interface functions were using pointers of type <c> char </c>
for passing strings to and from the host. These have been changed now to using Steinberg's defined
type <c> tchar </c> which is equivalent to <c> char16 </c>, i.e. 16 bit character. There are
theoretically many ways of how characters could be represented in 16 bits, but we chose
to use the industry standard <a href="http://en.wikipedia.org/wiki/Unicode">Unicode</a>, so strings
are expected to be encoded in <a href="http://en.wikipedia.org/wiki/UTF-16">UTF16</a>. \n
Accordingly also the implementation of a Plug-in needs to be adapted to deal correctly with Unicode
encoded strings, as well as only ever passing Unicode strings to the host. 
\n
\n
<b>Technical note</b>: Changing a function from using 8 bit to 16 bit character pointers seems to be only a
minor modification, but in interface design this is a major intrusion, because an interface is
a contract to the outer world that is never to be changed. Therefore, classes that were changed
to use Unicode strings are distinguished and also received a new unique class id.
\n
\n
**********************************
\subsection backward SDK backward compatibility
**********************************
Even with the current SDK it is still possible to develop non-Unicode Plug-ins. In the
file <a href="../../pluginterfaces/base/ftypes.h">pluginterfaces/base/ftypes.h</a> the line "#define UNICODE_OFF"
is commented out, but if it gets uncommented, then all interfaces will revert back to using single byte ASCI strings.
Alternatively you could also specify UNICODE_OFF as preprocessor definition in your project file.\n
Also the Plug-in's factory info will then not define the Unicode flag anymore, so a Unicode host will see the compiled
Plug-in as non-Unicode. As a matter of course, when reverting back to single byte strings the Plug-in's implementation also
has to be changed to behave correctly. 
\n
\n
<b>Technical note</b>: When undefining Unicode also the class ids will revert back to the old ones.
\n
\n
*/

//******************************************************
/**
\page howtoClass How to derive a class from an interface
*********************************************************
In the first example we derive a class directly from FUnknown, using some of the helper macros provided by the SDK. 

\code
class CMyClass: public FUnknown
{
public:
	CMyClass ();
	virtual ~CMyClass ();

	DECLARE_FUNKNOWN_METHODS  // declares queryInterface, addRef and release
};


CMyClass::CMyClass ()
{
	FUNKNOWN_CTOR // init reference counter, increment global object counter
}

CMyClass::~CMyClass ()
{
	FUNKNOWN_DTOR // decrement global object counter
}

IMPLEMENT_REFCOUNT (CMyClass) // implements reference counting

tresult CMyClass::queryInterface (const char* iid, void** obj)
{
	QUERY_INTERFACE (iid, obj, ::FUnknown::iid, CMyClass)
	return kNoInterface;
}
\endcode

Developing a class with more than one interface is done by multiple inheritance. Additionally you have to provide an appropriate cast for each interface in the queryInterface method. 

\code
class CMyMultiClass:	public Steinberg::IPluginBase,
			public Steinberg::IPlugController,
			public Steinberg::IEditorFactory
{
public:
	DECLARE_FUNKNOWN_METHODS

	// declare the methods of all inherited interfaces here...
};

IMPLEMENT_REFCOUNT (CMyMultiClass) // implements reference counting

tresult CMyMultiClass::queryInterface (const char* iid, void** obj)
{
	QUERY_INTERFACE (iid, obj, Steinberg::FUnknown::iid, IPluginBase)
	QUERY_INTERFACE (iid, obj, Steinberg::IPluginBase::iid, IPluginBase)
	QUERY_INTERFACE (iid, obj, Steinberg::IPlugController::iid, IPlugController)
	QUERY_INTERFACE (iid, obj, Steinberg::IEditorFactory::iid, IEditorFactory)
	*obj = 0;
	return kNoInterface;
}

\endcode
*/

//********************************************************************
/** \page versionInheritance Interface Versions and Inheritance
********************************************************************
\par

Unlike C++ classes, \b VST-MA interfaces do not use inheritance to express
specializations of objects. Usually all interfaces are derived from
FUnknown. This is because interfaces must \b never change after they
have been released. The VST Module Architecture Interfaces use inheritance
only for versioning! All specializations will be modelled as
separate interfaces!

For example the C++ classes
\code
class Shape
{
public:
	void setPosition (long x, long y);
protected:
	long x;
	long y;
};
class Rect : public Shape
{
public:
	void setDimension (long width, long height);
protected:
	long width;
	long height;
};
\endcode

expressed in \b VST-MA, define an interface for each inheritance level:
\code



class IShape : public FUnknown
{
public:
	virtual void setPosition (long x, long y) = 0;
};
class IRect : public FUnknown
{
public:
	virtual void setDimension (long width, long height) = 0;
};
\endcode

In the next program version there need to be changes to 
the \c Shape class that look like this:
\code
class Shape
{
public:
	void setPosition (long x, long y);
	void setColor (Color color);
protected:
	long x;
	long y;
	Color color;
};
\endcode

The \b VST-MA representation now reflect the changes to Shape by adding a
new interface that inherits from IShape and looks like the following
code, while the former interface definitions remain the same:
\code
class IShape2 : public IShape
{
public:
	virtual void setColor (Color color) = 0;
};
\endcode
*/

//********************************************************************
/** \page loadPlugin How the host will load a Plug-in
********************************************************************
\par

The host application will handle a Plug-in in the following manner 
(some code is Windows-specific!): 

\code
HMODULE hModule = LoadLibrary ("SomePlugin.dll");
if (hModule)
{
	InitModuleProc initProc = (InitModuleProc)GetProcAddress (hModule, "InitDll");
	if (initProc)
	{
		if (initProc () == false)
		{
			FreeLibrary (module);
			return false;
		}
	}

	GetFactoryProc proc = (GetFactoryProc)GetProcAddress (hModule, "GetPluginFactory");

	IPluginFactory* factory = proc ? proc () : 0;
	if (factory)
	{
		for (int32 i = 0; i < factory->countClasses (); i++)
		{
			PClassInfo ci;
			factory->getClassInfo (i, &ci);

			FUnknown* obj;
			factory->createInstance (ci.cid, FUnknown::iid, (void**)&obj);
			...
			obj->release ();
		}

		factory->release ();
	}

	ExitModuleProc exitProc = (ExitModuleProc)GetProcAddress (hModule, "ExitDll");
	if (exitProc)
		exitProc ();

	FreeLibrary (hModule);
}
\endcode
*/
