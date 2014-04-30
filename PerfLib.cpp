#include "PerfLib.h"
#include "GPUPerfAPI.h"

#if AMD_PERF_API_LINUX
	#include <dlfcn.h>	//dyopen, dlsym, dlclose
	#include <stdlib.h>
	#include <string.h>	//memeset
	#include <unistd.h>	//sleep
#elif AMD_PERF_API_WINDOWS
	#include <windows.h>
	#include <tchar.h>
#endif

#include <stdio.h>
#include <cstdlib>
#include <vector>

#include <iostream>
#include <stdexcept>

#define NIV_SAFE_GPA(expr) do { const auto r = (expr); if (r != GPA_STATUS_OK) throw std::runtime_error ("GPA failure"); } while (0)

namespace Amd {
namespace {
template <typename T>
T function_pointer_cast (const void* p)
{
	static_assert (sizeof (T) == sizeof (void*), "Size of a plain function pointer "
		"must be equal to size of a plain pointer.");
	T result;
	::memcpy (&result, &p, sizeof (void*));
	return result;
}

struct LogCallback
{
	GPA_LoggingCallbackPtrType	;
	std::function<void (int, const char*)>	callback;
};
}

struct PerformanceLibrary::Impl
{
	Impl (const ProfileApi::Enum api)
	{
#if AMD_PERF_API_LINUX
#elif AMD_PERF_API_WINDOWS
		switch (api) {
		case ProfileApi::Direct3D10: lib_	= LoadLibraryA ("GPUPerfAPICL-x64.dll"); break;
		case ProfileApi::Direct3D11: lib_	= LoadLibraryA ("GPUPerfAPIDX11-x64.dll"); break;
		case ProfileApi::OpenGL: lib_		= LoadLibraryA ("GPUPerfAPIGL-x64.dll"); break;
		case ProfileApi::OpenCL: lib_		= LoadLibraryA ("GPUPerfAPICL-x64.dll"); break;
		}
#endif

		InitFunctions ();

		initialize_ ();
	}

	~Impl ()
	{
		destroy_ ();

#if AMD_PERF_API_LINUX
		if (lib_ != nullptr) {
			dlclose (lib_);
		}
#elif AMD_PERF_API_WINDOWS
		if (lib_ != NULL) {
			FreeLibrary (lib_);
		}
#endif
	}

private:
	void InitFunctions ()
	{
		initialize_ 			= function_pointer_cast<GPA_InitializePtrType> (LoadFunction ("GPA_Initialize"));
		destroy_ 				= function_pointer_cast<GPA_DestroyPtrType> (LoadFunction ("GPA_Destroy"));
		openContext_ 			= function_pointer_cast<GPA_OpenContextPtrType> (LoadFunction ("GPA_OpenContext"));
		closeContext_ 			= function_pointer_cast<GPA_CloseContextPtrType> (LoadFunction ("GPA_CloseContext"));
		getNumCounters_ 		= function_pointer_cast<GPA_GetNumCountersPtrType> (LoadFunction ("GPA_GetNumCounters"));
		getCounterName_ 		= function_pointer_cast<GPA_GetCounterNamePtrType> (LoadFunction ("GPA_GetCounterName"));
		getCounterDataType_ 	= function_pointer_cast<GPA_GetCounterDataTypePtrType> (LoadFunction ("GPA_GetCounterDataType"));
		enableCounter_ 			= function_pointer_cast<GPA_EnableCounterPtrType> (LoadFunction ("GPA_EnableCounter"));
		disableCounter_ 		= function_pointer_cast<GPA_DisableCounterPtrType> (LoadFunction ("GPA_DisableCounter"));
		getPassCount_ 			= function_pointer_cast<GPA_GetPassCountPtrType> (LoadFunction ("GPA_GetPassCount"));
		beginSession_ 			= function_pointer_cast<GPA_BeginSessionPtrType> (LoadFunction ("GPA_BeginSession"));
		endSession_ 			= function_pointer_cast<GPA_EndSessionPtrType> (LoadFunction ("GPA_EndSession"));
		beginPass_ 				= function_pointer_cast<GPA_BeginPassPtrType> (LoadFunction ("GPA_BeginPass"));
		endPass_ 				= function_pointer_cast<GPA_EndPassPtrType> (LoadFunction ("GPA_EndPass"));
		beginSample_ 			= function_pointer_cast<GPA_BeginSamplePtrType> (LoadFunction ("GPA_BeginSample"));
		endSample_ 				= function_pointer_cast<GPA_EndSamplePtrType> (LoadFunction ("GPA_EndSample"));
		isSessionReady_ 		= function_pointer_cast<GPA_IsSessionReadyPtrType> (LoadFunction ("GPA_IsSessionReady"));
		getSampleUInt64_ 		= function_pointer_cast<GPA_GetSampleUInt64PtrType> (LoadFunction ("GPA_GetSampleUInt64"));
		getSampleUInt32_ 		= function_pointer_cast<GPA_GetSampleUInt32PtrType> (LoadFunction ("GPA_GetSampleUInt32"));
		getSampleFloat32_ 		= function_pointer_cast<GPA_GetSampleFloat32PtrType> (LoadFunction ("GPA_GetSampleFloat32"));
		getSampleFloat64_ 		= function_pointer_cast<GPA_GetSampleFloat64PtrType> (LoadFunction ("GPA_GetSampleFloat64"));
		getEnabledCount_ 		= function_pointer_cast<GPA_GetEnabledCountPtrType> (LoadFunction ("GPA_GetEnabledCount"));
		getEnabledIndex_ 		= function_pointer_cast<GPA_GetEnabledIndexPtrType> (LoadFunction ("GPA_GetEnabledIndex"));
		registerLoggingCallback_= function_pointer_cast<GPA_RegisterLoggingCallbackPtrType> (LoadFunction ("GPA_RegisterLoggingCallback"));
	}

