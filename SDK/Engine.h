#pragma once

namespace SDK
{
	struct FFastArraySerializerItem
	{
	public:
		int32 ReplicationID;
		int32 ReplicationKey;
		int32 MostRecentArrayReplicationKey;
	};

	struct alignas(0x08) FFastArraySerializer
	{
	public:
		TMap<int32, int32> ItemMap;
		int32 IDCounter;
		int32 ArrayReplicationKey;
		char GuidReferencesMap[0x50];

		int32& GetCachedItems()
		{
			return GetFromOffset<int32>(this, VersionInfo.EngineVersion >= 4.23 ? 0xf8 : 0xa8);
		}

		int32& GetCachedItemsToConsiderForWriting()
		{
			return GetFromOffset<int32>(this, VersionInfo.EngineVersion >= 4.23 ? 0xfc : 0xac);
		}

		/** This must be called if you add or change an item in the array */
		void MarkItemDirty(FFastArraySerializerItem& Item)
		{
			if (Item.ReplicationID == -1)
			{
				Item.ReplicationID = ++IDCounter;
				if (IDCounter == -1)
				{
					IDCounter++;
				}
			}

			Item.ReplicationKey++;
			MarkArrayDirty();
		}

		/** This must be called if you just remove something from the array */
		void MarkArrayDirty()
		{
			ItemMap.Reset();        // This allows to clients to add predictive elements to arrays without affecting replication.
			IncrementArrayReplicationKey();

			// Invalidate the cached item counts so that they're recomputed during the next write
			GetCachedItems() = -1;
			GetCachedItemsToConsiderForWriting() = -1;
		}

