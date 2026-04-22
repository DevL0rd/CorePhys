// SPDX-FileCopyrightText: 2026 DevL0rd
// SPDX-License-Identifier: MIT

#include "TaskScheduler_c.h"
#include "particle_parallel_benchmarks.h"
#include "particle_scenarios.h"
#include "test_macros.h"

#include "corephys/corephys.h"
#include "corephys/particle.h"
#include "corephys/types.h"

#include <stdint.h>
#include <math.h>
#include <string.h>

enum
{
	e_parallelMaxTasks = 2048,
	e_parallelEventGridWidth = 12,
	e_parallelEventGridHeight = 12,
};

typedef struct ParticleParallelTaskData
{
	b2TaskCallback* corephysTask;
	void* corephysContext;
} ParticleParallelTaskData;

typedef struct ParticleParallelTasks
{
	enkiTaskScheduler* scheduler;
	enkiTaskSet* tasks[e_parallelMaxTasks];
	ParticleParallelTaskData taskData[e_parallelMaxTasks];
	int taskCount;
} ParticleParallelTasks;

typedef struct ParticleParallelScenarioResult
{
	uint32_t hash;
	b2ParticleSystemCounters counters;
} ParticleParallelScenarioResult;

typedef struct ParticleParallelBenchmarkCaseResult
{
	uint32_t hash;
	b2ParticleSystemCounters counters;
	int particleCount;
	int dynamicBodyCount;
} ParticleParallelBenchmarkCaseResult;

typedef struct ParticleParallelMultiSystemResult
{
	uint32_t hashA;
	uint32_t hashB;
	int taskCount;
	int particleCount;
} ParticleParallelMultiSystemResult;

typedef struct ParticleParallelEventResult
{
	int firstContactCount;
	int beginCount;
	int emptyContactCount;
	int endCount;
	uint32_t beginHash;
	uint32_t endHash;
} ParticleParallelEventResult;

typedef struct ParticleParallelImpulseResult
{
	uint32_t bodyHash;
	uint32_t particleHash;
	uint32_t eventHash;
	int beginTotal;
	int endTotal;
	int finalBodyContactCount;
} ParticleParallelImpulseResult;

static uint32_t HashWord( uint32_t hash, uint32_t value )
{
	hash ^= value;
	hash *= 16777619u;
	return hash;
}

static uint32_t HashFloat( uint32_t hash, float value )
{
	uint32_t bits = 0;
	memcpy( &bits, &value, sizeof( bits ) );
	return HashWord( hash, bits );
}

static uint32_t HashVec2( uint32_t hash, b2Vec2 value )
{
	hash = HashFloat( hash, value.x );
	hash = HashFloat( hash, value.y );
	return hash;
}

static uint32_t HashRot( uint32_t hash, b2Rot value )
{
	hash = HashFloat( hash, value.c );
	hash = HashFloat( hash, value.s );
	return hash;
}

static uint32_t HashShapeId( uint32_t hash, b2ShapeId shapeId )
{
	uint64_t storedId = b2StoreShapeId( shapeId );
	hash = HashWord( hash, (uint32_t)storedId );
	hash = HashWord( hash, (uint32_t)( storedId >> 32 ) );
	return hash;
}

static uint32_t HashBodyState( uint32_t hash, b2BodyId bodyId )
{
	hash = HashVec2( hash, b2Body_GetPosition( bodyId ) );
	hash = HashRot( hash, b2Body_GetRotation( bodyId ) );
	hash = HashVec2( hash, b2Body_GetLinearVelocity( bodyId ) );
	hash = HashFloat( hash, b2Body_GetAngularVelocity( bodyId ) );
	return hash;
}

static uint32_t HashParticleSystemState( b2ParticleSystemId systemId )
{
	uint32_t hash = 2166136261u;
	b2ParticleSystemCounters counters = b2ParticleSystem_GetCounters( systemId );
	hash = HashWord( hash, (uint32_t)counters.particleCount );
	hash = HashWord( hash, (uint32_t)counters.contactCount );
	hash = HashWord( hash, (uint32_t)counters.bodyContactCount );
	hash = HashWord( hash, (uint32_t)counters.pairCount );
	hash = HashWord( hash, (uint32_t)counters.triadCount );

	const b2Vec2* positions = b2ParticleSystem_GetPositionBuffer( systemId );
	const b2Vec2* velocities = b2ParticleSystem_GetVelocityBuffer( systemId );
	const uint32_t* flags = b2ParticleSystem_GetFlagsBuffer( systemId );
	for ( int i = 0; i < counters.particleCount; ++i )
	{
		hash = HashVec2( hash, positions[i] );
		hash = HashVec2( hash, velocities[i] );
		hash = HashWord( hash, flags[i] );
	}

	return hash;
}

