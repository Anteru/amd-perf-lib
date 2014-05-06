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
#include <set>

#include <iostream>
#include <stdexcept>

#define NIV_SAFE_GPA(expr) do { const auto r = (expr); if (r != GPA_STATUS_OK) throw Exception (r); } while (0)

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

#if AMD_PERF_API_LINUX
	typedef void*	LibraryHandle;
#elif AMD_PERF_API_WINDOWS
	typedef HINSTANCE LibraryHandle;
#endif

void* LoadFunction (LibraryHandle lib, const char* name)
{
	void* result = nullptr;
#if AMD_PERF_API_LINUX
	result = dlsym (lib, name);
#elif AMD_PERF_API_WINDOWS
	result = GetProcAddress (lib, name);
#else
#error "Unsupported platform"
#endif

	if (result == nullptr) {
		throw std::runtime_error (std::string ("Could not load function: ") + name);
	}

	return result;
}
}

/////////////////////////////////////////////////////////////////////////////
Exception::Exception (const int error)
: std::runtime_error ("GPA error: " + std::to_string (error))
, errorCode_ (error)
{
}

/////////////////////////////////////////////////////////////////////////////
int Exception::GetErrorCode () const
{
	return errorCode_;
}

namespace Internal {
struct ImportTable
{
	static void LoadFunctions (LibraryHandle lib, ImportTable& table)
	{
		table.initialize 			= function_pointer_cast<GPA_InitializePtrType> (LoadFunction (lib, "GPA_Initialize"));
		table.destroy 				= function_pointer_cast<GPA_DestroyPtrType> (LoadFunction (lib, "GPA_Destroy"));
		table.openContext 			= function_pointer_cast<GPA_OpenContextPtrType> (LoadFunction (lib, "GPA_OpenContext"));
		table.selectContext			= function_pointer_cast<GPA_SelectContextPtrType>(LoadFunction (lib, "GPA_SelectContext"));
		table.closeContext 			= function_pointer_cast<GPA_CloseContextPtrType> (LoadFunction (lib, "GPA_CloseContext"));
		table.getNumCounters 		= function_pointer_cast<GPA_GetNumCountersPtrType> (LoadFunction (lib, "GPA_GetNumCounters"));
		table.getCounterName 		= function_pointer_cast<GPA_GetCounterNamePtrType> (LoadFunction (lib, "GPA_GetCounterName"));
		table.getCounterDataType 	= function_pointer_cast<GPA_GetCounterDataTypePtrType> (LoadFunction (lib, "GPA_GetCounterDataType"));
		table.getCounterUsageType	= function_pointer_cast<GPA_GetCounterUsageTypePtrType> (LoadFunction (lib, "GPA_GetCounterUsageType"));
		table.enableCounter 		= function_pointer_cast<GPA_EnableCounterPtrType> (LoadFunction (lib, "GPA_EnableCounter"));
		table.disableCounter 		= function_pointer_cast<GPA_DisableCounterPtrType> (LoadFunction (lib, "GPA_DisableCounter"));
		table.getPassCount 			= function_pointer_cast<GPA_GetPassCountPtrType> (LoadFunction (lib, "GPA_GetPassCount"));
		table.beginSession 			= function_pointer_cast<GPA_BeginSessionPtrType> (LoadFunction (lib, "GPA_BeginSession"));
		table.endSession 			= function_pointer_cast<GPA_EndSessionPtrType> (LoadFunction (lib, "GPA_EndSession"));
		table.beginPass 			= function_pointer_cast<GPA_BeginPassPtrType> (LoadFunction (lib, "GPA_BeginPass"));
		table.endPass 				= function_pointer_cast<GPA_EndPassPtrType> (LoadFunction (lib, "GPA_EndPass"));
		table.beginSample 			= function_pointer_cast<GPA_BeginSamplePtrType> (LoadFunction (lib, "GPA_BeginSample"));
		table.endSample 			= function_pointer_cast<GPA_EndSamplePtrType> (LoadFunction (lib, "GPA_EndSample"));
		table.isSessionReady 		= function_pointer_cast<GPA_IsSessionReadyPtrType> (LoadFunction (lib, "GPA_IsSessionReady"));
		table.getSampleUInt64 		= function_pointer_cast<GPA_GetSampleUInt64PtrType> (LoadFunction (lib, "GPA_GetSampleUInt64"));
		table.getSampleUInt32 		= function_pointer_cast<GPA_GetSampleUInt32PtrType> (LoadFunction (lib, "GPA_GetSampleUInt32"));
		table.getSampleFloat32 		= function_pointer_cast<GPA_GetSampleFloat32PtrType> (LoadFunction (lib, "GPA_GetSampleFloat32"));
		table.getSampleFloat64 		= function_pointer_cast<GPA_GetSampleFloat64PtrType> (LoadFunction (lib, "GPA_GetSampleFloat64"));
		table.getEnabledCount 		= function_pointer_cast<GPA_GetEnabledCountPtrType> (LoadFunction (lib, "GPA_GetEnabledCount"));
		table.getEnabledIndex 		= function_pointer_cast<GPA_GetEnabledIndexPtrType> (LoadFunction (lib, "GPA_GetEnabledIndex"));
	}

