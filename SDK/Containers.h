#pragma once

// Container implementations with iterators. See https://github.com/Fischsalat/UnrealContainers

#include <string>
#include <string_view>
#include <set>
#include <map>
#include <ostream>
#include <unordered_map>

template<typename ArrayElementType>
class TStdArray;

template<typename ArrayType>
class TStdArrayIterator
{
private:
	TStdArray<ArrayType>& IteratedArray;
	int32_t Index;

public:
	TStdArrayIterator(const TStdArray<ArrayType>& Array, int32_t StartIndex = 0x0)
		: IteratedArray(const_cast<TStdArray<ArrayType>&>(Array)), Index(StartIndex)
	{
	}

public:
	inline int32_t GetIndex()
	{
		return Index;
	}

	inline int32_t IsValid()
	{
		return IteratedArray.IsValidIndex(GetIndex());
	}

public:
	inline TStdArrayIterator& operator++()
	{
		++Index; return *this;
	}
	inline TStdArrayIterator& operator--()
	{
		--Index; return *this;
	}

	inline       ArrayType& operator*()
	{
		return IteratedArray[GetIndex()];
	}
	inline const ArrayType& operator*() const { return IteratedArray[GetIndex()]; }

	inline       ArrayType* operator->()
	{
		return &IteratedArray[GetIndex()];
	}
	inline const ArrayType* operator->() const { return &IteratedArray[GetIndex()]; }

	inline bool operator==(const TStdArrayIterator& Other) const { return &IteratedArray == &Other.IteratedArray && Index == Other.Index; }
	inline bool operator!=(const TStdArrayIterator& Other) const { return &IteratedArray != &Other.IteratedArray || Index != Other.Index; }
};


__declspec(noinline) void _TStdArrayAdd(void*& Data, int32_t& NumElements, int32_t& MaxElements, int32_t _Elem_Sz, const void* Element);

template<typename ArrayElementType>
class TStdArray
{
protected:
	static constexpr uint64_t ElementAlign = alignof(ArrayElementType);
	static constexpr uint64_t ElementSize = sizeof(ArrayElementType);

protected:
	ArrayElementType* Data;
	int32_t NumElements;
	int32_t MaxElements;

public:
	TStdArray()
		: Data(nullptr), NumElements(0), MaxElements(0)
	{
	}

	inline TStdArray(int32_t Size)
		: NumElements(0), MaxElements(Size), Data(malloc(Size * sizeof(ArrayElementType)))
	{
	}

	TStdArray(const TStdArray&) = default;

	TStdArray(TStdArray&&) = default;

public:
	TStdArray& operator=(TStdArray&&) = default;
	TStdArray& operator=(const TStdArray&) = default;

public:

	inline ArrayElementType& Add(const ArrayElementType& Element)
	{
		/*if (NumElements + 1 > MaxElements) Data = (ArrayElementType*)realloc(Data, (MaxElements = NumElements + 1) * sizeof(ArrayElementType));

		Data[NumElements++] = Element;
		return Data[NumElements];*/
		_TStdArrayAdd((void*&)Data, NumElements, MaxElements, sizeof(ArrayElementType), &Element);
		return Data[NumElements];
	}

public:
	inline       ArrayElementType& operator[](int32_t Index)
	{
		return Data[Index];
	}
	inline const ArrayElementType& operator[](int32_t Index) const { return Data[Index]; }

public:
	inline TStdArrayIterator<ArrayElementType> begin()
	{
		return TStdArrayIterator<ArrayElementType>(*this, 0);
	}
	inline TStdArrayIterator<ArrayElementType> begin() const { return TStdArrayIterator<ArrayElementType>(*this, 0); }
	inline TStdArrayIterator<ArrayElementType> end()
	{
		return TStdArrayIterator<ArrayElementType>(*this, NumElements);
	}
	inline TStdArrayIterator<ArrayElementType> end() const { return TStdArrayIterator<ArrayElementType>(*this, NumElements); }
};

namespace UC
{
	typedef int8_t  int8;
	typedef int16_t int16;
	typedef int32_t int32;
	typedef int64_t int64;

	typedef uint8_t  uint8;
	typedef uint16_t uint16;
	typedef uint32_t uint32;
	typedef uint64_t uint64;

	template<typename ArrayElementType>
	class TArray;

	template<typename SparseArrayElementType>
	class TSparseArray;

	template<typename SetElementType>
	class TSet;

	template<typename KeyElementType, typename ValueElementType>
	class TMap;

	template<typename KeyElementType, typename ValueElementType>
	class TPair;

	namespace Iterators
	{
		class FSetBitIterator;

		template<typename ArrayType>
		class TArrayIterator;

		template<class ContainerType>
		class TContainerIterator;

		template<typename SparseArrayElementType>
		using TSparseArrayIterator = TContainerIterator<TSparseArray<SparseArrayElementType>>;

		template<typename SetElementType>
		using TSetIterator = TContainerIterator<TSet<SetElementType>>;