static void ExecuteParticleParallelTask( uint32_t start, uint32_t end, uint32_t threadIndex, void* context )
{
	ParticleParallelTaskData* data = context;
	data->corephysTask( (int)start, (int)end, threadIndex, data->corephysContext );
}

static void* EnqueueParticleParallelTask( b2TaskCallback* corephysTask, int itemCount, int minRange, void* corephysContext,
										  void* userContext )
{
	ParticleParallelTasks* parallelTasks = userContext;

	if ( parallelTasks != NULL && parallelTasks->taskCount < e_parallelMaxTasks )
	{
		int taskIndex = parallelTasks->taskCount++;
		enkiTaskSet* task = parallelTasks->tasks[taskIndex];
		ParticleParallelTaskData* data = parallelTasks->taskData + taskIndex;
		data->corephysTask = corephysTask;
		data->corephysContext = corephysContext;

		struct enkiParamsTaskSet params;
		params.minRange = minRange;
		params.setSize = itemCount;
		params.pArgs = data;
		params.priority = 0;

		enkiSetParamsTaskSet( task, params );
		enkiAddTaskSet( parallelTasks->scheduler, task );
		return task;
	}

	corephysTask( 0, itemCount, 0, corephysContext );
	return NULL;
}

static void FinishParticleParallelTask( void* userTask, void* userContext )
{
	ParticleParallelTasks* parallelTasks = userContext;
	enkiTaskSet* task = userTask;
	enkiWaitForTaskSet( parallelTasks->scheduler, task );
}

static int CreateParallelTasks( ParticleParallelTasks* parallelTasks, int workerCount )
{
	*parallelTasks = (ParticleParallelTasks){ 0 };

	if ( workerCount <= 1 )
	{
		return 0;
	}

	parallelTasks->scheduler = enkiNewTaskScheduler();
	ENSURE( parallelTasks->scheduler != NULL );

	struct enkiTaskSchedulerConfig config = enkiGetTaskSchedulerConfig( parallelTasks->scheduler );
	config.numTaskThreadsToCreate = workerCount - 1;
	enkiInitTaskSchedulerWithConfig( parallelTasks->scheduler, config );

	for ( int i = 0; i < e_parallelMaxTasks; ++i )
	{
		parallelTasks->tasks[i] = enkiCreateTaskSet( parallelTasks->scheduler, ExecuteParticleParallelTask );
		ENSURE( parallelTasks->tasks[i] != NULL );
	}

	return 0;
}

static void DestroyParallelTasks( ParticleParallelTasks* parallelTasks )
{
	if ( parallelTasks->scheduler == NULL )
	{
		return;
	}

	for ( int i = 0; i < e_parallelMaxTasks; ++i )
	{
		if ( parallelTasks->tasks[i] != NULL )
		{
			enkiDeleteTaskSet( parallelTasks->scheduler, parallelTasks->tasks[i] );
		}
	}

	enkiDeleteTaskScheduler( parallelTasks->scheduler );
	*parallelTasks = (ParticleParallelTasks){ 0 };
}

static int CreateParticleParallelWorld( int workerCount, ParticleParallelTasks* parallelTasks, b2Vec2 gravity, b2WorldId* worldId )
{
	ENSURE( CreateParallelTasks( parallelTasks, workerCount ) == 0 );

	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = gravity;

	if ( workerCount > 1 )
	{
		worldDef.workerCount = workerCount;
		worldDef.enqueueTask = EnqueueParticleParallelTask;
		worldDef.finishTask = FinishParticleParallelTask;
		worldDef.userTaskContext = parallelTasks;
	}

	*worldId = b2CreateWorld( &worldDef );
	ENSURE( b2World_IsValid( *worldId ) );
	return 0;
}

static void StepParticleParallelWorld( b2WorldId worldId, ParticleParallelTasks* parallelTasks, ParticleScenario* scenario,
									   float timeStep )
{
	StepParticleScenario( worldId, scenario, timeStep );
	b2World_Step( worldId, timeStep, 4 );
	parallelTasks->taskCount = 0;
}

static void DestroyParticleParallelWorld( b2WorldId worldId, ParticleParallelTasks* parallelTasks )
{
	if ( b2World_IsValid( worldId ) )
	{
		b2DestroyWorld( worldId );
	}

	DestroyParallelTasks( parallelTasks );
}

static uint32_t HashParticleContactEvents( b2ParticleContactEvents events )
{
	uint32_t hash = 2166136261u;
	hash = HashWord( hash, (uint32_t)events.beginCount );
	hash = HashWord( hash, (uint32_t)events.endCount );

	for ( int i = 0; i < events.beginCount; ++i )
	{
		hash = HashWord( hash, (uint32_t)events.beginEvents[i].indexA );
		hash = HashWord( hash, (uint32_t)events.beginEvents[i].indexB );
	}

	for ( int i = 0; i < events.endCount; ++i )
	{
		hash = HashWord( hash, (uint32_t)events.endEvents[i].indexA );
		hash = HashWord( hash, (uint32_t)events.endEvents[i].indexB );
	}

	return hash;
}