	GPA_InitializePtrType 			initialize;
	GPA_DestroyPtrType 				destroy;

	GPA_OpenContextPtrType			openContext;
	GPA_SelectContextPtrType		selectContext;
	GPA_CloseContextPtrType			closeContext;

	GPA_GetNumCountersPtrType		getNumCounters;
	GPA_GetCounterNamePtrType		getCounterName;
	GPA_GetCounterDataTypePtrType	getCounterDataType;
	GPA_GetCounterUsageTypePtrType  getCounterUsageType;

	GPA_EnableCounterPtrType		enableCounter;
	GPA_DisableCounterPtrType		disableCounter;

	GPA_GetPassCountPtrType 		getPassCount;

	GPA_BeginSessionPtrType 		beginSession;
	GPA_EndSessionPtrType 			endSession;

	GPA_BeginPassPtrType 			beginPass;
	GPA_EndPassPtrType 				endPass;

	GPA_BeginSamplePtrType 			beginSample;
	GPA_EndSamplePtrType 			endSample;

	GPA_GetEnabledCountPtrType		getEnabledCount;
	GPA_GetEnabledIndexPtrType		getEnabledIndex;

	GPA_IsSessionReadyPtrType 		isSessionReady;
	GPA_GetSampleUInt64PtrType 		getSampleUInt64;
	GPA_GetSampleUInt32PtrType 		getSampleUInt32;
	GPA_GetSampleFloat32PtrType 	getSampleFloat32;
	GPA_GetSampleFloat64PtrType 	getSampleFloat64;
};
}

struct PerformanceLibrary::Impl
{
	Impl (const ProfileApi::Enum api)
	: lib_ (nullptr)
	{
#if AMD_PERF_API_LINUX
		switch (api) {
		case ProfileApi::OpenCL: lib_ = dlopen ("libGPUPerfAPICL.so", RTLD_NOW); break;
		case ProfileApi::OpenGL: lib_ = dlopen ("libGPUPerfAPIGL.so", RTLD_NOW); break;
		default:
			throw std::runtime_error ("Unsupported API");
		}

#elif AMD_PERF_API_WINDOWS
		switch (api) {
		case ProfileApi::Direct3D10: lib_ = LoadLibraryA ("GPUPerfAPICL-x64.dll"); break;
		case ProfileApi::Direct3D11: lib_ = LoadLibraryA ("GPUPerfAPIDX11-x64.dll"); break;
		case ProfileApi::OpenGL:	 lib_ = LoadLibraryA ("GPUPerfAPIGL-x64.dll"); break;
		case ProfileApi::OpenCL:	 lib_ = LoadLibraryA ("GPUPerfAPICL-x64.dll"); break;
		}
#else
	#error "Unsupported platform"
#endif

		if (lib_ == nullptr) {
			throw std::runtime_error ("Failed to initialize performance API library.");
		}
		
		::memset (&imports_, 0, sizeof (imports_));

		// Get the import functions
		Internal::ImportTable::LoadFunctions (lib_, imports_);
		
		// Initialize the API
		NIV_SAFE_GPA (imports_.initialize ());
	}