		template<typename KeyElementType, typename ValueElementType>
		using TMapIterator = TContainerIterator<TMap<KeyElementType, ValueElementType>>;
	}


	namespace ContainerImpl
	{
		namespace HelperFunctions
		{
			inline uint32 FloorLog2(uint32 Value)
			{
				uint32 pos = 0;
				if (Value >= 1 << 16)
				{
					Value >>= 16; pos += 16;
				}
				if (Value >= 1 << 8)
				{
					Value >>= 8; pos += 8;
				}
				if (Value >= 1 << 4)
				{
					Value >>= 4; pos += 4;
				}
				if (Value >= 1 << 2)
				{
					Value >>= 2; pos += 2;
				}
				if (Value >= 1 << 1)
				{
					pos += 1;
				}
				return pos;
			}

			inline uint32 CountLeadingZeros(uint32 Value)
			{
				if (Value == 0)
					return 32;

				return 31 - FloorLog2(Value);
			}
		}

		template<int32 Size, uint32 Alignment>
		struct TAlignedBytes
		{
			alignas(Alignment) uint8 Pad[Size];
		};

		template<uint32 NumInlineElements>
		class TInlineAllocator
		{
		public:
			template<typename ElementType>
			class ForElementType
			{
			private:
				static constexpr int32 ElementSize = sizeof(ElementType);
				static constexpr int32 ElementAlign = alignof(ElementType);

				static constexpr int32 InlineDataSizeBytes = NumInlineElements * ElementSize;

			private:
				TAlignedBytes<ElementSize, ElementAlign> InlineData[NumInlineElements];
				ElementType* SecondaryData;

			public:
				ForElementType()
					: InlineData{ 0x0 }, SecondaryData(nullptr)
				{
				}

				ForElementType(ForElementType&&) = default;
				ForElementType(const ForElementType&) = default;

			public:
				ForElementType& operator=(ForElementType&&) = default;
				ForElementType& operator=(const ForElementType&) = default;

			public:
				inline const ElementType* GetAllocation() const { return SecondaryData ? SecondaryData : reinterpret_cast<const ElementType*>(&InlineData); }
				inline ElementType* GetAllocation()
				{
					return SecondaryData ? SecondaryData : reinterpret_cast<ElementType*>(&InlineData);
				}

				inline uint32 GetNumInlineBytes() const { return NumInlineElements; }
			};
		};

		class FBitArray
		{
		protected:
			static constexpr int32 NumBitsPerDWORD = 32;
			static constexpr int32 NumBitsPerDWORDLogTwo = 5;

		private:
			TInlineAllocator<4>::ForElementType<int32> Data;
			int32 NumBits;
			int32 MaxBits;

		public:
			FBitArray()
				: NumBits(0), MaxBits(Data.GetNumInlineBytes()* NumBitsPerDWORD)
			{
			}

			FBitArray(const FBitArray&) = default;

			FBitArray(FBitArray&&) = default;

		public:
			FBitArray& operator=(FBitArray&&) = default;

			FBitArray& operator=(const FBitArray& Other) = default;

		public:
			inline int32 Num() const { return NumBits; }
			inline int32 Max() const { return MaxBits; }

			inline uint32* GetData()
			{
				return reinterpret_cast<uint32*>(Data.GetAllocation());
			}
			inline const uint32* GetData() const { return reinterpret_cast<const uint32*>(Data.GetAllocation()); }

			inline bool IsValidIndex(int32 Index) const { return Index >= 0 && Index < NumBits; }

			inline bool IsValid() const { return GetData() && NumBits > 0; }

		public:
			inline bool operator[](int32 Index) const { return GetData()[Index / NumBitsPerDWORD] & (1 << (Index & (NumBitsPerDWORD - 1))); }

			inline bool operator==(const FBitArray& Other) const { return NumBits == Other.NumBits && GetData() == Other.GetData(); }
			inline bool operator!=(const FBitArray& Other) const { return NumBits != Other.NumBits || GetData() != Other.GetData(); }

			inline void Set(const int32 Index, const bool Value, bool bIsSettingAllZero = false)
			{
				const int32 DWORDIndex = (Index >> ((int32)5));
				const int32 Mask = (1 << (Index & (((int32)32) - 1)));

				if (!bIsSettingAllZero)
					NumBits = Index >= NumBits ? Index < MaxBits ? Index + 1 : NumBits : NumBits;

				Value ? GetData()[DWORDIndex] |= Mask : GetData()[DWORDIndex] &= ~Mask;
			}

			inline void Reset()
			{
				NumBits = 0;
			}

		public:
			Iterators::FSetBitIterator begin();
			Iterators::FSetBitIterator end();
		};

		template<typename SparseArrayType>
		union TSparseArrayElementOrFreeListLink
		{
			SparseArrayType ElementData;

			struct
			{
				int32 PrevFreeIndex;
				int32 NextFreeIndex;
			};
		};