static uint32_t HashParticleBodyContactEvents( b2ParticleBodyContactEvents events )
{
	uint32_t hash = 2166136261u;
	hash = HashWord( hash, (uint32_t)events.beginCount );
	hash = HashWord( hash, (uint32_t)events.endCount );

	for ( int i = 0; i < events.beginCount; ++i )
	{
		hash = HashWord( hash, (uint32_t)events.beginEvents[i].particleIndex );
		hash = HashShapeId( hash, events.beginEvents[i].shapeId );
	}

	for ( int i = 0; i < events.endCount; ++i )
	{
		hash = HashWord( hash, (uint32_t)events.endEvents[i].particleIndex );
		hash = HashShapeId( hash, events.endEvents[i].shapeId );
	}

	return hash;
}

static void MoveParticlesOutOfContact( b2ParticleSystemId systemId )
{
	int particleCount = b2ParticleSystem_GetParticleCount( systemId );
	b2Vec2* positions = b2ParticleSystem_GetMutablePositionBuffer( systemId );
	b2Vec2* velocities = b2ParticleSystem_GetMutableVelocityBuffer( systemId );

	for ( int i = 0; i < particleCount; ++i )
	{
		positions[i] = (b2Vec2){ 100.0f + 0.5f * (float)i, 100.0f };
		velocities[i] = b2Vec2_zero;
	}
}

static b2ParticleSystemId CreateStableEventParticleSystem( b2WorldId worldId, bool bodyEvents )
{
	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.08f;
	systemDef.gravityScale = 0.0f;
	systemDef.pressureStrength = 0.0f;
	systemDef.dampingStrength = 0.0f;
	systemDef.enableParticleEvents = bodyEvents == false;
	systemDef.enableParticleBodyEvents = bodyEvents;

	return b2CreateParticleSystem( worldId, &systemDef );
}

static int CreateParticleEventGrid( b2ParticleSystemId systemId, uint32_t particleFlags )
{
	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.flags = particleFlags;

	for ( int y = 0; y < e_parallelEventGridHeight; ++y )
	{
		for ( int x = 0; x < e_parallelEventGridWidth; ++x )
		{
			particleDef.position = (b2Vec2){ 0.075f * (float)x, 0.075f * (float)y };
			ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) >= 0 );
		}
	}

	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == e_parallelEventGridWidth * e_parallelEventGridHeight );
	return 0;
}

static b2BodyId CreateParticleImpulseBody( b2WorldId worldId, b2Vec2 position, b2Vec2 halfExtents )
{
	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = position;
	bodyDef.enableSleep = false;
	bodyDef.isAwake = true;
	b2BodyId bodyId = b2CreateBody( worldId, &bodyDef );

	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = 0.35f;
	b2Polygon box = b2MakeBox( halfExtents.x, halfExtents.y );
	b2CreatePolygonShape( bodyId, &shapeDef, &box );

	return bodyId;
}

static int CreateParticleImpulseStream( b2ParticleSystemId systemId )
{
	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.flags = b2_waterParticle;
	particleDef.velocity = (b2Vec2){ 9.0f, 0.0f };

	for ( int y = 0; y < 8; ++y )
	{
		for ( int x = 0; x < 10; ++x )
		{
			particleDef.position = (b2Vec2){ -0.85f + 0.055f * (float)x, -0.42f + 0.12f * (float)y };
			ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) >= 0 );
		}
	}

	return 0;
}