	void* LoadFunction (const char* name) const
	{
#if AMD_PERF_API_LINUX
		return dlsym (lib_, name);
#elif AMD_PERF_API_WINDOWS
		return GetProcAddress (lib_, name);
#else
		return nullptr;
#endif
	}

public:
	std::vector<CounterInfo>	GetAvailableCounters () const
	{
		std::vector<CounterInfo> result;

		gpa_uint32 availableCounters = 0;

		NIV_SAFE_GPA (getNumCounters_ (&availableCounters));
		result.reserve (availableCounters);

		for (gpa_uint32 i = 0; i < availableCounters; ++i) {
			CounterInfo ci;

			const char* name = nullptr;
			NIV_SAFE_GPA (getCounterName_ (i, &name));

			ci.name = name;

			GPA_Type dataType;

			NIV_SAFE_GPA (getCounterDataType_ (i, &dataType));

			switch (dataType) {
				case GPA_TYPE_UINT32: ci.type = DataType::uint32; break;
				case GPA_TYPE_UINT64: ci.type = DataType::uint64; break;
				case GPA_TYPE_FLOAT32: ci.type = DataType::float32; break;
				case GPA_TYPE_FLOAT64: ci.type = DataType::float64; break;

				default:
					throw std::runtime_error ("Unhandled data type.");
			}

			ci.index = i;

			result.push_back (ci);
		}

		return result;
	}

	void EnableCounters (const std::vector<CounterInfo>& counters)
	{
		for (const auto& counter : counters) {
			NIV_SAFE_GPA (enableCounter_ (counter.index));
		}
	}

	void BeginPass ()
	{
		NIV_SAFE_GPA (beginPass_ ());
	}

	void EndPass ()
	{
		NIV_SAFE_GPA (endPass_ ());
	}

	ProfileSessionHandle BeginProfileSession ()
	{
		gpa_uint32 sessionId = 0;
		NIV_SAFE_GPA (beginSession_ (&sessionId));

		return sessionId;
	}

	void EndProfileSession ()
	{
		NIV_SAFE_GPA (endSession_ ());
	}

	bool GetProfileSessionResult (const ProfileSessionHandle handle,
		ProfileResult& result, const bool block) const
	{
		bool ready = false;
		while (! ready) {
			NIV_SAFE_GPA (isSessionReady_ (&ready, handle));

			if (! block) {
				continue;
			}
		}

		if (! ready) {
			return false;
		}

		gpa_uint32 enabledCounterCount = 0;
		NIV_SAFE_GPA (getEnabledCount_ (&enabledCounterCount));

		for (gpa_uint32 i = 0; i < enabledCounterCount; ++i) {
			gpa_uint32 index = 0;

			NIV_SAFE_GPA (getEnabledIndex_ (i, &index));
			const char* name = nullptr;

			NIV_SAFE_GPA (getCounterName_ (index, &name));

			GPA_Type type;
			NIV_SAFE_GPA (getCounterDataType_ (index, &type));

			switch (type) {
				case GPA_TYPE_INT32:
				{
					gpa_uint32 value;
					NIV_SAFE_GPA (getSampleUInt32_ (handle, 0, index, &value));
					result [name] = static_cast<std::int32_t> (value);
					break;
				}
				case GPA_TYPE_INT64:
				{
					gpa_uint64 value;
					NIV_SAFE_GPA (getSampleUInt64_ (handle, 0, index, &value));
					result [name] = static_cast<std::int32_t> (value);
					break;
				}
				case GPA_TYPE_UINT32:
				{
					gpa_uint32 value;
					NIV_SAFE_GPA (getSampleUInt32_ (handle, 0, index, &value));
					result [name] = value;
					break;
				}
				case GPA_TYPE_UINT64:
				{
					gpa_uint64 value;
					NIV_SAFE_GPA (getSampleUInt64_ (handle, 0, index, &value));
					result [name] = value;
					break;
				}
				case GPA_TYPE_FLOAT32:
				{
					gpa_float32 value;
					NIV_SAFE_GPA (getSampleFloat32_ (handle, 0, index, &value));
					result [name] = value;
					break;
				}
				case GPA_TYPE_FLOAT64:
				{
					gpa_float64 value;
					NIV_SAFE_GPA (getSampleFloat64_ (handle, 0, index, &value));
					result [name] = value;
					break;
				}

				default:
					throw std::runtime_error ("Unsupported data type.");
			}
		}

		return true;
	}