		template<typename SetType>
		class SetElement
		{
		private:
			template<typename SetDataType>
			friend class TSet;

		private:
			SetType Value;
			int32 HashNextId;
			int32 HashIndex;
		};
	}


	template <typename KeyType, typename ValueType>
	class TPair
	{
	public:
		KeyType First;
		ValueType Second;

	public:
		TPair(KeyType Key, ValueType Value)
			: First(Key), Second(Value)
		{
		}

	public:
		inline       KeyType& Key()
		{
			return First;
		}
		inline const KeyType& Key() const { return First; }

		inline       ValueType& Value()
		{
			return Second;
		}
		inline const ValueType& Value() const { return Second; }
	};

	class FMemory
	{
	public:
		static __forceinline void* InternalRealloc(void* _a1, __int64 _a2, unsigned int _a3)
		{
			return ((void* (*)(void*, __int64, unsigned int)) SDK::Offsets::Realloc)(_a1, _a2, _a3);
		}

		template<typename T = void>
		static inline T* Realloc(void* Ptr, uint64 Size, uint32 Alignment = 0x0)
		{
			return (T*)InternalRealloc(Ptr, Size, Alignment);
		}

		template<typename T>
		static inline T* ReallocForType(void* Ptr, uint64 Count, int32 Size = sizeof(T))
		{
			return (T*)InternalRealloc(Ptr, Count * Size, alignof(T));
		}


		template<typename T = void>
		static inline T* Malloc(uint64 Size, uint32 Alignment = 0x0)
		{
			return Realloc<T>(nullptr, Size, Alignment);
		}

		template<typename T>
		static inline T* MallocForType(uint64 Count, int32 Size = sizeof(T))
		{
			return ReallocForType<T>(nullptr, Count, Size);
		}

		static inline void Free(void* ptr)
		{
			Realloc(ptr, 0, 0);
		}

		template<typename T>
		static inline void FreeForType(T* ptr)
		{
			ReallocForType<T>(ptr, 0);
		}
	};


	template<class T>
	class TMemoryAllocator
	{
	public:
		typedef T value_type;

		T* allocate(size_t Count)
		{
			return FMemory::MallocForType<T>(Count);
		}

		void deallocate(T* ptr, size_t)
		{
			FMemory::FreeForType<T>(ptr);
		}

		template <typename NT>
		operator TMemoryAllocator<NT>()
		{
			return TMemoryAllocator<NT>();
		}
	};


	using UEAllocatedString = std::basic_string<char, std::char_traits<char>, TMemoryAllocator<char>>;
	using UEAllocatedWString = std::basic_string<wchar_t, std::char_traits<wchar_t>, TMemoryAllocator<wchar_t>>;
	template <class X>
	using UEAllocatedSet = std::set<X, TMemoryAllocator<X>>;
	template <class X>
	using UEAllocatedVector = std::vector<X, TMemoryAllocator<X>>;
	template <class X, class Y>
	using UEAllocatedMap = std::map<X, Y, std::less<X>, TMemoryAllocator<std::pair<const X, Y>>>;
	template <class X, class Y>
	using UEAllocatedUnorderedMap = std::unordered_map<X, Y, std::less<X>, TMemoryAllocator<std::pair<const X, Y>>>;
	using UEAllocatedStringStream = std::basic_stringstream<char, std::char_traits<char>, TMemoryAllocator<char>>;
	using UEAllocatedWStringStream = std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, TMemoryAllocator<wchar_t>>;

	template<typename ArrayElementType>
	class TArray
	{
	private:
		template<typename ArrayElementType>
		friend class TAllocatedArray;

		template<typename SparseArrayElementType>
		friend class TSparseArray;

	protected:
		static constexpr uint64 ElementAlign = alignof(ArrayElementType);
		static constexpr uint64 ElementSize = sizeof(ArrayElementType);

	public:
		ArrayElementType* Data;
		int32 NumElements;
		int32 MaxElements;

	public:
		constexpr TArray()
			: Data(nullptr), NumElements(0), MaxElements(0)
		{
		}

		inline TArray(int32 Size, int32 ElemSize = ElementSize)
			: NumElements(0), MaxElements(Size), Data(FMemory::MallocForType<ArrayElementType>(Size, ElemSize))
		{
		}

		TArray(const TArray&) = default;

		TArray(TArray&&) = default;

		//~TArray() { if (Data) FMemory::Free(Data); }

	public:
		TArray& operator=(TArray&&) = default;
		TArray& operator=(const TArray&) = default;

	private:
		inline int32 GetSlack() const { return MaxElements - NumElements; }

		inline       ArrayElementType& GetUnsafe(int32 Index)
		{
			return Data[Index];
		}
		inline const ArrayElementType& GetUnsafe(int32 Index) const { return Data[Index]; }

	public:
		inline void Reserve(const int Num, int32 Size = ElementSize)
		{
			if (Num + NumElements > MaxElements) Data = FMemory::ReallocForType<ArrayElementType>(Data, MaxElements = Num + NumElements, Size);
		}

