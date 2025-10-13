#pragma once
#include "Memcury.h"
#include <array>

namespace SDK
{
	struct FVersionInfo
	{
		double EngineVersion = 0.f;
		double FortniteVersion = 0.f;
	};
	struct FStringNoOps
	{
		wchar_t* Data;
		int32_t NumElements;
		int32_t MaxElements;
	};
	inline FVersionInfo VersionInfo{};

    namespace Offsets
    {
		inline uint64_t Realloc = 0;
		inline uint64_t AppendString = 0;
		inline uint64_t ToString = 0;
		inline uint64_t ProcessEvent = 0;
		inline uint64_t GObjectsChunked = 0;
		inline uint64_t GObjectsUnchunked = 0;
		inline uint64_t Step = 0;
		inline uint64_t StepExplicitProperty = 0;
		inline uint64_t GetInterfaceAddress = 0;
		inline uint64_t StaticFindObject = 0;
		inline uint64_t StaticLoadObject = 0;

		inline uint32_t Offset_Internal = 0;
		inline uint32_t ElementSize = 0;
		inline uint32_t PropertyFlags = 0;
		inline uint32_t PropertiesSize = 0;
		inline uint32_t Super = 0;
		inline uint32_t FieldMask = 0;
		inline uint32_t Children = 0;
		inline uint32_t FField_Next = 0;
		inline uint32_t FField_Name = 0;
		inline uint32_t ExecFunction = 0;
    }

	void Init();
}