		void IncrementArrayReplicationKey()
		{
			ArrayReplicationKey++;
			if (ArrayReplicationKey == -1)
			{
				ArrayReplicationKey++;
			}
		}
	};

#define DefineLWCProp(Name, NonLWCOffset, TypeLWC, TypeNonLWC, Ret) \
	Ret Get##Name() const \
	{ \
		if (VersionInfo.FortniteVersion >= 20) \
			return *(TypeLWC*)(__int64(this) + (NonLWCOffset * 2)); \
		else \
			return *(TypeNonLWC*)(__int64(this) + NonLWCOffset); \
	} \
	void Set##Name(Ret New) const \
	{ \
		if (VersionInfo.FortniteVersion >= 20) \
			*(TypeLWC*)(__int64(this) + (NonLWCOffset * 2)) = New; \
		else \
			*(TypeNonLWC*)(__int64(this) + NonLWCOffset) = (TypeNonLWC) New; \
	} \
	__declspec(property(get = Get##Name, put = Set##Name)) \
	TypeLWC Name;

	struct FVector
	{
	public:
		using UnderlayingType = double;                                                                   // 0x0000(0x0008)(NOT AUTO-GENERATED PROPERTY)

		uint8 Padding[0x18];
		DefineLWCProp(X, 0x0, double, float, double);
		DefineLWCProp(Y, 0x4, double, float, double);
		DefineLWCProp(Z, 0x8, double, float, double);
		//float X;
		//float Y;
		//float Z;

		FVector(double _X = 0, double _Y = 0, double _Z = 0)
		{
			X = _X;
			Y = _Y;
			Z = _Z;
		}

		static int32 Size()
		{
			return VersionInfo.FortniteVersion >= 20.00 ? 0x18 : 0xc;
		}

		FVector& operator=(FVector& Rhs)
		{
			memcpy((PBYTE)this, (const PBYTE)&Rhs, Size());
			return *this;
		}

		FVector& operator=(FVector&& Rhs)
		{
			memcpy((PBYTE)this, (const PBYTE)&Rhs, Size());
			return *this;
		}

		FVector(const FVector&) = default;

	public:
		UnderlayingType SizeSquared() const
		{
			return X * X + Y * Y + Z * Z;
		}

		FVector& Normalize()
		{
			*this /= Magnitude();
			return *this;
		}
		FVector& operator*=(const FVector& Other)
		{
			*this = *this * Other;
			return *this;
		}
		FVector& operator*=(float Scalar)
		{
			*this = *this * Scalar;
			return *this;
		}
		FVector& operator+=(const FVector& Other)
		{
			*this = *this + Other;
			return *this;
		}
		FVector& operator-=(const FVector& Other)
		{
			*this = *this - Other;
			return *this;
		}
		FVector& operator/=(const FVector& Other)
		{
			*this = *this / Other;
			return *this;
		}
		FVector& operator/=(UnderlayingType Scalar)
		{
			*this = *this / Scalar;
			return *this;
		}

		UnderlayingType Dot(const FVector& Other) const
		{
			return (X * Other.X) + (Y * Other.Y) + (Z * Other.Z);
		}
		UnderlayingType GetDistanceTo(const FVector& Other) const
		{
			FVector DiffVector = Other - *this;
			return DiffVector.Magnitude();
		}
		UnderlayingType GetDistanceToInMeters(const FVector& Other) const
		{
			return GetDistanceTo(Other) * 0.01f;
		}
		FVector GetNormalized() const
		{
			return *this / Magnitude();
		}
		bool IsZero() const
		{
			return X == 0.0 && Y == 0.0 && Z == 0.0;
		}
		UnderlayingType Magnitude() const
		{
			return std::sqrt((X * X) + (Y * Y) + (Z * Z));
		}
		bool operator!=(const FVector& Other) const
		{
			return X != Other.X || Y != Other.Y || Z != Other.Z;
		}
		FVector operator*(const FVector& Other) const
		{
			return { X * Other.X, Y * Other.Y, Z * Other.Z };
		}
		FVector operator*(UnderlayingType Scalar) const
		{
			return { X * Scalar, Y * Scalar, Z * Scalar };
		}
		FVector operator+(const FVector& Other) const
		{
			return { X + Other.X, Y + Other.Y, Z + Other.Z };
		}
		FVector operator-(const FVector& Other) const
		{
			return { X - Other.X, Y - Other.Y, Z - Other.Z };
		}
		FVector operator/(const FVector& Other) const
		{
			if (Other.X == 0.0f || Other.Y == 0.0f || Other.Z == 0.0f)
				return *this;

			return { X / Other.X, Y / Other.Y, Z / Other.Z };
		}
		FVector operator/(UnderlayingType Scalar) const
		{
			if (Scalar == 0.0f)
				return *this;

			return { X / Scalar, Y / Scalar, Z / Scalar };
		}
		bool operator==(const FVector& Other) const
		{
			return X == Other.X && Y == Other.Y && Z == Other.Z;
		}
	};

	struct FQuat
	{
	public:
		uint8 Padding[0x20];
		DefineLWCProp(X, 0x0, double, float, double);
		DefineLWCProp(Y, 0x4, double, float, double);
		DefineLWCProp(Z, 0x8, double, float, double);
		DefineLWCProp(W, 0xC, double, float, double);

		static int32 Size()
		{
			return VersionInfo.FortniteVersion >= 20.00 ? 0x20 : 0x10;
		}

		FQuat() = default;
		FQuat(const FQuat&) = default;


		FQuat& operator=(FQuat&& Rhs)
		{
			memcpy((PBYTE)this, (const PBYTE)&Rhs, Size());
			return *this;
		}

		FQuat& operator=(FQuat& Rhs)
		{
			memcpy((PBYTE)this, (const PBYTE)&Rhs, Size());
			return *this;
		}

		struct FRotator Rotator();
	};

	struct FRotator final
	{
	public:
		uint8 Padding[0x18];
		DefineLWCProp(Pitch, 0x0, double, float, double);
		DefineLWCProp(Yaw, 0x4, double, float, double);
		DefineLWCProp(Roll, 0x8, double, float, double);

		FRotator() = default;
		FRotator(const FRotator&) = default;

		static int32 Size()
		{
			return VersionInfo.FortniteVersion >= 20.00 ? 0x18 : 0xc;
		}

		FRotator& operator=(FRotator& Rhs)
		{
			memcpy((PBYTE)this, (const PBYTE)&Rhs, Size());
			return *this;
		}

		FRotator& operator=(FRotator&& Rhs)
		{
			memcpy((PBYTE)this, (const PBYTE)&Rhs, Size());
			return *this;
		}

		operator FQuat()
		{
			double halfOfARadian = 0.008726646259971648;
			double sinPitch = sin(Pitch * halfOfARadian),
				sinYaw = sin(Yaw * halfOfARadian),
				sinRoll = sin(Roll * halfOfARadian);
			double cosPitch = cos(Pitch * halfOfARadian),
				cosYaw = cos(Yaw * halfOfARadian),
				cosRoll = cos(Roll * halfOfARadian);

			FQuat out{};
			out.X = cosRoll * sinPitch * sinYaw - sinRoll * cosPitch * cosYaw;
			out.Y = -cosRoll * sinPitch * cosYaw - sinRoll * cosPitch * sinYaw;
			out.Z = cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw;
			out.W = cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw;
			return out;
		}

		operator FVector()
		{
			double oneRadian = 0.017453292519943295;
			double cosPitch = cos(Pitch * oneRadian), cosYaw = cos(Yaw * oneRadian);
			double sinPitch = sin(Pitch * oneRadian), sinYaw = sin(Yaw * oneRadian);

			return FVector(cosPitch * cosYaw, cosPitch * sinYaw, sinPitch);
		}

		static double UnwindDegrees(double Angle)
		{

			Angle = fmod(Angle, 360.0f); // rat

			if (Angle < 0.)
				Angle += 360.f;

			if (Angle > 180.)
				Angle -= 360.f;

			return Angle;
		}

		static double ClampAxis(double Angle)
		{
			Angle = fmod(Angle, 360.f); // rat

			if (Angle < 0.)
				Angle += 360.;
			return Angle;
		}

		static double NormalizeAxis(double Angle)
		{
			Angle = ClampAxis(Angle);

			if (Angle > 180.) Angle -= 360.;
			return Angle;
		}
	};

	inline FRotator FQuat::Rotator()
	{
		const double SingularityTest = Z * X - W * Y;
		const double YawY = 2.f * (W * Z + X * Y);
		const double YawX = (1.f - 2.f * ((Y * Y) + (Z * Z)));

		const double SINGULARITY_THRESHOLD = 0.4999995;
		const double RAD_TO_DEG = 57.29577951308232;
		FRotator RotatorFromQuat{};

		if (SingularityTest < -SINGULARITY_THRESHOLD)
		{
			RotatorFromQuat.Pitch = -90.;
			RotatorFromQuat.Yaw = atan2(YawY, YawX) * RAD_TO_DEG;
			RotatorFromQuat.Roll = FRotator::NormalizeAxis(-RotatorFromQuat.Yaw - (2.f * atan2(X, W) * RAD_TO_DEG));
		}
		else if (SingularityTest > SINGULARITY_THRESHOLD)
		{
			RotatorFromQuat.Pitch = 90.;
			RotatorFromQuat.Yaw = atan2(YawY, YawX) * RAD_TO_DEG;
			RotatorFromQuat.Roll = FRotator::NormalizeAxis(RotatorFromQuat.Yaw - (2.f * atan2(X, W) * RAD_TO_DEG));
		}
		else
		{
			RotatorFromQuat.Pitch = asin(2.f * SingularityTest) * RAD_TO_DEG;
			RotatorFromQuat.Yaw = atan2(YawY, YawX) * RAD_TO_DEG;
			RotatorFromQuat.Roll = atan2(-2.f * (W * X + Y * Z), (1.f - 2.f * ((X * X) + (Y * Y)))) * RAD_TO_DEG;
		}

		return RotatorFromQuat;
	}

	struct FTransform
	{
	public:
		uint8 Padding[0x60];
		DefineLWCProp(Rotation, 0x0, FQuat, FQuat, FQuat&);
		DefineLWCProp(Translation, 0x10, FVector, FVector, FVector&);
		DefineLWCProp(Scale3D, 0x20, FVector, FVector, FVector&);

		static int32 Size()
		{
			return VersionInfo.FortniteVersion >= 20.00 ? 0x60 : 0x30;
		}

		FTransform& operator=(FTransform&& Rhs)
		{
			memcpy((PBYTE)this, (const PBYTE)&Rhs, Size());
			return *this;
		}

		FTransform& operator=(FTransform& Rhs)
		{
			memcpy((PBYTE)this, (const PBYTE)&Rhs, Size());
			return *this;
		}

		FTransform(const FTransform&) = default;

		FTransform(FVector loc = {}, FQuat rot = {}, FVector scale = { 1, 1, 1 })
		{
			Rotation = rot;
			Translation = loc;
			Scale3D = scale;
		}
	};


	template<typename _Ct>
	class TSubclassOf
	{
	public:
		const UClass* ClassPtr;

		TSubclassOf() = default;

		inline TSubclassOf(const UClass* Class)
			: ClassPtr(Class)
		{
		}

		inline const UClass* Get()
		{
			return ClassPtr;
		}

		inline operator const UClass* () const
		{
			return ClassPtr;
		}

		template<typename Target, typename = std::enable_if<std::is_base_of_v<Target, _Ct>, bool>::type>
		inline operator TSubclassOf<Target>() const
		{
			return ClassPtr;
		}

		inline const UClass* operator->()
		{
			return ClassPtr;
		}

		inline TSubclassOf& operator=(UClass* Class)
		{
			ClassPtr = Class;

			return *this;
		}

		inline bool operator==(const TSubclassOf& Other) const
		{
			return ClassPtr == Other.ClassPtr;
		}

		inline bool operator!=(const TSubclassOf& Other) const
		{
			return ClassPtr != Other.ClassPtr;
		}

		inline bool operator==(UClass* Other) const
		{
			return ClassPtr == Other;
		}

		inline bool operator!=(UClass* Other) const
		{
			return ClassPtr != Other;
		}
	};

	class UKismetStringLibrary : public UObject
	{
	public:
		UCLASS_COMMON_MEMBERS(UKismetStringLibrary);

		DEFINE_STATIC_FUNC(Conv_StringToName, FName);
		DEFINE_STATIC_FUNC(Conv_NameToString, FString);

		struct KismetStringLibrary_Conv_StringToName
		{
		public:
			class FString InString;
			class FName ReturnValue;
		};

		/*static FName Conv_StringToName(FString Str)
	{
			KismetStringLibrary_Conv_StringToName Params{ Str };
			Conv_StringToName(&Params);
			return Params.ReturnValue;
		}*/
	};

	class UActorComponent : public UObject
	{
	public:
		UCLASS_COMMON_MEMBERS(UActorComponent);

		DEFINE_FUNC(GetOwner, UObject*); // AActor doesnt exist yet
		DEFINE_FUNC(K2_GetComponentToWorld, FTransform);
	};

	class AActor : public UObject
	{
	public:
		UCLASS_COMMON_MEMBERS(AActor);

		DEFINE_PROP(Owner, AActor*);
		DEFINE_PROP(Role, uint8);
		DEFINE_PROP(RemoteRole, uint8);
		DEFINE_PROP(NetDriverName, FName);
		DEFINE_PROP(NetDormancy, uint8);
		DEFINE_BITFIELD_PROP(bActorIsBeingDestroyed);
		DEFINE_BITFIELD_PROP(bNetStartup);
		DEFINE_BITFIELD_PROP(bOnlyRelevantToOwner);
		DEFINE_PROP(RootComponent, UActorComponent*);
		DEFINE_PROP(Instigator, AActor*);
		DEFINE_PROP(NetUpdateFrequency, float);
		DEFINE_PROP(NetCullDistanceSquared, float);
		DEFINE_BITFIELD_PROP(bAlwaysRelevant);
		DEFINE_BITFIELD_PROP(bCanBeDamaged);
		DEFINE_BITFIELD_PROP(bNetUseOwnerRelevancy);
		DEFINE_BITFIELD_PROP(bTearOff);
		DEFINE_BITFIELD_PROP(bReplicates);
		DEFINE_BITFIELD_PROP(bActorEnableCollision);

		DEFINE_FUNC(AddComponentByClass, UActorComponent*);
		DEFINE_FUNC(GetComponentByClass, UActorComponent*);
		DEFINE_FUNC(SetNetDormancy, void);
		DEFINE_FUNC(FlushNetDormancy, void);
		DEFINE_FUNC(ForceNetUpdate, void);
		DEFINE_FUNC(K2_GetActorLocation, FVector);
		DEFINE_FUNC(K2_GetActorRotation, FRotator);
		DEFINE_FUNC(K2_SetActorLocation, void);
		DEFINE_FUNC(K2_SetActorRotation, void);
		DEFINE_FUNC(GetTransform, FTransform);
		DEFINE_FUNC(SetTransform, void);
		DEFINE_FUNC(K2_TeleportTo, bool);
		DEFINE_FUNC(K2_DestroyActor, void);
		DEFINE_FUNC(GetActorForwardVector, FVector);
		DEFINE_FUNC(GetActorUpVector, FVector);
		DEFINE_FUNC(GetActorRightVector, FVector);
		DEFINE_FUNC(GetDistanceTo, float);
		DEFINE_FUNC(SetLifeSpan, void);
		DEFINE_FUNC(K2_AttachToComponent, void);
	};

	class UGameplayStatics : public SDK::UObject
	{
	public:
		UCLASS_COMMON_MEMBERS(UGameplayStatics);

		DEFINE_STATIC_FUNC(SpawnObject, UObject*);
		DEFINE_STATIC_FUNC(BeginDeferredActorSpawnFromClass, AActor*);
		DEFINE_STATIC_FUNC(FinishSpawningActor, AActor*);
		DEFINE_STATIC_FUNC(GetAllActorsOfClass, void);
		static double GetTimeSeconds(UObject* WorldContextObject)
		{
			static UFunction* GetTimeSeconds__Ptr = nullptr;
			if (!GetTimeSeconds__Ptr)
				GetTimeSeconds__Ptr = GetDefaultObj()->GetFunction("GetTimeSeconds");

			static auto RetSize = 0;

			if (RetSize == 0)
			{
				auto Params = GetTimeSeconds__Ptr->GetParams();

				for (auto& Param : Params.NameOffsetMap)
				{
					if ((Param.PropertyFlags & 0x400) == 0)
						continue;

					RetSize = Param.ElementSize;
					break;
				}
			}

			if (RetSize == 8)
				return GetDefaultObj()->Call<double>(GetTimeSeconds__Ptr, WorldContextObject);
			else
				return GetDefaultObj()->Call<float>(GetTimeSeconds__Ptr, WorldContextObject);
		}
	};

	class ULocalPlayer : public UObject
	{
	public:
		UCLASS_COMMON_MEMBERS(ULocalPlayer);

		DEFINE_PROP(PlayerController, UObject*);
		DEFINE_PROP(ViewportClient, UObject*);
	};

	class UGameInstance : public UObject
	{
	public:
		UCLASS_COMMON_MEMBERS(UGameInstance);

		DEFINE_PROP(LocalPlayers, TArray<ULocalPlayer*>);
	};

	class UGameViewportClient : public SDK::UObject
	{
	public:
		UCLASS_COMMON_MEMBERS(UGameViewportClient);

		DEFINE_PROP(ViewportConsole, SDK::UObject*);
		DEFINE_PROP(World, UObject*); // UWorld doesnt exist yet
	};

	class UEngine : public SDK::UObject
	{
	public:
		UCLASS_COMMON_MEMBERS(UEngine);

		DEFINE_PROP(ConsoleClass, SDK::UClass*);
		DEFINE_PROP(GameViewport, UGameViewportClient*);

		static UEngine* GetEngine()
		{
			static UEngine* _storage = nullptr;
			if (!_storage)
				_storage = (UEngine*)TUObjectArray::FindFirstObject("FortEngine");

			return _storage;
		}
	};

	struct FLevelCollection final
	{
	public:
		USCRIPTSTRUCT_COMMON_MEMBERS(FLevelCollection);

		DEFINE_STRUCT_PROP(NetDriver, UObject*);
	};

	class AServerStreamingLevelsVisibility : public UObject
	{
	public:
		UCLASS_COMMON_MEMBERS(AServerStreamingLevelsVisibility);
	};

	class ULevel : public UObject
	{
	public:
		UCLASS_COMMON_MEMBERS(ULevel);

		DEFINE_PROP(OwningWorld, UObject*);
		DEFINE_BITFIELD_PROP(bIsVisible);
	};

	class UWorld : public UObject
	{
	public:
		UCLASS_COMMON_MEMBERS(UWorld);

		DEFINE_PROP(OwningGameInstance, UGameInstance*);
		DEFINE_PROP(AuthorityGameMode, AActor*);
		DEFINE_PROP(GameState, AActor*);
		DEFINE_PROP(PersistentLevel, ULevel*);
		DEFINE_PROP(NetDriver, UObject*);
		DEFINE_PROP(DemoNetDriver, UObject*);
		DEFINE_PROP(LevelCollections, TArray<FLevelCollection>);
		DEFINE_PROP(ServerStreamingLevelsVisibility, AServerStreamingLevelsVisibility*);
		DEFINE_PROP(NavigationSystem, UObject*);

		struct GameplayStatics_BeginDeferredActorSpawnFromClass
		{
		public:
			const class UObject* WorldContextObject;
			TSubclassOf<class AActor> ActorClass;
			struct FTransform SpawnTransform;
			uint8 CollisionHandlingOverride;
			uint8 _Padding1[0x7];
			class AActor* Owner;
			class AActor* ReturnValue;
			uint8 _Padding2[0x8];
		};

		struct GameplayStatics_FinishSpawningActor
		{
		public:
			class AActor* Actor;
			uint8 _Padding1[0x8];
			struct FTransform SpawnTransform;
			class AActor* ReturnValue;
			uint8 _Padding2[0x8];
		};

		struct FActorSpawnParameters
		{
		public:
			FName Name;

			AActor* Template;
			AActor* Owner;
			AActor* Instigator;
			ULevel* OverrideLevel;
			UObject* OverrideParentComponent;
			uint8_t SpawnCollisionHandlingOverride;

		private:
			uint8 bRemoteOwned : 1;
		public:
			uint8 bNoFail : 1;
			uint8 bDeferConstruction : 1;
			uint8 bAllowDuringConstructionScript : 1;
			uint8_t NameMode;
			uint32_t ObjectFlags;
			void* CallbackSum_Callable;
			void* CallbackSum_HeapAllocation;
		};

		struct FActorSpawnParameters_UE52
		{
		public:
			FName Name;

			AActor* Template;
			AActor* Owner;
			AActor* Instigator;
			ULevel* OverrideLevel;
			UObject* OverrideParentComponent;
			uint8_t SpawnCollisionHandlingOverride;
			uint8_t TransformScaleMethod = 1;

		private:
			uint8 bRemoteOwned : 1;
		public:
			uint8 bNoFail : 1;
			uint8 bDeferConstruction : 1;
			uint8 bAllowDuringConstructionScript : 1;
			uint8_t NameMode;
			uint32_t ObjectFlags;
			void* CallbackSum_Callable;
			void* CallbackSum_HeapAllocation;
		};

		struct FActorSpawnParameters__Legacy
		{
			FName Name = FName(0);
			UObject* Template;
			UObject* Owner;
			AActor* Instigator;
			UObject* OverrideLevel;
			uint8_t SpawnCollisionHandlingOverride;
			uint16	bRemoteOwned : 1;
			uint16	bNoFail : 1;
			uint16	bDeferConstruction : 1;
			uint16	bAllowDuringConstructionScript : 1;
			uint32_t ObjectFlags;
		};

		static AActor* SpawnActor(const UClass* Class, FTransform Transform, AActor* Owner = nullptr, uint8_t CollisionOverride = 2)
		{
			/*if (VersionInfo.EngineVersion >= 5.2)
			{
				auto SpawnActorInternal = (AActor * (*)(UWorld*, const UClass*, FTransform*, void*)) Offsets::SpawnActor;

				FActorSpawnParameters_UE52 SpawnParameters{};

				SpawnParameters.Owner = Owner;
				SpawnParameters.bDeferConstruction = false;
				SpawnParameters.SpawnCollisionHandlingOverride = 1;
				SpawnParameters.NameMode = 3;

				return SpawnActorInternal(GetWorld(), Class, &Transform, &SpawnParameters);
			}
			else if (VersionInfo.EngineVersion >= 5.0)
			{
				auto SpawnActorInternal = (AActor * (*)(UWorld*, const UClass*, FTransform*, void*)) Offsets::SpawnActor;

				FActorSpawnParameters SpawnParameters{};

				SpawnParameters.Owner = Owner;
				SpawnParameters.bDeferConstruction = false;
				SpawnParameters.SpawnCollisionHandlingOverride = 1;
				SpawnParameters.NameMode = 3;

				return SpawnActorInternal(GetWorld(), Class, &Transform, &SpawnParameters);
			}
			else
			{
				auto SpawnActorInternal = (AActor * (*)(UWorld*, const UClass*, FTransform*, void*)) Offsets::SpawnActor;

				FActorSpawnParameters__Legacy SpawnParameters{};

				SpawnParameters.Owner = Owner;
				SpawnParameters.bDeferConstruction = false;
				SpawnParameters.SpawnCollisionHandlingOverride = 1;

				return SpawnActorInternal(GetWorld(), Class, &Transform, &SpawnParameters);
			}*/

			/*static auto BeginDeferredActorSpawnFromClassFn = UGameplayStatics::GetDefaultObj()->GetFunction("BeginDeferredActorSpawnFromClass");
			static auto FinishSpawningActorFn = UGameplayStatics::GetDefaultObj()->GetFunction("FinishSpawningActor");
			GameplayStatics_BeginDeferredActorSpawnFromClass Params{};
			Params.WorldContextObject = GetWorld();
			Params.ActorClass = Class;
			Params.SpawnTransform = Transform;
			Params.CollisionHandlingOverride = 2; // AdjustIfPossibleButAlwaysSpawn
			Params.Owner = Owner;
			
			UGameplayStatics::GetDefaultObj()->ProcessEvent(BeginDeferredActorSpawnFromClassFn, &Params);*/
			auto Actor = UGameplayStatics::BeginDeferredActorSpawnFromClass(GetWorld(), Class, Transform, CollisionOverride, Owner);

			/*GameplayStatics_FinishSpawningActor Params2{};
			Params2.Actor = Params.ReturnValue;
			Params2.SpawnTransform = Transform;
			UGameplayStatics::GetDefaultObj()->ProcessEvent(FinishSpawningActorFn, &Params2);
			return Params2.ReturnValue;*/
			//UGameplayStatics::FinishSpawningActor(&Params2);
			
			return UGameplayStatics::FinishSpawningActor(Actor, Transform);
		}

		static AActor* SpawnActor(const UClass* Class, FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
		{
			return SpawnActor(Class, FTransform(Loc, Rot), Owner);
		}

		template <typename T>
		static T* SpawnActor(const UClass* Class, FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
		{
			return (T*)SpawnActor(Class, Loc, Rot, Owner);
		}

		template <typename T>
		static T* SpawnActor(const UClass* Class, FTransform& Transform, AActor* Owner = nullptr)
		{
			return (T*)SpawnActor(Class, Transform, Owner);
		}

		template <typename T>
		static T* SpawnActor(FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
		{
			return (T*)SpawnActor(T::StaticClass(), Loc, Rot, Owner);
		}

		template <typename T>
		static T* SpawnActor(FTransform& Transform, AActor* Owner = nullptr)
		{
			return (T*)SpawnActor(T::StaticClass(), Transform, Owner);
		}

		template <typename T = AActor>
		static T* SpawnActorUnfinished(const UClass* Class, FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
		{
			FTransform Transform(Loc, Rot);

			return (T*)UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, 2, Owner);
		}

		template <typename T = AActor>
		static T* FinishSpawnActor(AActor* Actor, FVector Loc, FRotator Rot)
		{
			FTransform Transform(Loc, Rot);

			return (T*)UGameplayStatics::FinishSpawningActor(Actor, Transform);
		};

		static UWorld* GetWorld()
		{
			auto Engine = UEngine::GetEngine();

			if (!Engine->GameViewport)
				return nullptr;

			return (UWorld*)Engine->GameViewport->World;
		}
	};

	class UKismetMathLibrary : public SDK::UObject
	{
	public:
		UCLASS_COMMON_MEMBERS(UKismetMathLibrary);

		DEFINE_STATIC_FUNC(RandomIntegerInRange, int);
		DEFINE_STATIC_FUNC(RandomFloatInRange, float);
		DEFINE_STATIC_FUNC(InverseTransformLocation, FVector);
		DEFINE_STATIC_FUNC(InverseTransformDirection, FVector);
	};

	struct FPrimaryAssetId
	{
	public:
		FName PrimaryAssetType;
		uint8_t Padding[0x8];
	};

	class UKismetSystemLibrary : public SDK::UObject
	{
	public:
		UCLASS_COMMON_MEMBERS(UKismetSystemLibrary);

		DEFINE_STATIC_FUNC(ExecuteConsoleCommand, void);
		DEFINE_STATIC_FUNC(GetPathName, FString);
		DEFINE_STATIC_FUNC(GetObjectFromPrimaryAssetId, UObject*);
		DEFINE_STATIC_FUNC(K2_SetTimer, void);

		/*static void ExecuteConsoleCommand(FString Command)
		{
			struct KismetSystemLibrary_ExecuteConsoleCommand
			{
			public:
				class UObject* WorldContextObject;
				class FString Command;
				class UObject* SpecificPlayer;
			} Params{
				UWorld::GetFirstInstance(), Command, nullptr
			};

			ExecuteConsoleCommand(&Params);
		}*/
	};

	class FText
	{
		uint8_t Padding[0x18];


		UEAllocatedString ToString();
	};


	class UKismetTextLibrary : public UObject
	{
	public:
		UCLASS_COMMON_MEMBERS(UKismetTextLibrary);

		DEFINE_STATIC_FUNC(Conv_TextToString, FString);
		DEFINE_STATIC_FUNC(Conv_StringToText, FText);

		struct KismetTextLibrary_Conv_TextToString final
		{
		public:
			FText InText;
			FString ReturnValue;
		};

		/*static inline FString Conv_TextToString(FText text)
		{
			KismetTextLibrary_Conv_TextToString Params{ text };
			Conv_TextToString(&Params);
			return Params.ReturnValue;
		}*/
	};

	inline UEAllocatedString FText::ToString()
	{
		return UKismetTextLibrary::Conv_TextToString(*this).ToString();
	}

	class FOutputDevice
	{
	public:
		void** VTable;
		bool bSuppressEventTag;
		bool bAutoEmitLineTerminator;
	};

	class FFrame : public FOutputDevice
	{
	public:
		UFunction* Node;
		UObject* Object;
		uint8* Code;
		uint8* Locals;
		void* MostRecentProperty;
		uint8_t* MostRecentPropertyAddress;
		uint8_t _Padding1[0x40];
		const UField* PropertyChainForCompiledIn;

	public:
		UFunction* GetCurrentNativeFunction()
		{
			return *(UFunction**)(__int64(this) + Offsets::FFrame_CurrentNativeFunction);
		}
		
		__forceinline void StepCompiledIn(void* const Result = nullptr)
		{
			if (Code)
			{
				((void (*)(FFrame*, UObject*, void* const)) Offsets::Step)(this, Object, Result);
			}
			else
			{
				const UField* _Prop = *(const UField**)(__int64(this) + Offsets::FFrame_PropertyChainForCompiledIn);
				if (_Prop)
				{
					*(const UField**)(__int64(this) + Offsets::FFrame_PropertyChainForCompiledIn) = *(const UField**)(__int64(_Prop) + Offsets::FFrame_Next);
					((void (*)(FFrame*, void* const, const UField*)) Offsets::StepExplicitProperty)(this, Result, _Prop);
				}
			}
		}


		__forceinline void* StepCompiledInRefInternal(void* _Tm)
		{
			MostRecentPropertyAddress = nullptr;
			if (VersionInfo.FortniteVersion >= 20.20)
				*(void**)(__int64(this) + 0x40) = nullptr;

			if (Code)
			{
				((void (*)(FFrame*, UObject*, void* const)) Offsets::Step)(this, Object, _Tm);
			}
			else
			{
				const UField* _Prop = *(const UField**)(__int64(this) + Offsets::FFrame_PropertyChainForCompiledIn);
				*(const UField**)(__int64(this) + Offsets::FFrame_PropertyChainForCompiledIn) = *(const UField**)(__int64(_Prop) + Offsets::FFrame_Next);
				((void (*)(FFrame*, void* const, const UField*)) Offsets::StepExplicitProperty)(this, _Tm, _Prop);
			}

			return MostRecentPropertyAddress ? MostRecentPropertyAddress : _Tm;
		}

		template <typename T>
		__forceinline T& StepCompiledInRef()
		{
			T TempVal{};
			return *(T*)StepCompiledInRefInternal(&TempVal);
		}

		__forceinline void IncrementCode()
		{
			Code += !!Code;
		}
	};

	struct FScriptDelegate
	{
	public:
		FWeakObjectPtr Object;
		FName FunctionName;

		static int32 Size()
		{
			return VersionInfo.FortniteVersion >= 20 ? 0xC : 0x10;
		}
	};

	template<typename FunctionSignature>
	class TMulticastInlineDelegate
	{
	public:
		uint8 Padding[0x10];
	};

	template<typename Ret, typename... Args>
	class TMulticastInlineDelegate<Ret(Args...)>
	{
	public:
		TArray<FScriptDelegate> InvocationList;

		void Process(Args... args)
		{
			for (int i = 0; i < InvocationList.Num(); i++)
			{
				auto& ScriptDelegate = InvocationList.Get(i, FScriptDelegate::Size());

				ScriptDelegate.Object->Call(ScriptDelegate.Object->GetFunction(ScriptDelegate.FunctionName), std::forward<Args>(args)...);
			}
		}

		void Bind(UObject* Object, FName FunctionName)
		{
			FScriptDelegate NewDelegate;

			NewDelegate.Object = Object;
			NewDelegate.FunctionName = FunctionName;

			InvocationList.Add(NewDelegate, FScriptDelegate::Size());
		}
	};
}