static int RunParticleBodyImpulseCase( int workerCount, ParticleParallelImpulseResult* result )
{
	ParticleParallelTasks parallelTasks = { 0 };
	b2WorldId worldId = b2_nullWorldId;
	ENSURE( CreateParticleParallelWorld( workerCount, &parallelTasks, b2Vec2_zero, &worldId ) == 0 );
	b2World_EnableSleeping( worldId, false );

	b2BodyId bodyA = CreateParticleImpulseBody( worldId, (b2Vec2){ 0.0f, -0.25f }, (b2Vec2){ 0.16f, 0.22f } );
	b2BodyId bodyB = CreateParticleImpulseBody( worldId, (b2Vec2){ 0.05f, 0.35f }, (b2Vec2){ 0.18f, 0.20f } );
	ENSURE( b2Body_IsValid( bodyA ) );
	ENSURE( b2Body_IsValid( bodyB ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.045f;
	systemDef.density = 1.5f;
	systemDef.gravityScale = 0.0f;
	systemDef.dampingStrength = 0.2f;
	systemDef.pressureStrength = 0.03f;
	systemDef.enableParticleBodyEvents = true;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );
	ENSURE( CreateParticleImpulseStream( systemId ) == 0 );

	*result = (ParticleParallelImpulseResult){ .eventHash = 2166136261u };
	float timeStep = 1.0f / 60.0f;
	for ( int i = 0; i < 18; ++i )
	{
		b2World_Step( worldId, timeStep, 4 );
		parallelTasks.taskCount = 0;

		b2ParticleBodyContactEvents events = b2ParticleSystem_GetBodyContactEvents( systemId );
		result->beginTotal += events.beginCount;
		result->endTotal += events.endCount;
		result->eventHash = HashWord( result->eventHash, (uint32_t)events.beginCount );
		result->eventHash = HashWord( result->eventHash, (uint32_t)events.endCount );
		result->eventHash = HashWord( result->eventHash, HashParticleBodyContactEvents( events ) );
	}

	b2Vec2 bodyAPosition = b2Body_GetPosition( bodyA );
	b2Vec2 bodyBPosition = b2Body_GetPosition( bodyB );
	ENSURE( bodyAPosition.x > 0.0f );
	ENSURE( bodyBPosition.x > 0.0f );
	ENSURE( result->beginTotal > 0 );

	result->finalBodyContactCount = b2ParticleSystem_GetBodyContactCount( systemId );
	result->particleHash = HashParticleSystemState( systemId );
	result->bodyHash = 2166136261u;
	result->bodyHash = HashBodyState( result->bodyHash, bodyA );
	result->bodyHash = HashBodyState( result->bodyHash, bodyB );

	DestroyParticleParallelWorld( worldId, &parallelTasks );
	return 0;
}

static int CreateMultiSystemParticles( b2ParticleSystemId systemId, float xOffset, uint32_t flags )
{
	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.flags = flags;
	particleDef.color = (b2ParticleColor){ 90, 180, 255, 255 };

	for ( int y = 0; y < 10; ++y )
	{
		for ( int x = 0; x < 10; ++x )
		{
			particleDef.position = (b2Vec2){ xOffset + 0.055f * (float)x, 0.055f * (float)y };
			particleDef.velocity = (b2Vec2){ 0.01f * (float)y, -0.01f * (float)x };
			ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) >= 0 );
		}
	}

	return 0;
}

static int RunParticleMultiSystemCase( int workerCount, ParticleParallelMultiSystemResult* result )
{
	ParticleParallelTasks parallelTasks = { 0 };
	b2WorldId worldId = b2_nullWorldId;
	ENSURE( CreateParticleParallelWorld( workerCount, &parallelTasks, b2Vec2_zero, &worldId ) == 0 );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.04f;
	systemDef.gravityScale = 0.0f;
	systemDef.dampingStrength = 0.1f;
	systemDef.pressureStrength = 0.02f;
	systemDef.destroyByAge = false;
	b2ParticleSystemId systemA = b2CreateParticleSystem( worldId, &systemDef );
	b2ParticleSystemId systemB = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemA ) );
	ENSURE( b2ParticleSystem_IsValid( systemB ) );
	ENSURE( CreateMultiSystemParticles( systemA, -1.0f, b2_waterParticle | b2_colorMixingParticle ) == 0 );
	ENSURE( CreateMultiSystemParticles( systemB, 1.0f, b2_viscousParticle | b2_colorMixingParticle ) == 0 );

	for ( int i = 0; i < 4; ++i )
	{
		b2World_Step( worldId, 1.0f / 60.0f, 4 );
		parallelTasks.taskCount = 0;
	}

	b2Counters counters = b2World_GetCounters( worldId );
	result->hashA = HashParticleSystemState( systemA );
	result->hashB = HashParticleSystemState( systemB );
	result->taskCount = counters.taskCount;
	result->particleCount = counters.particleCount;
	ENSURE( result->hashA != 0 );
	ENSURE( result->hashB != 0 );
	ENSURE( result->particleCount == 200 );

	DestroyParticleParallelWorld( worldId, &parallelTasks );
	return 0;
}

static int RunParticleReductionContentionCase( int workerCount, ParticleParallelScenarioResult* result )
{
	ParticleParallelTasks parallelTasks = { 0 };
	b2WorldId worldId = b2_nullWorldId;
	ENSURE( CreateParticleParallelWorld( workerCount, &parallelTasks, b2Vec2_zero, &worldId ) == 0 );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.08f;
	systemDef.gravityScale = 0.0f;
	systemDef.dampingStrength = 0.15f;
	systemDef.pressureStrength = 0.02f;
	systemDef.destroyByAge = false;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.flags = b2_waterParticle | b2_tensileParticle | b2_colorMixingParticle;
	for ( int i = 0; i < 96; ++i )
	{
		float angle = ( 2.0f * B2_PI * (float)i ) / 95.0f;
		float radius = 0.0015f * (float)( i % 9 );
		particleDef.position = (b2Vec2){ radius * cosf( angle ), radius * sinf( angle ) };
		particleDef.velocity = (b2Vec2){ 0.01f * sinf( angle ), -0.01f * cosf( angle ) };
		particleDef.color = (b2ParticleColor){ (uint8_t)( 80 + i ), (uint8_t)( 220 - i ), 180, 255 };
		ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) >= 0 );
	}

	for ( int i = 0; i < 3; ++i )
	{
		b2World_Step( worldId, 1.0f / 60.0f, 4 );
		parallelTasks.taskCount = 0;
	}

	result->hash = HashParticleSystemState( systemId );
	result->counters = b2ParticleSystem_GetCounters( systemId );
	ENSURE( result->hash != 0 );
	ENSURE( result->counters.contactCount > 300 );
	ENSURE( result->counters.reductionDeltaCount > result->counters.particleCount );

	DestroyParticleParallelWorld( worldId, &parallelTasks );
	return 0;
}

