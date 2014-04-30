#ifndef NIV_AMD_PERF_LIB_PERFAPI_H_645363EE_8979_484F_8902_62C026502B0F
#define NIV_AMD_PERF_LIB_PERFAPI_H_645363EE_8979_484F_8902_62C026502B0F

#include <boost/noncopyable.hpp>
#include <boost/variant.hpp>
#include <string>
#include <cstdint>
#include <map>
#include <vector>
#include <functional>

namespace Amd {
struct DataType
{
	enum Enum
	{
		float32,
		float64,
		uint32,
		uint64,
		int32,
		int64
	};
};

struct ProfileApi
{
	enum Enum
	{
		Direct3D10,
		Direct3D11,
		OpenGL,
		OpenCL
	};
};

typedef std::map<std::string, boost::variant<float, double,
	std::uint32_t, std::uint64_t, std::int32_t, std::int64_t>> ProfileResult;

struct CounterInfo
{
	std::string 	name;
	int				index;
	DataType::Enum 	type;
};

class PerformanceLibrary : public boost::noncopyable
{
public:
	PerformanceLibrary (const ProfileApi::Enum targetApi);
	~PerformanceLibrary ();

	typedef std::uint32_t ProfileSessionHandle;

	int GetRequiredPassCount ();

	std::vector<CounterInfo>	GetAvailableCounters () const;
	void EnableCounters (const std::vector<CounterInfo>& counters);

	void BeginPass ();
	void EndPass ();

	ProfileSessionHandle BeginProfileSession ();
	void EndProfileSession ();

	bool GetProfileSessionResult (const ProfileSessionHandle handle,
		ProfileResult& result, const bool block) const;

	void BeginSampling ();
	void EndSampling ();

	void OpenContext (void* ctx);
	void CloseContext ();

	void RegisterLoggingCallback (std::function<void (const int messageType,
		const char* message) callback);
private:
	struct Impl;
	Impl*	impl_;
};
}

#endif