	void BeginSampling ()
	{
		NIV_SAFE_GPA (beginSample_ (0));
	}

	void EndSampling ()
	{
		NIV_SAFE_GPA (endSample_ ());
	}

	void OpenContext (void* ctx)
	{
		NIV_SAFE_GPA (openContext_ (ctx));
	}

	void CloseContext ()
	{
		NIV_SAFE_GPA (closeContext_ ());
	}

	std::uint32_t GetRequiredPassCount () const
	{
		gpa_uint32 numPasses;
		NIV_SAFE_GPA (getPassCount_ (&numPasses));

		return numPasses;
	}

private:
#if AMD_PERF_API_LINUX
	void*	lib_;
#elif AMD_PERF_API_WINDOWS
	HINSTANCE lib_;
#endif

	GPA_InitializePtrType 			initialize_;
	GPA_DestroyPtrType 				destroy_;

	GPA_OpenContextPtrType			openContext_;
	GPA_CloseContextPtrType			closeContext_;

	GPA_GetNumCountersPtrType		getNumCounters_;
	GPA_GetCounterNamePtrType		getCounterName_;
	GPA_GetCounterDataTypePtrType	getCounterDataType_;

	GPA_EnableCounterPtrType		enableCounter_;
	GPA_DisableCounterPtrType		disableCounter_;

	GPA_GetPassCountPtrType 		getPassCount_;

	GPA_BeginSessionPtrType 		beginSession_;
	GPA_EndSessionPtrType 			endSession_;

	GPA_BeginPassPtrType 			beginPass_;
	GPA_EndPassPtrType 				endPass_;

	GPA_BeginSamplePtrType 			beginSample_;
	GPA_EndSamplePtrType 			endSample_;

	GPA_GetEnabledCountPtrType		getEnabledCount_;
	GPA_GetEnabledIndexPtrType		getEnabledIndex_;

	GPA_IsSessionReadyPtrType 		isSessionReady_;
	GPA_GetSampleUInt64PtrType 		getSampleUInt64_;
	GPA_GetSampleUInt32PtrType 		getSampleUInt32_;
	GPA_GetSampleFloat32PtrType 	getSampleFloat32_;
	GPA_GetSampleFloat64PtrType 	getSampleFloat64_;

	GPA_RegisterLoggingCallbackPtrType	registerLoggingCallback_;
};

////////////////////////////////////////////////////////////////////////////////
PerformanceLibrary::PerformanceLibrary (const ProfileApi::Enum targetApi)
: impl_ (new Impl (targetApi))
{
}

////////////////////////////////////////////////////////////////////////////////
PerformanceLibrary::~PerformanceLibrary ()
{
	delete impl_;
}

////////////////////////////////////////////////////////////////////////////////
int PerformanceLibrary::GetRequiredPassCount ()
{
	return static_cast<int> (impl_->GetRequiredPassCount ());
}

////////////////////////////////////////////////////////////////////////////////
std::vector<CounterInfo>	PerformanceLibrary::GetAvailableCounters () const
{
	return impl_->GetAvailableCounters ();
}

////////////////////////////////////////////////////////////////////////////////
void PerformanceLibrary::EnableCounters (const std::vector<CounterInfo>& counters)
{
	impl_->EnableCounters (counters);
}

////////////////////////////////////////////////////////////////////////////////
void PerformanceLibrary::BeginPass ()
{
	impl_->BeginPass ();
}

////////////////////////////////////////////////////////////////////////////////
void PerformanceLibrary::EndPass ()
{
	impl_->EndPass ();
}

////////////////////////////////////////////////////////////////////////////////
PerformanceLibrary::ProfileSessionHandle PerformanceLibrary::BeginProfileSession ()
{
	return impl_->BeginProfileSession ();
}

////////////////////////////////////////////////////////////////////////////////
void PerformanceLibrary::EndProfileSession ()
{
	impl_->EndProfileSession ();
}

////////////////////////////////////////////////////////////////////////////////
bool PerformanceLibrary::GetProfileSessionResult (const ProfileSessionHandle handle,
	ProfileResult& result, const bool block) const
{
	return impl_->GetProfileSessionResult (handle, result, block);
}

////////////////////////////////////////////////////////////////////////////////
void PerformanceLibrary::BeginSampling ()
{
	impl_->BeginSampling ();
}

////////////////////////////////////////////////////////////////////////////////
void PerformanceLibrary::EndSampling ()
{
	impl_->EndSampling ();
}

////////////////////////////////////////////////////////////////////////////////
void PerformanceLibrary::OpenContext (void* ctx)
{
	impl_->OpenContext (ctx);
}

////////////////////////////////////////////////////////////////////////////////
void PerformanceLibrary::CloseContext ()
{
	impl_->CloseContext ();
}
}