static int RunParticleEventDiffCase( int workerCount, ParticleParallelEventResult* result )
{
	ParticleParallelTasks parallelTasks = { 0 };
	b2WorldId worldId = b2_nullWorldId;
	ENSURE( CreateParticleParallelWorld( workerCount, &parallelTasks, b2Vec2_zero, &worldId ) == 0 );

	b2ParticleSystemId systemId = CreateStableEventParticleSystem( worldId, false );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );
	ENSURE( CreateParticleEventGrid( systemId, b2_waterParticle ) == 0 );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	parallelTasks.taskCount = 0;

	b2ParticleContactEvents events = b2ParticleSystem_GetContactEvents( systemId );
	*result = (ParticleParallelEventResult){ 0 };
	result->firstContactCount = b2ParticleSystem_GetContactCount( systemId );
	result->beginCount = events.beginCount;
	result->beginHash = HashParticleContactEvents( events );
	ENSURE( result->firstContactCount > b2ParticleSystem_GetParticleCount( systemId ) );
	ENSURE( result->beginCount == result->firstContactCount );
	ENSURE( events.endCount == 0 );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	parallelTasks.taskCount = 0;
	events = b2ParticleSystem_GetContactEvents( systemId );
	ENSURE( events.beginCount == 0 );
	ENSURE( events.endCount == 0 );

	MoveParticlesOutOfContact( systemId );
	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	parallelTasks.taskCount = 0;

	events = b2ParticleSystem_GetContactEvents( systemId );
	result->emptyContactCount = b2ParticleSystem_GetContactCount( systemId );
	result->endCount = events.endCount;
	result->endHash = HashParticleContactEvents( events );
	ENSURE( result->emptyContactCount == 0 );
	ENSURE( events.beginCount == 0 );
	ENSURE( result->endCount == result->firstContactCount );

	DestroyParticleParallelWorld( worldId, &parallelTasks );
	return 0;
}

static int RunParticleBodyEventDiffCase( int workerCount, ParticleParallelEventResult* result )
{
	ParticleParallelTasks parallelTasks = { 0 };
	b2WorldId worldId = b2_nullWorldId;
	ENSURE( CreateParticleParallelWorld( workerCount, &parallelTasks, b2Vec2_zero, &worldId ) == 0 );

	b2BodyDef bodyDef = b2DefaultBodyDef();
	b2BodyId bodyId = b2CreateBody( worldId, &bodyDef );
	ENSURE( b2Body_IsValid( bodyId ) );

	b2Polygon box = b2MakeBox( 2.0f, 2.0f );
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2ShapeId shapeId = b2CreatePolygonShape( bodyId, &shapeDef, &box );
	ENSURE( b2Shape_IsValid( shapeId ) );

	b2ParticleSystemId systemId = CreateStableEventParticleSystem( worldId, true );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );
	ENSURE( CreateParticleEventGrid( systemId, b2_waterParticle ) == 0 );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	parallelTasks.taskCount = 0;

	b2ParticleBodyContactEvents events = b2ParticleSystem_GetBodyContactEvents( systemId );
	*result = (ParticleParallelEventResult){ 0 };
	result->firstContactCount = b2ParticleSystem_GetBodyContactCount( systemId );
	result->beginCount = events.beginCount;
	result->beginHash = HashParticleBodyContactEvents( events );
	ENSURE( result->firstContactCount == b2ParticleSystem_GetParticleCount( systemId ) );
	ENSURE( result->beginCount == result->firstContactCount );
	ENSURE( events.endCount == 0 );
	ENSURE( B2_ID_EQUALS( events.beginEvents[0].shapeId, shapeId ) );

	MoveParticlesOutOfContact( systemId );
	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	parallelTasks.taskCount = 0;

	events = b2ParticleSystem_GetBodyContactEvents( systemId );
	result->emptyContactCount = b2ParticleSystem_GetBodyContactCount( systemId );
	result->endCount = events.endCount;
	result->endHash = HashParticleBodyContactEvents( events );
	ENSURE( result->emptyContactCount == 0 );
	ENSURE( events.beginCount == 0 );
	ENSURE( result->endCount == result->firstContactCount );

	DestroyParticleParallelWorld( worldId, &parallelTasks );
	return 0;
}