		ArrayElementType* GetData() const
		{
			return Data;
		}

		/* Adds to the array if there is still space for one more element */
		inline ArrayElementType& Add(const ArrayElementType& Element, int32 Size = ElementSize)
		{
			Reserve(1, Size);

			memcpy(PBYTE(Data) + (NumElements * Size), (const PBYTE)&Element, Size);
			NumElements++;
			return *(ArrayElementType*)((uint8*)Data + ((NumElements - 1) * Size));
		}

		inline ArrayElementType& AddAt(const ArrayElementType& Element, int32 Index, int32 Size = ElementSize)
		{
			Reserve(1, Size);

			for (int i = NumElements; i > Index; i--)
			{
				memcpy(PBYTE((uint8*)Data + i * Size), (const PBYTE)((uint8*)Data + (i - 1) * Size), Size);
			}
			NumElements++;
			memcpy(PBYTE((uint8*)Data + Index * Size), (const PBYTE)&Element, Size);
			//*(ArrayElementType*)((uint8*)Data + Index * Size) = Element;
			return *(ArrayElementType*)((uint8*)Data + Index * Size);
		}

		inline bool Remove(int32 Index, int32 Size = ElementSize)
		{
			if (!IsValidIndex(Index))
				return false;

			NumElements--;

			for (int i = Index; i < NumElements; i++)
			{
				/* NumElements was decremented, acessing i + 1 is safe */
				memcpy(PBYTE((uint8*)Data + i * Size), (const PBYTE)((uint8*)Data + (i + 1) * Size), Size);
			}

			return true;
		}

		//template <typename ComparisonType>
		bool Contains(/*ComparisonType*/ ArrayElementType Item, int32 Size = ElementSize) const
		{
			for (int Idx = 0; Idx < NumElements; Idx++)
			{
				if (memcmp((ArrayElementType*)((uint8*)Data + Idx * Size), &Item, Size) == 0)
				{
					return true;
				}
			}

			return false;
		}

		inline void Free()
		{
			if (Data)
				FMemory::Free(Data);

			MaxElements = 0;
			NumElements = 0;
			Data = nullptr;
		}

		inline void Free2()
		{
			if (Data && NumElements > 0 && sizeof(ArrayElementType) > 0)
			{
				VirtualFree(Data, sizeof(ArrayElementType) * NumElements, MEM_RELEASE);
			}

			Data = nullptr;
			NumElements = 0;
			MaxElements = 0;
		}

		inline void ResetNum()
		{
			NumElements = 0;
		}

		inline void Clear(int32 Size = ElementSize)
		{
			NumElements = 0;

			if (Data)
				memset((PBYTE)Data, 0, NumElements * Size);
		}

	public:
		inline int32 Num() const { return NumElements; }
		inline int32 Max() const { return MaxElements; }

		inline bool IsValidIndex(int32 Index) const { return Data && Index >= 0 && Index < NumElements; }

		inline bool IsValid() const { return Data && NumElements > 0 && MaxElements >= NumElements; }

		template <class PT>
		__forceinline ArrayElementType* Search(PT Predicate, int32 Size = ElementSize)
		{
			for (int i = 0; i < Num(); i++)
			{
				auto& v = Get(i, Size);

				if (Predicate(v)) return &v;
			}
			return nullptr;
		}

		template <class PT>
		__forceinline int32_t SearchIndex(PT Predicate, int32 Size = ElementSize)
		{
			for (int32_t i = 0; i < Num(); i++)
			{
				auto& v = Get(i, Size);

				if (Predicate(v))
					return i;
			}
			return -1;
		}

		~TArray()
		{
			//if (Data)
			//	FMemory::FreeForType<ArrayElementType>(Data);
		}

	public:
		inline       ArrayElementType& operator[](int32 Index)
		{
			return Data[Index];
		}
		inline const ArrayElementType& operator[](int32 Index) const { return Data[Index]; }

		inline       ArrayElementType& Get(int32 Index, int32 Size = ElementSize)
		{
			return *(ArrayElementType*)((uint8*)Data + Index * Size);
		}
		inline const ArrayElementType& Get(int32 Index, int32 Size = ElementSize) const { return *(ArrayElementType*)((uint8*)Data + Index * Size); }

		inline bool operator==(const TArray<ArrayElementType>& Other) const { return Data == Other.Data; }
		inline bool operator!=(const TArray<ArrayElementType>& Other) const { return Data != Other.Data; }

		template <typename NT>
		operator TArray<NT*>()
		{
			return *(TArray<NT*>*)this;
		}

		inline explicit operator bool() const { return IsValid(); };

