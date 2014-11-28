#ifndef NIV_AMD_PERF_LIB_PERFAPI_H_645363EE_8979_484F_8902_62C026502B0F
#define NIV_AMD_PERF_LIB_PERFAPI_H_645363EE_8979_484F_8902_62C026502B0F

#include <string>
#include <cstdint>
#include <map>
#include <vector>

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

struct UsageType
{
	enum Enum
	{
		Ratio,         ///< Result is a ratio of two different values or types
		Percentage,    ///< Result is a percentage, typically within [0,100] range, but may be higher for certain counters
		Cycles,        ///< Result is in clock cycles
		Milliseconds,  ///< Result is in milliseconds
		Bytes,         ///< Result is in bytes
		Items,         ///< Result is a count of items or objects (ie, vertices, triangles, threads, pixels, texels, etc)
		Kilobytes
	};
};

struct ProfileApi
{
	enum Enum
	{
		Direct3D11,
		OpenGL,
		OpenCL
	};
};

struct ResultEntry
{
	union
	{
		float f32;
		double f64;
		std::uint32_t u32;
		std::uint64_t u64;
		std::int32_t i32;
		std::int64_t i64;
	};

	DataType::Enum dataType;
};

typedef std::map<std::string, ResultEntry> SessionResult;

struct Counter
{
	int				index;
	DataType::Enum 	type;
	UsageType::Enum usage;
};

class Exception : public std::runtime_error
{
public:
	Exception (const int errorCode);

	int GetErrorCode () const;

private:
	int errorCode_;
};

namespace Internal {
struct ImportTable;
}

class CounterSet
{
public:
	typedef std::map<std::string, Counter> CounterMap;

	CounterSet (Internal::ImportTable* importTable, const CounterMap& counters);
	CounterSet ();

	CounterMap::const_iterator begin () const;
	CounterMap::const_iterator end () const;
	CounterMap::const_iterator cbegin () const;
	CounterMap::const_iterator cend () const;

	const Counter& operator [] (const std::string& name) const;

	void Keep (const std::vector<std::string>& counters);

	int GetRequiredPassCount () const;

	void Enable ();
	void Disable ();

private:
	Internal::ImportTable*			imports_;
	std::map<std::string, Counter>	counters_;
};

class Sample
{
public:
	// Noncopyable
	Sample (const Sample& other) = delete;
	Sample& operator= (const Sample& other) = delete;

	Sample (Sample&& other);
	Sample& operator= (Sample&& other);
	
	Sample (Internal::ImportTable* importTable, std::uint32_t id);
	~Sample ();

	void End ();

private:
	Internal::ImportTable*	imports_;
	bool					active_;
};

class Pass
{
public:
	// Noncopyable
	Pass (const Pass& other) = delete;
	Pass& operator= (const Pass& other) = delete;

	Pass (Pass&& other);
	Pass& operator= (Pass&& other);
	
	Pass (Internal::ImportTable* importTable);
	~Pass ();

	void End ();

	Sample BeginSample ();
	Sample BeginSample (const std::uint32_t id);

private:
	Internal::ImportTable* 	imports_;
	bool					active_;
};

class Session
{
public:
	// Noncopyable
	Session (const Session& other) = delete;
	Session& operator= (const Session& other) = delete;
	
	Session (Session&& other);
	Session& operator=(Session&& other);
	
	Session (Internal::ImportTable* importTable);
	~Session ();

	Pass BeginPass ();
	void End ();

	bool IsReady () const;
	SessionResult GetResult (const bool block) const;
	SessionResult GetResult () const;

private:
	Internal::ImportTable*	imports_;
	std::uint32_t			id_;
	bool					active_;
};

class Context
{
public:
	// Noncopyable
	Context (const Context& other) = delete;
	Context& operator= (const Context& other) = delete;

	Context (Internal::ImportTable* imports, void* ctx);
	Context ();
	~Context ();

	Context (Context&& other);
	Context& operator= (Context&& other);

	void Select ();
	void Close ();

	CounterSet	GetAvailableCounters () const;

	Session BeginSession ();

private:
	Internal::ImportTable*	imports_;
	void*					context_;
};

class PerformanceLibrary
{
public:
	// Noncopyable
	PerformanceLibrary (const PerformanceLibrary& other) = delete;
	PerformanceLibrary& operator= (const PerformanceLibrary& other) = delete;

	PerformanceLibrary (const ProfileApi::Enum targetApi);
	~PerformanceLibrary ();

	Context	OpenContext (void* ctx);

private:
	struct Impl;
	Impl*	impl_;
};
}

#endif