static int RunScenarioCase( ParticleScenarioType scenarioType, int workerCount, int stepCount,
							ParticleParallelScenarioResult* result )
{
	ParticleParallelTasks parallelTasks = { 0 };
	b2WorldId worldId = b2_nullWorldId;
	ENSURE( CreateParticleParallelWorld( workerCount, &parallelTasks, (b2Vec2){ 0.0f, -10.0f }, &worldId ) == 0 );

	ParticleScenario scenario = CreateParticleScenario( worldId, scenarioType );
	ENSURE( b2ParticleSystem_IsValid( scenario.systemId ) );

	float timeStep = 1.0f / 60.0f;
	for ( int i = 0; i < stepCount; ++i )
	{
		StepParticleParallelWorld( worldId, &parallelTasks, &scenario, timeStep );
	}

	result->hash = HashParticleScenario( &scenario );
	result->counters = b2ParticleSystem_GetCounters( scenario.systemId );
	ENSURE( result->hash != 0 );
	ENSURE( result->counters.particleCount > 0 );
	ENSURE( result->counters.contactCount > 0 );

	switch ( scenarioType )
	{
		case ParticleScenario_Barrier:
			ENSURE( result->counters.groupCount == 3 );
			ENSURE( result->counters.pairCount > 0 );
			break;

		case ParticleScenario_ElasticGroups:
			ENSURE( result->counters.groupCount == 3 );
			ENSURE( result->counters.pairCount > 0 );
			ENSURE( result->counters.triadCount > 0 );
			break;

		case ParticleScenario_RigidGroups:
			ENSURE( result->counters.groupCount == 3 );
			break;

		case ParticleScenario_WaveMachine:
			ENSURE( result->counters.bodyContactCount > 0 );
			break;

		default:
			ENSURE( false );
			break;
	}

	DestroyParticleParallelWorld( worldId, &parallelTasks );
	return 0;
}

static int RunBenchmarkCase( ParticleParallelBenchmarkType benchmarkType, int workerCount, int stepCount,
							 ParticleParallelBenchmarkCaseResult* result )
{
	ParticleParallelTasks parallelTasks = { 0 };
	b2WorldId worldId = b2_nullWorldId;
	ENSURE( CreateParticleParallelWorld( workerCount, &parallelTasks, (b2Vec2){ 0.0f, -10.0f }, &worldId ) == 0 );

	ParticleParallelBenchmark benchmark = CreateParticleParallelBenchmark( worldId, benchmarkType );
	ENSURE( b2ParticleSystem_IsValid( benchmark.systemId ) );

	for ( int i = 0; i < stepCount; ++i )
	{
		StepParticleParallelBenchmark( worldId, &benchmark, 1.0f / 60.0f, 4 );
		parallelTasks.taskCount = 0;
	}

	result->hash = HashParticleParallelBenchmark( &benchmark );
	result->counters = b2ParticleSystem_GetCounters( benchmark.systemId );
	result->particleCount = result->counters.particleCount;
	result->dynamicBodyCount = benchmark.dynamicBodyCount;
	ENSURE( result->hash != 0 );
	ENSURE( result->particleCount > 0 );
	ENSURE( result->counters.contactCount > 0 );

	switch ( benchmarkType )
	{
		case ParticleParallelBenchmark_Faucet:
			ENSURE( result->particleCount <= b2ParticleSystem_GetMaxParticleCount( benchmark.systemId ) );
			break;

		case ParticleParallelBenchmark_WaveMachine:
			ENSURE( B2_IS_NON_NULL( benchmark.waveJointId ) );
			ENSURE( b2Joint_IsValid( benchmark.waveJointId ) );
			break;

		case ParticleParallelBenchmark_ParticleBodyCollision:
			ENSURE( result->dynamicBodyCount > 0 );
			ENSURE( result->counters.bodyContactCount > 0 );
			break;

		case ParticleParallelBenchmark_Powder:
			ENSURE( result->counters.groupCount == 2 );
			break;

		case ParticleParallelBenchmark_ViscousFluid:
			ENSURE( result->counters.groupCount == 2 );
			break;

		case ParticleParallelBenchmark_Barrier:
		case ParticleParallelBenchmark_BarrierContacts:
			ENSURE( result->counters.pairCount > 0 );
			break;

		case ParticleParallelBenchmark_SolidGroups:
		case ParticleParallelBenchmark_SolidGroupRefresh:
			ENSURE( result->counters.groupCount >= 3 );
			break;

		case ParticleParallelBenchmark_RigidGroups:
		case ParticleParallelBenchmark_RigidGroupRefresh:
			ENSURE( result->counters.groupCount >= 3 );
			break;

		case ParticleParallelBenchmark_ElasticGroups:
			ENSURE( result->counters.pairCount > 0 );
			ENSURE( result->counters.triadCount > 0 );
			break;

		case ParticleParallelBenchmark_LargeDamBreak:
		case ParticleParallelBenchmark_SpatialIndexContacts:
			break;

		case ParticleParallelBenchmark_Count:
		default:
			ENSURE( false );
			break;
	}

	DestroyParticleParallelWorld( worldId, &parallelTasks );
	return 0;
}