	public:
		inline Iterators::TArrayIterator<ArrayElementType> begin()
		{
			return Iterators::TArrayIterator<ArrayElementType>(*this, 0);
		}
		inline Iterators::TArrayIterator<ArrayElementType> begin() const { return Iterators::TArrayIterator<ArrayElementType>(*this, 0); }
		inline Iterators::TArrayIterator<ArrayElementType> end()
		{
			return Iterators::TArrayIterator<ArrayElementType>(*this, Num());
		}
		inline Iterators::TArrayIterator<ArrayElementType> end() const { return Iterators::TArrayIterator<ArrayElementType>(*this, Num()); }
	};

	class FString : public TArray<wchar_t>
	{
	public:
		friend std::ostream& operator<<(std::ostream& Stream, const UC::FString& Str)
		{
			return Stream << Str.ToString();
		}

	public:
		using TArray::TArray;

		FString(const wchar_t* Str)
		{
			const uint32 NullTerminatedLength = static_cast<uint32>(std::wstring_view(Str).size() + 0x1);

			//Data = const_cast<wchar_t*>(Str);
			Data = FMemory::MallocForType<wchar_t>(NullTerminatedLength);
			memcpy(Data, Str, NullTerminatedLength * sizeof(wchar_t));
			NumElements = NullTerminatedLength;
			MaxElements = NullTerminatedLength;
		}

		FString(UEAllocatedString Str)
			: FString(UEAllocatedWString(Str.begin(), Str.end()).c_str())
		{
		}

	public:
		inline UEAllocatedString ToString() const
		{
			if (*this)
			{
				UEAllocatedWString WData(Data);
#pragma warning(suppress: 4244)
				return UEAllocatedString(WData.begin(), WData.end());
			}

			return "";
		}

		inline UEAllocatedString ToStr() const /* hopefully this shit ffucking wokrsksrkrskrskrsksrk!! */
		{
			if (*this)
			{
				UEAllocatedWString WData(Data);
#pragma warning(suppress: 4244)
				return UEAllocatedString(WData.begin(), WData.end());
			}
		}

		inline UEAllocatedWString ToWString() const
		{
			if (*this)
				return UEAllocatedWString(Data);

			return L"";
		}

	public:
		inline       wchar_t* CStr()
		{
			return Data;
		}
		inline const wchar_t* CStr() const { return Data; }

	public:
		inline bool operator==(const FString& Other) const { return Other ? NumElements == Other.NumElements && wcscmp(Data, Other.Data) == 0 : false; }
		inline bool operator!=(const FString& Other) const { return Other ? NumElements != Other.NumElements || wcscmp(Data, Other.Data) != 0 : true; }
	};

	/*
	* Class to allow construction of a TArray, that uses c-style standard-library memory allocation.
	*
	* Useful for calling functions that expect a buffer of a certain size and do not reallocate that buffer.
	* This avoids leaking memory, if the array would otherwise be allocated by the engine, and couldn't be freed without FMemory-functions.
	*/
	template<typename ArrayElementType>
	class TAllocatedArray : public TArray<ArrayElementType>
	{
	};

	/*
	* Class to allow construction of an FString, that uses c-style standard-library memory allocation.
	*
	* Useful for calling functions that expect a buffer of a certain size and do not reallocate that buffer.
	* This avoids leaking memory, if the array would otherwise be allocated by the engine, and couldn't be freed without FMemory-functions.
	*/
	class FAllocatedString : public FString
	{
	};

	template<typename SparseArrayElementType>
	class TSparseArray
	{
	private:
		static constexpr uint32 ElementAlign = alignof(SparseArrayElementType);
		static constexpr uint32 ElementSize = sizeof(SparseArrayElementType);

	private:
		using FElementOrFreeListLink = ContainerImpl::TSparseArrayElementOrFreeListLink<ContainerImpl::TAlignedBytes<ElementSize, ElementAlign>>;

	public:
		TArray<FElementOrFreeListLink> Data;
		ContainerImpl::FBitArray AllocationFlags;
		int32 FirstFreeIndex;
		int32 NumFreeIndices;

	public:
		TSparseArray()
			: FirstFreeIndex(-1), NumFreeIndices(0)
		{
		}

		TSparseArray(TSparseArray&&) = default;
		TSparseArray(const TSparseArray&) = default;

	public:
		TSparseArray& operator=(TSparseArray&&) = default;
		TSparseArray& operator=(const TSparseArray&) = default;

	public:
		inline int32 NumAllocated() const { return Data.Num(); }

		inline int32 Num() const { return NumAllocated() - NumFreeIndices; }
		inline int32 Max() const { return Data.Max(); }

		inline bool IsValidIndex(int32 Index) const { return Data.IsValidIndex(Index) && AllocationFlags[Index]; }

		inline bool IsValid() const { return Data.IsValid() && AllocationFlags.IsValid(); }

		void Remove(int32 Index, int32 Count = 1)
		{
			for (; Count; --Count)
			{
				if (NumFreeIndices) Data[FirstFreeIndex].PrevFreeIndex = Index;
				auto& IndexData = Data[Index];
				IndexData.PrevFreeIndex = -1;
				IndexData.NextFreeIndex = NumFreeIndices > 0 ? FirstFreeIndex : -1;
				FirstFreeIndex = Index;
				++NumFreeIndices;
				AllocationFlags.Set(Index, false);

				++Index;
			}
		}

