// SPDX-FileCopyrightText: 2026 DevL0rd
// SPDX-License-Identifier: MIT

#pragma once

#include "base.h"
#include "collision.h"
#include "id.h"
#include "math_functions.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @defgroup particle Particles
 * CorePhys particle and fluid simulation.
 *
 * Particles are dense per-system indices. Particle indices may change after
 * destruction and compaction. Particle systems and particle groups use opaque
 * ids owned by a world.
 * @{
 */

/// Particle behavior flags. Flags may be combined with bitwise OR.
typedef enum b2ParticleFlag
{
	b2_waterParticle = 0,
	b2_zombieParticle = 1 << 1,
	b2_wallParticle = 1 << 2,
	b2_springParticle = 1 << 3,
	b2_elasticParticle = 1 << 4,
	b2_viscousParticle = 1 << 5,
	b2_powderParticle = 1 << 6,
	b2_tensileParticle = 1 << 7,
	b2_colorMixingParticle = 1 << 8,
	b2_destructionListenerParticle = 1 << 9,
	b2_barrierParticle = 1 << 10,
	b2_staticPressureParticle = 1 << 11,
	b2_reactiveParticle = 1 << 12,
	b2_repulsiveParticle = 1 << 13,
	b2_bodyContactListenerParticle = 1 << 14,
	b2_particleContactListenerParticle = 1 << 15,
	b2_bodyContactFilterParticle = 1 << 16,
	b2_particleContactFilterParticle = 1 << 17,
} b2ParticleFlag;

/// Particle group construction flags. Flags may be combined with bitwise OR.
typedef enum b2ParticleGroupFlag
{
	b2_solidParticleGroup = 1 << 0,
	b2_rigidParticleGroup = 1 << 1,
	b2_particleGroupCanBeEmpty = 1 << 2,
} b2ParticleGroupFlag;

/// Small color stored per particle.
typedef struct b2ParticleColor
{
	uint8_t r, g, b, a;
} b2ParticleColor;

/// Particle system definition. Must be initialized with b2DefaultParticleSystemDef().
typedef struct b2ParticleSystemDef
{
	bool strictContactCheck;
	float density;
	float gravityScale;
	float radius;
	int maxCount;
	float pressureStrength;
	float dampingStrength;
	float elasticStrength;
	float springStrength;
	float viscousStrength;
	float surfaceTensionPressureStrength;
	float surfaceTensionNormalStrength;
	float repulsiveStrength;
	float powderStrength;
	float ejectionStrength;
	float staticPressureStrength;
	float staticPressureRelaxation;
	int staticPressureIterations;
	float colorMixingStrength;
	bool destroyByAge;
	float lifetimeGranularity;
	int iterationCount;
	bool enableParticleEvents;
	bool enableParticleBodyEvents;
	bool enableParticleDebugDraw;
	void* userData;
	int internalValue;
} b2ParticleSystemDef;

/// Particle definition. Must be initialized with b2DefaultParticleDef().
typedef struct b2ParticleDef
{
	uint32_t flags;
	b2Vec2 position;
	b2Vec2 velocity;
	b2ParticleColor color;
	float lifetime;
	void* userData;
	b2ParticleGroupId groupId;
	int internalValue;
} b2ParticleDef;

/// Particle group definition. Must be initialized with b2DefaultParticleGroupDef().
typedef struct b2ParticleGroupDef
{
	uint32_t flags;
	uint32_t groupFlags;
	b2Vec2 position;
	b2Rot rotation;
	b2Vec2 linearVelocity;
	float angularVelocity;
	b2ParticleColor color;
	float strength;
	const b2Circle* circle;
	const b2Capsule* capsule;
	const b2Segment* segment;
	const b2Polygon* polygon;
	float stride;
	int particleCount;
	const b2Vec2* positionData;
	float lifetime;
	void* userData;
	b2ParticleGroupId groupId;
	int internalValue;
} b2ParticleGroupDef;

/// Particle-particle contact data.
typedef struct b2ParticleContactData
{
	int indexA;
	int indexB;
	float weight;
	b2Vec2 normal;
} b2ParticleContactData;

/// Particle-body contact data.
typedef struct b2ParticleBodyContactData
{
	int particleIndex;
	b2ShapeId shapeId;
	float weight;
	float mass;
	b2Vec2 normal;
} b2ParticleBodyContactData;

/// Persistent spring/barrier connection data.
typedef struct b2ParticlePairData
{
	int indexA;
	int indexB;
	uint32_t flags;
	float strength;
	float distance;
} b2ParticlePairData;

/// Persistent elastic triangle connection data.
typedef struct b2ParticleTriadData
{
	int indexA;
	int indexB;
	int indexC;
	uint32_t flags;
	float strength;
	b2Vec2 pa;
	b2Vec2 pb;
	b2Vec2 pc;
	float ka;
	float kb;
	float kc;
	float s;
} b2ParticleTriadData;

/// Particle destruction event.
typedef struct b2ParticleDestructionEvent
{
	b2ParticleSystemId systemId;
	int particleIndex;
	void* userData;
} b2ParticleDestructionEvent;

/// Particle contact events for the current world step.
typedef struct b2ParticleContactEvents
{
	const b2ParticleContactData* beginEvents;
	const b2ParticleContactData* endEvents;
	int beginCount;
	int endCount;
} b2ParticleContactEvents;

/// Particle-body contact events for the current world step.
typedef struct b2ParticleBodyContactEvents
{
	const b2ParticleBodyContactData* beginEvents;
	const b2ParticleBodyContactData* endEvents;
	int beginCount;
	int endCount;
} b2ParticleBodyContactEvents;

/// Particle destruction events for the current world step.
typedef struct b2ParticleDestructionEvents
{
	const b2ParticleDestructionEvent* events;
	int count;
} b2ParticleDestructionEvents;

/// Particle-particle contact filter callback. Return false to reject the contact.
typedef bool b2ParticleContactFilterFcn( b2ParticleSystemId systemId, int indexA, int indexB, void* context );

/// Particle-body contact filter callback. Return false to reject the contact or collision.
typedef bool b2ParticleBodyContactFilterFcn( b2ParticleSystemId systemId, b2ShapeId shapeId, int particleIndex, void* context );

/// Particle query filter callback. Return false to exclude the particle from query destruction.
typedef bool b2ParticleQueryFilterFcn( b2ParticleSystemId systemId, int particleIndex, void* context );

/// Particle profile data in milliseconds.
typedef struct b2ParticleProfile
{
	float step;
	float lifetimes;
	float zombie;
	float proxies;
	float spatialIndexBuild;
	float spatialIndexScatter;
	float contacts;
	float contactCandidates;
	float contactMerge;
	float bodyContacts;
	float bodyCandidateQuery;
	float weights;
	float forces;
	float pressure;
	float damping;
	float reductionGenerate;
	float reductionApply;
	float collision;
	float solidDepth;
	float solidEjection;
	float groups;
	float groupRefresh;
	float barrier;
	float compaction;
	float compactionMark;
	float compactionPrefix;
	float compactionScatter;
	float compactionRemap;
	float scratch;
	float events;
} b2ParticleProfile;

/// Particle system counters.
typedef struct b2ParticleSystemCounters
{
	int particleCount;
	int groupCount;
	int contactCount;
	int bodyContactCount;
	int pairCount;
	int triadCount;
	int destructionEventCount;
	int taskRangeCount;
	int taskRangeMin;
	int taskRangeMax;
	int spatialCellCount;
	int occupiedCellCount;
	int spatialProxyCount;
	int spatialScatterCount;
	int contactCandidateCount;
	int bodyShapeCandidateCount;
	int barrierCandidateCount;
	int reductionDeltaCount;
	int reductionApplyCount;
	int groupRefreshCount;
	int compactionMoveCount;
	int compactionRemapCount;
	int byteCount;
	int scratchByteCount;
	int scratchHighWaterByteCount;
} b2ParticleSystemCounters;

B2_API b2ParticleSystemDef b2DefaultParticleSystemDef( void );
B2_API b2ParticleDef b2DefaultParticleDef( void );
B2_API b2ParticleGroupDef b2DefaultParticleGroupDef( void );
B2_API int b2CalculateParticleIterations( float gravity, float radius, float timeStep );

B2_API b2ParticleSystemId b2CreateParticleSystem( b2WorldId worldId, const b2ParticleSystemDef* def );
B2_API void b2DestroyParticleSystem( b2ParticleSystemId systemId );
B2_API bool b2ParticleSystem_IsValid( b2ParticleSystemId systemId );
B2_API b2WorldId b2ParticleSystem_GetWorld( b2ParticleSystemId systemId );
B2_API void b2ParticleSystem_SetUserData( b2ParticleSystemId systemId, void* userData );
B2_API void* b2ParticleSystem_GetUserData( b2ParticleSystemId systemId );

B2_API int b2ParticleSystem_CreateParticle( b2ParticleSystemId systemId, const b2ParticleDef* def );
B2_API void b2ParticleSystem_DestroyParticle( b2ParticleSystemId systemId, int particleIndex );
B2_API int b2ParticleSystem_DestroyParticlesInCircle( b2ParticleSystemId systemId, const b2Circle* circle, b2Transform transform );
B2_API int b2ParticleSystem_DestroyParticlesInSegment( b2ParticleSystemId systemId, const b2Segment* segment, b2Transform transform );
B2_API int b2ParticleSystem_DestroyParticlesInCapsule( b2ParticleSystemId systemId, const b2Capsule* capsule, b2Transform transform );
B2_API int b2ParticleSystem_DestroyParticlesInPolygon( b2ParticleSystemId systemId, const b2Polygon* polygon, b2Transform transform );
B2_API int b2ParticleSystem_GetParticleCount( b2ParticleSystemId systemId );
B2_API int b2ParticleSystem_GetMaxParticleCount( b2ParticleSystemId systemId );
B2_API void b2ParticleSystem_SetMaxParticleCount( b2ParticleSystemId systemId, int count );
B2_API void b2ParticleSystem_SetParticleLifetime( b2ParticleSystemId systemId, int particleIndex, float lifetime );
B2_API float b2ParticleSystem_GetParticleLifetime( b2ParticleSystemId systemId, int particleIndex );

B2_API b2ParticleGroupId b2ParticleSystem_CreateParticleGroup( b2ParticleSystemId systemId, const b2ParticleGroupDef* def );
B2_API void b2DestroyParticleGroup( b2ParticleGroupId groupId );
B2_API bool b2ParticleGroup_IsValid( b2ParticleGroupId groupId );
B2_API int b2ParticleGroup_GetParticleCount( b2ParticleGroupId groupId );
B2_API void b2ParticleGroup_GetParticleRange( b2ParticleGroupId groupId, int* startIndex, int* count );
B2_API float b2ParticleGroup_GetMass( b2ParticleGroupId groupId );
B2_API float b2ParticleGroup_GetInertia( b2ParticleGroupId groupId );
B2_API b2Vec2 b2ParticleGroup_GetCenter( b2ParticleGroupId groupId );
B2_API b2Vec2 b2ParticleGroup_GetLinearVelocity( b2ParticleGroupId groupId );
B2_API float b2ParticleGroup_GetAngularVelocity( b2ParticleGroupId groupId );
B2_API b2Transform b2ParticleGroup_GetTransform( b2ParticleGroupId groupId );
B2_API float b2ParticleGroup_GetAngle( b2ParticleGroupId groupId );
B2_API b2Vec2 b2ParticleGroup_GetLinearVelocityFromWorldPoint( b2ParticleGroupId groupId, b2Vec2 worldPoint );
B2_API void b2ParticleGroup_Join( b2ParticleGroupId groupA, b2ParticleGroupId groupB );
B2_API void b2ParticleGroup_Split( b2ParticleGroupId groupId );

B2_API float b2ParticleSystem_GetRadius( b2ParticleSystemId systemId );
B2_API void b2ParticleSystem_SetRadius( b2ParticleSystemId systemId, float radius );
B2_API float b2ParticleSystem_GetDensity( b2ParticleSystemId systemId );
B2_API void b2ParticleSystem_SetDensity( b2ParticleSystemId systemId, float density );
B2_API float b2ParticleSystem_GetGravityScale( b2ParticleSystemId systemId );
B2_API void b2ParticleSystem_SetGravityScale( b2ParticleSystemId systemId, float gravityScale );
B2_API float b2ParticleSystem_GetDamping( b2ParticleSystemId systemId );
B2_API void b2ParticleSystem_SetDamping( b2ParticleSystemId systemId, float damping );
B2_API int b2ParticleSystem_GetIterationCount( b2ParticleSystemId systemId );
B2_API void b2ParticleSystem_SetIterationCount( b2ParticleSystemId systemId, int count );
B2_API void b2ParticleSystem_SetPressureStrength( b2ParticleSystemId systemId, float value );
B2_API void b2ParticleSystem_SetViscousStrength( b2ParticleSystemId systemId, float value );
B2_API void b2ParticleSystem_SetPowderStrength( b2ParticleSystemId systemId, float value );
B2_API void b2ParticleSystem_SetSurfaceTensionStrengths( b2ParticleSystemId systemId, float pressure, float normal );
B2_API void b2ParticleSystem_SetStaticPressureStrengths( b2ParticleSystemId systemId, float strength, float relaxation, int iterations );
B2_API void b2ParticleSystem_SetColorMixingStrength( b2ParticleSystemId systemId, float value );
B2_API void b2ParticleSystem_SetDestructionByAge( b2ParticleSystemId systemId, bool enabled );
B2_API void b2ParticleSystem_SetContactFilter( b2ParticleSystemId systemId, b2ParticleContactFilterFcn* filter, void* context );
B2_API void b2ParticleSystem_SetBodyContactFilter( b2ParticleSystemId systemId, b2ParticleBodyContactFilterFcn* filter,
												   void* context );
B2_API void b2ParticleSystem_SetQueryFilter( b2ParticleSystemId systemId, b2ParticleQueryFilterFcn* filter, void* context );

B2_API const b2Vec2* b2ParticleSystem_GetPositionBuffer( b2ParticleSystemId systemId );
B2_API b2Vec2* b2ParticleSystem_GetMutablePositionBuffer( b2ParticleSystemId systemId );
B2_API const b2Vec2* b2ParticleSystem_GetVelocityBuffer( b2ParticleSystemId systemId );
B2_API b2Vec2* b2ParticleSystem_GetMutableVelocityBuffer( b2ParticleSystemId systemId );
B2_API const uint32_t* b2ParticleSystem_GetFlagsBuffer( b2ParticleSystemId systemId );
B2_API uint32_t* b2ParticleSystem_GetMutableFlagsBuffer( b2ParticleSystemId systemId );
B2_API const b2ParticleColor* b2ParticleSystem_GetColorBuffer( b2ParticleSystemId systemId );
B2_API b2ParticleColor* b2ParticleSystem_GetMutableColorBuffer( b2ParticleSystemId systemId );
B2_API const float* b2ParticleSystem_GetWeightBuffer( b2ParticleSystemId systemId );
B2_API const float* b2ParticleSystem_GetLifetimeBuffer( b2ParticleSystemId systemId );
B2_API float* b2ParticleSystem_GetMutableLifetimeBuffer( b2ParticleSystemId systemId );
B2_API void* const* b2ParticleSystem_GetUserDataBuffer( b2ParticleSystemId systemId );
B2_API void** b2ParticleSystem_GetMutableUserDataBuffer( b2ParticleSystemId systemId );

B2_API const b2ParticleContactData* b2ParticleSystem_GetContacts( b2ParticleSystemId systemId );
B2_API int b2ParticleSystem_GetContactCount( b2ParticleSystemId systemId );
B2_API const b2ParticleBodyContactData* b2ParticleSystem_GetBodyContacts( b2ParticleSystemId systemId );
B2_API int b2ParticleSystem_GetBodyContactCount( b2ParticleSystemId systemId );
B2_API const b2ParticlePairData* b2ParticleSystem_GetPairs( b2ParticleSystemId systemId );
B2_API int b2ParticleSystem_GetPairCount( b2ParticleSystemId systemId );
B2_API const b2ParticleTriadData* b2ParticleSystem_GetTriads( b2ParticleSystemId systemId );
B2_API int b2ParticleSystem_GetTriadCount( b2ParticleSystemId systemId );
B2_API b2ParticleSystemCounters b2ParticleSystem_GetCounters( b2ParticleSystemId systemId );
B2_API b2ParticleProfile b2ParticleSystem_GetProfile( b2ParticleSystemId systemId );
B2_API b2ParticleContactEvents b2ParticleSystem_GetContactEvents( b2ParticleSystemId systemId );
B2_API b2ParticleBodyContactEvents b2ParticleSystem_GetBodyContactEvents( b2ParticleSystemId systemId );
B2_API b2ParticleDestructionEvents b2ParticleSystem_GetDestructionEvents( b2ParticleSystemId systemId );

/** @} */