	~Impl ()
	{
		NIV_SAFE_GPA (imports_.destroy ());

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
	
	Context OpenContext (void* ctx)
	{
		return Context (&imports_, ctx);
	}

private:
	Internal::ImportTable	imports_;
	LibraryHandle			lib_;
};

////////////////////////////////////////////////////////////////////////////////
CounterSet::CounterSet (Internal::ImportTable* importTable, 
	const CounterSet::CounterMap& counters)
: imports_ (importTable)
, counters_ (counters)
{

}

////////////////////////////////////////////////////////////////////////////////
CounterSet::CounterSet ()
: imports_ (nullptr)
{

}

////////////////////////////////////////////////////////////////////////////////
CounterSet::CounterMap::iterator CounterSet::begin ()
{
	return counters_.begin ();
}

////////////////////////////////////////////////////////////////////////////////
CounterSet::CounterMap::iterator CounterSet::end ()
{
	return counters_.end ();
}

////////////////////////////////////////////////////////////////////////////////
CounterSet::CounterMap::const_iterator CounterSet::begin () const
{
	return counters_.begin ();
}

////////////////////////////////////////////////////////////////////////////////
CounterSet::CounterMap::const_iterator CounterSet::end () const
{
	return counters_.end ();
}

////////////////////////////////////////////////////////////////////////////////
CounterSet::CounterMap::const_iterator CounterSet::cbegin () const
{
	return counters_.cbegin ();
}

////////////////////////////////////////////////////////////////////////////////
CounterSet::CounterMap::const_iterator CounterSet::cend () const
{
	return counters_.cend ();
}

////////////////////////////////////////////////////////////////////////////////
Counter& CounterSet::operator [] (const std::string& name)
{
	return counters_ [name];
}

////////////////////////////////////////////////////////////////////////////////
int CounterSet::GetRequiredPassCount () const
{
	std::uint32_t passCount = 0;

	NIV_SAFE_GPA (imports_->getPassCount (&passCount));

	return static_cast<int> (passCount);
}

////////////////////////////////////////////////////////////////////////////////
void CounterSet::Enable ()
{
	for (const auto& kv : counters_) {
		NIV_SAFE_GPA (imports_->enableCounter (kv.second.index));
	}
}

////////////////////////////////////////////////////////////////////////////////
void CounterSet::Disable ()
{
	for (const auto& kv : counters_) {
		NIV_SAFE_GPA (imports_->disableCounter (kv.second.index));
	}
}

////////////////////////////////////////////////////////////////////////////////
void CounterSet::Keep (const std::vector<std::string>& counters)
{
	std::set<std::string> counterSet (counters.begin (), counters.end ());
	
	auto it = counters_.begin (), end = counters_.end ();

	while (it != end) {
		if (counterSet.find (it->first) == counterSet.end ()) {
			it = counters_.erase (it);
		} else {
			++it;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
Sample::Sample (Internal::ImportTable* importTable, std::uint32_t id)
: imports_ (importTable)
, active_ (false)
{
	NIV_SAFE_GPA (imports_->beginSample (id));
	active_ = true;
}

////////////////////////////////////////////////////////////////////////////////
Sample::~Sample ()
{
	if (active_) {
		NIV_SAFE_GPA (imports_->endSample ());
	}
}

////////////////////////////////////////////////////////////////////////////////
void Sample::End ()
{
	NIV_SAFE_GPA (imports_->endSample ());
	active_ = false;
}

////////////////////////////////////////////////////////////////////////////////
Pass::Pass (Internal::ImportTable* importTable)
: imports_ (importTable)
, active_ (false)
{
	NIV_SAFE_GPA (imports_->beginPass ());
	active_ = true;
}

////////////////////////////////////////////////////////////////////////////////
Pass::~Pass ()
{
	if (active_) {
		NIV_SAFE_GPA (imports_->endPass ());
	}
}

////////////////////////////////////////////////////////////////////////////////
void Pass::End ()
{
	NIV_SAFE_GPA (imports_->endPass ());
	active_ = false;
}

////////////////////////////////////////////////////////////////////////////////
Sample Pass::BeginSample ()
{
	return BeginSample (0);
}

////////////////////////////////////////////////////////////////////////////////
Sample Pass::BeginSample (const std::uint32_t id)
{
	return Sample (imports_, id);
}

////////////////////////////////////////////////////////////////////////////////
Session::Session (Internal::ImportTable* importTable)
: imports_ (importTable)
, id_ (0)
, active_ (false)
{
	NIV_SAFE_GPA (imports_->beginSession (&id_));
	active_ = true;
}

////////////////////////////////////////////////////////////////////////////////
Session::~Session()
{
	if (active_) {
		NIV_SAFE_GPA (imports_->endSession ());
	}
}

////////////////////////////////////////////////////////////////////////////////
Pass Session::BeginPass ()
{
	return Pass (imports_);
}

////////////////////////////////////////////////////////////////////////////////
void Session::End ()
{
	NIV_SAFE_GPA (imports_->endSession ());
	active_ = false;
}

////////////////////////////////////////////////////////////////////////////////
bool Session::IsReady () const
{
	bool ready = false;

	NIV_SAFE_GPA (imports_->isSessionReady (&ready, id_));

	return ready;
}

////////////////////////////////////////////////////////////////////////////////
SessionResult Session::GetResult () const
{
	return GetResult (true);
}

////////////////////////////////////////////////////////////////////////////////
SessionResult Session::GetResult (const bool block) const
{
	SessionResult result;

	bool ready = IsReady ();

	if (!block && !ready) {
		// Not ready and we don't block to get results, return empty result
		return result;
	} else if (block && !ready) {
		// Not ready and we block to get results, loop until ready
		while (!IsReady ()) { 
			/* Wait */ 
		}
		
		// Will be ready at this point
	} // else, ready, go ahead and fetch results
	
	gpa_uint32 enabledCounterCount = 0;
	NIV_SAFE_GPA (imports_->getEnabledCount (&enabledCounterCount));

	for (gpa_uint32 i = 0; i < enabledCounterCount; ++i) {
		gpa_uint32 index = 0;

		NIV_SAFE_GPA (imports_->getEnabledIndex (i, &index));
		const char* name = nullptr;

		NIV_SAFE_GPA (imports_->getCounterName (index, &name));

		GPA_Type type;
		NIV_SAFE_GPA (imports_->getCounterDataType (index, &type));

		switch (type) {
			case GPA_TYPE_INT32:
			{
				gpa_uint32 value;
				NIV_SAFE_GPA (imports_->getSampleUInt32 (id_, 0, index, &value));
				result [name] = static_cast<std::int32_t> (value);
				break;
			}
			case GPA_TYPE_INT64:
			{
				gpa_uint64 value;
				NIV_SAFE_GPA (imports_->getSampleUInt64 (id_, 0, index, &value));
				result [name] = static_cast<std::int32_t> (value);
				break;
			}
			case GPA_TYPE_UINT32:
			{
				gpa_uint32 value;
				NIV_SAFE_GPA (imports_->getSampleUInt32 (id_, 0, index, &value));
				result [name] = value;
				break;
			}
			case GPA_TYPE_UINT64:
			{
				gpa_uint64 value;
				NIV_SAFE_GPA (imports_->getSampleUInt64 (id_, 0, index, &value));
				result [name] = value;
				break;
			}
			case GPA_TYPE_FLOAT32:
			{
				gpa_float32 value;
				NIV_SAFE_GPA (imports_->getSampleFloat32 (id_, 0, index, &value));
				result [name] = value;
				break;
			}
			case GPA_TYPE_FLOAT64:
			{
				gpa_float64 value;
				NIV_SAFE_GPA (imports_->getSampleFloat64 (id_, 0, index, &value));
				result [name] = value;
				break;
			}

			default:
				throw std::runtime_error ("Unsupported data type.");
		}
	}

	return result;
}

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
Context PerformanceLibrary::OpenContext (void* ctx)
{
	return impl_->OpenContext (ctx);
}

////////////////////////////////////////////////////////////////////////////////
Context::Context (Internal::ImportTable* imports, void* ctx)
: imports_ (imports)
, context_ (ctx)
{
	NIV_SAFE_GPA (imports_->openContext (ctx));
}

////////////////////////////////////////////////////////////////////////////////
Context::Context ()
: imports_ (nullptr)
, context_ (nullptr)
{
}

////////////////////////////////////////////////////////////////////////////////
Context::~Context()
{
	if (context_) {
		Close ();
	}
}

////////////////////////////////////////////////////////////////////////////////
Context::Context (Context&& other)
: imports_ (other.imports_)
, context_ (other.context_)
{
	other.context_ = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
Context& Context::operator= (Context&& other)
{
	imports_ = other.imports_;
	context_ = other.context_;
	other.context_ = nullptr;

	return *this;
}

////////////////////////////////////////////////////////////////////////////////
CounterSet Context::GetAvailableCounters () const
{
	CounterSet::CounterMap result;
	
	gpa_uint32 availableCounters = 0;
	NIV_SAFE_GPA (imports_->getNumCounters (&availableCounters));

	for (gpa_uint32 i = 0; i < availableCounters; ++i) {
		Counter counter;

		const char* name = nullptr;
		NIV_SAFE_GPA (imports_->getCounterName (i, &name));

		GPA_Type dataType;

		NIV_SAFE_GPA (imports_->getCounterDataType (i, &dataType));

		switch (dataType) {
			case GPA_TYPE_UINT32:	counter.type = DataType::uint32; break;
			case GPA_TYPE_UINT64:	counter.type = DataType::uint64; break;
			case GPA_TYPE_FLOAT32:	counter.type = DataType::float32; break;
			case GPA_TYPE_FLOAT64:	counter.type = DataType::float64; break;
			case GPA_TYPE_INT32:	counter.type = DataType::int32; break;
			case GPA_TYPE_INT64:	counter.type = DataType::int64; break;

			default:
				throw std::runtime_error ("Unknown data type.");
		}

		GPA_Usage_Type usageType;

		NIV_SAFE_GPA (imports_->getCounterUsageType (i, &usageType));

		switch (usageType) {
		case GPA_USAGE_TYPE_RATIO:			counter.usage = UsageType::Ratio; break;
		case GPA_USAGE_TYPE_PERCENTAGE:		counter.usage = UsageType::Percentage; break;
		case GPA_USAGE_TYPE_CYCLES:			counter.usage = UsageType::Cycles; break;
		case GPA_USAGE_TYPE_MILLISECONDS:	counter.usage = UsageType::Milliseconds; break;
		case GPA_USAGE_TYPE_BYTES:			counter.usage = UsageType::Bytes; break;
		case GPA_USAGE_TYPE_ITEMS:			counter.usage = UsageType::Items; break;
		case GPA_USAGE_TYPE_KILOBYTES:		counter.usage = UsageType::Kilobytes; break;
		
		default:
			throw std::runtime_error ("Unknown usage type.");
		}

		counter.index = i;

		result [name] = counter;
	}
	
	return CounterSet (imports_, result);
}


////////////////////////////////////////////////////////////////////////////////
void Context::Select ()
{
	NIV_SAFE_GPA (imports_->selectContext (context_));
}

////////////////////////////////////////////////////////////////////////////////
void Context::Close ()
{
	NIV_SAFE_GPA (imports_->closeContext ());
	context_ = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
Session Context::BeginSession()
{
	return Session (imports_);
}
}