		/** Empties the array, but keep its allocated memory as slack. */
		void Reset()
		{

			// Free the allocated elements.
			Data.ResetNum();
			FirstFreeIndex = -1;
			NumFreeIndices = 0;
			AllocationFlags.Reset();
		}

	public:
		const ContainerImpl::FBitArray& GetAllocationFlags() const { return AllocationFlags; }

	public:
		inline       SparseArrayElementType& operator[](int32 Index)
		{
			return *reinterpret_cast<SparseArrayElementType*>(&Data.GetUnsafe(Index).ElementData);
		}
		inline const SparseArrayElementType& operator[](int32 Index) const { return *reinterpret_cast<SparseArrayElementType*>(&Data.GetUnsafe(Index).ElementData); }

		inline bool operator==(const TSparseArray<SparseArrayElementType>& Other) const { return Data == Other.Data; }
		inline bool operator!=(const TSparseArray<SparseArrayElementType>& Other) const { return Data != Other.Data; }

	public:
		inline Iterators::TSparseArrayIterator<SparseArrayElementType> begin()
		{
			return Iterators::TSparseArrayIterator<SparseArrayElementType>(*this, GetAllocationFlags(), 0);
		}
		inline Iterators::TSparseArrayIterator<SparseArrayElementType> end()
		{
			return Iterators::TSparseArrayIterator<SparseArrayElementType>(*this, GetAllocationFlags(), NumAllocated());
		}
	};

	template<typename SetElementType>
	class TSet
	{
	private:
		static constexpr uint32 ElementAlign = alignof(SetElementType);
		static constexpr uint32 ElementSize = sizeof(SetElementType);

	private:
		using SetDataType = ContainerImpl::SetElement<SetElementType>;
		using HashType = ContainerImpl::TInlineAllocator<1>::ForElementType<int32>;

	public:
		TSparseArray<SetDataType> Elements;
		HashType Hash;
		int32 HashSize;

	public:
		TSet()
			: HashSize(0)
		{
		}

		TSet(TSet&&) = default;
		TSet(const TSet&) = default;

	public:
		TSet& operator=(TSet&&) = default;
		TSet& operator=(const TSet&) = default;

	public:
		inline int32 NumAllocated() const { return Elements.NumAllocated(); }

		inline int32 Num() const { return Elements.Num(); }
		inline int32 Max() const { return Elements.Max(); }

		inline bool IsValidIndex(int32 Index) const { return Elements.IsValidIndex(Index); }

		inline bool IsValid() const { return Elements.IsValid(); }

		inline void Remove(int32 Index)
		{
			const SetDataType& ElementBeingRemoved = Elements[Index];

			int32* HashPtr = Hash.GetAllocation();
			int32* NextElementIndexIter = &HashPtr[ElementBeingRemoved.HashIndex];
			for (;;)
			{
				int32 NextElementIndex = *NextElementIndexIter;

				if (NextElementIndex == Index)
				{
					*NextElementIndexIter = ElementBeingRemoved.HashNextId;
					break;
				}

				NextElementIndexIter = &Elements[NextElementIndex].HashNextId;
			}

			return Elements.Remove(Index);
		}

		template <typename ComparisonType>
		__forceinline void Remove(const ComparisonType& Element)
		{
			for (auto It = this->begin(); It != this->end(); ++It)
			{
				auto& Elem = *It;
				if (Elem == Element)
				{
					return Remove(It.GetIndex());
				}
			}
		}

		template <typename _Tp> // for operator==
		__forceinline bool Contains(const _Tp& Element)
		{
			for (auto& SetElement : *this)
			{
				if (SetElement == Element) 
					return true;
			}

			return false;
		}

		bool ShouldClearByElements()
		{
			return Num() < (HashSize / 4);
		}

		void UnhashElements()
		{
			if (ShouldClearByElements())
			{
				// Faster path: only reset hash buckets to FSetElementId for elements in the hash
				for (const SetDataType& Element : Elements)
				{
					Hash.GetAllocation()[Element.HashIndex] = -1;
				}
			}
			else
			{
				for (int32 I = 0; I < HashSize; ++I)
				{
					Hash.GetAllocation()[I] = -1;
				}
			}
		}

		void Reset()
		{
			if (Num() == 0)
			{
				return;
			}

			UnhashElements();
			Elements.Reset();
		}

	public:
		const ContainerImpl::FBitArray& GetAllocationFlags() const { return Elements.GetAllocationFlags(); }

	public:
		inline       SetElementType& operator[] (int32 Index)
		{
			return Elements[Index].Value;
		}
		inline const SetElementType& operator[] (int32 Index) const { return Elements[Index].Value; }

