// SPDX-FileCopyrightText: 2026 DevL0rd
// SPDX-License-Identifier: MIT

#pragma once

#include "id_pool.h"
#include "particle_scratch.h"

#include "corephys/particle.h"
#include "corephys/types.h"

#include <stdint.h>

typedef struct b2World b2World;

typedef struct b2ParticleProxy
{
	int index;
	int cellX;
	int cellY;
	uint64_t key;
} b2ParticleProxy;

typedef struct b2ParticleState
{
	b2Vec2 position;
	b2Vec2 velocity;
	b2Vec2 force;
	uint32_t flags;
	b2ParticleColor color;
	void* userData;
	float weight;
	float lifetime;
	float age;
	float depth;
	int groupIndex;
} b2ParticleState;

typedef struct b2ParticleGroup
{
	// World-unique public id.
	int id;
	// Local slot inside the owning particle system.
	int localIndex;
	int systemId;
	int firstIndex;
	int count;
	uint32_t flags;
	uint32_t groupFlags;
	b2ParticleColor color;
	float strength;
	float mass;
	float inertia;
	b2Vec2 center;
	b2Vec2 linearVelocity;
	float angularVelocity;
	b2Transform transform;
	void* userData;
	uint16_t generation;
} b2ParticleGroup;

typedef struct b2ParticlePayloadStorage
{
	b2ParticleScratchArray* arrays;
	int count;
	int capacity;
	int elementSize;
} b2ParticlePayloadStorage;

typedef struct b2ParticleSystem
{
	int id;
	uint16_t generation;
	int worldId;
	void* userData;
	b2ParticleSystemDef def;

	int particleCount;
	int particleCapacity;
	uint32_t allParticleFlags;
	b2Vec2* positions;
	b2Vec2* velocities;
	b2Vec2* forces;
	uint32_t* flags;
	b2ParticleColor* colors;
	void** userDataBuffer;
	float* weights;
	float* lifetimes;
	float* ages;
	float* accumulations;
	float* staticPressures;
	float* depths;
	b2Vec2* accumulationVectors;
	int* groupIndices;

	int proxyCount;
	int proxyCapacity;
	b2ParticleProxy* proxies;

	int contactCount;
	int contactCapacity;
	b2ParticleContactData* contacts;
	int previousContactCount;
	int previousContactCapacity;
	b2ParticleContactData* previousContacts;
	int contactBeginCount;
	int contactBeginCapacity;
	b2ParticleContactData* contactBeginEvents;
	int contactEndCount;
	int contactEndCapacity;
	b2ParticleContactData* contactEndEvents;

	int bodyContactCount;
	int bodyContactCapacity;
	b2ParticleBodyContactData* bodyContacts;
	int previousBodyContactCount;
	int previousBodyContactCapacity;
	b2ParticleBodyContactData* previousBodyContacts;
	int bodyContactBeginCount;
	int bodyContactBeginCapacity;
	b2ParticleBodyContactData* bodyContactBeginEvents;
	int bodyContactEndCount;
	int bodyContactEndCapacity;
	b2ParticleBodyContactData* bodyContactEndEvents;

	int destructionEventCount;
	int destructionEventCapacity;
	b2ParticleDestructionEvent* destructionEvents;

	int pairCount;
	int pairCapacity;
	b2ParticlePairData* pairs;

	int triadCount;
	int triadCapacity;
	b2ParticleTriadData* triads;

	b2IdPool groupIdPool;
	int groupCount;
	int groupCapacity;
	b2ParticleGroup* groups;

	b2ParticleContactFilterFcn* contactFilterFcn;
	void* contactFilterContext;
	b2ParticleBodyContactFilterFcn* bodyContactFilterFcn;
	void* bodyContactFilterContext;
	b2ParticleQueryFilterFcn* queryFilterFcn;
	void* queryFilterContext;

	int taskCount;
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

	b2ParticleScratchBuffer scratch;
	b2ParticlePayloadStorage contactBlockPayloads;
	b2ParticlePayloadStorage bodyContactBlockPayloads;
	b2ParticlePayloadStorage bodyImpulseBlockPayloads;
	b2ParticlePayloadStorage shapeCandidatePayloads;
	b2ParticlePayloadStorage floatDeltaBlockPayloads;
	b2ParticlePayloadStorage vec2DeltaBlockPayloads;
	b2ParticlePayloadStorage barrierGroupDeltaBlockPayloads;
	b2ParticlePayloadStorage eventContactSortPayloads;
	b2ParticlePayloadStorage eventBodyContactSortPayloads;
	b2ParticleProfile profile;
} b2ParticleSystem;

void b2CreateParticleWorld( b2World* world );
void b2DestroyParticleWorld( b2World* world );
void b2ClearParticleEvents( b2World* world );
void b2StepParticles( b2World* world, float timeStep );
void b2DrawParticles( b2World* world, b2DebugDraw* draw );
void b2GetParticleWorldCounters( b2World* world, b2Counters* counters );

b2ParticleSystem* b2GetParticleSystemFullId( b2World* world, b2ParticleSystemId systemId );
b2ParticleGroup* b2GetParticleGroupFullId( b2World* world, b2ParticleGroupId groupId );
