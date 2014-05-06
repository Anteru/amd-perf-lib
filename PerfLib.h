#ifndef NIV_AMD_PERF_LIB_PERFAPI_H_645363EE_8979_484F_8902_62C026502B0F
#define NIV_AMD_PERF_LIB_PERFAPI_H_645363EE_8979_484F_8902_62C026502B0F

#include <boost/noncopyable.hpp>
#include <boost/variant.hpp>
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
		Direct3D10,
		Direct3D11,
		OpenGL,
		OpenCL
	};
};

typedef std::map<std::string,
	boost::variant<
		float, double,
		std::uint32_t, std::uint64_t,
		std::int32_t, std::int64_t>
	> SessionResult;

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

	CounterMap::iterator begin ();
	CounterMap::iterator end ();
	CounterMap::const_iterator begin () const;
	CounterMap::const_iterator end () const;
	CounterMap::const_iterator cbegin () const;
	CounterMap::const_iterator cend () const;

	Counter& operator [] (const std::string& name);

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
	Sample (Internal::ImportTable* importTable, std::uint32_t id);
	~Sample ();

	void End ();

private:
	Internal::ImportTable*	imports_;
	bool					active_;
};

class Pass : public boost::noncopyable
{
public:
	Pass (Internal::ImportTable* importTable);
	~Pass ();

	void End ();

	Sample BeginSample ();
	Sample BeginSample (const std::uint32_t id);

private:
	Internal::ImportTable* 	imports_;
	bool					active_;
};

class Session : public boost::noncopyable
{
public:
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

class Context : public boost::noncopyable
{
public:
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

class PerformanceLibrary : public boost::noncopyable
{
public:
	PerformanceLibrary (const ProfileApi::Enum targetApi);
	~PerformanceLibrary ();

	Context	OpenContext (void* ctx);

private:
	struct Impl;
	Impl*	impl_;
};
}

#endif