		inline bool operator==(const TSet<SetElementType>& Other) const { return Elements == Other.Elements; }
		inline bool operator!=(const TSet<SetElementType>& Other) const { return Elements != Other.Elements; }

	public:
		inline Iterators::TSetIterator<SetElementType> begin() const { return Iterators::TSetIterator<SetElementType>(*this, GetAllocationFlags(), 0); }
		inline Iterators::TSetIterator<SetElementType> end() const { return Iterators::TSetIterator<SetElementType>(*this, GetAllocationFlags(), NumAllocated()); }
	};

	template<typename KeyElementType, typename ValueElementType>
	class TMap
	{
	public:
		using ElementType = TPair<KeyElementType, ValueElementType>;

	public:
		TSet<ElementType> Elements;

	public:
		inline int32 NumAllocated() const { return Elements.NumAllocated(); }

		inline int32 Num() const { return Elements.Num(); }
		inline int32 Max() const { return Elements.Max(); }

		inline bool IsValidIndex(int32 Index) const { return Elements.IsValidIndex(Index); }

		inline bool IsValid() const { return Elements.IsValid(); }

	public:
		const ContainerImpl::FBitArray& GetAllocationFlags() const { return Elements.GetAllocationFlags(); }

	public:
		__forceinline auto Find(const KeyElementType& Key, bool(*Equals)(const KeyElementType& LeftKey, const KeyElementType& RightKey))
		{
			for (auto It = this->begin(); It != this->end(); ++It)
			{
				if (Equals(It->Key(), Key))
					return It;
			}

			return this->end();
		}

		template<typename NewValueType>
		operator TMap<KeyElementType, NewValueType*>()
		{
			return *(TMap<KeyElementType, NewValueType*> *) this;
		}

		template<typename NewValueType>
		operator TMap<KeyElementType, NewValueType*>() const
		{
			return *(TMap<KeyElementType, NewValueType*> *) this;
		}

		template <class PT>
		ValueElementType* Search(PT Predicate)
		{
			for (auto& [k, v] : *this)
			{
				if (Predicate(k, v)) return &v;
			}
			return nullptr;
		}


		template <class PT>
		KeyElementType* SearchForKey(PT Predicate)
		{
			for (auto& [k, v] : *this)
			{
				if (Predicate(k, v)) return &k;
			}
			return nullptr;
		}

		void Reset()
		{
			Elements.Reset();
		}



	public:
		inline       ElementType& operator[] (int32 Index)
		{
			return Elements[Index];
		}
		inline const ElementType& operator[] (int32 Index) const { return Elements[Index]; }

		inline bool operator==(const TMap<KeyElementType, ValueElementType>& Other) const { return Elements == Other.Elements; }
		inline bool operator!=(const TMap<KeyElementType, ValueElementType>& Other) const { return Elements != Other.Elements; }

	public:
		inline Iterators::TMapIterator<KeyElementType, ValueElementType> begin()
		{
			return Iterators::TMapIterator<KeyElementType, ValueElementType>(*this, GetAllocationFlags(), 0);
		}
		inline Iterators::TMapIterator<KeyElementType, ValueElementType> end()
		{
			return Iterators::TMapIterator<KeyElementType, ValueElementType>(*this, GetAllocationFlags(), NumAllocated());
		}
	};

	namespace Iterators
	{
		class FRelativeBitReference
		{
		protected:
			static constexpr int32 NumBitsPerDWORD = 32;
			static constexpr int32 NumBitsPerDWORDLogTwo = 5;

		public:
			inline explicit FRelativeBitReference(int32 BitIndex)
				: WordIndex(BitIndex >> NumBitsPerDWORDLogTwo)
				, Mask(1 << (BitIndex & (NumBitsPerDWORD - 1)))
			{
			}

			int32  WordIndex;
			uint32 Mask;
		};

		class FSetBitIterator : public FRelativeBitReference
		{
		private:
			const ContainerImpl::FBitArray& Array;

			uint32 UnvisitedBitMask;
			int32 CurrentBitIndex;
			int32 BaseBitIndex;

		public:
			explicit FSetBitIterator(const ContainerImpl::FBitArray& InArray, int32 StartIndex = 0)
				: FRelativeBitReference(StartIndex)
				, Array(InArray)
				, UnvisitedBitMask((~0U) << (StartIndex & (NumBitsPerDWORD - 1)))
				, CurrentBitIndex(StartIndex)
				, BaseBitIndex(StartIndex & ~(NumBitsPerDWORD - 1))
			{
				if (StartIndex != Array.Num())
					FindFirstSetBit();
			}

		public:
			inline FSetBitIterator& operator++()
			{
				UnvisitedBitMask &= ~this->Mask;

				FindFirstSetBit();

				return *this;
			}

			inline explicit operator bool() const { return CurrentBitIndex < Array.Num(); }