static int ImpulseResultsEqual( const ParticleParallelImpulseResult* a, const ParticleParallelImpulseResult* b )
{
	ENSURE( a->bodyHash == b->bodyHash );
	ENSURE( a->particleHash == b->particleHash );
	ENSURE( a->eventHash == b->eventHash );
	ENSURE( a->beginTotal == b->beginTotal );
	ENSURE( a->endTotal == b->endTotal );
	ENSURE( a->finalBodyContactCount == b->finalBodyContactCount );
	return 0;
}

static int EventResultsEqual( const ParticleParallelEventResult* a, const ParticleParallelEventResult* b )
{
	ENSURE( a->firstContactCount == b->firstContactCount );
	ENSURE( a->beginCount == b->beginCount );
	ENSURE( a->emptyContactCount == b->emptyContactCount );
	ENSURE( a->endCount == b->endCount );
	ENSURE( a->beginHash == b->beginHash );
	ENSURE( a->endHash == b->endHash );
	return 0;
}

static int ScenarioResultsEqual( const ParticleParallelScenarioResult* a, const ParticleParallelScenarioResult* b )
{
	ENSURE( a->hash == b->hash );
	ENSURE( a->counters.particleCount == b->counters.particleCount );
	ENSURE( a->counters.groupCount == b->counters.groupCount );
	ENSURE( a->counters.contactCount == b->counters.contactCount );
	ENSURE( a->counters.bodyContactCount == b->counters.bodyContactCount );
	ENSURE( a->counters.pairCount == b->counters.pairCount );
	ENSURE( a->counters.triadCount == b->counters.triadCount );
	return 0;
}

static int BenchmarkCaseResultsEqual( const ParticleParallelBenchmarkCaseResult* a,
									  const ParticleParallelBenchmarkCaseResult* b )
{
	ENSURE( a->hash == b->hash );
	ENSURE( a->particleCount == b->particleCount );
	ENSURE( a->dynamicBodyCount == b->dynamicBodyCount );
	ENSURE( a->counters.groupCount == b->counters.groupCount );
	ENSURE( a->counters.contactCount == b->counters.contactCount );
	ENSURE( a->counters.bodyContactCount == b->counters.bodyContactCount );
	ENSURE( a->counters.pairCount == b->counters.pairCount );
	ENSURE( a->counters.triadCount == b->counters.triadCount );
	return 0;
}

static int ParticleParallelScenarioDeterminism( void )
{
	static const int workerCounts[] = { 1, 3 };
	static const ParticleScenarioType scenarioTypes[] = {
		ParticleScenario_WaveMachine,
		ParticleScenario_Barrier,
		ParticleScenario_ElasticGroups,
		ParticleScenario_RigidGroups,
	};

	for ( int scenarioIndex = 0; scenarioIndex < ARRAY_COUNT( scenarioTypes ); ++scenarioIndex )
	{
		ParticleParallelScenarioResult reference = { 0 };

		for ( int workerIndex = 0; workerIndex < ARRAY_COUNT( workerCounts ); ++workerIndex )
		{
			ParticleParallelScenarioResult result = { 0 };
			ENSURE( RunScenarioCase( scenarioTypes[scenarioIndex], workerCounts[workerIndex], 6, &result ) == 0 );

			if ( workerIndex == 0 )
			{
				reference = result;
			}
			else
			{
				ENSURE( ScenarioResultsEqual( &reference, &result ) == 0 );
			}
		}
	}

	return 0;
}

static int ParticleParallelEventDiffs( void )
{
	static const int workerCounts[] = { 1, 3 };

	ParticleParallelEventResult referenceParticleEvents = { 0 };
	ParticleParallelEventResult referenceBodyEvents = { 0 };

	for ( int workerIndex = 0; workerIndex < ARRAY_COUNT( workerCounts ); ++workerIndex )
	{
		ParticleParallelEventResult particleEvents = { 0 };
		ENSURE( RunParticleEventDiffCase( workerCounts[workerIndex], &particleEvents ) == 0 );

		ParticleParallelEventResult bodyEvents = { 0 };
		ENSURE( RunParticleBodyEventDiffCase( workerCounts[workerIndex], &bodyEvents ) == 0 );

		if ( workerIndex == 0 )
		{
			referenceParticleEvents = particleEvents;
			referenceBodyEvents = bodyEvents;
		}
		else
		{
			ENSURE( EventResultsEqual( &referenceParticleEvents, &particleEvents ) == 0 );
			ENSURE( EventResultsEqual( &referenceBodyEvents, &bodyEvents ) == 0 );
		}
	}

	return 0;
}

