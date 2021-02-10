//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/aaxwrapper/aaxwrapper_parameters.h
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

#pragma once

#include "public.sdk/source/vst/basewrapper/basewrapper.h"

#include "AAX_CEffectParameters.h"
#include "AAX_Push8ByteStructAlignment.h"

class AAXWrapper;
struct AAX_Plugin_Desc;

//------------------------------------------------------------------------
// helper to convert to/from AAX/Vst IDs
struct AAX_CID
{
	char str[10];
	AAX_CID () {}
	AAX_CID (Steinberg::Vst::ParamID id) { set (id); }
	void set (Steinberg::Vst::ParamID id) { sprintf (str, "p%X", id); }
	operator const char* () const { return str; }
};

Steinberg::Vst::ParamID getVstParamID (const char* aaxid);

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wincompatible-ms-struct"
#endif

//------------------------------------------------------------------------
class AAXWrapper_Parameters : public AAX_CEffectParameters
{
public:
	// Constructor
	AAXWrapper_Parameters (int32_t plugIndex);
	~AAXWrapper_Parameters ();

	static AAX_CEffectParameters* AAX_CALLBACK Create ();

	// Overrides from AAX_CEffectParameters
	AAX_Result EffectInit () SMTG_OVERRIDE;
	AAX_Result ResetFieldData (AAX_CFieldIndex index, void* inData,
	                           uint32_t inDataSize) const SMTG_OVERRIDE;
	AAX_Result NotificationReceived (AAX_CTypeID iNotificationType, const void* iNotificationData,
	                                 uint32_t iNotificationDataSize) SMTG_OVERRIDE;

	/* Parameter information */
	AAX_Result GetNumberOfParameters (int32_t* oNumControls) const SMTG_OVERRIDE;
	AAX_Result GetMasterBypassParameter (AAX_IString* oIDString) const SMTG_OVERRIDE;
	AAX_Result GetParameterIsAutomatable (AAX_CParamID iParameterID,
	                                      AAX_CBoolean* oAutomatable) const SMTG_OVERRIDE;
	AAX_Result GetParameterNumberOfSteps (AAX_CParamID iParameterID,
	                                      int32_t* oNumSteps) const SMTG_OVERRIDE;
	AAX_Result GetParameterName (AAX_CParamID iParameterID, AAX_IString* oName) const SMTG_OVERRIDE;
	AAX_Result GetParameterNameOfLength (AAX_CParamID iParameterID, AAX_IString* oName,
	                                     int32_t iNameLength) const SMTG_OVERRIDE;
	AAX_Result GetParameterDefaultNormalizedValue (AAX_CParamID iParameterID,
	                                               double* oValue) const SMTG_OVERRIDE;
	AAX_Result SetParameterDefaultNormalizedValue (AAX_CParamID iParameterID,
	                                               double iValue) SMTG_OVERRIDE;
	AAX_Result GetParameterType (AAX_CParamID iParameterID,
	                             AAX_EParameterType* oParameterType) const SMTG_OVERRIDE;
	AAX_Result GetParameterOrientation (AAX_CParamID iParameterID,
	                                    AAX_EParameterOrientation* oParameterOrientation) const
	    SMTG_OVERRIDE;
	AAX_Result GetParameter (AAX_CParamID iParameterID, AAX_IParameter** oParameter) SMTG_OVERRIDE;
	AAX_Result GetParameterIndex (AAX_CParamID iParameterID,
	                              int32_t* oControlIndex) const SMTG_OVERRIDE;
	AAX_Result GetParameterIDFromIndex (int32_t iControlIndex,
	                                    AAX_IString* oParameterIDString) const SMTG_OVERRIDE;
	AAX_Result GetParameterValueInfo (AAX_CParamID iParameterID, int32_t iSelector,
	                                  int32_t* oValue) const SMTG_OVERRIDE;

	/** Parameter setters and getters */
	AAX_Result GetParameterValueFromString (AAX_CParamID iParameterID, double* oValue,
	                                        const AAX_IString& iValueString) const SMTG_OVERRIDE;
	AAX_Result GetParameterStringFromValue (AAX_CParamID iParameterID, double iValue,
	                                        AAX_IString* oValueString,
	                                        int32_t maxLength) const SMTG_OVERRIDE;
	AAX_Result GetParameterValueString (AAX_CParamID iParameterID, AAX_IString* oValueString,
	                                    int32_t iMaxLength) const SMTG_OVERRIDE;
	AAX_Result GetParameterNormalizedValue (AAX_CParamID iParameterID,
	                                        double* oValuePtr) const SMTG_OVERRIDE;
	AAX_Result SetParameterNormalizedValue (AAX_CParamID iParameterID, double iValue) SMTG_OVERRIDE;
	AAX_Result SetParameterNormalizedRelative (AAX_CParamID iParameterID,
	                                           double iValue) SMTG_OVERRIDE;

	/* Automated parameter helpers */
	AAX_Result TouchParameter (AAX_CParamID iParameterID) SMTG_OVERRIDE;
	AAX_Result ReleaseParameter (AAX_CParamID iParameterID) SMTG_OVERRIDE;
	AAX_Result UpdateParameterTouch (AAX_CParamID iParameterID,
	                                 AAX_CBoolean iTouchState) SMTG_OVERRIDE;

	/* Asynchronous parameter update methods */
	AAX_Result UpdateParameterNormalizedValue (AAX_CParamID iParameterID, double iValue,
	                                           AAX_EUpdateSource iSource) SMTG_OVERRIDE;
	AAX_Result _UpdateParameterNormalizedRelative (AAX_CParamID iParameterID, double iValue);
	AAX_Result _GenerateCoefficients ();

	/* Chunk methods */
	AAX_Result GetNumberOfChunks (int32_t* numChunks) const SMTG_OVERRIDE;
	AAX_Result GetChunkIDFromIndex (int32_t index, AAX_CTypeID* chunkID) const SMTG_OVERRIDE;
	AAX_Result GetChunkSize (AAX_CTypeID chunkID, uint32_t* oSize) const SMTG_OVERRIDE;
	AAX_Result GetChunk (AAX_CTypeID chunkID, AAX_SPlugInChunk* oChunk) const SMTG_OVERRIDE;
	AAX_Result SetChunk (AAX_CTypeID chunkID, const AAX_SPlugInChunk* iChunk) SMTG_OVERRIDE;
	AAX_Result CompareActiveChunk (const AAX_SPlugInChunk* iChunk,
	                               AAX_CBoolean* oIsEqual) const SMTG_OVERRIDE;
	AAX_Result _GetNumberOfChanges (int32_t* oValue) const;

	// Override this method to receive MIDI
	// packets for the described MIDI nodes
	AAX_Result UpdateMIDINodes (AAX_CFieldIndex inFieldIndex,
	                            AAX_CMidiPacket& inPacket) SMTG_OVERRIDE;

	AAXWrapper* getWrapper () { return mWrapper; }
	void setDirty (bool state);

private:
	Steinberg::int32 getParameterInfo (AAX_CParamID aaxId,
	                                   Steinberg::Vst::ParameterInfo& paramInfo) const;

	AAXWrapper* mWrapper = nullptr;
	std::vector<AAX_CID> mParamNames;

	AAX_Plugin_Desc* mPluginDesc = nullptr;
	bool mSimulateBypass = false;
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "AAX_PopStructAlignment.h"