			inline bool operator==(const FSetBitIterator& Rhs) const { return CurrentBitIndex == Rhs.CurrentBitIndex && &Array == &Rhs.Array; }
			inline bool operator!=(const FSetBitIterator& Rhs) const { return CurrentBitIndex != Rhs.CurrentBitIndex || &Array != &Rhs.Array; }

		public:
			inline int32 GetIndex()
			{
				return CurrentBitIndex;
			}

			void FindFirstSetBit()
			{
				const uint32* ArrayData = Array.GetData();
				const int32   ArrayNum = Array.Num();
				const int32   LastWordIndex = (ArrayNum - 1) / NumBitsPerDWORD;

				uint32 RemainingBitMask = ArrayData[this->WordIndex] & UnvisitedBitMask;
				while (!RemainingBitMask)
				{
					++this->WordIndex;
					BaseBitIndex += NumBitsPerDWORD;
					if (this->WordIndex > LastWordIndex)
					{
						CurrentBitIndex = ArrayNum;
						return;
					}

					RemainingBitMask = ArrayData[this->WordIndex];
					UnvisitedBitMask = ~0;
				}

				const uint32 NewRemainingBitMask = RemainingBitMask & (RemainingBitMask - 1);

				this->Mask = NewRemainingBitMask ^ RemainingBitMask;

				CurrentBitIndex = BaseBitIndex + NumBitsPerDWORD - 1 - ContainerImpl::HelperFunctions::CountLeadingZeros(this->Mask);

				if (CurrentBitIndex > ArrayNum)
					CurrentBitIndex = ArrayNum;
			}
		};

		template<typename ArrayType>
		class TArrayIterator
		{
		private:
			TArray<ArrayType>& IteratedArray;
			int32 Index;

		public:
			TArrayIterator(const TArray<ArrayType>& Array, int32 StartIndex = 0x0)
				: IteratedArray(const_cast<TArray<ArrayType>&>(Array)), Index(StartIndex)
			{
			}

		public:
			inline int32 GetIndex()
			{
				return Index;
			}

			inline int32 IsValid()
			{
				return IteratedArray.IsValidIndex(GetIndex());
			}

		public:
			inline TArrayIterator& operator++()
			{
				++Index; return *this;
			}
			inline TArrayIterator& operator--()
			{
				--Index; return *this;
			}

			inline       ArrayType& operator*()
			{
				return IteratedArray[GetIndex()];
			}
			inline const ArrayType& operator*() const { return IteratedArray[GetIndex()]; }

			inline       ArrayType* operator->()
			{
				return &IteratedArray[GetIndex()];
			}
			inline const ArrayType* operator->() const { return &IteratedArray[GetIndex()]; }

			inline bool operator==(const TArrayIterator& Other) const { return &IteratedArray == &Other.IteratedArray && Index == Other.Index; }
			inline bool operator!=(const TArrayIterator& Other) const { return &IteratedArray != &Other.IteratedArray || Index != Other.Index; }
		};

		template<class ContainerType>
		class TContainerIterator
		{
		private:
			ContainerType& IteratedContainer;
			FSetBitIterator BitIterator;

		public:
			TContainerIterator(const ContainerType& Container, const ContainerImpl::FBitArray& BitArray, int32 StartIndex = 0x0)
				: IteratedContainer(const_cast<ContainerType&>(Container)), BitIterator(BitArray, StartIndex)
			{
			}

		public:
			inline int32 GetIndex()
			{
				return BitIterator.GetIndex();
			}

			inline int32 IsValid()
			{
				return IteratedContainer.IsValidIndex(GetIndex());
			}

		public:
			inline TContainerIterator& operator++()
			{
				++BitIterator; return *this;
			}
			inline TContainerIterator& operator--()
			{
				--BitIterator; return *this;
			}

			inline       auto& operator*()
			{
				return IteratedContainer[GetIndex()];
			}
			inline const auto& operator*() const { return IteratedContainer[GetIndex()]; }

			inline       auto* operator->()
			{
				return &IteratedContainer[GetIndex()];
			}
			inline const auto* operator->() const { return &IteratedContainer[GetIndex()]; }

			inline bool operator==(const TContainerIterator& Other) const { return &IteratedContainer == &Other.IteratedContainer && BitIterator == Other.BitIterator; }
			inline bool operator!=(const TContainerIterator& Other) const { return &IteratedContainer != &Other.IteratedContainer || BitIterator != Other.BitIterator; }
		};
	}

	inline Iterators::FSetBitIterator ContainerImpl::FBitArray::begin()
	{
		return Iterators::FSetBitIterator(*this, 0);
	}
	inline Iterators::FSetBitIterator ContainerImpl::FBitArray::end()
	{
		return Iterators::FSetBitIterator(*this, Num());
	}

	static_assert(sizeof(TArray<int32>) == 0x10, "TArray has a wrong size!");
	static_assert(sizeof(TSet<int32>) == 0x50, "TSet has a wrong size!");
	static_assert(sizeof(TMap<int32, int32>) == 0x50, "TMap has a wrong size!");
}