static int ParticleParallelBodyImpulseDeterminism( void )
{
	ParticleParallelImpulseResult reference = { 0 };
	ParticleParallelImpulseResult parallel = { 0 };

	ENSURE( RunParticleBodyImpulseCase( 1, &reference ) == 0 );
	ENSURE( RunParticleBodyImpulseCase( 3, &parallel ) == 0 );
	ENSURE( ImpulseResultsEqual( &reference, &parallel ) == 0 );

	return 0;
}

static int ParticleParallelMultiSystemScheduling( void )
{
	ParticleParallelMultiSystemResult reference = { 0 };
	ParticleParallelMultiSystemResult parallel = { 0 };

	ENSURE( RunParticleMultiSystemCase( 1, &reference ) == 0 );
	ENSURE( RunParticleMultiSystemCase( 3, &parallel ) == 0 );
	ENSURE( reference.hashA == parallel.hashA );
	ENSURE( reference.hashB == parallel.hashB );
	ENSURE( reference.particleCount == parallel.particleCount );
	ENSURE( parallel.taskCount > reference.taskCount );

	return 0;
}

static int ParticleParallelReductionContention( void )
{
	ParticleParallelScenarioResult reference = { 0 };
	ParticleParallelScenarioResult parallel = { 0 };

	ENSURE( RunParticleReductionContentionCase( 1, &reference ) == 0 );
	ENSURE( RunParticleReductionContentionCase( 3, &parallel ) == 0 );
	ENSURE( ScenarioResultsEqual( &reference, &parallel ) == 0 );

	return 0;
}

static int ParticleParallelLargeBenchmarkCoverage( void )
{
	static const ParticleParallelBenchmarkType benchmarkTypes[] = {
		ParticleParallelBenchmark_LargeDamBreak,
		ParticleParallelBenchmark_Faucet,
		ParticleParallelBenchmark_WaveMachine,
		ParticleParallelBenchmark_Powder,
		ParticleParallelBenchmark_ViscousFluid,
		ParticleParallelBenchmark_ElasticGroups,
		ParticleParallelBenchmark_RigidGroups,
		ParticleParallelBenchmark_SolidGroups,
		ParticleParallelBenchmark_Barrier,
	};

	for ( int benchmarkIndex = 0; benchmarkIndex < ARRAY_COUNT( benchmarkTypes ); ++benchmarkIndex )
	{
		ParticleParallelBenchmarkCaseResult result = { 0 };
		ENSURE( RunBenchmarkCase( benchmarkTypes[benchmarkIndex], 1, 1, &result ) == 0 );
		ENSURE( result.particleCount >= 400 );
	}

	return 0;
}

static int ParticleParallelAdvancedBenchmarkDeterminism( void )
{
	static const int workerCounts[] = { 1, 3 };
	static const ParticleParallelBenchmarkType benchmarkTypes[] = {
		ParticleParallelBenchmark_Faucet,
		ParticleParallelBenchmark_ParticleBodyCollision,
		ParticleParallelBenchmark_Powder,
		ParticleParallelBenchmark_ViscousFluid,
		ParticleParallelBenchmark_ElasticGroups,
		ParticleParallelBenchmark_SolidGroupRefresh,
		ParticleParallelBenchmark_RigidGroupRefresh,
	};

	for ( int benchmarkIndex = 0; benchmarkIndex < ARRAY_COUNT( benchmarkTypes ); ++benchmarkIndex )
	{
		ParticleParallelBenchmarkCaseResult reference = { 0 };

		for ( int workerIndex = 0; workerIndex < ARRAY_COUNT( workerCounts ); ++workerIndex )
		{
			ParticleParallelBenchmarkCaseResult result = { 0 };
			ENSURE( RunBenchmarkCase( benchmarkTypes[benchmarkIndex], workerCounts[workerIndex], 1, &result ) == 0 );

			if ( workerIndex == 0 )
			{
				reference = result;
			}
			else
			{
				ENSURE( BenchmarkCaseResultsEqual( &reference, &result ) == 0 );
			}
		}
	}

	return 0;
}

int ParticleParallelTest( void )
{
	RUN_SUBTEST( ParticleParallelScenarioDeterminism );
	RUN_SUBTEST( ParticleParallelEventDiffs );
	RUN_SUBTEST( ParticleParallelBodyImpulseDeterminism );
	RUN_SUBTEST( ParticleParallelMultiSystemScheduling );
	RUN_SUBTEST( ParticleParallelReductionContention );
	RUN_SUBTEST( ParticleParallelLargeBenchmarkCoverage );
	RUN_SUBTEST( ParticleParallelAdvancedBenchmarkDeterminism );

	return 0;
}
