// SPDX-FileCopyrightText: 2026 DevL0rd
// SPDX-License-Identifier: MIT

#include "particle_system.h"

#include "body.h"
#include "core.h"
#include "particle_sort.h"
#include "physics_world.h"
#include "shape.h"
#include "solver_set.h"

#include "corephys/constants.h"
#include "corephys/corephys.h"

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

#define B2_PARTICLE_STRIDE 0.75f
#define B2_MIN_PARTICLE_WEIGHT 1.0f
#define B2_MAX_PARTICLE_PRESSURE 0.25f
#define B2_MAX_PARTICLE_FORCE 0.5f
#define B2_MAX_TRIAD_DISTANCE 2.0f
#define B2_BARRIER_COLLISION_TIME 2.5f
#define B2_PARTICLE_TASK_MIN_RANGE 256

static const b2ParticleColor b2_particleColorZero = { 0, 0, 0, 0 };

typedef enum b2ParticleRangeTaskType
{
	b2_particleTaskBuildProxies,
	b2_particleTaskSolveLifetimes,
	b2_particleTaskClearWeights,
	b2_particleTaskClearAccumulations,
	b2_particleTaskClearStaticPressures,
	b2_particleTaskClearDepths,
	b2_particleTaskClearAccumulationVectors,
	b2_particleTaskSolveForces,
	b2_particleTaskSolveGravity,
	b2_particleTaskPressureAccumulation,
	b2_particleTaskStaticPressureUpdate,
	b2_particleTaskLimitVelocities,
	b2_particleTaskSolveBarrierWalls,
	b2_particleTaskSolveWalls,
	b2_particleTaskIntegrate
} b2ParticleRangeTaskType;

typedef struct b2ParticleRangeTaskContext
{
	b2ParticleSystem* system;
	b2ParticleRangeTaskType type;
	float dt;
	b2Vec2 vector;
	float scalar;
} b2ParticleRangeTaskContext;

typedef struct b2ParticleContactBlock
{
	b2ParticleContactData* contacts;
	int count;
	int capacity;
	int candidateCount;
	b2ParticleScratchArray* payload;
} b2ParticleContactBlock;

typedef struct b2ParticleCell
{
	int cellX;
	int cellY;
	uint64_t key;
	int firstParticle;
	int lastParticle;
	int count;
} b2ParticleCell;

typedef struct b2ParticleCellGrid
{
	b2ParticleCell* cells;
	int cellCount;
	int cellCapacity;
	int* table;
	int tableCapacity;
	int* nextParticles;
	int nextParticleCapacity;
	bool usesScratch;
} b2ParticleCellGrid;

typedef enum b2ParticleCellBuildTaskType
{
	b2_particleCellBuildTaskBoundaries,
	b2_particleCellBuildTaskScatter,
} b2ParticleCellBuildTaskType;

typedef struct b2ParticleCellBuildTaskContext
{
	b2ParticleSystem* system;
	b2ParticleCellBuildTaskType type;
	b2ParticleCellGrid* grid;
	int* cellStartFlags;
	int* proxyCellIndices;
} b2ParticleCellBuildTaskContext;

typedef struct b2ParticleBodyContactBlock
{
	b2ParticleBodyContactData* contacts;
	int count;
	int capacity;
	int candidateCount;
	b2ParticleScratchArray* payload;
} b2ParticleBodyContactBlock;

typedef struct b2ParticleBodyImpulseData
{
	b2ShapeId shapeId;
	b2Vec2 impulse;
	b2Vec2 point;
} b2ParticleBodyImpulseData;

typedef struct b2ParticleBodyImpulseBlock
{
	b2ParticleBodyImpulseData* impulses;
	int count;
	int capacity;
	int candidateCount;
	b2ParticleScratchArray* payload;
} b2ParticleBodyImpulseBlock;

typedef struct b2ParticleBodyImpulseSum
{
	b2Vec2 linearImpulse;
	float angularImpulse;
	bool touched;
} b2ParticleBodyImpulseSum;

typedef struct b2ParticleShapeCandidateBlock
{
	int* shapeIndices;
	int count;
	int capacity;
	b2ParticleScratchArray* payload;
} b2ParticleShapeCandidateBlock;

typedef struct b2ParticleFloatDeltaData
{
	int index;
	float delta;
} b2ParticleFloatDeltaData;

typedef struct b2ParticleFloatDeltaBlock
{
	b2ParticleFloatDeltaData* deltas;
	int count;
	int capacity;
	b2ParticleScratchArray* payload;
} b2ParticleFloatDeltaBlock;

typedef struct b2ParticleVec2DeltaData
{
	int index;
	b2Vec2 delta;
} b2ParticleVec2DeltaData;

typedef struct b2ParticleVec2DeltaBlock
{
	b2ParticleVec2DeltaData* deltas;
	int count;
	int capacity;
	b2ParticleScratchArray* payload;
} b2ParticleVec2DeltaBlock;

typedef struct b2ParticleContactTaskContext
{
	b2ParticleSystem* system;
	b2ParticleContactBlock* blocks;
	b2ParticleProxy* proxies;
	int proxyCount;
	int blockSize;
	float diameter;
	float invDiameter;
	float diameterSquared;
} b2ParticleContactTaskContext;

typedef struct b2ParticleBodyContactTaskContext
{
	b2World* world;
	b2ParticleSystem* system;
	b2ParticleBodyContactBlock* blocks;
	b2ParticleShapeCandidateBlock* shapeCandidateBlocks;
	int blockSize;
	float diameter;
	float invDiameter;
	float particleInvMass;
} b2ParticleBodyContactTaskContext;

typedef struct b2ParticleBodyCollisionTaskContext
{
	b2World* world;
	b2ParticleSystem* system;
	b2ParticleBodyImpulseBlock* blocks;
	b2ParticleShapeCandidateBlock* shapeCandidateBlocks;
	int blockSize;
	float dt;
	float invDt;
	float particleMass;
} b2ParticleBodyCollisionTaskContext;

typedef enum b2ParticleContactSolveTaskType
{
	b2_particleContactTaskWeights,
	b2_particleContactTaskRepulsive,
	b2_particleContactTaskPowder,
	b2_particleContactTaskViscous,
	b2_particleContactTaskTensileAccumulation,
	b2_particleContactTaskTensileVelocity,
	b2_particleContactTaskSolidDepthWeights,
	b2_particleContactTaskSolidDepthRelaxation,
	b2_particleContactTaskSolidEjection,
	b2_particleContactTaskStaticPressure,
	b2_particleContactTaskPressure,
	b2_particleContactTaskDamping
} b2ParticleContactSolveTaskType;

typedef struct b2ParticleContactSolveTaskContext
{
	b2World* world;
	b2ParticleSystem* system;
	b2ParticleContactSolveTaskType type;
	b2ParticleFloatDeltaBlock* floatBlocks;
	b2ParticleVec2DeltaBlock* vec2Blocks;
	b2ParticleBodyImpulseBlock* bodyImpulseBlocks;
	int contactBlockCount;
	int bodyContactBlockCount;
	int blockSize;
	int deltaPartitionCount;
	int deltaPartitionSize;
	int iteration;
	float dt;
	float scalarA;
	float scalarB;
	float scalarC;
	float scalarD;
	float* sourceDepths;
} b2ParticleContactSolveTaskContext;

typedef enum b2ParticleSpringSolveTaskType
{
	b2_particleSpringTaskPair,
	b2_particleSpringTaskTriad
} b2ParticleSpringSolveTaskType;

typedef struct b2ParticleSpringSolveTaskContext
{
	b2ParticleSystem* system;
	b2ParticleSpringSolveTaskType type;
	b2ParticleVec2DeltaBlock* vec2Blocks;
	int blockSize;
	int deltaPartitionCount;
	int deltaPartitionSize;
	float dt;
	float invDt;
	float strength;
} b2ParticleSpringSolveTaskContext;

typedef enum b2ParticleDeltaApplyTaskType
{
	b2_particleDeltaTaskFloat,
	b2_particleDeltaTaskFloatMin,
	b2_particleDeltaTaskVec2
} b2ParticleDeltaApplyTaskType;

typedef struct b2ParticleDeltaApplyTaskContext
{
	b2ParticleDeltaApplyTaskType type;
	void* blocks;
	int sourceBlockCount;
	int partitionCount;
	float* floatValues;
	b2Vec2* vec2Values;
} b2ParticleDeltaApplyTaskContext;

typedef struct b2ParticleBodyContactQueryContext
{
	b2World* world;
	b2ParticleSystem* system;
	b2ParticleBodyContactBlock* block;
	b2ParticleSystemId systemId;
	b2AABB aabb;
	int particleIndex;
	float diameter;
	float invDiameter;
	float particleInvMass;
} b2ParticleBodyContactQueryContext;

typedef struct b2ParticleBodyCollisionQueryContext
{
	b2World* world;
	b2ParticleSystem* system;
	b2ParticleBodyImpulseBlock* block;
	b2ParticleSystemId systemId;
	b2AABB aabb;
	int particleIndex;
	b2Vec2 position;
	b2Vec2 translation;
	float dt;
	float invDt;
	float particleMass;
	float minFraction;
	b2CastOutput bestOutput;
	b2ShapeId bestShapeId;
} b2ParticleBodyCollisionQueryContext;

typedef struct b2ParticleShapeCandidateQueryContext
{
	b2World* world;
	b2ParticleShapeCandidateBlock* block;
	b2AABB aabb;
} b2ParticleShapeCandidateQueryContext;

typedef struct b2ParticleRigidGroupTaskContext
{
	b2ParticleSystem* system;
	int groupLocalIndex;
	b2Transform velocityTransform;
} b2ParticleRigidGroupTaskContext;

typedef struct b2ParticleGroupAccumulation
{
	int firstIndex;
	int count;
	float mass;
	float inertia;
	b2Vec2 center;
	b2Vec2 linearVelocity;
	float angularVelocity;
} b2ParticleGroupAccumulation;

typedef enum b2ParticleGroupRefreshTaskType
{
	b2_particleGroupRefreshMass,
	b2_particleGroupRefreshAngular
} b2ParticleGroupRefreshTaskType;

typedef struct b2ParticleGroupRefreshTaskContext
{
	b2ParticleSystem* system;
	b2ParticleGroupAccumulation* accumulations;
	b2ParticleGroupRefreshTaskType type;
	int blockSize;
	int groupCount;
	float particleMass;
} b2ParticleGroupRefreshTaskContext;

typedef enum b2ParticleCompactionTaskType
{
	b2_particleCompactionTaskMark,
	b2_particleCompactionTaskFinalize,
	b2_particleCompactionTaskScatter,
	b2_particleCompactionTaskPublish
} b2ParticleCompactionTaskType;

typedef struct b2ParticleCoreBuffers
{
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
} b2ParticleCoreBuffers;

typedef struct b2ParticleCompactionTaskContext
{
	b2ParticleSystem* system;
	b2ParticleCompactionTaskType type;
	b2ParticleCoreBuffers buffers;
	int oldCount;
	int blockSize;
	int* newIndices;
	const int* blockOffsets;
	int* blockLiveCounts;
	int* blockMoveCounts;
} b2ParticleCompactionTaskContext;

typedef enum b2ParticleRemapBufferType
{
	b2_particleRemapProxies,
	b2_particleRemapContacts,
	b2_particleRemapBodyContacts,
	b2_particleRemapPairs,
	b2_particleRemapTriads
} b2ParticleRemapBufferType;

typedef struct b2ParticleRemapTaskContext
{
	b2ParticleRemapBufferType type;
	const int* newIndices;
	int oldCount;
	int itemCount;
	int blockSize;
	int* blockCounts;
	const int* blockOffsets;
	const void* input;
	void* output;
	bool scatter;
} b2ParticleRemapTaskContext;

typedef enum b2ParticleSolidDepthTaskType
{
	b2_particleSolidDepthTaskInitialize,
	b2_particleSolidDepthTaskCopy,
	b2_particleSolidDepthTaskScale
} b2ParticleSolidDepthTaskType;

typedef struct b2ParticleSolidDepthTaskContext
{
	b2ParticleSystem* system;
	b2ParticleSolidDepthTaskType type;
	float* sourceDepths;
	float* targetDepths;
	float diameter;
} b2ParticleSolidDepthTaskContext;

static void b2ResetParticleStepCounters( b2ParticleSystem* system )
{
	system->taskCount = 0;
	system->taskRangeCount = 0;
	system->taskRangeMin = 0;
	system->taskRangeMax = 0;
	system->spatialCellCount = 0;
	system->occupiedCellCount = 0;
	system->spatialProxyCount = 0;
	system->spatialScatterCount = 0;
	system->contactCandidateCount = 0;
	system->bodyShapeCandidateCount = 0;
	system->barrierCandidateCount = 0;
	system->reductionDeltaCount = 0;
	system->reductionApplyCount = 0;
	system->groupRefreshCount = 0;
	system->compactionMoveCount = 0;
	system->compactionRemapCount = 0;
}

static void b2RecordParticleTaskStats( b2ParticleSystem* system, int itemCount, int minRange )
{
	if ( system == NULL || itemCount <= 0 )
	{
		return;
	}

	int rangeSize = b2MaxInt( 1, minRange );
	int rangeCount = ( itemCount + rangeSize - 1 ) / rangeSize;
	system->taskCount += 1;
	system->taskRangeCount += rangeCount;
	system->taskRangeMin = system->taskRangeMin == 0 ? rangeSize : b2MinInt( system->taskRangeMin, rangeSize );
	system->taskRangeMax = b2MaxInt( system->taskRangeMax, b2MinInt( itemCount, rangeSize ) );
}

static void b2RunParticleTask( b2World* world, b2ParticleSystem* system, b2TaskCallback* task, int itemCount, int minRange,
							   void* taskContext )
{
	if ( task == NULL || itemCount <= 0 )
	{
		return;
	}

	b2RecordParticleTaskStats( system, itemCount, minRange );

	if ( world == NULL || world->particleSystemTaskActive || world->workerCount <= 1 || itemCount <= 1 || itemCount < minRange )
	{
		task( 0, itemCount, 0, taskContext );
		return;
	}

	void* userTask = world->enqueueTaskFcn( task, itemCount, minRange, taskContext, world->userTaskContext );
	if ( userTask != NULL )
	{
		world->taskCount += 1;
		world->activeTaskCount += 1;
		world->finishTaskFcn( userTask, world->userTaskContext );
		world->activeTaskCount -= 1;
	}
}

static void b2RunParticleReductionTask( b2World* world, b2ParticleSystem* system, b2TaskCallback* task, int itemCount,
									   int minRange, void* taskContext )
{
	uint64_t ticks = b2GetTicks();
	b2RunParticleTask( world, system, task, itemCount, minRange, taskContext );
	system->profile.reductionGenerate += b2GetMilliseconds( ticks );
}

static void b2ParticleRangeTask( int startIndex, int endIndex, uint32_t workerIndex, void* taskContext )
{
	B2_UNUSED( workerIndex );

	b2ParticleRangeTaskContext* context = taskContext;
	b2ParticleSystem* system = context->system;

	switch ( context->type )
	{
		case b2_particleTaskBuildProxies:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				int cellX = (int)floorf( system->positions[i].x * context->scalar );
				int cellY = (int)floorf( system->positions[i].y * context->scalar );
				uint64_t ux = (uint32_t)( cellX + INT32_MIN );
				uint64_t uy = (uint32_t)( cellY + INT32_MIN );
				system->proxies[i] = (b2ParticleProxy){ i, cellX, cellY, ( uy << 32 ) | ux };
			}
			break;

		case b2_particleTaskSolveLifetimes:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				if ( system->lifetimes[i] <= 0.0f )
				{
					continue;
				}

				system->ages[i] += context->dt;
				if ( system->ages[i] >= system->lifetimes[i] )
				{
					system->flags[i] |= b2_zombieParticle;
				}
			}
			break;

		case b2_particleTaskClearWeights:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				system->weights[i] = 0.0f;
			}
			break;

		case b2_particleTaskClearAccumulations:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				system->accumulations[i] = 0.0f;
			}
			break;

		case b2_particleTaskClearStaticPressures:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				system->staticPressures[i] = 0.0f;
			}
			break;

		case b2_particleTaskClearDepths:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				system->depths[i] = 0.0f;
			}
			break;

		case b2_particleTaskClearAccumulationVectors:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				system->accumulationVectors[i] = b2Vec2_zero;
			}
			break;

		case b2_particleTaskSolveForces:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				if ( b2LengthSquared( system->forces[i] ) > 0.0f )
				{
					system->velocities[i] = b2MulAdd( system->velocities[i], context->scalar, system->forces[i] );
					system->forces[i] = b2Vec2_zero;
				}
			}
			break;

		case b2_particleTaskSolveGravity:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				system->velocities[i] = b2Add( system->velocities[i], context->vector );
			}
			break;

		case b2_particleTaskPressureAccumulation:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				uint32_t flags = system->flags[i];
				float pressure = ( flags & ( b2_powderParticle | b2_tensileParticle ) ) != 0
									 ? 0.0f
									 : b2MinFloat( context->dt * b2MaxFloat( 0.0f, system->weights[i] - B2_MIN_PARTICLE_WEIGHT ),
												   context->scalar );
				if ( ( flags & b2_staticPressureParticle ) != 0 )
				{
					pressure += system->staticPressures[i];
				}
				system->accumulations[i] = pressure;
			}
			break;

		case b2_particleTaskStaticPressureUpdate:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				if ( ( system->flags[i] & b2_staticPressureParticle ) == 0 )
				{
					system->staticPressures[i] = 0.0f;
					continue;
				}

				float denominator = system->weights[i] + context->scalar;
				float pressure = denominator > 0.0f
									 ? ( system->accumulations[i] + context->dt * ( system->weights[i] - B2_MIN_PARTICLE_WEIGHT ) ) /
										   denominator
									 : 0.0f;
				system->staticPressures[i] = b2ClampFloat( pressure, 0.0f, context->vector.x );
			}
			break;

		case b2_particleTaskLimitVelocities:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				float velocitySquared = b2LengthSquared( system->velocities[i] );
				if ( velocitySquared > context->scalar )
				{
					system->velocities[i] = b2MulSV( sqrtf( context->scalar / velocitySquared ), system->velocities[i] );
				}
			}
			break;

		case b2_particleTaskSolveBarrierWalls:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				if ( ( system->flags[i] & ( b2_barrierParticle | b2_wallParticle ) ) ==
					 ( b2_barrierParticle | b2_wallParticle ) )
				{
					system->velocities[i] = b2Vec2_zero;
				}
			}
			break;

		case b2_particleTaskSolveWalls:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				if ( ( system->flags[i] & b2_wallParticle ) != 0 )
				{
					system->velocities[i] = b2Vec2_zero;
				}
			}
			break;

		case b2_particleTaskIntegrate:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				system->positions[i] = b2MulAdd( system->positions[i], context->dt, system->velocities[i] );
			}
			break;
	}
}

static void b2RunParticleRangeTask( b2World* world, b2ParticleRangeTaskContext* context, int itemCount )
{
	b2RunParticleTask( world, context->system, b2ParticleRangeTask, itemCount, B2_PARTICLE_TASK_MIN_RANGE, context );
}

static int b2MaxParticleCapacity( int oldCapacity, int minCapacity )
{
	int newCapacity = oldCapacity < 8 ? 8 : oldCapacity + ( oldCapacity >> 1 );
	return b2MaxInt( newCapacity, minCapacity );
}

static void* b2GrowParticleBuffer( void* buffer, int oldCapacity, int newCapacity, int elementSize )
{
	return b2GrowAlloc( buffer, oldCapacity * elementSize, newCapacity * elementSize );
}

static void b2ReserveParticles( b2ParticleSystem* system, int capacity )
{
	if ( capacity <= system->particleCapacity )
	{
		return;
	}

	int oldCapacity = system->particleCapacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	system->positions = b2GrowParticleBuffer( system->positions, oldCapacity, newCapacity, sizeof( b2Vec2 ) );
	system->velocities = b2GrowParticleBuffer( system->velocities, oldCapacity, newCapacity, sizeof( b2Vec2 ) );
	system->forces = b2GrowParticleBuffer( system->forces, oldCapacity, newCapacity, sizeof( b2Vec2 ) );
	system->flags = b2GrowParticleBuffer( system->flags, oldCapacity, newCapacity, sizeof( uint32_t ) );
	system->colors = b2GrowParticleBuffer( system->colors, oldCapacity, newCapacity, sizeof( b2ParticleColor ) );
	system->userDataBuffer = b2GrowParticleBuffer( system->userDataBuffer, oldCapacity, newCapacity, sizeof( void* ) );
	system->weights = b2GrowParticleBuffer( system->weights, oldCapacity, newCapacity, sizeof( float ) );
	system->lifetimes = b2GrowParticleBuffer( system->lifetimes, oldCapacity, newCapacity, sizeof( float ) );
	system->ages = b2GrowParticleBuffer( system->ages, oldCapacity, newCapacity, sizeof( float ) );
	system->accumulations = b2GrowParticleBuffer( system->accumulations, oldCapacity, newCapacity, sizeof( float ) );
	system->staticPressures = b2GrowParticleBuffer( system->staticPressures, oldCapacity, newCapacity, sizeof( float ) );
	system->depths = b2GrowParticleBuffer( system->depths, oldCapacity, newCapacity, sizeof( float ) );
	system->accumulationVectors = b2GrowParticleBuffer( system->accumulationVectors, oldCapacity, newCapacity, sizeof( b2Vec2 ) );
	system->groupIndices = b2GrowParticleBuffer( system->groupIndices, oldCapacity, newCapacity, sizeof( int ) );
	system->particleCapacity = newCapacity;
}

static void b2ReserveProxies( b2ParticleSystem* system, int capacity )
{
	if ( capacity <= system->proxyCapacity )
	{
		return;
	}

	int oldCapacity = system->proxyCapacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	system->proxies = b2GrowParticleBuffer( system->proxies, oldCapacity, newCapacity, sizeof( b2ParticleProxy ) );
	system->proxyCapacity = newCapacity;
}

static void b2ReserveContacts( b2ParticleSystem* system, int capacity )
{
	if ( capacity <= system->contactCapacity )
	{
		return;
	}

	int oldCapacity = system->contactCapacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	system->contacts = b2GrowParticleBuffer( system->contacts, oldCapacity, newCapacity, sizeof( b2ParticleContactData ) );
	system->contactCapacity = newCapacity;
}

static void b2ReserveContactData( b2ParticleContactData** buffer, int* bufferCapacity, int capacity )
{
	if ( capacity <= *bufferCapacity )
	{
		return;
	}

	int oldCapacity = *bufferCapacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	*buffer = b2GrowParticleBuffer( *buffer, oldCapacity, newCapacity, sizeof( b2ParticleContactData ) );
	*bufferCapacity = newCapacity;
}

static void b2ReserveParticleContactBlock( b2ParticleContactBlock* block, int capacity )
{
	if ( block == NULL || capacity <= block->capacity )
	{
		return;
	}

	int oldCapacity = block->capacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	if ( block->payload != NULL )
	{
		b2ParticleScratchArray_Reserve( block->payload, newCapacity );
		block->contacts = b2ParticleScratchArray_Data( block->payload, b2ParticleContactData );
		newCapacity = block->payload->capacity;
	}
	else
	{
		block->contacts = b2GrowParticleBuffer( block->contacts, oldCapacity, newCapacity, sizeof( b2ParticleContactData ) );
	}
	block->capacity = newCapacity;
}

static void b2AppendParticleContactBlock( b2ParticleContactBlock* block, b2ParticleContactData contact )
{
	b2ReserveParticleContactBlock( block, block->count + 1 );
	block->contacts[block->count++] = contact;
}

static void b2DestroyParticleCellGrid( b2ParticleCellGrid* grid )
{
	if ( grid == NULL )
	{
		return;
	}

	if ( grid->usesScratch == false )
	{
		b2Free( grid->cells, grid->cellCapacity * (int)sizeof( b2ParticleCell ) );
		b2Free( grid->table, grid->tableCapacity * (int)sizeof( int ) );
		b2Free( grid->nextParticles, grid->nextParticleCapacity * (int)sizeof( int ) );
	}
	*grid = (b2ParticleCellGrid){ 0 };
}

static void b2ReserveParticlePayloadStorage( b2ParticlePayloadStorage* storage, int count, int elementSize )
{
	B2_ASSERT( storage != NULL );
	B2_ASSERT( count >= 0 );
	B2_ASSERT( elementSize > 0 );
	B2_ASSERT( storage->elementSize == 0 || storage->elementSize == elementSize );

	if ( storage->elementSize == 0 )
	{
		storage->elementSize = elementSize;
	}

	if ( count <= storage->capacity )
	{
		storage->count = b2MaxInt( storage->count, count );
		return;
	}

	int oldCapacity = storage->capacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, count );
	storage->arrays = b2GrowParticleBuffer( storage->arrays, oldCapacity, newCapacity, sizeof( b2ParticleScratchArray ) );
	for ( int i = oldCapacity; i < newCapacity; ++i )
	{
		b2ParticleScratchArray_Init( storage->arrays + i, elementSize );
	}

	storage->capacity = newCapacity;
	storage->count = count;
}

static void b2DestroyParticlePayloadStorage( b2ParticlePayloadStorage* storage )
{
	if ( storage == NULL )
	{
		return;
	}

	for ( int i = 0; i < storage->capacity; ++i )
	{
		b2ParticleScratchArray_Destroy( storage->arrays + i );
	}

	b2Free( storage->arrays, storage->capacity * (int)sizeof( b2ParticleScratchArray ) );
	*storage = (b2ParticlePayloadStorage){ 0 };
}

static int b2GetParticlePayloadStorageByteCount( const b2ParticlePayloadStorage* storage )
{
	if ( storage == NULL )
	{
		return 0;
	}

	int byteCount = storage->capacity * (int)sizeof( b2ParticleScratchArray );
	for ( int i = 0; i < storage->capacity; ++i )
	{
		byteCount += storage->arrays[i].capacity * storage->arrays[i].elementSize;
	}

	return byteCount;
}

static b2ParticleScratchArray* b2GetParticlePayloadArray( b2ParticlePayloadStorage* storage, int index, int elementSize )
{
	b2ReserveParticlePayloadStorage( storage, index + 1, elementSize );
	return storage->arrays + index;
}

static void b2AttachParticleContactPayloads( b2ParticleSystem* system, b2ParticleContactBlock* blocks, int blockCount )
{
	b2ReserveParticlePayloadStorage( &system->contactBlockPayloads, blockCount, (int)sizeof( b2ParticleContactData ) );
	for ( int i = 0; i < blockCount; ++i )
	{
		b2ParticleScratchArray* payload = system->contactBlockPayloads.arrays + i;
		blocks[i].payload = payload;
		blocks[i].contacts = b2ParticleScratchArray_Data( payload, b2ParticleContactData );
		blocks[i].capacity = payload->capacity;
	}
}

static void b2AttachParticleShapeCandidatePayloads( b2ParticleSystem* system, b2ParticleShapeCandidateBlock* blocks,
												   int blockCount )
{
	b2ReserveParticlePayloadStorage( &system->shapeCandidatePayloads, blockCount, (int)sizeof( int ) );
	for ( int i = 0; i < blockCount; ++i )
	{
		b2ParticleScratchArray* payload = system->shapeCandidatePayloads.arrays + i;
		blocks[i].payload = payload;
		blocks[i].shapeIndices = b2ParticleScratchArray_Data( payload, int );
		blocks[i].capacity = payload->capacity;
	}
}

static void b2AttachParticleBodyContactPayloads( b2ParticleSystem* system, b2ParticleBodyContactBlock* blocks, int blockCount )
{
	b2ReserveParticlePayloadStorage( &system->bodyContactBlockPayloads, blockCount, (int)sizeof( b2ParticleBodyContactData ) );
	for ( int i = 0; i < blockCount; ++i )
	{
		b2ParticleScratchArray* payload = system->bodyContactBlockPayloads.arrays + i;
		blocks[i].payload = payload;
		blocks[i].contacts = b2ParticleScratchArray_Data( payload, b2ParticleBodyContactData );
		blocks[i].capacity = payload->capacity;
	}
}

static void b2AttachParticleBodyImpulsePayloads( b2ParticleSystem* system, b2ParticleBodyImpulseBlock* blocks, int blockCount )
{
	b2ReserveParticlePayloadStorage( &system->bodyImpulseBlockPayloads, blockCount, (int)sizeof( b2ParticleBodyImpulseData ) );
	for ( int i = 0; i < blockCount; ++i )
	{
		b2ParticleScratchArray* payload = system->bodyImpulseBlockPayloads.arrays + i;
		blocks[i].payload = payload;
		blocks[i].impulses = b2ParticleScratchArray_Data( payload, b2ParticleBodyImpulseData );
		blocks[i].capacity = payload->capacity;
	}
}

static void b2AttachParticleFloatDeltaPayloads( b2ParticleSystem* system, b2ParticleFloatDeltaBlock* blocks, int blockCount )
{
	b2ReserveParticlePayloadStorage( &system->floatDeltaBlockPayloads, blockCount, (int)sizeof( b2ParticleFloatDeltaData ) );
	for ( int i = 0; i < blockCount; ++i )
	{
		b2ParticleScratchArray* payload = system->floatDeltaBlockPayloads.arrays + i;
		blocks[i].payload = payload;
		blocks[i].deltas = b2ParticleScratchArray_Data( payload, b2ParticleFloatDeltaData );
		blocks[i].capacity = payload->capacity;
	}
}

static void b2AttachParticleVec2DeltaPayloads( b2ParticleSystem* system, b2ParticleVec2DeltaBlock* blocks, int blockCount )
{
	b2ReserveParticlePayloadStorage( &system->vec2DeltaBlockPayloads, blockCount, (int)sizeof( b2ParticleVec2DeltaData ) );
	for ( int i = 0; i < blockCount; ++i )
	{
		b2ParticleScratchArray* payload = system->vec2DeltaBlockPayloads.arrays + i;
		blocks[i].payload = payload;
		blocks[i].deltas = b2ParticleScratchArray_Data( payload, b2ParticleVec2DeltaData );
		blocks[i].capacity = payload->capacity;
	}
}

static int b2NextPowerOfTwoInt( int value )
{
	int result = 1;
	while ( result < value && result < ( 1 << 30 ) )
	{
		result <<= 1;
	}
	return result;
}

static uint64_t b2GetParticleCellKey( int cellX, int cellY )
{
	uint64_t ux = (uint32_t)( cellX + INT32_MIN );
	uint64_t uy = (uint32_t)( cellY + INT32_MIN );
	return ( uy << 32 ) | ux;
}

static uint32_t b2HashParticleCellKey( uint64_t key )
{
	key ^= key >> 33;
	key *= UINT64_C( 0xff51afd7ed558ccd );
	key ^= key >> 33;
	key *= UINT64_C( 0xc4ceb9fe1a85ec53 );
	key ^= key >> 33;
	return (uint32_t)key;
}

static int b2FindParticleCell( const b2ParticleCellGrid* grid, uint64_t key )
{
	if ( grid == NULL || grid->tableCapacity == 0 )
	{
		return B2_NULL_INDEX;
	}

	int mask = grid->tableCapacity - 1;
	int slot = (int)( b2HashParticleCellKey( key ) & (uint32_t)mask );
	for (;;)
	{
		int cellIndex = grid->table[slot];
		if ( cellIndex == B2_NULL_INDEX )
		{
			return B2_NULL_INDEX;
		}

		if ( grid->cells[cellIndex].key == key )
		{
			return cellIndex;
		}

		slot = ( slot + 1 ) & mask;
	}
}

static void b2InsertParticleCellIntoTable( b2ParticleCellGrid* grid, int cellIndex )
{
	uint64_t key = grid->cells[cellIndex].key;
	int mask = grid->tableCapacity - 1;
	int slot = (int)( b2HashParticleCellKey( key ) & (uint32_t)mask );
	while ( grid->table[slot] != B2_NULL_INDEX )
	{
		slot = ( slot + 1 ) & mask;
	}

	grid->table[slot] = cellIndex;
}

static int b2AddParticleScratchReserveBytes( int byteCount, int count, size_t elementSize )
{
	if ( count <= 0 )
	{
		return byteCount;
	}

	int itemBytes = b2ParticleScratchByteCount( count, elementSize );
	if ( byteCount > INT32_MAX - itemBytes - 64 )
	{
		return INT32_MAX;
	}

	return byteCount + itemBytes + 64;
}

static int b2GetParticleCellGridScratchByteCount( int particleCount, int extraByteCount )
{
	if ( particleCount <= 0 )
	{
		return 0;
	}

	int byteCount = 0;
	byteCount = b2AddParticleScratchReserveBytes( byteCount, particleCount, sizeof( b2ParticleProxy ) );
	byteCount = b2AddParticleScratchReserveBytes( byteCount, particleCount, sizeof( int ) );
	byteCount = b2AddParticleScratchReserveBytes( byteCount, particleCount, sizeof( int ) );
	byteCount = b2AddParticleScratchReserveBytes( byteCount, particleCount, sizeof( b2ParticleCell ) );
	byteCount = b2AddParticleScratchReserveBytes(
		byteCount, b2NextPowerOfTwoInt( b2MaxInt( 16, 2 * particleCount ) ), sizeof( int ) );
	byteCount = b2AddParticleScratchReserveBytes( byteCount, particleCount, sizeof( int ) );
	if ( byteCount > INT32_MAX - extraByteCount - 256 )
	{
		return INT32_MAX;
	}

	return byteCount + extraByteCount + 256;
}

static void b2ReserveParticleCellGridScratch( b2ParticleSystem* system, int particleCount, int extraByteCount )
{
	uint64_t ticks = b2GetTicks();
	b2ParticleScratchBuffer_Reserve( &system->scratch, b2GetParticleCellGridScratchByteCount( particleCount, extraByteCount ) );
	system->profile.scratch += b2GetMilliseconds( ticks );
}

static int b2GetParticleProxySortScratchByteCount( int particleCount, int extraByteCount )
{
	if ( particleCount <= 1 )
	{
		return extraByteCount + 256;
	}

	int byteCount = b2ParticleScratchByteCount(
		b2GetParticleSortScratchByteCount( particleCount, (int)sizeof( b2ParticleProxy ) ), 1 );
	if ( byteCount > INT32_MAX - extraByteCount - 256 )
	{
		return INT32_MAX;
	}

	return byteCount + extraByteCount + 256;
}

static void b2ReserveParticleProxySortScratch( b2ParticleSystem* system, int particleCount, int extraByteCount )
{
	uint64_t ticks = b2GetTicks();
	b2ParticleScratchBuffer_Reserve( &system->scratch, b2GetParticleProxySortScratchByteCount( particleCount, extraByteCount ) );
	system->profile.scratch += b2GetMilliseconds( ticks );
}

static void b2SortParticleProxies( b2ParticleSystem* system )
{
	uint64_t ticks = b2GetTicks();
	int count = system->proxyCount;
	b2ParticleScratchBuffer_Reset( &system->scratch );
	void* sortScratch = count > 1
							? b2ParticleScratchBuffer_Alloc( &system->scratch,
															 b2GetParticleSortScratchByteCount( count, (int)sizeof( b2ParticleProxy ) ),
															 (int)_Alignof( b2ParticleProxy ) )
							: NULL;
	b2SortParticleRecordsByKeyWithScratch(
		system->proxies, sortScratch, count, (int)sizeof( b2ParticleProxy ), (int)offsetof( b2ParticleProxy, key ) );

	float milliseconds = b2GetMilliseconds( ticks );
	system->profile.scratch += milliseconds;
	system->profile.spatialIndexBuild += milliseconds;
}

static void b2ParticleCellBuildTask( int startIndex, int endIndex, uint32_t workerIndex, void* taskContext )
{
	B2_UNUSED( workerIndex );

	b2ParticleCellBuildTaskContext* context = taskContext;
	b2ParticleSystem* system = context->system;
	b2ParticleCellGrid* grid = context->grid;

	switch ( context->type )
	{
		case b2_particleCellBuildTaskBoundaries:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				context->cellStartFlags[i] =
					i == 0 || system->proxies[i].key != system->proxies[i - 1].key ? 1 : 0;
			}
			break;

		case b2_particleCellBuildTaskScatter:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				b2ParticleProxy* proxy = system->proxies + i;
				int cellIndex = context->proxyCellIndices[i];
				bool isCellStart = context->cellStartFlags[i] != 0;
				bool isCellEnd = i + 1 == system->proxyCount || context->cellStartFlags[i + 1] != 0;

				grid->nextParticles[proxy->index] = isCellEnd ? B2_NULL_INDEX : system->proxies[i + 1].index;
				if ( isCellStart )
				{
					int endIndexInCell = i + 1;
					while ( endIndexInCell < system->proxyCount && context->cellStartFlags[endIndexInCell] == 0 )
					{
						endIndexInCell += 1;
					}

					grid->cells[cellIndex] = (b2ParticleCell){
						.cellX = proxy->cellX,
						.cellY = proxy->cellY,
						.key = proxy->key,
						.firstParticle = proxy->index,
						.lastParticle = system->proxies[endIndexInCell - 1].index,
						.count = endIndexInCell - i,
					};
				}
			}
			break;
	}
}

static void b2BuildParticleCellGrid( b2ParticleSystem* system, b2ParticleCellGrid* grid )
{
	int count = system->particleCount;
	if ( count == 0 )
	{
		return;
	}

	uint64_t buildTicks = b2GetTicks();
	b2ParticleScratchBuffer_Reset( &system->scratch );
	void* sortScratch = count > 1
							? b2ParticleScratchBuffer_Alloc( &system->scratch,
															 b2GetParticleSortScratchByteCount( count, (int)sizeof( b2ParticleProxy ) ),
															 (int)_Alignof( b2ParticleProxy ) )
							: NULL;
	b2SortParticleRecordsByKeyWithScratch(
		system->proxies, sortScratch, system->proxyCount, (int)sizeof( b2ParticleProxy ),
		(int)offsetof( b2ParticleProxy, key ) );
	int* cellStartFlags = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, int );
	int* proxyCellIndices = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, int );
	grid->cellCapacity = count;
	grid->cells = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, b2ParticleCell );
	memset( grid->cells, 0, count * sizeof( b2ParticleCell ) );
	grid->tableCapacity = b2NextPowerOfTwoInt( b2MaxInt( 16, 2 * count ) );
	grid->table = b2ParticleScratchBuffer_AllocArray( &system->scratch, grid->tableCapacity, int );
	for ( int i = 0; i < grid->tableCapacity; ++i )
	{
		grid->table[i] = B2_NULL_INDEX;
	}

	grid->nextParticleCapacity = count;
	grid->nextParticles = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, int );
	for ( int i = 0; i < count; ++i )
	{
		grid->nextParticles[i] = B2_NULL_INDEX;
	}
	grid->usesScratch = true;
	b2ParticleCellBuildTaskContext boundaryContext = {
		.system = system,
		.type = b2_particleCellBuildTaskBoundaries,
		.grid = grid,
		.cellStartFlags = cellStartFlags,
		.proxyCellIndices = proxyCellIndices,
	};
	b2World* world = b2TryGetWorld( system->worldId );
	b2RunParticleTask( world, system, b2ParticleCellBuildTask, system->proxyCount, B2_PARTICLE_TASK_MIN_RANGE, &boundaryContext );

	int cellCount = 0;
	for ( int i = 0; i < system->proxyCount; ++i )
	{
		if ( cellStartFlags[i] != 0 )
		{
			cellCount += 1;
		}
		proxyCellIndices[i] = cellCount - 1;
	}
	grid->cellCount = cellCount;
	float buildMilliseconds = b2GetMilliseconds( buildTicks );
	system->profile.scratch += buildMilliseconds;
	system->profile.spatialIndexBuild += buildMilliseconds;

	uint64_t scatterTicks = b2GetTicks();
	b2ParticleCellBuildTaskContext scatterContext = {
		.system = system,
		.type = b2_particleCellBuildTaskScatter,
		.grid = grid,
		.cellStartFlags = cellStartFlags,
		.proxyCellIndices = proxyCellIndices,
	};
	b2RunParticleTask( world, system, b2ParticleCellBuildTask, system->proxyCount, B2_PARTICLE_TASK_MIN_RANGE, &scatterContext );
	for ( int cellIndex = 0; cellIndex < grid->cellCount; ++cellIndex )
	{
		b2InsertParticleCellIntoTable( grid, cellIndex );
	}
	system->profile.spatialIndexScatter += b2GetMilliseconds( scatterTicks );

	system->spatialCellCount += grid->tableCapacity;
	system->occupiedCellCount += grid->cellCount;
	system->spatialProxyCount += system->proxyCount;
	system->spatialScatterCount += system->proxyCount;
}

static void b2ReserveBodyContacts( b2ParticleSystem* system, int capacity )
{
	if ( capacity <= system->bodyContactCapacity )
	{
		return;
	}

	int oldCapacity = system->bodyContactCapacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	system->bodyContacts = b2GrowParticleBuffer( system->bodyContacts, oldCapacity, newCapacity, sizeof( b2ParticleBodyContactData ) );
	system->bodyContactCapacity = newCapacity;
}

static void b2ReserveBodyContactData( b2ParticleBodyContactData** buffer, int* bufferCapacity, int capacity )
{
	if ( capacity <= *bufferCapacity )
	{
		return;
	}

	int oldCapacity = *bufferCapacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	*buffer = b2GrowParticleBuffer( *buffer, oldCapacity, newCapacity, sizeof( b2ParticleBodyContactData ) );
	*bufferCapacity = newCapacity;
}

static void b2ReserveParticleBodyContactBlock( b2ParticleBodyContactBlock* block, int capacity )
{
	if ( block == NULL || capacity <= block->capacity )
	{
		return;
	}

	int oldCapacity = block->capacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	if ( block->payload != NULL )
	{
		b2ParticleScratchArray_Reserve( block->payload, newCapacity );
		block->contacts = b2ParticleScratchArray_Data( block->payload, b2ParticleBodyContactData );
		newCapacity = block->payload->capacity;
	}
	else
	{
		block->contacts = b2GrowParticleBuffer( block->contacts, oldCapacity, newCapacity, sizeof( b2ParticleBodyContactData ) );
	}
	block->capacity = newCapacity;
}

static void b2AppendParticleBodyContactBlock( b2ParticleBodyContactBlock* block, b2ParticleBodyContactData contact )
{
	b2ReserveParticleBodyContactBlock( block, block->count + 1 );
	block->contacts[block->count++] = contact;
}

static void b2ReserveParticleBodyImpulseBlock( b2ParticleBodyImpulseBlock* block, int capacity )
{
	if ( block == NULL || capacity <= block->capacity )
	{
		return;
	}

	int oldCapacity = block->capacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	if ( block->payload != NULL )
	{
		b2ParticleScratchArray_Reserve( block->payload, newCapacity );
		block->impulses = b2ParticleScratchArray_Data( block->payload, b2ParticleBodyImpulseData );
		newCapacity = block->payload->capacity;
	}
	else
	{
		block->impulses = b2GrowParticleBuffer( block->impulses, oldCapacity, newCapacity, sizeof( b2ParticleBodyImpulseData ) );
	}
	block->capacity = newCapacity;
}

static void b2AppendParticleBodyImpulseBlock( b2ParticleBodyImpulseBlock* block, b2ParticleBodyImpulseData impulse )
{
	b2ReserveParticleBodyImpulseBlock( block, block->count + 1 );
	block->impulses[block->count++] = impulse;
}

static void b2ReserveParticleShapeCandidateBlock( b2ParticleShapeCandidateBlock* block, int capacity )
{
	if ( block == NULL || capacity <= block->capacity )
	{
		return;
	}

	int oldCapacity = block->capacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	if ( block->payload != NULL )
	{
		b2ParticleScratchArray_Reserve( block->payload, newCapacity );
		block->shapeIndices = b2ParticleScratchArray_Data( block->payload, int );
		newCapacity = block->payload->capacity;
	}
	else
	{
		block->shapeIndices = b2GrowParticleBuffer( block->shapeIndices, oldCapacity, newCapacity, sizeof( int ) );
	}
	block->capacity = newCapacity;
}

static void b2AppendParticleShapeCandidateBlock( b2ParticleShapeCandidateBlock* block, int shapeIndex )
{
	for ( int i = 0; i < block->count; ++i )
	{
		if ( block->shapeIndices[i] == shapeIndex )
		{
			return;
		}
	}

	b2ReserveParticleShapeCandidateBlock( block, block->count + 1 );
	block->shapeIndices[block->count++] = shapeIndex;
}

static void b2FreeParticleShapeCandidateBlock( b2ParticleShapeCandidateBlock* block )
{
	if ( block == NULL )
	{
		return;
	}

	if ( block->payload == NULL )
	{
		b2Free( block->shapeIndices, block->capacity * (int)sizeof( int ) );
	}
	*block = (b2ParticleShapeCandidateBlock){ 0 };
}

static void b2ReserveParticleFloatDeltaBlock( b2ParticleFloatDeltaBlock* block, int capacity )
{
	if ( block == NULL || capacity <= block->capacity )
	{
		return;
	}

	int oldCapacity = block->capacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	if ( block->payload != NULL )
	{
		b2ParticleScratchArray_Reserve( block->payload, newCapacity );
		block->deltas = b2ParticleScratchArray_Data( block->payload, b2ParticleFloatDeltaData );
		newCapacity = block->payload->capacity;
	}
	else
	{
		block->deltas = b2GrowParticleBuffer( block->deltas, oldCapacity, newCapacity, sizeof( b2ParticleFloatDeltaData ) );
	}
	block->capacity = newCapacity;
}

static void b2AppendParticleFloatDeltaBlock( b2ParticleFloatDeltaBlock* block, int index, float delta )
{
	b2ReserveParticleFloatDeltaBlock( block, block->count + 1 );
	block->deltas[block->count++] = (b2ParticleFloatDeltaData){ index, delta };
}

static void b2ReserveParticleVec2DeltaBlock( b2ParticleVec2DeltaBlock* block, int capacity )
{
	if ( block == NULL || capacity <= block->capacity )
	{
		return;
	}

	int oldCapacity = block->capacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	if ( block->payload != NULL )
	{
		b2ParticleScratchArray_Reserve( block->payload, newCapacity );
		block->deltas = b2ParticleScratchArray_Data( block->payload, b2ParticleVec2DeltaData );
		newCapacity = block->payload->capacity;
	}
	else
	{
		block->deltas = b2GrowParticleBuffer( block->deltas, oldCapacity, newCapacity, sizeof( b2ParticleVec2DeltaData ) );
	}
	block->capacity = newCapacity;
}

static void b2AppendParticleVec2DeltaBlock( b2ParticleVec2DeltaBlock* block, int index, b2Vec2 delta )
{
	b2ReserveParticleVec2DeltaBlock( block, block->count + 1 );
	block->deltas[block->count++] = (b2ParticleVec2DeltaData){ index, delta };
}

static b2ParticleFloatDeltaBlock* b2AllocateParticleFloatDeltaBlocks( b2ParticleSystem* system, int count, bool resetScratch )
{
	uint64_t ticks = b2GetTicks();
	if ( resetScratch )
	{
		b2ParticleScratchBuffer_Reset( &system->scratch );
	}

	b2ParticleFloatDeltaBlock* blocks = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, b2ParticleFloatDeltaBlock );
	memset( blocks, 0, count * sizeof( b2ParticleFloatDeltaBlock ) );
	b2AttachParticleFloatDeltaPayloads( system, blocks, count );
	system->profile.scratch += b2GetMilliseconds( ticks );
	return blocks;
}

static b2ParticleVec2DeltaBlock* b2AllocateParticleVec2DeltaBlocks( b2ParticleSystem* system, int count, bool resetScratch )
{
	uint64_t ticks = b2GetTicks();
	if ( resetScratch )
	{
		b2ParticleScratchBuffer_Reset( &system->scratch );
	}

	b2ParticleVec2DeltaBlock* blocks = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, b2ParticleVec2DeltaBlock );
	memset( blocks, 0, count * sizeof( b2ParticleVec2DeltaBlock ) );
	b2AttachParticleVec2DeltaPayloads( system, blocks, count );
	system->profile.scratch += b2GetMilliseconds( ticks );
	return blocks;
}

static b2ParticleBodyImpulseBlock* b2AllocateParticleBodyImpulseBlocks( b2ParticleSystem* system, int count, bool resetScratch )
{
	uint64_t ticks = b2GetTicks();
	if ( resetScratch )
	{
		b2ParticleScratchBuffer_Reset( &system->scratch );
	}

	b2ParticleBodyImpulseBlock* blocks = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, b2ParticleBodyImpulseBlock );
	memset( blocks, 0, count * sizeof( b2ParticleBodyImpulseBlock ) );
	b2AttachParticleBodyImpulsePayloads( system, blocks, count );
	system->profile.scratch += b2GetMilliseconds( ticks );
	return blocks;
}

static int b2GetParticleBlockCount( int itemCount )
{
	return itemCount > 0 ? ( itemCount + B2_PARTICLE_TASK_MIN_RANGE - 1 ) / B2_PARTICLE_TASK_MIN_RANGE : 0;
}

static int b2AddParticleScratchBytes( int byteCount, int count, size_t elementSize )
{
	if ( count <= 0 )
	{
		return byteCount;
	}

	int elementBytes = b2ParticleScratchByteCount( count, elementSize );
	int paddedBytes = elementBytes + 64;
	if ( byteCount > INT32_MAX - paddedBytes )
	{
		return INT32_MAX;
	}

	return byteCount + paddedBytes;
}

static int b2GetParticleCoreBufferScratchByteCount( int count )
{
	int byteCount = 0;
	byteCount = b2AddParticleScratchBytes( byteCount, count, sizeof( b2Vec2 ) );
	byteCount = b2AddParticleScratchBytes( byteCount, count, sizeof( b2Vec2 ) );
	byteCount = b2AddParticleScratchBytes( byteCount, count, sizeof( b2Vec2 ) );
	byteCount = b2AddParticleScratchBytes( byteCount, count, sizeof( uint32_t ) );
	byteCount = b2AddParticleScratchBytes( byteCount, count, sizeof( b2ParticleColor ) );
	byteCount = b2AddParticleScratchBytes( byteCount, count, sizeof( void* ) );
	byteCount = b2AddParticleScratchBytes( byteCount, count, sizeof( float ) );
	byteCount = b2AddParticleScratchBytes( byteCount, count, sizeof( float ) );
	byteCount = b2AddParticleScratchBytes( byteCount, count, sizeof( float ) );
	byteCount = b2AddParticleScratchBytes( byteCount, count, sizeof( float ) );
	byteCount = b2AddParticleScratchBytes( byteCount, count, sizeof( float ) );
	byteCount = b2AddParticleScratchBytes( byteCount, count, sizeof( float ) );
	byteCount = b2AddParticleScratchBytes( byteCount, count, sizeof( b2Vec2 ) );
	byteCount = b2AddParticleScratchBytes( byteCount, count, sizeof( int ) );
	return byteCount;
}

static int b2GetParticleMaxSideRemapCount( const b2ParticleSystem* system )
{
	int count = system->proxyCount;
	count = b2MaxInt( count, system->contactCount );
	count = b2MaxInt( count, system->previousContactCount );
	count = b2MaxInt( count, system->contactBeginCount );
	count = b2MaxInt( count, system->contactEndCount );
	count = b2MaxInt( count, system->bodyContactCount );
	count = b2MaxInt( count, system->previousBodyContactCount );
	count = b2MaxInt( count, system->bodyContactBeginCount );
	count = b2MaxInt( count, system->bodyContactEndCount );
	count = b2MaxInt( count, system->pairCount );
	count = b2MaxInt( count, system->triadCount );
	return count;
}

static int b2GetParticleSideRemapScratchByteCount( int maxItemCount )
{
	int blockCount = b2GetParticleBlockCount( maxItemCount );
	int maxElementSize = (int)sizeof( b2ParticleProxy );
	maxElementSize = b2MaxInt( maxElementSize, (int)sizeof( b2ParticleContactData ) );
	maxElementSize = b2MaxInt( maxElementSize, (int)sizeof( b2ParticleBodyContactData ) );
	maxElementSize = b2MaxInt( maxElementSize, (int)sizeof( b2ParticlePairData ) );
	maxElementSize = b2MaxInt( maxElementSize, (int)sizeof( b2ParticleTriadData ) );

	int byteCount = 0;
	byteCount = b2AddParticleScratchBytes( byteCount, blockCount, sizeof( int ) );
	byteCount = b2AddParticleScratchBytes( byteCount, blockCount, sizeof( int ) );
	byteCount = b2AddParticleScratchBytes( byteCount, maxItemCount, (size_t)maxElementSize );
	return byteCount;
}

static int b2GetParticleCompactionScratchByteCount( const b2ParticleSystem* system, int oldCount, int blockCount )
{
	int byteCount = 0;
	byteCount = b2AddParticleScratchBytes( byteCount, oldCount, sizeof( int ) );

	int coreWorkingBytes = 0;
	coreWorkingBytes = b2AddParticleScratchBytes( coreWorkingBytes, blockCount, sizeof( int ) );
	coreWorkingBytes = b2AddParticleScratchBytes( coreWorkingBytes, blockCount, sizeof( int ) );
	coreWorkingBytes = b2AddParticleScratchBytes( coreWorkingBytes, blockCount, sizeof( int ) );
	int coreBufferBytes = b2GetParticleCoreBufferScratchByteCount( oldCount );
	coreWorkingBytes =
		coreWorkingBytes > INT32_MAX - coreBufferBytes ? INT32_MAX : coreWorkingBytes + coreBufferBytes;

	int sideWorkingBytes = b2GetParticleSideRemapScratchByteCount( b2GetParticleMaxSideRemapCount( system ) );
	int workingBytes = b2MaxInt( coreWorkingBytes, sideWorkingBytes );
	if ( byteCount > INT32_MAX - workingBytes || byteCount + workingBytes > INT32_MAX - 256 )
	{
		return INT32_MAX;
	}

	byteCount += workingBytes;
	return byteCount + 256;
}

static int b2BuildParticleBlockOffsets( const int* blockCounts, int* blockOffsets, int blockCount )
{
	int totalCount = 0;
	for ( int i = 0; i < blockCount; ++i )
	{
		blockOffsets[i] = totalCount;
		totalCount += blockCounts[i];
	}

	return totalCount;
}

static b2ParticleCoreBuffers b2AllocateParticleCoreBuffers( b2ParticleSystem* system, int count )
{
	return (b2ParticleCoreBuffers){
		.positions = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, b2Vec2 ),
		.velocities = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, b2Vec2 ),
		.forces = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, b2Vec2 ),
		.flags = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, uint32_t ),
		.colors = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, b2ParticleColor ),
		.userDataBuffer = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, void* ),
		.weights = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, float ),
		.lifetimes = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, float ),
		.ages = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, float ),
		.accumulations = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, float ),
		.staticPressures = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, float ),
		.depths = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, float ),
		.accumulationVectors = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, b2Vec2 ),
		.groupIndices = b2ParticleScratchBuffer_AllocArray( &system->scratch, count, int ),
	};
}

static void b2ParticleCompactionTask( int startIndex, int endIndex, uint32_t workerIndex, void* taskContext )
{
	B2_UNUSED( workerIndex );

	b2ParticleCompactionTaskContext* context = taskContext;
	b2ParticleSystem* system = context->system;
	int blockSize = context->blockSize;

	switch ( context->type )
	{
		case b2_particleCompactionTaskMark:
			for ( int blockIndex = startIndex; blockIndex < endIndex; ++blockIndex )
			{
				int firstIndex = blockIndex * blockSize;
				int lastIndex = b2MinInt( firstIndex + blockSize, context->oldCount );
				int liveCount = 0;
				for ( int i = firstIndex; i < lastIndex; ++i )
				{
					if ( ( system->flags[i] & b2_zombieParticle ) != 0 )
					{
						context->newIndices[i] = B2_NULL_INDEX;
						continue;
					}

					context->newIndices[i] = liveCount;
					liveCount += 1;
				}

				context->blockLiveCounts[blockIndex] = liveCount;
			}
			break;

		case b2_particleCompactionTaskFinalize:
			for ( int blockIndex = startIndex; blockIndex < endIndex; ++blockIndex )
			{
				int firstIndex = blockIndex * blockSize;
				int lastIndex = b2MinInt( firstIndex + blockSize, context->oldCount );
				int blockOffset = context->blockOffsets[blockIndex];
				int moveCount = 0;
				for ( int i = firstIndex; i < lastIndex; ++i )
				{
					int localIndex = context->newIndices[i];
					if ( localIndex == B2_NULL_INDEX )
					{
						continue;
					}

					int newIndex = blockOffset + localIndex;
					context->newIndices[i] = newIndex;
					if ( newIndex != i )
					{
						moveCount += 1;
					}
				}

				context->blockMoveCounts[blockIndex] = moveCount;
			}
			break;

		case b2_particleCompactionTaskScatter:
			for ( int blockIndex = startIndex; blockIndex < endIndex; ++blockIndex )
			{
				int firstIndex = blockIndex * blockSize;
				int lastIndex = b2MinInt( firstIndex + blockSize, context->oldCount );
				for ( int oldIndex = firstIndex; oldIndex < lastIndex; ++oldIndex )
				{
					int newIndex = context->newIndices[oldIndex];
					if ( newIndex == B2_NULL_INDEX )
					{
						continue;
					}

					context->buffers.positions[newIndex] = system->positions[oldIndex];
					context->buffers.velocities[newIndex] = system->velocities[oldIndex];
					context->buffers.forces[newIndex] = system->forces[oldIndex];
					context->buffers.flags[newIndex] = system->flags[oldIndex];
					context->buffers.colors[newIndex] = system->colors[oldIndex];
					context->buffers.userDataBuffer[newIndex] = system->userDataBuffer[oldIndex];
					context->buffers.weights[newIndex] = system->weights[oldIndex];
					context->buffers.lifetimes[newIndex] = system->lifetimes[oldIndex];
					context->buffers.ages[newIndex] = system->ages[oldIndex];
					context->buffers.accumulations[newIndex] = system->accumulations[oldIndex];
					context->buffers.staticPressures[newIndex] = system->staticPressures[oldIndex];
					context->buffers.depths[newIndex] = system->depths[oldIndex];
					context->buffers.accumulationVectors[newIndex] = system->accumulationVectors[oldIndex];
					context->buffers.groupIndices[newIndex] = system->groupIndices[oldIndex];
				}
			}
			break;

		case b2_particleCompactionTaskPublish:
			for ( int i = startIndex; i < endIndex; ++i )
			{
				system->positions[i] = context->buffers.positions[i];
				system->velocities[i] = context->buffers.velocities[i];
				system->forces[i] = context->buffers.forces[i];
				system->flags[i] = context->buffers.flags[i];
				system->colors[i] = context->buffers.colors[i];
				system->userDataBuffer[i] = context->buffers.userDataBuffer[i];
				system->weights[i] = context->buffers.weights[i];
				system->lifetimes[i] = context->buffers.lifetimes[i];
				system->ages[i] = context->buffers.ages[i];
				system->accumulations[i] = context->buffers.accumulations[i];
				system->staticPressures[i] = context->buffers.staticPressures[i];
				system->depths[i] = context->buffers.depths[i];
				system->accumulationVectors[i] = context->buffers.accumulationVectors[i];
				system->groupIndices[i] = context->buffers.groupIndices[i];
			}
			break;
	}
}

static int b2GetParticleBodyTaskBlockSize( const b2World* world, const b2ParticleSystem* system )
{
	int blockSize = B2_PARTICLE_TASK_MIN_RANGE;
	int particleCount = system != NULL ? system->particleCount : 0;
	int shapeCount = world != NULL ? world->shapes.count : 0;
	int candidatePressure = shapeCount * b2MaxInt( 1, particleCount / B2_PARTICLE_TASK_MIN_RANGE );
	if ( candidatePressure > 2048 )
	{
		blockSize = B2_PARTICLE_TASK_MIN_RANGE / 4;
	}
	else if ( candidatePressure > 512 )
	{
		blockSize = B2_PARTICLE_TASK_MIN_RANGE / 2;
	}

	return b2MaxInt( 32, blockSize );
}

static int b2GetParticleDeltaPartitionCount( const b2World* world, const b2ParticleSystem* system )
{
	int workerCount = world != NULL ? world->workerCount : 1;
	int particlePartitions = b2GetParticleBlockCount( system != NULL ? system->particleCount : 0 );
	return b2MaxInt( 1, b2MinInt( workerCount, particlePartitions ) );
}

static int b2GetParticleDeltaPartitionSize( const b2ParticleSystem* system, int partitionCount )
{
	int particleCount = system != NULL ? system->particleCount : 0;
	return partitionCount > 0 ? b2MaxInt( 1, ( particleCount + partitionCount - 1 ) / partitionCount ) : 1;
}

static int b2GetParticleDeltaPartition( int index, int partitionCount, int partitionSize )
{
	if ( partitionCount <= 1 )
	{
		return 0;
	}

	int partition = index / partitionSize;
	return b2ClampInt( partition, 0, partitionCount - 1 );
}

static b2ParticleFloatDeltaBlock* b2GetParticleFloatDeltaBlock( b2ParticleContactSolveTaskContext* context, int blockIndex,
																int particleIndex )
{
	if ( context->floatBlocks == NULL )
	{
		return NULL;
	}

	int partition =
		b2GetParticleDeltaPartition( particleIndex, context->deltaPartitionCount, context->deltaPartitionSize );
	return context->floatBlocks + blockIndex * context->deltaPartitionCount + partition;
}

static b2ParticleVec2DeltaBlock* b2GetParticleVec2DeltaBlock( b2ParticleContactSolveTaskContext* context, int blockIndex,
															  int particleIndex )
{
	if ( context->vec2Blocks == NULL )
	{
		return NULL;
	}

	int partition =
		b2GetParticleDeltaPartition( particleIndex, context->deltaPartitionCount, context->deltaPartitionSize );
	return context->vec2Blocks + blockIndex * context->deltaPartitionCount + partition;
}

static b2ParticleVec2DeltaBlock* b2GetParticleSpringVec2DeltaBlock( b2ParticleSpringSolveTaskContext* context, int blockIndex,
																	int particleIndex )
{
	if ( context->vec2Blocks == NULL )
	{
		return NULL;
	}

	int partition =
		b2GetParticleDeltaPartition( particleIndex, context->deltaPartitionCount, context->deltaPartitionSize );
	return context->vec2Blocks + blockIndex * context->deltaPartitionCount + partition;
}

static void b2AppendParticleTaskFloatDelta( b2ParticleContactSolveTaskContext* context, int blockIndex, int particleIndex,
										   float delta )
{
	b2AppendParticleFloatDeltaBlock( b2GetParticleFloatDeltaBlock( context, blockIndex, particleIndex ), particleIndex, delta );
}

static void b2AppendParticleTaskVec2Delta( b2ParticleContactSolveTaskContext* context, int blockIndex, int particleIndex,
										  b2Vec2 delta )
{
	b2AppendParticleVec2DeltaBlock( b2GetParticleVec2DeltaBlock( context, blockIndex, particleIndex ), particleIndex, delta );
}

static void b2AppendParticleSpringVec2Delta( b2ParticleSpringSolveTaskContext* context, int blockIndex, int particleIndex,
											b2Vec2 delta )
{
	b2AppendParticleVec2DeltaBlock( b2GetParticleSpringVec2DeltaBlock( context, blockIndex, particleIndex ), particleIndex, delta );
}

static void b2ParticleDeltaApplyTask( int startIndex, int endIndex, uint32_t workerIndex, void* taskContext )
{
	B2_UNUSED( workerIndex );

	b2ParticleDeltaApplyTaskContext* context = taskContext;
	for ( int partitionIndex = startIndex; partitionIndex < endIndex; ++partitionIndex )
	{
		for ( int blockIndex = 0; blockIndex < context->sourceBlockCount; ++blockIndex )
		{
			int deltaBlockIndex = blockIndex * context->partitionCount + partitionIndex;
			switch ( context->type )
			{
				case b2_particleDeltaTaskFloat:
				{
					b2ParticleFloatDeltaBlock* block = ( (b2ParticleFloatDeltaBlock*)context->blocks ) + deltaBlockIndex;
					for ( int i = 0; i < block->count; ++i )
					{
						b2ParticleFloatDeltaData* delta = block->deltas + i;
						context->floatValues[delta->index] += delta->delta;
					}
					if ( block->payload == NULL )
					{
						b2Free( block->deltas, block->capacity * (int)sizeof( b2ParticleFloatDeltaData ) );
					}
				}
				break;

				case b2_particleDeltaTaskFloatMin:
				{
					b2ParticleFloatDeltaBlock* block = ( (b2ParticleFloatDeltaBlock*)context->blocks ) + deltaBlockIndex;
					for ( int i = 0; i < block->count; ++i )
					{
						b2ParticleFloatDeltaData* delta = block->deltas + i;
						context->floatValues[delta->index] = b2MinFloat( context->floatValues[delta->index], delta->delta );
					}
					if ( block->payload == NULL )
					{
						b2Free( block->deltas, block->capacity * (int)sizeof( b2ParticleFloatDeltaData ) );
					}
				}
				break;

				case b2_particleDeltaTaskVec2:
				{
					b2ParticleVec2DeltaBlock* block = ( (b2ParticleVec2DeltaBlock*)context->blocks ) + deltaBlockIndex;
					for ( int i = 0; i < block->count; ++i )
					{
						b2ParticleVec2DeltaData* delta = block->deltas + i;
						context->vec2Values[delta->index] = b2Add( context->vec2Values[delta->index], delta->delta );
					}
					if ( block->payload == NULL )
					{
						b2Free( block->deltas, block->capacity * (int)sizeof( b2ParticleVec2DeltaData ) );
					}
				}
				break;

			}
		}
	}
}

static void b2ApplyParticleFloatDeltaBlocks( b2World* world, b2ParticleSystem* system, b2ParticleFloatDeltaBlock* blocks,
											int blockCount, int partitionCount, float* values )
{
	uint64_t ticks = b2GetTicks();
	int deltaBlockCount = blockCount * partitionCount;
	int deltaCount = 0;
	for ( int i = 0; i < deltaBlockCount; ++i )
	{
		deltaCount += blocks[i].count;
	}
	system->reductionDeltaCount += deltaCount;
	if ( deltaCount == 0 )
	{
		system->profile.reductionApply += b2GetMilliseconds( ticks );
		return;
	}

	b2ParticleDeltaApplyTaskContext context = {
		.type = b2_particleDeltaTaskFloat,
		.blocks = blocks,
		.sourceBlockCount = blockCount,
		.partitionCount = partitionCount,
		.floatValues = values,
	};
	b2RunParticleTask( world, system, b2ParticleDeltaApplyTask, partitionCount, 1, &context );
	system->reductionApplyCount += blockCount * partitionCount;
	system->profile.reductionApply += b2GetMilliseconds( ticks );
}

static int b2CountParticleFloatDeltaBlocks( b2ParticleFloatDeltaBlock* blocks, int blockCount, int partitionCount )
{
	int deltaBlockCount = blockCount * partitionCount;
	int count = 0;
	for ( int i = 0; i < deltaBlockCount; ++i )
	{
		count += blocks[i].count;
	}
	return count;
}

static void b2ApplyParticleFloatMinDeltaBlocks( b2World* world, b2ParticleSystem* system, b2ParticleFloatDeltaBlock* blocks,
											   int blockCount, int partitionCount, float* values )
{
	uint64_t ticks = b2GetTicks();
	int deltaBlockCount = blockCount * partitionCount;
	int deltaCount = 0;
	for ( int i = 0; i < deltaBlockCount; ++i )
	{
		deltaCount += blocks[i].count;
	}
	system->reductionDeltaCount += deltaCount;
	if ( deltaCount == 0 )
	{
		system->profile.reductionApply += b2GetMilliseconds( ticks );
		return;
	}

	b2ParticleDeltaApplyTaskContext context = {
		.type = b2_particleDeltaTaskFloatMin,
		.blocks = blocks,
		.sourceBlockCount = blockCount,
		.partitionCount = partitionCount,
		.floatValues = values,
	};
	b2RunParticleTask( world, system, b2ParticleDeltaApplyTask, partitionCount, 1, &context );
	system->reductionApplyCount += blockCount * partitionCount;
	system->profile.reductionApply += b2GetMilliseconds( ticks );
}

static void b2ApplyParticleVec2DeltaBlocks( b2World* world, b2ParticleSystem* system, b2ParticleVec2DeltaBlock* blocks,
										   int blockCount, int partitionCount, b2Vec2* values )
{
	uint64_t ticks = b2GetTicks();
	int deltaBlockCount = blockCount * partitionCount;
	int deltaCount = 0;
	for ( int i = 0; i < deltaBlockCount; ++i )
	{
		deltaCount += blocks[i].count;
	}
	system->reductionDeltaCount += deltaCount;
	if ( deltaCount == 0 )
	{
		system->profile.reductionApply += b2GetMilliseconds( ticks );
		return;
	}

	b2ParticleDeltaApplyTaskContext context = {
		.type = b2_particleDeltaTaskVec2,
		.blocks = blocks,
		.sourceBlockCount = blockCount,
		.partitionCount = partitionCount,
		.vec2Values = values,
	};
	b2RunParticleTask( world, system, b2ParticleDeltaApplyTask, partitionCount, 1, &context );
	system->reductionApplyCount += blockCount * partitionCount;
	system->profile.reductionApply += b2GetMilliseconds( ticks );
}

static void b2ReserveDestructionEvents( b2ParticleSystem* system, int capacity )
{
	if ( capacity <= system->destructionEventCapacity )
	{
		return;
	}

	int oldCapacity = system->destructionEventCapacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	system->destructionEvents =
		b2GrowParticleBuffer( system->destructionEvents, oldCapacity, newCapacity, sizeof( b2ParticleDestructionEvent ) );
	system->destructionEventCapacity = newCapacity;
}

static void b2ReservePairs( b2ParticleSystem* system, int capacity )
{
	if ( capacity <= system->pairCapacity )
	{
		return;
	}

	int oldCapacity = system->pairCapacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	system->pairs = b2GrowParticleBuffer( system->pairs, oldCapacity, newCapacity, sizeof( b2ParticlePairData ) );
	system->pairCapacity = newCapacity;
}

static void b2ReserveTriads( b2ParticleSystem* system, int capacity )
{
	if ( capacity <= system->triadCapacity )
	{
		return;
	}

	int oldCapacity = system->triadCapacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	system->triads = b2GrowParticleBuffer( system->triads, oldCapacity, newCapacity, sizeof( b2ParticleTriadData ) );
	system->triadCapacity = newCapacity;
}

static void b2ReserveGroups( b2ParticleSystem* system, int capacity )
{
	if ( capacity <= system->groupCapacity )
	{
		return;
	}

	int oldCapacity = system->groupCapacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	system->groups = b2GrowParticleBuffer( system->groups, oldCapacity, newCapacity, sizeof( b2ParticleGroup ) );
	for ( int i = oldCapacity; i < newCapacity; ++i )
	{
		system->groups[i] = (b2ParticleGroup){ .id = B2_NULL_INDEX, .localIndex = B2_NULL_INDEX };
	}
	system->groupCapacity = newCapacity;
}

static void b2DestroyParticleSystemStorage( b2ParticleSystem* system )
{
	b2Free( system->positions, system->particleCapacity * (int)sizeof( b2Vec2 ) );
	b2Free( system->velocities, system->particleCapacity * (int)sizeof( b2Vec2 ) );
	b2Free( system->forces, system->particleCapacity * (int)sizeof( b2Vec2 ) );
	b2Free( system->flags, system->particleCapacity * (int)sizeof( uint32_t ) );
	b2Free( system->colors, system->particleCapacity * (int)sizeof( b2ParticleColor ) );
	b2Free( system->userDataBuffer, system->particleCapacity * (int)sizeof( void* ) );
	b2Free( system->weights, system->particleCapacity * (int)sizeof( float ) );
	b2Free( system->lifetimes, system->particleCapacity * (int)sizeof( float ) );
	b2Free( system->ages, system->particleCapacity * (int)sizeof( float ) );
	b2Free( system->accumulations, system->particleCapacity * (int)sizeof( float ) );
	b2Free( system->staticPressures, system->particleCapacity * (int)sizeof( float ) );
	b2Free( system->depths, system->particleCapacity * (int)sizeof( float ) );
	b2Free( system->accumulationVectors, system->particleCapacity * (int)sizeof( b2Vec2 ) );
	b2Free( system->groupIndices, system->particleCapacity * (int)sizeof( int ) );
	b2Free( system->proxies, system->proxyCapacity * (int)sizeof( b2ParticleProxy ) );
	b2Free( system->contacts, system->contactCapacity * (int)sizeof( b2ParticleContactData ) );
	b2Free( system->previousContacts, system->previousContactCapacity * (int)sizeof( b2ParticleContactData ) );
	b2Free( system->contactBeginEvents, system->contactBeginCapacity * (int)sizeof( b2ParticleContactData ) );
	b2Free( system->contactEndEvents, system->contactEndCapacity * (int)sizeof( b2ParticleContactData ) );
	b2Free( system->bodyContacts, system->bodyContactCapacity * (int)sizeof( b2ParticleBodyContactData ) );
	b2Free( system->previousBodyContacts, system->previousBodyContactCapacity * (int)sizeof( b2ParticleBodyContactData ) );
	b2Free( system->bodyContactBeginEvents, system->bodyContactBeginCapacity * (int)sizeof( b2ParticleBodyContactData ) );
	b2Free( system->bodyContactEndEvents, system->bodyContactEndCapacity * (int)sizeof( b2ParticleBodyContactData ) );
	b2Free( system->destructionEvents, system->destructionEventCapacity * (int)sizeof( b2ParticleDestructionEvent ) );
	b2Free( system->pairs, system->pairCapacity * (int)sizeof( b2ParticlePairData ) );
	b2Free( system->triads, system->triadCapacity * (int)sizeof( b2ParticleTriadData ) );
	b2Free( system->groups, system->groupCapacity * (int)sizeof( b2ParticleGroup ) );
	b2ParticleScratchBuffer_Destroy( &system->scratch );
	b2DestroyParticlePayloadStorage( &system->contactBlockPayloads );
	b2DestroyParticlePayloadStorage( &system->bodyContactBlockPayloads );
	b2DestroyParticlePayloadStorage( &system->bodyImpulseBlockPayloads );
	b2DestroyParticlePayloadStorage( &system->shapeCandidatePayloads );
	b2DestroyParticlePayloadStorage( &system->floatDeltaBlockPayloads );
	b2DestroyParticlePayloadStorage( &system->vec2DeltaBlockPayloads );
	b2DestroyParticlePayloadStorage( &system->barrierGroupDeltaBlockPayloads );
	b2DestroyParticlePayloadStorage( &system->eventContactSortPayloads );
	b2DestroyParticlePayloadStorage( &system->eventBodyContactSortPayloads );
	b2DestroyIdPool( &system->groupIdPool );
}

static void b2FreeParticleGroupIds( b2World* world, b2ParticleSystem* system )
{
	for ( int i = 0; i < system->groupCount; ++i )
	{
		b2ParticleGroup* group = system->groups + i;
		if ( group->id != B2_NULL_INDEX )
		{
			b2FreeId( &world->particleGroupIdPool, group->id );
		}
	}
}

void b2CreateParticleWorld( b2World* world )
{
	world->particleSystemIdPool = b2CreateIdPool();
	world->particleGroupIdPool = b2CreateIdPool();
	world->particleSystems = NULL;
	world->particleSystemCount = 0;
	world->particleSystemCapacity = 0;
}

void b2DestroyParticleWorld( b2World* world )
{
	for ( int i = 0; i < world->particleSystemCount; ++i )
	{
		b2ParticleSystem* system = world->particleSystems + i;
		if ( system->id != B2_NULL_INDEX )
		{
			b2FreeParticleGroupIds( world, system );
			b2DestroyParticleSystemStorage( system );
		}
	}

	b2Free( world->particleSystems, world->particleSystemCapacity * (int)sizeof( b2ParticleSystem ) );
	world->particleSystems = NULL;
	world->particleSystemCount = 0;
	world->particleSystemCapacity = 0;
	b2DestroyIdPool( &world->particleSystemIdPool );
	b2DestroyIdPool( &world->particleGroupIdPool );
}

void b2ClearParticleEvents( b2World* world )
{
	for ( int i = 0; i < world->particleSystemCount; ++i )
	{
		b2ParticleSystem* system = world->particleSystems + i;
		if ( system->id == B2_NULL_INDEX )
		{
			continue;
		}

		system->contactBeginCount = 0;
		system->contactEndCount = 0;
		system->bodyContactBeginCount = 0;
		system->bodyContactEndCount = 0;
		system->destructionEventCount = 0;
	}
}

static b2ParticleSystemId b2MakeParticleSystemId( b2World* world, int systemId )
{
	b2ParticleSystem* system = world->particleSystems + systemId;
	return (b2ParticleSystemId){ systemId + 1, world->worldId, system->generation };
}

static b2ParticleGroupId b2MakeParticleGroupId( b2World* world, b2ParticleSystem* system, int groupId )
{
	b2ParticleGroup* group = system->groups + groupId;
	return (b2ParticleGroupId){ group->id + 1, world->worldId, group->generation };
}

b2ParticleSystem* b2GetParticleSystemFullId( b2World* world, b2ParticleSystemId systemId )
{
	int id = systemId.index1 - 1;
	B2_ASSERT( 0 <= id && id < world->particleSystemCount );
	b2ParticleSystem* system = world->particleSystems + id;
	B2_ASSERT( system->id == id );
	B2_ASSERT( system->generation == systemId.generation );
	return system;
}

b2ParticleGroup* b2GetParticleGroupFullId( b2World* world, b2ParticleGroupId groupId )
{
	for ( int i = 0; i < world->particleSystemCount; ++i )
	{
		b2ParticleSystem* system = world->particleSystems + i;
		if ( system->id == B2_NULL_INDEX || groupId.index1 < 1 )
		{
			continue;
		}

		for ( int groupIndex = 0; groupIndex < system->groupCount; ++groupIndex )
		{
			b2ParticleGroup* group = system->groups + groupIndex;
			if ( group->id == groupId.index1 - 1 && group->generation == groupId.generation )
			{
				return group;
			}
		}
	}

	B2_ASSERT( false );
	return NULL;
}

static b2ParticleSystem* b2TryGetParticleSystem( b2ParticleSystemId systemId )
{
	b2World* world = b2TryGetWorld( systemId.world0 );
	if ( world == NULL )
	{
		return NULL;
	}

	int id = systemId.index1 - 1;
	if ( id < 0 || world->particleSystemCount <= id )
	{
		return NULL;
	}

	b2ParticleSystem* system = world->particleSystems + id;
	if ( system->id != id || system->generation != systemId.generation )
	{
		return NULL;
	}

	return system;
}

static b2World* b2GetWorldFromParticleSystemId( b2ParticleSystemId systemId )
{
	b2World* world = b2GetWorld( systemId.world0 );
	B2_ASSERT( world != NULL );
	return world;
}

static b2ParticleGroup* b2TryGetParticleGroup( b2ParticleGroupId groupId, b2ParticleSystem** outSystem )
{
	if ( groupId.index1 < 1 )
	{
		return NULL;
	}

	b2World* world = b2TryGetWorld( groupId.world0 );
	if ( world == NULL )
	{
		return NULL;
	}

	for ( int i = 0; i < world->particleSystemCount; ++i )
	{
		b2ParticleSystem* system = world->particleSystems + i;
		if ( system->id == B2_NULL_INDEX )
		{
			continue;
		}

		for ( int groupIndex = 0; groupIndex < system->groupCount; ++groupIndex )
		{
			b2ParticleGroup* group = system->groups + groupIndex;
			if ( group->id == groupId.index1 - 1 && group->generation == groupId.generation )
			{
				if ( outSystem != NULL )
				{
					*outSystem = system;
				}
				return group;
			}
		}
	}

	return NULL;
}

b2ParticleSystemDef b2DefaultParticleSystemDef( void )
{
	b2ParticleSystemDef def = { 0 };
	def.density = 1.0f;
	def.gravityScale = 1.0f;
	def.radius = 1.0f;
	def.pressureStrength = 0.05f;
	def.dampingStrength = 1.0f;
	def.elasticStrength = 0.25f;
	def.springStrength = 0.25f;
	def.viscousStrength = 0.25f;
	def.surfaceTensionPressureStrength = 0.2f;
	def.surfaceTensionNormalStrength = 0.2f;
	def.repulsiveStrength = 1.0f;
	def.powderStrength = 0.5f;
	def.ejectionStrength = 0.5f;
	def.staticPressureStrength = 0.2f;
	def.staticPressureRelaxation = 0.2f;
	def.staticPressureIterations = 8;
	def.colorMixingStrength = 0.5f;
	def.destroyByAge = true;
	def.lifetimeGranularity = 1.0f / 60.0f;
	def.iterationCount = 1;
	def.enableParticleDebugDraw = true;
	def.internalValue = B2_SECRET_COOKIE;
	return def;
}

b2ParticleDef b2DefaultParticleDef( void )
{
	b2ParticleDef def = { 0 };
	def.color = b2_particleColorZero;
	def.groupId = b2_nullParticleGroupId;
	def.internalValue = B2_SECRET_COOKIE;
	return def;
}

b2ParticleGroupDef b2DefaultParticleGroupDef( void )
{
	b2ParticleGroupDef def = { 0 };
	def.rotation = b2Rot_identity;
	def.color = b2_particleColorZero;
	def.strength = 1.0f;
	def.groupId = b2_nullParticleGroupId;
	def.internalValue = B2_SECRET_COOKIE;
	return def;
}

int b2CalculateParticleIterations( float gravity, float radius, float timeStep )
{
	const int maxRecommendedParticleIterations = 8;
	const float radiusThreshold = 0.01f;
	if ( gravity <= 0.0f || radius <= 0.0f || timeStep <= 0.0f )
	{
		return 1;
	}

	int iterations = (int)ceilf( sqrtf( gravity / ( radiusThreshold * radius ) ) * timeStep );
	return b2ClampInt( iterations, 1, maxRecommendedParticleIterations );
}

b2ParticleSystemId b2CreateParticleSystem( b2WorldId worldId, const b2ParticleSystemDef* def )
{
	B2_CHECK_DEF( def );
	B2_ASSERT( def->density > 0.0f );
	B2_ASSERT( def->radius > 0.0f );

	if ( b2World_IsValid( worldId ) == false )
	{
		return b2_nullParticleSystemId;
	}

	b2World* world = b2GetWorldFromId( worldId );
	B2_ASSERT( world->locked == false );
	if ( world->locked )
	{
		return b2_nullParticleSystemId;
	}

	int systemId = b2AllocId( &world->particleSystemIdPool );
	if ( systemId >= world->particleSystemCapacity )
	{
		int oldCapacity = world->particleSystemCapacity;
		int newCapacity = b2MaxParticleCapacity( oldCapacity, systemId + 1 );
		world->particleSystems =
			b2GrowParticleBuffer( world->particleSystems, oldCapacity, newCapacity, sizeof( b2ParticleSystem ) );
		for ( int i = oldCapacity; i < newCapacity; ++i )
		{
			world->particleSystems[i] = (b2ParticleSystem){ .id = B2_NULL_INDEX };
		}
		world->particleSystemCapacity = newCapacity;
	}

	world->particleSystemCount = b2MaxInt( world->particleSystemCount, systemId + 1 );
	b2ParticleSystem* system = world->particleSystems + systemId;
	uint16_t generation = system->generation + 1;
	*system = (b2ParticleSystem){ 0 };
	system->id = systemId;
	system->worldId = world->worldId;
	system->generation = generation;
	system->def = *def;
	system->def.iterationCount = b2MaxInt( 1, def->iterationCount );
	system->userData = def->userData;
	system->groupIdPool = b2CreateIdPool();
	b2ParticleScratchBuffer_Init( &system->scratch );

	return b2MakeParticleSystemId( world, systemId );
}

void b2DestroyParticleSystem( b2ParticleSystemId systemId )
{
	b2World* world = b2GetWorldLocked( systemId.world0 );
	if ( world == NULL )
	{
		return;
	}

	b2ParticleSystem* system = b2GetParticleSystemFullId( world, systemId );
	b2FreeParticleGroupIds( world, system );
	b2DestroyParticleSystemStorage( system );
	uint16_t generation = system->generation + 1;
	int id = system->id;
	*system = (b2ParticleSystem){ .id = B2_NULL_INDEX, .generation = generation };
	b2FreeId( &world->particleSystemIdPool, id );
}

bool b2ParticleSystem_IsValid( b2ParticleSystemId systemId )
{
	return b2TryGetParticleSystem( systemId ) != NULL;
}

b2WorldId b2ParticleSystem_GetWorld( b2ParticleSystemId systemId )
{
	b2World* world = b2GetWorldFromParticleSystemId( systemId );
	b2GetParticleSystemFullId( world, systemId );
	return (b2WorldId){ systemId.world0 + 1, world->generation };
}

void b2ParticleSystem_SetUserData( b2ParticleSystemId systemId, void* userData )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL )
	{
		system->userData = userData;
	}
}

void* b2ParticleSystem_GetUserData( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->userData : NULL;
}

static void b2ParticleGroupRefreshTask( int startIndex, int endIndex, uint32_t workerIndex, void* taskContext )
{
	B2_UNUSED( workerIndex );

	b2ParticleGroupRefreshTaskContext* context = taskContext;
	b2ParticleSystem* system = context->system;
	for ( int blockIndex = startIndex; blockIndex < endIndex; ++blockIndex )
	{
		b2ParticleGroupAccumulation* accumulations = context->accumulations + blockIndex * context->groupCount;
		int firstParticle = blockIndex * context->blockSize;
		int lastParticle = b2MinInt( firstParticle + context->blockSize, system->particleCount );
		for ( int particleIndex = firstParticle; particleIndex < lastParticle; ++particleIndex )
		{
			int groupIndex = system->groupIndices[particleIndex];
			if ( groupIndex == B2_NULL_INDEX )
			{
				continue;
			}

			b2ParticleGroup* group = system->groups + groupIndex;
			if ( group->id == B2_NULL_INDEX )
			{
				system->groupIndices[particleIndex] = B2_NULL_INDEX;
				continue;
			}

			b2ParticleGroupAccumulation* accumulation = accumulations + groupIndex;
			if ( context->type == b2_particleGroupRefreshMass )
			{
				if ( accumulation->firstIndex == B2_NULL_INDEX )
				{
					accumulation->firstIndex = particleIndex;
				}

				accumulation->count += 1;
				accumulation->mass += context->particleMass;
				accumulation->center =
					b2MulAdd( accumulation->center, context->particleMass, system->positions[particleIndex] );
				accumulation->linearVelocity =
					b2MulAdd( accumulation->linearVelocity, context->particleMass, system->velocities[particleIndex] );
			}
			else
			{
				if ( group->count == 0 )
				{
					continue;
				}

				b2Vec2 r = b2Sub( system->positions[particleIndex], group->center );
				b2Vec2 relativeVelocity = b2Sub( system->velocities[particleIndex], group->linearVelocity );
				accumulation->inertia += context->particleMass * b2LengthSquared( r );
				accumulation->angularVelocity += context->particleMass * b2Cross( r, relativeVelocity );
			}
		}
	}
}

static void b2InitializeGroupAccumulations( b2ParticleGroupAccumulation* accumulations, int count )
{
	for ( int i = 0; i < count; ++i )
	{
		accumulations[i].firstIndex = B2_NULL_INDEX;
		accumulations[i].count = 0;
		accumulations[i].mass = 0.0f;
		accumulations[i].inertia = 0.0f;
		accumulations[i].center = b2Vec2_zero;
		accumulations[i].linearVelocity = b2Vec2_zero;
		accumulations[i].angularVelocity = 0.0f;
	}
}

static void b2RefreshGroups( b2ParticleSystem* system )
{
	uint64_t ticks = b2GetTicks();
	system->groupRefreshCount += system->particleCount;
	for ( int i = 0; i < system->groupCount; ++i )
	{
		b2ParticleGroup* group = system->groups + i;
		if ( group->id == B2_NULL_INDEX )
		{
			continue;
		}

		group->firstIndex = B2_NULL_INDEX;
		group->count = 0;
		group->mass = 0.0f;
		group->inertia = 0.0f;
		group->center = b2Vec2_zero;
		group->linearVelocity = b2Vec2_zero;
		group->angularVelocity = 0.0f;
	}

	float particleMass = system->def.density * B2_PARTICLE_STRIDE * B2_PARTICLE_STRIDE *
						 ( 2.0f * system->def.radius ) * ( 2.0f * system->def.radius );

	int blockSize = B2_PARTICLE_TASK_MIN_RANGE;
	int blockCount = b2GetParticleBlockCount( system->particleCount );
	b2World* world = b2TryGetWorld( system->worldId );
	b2ParticleGroupAccumulation* accumulations = NULL;
	if ( blockCount > 0 && system->groupCount > 0 )
	{
		int accumulationCount = blockCount * system->groupCount;
		uint64_t scratchTicks = b2GetTicks();
		b2ParticleScratchBuffer_Reset( &system->scratch );
		accumulations = b2ParticleScratchBuffer_AllocArray( &system->scratch, accumulationCount, b2ParticleGroupAccumulation );
		system->profile.scratch += b2GetMilliseconds( scratchTicks );
		b2InitializeGroupAccumulations( accumulations, accumulationCount );

		b2ParticleGroupRefreshTaskContext context = {
			.system = system,
			.accumulations = accumulations,
			.type = b2_particleGroupRefreshMass,
			.blockSize = blockSize,
			.groupCount = system->groupCount,
			.particleMass = particleMass,
		};
		b2RunParticleTask( world, system, b2ParticleGroupRefreshTask, blockCount, 1, &context );
	}

	for ( int i = 0; i < system->groupCount; ++i )
	{
		b2ParticleGroup* group = system->groups + i;
		if ( group->id == B2_NULL_INDEX )
		{
			continue;
		}

		for ( int blockIndex = 0; blockIndex < blockCount; ++blockIndex )
		{
			b2ParticleGroupAccumulation* accumulation = accumulations + blockIndex * system->groupCount + i;
			if ( accumulation->count == 0 )
			{
				continue;
			}

			if ( group->firstIndex == B2_NULL_INDEX )
			{
				group->firstIndex = accumulation->firstIndex;
			}

			group->count += accumulation->count;
			group->mass += accumulation->mass;
			group->center = b2Add( group->center, accumulation->center );
			group->linearVelocity = b2Add( group->linearVelocity, accumulation->linearVelocity );
		}

		if ( group->count == 0 )
		{
			if ( ( group->groupFlags & b2_particleGroupCanBeEmpty ) == 0 )
			{
				b2World* world = b2TryGetWorld( system->worldId );
				int id = group->id;
				int localIndex = group->localIndex;
				uint16_t generation = group->generation + 1;
				*group = (b2ParticleGroup){ .id = B2_NULL_INDEX, .generation = generation };
				if ( world != NULL )
				{
					b2FreeId( &world->particleGroupIdPool, id );
				}
				b2FreeId( &system->groupIdPool, localIndex );
			}
			continue;
		}

		float invMass = group->mass > 0.0f ? 1.0f / group->mass : 0.0f;
		group->center = b2MulSV( invMass, group->center );
		group->linearVelocity = b2MulSV( invMass, group->linearVelocity );
	}

	if ( accumulations != NULL )
	{
		int accumulationCount = blockCount * system->groupCount;
		b2InitializeGroupAccumulations( accumulations, accumulationCount );

		b2ParticleGroupRefreshTaskContext context = {
			.system = system,
			.accumulations = accumulations,
			.type = b2_particleGroupRefreshAngular,
			.blockSize = blockSize,
			.groupCount = system->groupCount,
			.particleMass = particleMass,
		};
		b2RunParticleTask( world, system, b2ParticleGroupRefreshTask, blockCount, 1, &context );
	}

	for ( int i = 0; i < system->groupCount; ++i )
	{
		b2ParticleGroup* group = system->groups + i;
		if ( group->id == B2_NULL_INDEX )
		{
			continue;
		}

		for ( int blockIndex = 0; blockIndex < blockCount; ++blockIndex )
		{
			b2ParticleGroupAccumulation* accumulation = accumulations + blockIndex * system->groupCount + i;
			group->inertia += accumulation->inertia;
			group->angularVelocity += accumulation->angularVelocity;
		}

		if ( group->inertia > 0.0f )
		{
			group->angularVelocity /= group->inertia;
		}
	}

	if ( accumulations != NULL )
	{
		b2ParticleScratchBuffer_Reset( &system->scratch );
	}

	system->profile.groupRefresh += b2GetMilliseconds( ticks );
}

static b2ParticleState b2ReadParticleState( const b2ParticleSystem* system, int index )
{
	return (b2ParticleState){
		.position = system->positions[index],
		.velocity = system->velocities[index],
		.force = system->forces[index],
		.flags = system->flags[index],
		.color = system->colors[index],
		.userData = system->userDataBuffer[index],
		.weight = system->weights[index],
		.lifetime = system->lifetimes[index],
		.age = system->ages[index],
		.depth = system->depths[index],
		.groupIndex = system->groupIndices[index],
	};
}

static void b2WriteParticleState( b2ParticleSystem* system, int index, const b2ParticleState* state )
{
	system->positions[index] = state->position;
	system->velocities[index] = state->velocity;
	system->forces[index] = state->force;
	system->flags[index] = state->flags;
	system->colors[index] = state->color;
	system->userDataBuffer[index] = state->userData;
	system->weights[index] = state->weight;
	system->lifetimes[index] = state->lifetime;
	system->ages[index] = state->age;
	system->depths[index] = state->depth;
	system->groupIndices[index] = state->groupIndex;
}

static bool b2TryRemapParticleProxy( b2ParticleProxy proxy, const int* newIndices, int oldCount, b2ParticleProxy* remappedProxy )
{
	if ( proxy.index < 0 || oldCount <= proxy.index )
	{
		return false;
	}

	int particleIndex = newIndices[proxy.index];
	if ( particleIndex == B2_NULL_INDEX )
	{
		return false;
	}

	proxy.index = particleIndex;
	*remappedProxy = proxy;
	return true;
}

static bool b2TryRemapParticleContact( b2ParticleContactData contact, const int* newIndices, int oldCount,
									   b2ParticleContactData* remappedContact )
{
	if ( contact.indexA < 0 || oldCount <= contact.indexA || contact.indexB < 0 || oldCount <= contact.indexB )
	{
		return false;
	}

	int indexA = newIndices[contact.indexA];
	int indexB = newIndices[contact.indexB];
	if ( indexA == B2_NULL_INDEX || indexB == B2_NULL_INDEX || indexA == indexB )
	{
		return false;
	}

	contact.indexA = indexA;
	contact.indexB = indexB;
	*remappedContact = contact;
	return true;
}

static bool b2TryRemapParticleBodyContact( b2ParticleBodyContactData contact, const int* newIndices, int oldCount,
										   b2ParticleBodyContactData* remappedContact )
{
	if ( contact.particleIndex < 0 || oldCount <= contact.particleIndex )
	{
		return false;
	}

	int particleIndex = newIndices[contact.particleIndex];
	if ( particleIndex == B2_NULL_INDEX )
	{
		return false;
	}

	contact.particleIndex = particleIndex;
	*remappedContact = contact;
	return true;
}

static bool b2TryRemapParticlePair( b2ParticlePairData pair, const int* newIndices, int oldCount,
									b2ParticlePairData* remappedPair )
{
	if ( pair.indexA < 0 || oldCount <= pair.indexA || pair.indexB < 0 || oldCount <= pair.indexB )
	{
		return false;
	}

	int indexA = newIndices[pair.indexA];
	int indexB = newIndices[pair.indexB];
	if ( indexA == B2_NULL_INDEX || indexB == B2_NULL_INDEX || indexA == indexB )
	{
		return false;
	}

	if ( indexB < indexA )
	{
		int swap = indexA;
		indexA = indexB;
		indexB = swap;
	}

	pair.indexA = indexA;
	pair.indexB = indexB;
	*remappedPair = pair;
	return true;
}

static bool b2TryRemapParticleTriad( b2ParticleTriadData triad, const int* newIndices, int oldCount,
									 b2ParticleTriadData* remappedTriad )
{
	if ( triad.indexA < 0 || oldCount <= triad.indexA || triad.indexB < 0 || oldCount <= triad.indexB || triad.indexC < 0 ||
		 oldCount <= triad.indexC )
	{
		return false;
	}

	int indexA = newIndices[triad.indexA];
	int indexB = newIndices[triad.indexB];
	int indexC = newIndices[triad.indexC];
	if ( indexA == B2_NULL_INDEX || indexB == B2_NULL_INDEX || indexC == B2_NULL_INDEX || indexA == indexB ||
		 indexA == indexC || indexB == indexC )
	{
		return false;
	}

	triad.indexA = indexA;
	triad.indexB = indexB;
	triad.indexC = indexC;
	*remappedTriad = triad;
	return true;
}

static void b2ParticleRemapTask( int startIndex, int endIndex, uint32_t workerIndex, void* taskContext )
{
	B2_UNUSED( workerIndex );

	b2ParticleRemapTaskContext* context = taskContext;
	int blockSize = context->blockSize;

	switch ( context->type )
	{
		case b2_particleRemapProxies:
		{
			const b2ParticleProxy* input = context->input;
			b2ParticleProxy* output = context->output;
			for ( int blockIndex = startIndex; blockIndex < endIndex; ++blockIndex )
			{
				int firstIndex = blockIndex * blockSize;
				int lastIndex = b2MinInt( firstIndex + blockSize, context->itemCount );
				int writeIndex = context->scatter ? context->blockOffsets[blockIndex] : 0;
				for ( int i = firstIndex; i < lastIndex; ++i )
				{
					b2ParticleProxy proxy;
					if ( b2TryRemapParticleProxy( input[i], context->newIndices, context->oldCount, &proxy ) )
					{
						if ( context->scatter )
						{
							output[writeIndex] = proxy;
						}
						writeIndex += 1;
					}
				}
				if ( context->scatter )
				{
					B2_ASSERT( writeIndex == context->blockOffsets[blockIndex] + context->blockCounts[blockIndex] );
				}
				else
				{
					context->blockCounts[blockIndex] = writeIndex;
				}
			}
			break;
		}

		case b2_particleRemapContacts:
		{
			const b2ParticleContactData* input = context->input;
			b2ParticleContactData* output = context->output;
			for ( int blockIndex = startIndex; blockIndex < endIndex; ++blockIndex )
			{
				int firstIndex = blockIndex * blockSize;
				int lastIndex = b2MinInt( firstIndex + blockSize, context->itemCount );
				int writeIndex = context->scatter ? context->blockOffsets[blockIndex] : 0;
				for ( int i = firstIndex; i < lastIndex; ++i )
				{
					b2ParticleContactData contact;
					if ( b2TryRemapParticleContact( input[i], context->newIndices, context->oldCount, &contact ) )
					{
						if ( context->scatter )
						{
							output[writeIndex] = contact;
						}
						writeIndex += 1;
					}
				}
				if ( context->scatter )
				{
					B2_ASSERT( writeIndex == context->blockOffsets[blockIndex] + context->blockCounts[blockIndex] );
				}
				else
				{
					context->blockCounts[blockIndex] = writeIndex;
				}
			}
			break;
		}

		case b2_particleRemapBodyContacts:
		{
			const b2ParticleBodyContactData* input = context->input;
			b2ParticleBodyContactData* output = context->output;
			for ( int blockIndex = startIndex; blockIndex < endIndex; ++blockIndex )
			{
				int firstIndex = blockIndex * blockSize;
				int lastIndex = b2MinInt( firstIndex + blockSize, context->itemCount );
				int writeIndex = context->scatter ? context->blockOffsets[blockIndex] : 0;
				for ( int i = firstIndex; i < lastIndex; ++i )
				{
					b2ParticleBodyContactData contact;
					if ( b2TryRemapParticleBodyContact( input[i], context->newIndices, context->oldCount, &contact ) )
					{
						if ( context->scatter )
						{
							output[writeIndex] = contact;
						}
						writeIndex += 1;
					}
				}
				if ( context->scatter )
				{
					B2_ASSERT( writeIndex == context->blockOffsets[blockIndex] + context->blockCounts[blockIndex] );
				}
				else
				{
					context->blockCounts[blockIndex] = writeIndex;
				}
			}
			break;
		}

		case b2_particleRemapPairs:
		{
			const b2ParticlePairData* input = context->input;
			b2ParticlePairData* output = context->output;
			for ( int blockIndex = startIndex; blockIndex < endIndex; ++blockIndex )
			{
				int firstIndex = blockIndex * blockSize;
				int lastIndex = b2MinInt( firstIndex + blockSize, context->itemCount );
				int writeIndex = context->scatter ? context->blockOffsets[blockIndex] : 0;
				for ( int i = firstIndex; i < lastIndex; ++i )
				{
					b2ParticlePairData pair;
					if ( b2TryRemapParticlePair( input[i], context->newIndices, context->oldCount, &pair ) )
					{
						if ( context->scatter )
						{
							output[writeIndex] = pair;
						}
						writeIndex += 1;
					}
				}
				if ( context->scatter )
				{
					B2_ASSERT( writeIndex == context->blockOffsets[blockIndex] + context->blockCounts[blockIndex] );
				}
				else
				{
					context->blockCounts[blockIndex] = writeIndex;
				}
			}
			break;
		}

		case b2_particleRemapTriads:
		{
			const b2ParticleTriadData* input = context->input;
			b2ParticleTriadData* output = context->output;
			for ( int blockIndex = startIndex; blockIndex < endIndex; ++blockIndex )
			{
				int firstIndex = blockIndex * blockSize;
				int lastIndex = b2MinInt( firstIndex + blockSize, context->itemCount );
				int writeIndex = context->scatter ? context->blockOffsets[blockIndex] : 0;
				for ( int i = firstIndex; i < lastIndex; ++i )
				{
					b2ParticleTriadData triad;
					if ( b2TryRemapParticleTriad( input[i], context->newIndices, context->oldCount, &triad ) )
					{
						if ( context->scatter )
						{
							output[writeIndex] = triad;
						}
						writeIndex += 1;
					}
				}
				if ( context->scatter )
				{
					B2_ASSERT( writeIndex == context->blockOffsets[blockIndex] + context->blockCounts[blockIndex] );
				}
				else
				{
					context->blockCounts[blockIndex] = writeIndex;
				}
			}
			break;
		}
	}
}

static int b2RemapParticleBuffer( b2World* world, b2ParticleSystem* system, b2ParticleRemapBufferType type, void* buffer,
								  int itemCount, int elementSize, int alignment, const int* newIndices, int oldCount,
								  int scratchBase )
{
	if ( itemCount == 0 )
	{
		return 0;
	}

	system->scratch.byteCount = scratchBase;
	int blockCount = b2GetParticleBlockCount( itemCount );
	int* blockCounts = b2ParticleScratchBuffer_AllocArray( &system->scratch, blockCount, int );
	int* blockOffsets = b2ParticleScratchBuffer_AllocArray( &system->scratch, blockCount, int );
	void* output = b2ParticleScratchBuffer_Alloc( &system->scratch, b2ParticleScratchByteCount( itemCount, (size_t)elementSize ),
												 alignment );

	b2ParticleRemapTaskContext context = {
		.type = type,
		.newIndices = newIndices,
		.oldCount = oldCount,
		.itemCount = itemCount,
		.blockSize = B2_PARTICLE_TASK_MIN_RANGE,
		.blockCounts = blockCounts,
		.blockOffsets = blockOffsets,
		.input = buffer,
		.output = output,
		.scatter = false,
	};
	b2RunParticleTask( world, system, b2ParticleRemapTask, blockCount, 1, &context );

	int newCount = b2BuildParticleBlockOffsets( blockCounts, blockOffsets, blockCount );
	context.scatter = true;
	b2RunParticleTask( world, system, b2ParticleRemapTask, blockCount, 1, &context );

	if ( newCount > 0 )
	{
		memcpy( buffer, output, b2ParticleScratchByteCount( newCount, (size_t)elementSize ) );
	}

	return newCount;
}

static void b2RemapParticleSideBuffers( b2World* world, b2ParticleSystem* system, const int* newIndices, int oldCount,
										int scratchBase )
{
	system->proxyCount = b2RemapParticleBuffer( world, system, b2_particleRemapProxies, system->proxies, system->proxyCount,
											   (int)sizeof( b2ParticleProxy ), (int)_Alignof( b2ParticleProxy ), newIndices,
											   oldCount, scratchBase );

	system->contactCount = b2RemapParticleBuffer( world, system, b2_particleRemapContacts, system->contacts, system->contactCount,
												 (int)sizeof( b2ParticleContactData ), (int)_Alignof( b2ParticleContactData ),
												 newIndices, oldCount, scratchBase );
	system->previousContactCount =
		b2RemapParticleBuffer( world, system, b2_particleRemapContacts, system->previousContacts, system->previousContactCount,
							   (int)sizeof( b2ParticleContactData ), (int)_Alignof( b2ParticleContactData ), newIndices,
							   oldCount, scratchBase );
	system->contactBeginCount =
		b2RemapParticleBuffer( world, system, b2_particleRemapContacts, system->contactBeginEvents, system->contactBeginCount,
							   (int)sizeof( b2ParticleContactData ), (int)_Alignof( b2ParticleContactData ), newIndices,
							   oldCount, scratchBase );
	system->contactEndCount =
		b2RemapParticleBuffer( world, system, b2_particleRemapContacts, system->contactEndEvents, system->contactEndCount,
							   (int)sizeof( b2ParticleContactData ), (int)_Alignof( b2ParticleContactData ), newIndices,
							   oldCount, scratchBase );

	system->bodyContactCount =
		b2RemapParticleBuffer( world, system, b2_particleRemapBodyContacts, system->bodyContacts, system->bodyContactCount,
							   (int)sizeof( b2ParticleBodyContactData ), (int)_Alignof( b2ParticleBodyContactData ), newIndices,
							   oldCount, scratchBase );
	system->previousBodyContactCount = b2RemapParticleBuffer(
		world, system, b2_particleRemapBodyContacts, system->previousBodyContacts, system->previousBodyContactCount,
		(int)sizeof( b2ParticleBodyContactData ), (int)_Alignof( b2ParticleBodyContactData ), newIndices, oldCount,
		scratchBase );
	system->bodyContactBeginCount = b2RemapParticleBuffer(
		world, system, b2_particleRemapBodyContacts, system->bodyContactBeginEvents, system->bodyContactBeginCount,
		(int)sizeof( b2ParticleBodyContactData ), (int)_Alignof( b2ParticleBodyContactData ), newIndices, oldCount,
		scratchBase );
	system->bodyContactEndCount =
		b2RemapParticleBuffer( world, system, b2_particleRemapBodyContacts, system->bodyContactEndEvents,
							   system->bodyContactEndCount, (int)sizeof( b2ParticleBodyContactData ),
							   (int)_Alignof( b2ParticleBodyContactData ), newIndices, oldCount, scratchBase );

	system->pairCount = b2RemapParticleBuffer( world, system, b2_particleRemapPairs, system->pairs, system->pairCount,
											  (int)sizeof( b2ParticlePairData ), (int)_Alignof( b2ParticlePairData ), newIndices,
											  oldCount, scratchBase );
	system->triadCount = b2RemapParticleBuffer( world, system, b2_particleRemapTriads, system->triads, system->triadCount,
											   (int)sizeof( b2ParticleTriadData ), (int)_Alignof( b2ParticleTriadData ),
											   newIndices, oldCount, scratchBase );
}

static int b2ReorderParticles( b2ParticleSystem* system, const int* order, int targetOldIndex )
{
	int count = system->particleCount;
	if ( count == 0 )
	{
		return B2_NULL_INDEX;
	}

	bool identity = true;
	for ( int i = 0; i < count; ++i )
	{
		if ( order[i] != i )
		{
			identity = false;
			break;
		}
	}

	if ( identity )
	{
		b2RefreshGroups( system );
		return targetOldIndex;
	}

	b2ParticleState* oldStates = b2Alloc( count * (int)sizeof( b2ParticleState ) );
	int* newIndices = b2Alloc( count * (int)sizeof( int ) );
	for ( int i = 0; i < count; ++i )
	{
		oldStates[i] = b2ReadParticleState( system, i );
		newIndices[i] = B2_NULL_INDEX;
	}

	int targetNewIndex = B2_NULL_INDEX;
	for ( int newIndex = 0; newIndex < count; ++newIndex )
	{
		int oldIndex = order[newIndex];
		B2_ASSERT( 0 <= oldIndex && oldIndex < count );
		b2WriteParticleState( system, newIndex, oldStates + oldIndex );
		newIndices[oldIndex] = newIndex;
		if ( oldIndex == targetOldIndex )
		{
			targetNewIndex = newIndex;
		}
	}

	b2World* world = b2TryGetWorld( system->worldId );
	b2ParticleScratchBuffer_Reset( &system->scratch );
	b2RemapParticleSideBuffers( world, system, newIndices, count, 0 );
	b2ParticleScratchBuffer_Reset( &system->scratch );
	b2Free( newIndices, count * (int)sizeof( int ) );
	b2Free( oldStates, count * (int)sizeof( b2ParticleState ) );
	b2RefreshGroups( system );
	return targetNewIndex;
}

static int b2ReorderParticlesByGroup( b2ParticleSystem* system, int targetOldIndex )
{
	int count = system->particleCount;
	if ( count == 0 )
	{
		b2RefreshGroups( system );
		return B2_NULL_INDEX;
	}

	int* order = b2Alloc( count * (int)sizeof( int ) );
	bool* used = b2AllocZeroInit( count * (int)sizeof( bool ) );
	int writeIndex = 0;

	for ( int groupIndex = 0; groupIndex < system->groupCount; ++groupIndex )
	{
		b2ParticleGroup* group = system->groups + groupIndex;
		if ( group->id == B2_NULL_INDEX )
		{
			continue;
		}

		for ( int i = 0; i < count; ++i )
		{
			if ( used[i] == false && system->groupIndices[i] == group->localIndex )
			{
				used[i] = true;
				order[writeIndex++] = i;
			}
		}
	}

	for ( int i = 0; i < count; ++i )
	{
		if ( used[i] == false )
		{
			order[writeIndex++] = i;
		}
	}

	B2_ASSERT( writeIndex == count );
	int targetNewIndex = b2ReorderParticles( system, order, targetOldIndex );
	b2Free( used, count * (int)sizeof( bool ) );
	b2Free( order, count * (int)sizeof( int ) );
	return targetNewIndex;
}

static void b2EmitParticleDestructionEvent( b2ParticleSystem* system, int particleIndex );

static void b2RemoveZombieParticles( b2ParticleSystem* system )
{
	uint64_t ticks = b2GetTicks();
	int oldCount = system->particleCount;
	if ( oldCount == 0 )
	{
		return;
	}

	b2World* world = b2TryGetWorld( system->worldId );
	int blockSize = B2_PARTICLE_TASK_MIN_RANGE;
	int blockCount = b2GetParticleBlockCount( oldCount );
	uint64_t scratchTicks = b2GetTicks();
	b2ParticleScratchBuffer_Reset( &system->scratch );
	b2ParticleScratchBuffer_Reserve( &system->scratch, b2GetParticleCompactionScratchByteCount( system, oldCount, blockCount ) );
	int* newIndices = b2ParticleScratchBuffer_AllocArray( &system->scratch, oldCount, int );
	int remapScratchBase = system->scratch.byteCount;
	int* blockLiveCounts = b2ParticleScratchBuffer_AllocArray( &system->scratch, blockCount, int );
	int* blockOffsets = b2ParticleScratchBuffer_AllocArray( &system->scratch, blockCount, int );
	int* blockMoveCounts = b2ParticleScratchBuffer_AllocArray( &system->scratch, blockCount, int );
	system->profile.scratch += b2GetMilliseconds( scratchTicks );

	b2ParticleCompactionTaskContext context = {
		.system = system,
		.type = b2_particleCompactionTaskMark,
		.oldCount = oldCount,
		.blockSize = blockSize,
		.newIndices = newIndices,
		.blockOffsets = blockOffsets,
		.blockLiveCounts = blockLiveCounts,
		.blockMoveCounts = blockMoveCounts,
	};
	uint64_t phaseTicks = b2GetTicks();
	b2RunParticleTask( world, system, b2ParticleCompactionTask, blockCount, 1, &context );
	system->profile.compactionMark += b2GetMilliseconds( phaseTicks );

	phaseTicks = b2GetTicks();
	int writeIndex = b2BuildParticleBlockOffsets( blockLiveCounts, blockOffsets, blockCount );
	if ( writeIndex == oldCount )
	{
		b2ParticleScratchBuffer_Reset( &system->scratch );
		system->profile.compactionPrefix += b2GetMilliseconds( phaseTicks );
		system->profile.compaction += b2GetMilliseconds( ticks );
		return;
	}

	context.type = b2_particleCompactionTaskFinalize;
	b2RunParticleTask( world, system, b2ParticleCompactionTask, blockCount, 1, &context );

	for ( int blockIndex = 0; blockIndex < blockCount; ++blockIndex )
	{
		system->compactionMoveCount += blockMoveCounts[blockIndex];
	}
	system->profile.compactionPrefix += b2GetMilliseconds( phaseTicks );

	for ( int readIndex = 0; readIndex < oldCount; ++readIndex )
	{
		if ( ( system->flags[readIndex] & b2_zombieParticle ) != 0 )
		{
			b2EmitParticleDestructionEvent( system, readIndex );
		}
	}

	scratchTicks = b2GetTicks();
	context.buffers = b2AllocateParticleCoreBuffers( system, writeIndex );
	system->profile.scratch += b2GetMilliseconds( scratchTicks );

	phaseTicks = b2GetTicks();
	context.type = b2_particleCompactionTaskScatter;
	b2RunParticleTask( world, system, b2ParticleCompactionTask, blockCount, 1, &context );

	if ( writeIndex > 0 )
	{
		context.type = b2_particleCompactionTaskPublish;
		b2RunParticleTask( world, system, b2ParticleCompactionTask, writeIndex, B2_PARTICLE_TASK_MIN_RANGE, &context );
	}
	system->profile.compactionScatter += b2GetMilliseconds( phaseTicks );

	system->particleCount = writeIndex;
	system->scratch.byteCount = remapScratchBase;
	phaseTicks = b2GetTicks();
	b2RemapParticleSideBuffers( world, system, newIndices, oldCount, remapScratchBase );
	system->profile.compactionRemap += b2GetMilliseconds( phaseTicks );
	system->compactionRemapCount += oldCount;
	b2RefreshGroups( system );
	b2ParticleScratchBuffer_Reset( &system->scratch );
	system->profile.compaction += b2GetMilliseconds( ticks );
}

static int b2GetOldestParticleIndex( const b2ParticleSystem* system )
{
	if ( system->particleCount == 0 )
	{
		return B2_NULL_INDEX;
	}

	int oldestIndex = 0;
	float oldestAge = system->ages[0];
	for ( int i = 1; i < system->particleCount; ++i )
	{
		if ( system->ages[i] > oldestAge )
		{
			oldestAge = system->ages[i];
			oldestIndex = i;
		}
	}

	return oldestIndex;
}

static int b2AdmitParticle( b2ParticleSystem* system )
{
	if ( system->def.maxCount <= 0 || system->particleCount < system->def.maxCount )
	{
		return 0;
	}

	if ( system->def.destroyByAge == false || system->particleCount == 0 )
	{
		return -1;
	}

	int oldestIndex = b2GetOldestParticleIndex( system );
	system->flags[oldestIndex] |= b2_zombieParticle;
	b2RemoveZombieParticles( system );
	return system->particleCount < system->def.maxCount ? 0 : -1;
}

static void b2EmitParticleDestructionEvent( b2ParticleSystem* system, int particleIndex )
{
	if ( system->def.enableParticleEvents == false && ( system->flags[particleIndex] & b2_destructionListenerParticle ) == 0 )
	{
		return;
	}

	b2ReserveDestructionEvents( system, system->destructionEventCount + 1 );
	system->destructionEvents[system->destructionEventCount++] = (b2ParticleDestructionEvent){
		.systemId = { system->id + 1, system->worldId, system->generation },
		.particleIndex = particleIndex,
		.userData = system->userDataBuffer[particleIndex],
	};
}

static float b2QuantizeParticleLifetime( const b2ParticleSystem* system, float lifetime );

static int b2AppendParticle( b2ParticleSystem* system, const b2ParticleDef* def, int groupIndex )
{
	if ( b2AdmitParticle( system ) != 0 )
	{
		return B2_NULL_INDEX;
	}

	int index = system->particleCount;
	b2ReserveParticles( system, index + 1 );
	system->particleCount += 1;
	system->positions[index] = def->position;
	system->velocities[index] = def->velocity;
	system->forces[index] = b2Vec2_zero;
	system->flags[index] = def->flags;
	system->allParticleFlags |= def->flags;
	system->colors[index] = def->color;
	system->userDataBuffer[index] = def->userData;
	system->weights[index] = 0.0f;
	system->lifetimes[index] = b2QuantizeParticleLifetime( system, def->lifetime );
	system->ages[index] = 0.0f;
	system->accumulations[index] = 0.0f;
	system->staticPressures[index] = 0.0f;
	system->depths[index] = 0.0f;
	system->accumulationVectors[index] = b2Vec2_zero;
	system->groupIndices[index] = groupIndex;

	return index;
}

int b2ParticleSystem_CreateParticle( b2ParticleSystemId systemId, const b2ParticleDef* def )
{
	B2_CHECK_DEF( def );

	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system == NULL )
	{
		return B2_NULL_INDEX;
	}

	int groupIndex = B2_NULL_INDEX;
	if ( B2_IS_NON_NULL( def->groupId ) )
	{
		b2ParticleSystem* groupSystem = NULL;
		b2ParticleGroup* group = b2TryGetParticleGroup( def->groupId, &groupSystem );
		if ( group == NULL || groupSystem != system )
		{
			return B2_NULL_INDEX;
		}
		groupIndex = group->localIndex;
	}

	int index = b2AppendParticle( system, def, groupIndex );
	if ( index == B2_NULL_INDEX )
	{
		return B2_NULL_INDEX;
	}

	if ( groupIndex != B2_NULL_INDEX )
	{
		return b2ReorderParticlesByGroup( system, index );
	}

	b2RefreshGroups( system );
	return index;
}

void b2ParticleSystem_DestroyParticle( b2ParticleSystemId systemId, int particleIndex )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system == NULL || particleIndex < 0 || system->particleCount <= particleIndex )
	{
		return;
	}

	system->flags[particleIndex] |= b2_zombieParticle;
	b2RemoveZombieParticles( system );
}

static bool b2ParticlePointInCircle( const b2Circle* circle, b2Transform transform, b2Vec2 point )
{
	b2Vec2 localPoint = b2InvTransformPoint( transform, point );
	return b2PointInCircle( circle, localPoint );
}

static bool b2ParticlePointInCapsule( const b2Capsule* capsule, b2Transform transform, b2Vec2 point )
{
	b2Vec2 localPoint = b2InvTransformPoint( transform, point );
	return b2PointInCapsule( capsule, localPoint );
}

static bool b2ParticlePointInPolygon( const b2Polygon* polygon, b2Transform transform, b2Vec2 point )
{
	b2Vec2 localPoint = b2InvTransformPoint( transform, point );
	return b2PointInPolygon( polygon, localPoint );
}

static bool b2ParticlePointNearSegment( const b2Segment* segment, b2Transform transform, b2Vec2 point, float radius )
{
	b2Vec2 localPoint = b2InvTransformPoint( transform, point );
	b2Vec2 segmentDelta = b2Sub( segment->point2, segment->point1 );
	float lengthSquared = b2LengthSquared( segmentDelta );
	float fraction = lengthSquared > 0.0f ? b2ClampFloat( b2Dot( b2Sub( localPoint, segment->point1 ), segmentDelta ) / lengthSquared, 0.0f, 1.0f )
										  : 0.0f;
	b2Vec2 closest = b2MulAdd( segment->point1, fraction, segmentDelta );
	return b2DistanceSquared( localPoint, closest ) <= radius * radius;
}

static bool b2ShouldDestroyParticleByQuery( b2ParticleSystem* system, int particleIndex )
{
	if ( system->queryFilterFcn == NULL )
	{
		return true;
	}

	b2ParticleSystemId systemId = { system->id + 1, system->worldId, system->generation };
	return system->queryFilterFcn( systemId, particleIndex, system->queryFilterContext );
}

int b2ParticleSystem_DestroyParticlesInCircle( b2ParticleSystemId systemId, const b2Circle* circle, b2Transform transform )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system == NULL || circle == NULL )
	{
		return 0;
	}

	int count = 0;
	for ( int i = 0; i < system->particleCount; ++i )
	{
		if ( b2ParticlePointInCircle( circle, transform, system->positions[i] ) && b2ShouldDestroyParticleByQuery( system, i ) )
		{
			system->flags[i] |= b2_zombieParticle;
			count += 1;
		}
	}
	b2RemoveZombieParticles( system );
	return count;
}

int b2ParticleSystem_DestroyParticlesInSegment( b2ParticleSystemId systemId, const b2Segment* segment, b2Transform transform )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system == NULL || segment == NULL )
	{
		return 0;
	}

	int count = 0;
	for ( int i = 0; i < system->particleCount; ++i )
	{
		if ( b2ParticlePointNearSegment( segment, transform, system->positions[i], system->def.radius ) &&
			 b2ShouldDestroyParticleByQuery( system, i ) )
		{
			system->flags[i] |= b2_zombieParticle;
			count += 1;
		}
	}
	b2RemoveZombieParticles( system );
	return count;
}

int b2ParticleSystem_DestroyParticlesInCapsule( b2ParticleSystemId systemId, const b2Capsule* capsule, b2Transform transform )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system == NULL || capsule == NULL )
	{
		return 0;
	}

	int count = 0;
	for ( int i = 0; i < system->particleCount; ++i )
	{
		if ( b2ParticlePointInCapsule( capsule, transform, system->positions[i] ) && b2ShouldDestroyParticleByQuery( system, i ) )
		{
			system->flags[i] |= b2_zombieParticle;
			count += 1;
		}
	}
	b2RemoveZombieParticles( system );
	return count;
}

int b2ParticleSystem_DestroyParticlesInPolygon( b2ParticleSystemId systemId, const b2Polygon* polygon, b2Transform transform )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system == NULL || polygon == NULL )
	{
		return 0;
	}

	int count = 0;
	for ( int i = 0; i < system->particleCount; ++i )
	{
		if ( b2ParticlePointInPolygon( polygon, transform, system->positions[i] ) && b2ShouldDestroyParticleByQuery( system, i ) )
		{
			system->flags[i] |= b2_zombieParticle;
			count += 1;
		}
	}
	b2RemoveZombieParticles( system );
	return count;
}

int b2ParticleSystem_GetParticleCount( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->particleCount : 0;
}

int b2ParticleSystem_GetMaxParticleCount( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->def.maxCount : 0;
}

void b2ParticleSystem_SetMaxParticleCount( b2ParticleSystemId systemId, int count )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL )
	{
		system->def.maxCount = b2MaxInt( 0, count );
		while ( system->def.maxCount > 0 && system->particleCount > system->def.maxCount )
		{
			int oldestIndex = b2GetOldestParticleIndex( system );
			if ( oldestIndex == B2_NULL_INDEX )
			{
				break;
			}
			system->flags[oldestIndex] |= b2_zombieParticle;
			b2RemoveZombieParticles( system );
		}
	}
}

void b2ParticleSystem_SetParticleLifetime( b2ParticleSystemId systemId, int particleIndex, float lifetime )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL && 0 <= particleIndex && particleIndex < system->particleCount )
	{
		system->lifetimes[particleIndex] = b2QuantizeParticleLifetime( system, lifetime );
		system->ages[particleIndex] = 0.0f;
	}
}

float b2ParticleSystem_GetParticleLifetime( b2ParticleSystemId systemId, int particleIndex )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system == NULL || particleIndex < 0 || system->particleCount <= particleIndex )
	{
		return 0.0f;
	}
	return system->lifetimes[particleIndex];
}

static b2ParticleGroup* b2CreateGroup( b2World* world, b2ParticleSystem* system, const b2ParticleGroupDef* def )
{
	if ( B2_IS_NON_NULL( def->groupId ) )
	{
		b2ParticleSystem* groupSystem = NULL;
		b2ParticleGroup* group = b2TryGetParticleGroup( def->groupId, &groupSystem );
		return groupSystem == system ? group : NULL;
	}

	int localIndex = b2AllocId( &system->groupIdPool );
	int groupId = b2AllocId( &world->particleGroupIdPool );
	b2ReserveGroups( system, localIndex + 1 );
	system->groupCount = b2MaxInt( system->groupCount, localIndex + 1 );

	b2ParticleGroup* group = system->groups + localIndex;
	uint16_t generation = group->generation + 1;
	*group = (b2ParticleGroup){ 0 };
	group->id = groupId;
	group->localIndex = localIndex;
	group->systemId = system->id;
	group->firstIndex = system->particleCount;
	group->flags = def->flags;
	group->groupFlags = def->groupFlags;
	group->color = def->color;
	group->strength = def->strength;
	group->transform = (b2Transform){ def->position, def->rotation };
	group->userData = def->userData;
	group->generation = generation;
	return group;
}

static b2Vec2 b2ParticleGroupVelocity( const b2ParticleGroupDef* def, b2Vec2 position )
{
	b2Vec2 r = b2Sub( position, def->position );
	b2Vec2 angular = { -def->angularVelocity * r.y, def->angularVelocity * r.x };
	return b2Add( def->linearVelocity, angular );
}

static int b2CreateGroupParticle( b2ParticleSystem* system, int groupIndex, const b2ParticleGroupDef* groupDef,
								  b2Vec2 position )
{
	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.flags = groupDef->flags;
	particleDef.position = position;
	particleDef.velocity = b2ParticleGroupVelocity( groupDef, position );
	particleDef.color = groupDef->color;
	particleDef.lifetime = groupDef->lifetime;
	particleDef.userData = groupDef->userData;
	return b2AppendParticle( system, &particleDef, groupIndex );
}

static void b2RollbackParticleGroupCreation( b2World* world, b2ParticleSystem* system, b2ParticleGroup* group, int oldParticleCount,
											 bool destroyGroup )
{
	int firstIndex = destroyGroup ? 0 : oldParticleCount;
	for ( int i = firstIndex; i < system->particleCount; ++i )
	{
		if ( system->groupIndices[i] == group->localIndex )
		{
			system->flags[i] |= b2_zombieParticle;
		}
	}

	if ( destroyGroup )
	{
		int id = group->id;
		int localIndex = group->localIndex;
		uint16_t generation = group->generation + 1;
		*group = (b2ParticleGroup){ .id = B2_NULL_INDEX, .generation = generation };
		b2FreeId( &world->particleGroupIdPool, id );
		b2FreeId( &system->groupIdPool, localIndex );
	}

	b2RemoveZombieParticles( system );
}

static bool b2SampleAABB( b2ParticleSystem* system, int groupIndex, const b2ParticleGroupDef* def, b2Transform transform,
						  b2AABB aabb, bool ( *testFcn )( const b2ParticleGroupDef*, b2Transform, b2Vec2 ) )
{
	float diameter = 2.0f * system->def.radius;
	float stride = def->stride > 0.0f ? def->stride : B2_PARTICLE_STRIDE * diameter;
	for ( float y = floorf( aabb.lowerBound.y / stride ) * stride; y < aabb.upperBound.y; y += stride )
	{
		for ( float x = floorf( aabb.lowerBound.x / stride ) * stride; x < aabb.upperBound.x; x += stride )
		{
			b2Vec2 p = { x, y };
			if ( testFcn( def, transform, p ) )
			{
				if ( b2CreateGroupParticle( system, groupIndex, def, p ) == B2_NULL_INDEX )
				{
					return false;
				}
			}
		}
	}
	return true;
}

static bool b2TestGroupCircle( const b2ParticleGroupDef* def, b2Transform transform, b2Vec2 point )
{
	return b2ParticlePointInCircle( def->circle, transform, point );
}

static bool b2TestGroupCapsule( const b2ParticleGroupDef* def, b2Transform transform, b2Vec2 point )
{
	return b2ParticlePointInCapsule( def->capsule, transform, point );
}

static bool b2TestGroupPolygon( const b2ParticleGroupDef* def, b2Transform transform, b2Vec2 point )
{
	return b2ParticlePointInPolygon( def->polygon, transform, point );
}

static void b2UpdateParticleContacts( b2ParticleSystem* system );
static void b2UpdateParticlePairsAndTriads( b2ParticleSystem* system, int firstIndex, int lastIndex, bool reactiveOnly );

b2ParticleGroupId b2ParticleSystem_CreateParticleGroup( b2ParticleSystemId systemId, const b2ParticleGroupDef* def )
{
	B2_CHECK_DEF( def );
	b2World* world = b2GetWorldFromParticleSystemId( systemId );
	b2ParticleSystem* system = b2GetParticleSystemFullId( world, systemId );
	b2ParticleGroup* group = b2CreateGroup( world, system, def );
	if ( group == NULL )
	{
		return b2_nullParticleGroupId;
	}

	b2ParticleGroupId groupId = b2MakeParticleGroupId( world, system, group->localIndex );
	b2Transform transform = { def->position, def->rotation };
	int oldParticleCount = system->particleCount;
	bool createdNewGroup = B2_IS_NULL( def->groupId );
	bool success = true;

	if ( def->positionData != NULL && def->particleCount > 0 )
	{
		for ( int i = 0; i < def->particleCount; ++i )
		{
			if ( b2CreateGroupParticle( system, group->localIndex, def, b2TransformPoint( transform, def->positionData[i] ) ) ==
				 B2_NULL_INDEX )
			{
				success = false;
				break;
			}
		}
	}

	if ( success && def->circle != NULL )
	{
		b2AABB aabb = b2ComputeCircleAABB( def->circle, transform );
		success = b2SampleAABB( system, group->localIndex, def, transform, aabb, b2TestGroupCircle );
	}

	if ( success && def->capsule != NULL )
	{
		b2AABB aabb = b2ComputeCapsuleAABB( def->capsule, transform );
		success = b2SampleAABB( system, group->localIndex, def, transform, aabb, b2TestGroupCapsule );
	}

	if ( success && def->polygon != NULL )
	{
		b2AABB aabb = b2ComputePolygonAABB( def->polygon, transform );
		success = b2SampleAABB( system, group->localIndex, def, transform, aabb, b2TestGroupPolygon );
	}

	if ( success && def->segment != NULL )
	{
		b2Vec2 p1 = b2TransformPoint( transform, def->segment->point1 );
		b2Vec2 p2 = b2TransformPoint( transform, def->segment->point2 );
		b2Vec2 d = b2Sub( p2, p1 );
		float length = b2Length( d );
		float diameter = 2.0f * system->def.radius;
		float stride = def->stride > 0.0f ? def->stride : B2_PARTICLE_STRIDE * diameter;
		int count = b2MaxInt( 1, (int)floorf( length / stride ) + 1 );
		for ( int i = 0; i < count; ++i )
		{
			float fraction = count == 1 ? 0.0f : (float)i / (float)( count - 1 );
			if ( b2CreateGroupParticle( system, group->localIndex, def, b2Lerp( p1, p2, fraction ) ) == B2_NULL_INDEX )
			{
				success = false;
				break;
			}
		}
	}

	if ( success == false )
	{
		b2RollbackParticleGroupCreation( world, system, group, oldParticleCount, createdNewGroup );
		return b2_nullParticleGroupId;
	}

	b2ReorderParticlesByGroup( system, B2_NULL_INDEX );
	if ( b2ParticleGroup_IsValid( groupId ) == false )
	{
		return b2_nullParticleGroupId;
	}

	b2UpdateParticleContacts( system );
	b2UpdateParticlePairsAndTriads( system, group->firstIndex, group->firstIndex + group->count, false );

	return groupId;
}

void b2DestroyParticleGroup( b2ParticleGroupId groupId )
{
	b2ParticleSystem* system = NULL;
	b2ParticleGroup* group = b2TryGetParticleGroup( groupId, &system );
	if ( group == NULL )
	{
		return;
	}

	for ( int i = 0; i < system->particleCount; ++i )
	{
		if ( system->groupIndices[i] == group->localIndex )
		{
			system->flags[i] |= b2_zombieParticle;
		}
	}

	int id = group->id;
	int localIndex = group->localIndex;
	uint16_t generation = group->generation + 1;
	*group = (b2ParticleGroup){ .id = B2_NULL_INDEX, .generation = generation };
	b2World* world = b2TryGetWorld( groupId.world0 );
	if ( world != NULL )
	{
		b2FreeId( &world->particleGroupIdPool, id );
	}
	b2FreeId( &system->groupIdPool, localIndex );
	b2RemoveZombieParticles( system );
}

bool b2ParticleGroup_IsValid( b2ParticleGroupId groupId )
{
	return b2TryGetParticleGroup( groupId, NULL ) != NULL;
}

int b2ParticleGroup_GetParticleCount( b2ParticleGroupId groupId )
{
	b2ParticleGroup* group = b2TryGetParticleGroup( groupId, NULL );
	return group != NULL ? group->count : 0;
}

void b2ParticleGroup_GetParticleRange( b2ParticleGroupId groupId, int* startIndex, int* count )
{
	b2ParticleGroup* group = b2TryGetParticleGroup( groupId, NULL );
	if ( group == NULL )
	{
		if ( startIndex != NULL )
		{
			*startIndex = B2_NULL_INDEX;
		}
		if ( count != NULL )
		{
			*count = 0;
		}
		return;
	}

	if ( startIndex != NULL )
	{
		*startIndex = group->firstIndex;
	}
	if ( count != NULL )
	{
		*count = group->count;
	}
}

float b2ParticleGroup_GetMass( b2ParticleGroupId groupId )
{
	b2ParticleGroup* group = b2TryGetParticleGroup( groupId, NULL );
	return group != NULL ? group->mass : 0.0f;
}

float b2ParticleGroup_GetInertia( b2ParticleGroupId groupId )
{
	b2ParticleGroup* group = b2TryGetParticleGroup( groupId, NULL );
	return group != NULL ? group->inertia : 0.0f;
}

b2Vec2 b2ParticleGroup_GetCenter( b2ParticleGroupId groupId )
{
	b2ParticleGroup* group = b2TryGetParticleGroup( groupId, NULL );
	return group != NULL ? group->center : b2Vec2_zero;
}

b2Vec2 b2ParticleGroup_GetLinearVelocity( b2ParticleGroupId groupId )
{
	b2ParticleGroup* group = b2TryGetParticleGroup( groupId, NULL );
	return group != NULL ? group->linearVelocity : b2Vec2_zero;
}

float b2ParticleGroup_GetAngularVelocity( b2ParticleGroupId groupId )
{
	b2ParticleGroup* group = b2TryGetParticleGroup( groupId, NULL );
	return group != NULL ? group->angularVelocity : 0.0f;
}

b2Transform b2ParticleGroup_GetTransform( b2ParticleGroupId groupId )
{
	b2ParticleGroup* group = b2TryGetParticleGroup( groupId, NULL );
	return group != NULL ? group->transform : b2Transform_identity;
}

float b2ParticleGroup_GetAngle( b2ParticleGroupId groupId )
{
	b2ParticleGroup* group = b2TryGetParticleGroup( groupId, NULL );
	return group != NULL ? b2Rot_GetAngle( group->transform.q ) : 0.0f;
}

b2Vec2 b2ParticleGroup_GetLinearVelocityFromWorldPoint( b2ParticleGroupId groupId, b2Vec2 worldPoint )
{
	b2ParticleGroup* group = b2TryGetParticleGroup( groupId, NULL );
	if ( group == NULL )
	{
		return b2Vec2_zero;
	}

	return b2Add( group->linearVelocity, b2CrossSV( group->angularVelocity, b2Sub( worldPoint, group->center ) ) );
}

void b2ParticleGroup_Join( b2ParticleGroupId groupA, b2ParticleGroupId groupB )
{
	b2ParticleSystem* systemA = NULL;
	b2ParticleSystem* systemB = NULL;
	b2ParticleGroup* a = b2TryGetParticleGroup( groupA, &systemA );
	b2ParticleGroup* b = b2TryGetParticleGroup( groupB, &systemB );
	if ( a == NULL || b == NULL || systemA != systemB || a == b )
	{
		return;
	}

	for ( int i = 0; i < systemA->particleCount; ++i )
	{
		if ( systemA->groupIndices[i] == b->localIndex )
		{
			systemA->groupIndices[i] = a->localIndex;
		}
	}
	a->groupFlags |= b->groupFlags;
	a->flags |= b->flags;

	int id = b->id;
	int localIndex = b->localIndex;
	uint16_t generation = b->generation + 1;
	*b = (b2ParticleGroup){ .id = B2_NULL_INDEX, .generation = generation };
	b2World* world = b2TryGetWorld( groupB.world0 );
	if ( world != NULL )
	{
		b2FreeId( &world->particleGroupIdPool, id );
	}
	b2FreeId( &systemA->groupIdPool, localIndex );
	b2ReorderParticlesByGroup( systemA, B2_NULL_INDEX );
	b2UpdateParticleContacts( systemA );
	if ( a->id != B2_NULL_INDEX && a->count > 0 )
	{
		b2UpdateParticlePairsAndTriads( systemA, a->firstIndex, a->firstIndex + a->count, false );
	}
}

void b2ParticleGroup_Split( b2ParticleGroupId groupId )
{
	b2ParticleSystem* system = NULL;
	b2ParticleGroup* group = b2TryGetParticleGroup( groupId, &system );
	if ( group == NULL || group->count <= 1 )
	{
		return;
	}

	b2UpdateParticleContacts( system );

	int firstIndex = group->firstIndex;
	int count = group->count;
	int* parents = b2Alloc( count * (int)sizeof( int ) );
	int* ranks = b2AllocZeroInit( count * (int)sizeof( int ) );
	for ( int i = 0; i < count; ++i )
	{
		parents[i] = i;
	}

	for ( int contactIndex = 0; contactIndex < system->contactCount; ++contactIndex )
	{
		b2ParticleContactData contact = system->contacts[contactIndex];
		if ( system->groupIndices[contact.indexA] != group->localIndex || system->groupIndices[contact.indexB] != group->localIndex )
		{
			continue;
		}

		int a = contact.indexA - firstIndex;
		int b = contact.indexB - firstIndex;
		if ( a < 0 || count <= a || b < 0 || count <= b )
		{
			continue;
		}

		int rootA = a;
		while ( parents[rootA] != rootA )
		{
			rootA = parents[rootA];
		}

		int rootB = b;
		while ( parents[rootB] != rootB )
		{
			rootB = parents[rootB];
		}

		if ( rootA == rootB )
		{
			continue;
		}

		if ( ranks[rootA] < ranks[rootB] )
		{
			parents[rootA] = rootB;
		}
		else if ( ranks[rootA] > ranks[rootB] )
		{
			parents[rootB] = rootA;
		}
		else
		{
			parents[rootB] = rootA;
			ranks[rootA] += 1;
		}
	}

	int* componentForRoot = b2Alloc( count * (int)sizeof( int ) );
	int* componentForParticle = b2Alloc( count * (int)sizeof( int ) );
	int* componentCounts = b2AllocZeroInit( count * (int)sizeof( int ) );
	for ( int i = 0; i < count; ++i )
	{
		componentForRoot[i] = B2_NULL_INDEX;
	}

	int componentCount = 0;
	for ( int i = 0; i < count; ++i )
	{
		int root = i;
		while ( parents[root] != root )
		{
			root = parents[root];
		}

		int component = componentForRoot[root];
		if ( component == B2_NULL_INDEX )
		{
			component = componentCount++;
			componentForRoot[root] = component;
		}

		componentForParticle[i] = component;
		componentCounts[component] += 1;
	}

	if ( componentCount <= 1 )
	{
		b2Free( componentCounts, count * (int)sizeof( int ) );
		b2Free( componentForParticle, count * (int)sizeof( int ) );
		b2Free( componentForRoot, count * (int)sizeof( int ) );
		b2Free( ranks, count * (int)sizeof( int ) );
		b2Free( parents, count * (int)sizeof( int ) );
		return;
	}

	int largestComponent = 0;
	for ( int i = 1; i < componentCount; ++i )
	{
		if ( componentCounts[largestComponent] < componentCounts[i] )
		{
			largestComponent = i;
		}
	}

	int* componentGroups = b2Alloc( componentCount * (int)sizeof( int ) );
	for ( int i = 0; i < componentCount; ++i )
	{
		componentGroups[i] = group->localIndex;
	}

	b2World* world = b2TryGetWorld( system->worldId );
	if ( world != NULL )
	{
		for ( int component = 0; component < componentCount; ++component )
		{
			if ( component == largestComponent )
			{
				continue;
			}

			b2ParticleGroupDef def = b2DefaultParticleGroupDef();
			def.flags = group->flags;
			def.groupFlags = group->groupFlags;
			def.color = group->color;
			def.strength = group->strength;
			def.userData = group->userData;
			b2ParticleGroup* newGroup = b2CreateGroup( world, system, &def );
			if ( newGroup != NULL )
			{
				componentGroups[component] = newGroup->localIndex;
			}
		}
	}

	for ( int i = 0; i < count; ++i )
	{
		int particleIndex = firstIndex + i;
		int component = componentForParticle[i];
		system->groupIndices[particleIndex] = componentGroups[component];
	}

	b2ReorderParticlesByGroup( system, B2_NULL_INDEX );
	b2Free( componentGroups, componentCount * (int)sizeof( int ) );
	b2Free( componentCounts, count * (int)sizeof( int ) );
	b2Free( componentForParticle, count * (int)sizeof( int ) );
	b2Free( componentForRoot, count * (int)sizeof( int ) );
	b2Free( ranks, count * (int)sizeof( int ) );
	b2Free( parents, count * (int)sizeof( int ) );
}

typedef struct b2SortableParticlePair
{
	uint64_t key;
	b2ParticlePairData pair;
} b2SortableParticlePair;

typedef struct b2SortableParticleTriad
{
	uint64_t key;
	b2ParticleTriadData triad;
} b2SortableParticleTriad;

static uint64_t b2ParticlePairKey( int indexA, int indexB )
{
	return ( (uint64_t)(uint32_t)indexA << 32 ) | (uint64_t)(uint32_t)indexB;
}

static void* b2AllocateParticleSortRecords( b2ParticleSystem* system, int count, int recordSize, int alignment, void** sortScratch )
{
	uint64_t ticks = b2GetTicks();
	int scratchByteCount = b2GetParticleSortScratchByteCount( count, recordSize );
	int recordByteCount = b2ParticleScratchByteCount( count, (size_t)recordSize );
	int reserveByteCount = recordByteCount + scratchByteCount + 2 * ( alignment - 1 );
	b2ParticleScratchBuffer_Reset( &system->scratch );
	b2ParticleScratchBuffer_Reserve( &system->scratch, reserveByteCount );

	void* records = b2ParticleScratchBuffer_Alloc( &system->scratch, recordByteCount, alignment );
	*sortScratch = scratchByteCount > 0 ? b2ParticleScratchBuffer_Alloc( &system->scratch, scratchByteCount, alignment ) : NULL;
	system->profile.scratch += b2GetMilliseconds( ticks );
	return records;
}

static void b2SortParticlePairs( b2ParticleSystem* system, b2ParticlePairData* pairs, int count )
{
	if ( count <= 1 )
	{
		return;
	}

	void* sortScratch = NULL;
	b2SortableParticlePair* records = b2AllocateParticleSortRecords(
		system, count, (int)sizeof( b2SortableParticlePair ), (int)_Alignof( b2SortableParticlePair ), &sortScratch );
	for ( int i = 0; i < count; ++i )
	{
		records[i] = (b2SortableParticlePair){ b2ParticlePairKey( pairs[i].indexA, pairs[i].indexB ), pairs[i] };
	}

	b2SortParticleRecordsByKeyWithScratch( records, sortScratch, count, (int)sizeof( b2SortableParticlePair ), 0 );
	for ( int i = 0; i < count; ++i )
	{
		pairs[i] = records[i].pair;
	}

	b2ParticleScratchBuffer_Reset( &system->scratch );
}

static void b2SortParticleTriads( b2ParticleSystem* system, b2ParticleTriadData* triads, int count )
{
	if ( count <= 1 )
	{
		return;
	}

	void* sortScratch = NULL;
	b2SortableParticleTriad* records = b2AllocateParticleSortRecords(
		system, count, (int)sizeof( b2SortableParticleTriad ), (int)_Alignof( b2SortableParticleTriad ), &sortScratch );
	for ( int i = 0; i < count; ++i )
	{
		records[i] = (b2SortableParticleTriad){ (uint64_t)(uint32_t)triads[i].indexC, triads[i] };
	}

	b2SortParticleRecordsByKeyWithScratch( records, sortScratch, count, (int)sizeof( b2SortableParticleTriad ), 0 );
	for ( int i = 0; i < count; ++i )
	{
		records[i].key = b2ParticlePairKey( records[i].triad.indexA, records[i].triad.indexB );
	}
	b2SortParticleRecordsByKeyWithScratch( records, sortScratch, count, (int)sizeof( b2SortableParticleTriad ), 0 );

	for ( int i = 0; i < count; ++i )
	{
		triads[i] = records[i].triad;
	}

	b2ParticleScratchBuffer_Reset( &system->scratch );
}

static void b2UniqueParticlePairs( b2ParticleSystem* system )
{
	if ( system->pairCount <= 1 )
	{
		return;
	}

	b2SortParticlePairs( system, system->pairs, system->pairCount );
	int writeIndex = 1;
	for ( int i = 1; i < system->pairCount; ++i )
	{
		b2ParticlePairData* previous = system->pairs + writeIndex - 1;
		b2ParticlePairData* pair = system->pairs + i;
		if ( previous->indexA == pair->indexA && previous->indexB == pair->indexB )
		{
			continue;
		}

		system->pairs[writeIndex++] = *pair;
	}
	system->pairCount = writeIndex;
}

static void b2UniqueParticleTriads( b2ParticleSystem* system )
{
	if ( system->triadCount <= 1 )
	{
		return;
	}

	b2SortParticleTriads( system, system->triads, system->triadCount );
	int writeIndex = 1;
	for ( int i = 1; i < system->triadCount; ++i )
	{
		b2ParticleTriadData* previous = system->triads + writeIndex - 1;
		b2ParticleTriadData* triad = system->triads + i;
		if ( previous->indexA == triad->indexA && previous->indexB == triad->indexB && previous->indexC == triad->indexC )
		{
			continue;
		}

		system->triads[writeIndex++] = *triad;
	}
	system->triadCount = writeIndex;
}

static b2ParticleGroup* b2GetParticleGroupByLocalIndex( b2ParticleSystem* system, int groupIndex )
{
	if ( groupIndex == B2_NULL_INDEX || groupIndex < 0 || system->groupCount <= groupIndex )
	{
		return NULL;
	}

	b2ParticleGroup* group = system->groups + groupIndex;
	return group->id != B2_NULL_INDEX ? group : NULL;
}

static float b2GetParticleGroupStrength( b2ParticleSystem* system, int particleIndex )
{
	b2ParticleGroup* group = b2GetParticleGroupByLocalIndex( system, system->groupIndices[particleIndex] );
	return group != NULL ? group->strength : 1.0f;
}

static bool b2ParticleCanBeConnected( b2ParticleSystem* system, int particleIndex )
{
	uint32_t flags = system->flags[particleIndex];
	if ( ( flags & ( b2_wallParticle | b2_springParticle | b2_elasticParticle ) ) != 0 )
	{
		return true;
	}

	b2ParticleGroup* group = b2GetParticleGroupByLocalIndex( system, system->groupIndices[particleIndex] );
	return group != NULL && ( group->groupFlags & b2_rigidParticleGroup ) != 0;
}

static bool b2ParticleConnectionIsNecessary( const b2ParticleSystem* system, int particleIndex, bool reactiveOnly )
{
	return reactiveOnly == false || ( system->flags[particleIndex] & b2_reactiveParticle ) != 0;
}

static bool b2ShouldCreateParticlePair( b2ParticleSystem* system, int indexA, int indexB )
{
	uint32_t flags = system->flags[indexA] | system->flags[indexB];
	if ( ( flags & b2_particleContactFilterParticle ) == 0 || system->contactFilterFcn == NULL )
	{
		return true;
	}

	b2ParticleSystemId systemId = { system->id + 1, system->worldId, system->generation };
	return system->contactFilterFcn( systemId, indexA, indexB, system->contactFilterContext );
}

static bool b2ShouldCreateParticleTriad( b2ParticleSystem* system, int indexA, int indexB, int indexC )
{
	uint32_t flags = system->flags[indexA] | system->flags[indexB] | system->flags[indexC];
	if ( ( flags & b2_particleContactFilterParticle ) == 0 || system->contactFilterFcn == NULL )
	{
		return true;
	}

	b2ParticleSystemId systemId = { system->id + 1, system->worldId, system->generation };
	return system->contactFilterFcn( systemId, indexA, indexB, system->contactFilterContext ) &&
		   system->contactFilterFcn( systemId, indexB, indexC, system->contactFilterContext ) &&
		   system->contactFilterFcn( systemId, indexC, indexA, system->contactFilterContext );
}

typedef struct b2VoronoiGenerator
{
	b2Vec2 center;
	int tag;
	bool necessary;
} b2VoronoiGenerator;

typedef struct b2VoronoiTask
{
	int x;
	int y;
	int index;
	b2VoronoiGenerator* generator;
} b2VoronoiTask;

typedef struct b2VoronoiQueue
{
	b2VoronoiTask* data;
	int head;
	int count;
	int capacity;
} b2VoronoiQueue;

static void b2PushVoronoiTask( b2VoronoiQueue* queue, b2VoronoiTask task )
{
	if ( queue->count == queue->capacity )
	{
		int oldCapacity = queue->capacity;
		int newCapacity = b2MaxParticleCapacity( oldCapacity, queue->count + 1 );
		queue->data = b2GrowParticleBuffer( queue->data, oldCapacity, newCapacity, sizeof( b2VoronoiTask ) );
		queue->capacity = newCapacity;
	}

	queue->data[queue->count++] = task;
}

static b2VoronoiTask b2PopVoronoiTask( b2VoronoiQueue* queue )
{
	B2_ASSERT( queue->head < queue->count );
	b2VoronoiTask task = queue->data[queue->head++];
	if ( queue->head == queue->count )
	{
		queue->head = 0;
		queue->count = 0;
	}
	return task;
}

static void b2TryCreateParticleTriad( b2ParticleSystem* system, int indexA, int indexB, int indexC )
{
	uint32_t flags = system->flags[indexA] | system->flags[indexB] | system->flags[indexC];
	if ( ( flags & b2_elasticParticle ) == 0 || b2ShouldCreateParticleTriad( system, indexA, indexB, indexC ) == false )
	{
		return;
	}

	const b2Vec2 pa = system->positions[indexA];
	const b2Vec2 pb = system->positions[indexB];
	const b2Vec2 pc = system->positions[indexC];
	b2Vec2 dab = b2Sub( pa, pb );
	b2Vec2 dbc = b2Sub( pb, pc );
	b2Vec2 dca = b2Sub( pc, pa );
	float diameter = 2.0f * system->def.radius;
	float maxDistanceSquared = B2_MAX_TRIAD_DISTANCE * B2_MAX_TRIAD_DISTANCE * diameter * diameter;
	if ( b2LengthSquared( dab ) > maxDistanceSquared || b2LengthSquared( dbc ) > maxDistanceSquared ||
		 b2LengthSquared( dca ) > maxDistanceSquared )
	{
		return;
	}

	float strength = b2MinFloat( b2MinFloat( b2GetParticleGroupStrength( system, indexA ),
											 b2GetParticleGroupStrength( system, indexB ) ),
								 b2GetParticleGroupStrength( system, indexC ) );
	b2Vec2 midpoint = b2MulSV( 1.0f / 3.0f, b2Add( b2Add( pa, pb ), pc ) );
	b2ReserveTriads( system, system->triadCount + 1 );
	system->triads[system->triadCount++] = (b2ParticleTriadData){
		.indexA = indexA,
		.indexB = indexB,
		.indexC = indexC,
		.flags = flags,
		.strength = strength,
		.pa = b2Sub( pa, midpoint ),
		.pb = b2Sub( pb, midpoint ),
		.pc = b2Sub( pc, midpoint ),
		.ka = -b2Dot( dca, dab ),
		.kb = -b2Dot( dab, dbc ),
		.kc = -b2Dot( dbc, dca ),
		.s = b2Cross( pa, pb ) + b2Cross( pb, pc ) + b2Cross( pc, pa ),
	};
}

static void b2CreateParticleTriadsFromVoronoi( b2ParticleSystem* system, int firstIndex, int lastIndex, bool reactiveOnly )
{
	int generatorCapacity = lastIndex - firstIndex;
	if ( generatorCapacity <= 0 )
	{
		return;
	}

	b2VoronoiGenerator* generators = b2Alloc( generatorCapacity * (int)sizeof( b2VoronoiGenerator ) );
	int generatorCount = 0;
	b2Vec2 lower = { FLT_MAX, FLT_MAX };
	b2Vec2 upper = { -FLT_MAX, -FLT_MAX };
	bool hasNecessary = false;
	for ( int i = firstIndex; i < lastIndex; ++i )
	{
		if ( ( system->flags[i] & b2_zombieParticle ) != 0 || b2ParticleCanBeConnected( system, i ) == false )
		{
			continue;
		}

		bool necessary = b2ParticleConnectionIsNecessary( system, i, reactiveOnly );
		generators[generatorCount++] = (b2VoronoiGenerator){ system->positions[i], i, necessary };
		if ( necessary )
		{
			lower = b2Min( lower, system->positions[i] );
			upper = b2Max( upper, system->positions[i] );
			hasNecessary = true;
		}
	}

	if ( generatorCount < 3 || hasNecessary == false )
	{
		b2Free( generators, generatorCapacity * (int)sizeof( b2VoronoiGenerator ) );
		return;
	}

	float stride = B2_PARTICLE_STRIDE * 2.0f * system->def.radius;
	float radius = 0.5f * stride;
	float inverseRadius = radius > 0.0f ? 1.0f / radius : 0.0f;
	float margin = 2.0f * stride;
	lower.x -= margin;
	lower.y -= margin;
	upper.x += margin;
	upper.y += margin;
	int countX = 1 + (int)( inverseRadius * ( upper.x - lower.x ) );
	int countY = 1 + (int)( inverseRadius * ( upper.y - lower.y ) );
	if ( countX <= 1 || countY <= 1 )
	{
		b2Free( generators, generatorCapacity * (int)sizeof( b2VoronoiGenerator ) );
		return;
	}

	int diagramCount = countX * countY;
	b2VoronoiGenerator** diagram = b2AllocZeroInit( diagramCount * (int)sizeof( b2VoronoiGenerator* ) );
	b2VoronoiQueue queue = { 0 };
	for ( int i = 0; i < generatorCount; ++i )
	{
		b2VoronoiGenerator* generator = generators + i;
		generator->center = b2MulSV( inverseRadius, b2Sub( generator->center, lower ) );
		int x = (int)generator->center.x;
		int y = (int)generator->center.y;
		if ( 0 <= x && 0 <= y && x < countX && y < countY )
		{
			b2PushVoronoiTask( &queue, (b2VoronoiTask){ x, y, x + y * countX, generator } );
		}
	}

	while ( queue.count > 0 )
	{
		b2VoronoiTask task = b2PopVoronoiTask( &queue );
		if ( diagram[task.index] != NULL )
		{
			continue;
		}

		diagram[task.index] = task.generator;
		if ( task.x > 0 )
		{
			b2PushVoronoiTask( &queue, (b2VoronoiTask){ task.x - 1, task.y, task.index - 1, task.generator } );
		}
		if ( task.y > 0 )
		{
			b2PushVoronoiTask( &queue, (b2VoronoiTask){ task.x, task.y - 1, task.index - countX, task.generator } );
		}
		if ( task.x < countX - 1 )
		{
			b2PushVoronoiTask( &queue, (b2VoronoiTask){ task.x + 1, task.y, task.index + 1, task.generator } );
		}
		if ( task.y < countY - 1 )
		{
			b2PushVoronoiTask( &queue, (b2VoronoiTask){ task.x, task.y + 1, task.index + countX, task.generator } );
		}
	}

	for ( int y = 0; y < countY; ++y )
	{
		for ( int x = 0; x < countX - 1; ++x )
		{
			int i = x + y * countX;
			b2VoronoiGenerator* a = diagram[i];
			b2VoronoiGenerator* b = diagram[i + 1];
			if ( a != b )
			{
				b2PushVoronoiTask( &queue, (b2VoronoiTask){ x, y, i, b } );
				b2PushVoronoiTask( &queue, (b2VoronoiTask){ x + 1, y, i + 1, a } );
			}
		}
	}

	for ( int y = 0; y < countY - 1; ++y )
	{
		for ( int x = 0; x < countX; ++x )
		{
			int i = x + y * countX;
			b2VoronoiGenerator* a = diagram[i];
			b2VoronoiGenerator* b = diagram[i + countX];
			if ( a != b )
			{
				b2PushVoronoiTask( &queue, (b2VoronoiTask){ x, y, i, b } );
				b2PushVoronoiTask( &queue, (b2VoronoiTask){ x, y + 1, i + countX, a } );
			}
		}
	}

	while ( queue.count > 0 )
	{
		b2VoronoiTask task = b2PopVoronoiTask( &queue );
		b2VoronoiGenerator* a = diagram[task.index];
		b2VoronoiGenerator* b = task.generator;
		if ( a == b || a == NULL || b == NULL )
		{
			continue;
		}

		float ax = a->center.x - (float)task.x;
		float ay = a->center.y - (float)task.y;
		float bx = b->center.x - (float)task.x;
		float by = b->center.y - (float)task.y;
		if ( ax * ax + ay * ay <= bx * bx + by * by )
		{
			continue;
		}

		diagram[task.index] = b;
		if ( task.x > 0 )
		{
			b2PushVoronoiTask( &queue, (b2VoronoiTask){ task.x - 1, task.y, task.index - 1, b } );
		}
		if ( task.y > 0 )
		{
			b2PushVoronoiTask( &queue, (b2VoronoiTask){ task.x, task.y - 1, task.index - countX, b } );
		}
		if ( task.x < countX - 1 )
		{
			b2PushVoronoiTask( &queue, (b2VoronoiTask){ task.x + 1, task.y, task.index + 1, b } );
		}
		if ( task.y < countY - 1 )
		{
			b2PushVoronoiTask( &queue, (b2VoronoiTask){ task.x, task.y + 1, task.index + countX, b } );
		}
	}

	for ( int y = 0; y < countY - 1; ++y )
	{
		for ( int x = 0; x < countX - 1; ++x )
		{
			int i = x + y * countX;
			b2VoronoiGenerator* a = diagram[i];
			b2VoronoiGenerator* b = diagram[i + 1];
			b2VoronoiGenerator* c = diagram[i + countX];
			b2VoronoiGenerator* d = diagram[i + 1 + countX];
			if ( a == NULL || b == NULL || c == NULL || d == NULL || b == c )
			{
				continue;
			}

			if ( a != b && a != c && ( a->necessary || b->necessary || c->necessary ) )
			{
				b2TryCreateParticleTriad( system, a->tag, b->tag, c->tag );
			}
			if ( d != b && d != c && ( b->necessary || d->necessary || c->necessary ) )
			{
				b2TryCreateParticleTriad( system, b->tag, d->tag, c->tag );
			}
		}
	}

	b2UniqueParticleTriads( system );
	b2Free( queue.data, queue.capacity * (int)sizeof( b2VoronoiTask ) );
	b2Free( diagram, diagramCount * (int)sizeof( b2VoronoiGenerator* ) );
	b2Free( generators, generatorCapacity * (int)sizeof( b2VoronoiGenerator ) );
}

static void b2UpdateParticlePairsAndTriads( b2ParticleSystem* system, int firstIndex, int lastIndex, bool reactiveOnly )
{
	B2_ASSERT( firstIndex <= lastIndex );
	uint32_t particleFlags = 0;
	for ( int i = firstIndex; i < lastIndex; ++i )
	{
		particleFlags |= system->flags[i];
	}

	if ( ( particleFlags & ( b2_springParticle | b2_barrierParticle ) ) != 0 )
	{
		for ( int i = 0; i < system->contactCount; ++i )
		{
			const b2ParticleContactData* contact = system->contacts + i;
			int indexA = contact->indexA;
			int indexB = contact->indexB;
			uint32_t flags = system->flags[indexA] | system->flags[indexB];
			if ( indexA < firstIndex || lastIndex <= indexA || indexB < firstIndex || lastIndex <= indexB ||
				 ( flags & b2_zombieParticle ) != 0 || ( flags & ( b2_springParticle | b2_barrierParticle ) ) == 0 ||
				 ( b2ParticleConnectionIsNecessary( system, indexA, reactiveOnly ) == false &&
				   b2ParticleConnectionIsNecessary( system, indexB, reactiveOnly ) == false ) ||
				 b2ParticleCanBeConnected( system, indexA ) == false || b2ParticleCanBeConnected( system, indexB ) == false ||
				 b2ShouldCreateParticlePair( system, indexA, indexB ) == false )
			{
				continue;
			}

			if ( indexB < indexA )
			{
				int swap = indexA;
				indexA = indexB;
				indexB = swap;
			}

			float strength =
				b2MinFloat( b2GetParticleGroupStrength( system, indexA ), b2GetParticleGroupStrength( system, indexB ) );
			b2ReservePairs( system, system->pairCount + 1 );
			system->pairs[system->pairCount++] = (b2ParticlePairData){
				.indexA = indexA,
				.indexB = indexB,
				.flags = flags,
				.strength = strength,
				.distance = b2Distance( system->positions[indexA], system->positions[indexB] ),
			};
		}
		b2UniqueParticlePairs( system );
	}

	if ( ( particleFlags & b2_elasticParticle ) != 0 )
	{
		b2CreateParticleTriadsFromVoronoi( system, firstIndex, lastIndex, reactiveOnly );
	}
}

static void b2UpdateReactiveParticlePairsAndTriads( b2ParticleSystem* system )
{
	if ( ( system->allParticleFlags & b2_reactiveParticle ) == 0 )
	{
		return;
	}

	b2UpdateParticlePairsAndTriads( system, 0, system->particleCount, true );
	for ( int i = 0; i < system->particleCount; ++i )
	{
		system->flags[i] &= ~b2_reactiveParticle;
	}
}

static int b2CompareParticleContactKeys( const void* a, const void* b )
{
	const b2ParticleContactData* contactA = a;
	const b2ParticleContactData* contactB = b;
	int minA = b2MinInt( contactA->indexA, contactA->indexB );
	int maxA = b2MaxInt( contactA->indexA, contactA->indexB );
	int minB = b2MinInt( contactB->indexA, contactB->indexB );
	int maxB = b2MaxInt( contactB->indexA, contactB->indexB );
	if ( minA != minB )
	{
		return minA < minB ? -1 : 1;
	}
	if ( maxA != maxB )
	{
		return maxA < maxB ? -1 : 1;
	}
	return 0;
}

static int b2CompareParticleBodyContactKeys( const void* a, const void* b )
{
	const b2ParticleBodyContactData* contactA = a;
	const b2ParticleBodyContactData* contactB = b;
	if ( contactA->particleIndex != contactB->particleIndex )
	{
		return contactA->particleIndex < contactB->particleIndex ? -1 : 1;
	}
	if ( contactA->shapeId.index1 != contactB->shapeId.index1 )
	{
		return contactA->shapeId.index1 < contactB->shapeId.index1 ? -1 : 1;
	}
	if ( contactA->shapeId.world0 != contactB->shapeId.world0 )
	{
		return contactA->shapeId.world0 < contactB->shapeId.world0 ? -1 : 1;
	}
	if ( contactA->shapeId.generation != contactB->shapeId.generation )
	{
		return contactA->shapeId.generation < contactB->shapeId.generation ? -1 : 1;
	}
	return 0;
}

typedef struct b2SortableParticleContact
{
	uint64_t key;
	b2ParticleContactData contact;
} b2SortableParticleContact;

typedef struct b2SortableParticleBodyContact
{
	uint64_t key;
	b2ParticleBodyContactData contact;
} b2SortableParticleBodyContact;

static void b2SortParticleContactsByKey( b2ParticleSystem* system, b2ParticleContactData* contacts, int count )
{
	if ( count <= 1 )
	{
		return;
	}

	void* sortScratch = NULL;
	b2SortableParticleContact* records = b2AllocateParticleSortRecords(
		system, count, (int)sizeof( b2SortableParticleContact ), (int)_Alignof( b2SortableParticleContact ), &sortScratch );
	for ( int i = 0; i < count; ++i )
	{
		records[i] = (b2SortableParticleContact){
			(uint64_t)(uint32_t)b2MaxInt( contacts[i].indexA, contacts[i].indexB ),
			contacts[i],
		};
	}

	b2SortParticleRecordsByKeyWithScratch( records, sortScratch, count, (int)sizeof( b2SortableParticleContact ), 0 );
	for ( int i = 0; i < count; ++i )
	{
		records[i].key = (uint64_t)(uint32_t)b2MinInt( records[i].contact.indexA, records[i].contact.indexB );
	}
	b2SortParticleRecordsByKeyWithScratch( records, sortScratch, count, (int)sizeof( b2SortableParticleContact ), 0 );

	for ( int i = 0; i < count; ++i )
	{
		contacts[i] = records[i].contact;
	}

	b2ParticleScratchBuffer_Reset( &system->scratch );
}

static void b2SortParticleBodyContactsByKey( b2ParticleSystem* system, b2ParticleBodyContactData* contacts, int count )
{
	if ( count <= 1 )
	{
		return;
	}

	void* sortScratch = NULL;
	b2SortableParticleBodyContact* records = b2AllocateParticleSortRecords(
		system, count, (int)sizeof( b2SortableParticleBodyContact ), (int)_Alignof( b2SortableParticleBodyContact ),
		&sortScratch );
	for ( int i = 0; i < count; ++i )
	{
		records[i] = (b2SortableParticleBodyContact){ (uint64_t)contacts[i].shapeId.generation, contacts[i] };
	}

	b2SortParticleRecordsByKeyWithScratch( records, sortScratch, count, (int)sizeof( b2SortableParticleBodyContact ), 0 );
	for ( int i = 0; i < count; ++i )
	{
		records[i].key = (uint64_t)records[i].contact.shapeId.world0;
	}
	b2SortParticleRecordsByKeyWithScratch( records, sortScratch, count, (int)sizeof( b2SortableParticleBodyContact ), 0 );
	for ( int i = 0; i < count; ++i )
	{
		records[i].key = (uint64_t)(uint32_t)records[i].contact.shapeId.index1;
	}
	b2SortParticleRecordsByKeyWithScratch( records, sortScratch, count, (int)sizeof( b2SortableParticleBodyContact ), 0 );
	for ( int i = 0; i < count; ++i )
	{
		records[i].key = (uint64_t)(uint32_t)records[i].contact.particleIndex;
	}
	b2SortParticleRecordsByKeyWithScratch( records, sortScratch, count, (int)sizeof( b2SortableParticleBodyContact ), 0 );

	for ( int i = 0; i < count; ++i )
	{
		contacts[i] = records[i].contact;
	}

	b2ParticleScratchBuffer_Reset( &system->scratch );
}

static void b2SavePreviousParticleContacts( b2ParticleSystem* system )
{
	b2ReserveContactData( &system->previousContacts, &system->previousContactCapacity, system->contactCount );
	if ( system->contactCount > 0 )
	{
		memcpy( system->previousContacts, system->contacts, system->contactCount * sizeof( b2ParticleContactData ) );
	}
	system->previousContactCount = system->contactCount;
}

static void b2SavePreviousParticleBodyContacts( b2ParticleSystem* system )
{
	b2ReserveBodyContactData( &system->previousBodyContacts, &system->previousBodyContactCapacity, system->bodyContactCount );
	if ( system->bodyContactCount > 0 )
	{
		memcpy( system->previousBodyContacts, system->bodyContacts, system->bodyContactCount * sizeof( b2ParticleBodyContactData ) );
	}
	system->previousBodyContactCount = system->bodyContactCount;
}

static uint32_t b2ComputeAllParticleFlags( const b2ParticleSystem* system )
{
	uint32_t flags = 0;
	for ( int i = 0; i < system->particleCount; ++i )
	{
		flags |= system->flags[i];
	}

	return flags;
}

static bool b2NeedsParticleContactEvents( const b2ParticleSystem* system )
{
	return system->def.enableParticleEvents || ( system->allParticleFlags & b2_particleContactListenerParticle ) != 0;
}

static bool b2NeedsParticleBodyContactEvents( const b2ParticleSystem* system )
{
	return system->def.enableParticleBodyEvents || ( system->allParticleFlags & b2_bodyContactListenerParticle ) != 0;
}

static void b2UpdateParticleContactEvents( b2ParticleSystem* system )
{
	if ( b2NeedsParticleContactEvents( system ) == false )
	{
		return;
	}

	uint64_t ticks = b2GetTicks();
	b2ParticleContactData* currentContacts = NULL;
	b2ParticleContactData* previousContacts = NULL;
	if ( system->contactCount > 0 )
	{
		b2ParticleScratchArray* currentPayload =
			b2GetParticlePayloadArray( &system->eventContactSortPayloads, 0, (int)sizeof( b2ParticleContactData ) );
		currentContacts = b2ParticleScratchArray_ResizeTyped( currentPayload, system->contactCount, b2ParticleContactData );
		memcpy( currentContacts, system->contacts, system->contactCount * sizeof( b2ParticleContactData ) );
		b2SortParticleContactsByKey( system, currentContacts, system->contactCount );
	}
	if ( system->previousContactCount > 0 )
	{
		b2ParticleScratchArray* previousPayload =
			b2GetParticlePayloadArray( &system->eventContactSortPayloads, 1, (int)sizeof( b2ParticleContactData ) );
		previousContacts =
			b2ParticleScratchArray_ResizeTyped( previousPayload, system->previousContactCount, b2ParticleContactData );
		memcpy( previousContacts, system->previousContacts, system->previousContactCount * sizeof( b2ParticleContactData ) );
		b2SortParticleContactsByKey( system, previousContacts, system->previousContactCount );
	}

	int currentIndex = 0;
	int previousIndex = 0;
	while ( currentIndex < system->contactCount || previousIndex < system->previousContactCount )
	{
		if ( previousIndex >= system->previousContactCount )
		{
			b2ReserveContactData( &system->contactBeginEvents, &system->contactBeginCapacity, system->contactBeginCount + 1 );
			system->contactBeginEvents[system->contactBeginCount++] = currentContacts[currentIndex++];
			continue;
		}

		if ( currentIndex >= system->contactCount )
		{
			b2ReserveContactData( &system->contactEndEvents, &system->contactEndCapacity, system->contactEndCount + 1 );
			system->contactEndEvents[system->contactEndCount++] = previousContacts[previousIndex++];
			continue;
		}

		int compare = b2CompareParticleContactKeys( currentContacts + currentIndex, previousContacts + previousIndex );
		if ( compare < 0 )
		{
			b2ReserveContactData( &system->contactBeginEvents, &system->contactBeginCapacity, system->contactBeginCount + 1 );
			system->contactBeginEvents[system->contactBeginCount++] = currentContacts[currentIndex++];
		}
		else if ( compare > 0 )
		{
			b2ReserveContactData( &system->contactEndEvents, &system->contactEndCapacity, system->contactEndCount + 1 );
			system->contactEndEvents[system->contactEndCount++] = previousContacts[previousIndex++];
		}
		else
		{
			currentIndex += 1;
			previousIndex += 1;
		}
	}
	system->profile.events += b2GetMilliseconds( ticks );
}

static void b2UpdateParticleBodyContactEvents( b2ParticleSystem* system )
{
	if ( b2NeedsParticleBodyContactEvents( system ) == false )
	{
		return;
	}

	uint64_t ticks = b2GetTicks();
	b2ParticleBodyContactData* currentContacts = NULL;
	b2ParticleBodyContactData* previousContacts = NULL;
	if ( system->bodyContactCount > 0 )
	{
		b2ParticleScratchArray* currentPayload = b2GetParticlePayloadArray(
			&system->eventBodyContactSortPayloads, 0, (int)sizeof( b2ParticleBodyContactData ) );
		currentContacts =
			b2ParticleScratchArray_ResizeTyped( currentPayload, system->bodyContactCount, b2ParticleBodyContactData );
		memcpy( currentContacts, system->bodyContacts, system->bodyContactCount * sizeof( b2ParticleBodyContactData ) );
		b2SortParticleBodyContactsByKey( system, currentContacts, system->bodyContactCount );
	}
	if ( system->previousBodyContactCount > 0 )
	{
		b2ParticleScratchArray* previousPayload = b2GetParticlePayloadArray(
			&system->eventBodyContactSortPayloads, 1, (int)sizeof( b2ParticleBodyContactData ) );
		previousContacts = b2ParticleScratchArray_ResizeTyped(
			previousPayload, system->previousBodyContactCount, b2ParticleBodyContactData );
		memcpy(
			previousContacts, system->previousBodyContacts,
			system->previousBodyContactCount * sizeof( b2ParticleBodyContactData ) );
		b2SortParticleBodyContactsByKey( system, previousContacts, system->previousBodyContactCount );
	}

	int currentIndex = 0;
	int previousIndex = 0;
	while ( currentIndex < system->bodyContactCount || previousIndex < system->previousBodyContactCount )
	{
		if ( previousIndex >= system->previousBodyContactCount )
		{
			b2ReserveBodyContactData(
				&system->bodyContactBeginEvents, &system->bodyContactBeginCapacity, system->bodyContactBeginCount + 1 );
			system->bodyContactBeginEvents[system->bodyContactBeginCount++] = currentContacts[currentIndex++];
			continue;
		}

		if ( currentIndex >= system->bodyContactCount )
		{
			b2ReserveBodyContactData(
				&system->bodyContactEndEvents, &system->bodyContactEndCapacity, system->bodyContactEndCount + 1 );
			system->bodyContactEndEvents[system->bodyContactEndCount++] = previousContacts[previousIndex++];
			continue;
		}

		int compare =
			b2CompareParticleBodyContactKeys( currentContacts + currentIndex, previousContacts + previousIndex );
		if ( compare < 0 )
		{
			b2ReserveBodyContactData(
				&system->bodyContactBeginEvents, &system->bodyContactBeginCapacity, system->bodyContactBeginCount + 1 );
			system->bodyContactBeginEvents[system->bodyContactBeginCount++] = currentContacts[currentIndex++];
		}
		else if ( compare > 0 )
		{
			b2ReserveBodyContactData(
				&system->bodyContactEndEvents, &system->bodyContactEndCapacity, system->bodyContactEndCount + 1 );
			system->bodyContactEndEvents[system->bodyContactEndCount++] = previousContacts[previousIndex++];
		}
		else
		{
			currentIndex += 1;
			previousIndex += 1;
		}
	}
	system->profile.events += b2GetMilliseconds( ticks );
}

static b2Shape* b2GetParticleContactShape( b2World* world, b2ShapeId shapeId );
static b2Vec2 b2GetBodyVelocityAtPoint( b2World* world, b2Body* body, b2Vec2 point );

static void b2ComputeParticleShapeDistance( const b2Shape* shape, b2Transform transform, b2Vec2 point, float* distance,
											b2Vec2* normal )
{
	switch ( shape->type )
	{
		case b2_circleShape:
		{
			b2Vec2 center = b2TransformPoint( transform, shape->circle.center );
			b2Vec2 d = b2Sub( point, center );
			float length = b2Length( d );
			*distance = length - shape->circle.radius;
			*normal = length > FLT_EPSILON ? b2MulSV( 1.0f / length, d ) : (b2Vec2){ 0.0f, 1.0f };
			return;
		}

		case b2_capsuleShape:
		{
			b2Vec2 point1 = b2TransformPoint( transform, shape->capsule.center1 );
			b2Vec2 point2 = b2TransformPoint( transform, shape->capsule.center2 );
			b2Vec2 d = b2Sub( point, point1 );
			b2Vec2 s = b2Sub( point2, point1 );
			float ds = b2Dot( d, s );
			if ( ds > 0.0f )
			{
				float s2 = b2Dot( s, s );
				if ( ds > s2 )
				{
					d = b2Sub( point, point2 );
				}
				else if ( s2 > 0.0f )
				{
					d = b2MulSub( d, ds / s2, s );
				}
			}

			float length = b2Length( d );
			*distance = length - shape->capsule.radius;
			*normal = length > FLT_EPSILON ? b2MulSV( 1.0f / length, d ) : (b2Vec2){ 0.0f, 1.0f };
			return;
		}

		case b2_segmentShape:
		case b2_chainSegmentShape:
		{
			b2Segment segment = shape->type == b2_segmentShape ? shape->segment : shape->chainSegment.segment;
			b2Vec2 point1 = b2TransformPoint( transform, segment.point1 );
			b2Vec2 point2 = b2TransformPoint( transform, segment.point2 );
			b2Vec2 d = b2Sub( point, point1 );
			b2Vec2 s = b2Sub( point2, point1 );
			float ds = b2Dot( d, s );
			if ( ds > 0.0f )
			{
				float s2 = b2Dot( s, s );
				if ( ds > s2 )
				{
					d = b2Sub( point, point2 );
				}
				else if ( s2 > 0.0f )
				{
					d = b2MulSub( d, ds / s2, s );
				}
			}

			float length = b2Length( d );
			*distance = length;
			*normal = length > FLT_EPSILON ? b2MulSV( 1.0f / length, d ) : b2Vec2_zero;
			return;
		}

		case b2_polygonShape:
		{
			b2Vec2 localPoint = b2InvTransformPoint( transform, point );
			float maxDistance = -FLT_MAX;
			b2Vec2 normalForMaxDistance = localPoint;
			for ( int i = 0; i < shape->polygon.count; ++i )
			{
				float dot = b2Dot( shape->polygon.normals[i], b2Sub( localPoint, shape->polygon.vertices[i] ) );
				if ( dot > maxDistance )
				{
					maxDistance = dot;
					normalForMaxDistance = shape->polygon.normals[i];
				}
			}

			if ( maxDistance > 0.0f )
			{
				b2Vec2 minDistance = normalForMaxDistance;
				float minDistanceSquared = maxDistance * maxDistance;
				for ( int i = 0; i < shape->polygon.count; ++i )
				{
					b2Vec2 vertexDistance = b2Sub( localPoint, shape->polygon.vertices[i] );
					float distanceSquared = b2LengthSquared( vertexDistance );
					if ( minDistanceSquared > distanceSquared )
					{
						minDistance = vertexDistance;
						minDistanceSquared = distanceSquared;
					}
				}

				float length = sqrtf( minDistanceSquared );
				*distance = length;
				*normal = length > FLT_EPSILON ? b2RotateVector( transform.q, b2MulSV( 1.0f / length, minDistance ) )
											   : b2RotateVector( transform.q, normalForMaxDistance );
			}
			else
			{
				*distance = maxDistance;
				*normal = b2RotateVector( transform.q, normalForMaxDistance );
			}
			return;
		}

		default:
			*distance = FLT_MAX;
			*normal = b2Vec2_zero;
			return;
	}
}

static b2Shape* b2GetParticleShapeByIndex( b2World* world, int shapeIndex )
{
	if ( shapeIndex < 0 || world->shapes.count <= shapeIndex )
	{
		return NULL;
	}

	b2Shape* shape = world->shapes.data + shapeIndex;
	return shape->id == shapeIndex ? shape : NULL;
}

static void b2QueryParticleBodyTrees( b2World* world, b2AABB aabb, b2TreeQueryCallbackFcn* callback, void* context )
{
	for ( int i = 0; i < b2_bodyTypeCount; ++i )
	{
		b2DynamicTree_Query( world->broadPhase.trees + i, aabb, B2_DEFAULT_MASK_BITS, callback, context );
	}
}

static bool b2ParticleShapeCandidateQueryCallback( int proxyId, uint64_t userData, void* context )
{
	B2_UNUSED( proxyId );

	b2ParticleShapeCandidateQueryContext* query = context;
	b2World* world = query->world;
	int shapeIndex = (int)userData;
	b2Shape* shape = b2GetParticleShapeByIndex( world, shapeIndex );
	if ( shape == NULL || b2AABB_Overlaps( query->aabb, shape->aabb ) == false )
	{
		return true;
	}

	b2AppendParticleShapeCandidateBlock( query->block, shapeIndex );
	return true;
}

static void b2AppendParticleBodyContactForShape( b2ParticleBodyContactQueryContext* query, int shapeIndex )
{
	b2World* world = query->world;
	b2ParticleSystem* system = query->system;
	b2Shape* shape = b2GetParticleShapeByIndex( world, shapeIndex );
	if ( shape == NULL || b2AABB_Overlaps( query->aabb, shape->aabb ) == false )
	{
		return;
	}

	b2ShapeId shapeId = { shape->id + 1, world->worldId, shape->generation };
	if ( ( system->flags[query->particleIndex] & b2_bodyContactFilterParticle ) != 0 && system->bodyContactFilterFcn != NULL )
	{
		if ( system->bodyContactFilterFcn( query->systemId, shapeId, query->particleIndex, system->bodyContactFilterContext ) ==
			 false )
		{
			return;
		}
	}

	b2Vec2 p = system->positions[query->particleIndex];
	b2Body* body = b2BodyArray_Get( &world->bodies, shape->bodyId );
	b2Transform transform = b2GetBodyTransformQuick( world, body );
	float distance = 0.0f;
	b2Vec2 shapeNormal = b2Vec2_zero;
	b2ComputeParticleShapeDistance( shape, transform, p, &distance, &shapeNormal );
	if ( distance >= query->diameter )
	{
		return;
	}

	b2Vec2 normal = b2Neg( shapeNormal );
	if ( b2LengthSquared( normal ) < FLT_EPSILON )
	{
		normal = (b2Vec2){ 0.0f, 1.0f };
	}

	b2BodySim* sim = b2GetBodySim( world, body );
	float invParticleMass = ( system->flags[query->particleIndex] & b2_wallParticle ) != 0 ? 0.0f : query->particleInvMass;
	float invBodyMass = sim != NULL ? sim->invMass : 0.0f;
	float invBodyInertia = sim != NULL ? sim->invInertia : 0.0f;
	b2Vec2 r = sim != NULL ? b2Sub( p, sim->center ) : b2Vec2_zero;
	float rn = b2Cross( r, normal );
	float invMass = invParticleMass + invBodyMass + invBodyInertia * rn * rn;
	float contactMass = invMass > 0.0f ? 1.0f / invMass : 0.0f;
	float weight = 1.0f - distance * query->invDiameter;
	b2AppendParticleBodyContactBlock(
		query->block, (b2ParticleBodyContactData){ query->particleIndex, shapeId, weight, contactMass, normal } );
}

static bool b2UpdateParticleBodyCollisionForShape( b2ParticleBodyCollisionQueryContext* query, int shapeIndex )
{
	b2World* world = query->world;
	b2ParticleSystem* system = query->system;
	b2Shape* shape = b2GetParticleShapeByIndex( world, shapeIndex );
	if ( shape == NULL || b2AABB_Overlaps( query->aabb, shape->aabb ) == false )
	{
		return true;
	}

	b2ShapeId shapeId = { shape->id + 1, world->worldId, shape->generation };
	if ( ( system->flags[query->particleIndex] & b2_bodyContactFilterParticle ) != 0 && system->bodyContactFilterFcn != NULL )
	{
		if ( system->bodyContactFilterFcn( query->systemId, shapeId, query->particleIndex, system->bodyContactFilterContext ) ==
			 false )
		{
			return true;
		}
	}

	b2Body* body = b2BodyArray_Get( &world->bodies, shape->bodyId );
	b2Transform transform = b2GetBodyTransformQuick( world, body );
	b2RayCastInput input = { query->position, query->translation, query->minFraction };
	b2CastOutput output = b2RayCastShape( &input, shape, transform );
	if ( output.hit && output.fraction > 0.0f && output.fraction < query->minFraction )
	{
		query->minFraction = output.fraction;
		query->bestOutput = output;
		query->bestShapeId = shapeId;
	}

	return query->minFraction > 0.0f;
}

static void b2AppendParticleContactIfTouching( b2ParticleContactTaskContext* context, b2ParticleContactBlock* block,
											  b2ParticleSystemId systemId, int indexA, int indexB )
{
	block->candidateCount += 1;

	b2ParticleSystem* system = context->system;
	b2Vec2 d = b2Sub( system->positions[indexB], system->positions[indexA] );
	float distanceSquared = b2LengthSquared( d );
	float distance = 0.0f;
	if ( system->def.strictContactCheck )
	{
		distance = b2Distance( system->positions[indexA], system->positions[indexB] );
		if ( distance >= context->diameter )
		{
			return;
		}
	}
	else if ( distanceSquared >= context->diameterSquared )
	{
		return;
	}
	else
	{
		distance = sqrtf( distanceSquared );
	}

	b2Vec2 normal = distance > FLT_EPSILON ? b2MulSV( 1.0f / distance, d ) : (b2Vec2){ 1.0f, 0.0f };
	float weight = 1.0f - distance * context->invDiameter;
	uint32_t flags = system->flags[indexA] | system->flags[indexB];
	if ( ( flags & b2_particleContactFilterParticle ) != 0 && system->contactFilterFcn != NULL )
	{
		if ( system->contactFilterFcn( systemId, indexA, indexB, system->contactFilterContext ) == false )
		{
			return;
		}
	}

	b2AppendParticleContactBlock( block, (b2ParticleContactData){ indexA, indexB, weight, normal } );
}

static int b2LowerBoundParticleProxyKey( const b2ParticleProxy* proxies, int count, uint64_t key )
{
	int first = 0;
	int last = count;
	while ( first < last )
	{
		int mid = first + ( last - first ) / 2;
		if ( proxies[mid].key < key )
		{
			first = mid + 1;
		}
		else
		{
			last = mid;
		}
	}

	return first;
}

static void b2ParticleContactsTask( int startIndex, int endIndex, uint32_t workerIndex, void* taskContext )
{
	B2_UNUSED( workerIndex );

	b2ParticleContactTaskContext* context = taskContext;
	b2ParticleSystem* system = context->system;
	b2ParticleSystemId systemId = { system->id + 1, system->worldId, system->generation };
	b2ParticleProxy* proxies = context->proxies;
	int proxyCount = context->proxyCount;

	for ( int blockIndex = startIndex; blockIndex < endIndex; ++blockIndex )
	{
		b2ParticleContactBlock* block = context->blocks + blockIndex;
		int firstProxy = blockIndex * context->blockSize;
		int lastProxy = b2MinInt( firstProxy + context->blockSize, proxyCount );
		block->count = 0;

		int lowerProxy = firstProxy < lastProxy
							 ? b2LowerBoundParticleProxyKey(
								   proxies, proxyCount,
								   b2GetParticleCellKey( proxies[firstProxy].cellX - 1, proxies[firstProxy].cellY + 1 ) )
							 : proxyCount;

		for ( int proxyIndex = firstProxy; proxyIndex < lastProxy; ++proxyIndex )
		{
			b2ParticleProxy proxyA = proxies[proxyIndex];
			uint64_t rightKey = b2GetParticleCellKey( proxyA.cellX + 1, proxyA.cellY );
			for ( int proxyB = proxyIndex + 1; proxyB < proxyCount && proxies[proxyB].cellY == proxyA.cellY; ++proxyB )
			{
				if ( proxies[proxyB].key > rightKey )
				{
					break;
				}

				b2AppendParticleContactIfTouching( context, block, systemId, proxyA.index, proxies[proxyB].index );
			}

			uint64_t bottomLeftKey = b2GetParticleCellKey( proxyA.cellX - 1, proxyA.cellY + 1 );
			while ( lowerProxy < proxyCount && proxies[lowerProxy].key < bottomLeftKey )
			{
				lowerProxy += 1;
			}

			uint64_t bottomRightKey = b2GetParticleCellKey( proxyA.cellX + 1, proxyA.cellY + 1 );
			for ( int proxyB = lowerProxy; proxyB < proxyCount && proxies[proxyB].key <= bottomRightKey; ++proxyB )
			{
				b2AppendParticleContactIfTouching( context, block, systemId, proxyA.index, proxies[proxyB].index );
			}
		}
	}
}

static void b2ParticleBodyContactsTask( int startIndex, int endIndex, uint32_t workerIndex, void* taskContext )
{
	B2_UNUSED( workerIndex );

	b2ParticleBodyContactTaskContext* context = taskContext;
	b2World* world = context->world;
	b2ParticleSystem* system = context->system;
	b2ParticleSystemId systemId = { system->id + 1, system->worldId, system->generation };

	for ( int blockIndex = startIndex; blockIndex < endIndex; ++blockIndex )
	{
		b2ParticleBodyContactBlock* block = context->blocks + blockIndex;
		int firstParticle = blockIndex * context->blockSize;
		int lastParticle = b2MinInt( firstParticle + context->blockSize, system->particleCount );
		b2AABB blockAABB = { { FLT_MAX, FLT_MAX }, { -FLT_MAX, -FLT_MAX } };
		b2ParticleShapeCandidateBlock candidates = context->shapeCandidateBlocks[blockIndex];
		block->count = 0;
		candidates.count = 0;

		for ( int particleIndex = firstParticle; particleIndex < lastParticle; ++particleIndex )
		{
			b2Vec2 p = system->positions[particleIndex];
			b2AABB particleAABB = {
				b2Sub( p, (b2Vec2){ context->diameter, context->diameter } ),
				b2Add( p, (b2Vec2){ context->diameter, context->diameter } )
			};
			blockAABB.lowerBound = b2Min( blockAABB.lowerBound, particleAABB.lowerBound );
			blockAABB.upperBound = b2Max( blockAABB.upperBound, particleAABB.upperBound );
		}

		b2ParticleShapeCandidateQueryContext candidateContext = {
			.world = world,
			.block = &candidates,
			.aabb = blockAABB,
		};
		b2QueryParticleBodyTrees( world, blockAABB, b2ParticleShapeCandidateQueryCallback, &candidateContext );
		block->candidateCount += candidates.count * b2MaxInt( 0, lastParticle - firstParticle );

		for ( int particleIndex = firstParticle; particleIndex < lastParticle; ++particleIndex )
		{
			b2Vec2 p = system->positions[particleIndex];
			b2AABB particleAABB = {
				b2Sub( p, (b2Vec2){ context->diameter, context->diameter } ),
				b2Add( p, (b2Vec2){ context->diameter, context->diameter } )
			};
			b2ParticleBodyContactQueryContext queryContext = {
				.world = world,
				.system = system,
				.block = block,
				.systemId = systemId,
				.aabb = particleAABB,
				.particleIndex = particleIndex,
				.diameter = context->diameter,
				.invDiameter = context->invDiameter,
				.particleInvMass = context->particleInvMass,
			};
			for ( int shapeCandidate = 0; shapeCandidate < candidates.count; ++shapeCandidate )
			{
				b2AppendParticleBodyContactForShape( &queryContext, candidates.shapeIndices[shapeCandidate] );
			}
		}

		b2FreeParticleShapeCandidateBlock( &candidates );
	}
}

static void b2ParticleBodyCollisionTask( int startIndex, int endIndex, uint32_t workerIndex, void* taskContext )
{
	B2_UNUSED( workerIndex );

	b2ParticleBodyCollisionTaskContext* context = taskContext;
	b2World* world = context->world;
	b2ParticleSystem* system = context->system;
	b2ParticleSystemId systemId = { system->id + 1, system->worldId, system->generation };
	b2Vec2 radius = { system->def.radius, system->def.radius };

	for ( int blockIndex = startIndex; blockIndex < endIndex; ++blockIndex )
	{
		b2ParticleBodyImpulseBlock* block = context->blocks + blockIndex;
		int firstParticle = blockIndex * context->blockSize;
		int lastParticle = b2MinInt( firstParticle + context->blockSize, system->particleCount );
		b2AABB blockAABB = { { FLT_MAX, FLT_MAX }, { -FLT_MAX, -FLT_MAX } };
		b2ParticleShapeCandidateBlock candidates = context->shapeCandidateBlocks[blockIndex];
		bool hasMovingParticles = false;
		int movingParticleCount = 0;
		block->count = 0;
		candidates.count = 0;

		for ( int particleIndex = firstParticle; particleIndex < lastParticle; ++particleIndex )
		{
			b2Vec2 velocity = system->velocities[particleIndex];
			b2Vec2 translation = b2MulSV( context->dt, velocity );
			if ( b2LengthSquared( translation ) <= FLT_EPSILON )
			{
				continue;
			}

			b2Vec2 p0 = system->positions[particleIndex];
			b2AABB sweepAABB = { b2Min( p0, b2Add( p0, translation ) ), b2Max( p0, b2Add( p0, translation ) ) };
			sweepAABB.lowerBound = b2Sub( sweepAABB.lowerBound, radius );
			sweepAABB.upperBound = b2Add( sweepAABB.upperBound, radius );
			blockAABB.lowerBound = b2Min( blockAABB.lowerBound, sweepAABB.lowerBound );
			blockAABB.upperBound = b2Max( blockAABB.upperBound, sweepAABB.upperBound );
			hasMovingParticles = true;
			movingParticleCount += 1;
		}

		if ( hasMovingParticles == false )
		{
			continue;
		}

		b2ParticleShapeCandidateQueryContext candidateContext = {
			.world = world,
			.block = &candidates,
			.aabb = blockAABB,
		};
		b2QueryParticleBodyTrees( world, blockAABB, b2ParticleShapeCandidateQueryCallback, &candidateContext );
		block->candidateCount += candidates.count * movingParticleCount;

		for ( int particleIndex = firstParticle; particleIndex < lastParticle; ++particleIndex )
		{
			b2Vec2 velocity = system->velocities[particleIndex];
			b2Vec2 translation = b2MulSV( context->dt, velocity );
			if ( b2LengthSquared( translation ) <= FLT_EPSILON )
			{
				continue;
			}

			b2Vec2 p0 = system->positions[particleIndex];
			b2AABB sweepAABB = { b2Min( p0, b2Add( p0, translation ) ), b2Max( p0, b2Add( p0, translation ) ) };
			sweepAABB.lowerBound = b2Sub( sweepAABB.lowerBound, radius );
			sweepAABB.upperBound = b2Add( sweepAABB.upperBound, radius );
			b2ParticleBodyCollisionQueryContext queryContext = {
				.world = world,
				.system = system,
				.block = block,
				.systemId = systemId,
				.aabb = sweepAABB,
				.particleIndex = particleIndex,
				.position = p0,
				.translation = translation,
				.dt = context->dt,
				.invDt = context->invDt,
				.particleMass = context->particleMass,
				.minFraction = 1.0f,
				.bestOutput = { .normal = b2Vec2_zero, .point = b2Vec2_zero },
				.bestShapeId = b2_nullShapeId,
			};
			for ( int shapeCandidate = 0; shapeCandidate < candidates.count; ++shapeCandidate )
			{
				if ( b2UpdateParticleBodyCollisionForShape( &queryContext, candidates.shapeIndices[shapeCandidate] ) == false )
				{
					break;
				}
			}

			if ( queryContext.bestOutput.hit )
			{
				b2Vec2 correctedPosition =
					b2MulAdd( queryContext.bestOutput.point, B2_LINEAR_SLOP, queryContext.bestOutput.normal );
				b2Vec2 newVelocity = b2MulSV( context->invDt, b2Sub( correctedPosition, p0 ) );
				system->velocities[particleIndex] = newVelocity;
				b2Vec2 impulse = b2MulSV( context->particleMass, b2Sub( velocity, newVelocity ) );
				system->forces[particleIndex] = b2MulAdd( system->forces[particleIndex], context->invDt, impulse );
				b2AppendParticleBodyImpulseBlock(
					block,
					(b2ParticleBodyImpulseData){
						.shapeId = queryContext.bestShapeId,
						.impulse = impulse,
						.point = queryContext.bestOutput.point,
					} );
			}
		}

		b2FreeParticleShapeCandidateBlock( &candidates );
	}
}

static b2ParticleGroup* b2GetParticleSolidContactGroup( b2ParticleSystem* system, int indexA, int indexB )
{
	int groupIndex = system->groupIndices[indexA];
	if ( groupIndex == B2_NULL_INDEX || groupIndex != system->groupIndices[indexB] )
	{
		return NULL;
	}

	b2ParticleGroup* group = b2GetParticleGroupByLocalIndex( system, groupIndex );
	if ( group == NULL || ( group->groupFlags & b2_solidParticleGroup ) == 0 )
	{
		return NULL;
	}

	return group;
}

static void b2ParticleContactSolveTask( int startIndex, int endIndex, uint32_t workerIndex, void* taskContext )
{
	B2_UNUSED( workerIndex );

	b2ParticleContactSolveTaskContext* context = taskContext;
	b2ParticleSystem* system = context->system;
	b2World* world = context->world;

	for ( int blockIndex = startIndex; blockIndex < endIndex; ++blockIndex )
	{
		if ( blockIndex < context->contactBlockCount )
		{
			int firstContact = blockIndex * context->blockSize;
			int lastContact = b2MinInt( firstContact + context->blockSize, system->contactCount );

			for ( int i = firstContact; i < lastContact; ++i )
			{
				b2ParticleContactData* contact = system->contacts + i;
				int indexA = contact->indexA;
				int indexB = contact->indexB;
				uint32_t flags = system->flags[indexA] | system->flags[indexB];

				switch ( context->type )
				{
					case b2_particleContactTaskWeights:
						b2AppendParticleTaskFloatDelta( context, blockIndex, indexA, contact->weight );
						b2AppendParticleTaskFloatDelta( context, blockIndex, indexB, contact->weight );
						break;

					case b2_particleContactTaskRepulsive:
						if ( ( flags & b2_repulsiveParticle ) != 0 && system->groupIndices[indexA] != system->groupIndices[indexB] )
						{
							b2Vec2 impulse = b2MulSV( context->scalarA * contact->weight, contact->normal );
							b2AppendParticleTaskVec2Delta( context, blockIndex, indexA, b2Neg( impulse ) );
							b2AppendParticleTaskVec2Delta( context, blockIndex, indexB, impulse );
						}
						break;

					case b2_particleContactTaskPowder:
						if ( ( flags & b2_powderParticle ) != 0 && contact->weight > context->scalarB )
						{
							b2Vec2 impulse = b2MulSV( context->scalarA * ( contact->weight - context->scalarB ), contact->normal );
							b2AppendParticleTaskVec2Delta( context, blockIndex, indexA, b2Neg( impulse ) );
							b2AppendParticleTaskVec2Delta( context, blockIndex, indexB, impulse );
						}
						break;

					case b2_particleContactTaskViscous:
						if ( ( flags & b2_viscousParticle ) != 0 )
						{
							b2Vec2 velocityDelta =
								b2MulSV( context->scalarA * contact->weight, b2Sub( system->velocities[indexB], system->velocities[indexA] ) );
							b2AppendParticleTaskVec2Delta( context, blockIndex, indexA, velocityDelta );
							b2AppendParticleTaskVec2Delta( context, blockIndex, indexB, b2Neg( velocityDelta ) );
						}
						break;

					case b2_particleContactTaskTensileAccumulation:
						if ( ( flags & b2_tensileParticle ) != 0 )
						{
							b2Vec2 weightedNormal = b2MulSV( ( 1.0f - contact->weight ) * contact->weight, contact->normal );
							b2AppendParticleTaskVec2Delta( context, blockIndex, indexA, b2Neg( weightedNormal ) );
							b2AppendParticleTaskVec2Delta( context, blockIndex, indexB, weightedNormal );
						}
						break;

					case b2_particleContactTaskTensileVelocity:
						if ( ( flags & b2_tensileParticle ) != 0 )
						{
							float h = system->weights[indexA] + system->weights[indexB];
							b2Vec2 s = b2Sub( system->accumulationVectors[indexB], system->accumulationVectors[indexA] );
							float fn = b2MinFloat(
										   context->scalarA * ( h - 2.0f ) + context->scalarB * b2Dot( s, contact->normal ),
										   context->scalarC ) *
									   contact->weight;
							b2Vec2 impulse = b2MulSV( fn, contact->normal );
							b2AppendParticleTaskVec2Delta( context, blockIndex, indexA, b2Neg( impulse ) );
							b2AppendParticleTaskVec2Delta( context, blockIndex, indexB, impulse );
						}
						break;

					case b2_particleContactTaskSolidDepthWeights:
						if ( b2GetParticleSolidContactGroup( system, indexA, indexB ) != NULL )
						{
							b2AppendParticleTaskFloatDelta( context, blockIndex, indexA, contact->weight );
							b2AppendParticleTaskFloatDelta( context, blockIndex, indexB, contact->weight );
						}
						break;

					case b2_particleContactTaskSolidDepthRelaxation:
					{
						b2ParticleGroup* group = b2GetParticleSolidContactGroup( system, indexA, indexB );
						if ( group == NULL || context->iteration >= (int)sqrtf( (float)group->count ) )
						{
							break;
						}

						float r = 1.0f - contact->weight;
						float depthA = context->sourceDepths[indexA];
						float depthB = context->sourceDepths[indexB];
						float depthA1 = depthB + r;
						float depthB1 = depthA + r;
						if ( depthA1 < depthA )
						{
							b2AppendParticleTaskFloatDelta( context, blockIndex, indexA, depthA1 );
						}
						if ( depthB1 < depthB )
						{
							b2AppendParticleTaskFloatDelta( context, blockIndex, indexB, depthB1 );
						}
					}
					break;

					case b2_particleContactTaskSolidEjection:
					{
						int groupA = system->groupIndices[indexA];
						int groupB = system->groupIndices[indexB];
						if ( groupA == groupB )
						{
							break;
						}

						uint32_t groupFlags = 0;
						if ( groupA != B2_NULL_INDEX && system->groups[groupA].id != B2_NULL_INDEX )
						{
							groupFlags |= system->groups[groupA].groupFlags;
						}
						if ( groupB != B2_NULL_INDEX && system->groups[groupB].id != B2_NULL_INDEX )
						{
							groupFlags |= system->groups[groupB].groupFlags;
						}
						if ( ( groupFlags & b2_solidParticleGroup ) == 0 )
						{
							break;
						}

						float depth = system->depths[indexA] + system->depths[indexB];
						b2Vec2 impulse = b2MulSV( context->scalarA * depth * contact->weight, contact->normal );
						b2AppendParticleTaskVec2Delta( context, blockIndex, indexA, b2Neg( impulse ) );
						b2AppendParticleTaskVec2Delta( context, blockIndex, indexB, impulse );
					}
					break;

					case b2_particleContactTaskStaticPressure:
						if ( ( flags & b2_staticPressureParticle ) != 0 )
						{
							float w = contact->weight;
							b2AppendParticleTaskFloatDelta( context, blockIndex, indexA, w * system->staticPressures[indexB] );
							b2AppendParticleTaskFloatDelta( context, blockIndex, indexB, w * system->staticPressures[indexA] );
						}
						break;

					case b2_particleContactTaskPressure:
					{
						b2Vec2 impulse = b2MulSV(
							context->scalarB * contact->weight * ( system->accumulations[indexA] + system->accumulations[indexB] ),
							contact->normal );
						b2AppendParticleTaskVec2Delta( context, blockIndex, indexA, b2Neg( impulse ) );
						b2AppendParticleTaskVec2Delta( context, blockIndex, indexB, impulse );
					}
					break;

					case b2_particleContactTaskDamping:
					{
						b2Vec2 relativeVelocity = b2Sub( system->velocities[indexB], system->velocities[indexA] );
						float vn = b2Dot( relativeVelocity, contact->normal );
						if ( vn < 0.0f )
						{
							float damping =
								b2MaxFloat( context->scalarB * contact->weight, b2MinFloat( -context->scalarA * vn, 0.5f ) );
							b2Vec2 impulse = b2MulSV( damping * vn, contact->normal );
							b2AppendParticleTaskVec2Delta( context, blockIndex, indexA, impulse );
							b2AppendParticleTaskVec2Delta( context, blockIndex, indexB, b2Neg( impulse ) );
						}
					}
					break;

				}
			}
		}
		else
		{
			int bodyBlockIndex = blockIndex - context->contactBlockCount;
			int firstContact = bodyBlockIndex * context->blockSize;
			int lastContact = b2MinInt( firstContact + context->blockSize, system->bodyContactCount );
			b2ParticleBodyImpulseBlock* bodyImpulseBlock =
				context->bodyImpulseBlocks != NULL ? context->bodyImpulseBlocks + bodyBlockIndex : NULL;

			for ( int i = firstContact; i < lastContact; ++i )
			{
				b2ParticleBodyContactData* contact = system->bodyContacts + i;
				int particleIndex = contact->particleIndex;

				switch ( context->type )
				{
					case b2_particleContactTaskWeights:
						b2AppendParticleTaskFloatDelta( context, blockIndex, particleIndex, contact->weight );
						break;

					case b2_particleContactTaskViscous:
					{
						if ( ( system->flags[particleIndex] & b2_viscousParticle ) == 0 )
						{
							break;
						}
						b2Shape* shape = b2GetParticleContactShape( world, contact->shapeId );
						if ( shape == NULL )
						{
							break;
						}
						b2Body* body = b2BodyArray_Get( &world->bodies, shape->bodyId );
						b2Vec2 point = system->positions[particleIndex];
						b2Vec2 bodyVelocity = b2GetBodyVelocityAtPoint( world, body, point );
						b2Vec2 impulse = b2MulSV(
							context->scalarA * contact->mass * contact->weight,
							b2Sub( bodyVelocity, system->velocities[particleIndex] ) );
						float invMass = ( system->flags[particleIndex] & b2_wallParticle ) != 0 ? 0.0f : context->scalarB;
						b2AppendParticleTaskVec2Delta( context, blockIndex, particleIndex, b2MulSV( invMass, impulse ) );
						b2AppendParticleBodyImpulseBlock(
							bodyImpulseBlock, (b2ParticleBodyImpulseData){ contact->shapeId, b2Neg( impulse ), point } );
					}
					break;

					case b2_particleContactTaskPressure:
					{
						float pressure = system->accumulations[particleIndex] + context->scalarA * contact->weight;
						if ( pressure <= 0.0f || contact->mass <= 0.0f )
						{
							break;
						}
						b2Vec2 impulse = b2MulSV( context->scalarB * contact->weight * contact->mass * pressure, contact->normal );
						b2AppendParticleTaskVec2Delta( context, blockIndex, particleIndex, b2MulSV( -context->scalarC, impulse ) );
						b2AppendParticleBodyImpulseBlock(
							bodyImpulseBlock, (b2ParticleBodyImpulseData){ contact->shapeId, impulse, system->positions[particleIndex] } );
					}
					break;

					case b2_particleContactTaskDamping:
					{
						b2Shape* shape = b2GetParticleContactShape( world, contact->shapeId );
						if ( shape == NULL )
						{
							break;
						}
						b2Body* body = b2BodyArray_Get( &world->bodies, shape->bodyId );
						b2Vec2 point = system->positions[particleIndex];
						b2Vec2 relativeVelocity = b2Sub( b2GetBodyVelocityAtPoint( world, body, point ), system->velocities[particleIndex] );
						float vn = b2Dot( relativeVelocity, contact->normal );
						if ( vn >= 0.0f )
						{
							break;
						}
						float damping = b2MaxFloat( context->scalarB * contact->weight, b2MinFloat( -context->scalarA * vn, 0.5f ) );
						b2Vec2 impulse = b2MulSV( damping * contact->mass * vn, contact->normal );
						float invMass = ( system->flags[particleIndex] & b2_wallParticle ) != 0 ? 0.0f : context->scalarC;
						b2AppendParticleTaskVec2Delta( context, blockIndex, particleIndex, b2MulSV( invMass, impulse ) );
						b2AppendParticleBodyImpulseBlock(
							bodyImpulseBlock, (b2ParticleBodyImpulseData){ contact->shapeId, b2Neg( impulse ), point } );
					}
					break;

					default:
						break;
				}
			}
		}
	}
}

static void b2ParticleSpringSolveTask( int startIndex, int endIndex, uint32_t workerIndex, void* taskContext )
{
	B2_UNUSED( workerIndex );

	b2ParticleSpringSolveTaskContext* context = taskContext;
	b2ParticleSystem* system = context->system;

	for ( int blockIndex = startIndex; blockIndex < endIndex; ++blockIndex )
	{
		int first = blockIndex * context->blockSize;
		int count = context->type == b2_particleSpringTaskPair ? system->pairCount : system->triadCount;
		int last = b2MinInt( first + context->blockSize, count );

		if ( context->type == b2_particleSpringTaskPair )
		{
			for ( int i = first; i < last; ++i )
			{
				b2ParticlePairData* pair = system->pairs + i;
				if ( ( pair->flags & b2_springParticle ) == 0 )
				{
					continue;
				}

				b2Vec2 pa = b2MulAdd( system->positions[pair->indexA], context->dt, system->velocities[pair->indexA] );
				b2Vec2 pb = b2MulAdd( system->positions[pair->indexB], context->dt, system->velocities[pair->indexB] );
				b2Vec2 d = b2Sub( pb, pa );
				float distance = b2Length( d );
				if ( distance <= FLT_EPSILON )
				{
					continue;
				}

				b2Vec2 impulse = b2MulSV( context->strength * pair->strength * ( pair->distance - distance ) / distance, d );
				b2AppendParticleSpringVec2Delta( context, blockIndex, pair->indexA, b2Neg( impulse ) );
				b2AppendParticleSpringVec2Delta( context, blockIndex, pair->indexB, impulse );
			}
		}
		else
		{
			for ( int i = first; i < last; ++i )
			{
				b2ParticleTriadData* triad = system->triads + i;
				if ( ( triad->flags & b2_elasticParticle ) == 0 )
				{
					continue;
				}

				b2Vec2 pa = b2MulAdd( system->positions[triad->indexA], context->dt, system->velocities[triad->indexA] );
				b2Vec2 pb = b2MulAdd( system->positions[triad->indexB], context->dt, system->velocities[triad->indexB] );
				b2Vec2 pc = b2MulAdd( system->positions[triad->indexC], context->dt, system->velocities[triad->indexC] );
				b2Vec2 midpoint = b2MulSV( 1.0f / 3.0f, b2Add( b2Add( pa, pb ), pc ) );
				pa = b2Sub( pa, midpoint );
				pb = b2Sub( pb, midpoint );
				pc = b2Sub( pc, midpoint );

				b2Rot r = {
					.c = b2Dot( triad->pa, pa ) + b2Dot( triad->pb, pb ) + b2Dot( triad->pc, pc ),
					.s = b2Cross( triad->pa, pa ) + b2Cross( triad->pb, pb ) + b2Cross( triad->pc, pc ),
				};
				if ( r.c * r.c + r.s * r.s <= FLT_EPSILON )
				{
					continue;
				}
				r = b2NormalizeRot( r );

				float strength = context->strength * triad->strength;
				b2AppendParticleSpringVec2Delta(
					context, blockIndex, triad->indexA, b2MulSV( strength, b2Sub( b2RotateVector( r, triad->pa ), pa ) ) );
				b2AppendParticleSpringVec2Delta(
					context, blockIndex, triad->indexB, b2MulSV( strength, b2Sub( b2RotateVector( r, triad->pb ), pb ) ) );
				b2AppendParticleSpringVec2Delta(
					context, blockIndex, triad->indexC, b2MulSV( strength, b2Sub( b2RotateVector( r, triad->pc ), pc ) ) );
			}
		}
	}
}

static void b2UpdateParticleContacts( b2ParticleSystem* system )
{
	uint64_t totalTicks = b2GetTicks();
	system->allParticleFlags = b2ComputeAllParticleFlags( system );
	if ( b2NeedsParticleContactEvents( system ) )
	{
		b2SavePreviousParticleContacts( system );
	}
	else
	{
		system->previousContactCount = 0;
	}

	b2World* world = b2TryGetWorld( system->worldId );
	int count = system->particleCount;
	b2ReserveProxies( system, count );
	system->proxyCount = count;

	float diameter = 2.0f * system->def.radius;
	float invDiameter = diameter > 0.0f ? 1.0f / diameter : 0.0f;
	b2ParticleRangeTaskContext proxyContext = {
		.system = system,
		.type = b2_particleTaskBuildProxies,
		.scalar = invDiameter,
	};
	uint64_t proxyTicks = b2GetTicks();
	b2RunParticleRangeTask( world, &proxyContext, count );
	system->profile.proxies += b2GetMilliseconds( proxyTicks );

	int contactBlockSize = b2MaxInt( 1, B2_PARTICLE_TASK_MIN_RANGE / 8 );
	int maxContactBlockCount = ( count + contactBlockSize - 1 ) / contactBlockSize;
	b2ReserveParticleProxySortScratch(
		system, count, b2ParticleScratchByteCount( maxContactBlockCount, sizeof( b2ParticleContactBlock ) ) + 64 );

	b2SortParticleProxies( system );
	b2ParticleScratchBuffer_Reset( &system->scratch );

	uint64_t contactTicks = b2GetTicks();
	system->contactCount = 0;
	float diameterSquared = diameter * diameter;
	if ( system->proxyCount > 0 )
	{
		int blockSize = contactBlockSize;
		int blockCount = ( system->proxyCount + blockSize - 1 ) / blockSize;
		uint64_t scratchTicks = b2GetTicks();
		b2ParticleContactBlock* blocks = b2ParticleScratchBuffer_AllocArray( &system->scratch, blockCount, b2ParticleContactBlock );
		memset( blocks, 0, blockCount * sizeof( b2ParticleContactBlock ) );
		b2AttachParticleContactPayloads( system, blocks, blockCount );
		system->profile.scratch += b2GetMilliseconds( scratchTicks );
		b2ParticleContactTaskContext contactContext = {
			.system = system,
			.blocks = blocks,
			.proxies = system->proxies,
			.proxyCount = system->proxyCount,
			.blockSize = blockSize,
			.diameter = diameter,
			.invDiameter = invDiameter,
			.diameterSquared = diameterSquared,
		};

		b2RunParticleTask( world, system, b2ParticleContactsTask, blockCount, 1, &contactContext );
		system->profile.contactCandidates += b2GetMilliseconds( contactTicks );

		uint64_t mergeTicks = b2GetTicks();
		int totalContactCount = 0;
		int totalCandidateCount = 0;
		for ( int blockIndex = 0; blockIndex < blockCount; ++blockIndex )
		{
			totalContactCount += blocks[blockIndex].count;
			totalCandidateCount += blocks[blockIndex].candidateCount;
		}
		system->contactCandidateCount += totalCandidateCount;
		b2ReserveContacts( system, totalContactCount );
		for ( int blockIndex = 0; blockIndex < blockCount; ++blockIndex )
		{
			b2ParticleContactBlock* block = blocks + blockIndex;
			if ( block->count > 0 )
			{
				memcpy( system->contacts + system->contactCount, block->contacts, block->count * sizeof( b2ParticleContactData ) );
				system->contactCount += block->count;
			}
			if ( block->payload == NULL )
			{
				b2Free( block->contacts, block->capacity * (int)sizeof( b2ParticleContactData ) );
			}
		}
		system->profile.contactMerge += b2GetMilliseconds( mergeTicks );
	}

	b2UpdateParticleContactEvents( system );
	system->profile.contacts += b2GetMilliseconds( totalTicks );
}

static float b2GetParticleInvMass( const b2ParticleSystem* system );

static void b2UpdateParticleBodyContacts( b2World* world, b2ParticleSystem* system )
{
	uint64_t ticks = b2GetTicks();
	if ( b2NeedsParticleBodyContactEvents( system ) )
	{
		b2SavePreviousParticleBodyContacts( system );
	}
	else
	{
		system->previousBodyContactCount = 0;
	}
	system->bodyContactCount = 0;

	float radius = system->def.radius;
	float diameter = 2.0f * radius;
	float invDiameter = diameter > 0.0f ? 1.0f / diameter : 0.0f;
	float particleInvMass = b2GetParticleInvMass( system );
	if ( system->particleCount > 0 && world->shapes.count > 0 )
	{
		int blockSize = b2GetParticleBodyTaskBlockSize( world, system );
		int blockCount = ( system->particleCount + blockSize - 1 ) / blockSize;
		uint64_t scratchTicks = b2GetTicks();
		b2ParticleScratchBuffer_Reset( &system->scratch );
		b2ParticleBodyContactBlock* blocks =
			b2ParticleScratchBuffer_AllocArray( &system->scratch, blockCount, b2ParticleBodyContactBlock );
		b2ParticleShapeCandidateBlock* shapeCandidateBlocks =
			b2ParticleScratchBuffer_AllocArray( &system->scratch, blockCount, b2ParticleShapeCandidateBlock );
		memset( blocks, 0, blockCount * sizeof( b2ParticleBodyContactBlock ) );
		memset( shapeCandidateBlocks, 0, blockCount * sizeof( b2ParticleShapeCandidateBlock ) );
		b2AttachParticleBodyContactPayloads( system, blocks, blockCount );
		b2AttachParticleShapeCandidatePayloads( system, shapeCandidateBlocks, blockCount );
		system->profile.scratch += b2GetMilliseconds( scratchTicks );
		b2ParticleBodyContactTaskContext contactContext = {
			.world = world,
			.system = system,
			.blocks = blocks,
			.shapeCandidateBlocks = shapeCandidateBlocks,
			.blockSize = blockSize,
			.diameter = diameter,
			.invDiameter = invDiameter,
			.particleInvMass = particleInvMass,
		};

		uint64_t queryTicks = b2GetTicks();
		b2RunParticleTask( world, system, b2ParticleBodyContactsTask, blockCount, 1, &contactContext );
		system->profile.bodyCandidateQuery += b2GetMilliseconds( queryTicks );

		int totalContactCount = 0;
		int totalCandidateCount = 0;
		for ( int blockIndex = 0; blockIndex < blockCount; ++blockIndex )
		{
			totalContactCount += blocks[blockIndex].count;
			totalCandidateCount += blocks[blockIndex].candidateCount;
		}
		system->bodyShapeCandidateCount += totalCandidateCount;
		b2ReserveBodyContacts( system, totalContactCount );
		for ( int blockIndex = 0; blockIndex < blockCount; ++blockIndex )
		{
			b2ParticleBodyContactBlock* block = blocks + blockIndex;
			if ( block->count > 0 )
			{
				memcpy(
					system->bodyContacts + system->bodyContactCount, block->contacts,
					block->count * sizeof( b2ParticleBodyContactData ) );
				system->bodyContactCount += block->count;
			}
			if ( block->payload == NULL )
			{
				b2Free( block->contacts, block->capacity * (int)sizeof( b2ParticleBodyContactData ) );
			}
		}
	}

	b2UpdateParticleBodyContactEvents( system );
	system->profile.bodyContacts += b2GetMilliseconds( ticks );
}

static void b2ComputeParticleWeights( b2World* world, b2ParticleSystem* system )
{
	uint64_t ticks = b2GetTicks();
	b2ParticleRangeTaskContext clearContext = {
		.system = system,
		.type = b2_particleTaskClearWeights,
	};
	b2RunParticleRangeTask( world, &clearContext, system->particleCount );

	int contactBlockCount = b2GetParticleBlockCount( system->contactCount );
	int bodyContactBlockCount = b2GetParticleBlockCount( system->bodyContactCount );
	int blockCount = contactBlockCount + bodyContactBlockCount;
	if ( blockCount > 0 )
	{
		int partitionCount = b2GetParticleDeltaPartitionCount( world, system );
		int partitionSize = b2GetParticleDeltaPartitionSize( system, partitionCount );
		int deltaBlockCount = blockCount * partitionCount;
		b2ParticleFloatDeltaBlock* blocks = b2AllocateParticleFloatDeltaBlocks( system, deltaBlockCount, true );
		b2ParticleContactSolveTaskContext context = {
			.world = world,
			.system = system,
			.type = b2_particleContactTaskWeights,
			.floatBlocks = blocks,
			.contactBlockCount = contactBlockCount,
			.bodyContactBlockCount = bodyContactBlockCount,
			.blockSize = B2_PARTICLE_TASK_MIN_RANGE,
			.deltaPartitionCount = partitionCount,
			.deltaPartitionSize = partitionSize,
		};
		b2RunParticleReductionTask( world, system, b2ParticleContactSolveTask, blockCount, 1, &context );
		b2ApplyParticleFloatDeltaBlocks( world, system, blocks, blockCount, partitionCount, system->weights );
	}
	system->profile.weights += b2GetMilliseconds( ticks );
}

static float b2GetParticleMass( const b2ParticleSystem* system )
{
	float diameter = 2.0f * system->def.radius;
	return system->def.density * B2_PARTICLE_STRIDE * B2_PARTICLE_STRIDE * diameter * diameter;
}

static float b2GetParticleInvMass( const b2ParticleSystem* system )
{
	float mass = b2GetParticleMass( system );
	return mass > 0.0f ? 1.0f / mass : 0.0f;
}

static float b2QuantizeParticleLifetime( const b2ParticleSystem* system, float lifetime )
{
	if ( lifetime <= 0.0f )
	{
		return 0.0f;
	}

	float granularity = system->def.lifetimeGranularity;
	return granularity > 0.0f ? ceilf( lifetime / granularity ) * granularity : lifetime;
}

static float b2GetCriticalVelocity( const b2ParticleSystem* system, float dt )
{
	return dt > 0.0f ? 2.0f * system->def.radius / dt : 0.0f;
}

static b2Shape* b2GetParticleContactShape( b2World* world, b2ShapeId shapeId )
{
	int shapeIndex = shapeId.index1 - 1;
	if ( shapeIndex < 0 || world->shapes.count <= shapeIndex )
	{
		return NULL;
	}

	b2Shape* shape = world->shapes.data + shapeIndex;
	if ( shape->id != shapeIndex || shape->generation != shapeId.generation )
	{
		return NULL;
	}

	return shape;
}

static b2Vec2 b2GetBodyVelocityAtPoint( b2World* world, b2Body* body, b2Vec2 point )
{
	b2BodyState* state = b2GetBodyState( world, body );
	if ( state == NULL )
	{
		return b2Vec2_zero;
	}

	b2BodySim* sim = b2GetBodySim( world, body );
	b2Vec2 r = b2Sub( point, sim->center );
	b2Vec2 angularVelocity = { -state->angularVelocity * r.y, state->angularVelocity * r.x };
	return b2Add( state->linearVelocity, angularVelocity );
}

static void b2LimitParticleBodyVelocity( b2BodyState* state, float maxLinearSpeed )
{
	float v2 = b2LengthSquared( state->linearVelocity );
	if ( v2 > maxLinearSpeed * maxLinearSpeed )
	{
		state->linearVelocity = b2MulSV( maxLinearSpeed / sqrtf( v2 ), state->linearVelocity );
	}
}

static void b2ApplyParticleBodyImpulseBlocks( b2World* world, b2ParticleBodyImpulseBlock* blocks, int blockCount )
{
	int bodyCount = world->bodies.count;
	if ( bodyCount == 0 )
	{
		for ( int blockIndex = 0; blockIndex < blockCount; ++blockIndex )
		{
			b2ParticleBodyImpulseBlock* block = blocks + blockIndex;
			if ( block->payload == NULL )
			{
				b2Free( block->impulses, block->capacity * (int)sizeof( b2ParticleBodyImpulseData ) );
			}
		}
		return;
	}

	b2ParticleBodyImpulseSum* sums = b2AllocZeroInit( bodyCount * (int)sizeof( b2ParticleBodyImpulseSum ) );

	for ( int blockIndex = 0; blockIndex < blockCount; ++blockIndex )
	{
		b2ParticleBodyImpulseBlock* block = blocks + blockIndex;
		for ( int impulseIndex = 0; impulseIndex < block->count; ++impulseIndex )
		{
			b2ParticleBodyImpulseData* impulse = block->impulses + impulseIndex;
			b2Shape* shape = b2GetParticleContactShape( world, impulse->shapeId );
			if ( shape == NULL )
			{
				continue;
			}

			b2Body* body = b2BodyArray_Get( &world->bodies, shape->bodyId );
			if ( body->type != b2_dynamicBody || body->setIndex == b2_disabledSet )
			{
				continue;
			}

			b2BodySim* sim = b2GetBodySim( world, body );
			b2ParticleBodyImpulseSum* sum = sums + body->id;
			sum->linearImpulse = b2Add( sum->linearImpulse, impulse->impulse );
			sum->angularImpulse += b2Cross( b2Sub( impulse->point, sim->center ), impulse->impulse );
			sum->touched = true;
		}

		if ( block->payload == NULL )
		{
			b2Free( block->impulses, block->capacity * (int)sizeof( b2ParticleBodyImpulseData ) );
		}
	}

	for ( int bodyIndex = 0; bodyIndex < bodyCount; ++bodyIndex )
	{
		b2ParticleBodyImpulseSum* sum = sums + bodyIndex;
		if ( sum->touched == false )
		{
			continue;
		}

		b2Body* body = b2BodyArray_Get( &world->bodies, bodyIndex );
		if ( body->type != b2_dynamicBody || body->setIndex == b2_disabledSet )
		{
			continue;
		}

		if ( body->setIndex >= b2_firstSleepingSet )
		{
			b2WakeBody( world, body );
		}

		if ( body->setIndex == b2_awakeSet )
		{
			b2BodyState* state = b2GetBodyState( world, body );
			b2BodySim* sim = b2GetBodySim( world, body );
			state->linearVelocity = b2MulAdd( state->linearVelocity, sim->invMass, sum->linearImpulse );
			b2LimitParticleBodyVelocity( state, world->maxLinearSpeed );
			state->angularVelocity += sim->invInertia * sum->angularImpulse;
		}
	}

	b2Free( sums, bodyCount * (int)sizeof( b2ParticleBodyImpulseSum ) );
}

static void b2SolveParticleForceBuffer( b2World* world, b2ParticleSystem* system, float dt )
{
	uint64_t ticks = b2GetTicks();
	float velocityPerForce = dt * b2GetParticleInvMass( system );
	b2ParticleRangeTaskContext context = {
		.system = system,
		.type = b2_particleTaskSolveForces,
		.scalar = velocityPerForce,
	};
	b2RunParticleRangeTask( world, &context, system->particleCount );

	system->profile.forces += b2GetMilliseconds( ticks );
}

static void b2SolveParticleGravity( b2World* world, b2ParticleSystem* system, float dt )
{
	b2Vec2 gravity = b2MulSV( dt * system->def.gravityScale, world->gravity );
	if ( gravity.x == 0.0f && gravity.y == 0.0f )
	{
		return;
	}

	b2ParticleRangeTaskContext context = {
		.system = system,
		.type = b2_particleTaskSolveGravity,
		.vector = gravity,
	};
	b2RunParticleRangeTask( world, &context, system->particleCount );
}

static void b2SolveParticleViscous( b2World* world, b2ParticleSystem* system )
{
	if ( ( system->allParticleFlags & b2_viscousParticle ) == 0 || system->def.viscousStrength == 0.0f )
	{
		return;
	}

	float viscousStrength = system->def.viscousStrength;
	float particleInvMass = b2GetParticleInvMass( system );
	int contactBlockCount = b2GetParticleBlockCount( system->contactCount );
	int bodyContactBlockCount = b2GetParticleBlockCount( system->bodyContactCount );
	int blockCount = contactBlockCount + bodyContactBlockCount;
	if ( blockCount > 0 )
	{
		int partitionCount = b2GetParticleDeltaPartitionCount( world, system );
		int partitionSize = b2GetParticleDeltaPartitionSize( system, partitionCount );
		int deltaBlockCount = blockCount * partitionCount;
		b2ParticleVec2DeltaBlock* velocityBlocks = b2AllocateParticleVec2DeltaBlocks( system, deltaBlockCount, true );
		b2ParticleBodyImpulseBlock* bodyImpulseBlocks =
			bodyContactBlockCount > 0 ? b2AllocateParticleBodyImpulseBlocks( system, bodyContactBlockCount, false ) : NULL;
		b2ParticleContactSolveTaskContext context = {
			.world = world,
			.system = system,
			.type = b2_particleContactTaskViscous,
			.vec2Blocks = velocityBlocks,
			.bodyImpulseBlocks = bodyImpulseBlocks,
			.contactBlockCount = contactBlockCount,
			.bodyContactBlockCount = bodyContactBlockCount,
			.blockSize = B2_PARTICLE_TASK_MIN_RANGE,
			.deltaPartitionCount = partitionCount,
			.deltaPartitionSize = partitionSize,
			.scalarA = viscousStrength,
			.scalarB = particleInvMass,
		};
		b2RunParticleReductionTask( world, system, b2ParticleContactSolveTask, blockCount, 1, &context );
		b2ApplyParticleVec2DeltaBlocks( world, system, velocityBlocks, blockCount, partitionCount, system->velocities );
		b2ApplyParticleBodyImpulseBlocks( world, bodyImpulseBlocks, bodyContactBlockCount );
	}
}

static void b2SolveParticleRepulsive( b2World* world, b2ParticleSystem* system, float dt )
{
	if ( ( system->allParticleFlags & b2_repulsiveParticle ) == 0 || system->def.repulsiveStrength == 0.0f )
	{
		return;
	}

	float repulsiveStrength = system->def.repulsiveStrength * b2GetCriticalVelocity( system, dt );
	int blockCount = b2GetParticleBlockCount( system->contactCount );
	if ( blockCount > 0 )
	{
		int partitionCount = b2GetParticleDeltaPartitionCount( world, system );
		int partitionSize = b2GetParticleDeltaPartitionSize( system, partitionCount );
		int deltaBlockCount = blockCount * partitionCount;
		b2ParticleVec2DeltaBlock* blocks = b2AllocateParticleVec2DeltaBlocks( system, deltaBlockCount, true );
		b2ParticleContactSolveTaskContext context = {
			.world = world,
			.system = system,
			.type = b2_particleContactTaskRepulsive,
			.vec2Blocks = blocks,
			.contactBlockCount = blockCount,
			.blockSize = B2_PARTICLE_TASK_MIN_RANGE,
			.deltaPartitionCount = partitionCount,
			.deltaPartitionSize = partitionSize,
			.scalarA = repulsiveStrength,
		};
		b2RunParticleReductionTask( world, system, b2ParticleContactSolveTask, blockCount, 1, &context );
		b2ApplyParticleVec2DeltaBlocks( world, system, blocks, blockCount, partitionCount, system->velocities );
	}
}

static void b2SolveParticlePowder( b2World* world, b2ParticleSystem* system, float dt )
{
	if ( ( system->allParticleFlags & b2_powderParticle ) == 0 || system->def.powderStrength == 0.0f )
	{
		return;
	}

	float powderStrength = system->def.powderStrength * b2GetCriticalVelocity( system, dt );
	float minWeight = 1.0f - B2_PARTICLE_STRIDE;
	int blockCount = b2GetParticleBlockCount( system->contactCount );
	if ( blockCount > 0 )
	{
		int partitionCount = b2GetParticleDeltaPartitionCount( world, system );
		int partitionSize = b2GetParticleDeltaPartitionSize( system, partitionCount );
		int deltaBlockCount = blockCount * partitionCount;
		b2ParticleVec2DeltaBlock* blocks = b2AllocateParticleVec2DeltaBlocks( system, deltaBlockCount, true );
		b2ParticleContactSolveTaskContext context = {
			.world = world,
			.system = system,
			.type = b2_particleContactTaskPowder,
			.vec2Blocks = blocks,
			.contactBlockCount = blockCount,
			.blockSize = B2_PARTICLE_TASK_MIN_RANGE,
			.deltaPartitionCount = partitionCount,
			.deltaPartitionSize = partitionSize,
			.scalarA = powderStrength,
			.scalarB = minWeight,
		};
		b2RunParticleReductionTask( world, system, b2ParticleContactSolveTask, blockCount, 1, &context );
		b2ApplyParticleVec2DeltaBlocks( world, system, blocks, blockCount, partitionCount, system->velocities );
	}
}

static void b2SolveParticleTensile( b2World* world, b2ParticleSystem* system, float dt )
{
	if ( ( system->allParticleFlags & b2_tensileParticle ) == 0 ||
		 ( system->def.surfaceTensionPressureStrength == 0.0f && system->def.surfaceTensionNormalStrength == 0.0f ) )
	{
		return;
	}

	b2ParticleRangeTaskContext clearContext = {
		.system = system,
		.type = b2_particleTaskClearAccumulationVectors,
	};
	b2RunParticleRangeTask( world, &clearContext, system->particleCount );

	int blockCount = b2GetParticleBlockCount( system->contactCount );
	if ( blockCount > 0 )
	{
		int partitionCount = b2GetParticleDeltaPartitionCount( world, system );
		int partitionSize = b2GetParticleDeltaPartitionSize( system, partitionCount );
		int deltaBlockCount = blockCount * partitionCount;
		b2ParticleVec2DeltaBlock* blocks = b2AllocateParticleVec2DeltaBlocks( system, deltaBlockCount, true );
		b2ParticleContactSolveTaskContext context = {
			.world = world,
			.system = system,
			.type = b2_particleContactTaskTensileAccumulation,
			.vec2Blocks = blocks,
			.contactBlockCount = blockCount,
			.blockSize = B2_PARTICLE_TASK_MIN_RANGE,
			.deltaPartitionCount = partitionCount,
			.deltaPartitionSize = partitionSize,
		};
		b2RunParticleReductionTask( world, system, b2ParticleContactSolveTask, blockCount, 1, &context );
		b2ApplyParticleVec2DeltaBlocks( world, system, blocks, blockCount, partitionCount, system->accumulationVectors );
	}

	float criticalVelocity = b2GetCriticalVelocity( system, dt );
	float pressureStrength = system->def.surfaceTensionPressureStrength * criticalVelocity;
	float normalStrength = system->def.surfaceTensionNormalStrength * criticalVelocity;
	float maxVelocityVariation = B2_MAX_PARTICLE_FORCE * criticalVelocity;
	if ( blockCount > 0 )
	{
		int partitionCount = b2GetParticleDeltaPartitionCount( world, system );
		int partitionSize = b2GetParticleDeltaPartitionSize( system, partitionCount );
		int deltaBlockCount = blockCount * partitionCount;
		b2ParticleVec2DeltaBlock* blocks = b2AllocateParticleVec2DeltaBlocks( system, deltaBlockCount, true );
		b2ParticleContactSolveTaskContext context = {
			.world = world,
			.system = system,
			.type = b2_particleContactTaskTensileVelocity,
			.vec2Blocks = blocks,
			.contactBlockCount = blockCount,
			.blockSize = B2_PARTICLE_TASK_MIN_RANGE,
			.deltaPartitionCount = partitionCount,
			.deltaPartitionSize = partitionSize,
			.scalarA = pressureStrength,
			.scalarB = normalStrength,
			.scalarC = maxVelocityVariation,
		};
		b2RunParticleReductionTask( world, system, b2ParticleContactSolveTask, blockCount, 1, &context );
		b2ApplyParticleVec2DeltaBlocks( world, system, blocks, blockCount, partitionCount, system->velocities );
	}
}

static void b2ParticleSolidDepthTask( int startIndex, int endIndex, uint32_t workerIndex, void* taskContext )
{
	B2_UNUSED( workerIndex );

	b2ParticleSolidDepthTaskContext* context = taskContext;
	b2ParticleSystem* system = context->system;
	for ( int groupIndex = startIndex; groupIndex < endIndex; ++groupIndex )
	{
		b2ParticleGroup* group = system->groups + groupIndex;
		if ( group->id == B2_NULL_INDEX || ( group->groupFlags & b2_solidParticleGroup ) == 0 || group->count == 0 )
		{
			continue;
		}

		int firstParticle = group->firstIndex;
		int lastParticle = group->firstIndex + group->count;
		switch ( context->type )
		{
			case b2_particleSolidDepthTaskInitialize:
				for ( int i = firstParticle; i < lastParticle; ++i )
				{
					context->targetDepths[i] = system->accumulations[i] < 0.8f ? 0.0f : FLT_MAX;
				}
				break;

			case b2_particleSolidDepthTaskCopy:
				for ( int i = firstParticle; i < lastParticle; ++i )
				{
					context->targetDepths[i] = context->sourceDepths[i];
				}
				break;

			case b2_particleSolidDepthTaskScale:
				for ( int i = firstParticle; i < lastParticle; ++i )
				{
					float depth = context->sourceDepths[i];
					context->targetDepths[i] = depth < FLT_MAX ? depth * context->diameter : 0.0f;
				}
				break;
		}
	}
}

static void b2RunParticleSolidDepthTask( b2World* world, b2ParticleSolidDepthTaskContext* context, int groupCount )
{
	b2RunParticleTask( world, context->system, b2ParticleSolidDepthTask, groupCount, 1, context );
}

static bool b2ParticleSystemHasGroupFlag( const b2ParticleSystem* system, uint32_t groupFlag )
{
	for ( int groupIndex = 0; groupIndex < system->groupCount; ++groupIndex )
	{
		const b2ParticleGroup* group = system->groups + groupIndex;
		if ( group->id != B2_NULL_INDEX && group->count > 0 && ( group->groupFlags & groupFlag ) != 0 )
		{
			return true;
		}
	}

	return false;
}

static void b2SolveParticleSolid( b2World* world, b2ParticleSystem* system, float dt )
{
	uint64_t depthTicks = b2GetTicks();
	bool hasSolidGroups = false;
	int maxIterationCount = 0;
	for ( int groupIndex = 0; groupIndex < system->groupCount; ++groupIndex )
	{
		b2ParticleGroup* group = system->groups + groupIndex;
		if ( group->id == B2_NULL_INDEX || ( group->groupFlags & b2_solidParticleGroup ) == 0 || group->count == 0 )
		{
			continue;
		}

		hasSolidGroups = true;
		maxIterationCount = b2MaxInt( maxIterationCount, (int)sqrtf( (float)group->count ) );
	}

	int blockCount = b2GetParticleBlockCount( system->contactCount );
	if ( hasSolidGroups == false )
	{
		system->profile.solidDepth += b2GetMilliseconds( depthTicks );
		return;
	}

	float diameter = 2.0f * system->def.radius;
	b2ParticleRangeTaskContext clearDepthContext = {
		.system = system,
		.type = b2_particleTaskClearDepths,
	};
	b2RunParticleRangeTask( world, &clearDepthContext, system->particleCount );

	b2ParticleRangeTaskContext clearAccumulationContext = {
		.system = system,
		.type = b2_particleTaskClearAccumulations,
	};
	b2RunParticleRangeTask( world, &clearAccumulationContext, system->particleCount );

	if ( blockCount > 0 )
	{
		int partitionCount = b2GetParticleDeltaPartitionCount( world, system );
		int partitionSize = b2GetParticleDeltaPartitionSize( system, partitionCount );
		int deltaBlockCount = blockCount * partitionCount;
		b2ParticleFloatDeltaBlock* blocks = b2AllocateParticleFloatDeltaBlocks( system, deltaBlockCount, true );
		b2ParticleContactSolveTaskContext context = {
			.world = world,
			.system = system,
			.type = b2_particleContactTaskSolidDepthWeights,
			.floatBlocks = blocks,
			.contactBlockCount = blockCount,
			.blockSize = B2_PARTICLE_TASK_MIN_RANGE,
			.deltaPartitionCount = partitionCount,
			.deltaPartitionSize = partitionSize,
		};
		b2RunParticleReductionTask( world, system, b2ParticleContactSolveTask, blockCount, 1, &context );
		b2ApplyParticleFloatDeltaBlocks( world, system, blocks, blockCount, partitionCount, system->accumulations );
	}

	b2ParticleSolidDepthTaskContext initializeContext = {
		.system = system,
		.type = b2_particleSolidDepthTaskInitialize,
		.targetDepths = system->depths,
	};
	b2RunParticleSolidDepthTask( world, &initializeContext, system->groupCount );

	float* currentDepths = system->depths;
	float* nextDepths = system->accumulations;
	for ( int iteration = 0; iteration < maxIterationCount && blockCount > 0; ++iteration )
	{
		b2ParticleSolidDepthTaskContext copyContext = {
			.system = system,
			.type = b2_particleSolidDepthTaskCopy,
			.sourceDepths = currentDepths,
			.targetDepths = nextDepths,
		};
		b2RunParticleSolidDepthTask( world, &copyContext, system->groupCount );

		int partitionCount = b2GetParticleDeltaPartitionCount( world, system );
		int partitionSize = b2GetParticleDeltaPartitionSize( system, partitionCount );
		int deltaBlockCount = blockCount * partitionCount;
		b2ParticleFloatDeltaBlock* blocks = b2AllocateParticleFloatDeltaBlocks( system, deltaBlockCount, true );
		b2ParticleContactSolveTaskContext context = {
			.world = world,
			.system = system,
			.type = b2_particleContactTaskSolidDepthRelaxation,
			.floatBlocks = blocks,
			.contactBlockCount = blockCount,
			.blockSize = B2_PARTICLE_TASK_MIN_RANGE,
			.deltaPartitionCount = partitionCount,
			.deltaPartitionSize = partitionSize,
			.iteration = iteration,
			.sourceDepths = currentDepths,
		};
		b2RunParticleReductionTask( world, system, b2ParticleContactSolveTask, blockCount, 1, &context );

		if ( b2CountParticleFloatDeltaBlocks( blocks, blockCount, partitionCount ) == 0 )
		{
			break;
		}

		b2ApplyParticleFloatMinDeltaBlocks( world, system, blocks, blockCount, partitionCount, nextDepths );
		float* swap = currentDepths;
		currentDepths = nextDepths;
		nextDepths = swap;
	}

	b2ParticleSolidDepthTaskContext scaleContext = {
		.system = system,
		.type = b2_particleSolidDepthTaskScale,
		.sourceDepths = currentDepths,
		.targetDepths = system->depths,
		.diameter = diameter,
	};
	b2RunParticleSolidDepthTask( world, &scaleContext, system->groupCount );
	system->profile.solidDepth += b2GetMilliseconds( depthTicks );

	uint64_t ejectionTicks = b2GetTicks();
	float ejectionStrength = dt > 0.0f ? system->def.ejectionStrength / dt : 0.0f;
	if ( blockCount > 0 )
	{
		int partitionCount = b2GetParticleDeltaPartitionCount( world, system );
		int partitionSize = b2GetParticleDeltaPartitionSize( system, partitionCount );
		int deltaBlockCount = blockCount * partitionCount;
		b2ParticleVec2DeltaBlock* blocks = b2AllocateParticleVec2DeltaBlocks( system, deltaBlockCount, true );
		b2ParticleContactSolveTaskContext context = {
			.world = world,
			.system = system,
			.type = b2_particleContactTaskSolidEjection,
			.vec2Blocks = blocks,
			.contactBlockCount = blockCount,
			.blockSize = B2_PARTICLE_TASK_MIN_RANGE,
			.deltaPartitionCount = partitionCount,
			.deltaPartitionSize = partitionSize,
			.scalarA = ejectionStrength,
		};
		b2RunParticleReductionTask( world, system, b2ParticleContactSolveTask, blockCount, 1, &context );
		b2ApplyParticleVec2DeltaBlocks( world, system, blocks, blockCount, partitionCount, system->velocities );
	}
	system->profile.solidEjection += b2GetMilliseconds( ejectionTicks );
}

static void b2SolveParticleStaticPressure( b2World* world, b2ParticleSystem* system, float dt )
{
	if ( ( system->allParticleFlags & b2_staticPressureParticle ) == 0 || system->def.staticPressureStrength == 0.0f ||
		 system->def.staticPressureIterations <= 0 )
	{
		return;
	}

	float criticalVelocity = b2GetCriticalVelocity( system, dt );
	float criticalPressure = system->def.density * criticalVelocity * criticalVelocity;
	float pressurePerWeight = system->def.staticPressureStrength * criticalPressure;
	float maxPressure = B2_MAX_PARTICLE_PRESSURE * criticalPressure;
	float relaxation = system->def.staticPressureRelaxation;

	b2ParticleRangeTaskContext clearPressureContext = {
		.system = system,
		.type = b2_particleTaskClearStaticPressures,
	};
	b2RunParticleRangeTask( world, &clearPressureContext, system->particleCount );

	for ( int iteration = 0; iteration < system->def.staticPressureIterations; ++iteration )
	{
		b2ParticleRangeTaskContext clearAccumulationContext = {
			.system = system,
			.type = b2_particleTaskClearAccumulations,
		};
		b2RunParticleRangeTask( world, &clearAccumulationContext, system->particleCount );

		int blockCount = b2GetParticleBlockCount( system->contactCount );
		if ( blockCount > 0 )
		{
			int partitionCount = b2GetParticleDeltaPartitionCount( world, system );
			int partitionSize = b2GetParticleDeltaPartitionSize( system, partitionCount );
			int deltaBlockCount = blockCount * partitionCount;
			b2ParticleFloatDeltaBlock* blocks = b2AllocateParticleFloatDeltaBlocks( system, deltaBlockCount, true );
			b2ParticleContactSolveTaskContext context = {
				.world = world,
				.system = system,
				.type = b2_particleContactTaskStaticPressure,
				.floatBlocks = blocks,
				.contactBlockCount = blockCount,
				.blockSize = B2_PARTICLE_TASK_MIN_RANGE,
				.deltaPartitionCount = partitionCount,
				.deltaPartitionSize = partitionSize,
			};
			b2RunParticleReductionTask( world, system, b2ParticleContactSolveTask, blockCount, 1, &context );
			b2ApplyParticleFloatDeltaBlocks( world, system, blocks, blockCount, partitionCount, system->accumulations );
		}

		b2ParticleRangeTaskContext updateContext = {
			.system = system,
			.type = b2_particleTaskStaticPressureUpdate,
			.dt = pressurePerWeight,
			.vector = { maxPressure, 0.0f },
			.scalar = relaxation,
		};
		b2RunParticleRangeTask( world, &updateContext, system->particleCount );
	}
}

static void b2SolveParticlePressure( b2World* world, b2ParticleSystem* system, float dt )
{
	uint64_t ticks = b2GetTicks();
	float diameter = 2.0f * system->def.radius;
	float criticalVelocity = b2GetCriticalVelocity( system, dt );
	float criticalPressure = system->def.density * criticalVelocity * criticalVelocity;
	float pressurePerWeight = system->def.pressureStrength * criticalPressure;
	float maxPressure = B2_MAX_PARTICLE_PRESSURE * criticalPressure;
	float scale = dt / ( system->def.density * diameter );
	float particleInvMass = b2GetParticleInvMass( system );

	b2ParticleRangeTaskContext pressureContext = {
		.system = system,
		.type = b2_particleTaskPressureAccumulation,
		.dt = pressurePerWeight,
		.scalar = maxPressure,
	};
	b2RunParticleRangeTask( world, &pressureContext, system->particleCount );

	int contactBlockCount = b2GetParticleBlockCount( system->contactCount );
	int bodyContactBlockCount = b2GetParticleBlockCount( system->bodyContactCount );
	int blockCount = contactBlockCount + bodyContactBlockCount;
	if ( blockCount > 0 )
	{
		int partitionCount = b2GetParticleDeltaPartitionCount( world, system );
		int partitionSize = b2GetParticleDeltaPartitionSize( system, partitionCount );
		int deltaBlockCount = blockCount * partitionCount;
		b2ParticleVec2DeltaBlock* velocityBlocks = b2AllocateParticleVec2DeltaBlocks( system, deltaBlockCount, true );
		b2ParticleBodyImpulseBlock* bodyImpulseBlocks =
			bodyContactBlockCount > 0 ? b2AllocateParticleBodyImpulseBlocks( system, bodyContactBlockCount, false ) : NULL;
		b2ParticleContactSolveTaskContext context = {
			.world = world,
			.system = system,
			.type = b2_particleContactTaskPressure,
			.vec2Blocks = velocityBlocks,
			.bodyImpulseBlocks = bodyImpulseBlocks,
			.contactBlockCount = contactBlockCount,
			.bodyContactBlockCount = bodyContactBlockCount,
			.blockSize = B2_PARTICLE_TASK_MIN_RANGE,
			.deltaPartitionCount = partitionCount,
			.deltaPartitionSize = partitionSize,
			.scalarA = pressurePerWeight,
			.scalarB = scale,
			.scalarC = particleInvMass,
		};
		b2RunParticleReductionTask( world, system, b2ParticleContactSolveTask, blockCount, 1, &context );
		b2ApplyParticleVec2DeltaBlocks( world, system, velocityBlocks, blockCount, partitionCount, system->velocities );
		b2ApplyParticleBodyImpulseBlocks( world, bodyImpulseBlocks, bodyContactBlockCount );
	}

	system->profile.pressure += b2GetMilliseconds( ticks );
}

static void b2SolveParticleDamping( b2World* world, b2ParticleSystem* system, float dt )
{
	uint64_t ticks = b2GetTicks();
	float criticalVelocity = b2GetCriticalVelocity( system, dt );
	float quadraticDamping = criticalVelocity > 0.0f ? 1.0f / criticalVelocity : 0.0f;
	float particleInvMass = b2GetParticleInvMass( system );
	int contactBlockCount = b2GetParticleBlockCount( system->contactCount );
	int bodyContactBlockCount = b2GetParticleBlockCount( system->bodyContactCount );
	int blockCount = contactBlockCount + bodyContactBlockCount;
	if ( blockCount > 0 )
	{
		int partitionCount = b2GetParticleDeltaPartitionCount( world, system );
		int partitionSize = b2GetParticleDeltaPartitionSize( system, partitionCount );
		int deltaBlockCount = blockCount * partitionCount;
		b2ParticleVec2DeltaBlock* velocityBlocks = b2AllocateParticleVec2DeltaBlocks( system, deltaBlockCount, true );
		b2ParticleBodyImpulseBlock* bodyImpulseBlocks =
			bodyContactBlockCount > 0 ? b2AllocateParticleBodyImpulseBlocks( system, bodyContactBlockCount, false ) : NULL;
		b2ParticleContactSolveTaskContext context = {
			.world = world,
			.system = system,
			.type = b2_particleContactTaskDamping,
			.vec2Blocks = velocityBlocks,
			.bodyImpulseBlocks = bodyImpulseBlocks,
			.contactBlockCount = contactBlockCount,
			.bodyContactBlockCount = bodyContactBlockCount,
			.blockSize = B2_PARTICLE_TASK_MIN_RANGE,
			.deltaPartitionCount = partitionCount,
			.deltaPartitionSize = partitionSize,
			.scalarA = quadraticDamping,
			.scalarB = system->def.dampingStrength,
			.scalarC = particleInvMass,
		};
		b2RunParticleReductionTask( world, system, b2ParticleContactSolveTask, blockCount, 1, &context );
		b2ApplyParticleVec2DeltaBlocks( world, system, velocityBlocks, blockCount, partitionCount, system->velocities );
		b2ApplyParticleBodyImpulseBlocks( world, bodyImpulseBlocks, bodyContactBlockCount );
	}

	system->profile.damping += b2GetMilliseconds( ticks );
}

static void b2MixParticleColors( b2ParticleColor* colorA, b2ParticleColor* colorB, int strength )
{
	int dr = ( strength * ( (int)colorB->r - (int)colorA->r ) ) >> 8;
	int dg = ( strength * ( (int)colorB->g - (int)colorA->g ) ) >> 8;
	int db = ( strength * ( (int)colorB->b - (int)colorA->b ) ) >> 8;
	int da = ( strength * ( (int)colorB->a - (int)colorA->a ) ) >> 8;

	colorA->r = (uint8_t)( (int)colorA->r + dr );
	colorA->g = (uint8_t)( (int)colorA->g + dg );
	colorA->b = (uint8_t)( (int)colorA->b + db );
	colorA->a = (uint8_t)( (int)colorA->a + da );
	colorB->r = (uint8_t)( (int)colorB->r - dr );
	colorB->g = (uint8_t)( (int)colorB->g - dg );
	colorB->b = (uint8_t)( (int)colorB->b - db );
	colorB->a = (uint8_t)( (int)colorB->a - da );
}

static void b2SolveParticleColorMixing( b2ParticleSystem* system )
{
	if ( system->colors == NULL || ( system->allParticleFlags & b2_colorMixingParticle ) == 0 )
	{
		return;
	}

	int strength = (int)( 128.0f * system->def.colorMixingStrength );
	if ( strength == 0 )
	{
		return;
	}

	for ( int contactIndex = 0; contactIndex < system->contactCount; ++contactIndex )
	{
		b2ParticleContactData* contact = system->contacts + contactIndex;
		int indexA = contact->indexA;
		int indexB = contact->indexB;
		if ( ( system->flags[indexA] & system->flags[indexB] & b2_colorMixingParticle ) != 0 )
		{
			b2MixParticleColors( system->colors + indexA, system->colors + indexB, strength );
		}
	}
}

static void b2SolveParticleSpringElastic( b2World* world, b2ParticleSystem* system, float dt )
{
	float invDt = dt > 0.0f ? 1.0f / dt : 0.0f;
	float springStrength = invDt * system->def.springStrength;
	int pairBlockCount = b2GetParticleBlockCount( system->pairCount );
	if ( pairBlockCount > 0 && ( system->allParticleFlags & b2_springParticle ) != 0 && system->def.springStrength != 0.0f )
	{
		int partitionCount = b2GetParticleDeltaPartitionCount( world, system );
		int partitionSize = b2GetParticleDeltaPartitionSize( system, partitionCount );
		int deltaBlockCount = pairBlockCount * partitionCount;
		b2ParticleVec2DeltaBlock* blocks = b2AllocateParticleVec2DeltaBlocks( system, deltaBlockCount, true );
		b2ParticleSpringSolveTaskContext context = {
			.system = system,
			.type = b2_particleSpringTaskPair,
			.vec2Blocks = blocks,
			.blockSize = B2_PARTICLE_TASK_MIN_RANGE,
			.deltaPartitionCount = partitionCount,
			.deltaPartitionSize = partitionSize,
			.dt = dt,
			.invDt = invDt,
			.strength = springStrength,
		};
		b2RunParticleReductionTask( world, system, b2ParticleSpringSolveTask, pairBlockCount, 1, &context );
		b2ApplyParticleVec2DeltaBlocks( world, system, blocks, pairBlockCount, partitionCount, system->velocities );
	}

	float elasticStrength = invDt * system->def.elasticStrength;
	int triadBlockCount = b2GetParticleBlockCount( system->triadCount );
	if ( triadBlockCount > 0 && ( system->allParticleFlags & b2_elasticParticle ) != 0 && system->def.elasticStrength != 0.0f )
	{
		int partitionCount = b2GetParticleDeltaPartitionCount( world, system );
		int partitionSize = b2GetParticleDeltaPartitionSize( system, partitionCount );
		int deltaBlockCount = triadBlockCount * partitionCount;
		b2ParticleVec2DeltaBlock* blocks = b2AllocateParticleVec2DeltaBlocks( system, deltaBlockCount, true );
		b2ParticleSpringSolveTaskContext context = {
			.system = system,
			.type = b2_particleSpringTaskTriad,
			.vec2Blocks = blocks,
			.blockSize = B2_PARTICLE_TASK_MIN_RANGE,
			.deltaPartitionCount = partitionCount,
			.deltaPartitionSize = partitionSize,
			.dt = dt,
			.invDt = invDt,
			.strength = elasticStrength,
		};
		b2RunParticleReductionTask( world, system, b2ParticleSpringSolveTask, triadBlockCount, 1, &context );
		b2ApplyParticleVec2DeltaBlocks( world, system, blocks, triadBlockCount, partitionCount, system->velocities );
	}
}

static b2Vec2 b2GetParticleLinearVelocity( b2ParticleSystem* system, int particleIndex, b2Vec2 point )
{
	b2ParticleGroup* group = b2GetParticleGroupByLocalIndex( system, system->groupIndices[particleIndex] );
	if ( group != NULL && ( group->groupFlags & b2_rigidParticleGroup ) != 0 )
	{
		b2Vec2 r = b2Sub( point, group->center );
		return b2Add( group->linearVelocity, b2CrossSV( group->angularVelocity, r ) );
	}

	return system->velocities[particleIndex];
}

static bool b2SolveParticleBarrierIntersection( b2Vec2 pa, b2Vec2 va, b2Vec2 pb, b2Vec2 vb, b2Vec2 pc, b2Vec2 vc, float tmax,
												float* fraction )
{
	b2Vec2 pba = b2Sub( pb, pa );
	b2Vec2 vba = b2Sub( vb, va );
	b2Vec2 pca = b2Sub( pc, pa );
	b2Vec2 vca = b2Sub( vc, va );
	float e2 = b2Cross( vba, vca );
	float e1 = b2Cross( pba, vca ) - b2Cross( pca, vba );
	float e0 = b2Cross( pba, pca );
	float t = -1.0f;
	if ( b2AbsFloat( e2 ) < FLT_EPSILON )
	{
		if ( b2AbsFloat( e1 ) < FLT_EPSILON )
		{
			return false;
		}
		t = -e0 / e1;
	}
	else
	{
		float det = e1 * e1 - 4.0f * e2 * e0;
		if ( det < 0.0f )
		{
			return false;
		}

		float sqrtDet = sqrtf( det );
		float invDenominator = 0.5f / e2;
		float t1 = ( -e1 - sqrtDet ) * invDenominator;
		float t2 = ( -e1 + sqrtDet ) * invDenominator;
		if ( 0.0f <= t1 && t1 < tmax )
		{
			t = t1;
		}
		else if ( 0.0f <= t2 && t2 < tmax )
		{
			t = t2;
		}
		else
		{
			return false;
		}
	}

	if ( t < 0.0f || tmax <= t )
	{
		return false;
	}

	b2Vec2 qba = b2MulAdd( pba, t, vba );
	b2Vec2 qca = b2MulAdd( pca, t, vca );
	float denominator = b2Dot( qba, qba );
	if ( denominator <= FLT_EPSILON )
	{
		return false;
	}

	float s = b2Dot( qba, qca ) / denominator;
	if ( s < 0.0f || 1.0f < s )
	{
		return false;
	}

	*fraction = s;
	return true;
}

typedef struct b2ParticleBarrierGroupDeltaData
{
	int groupIndex;
	b2Vec2 linearDelta;
	float angularDelta;
} b2ParticleBarrierGroupDeltaData;

typedef struct b2ParticleBarrierBlock
{
	b2ParticleBarrierGroupDeltaData* groupDeltas;
	int groupDeltaCount;
	int groupDeltaCapacity;
	int candidateCount;
	b2ParticleScratchArray* groupDeltaPayload;
} b2ParticleBarrierBlock;

typedef struct b2ParticleBarrierTaskContext
{
	b2ParticleSystem* system;
	b2ParticleBarrierBlock* blocks;
	b2ParticleVec2DeltaBlock* velocityBlocks;
	b2ParticleVec2DeltaBlock* forceBlocks;
	b2ParticleCell* cells;
	int* nextParticles;
	int* cellTable;
	int cellCount;
	int cellTableCapacity;
	int minCellX;
	int minCellY;
	int maxCellX;
	int maxCellY;
	int blockSize;
	int deltaPartitionCount;
	int deltaPartitionSize;
	float invDiameter;
	float tmax;
	float invDt;
	float particleMass;
	float queryExpansion;
} b2ParticleBarrierTaskContext;

static void b2AttachParticleBarrierGroupDeltaPayloads( b2ParticleSystem* system, b2ParticleBarrierBlock* blocks,
													  int blockCount )
{
	b2ReserveParticlePayloadStorage(
		&system->barrierGroupDeltaBlockPayloads, blockCount, (int)sizeof( b2ParticleBarrierGroupDeltaData ) );
	for ( int i = 0; i < blockCount; ++i )
	{
		b2ParticleScratchArray* payload = system->barrierGroupDeltaBlockPayloads.arrays + i;
		blocks[i].groupDeltaPayload = payload;
		blocks[i].groupDeltas = b2ParticleScratchArray_Data( payload, b2ParticleBarrierGroupDeltaData );
		blocks[i].groupDeltaCapacity = payload->capacity;
	}
}

static void b2ReserveParticleBarrierGroupDeltaBlock( b2ParticleBarrierBlock* block, int capacity )
{
	if ( block == NULL || capacity <= block->groupDeltaCapacity )
	{
		return;
	}

	int oldCapacity = block->groupDeltaCapacity;
	int newCapacity = b2MaxParticleCapacity( oldCapacity, capacity );
	if ( block->groupDeltaPayload != NULL )
	{
		b2ParticleScratchArray_Reserve( block->groupDeltaPayload, newCapacity );
		block->groupDeltas = b2ParticleScratchArray_Data( block->groupDeltaPayload, b2ParticleBarrierGroupDeltaData );
		newCapacity = block->groupDeltaPayload->capacity;
	}
	else
	{
		block->groupDeltas =
			b2GrowParticleBuffer( block->groupDeltas, oldCapacity, newCapacity, sizeof( b2ParticleBarrierGroupDeltaData ) );
	}
	block->groupDeltaCapacity = newCapacity;
}

static void b2AppendParticleBarrierGroupDeltaBlock( b2ParticleBarrierBlock* block, b2ParticleBarrierGroupDeltaData delta )
{
	b2ReserveParticleBarrierGroupDeltaBlock( block, block->groupDeltaCount + 1 );
	block->groupDeltas[block->groupDeltaCount++] = delta;
}

static void b2AppendParticleBarrierVec2Delta( b2ParticleVec2DeltaBlock* blocks, int blockIndex, int partitionCount,
											 int partitionSize, int particleIndex, b2Vec2 delta )
{
	int partition = b2GetParticleDeltaPartition( particleIndex, partitionCount, partitionSize );
	b2AppendParticleVec2DeltaBlock( blocks + blockIndex * partitionCount + partition, particleIndex, delta );
}

static void b2AppendParticleBarrierVelocityDelta( b2ParticleBarrierTaskContext* context, int blockIndex, int particleIndex,
												 b2Vec2 velocityDelta )
{
	b2ParticleSystem* system = context->system;
	b2ParticleGroup* group = b2GetParticleGroupByLocalIndex( system, system->groupIndices[particleIndex] );
	if ( group != NULL && ( group->groupFlags & b2_rigidParticleGroup ) != 0 )
	{
		b2Vec2 impulse = b2MulSV( context->particleMass, velocityDelta );
		b2Vec2 linearDelta = b2Vec2_zero;
		float angularDelta = 0.0f;
		if ( group->mass > 0.0f )
		{
			linearDelta = b2MulSV( 1.0f / group->mass, impulse );
		}
		if ( group->inertia > 0.0f )
		{
			angularDelta = b2Cross( b2Sub( system->positions[particleIndex], group->center ), impulse ) / group->inertia;
		}
		if ( b2LengthSquared( linearDelta ) > 0.0f || angularDelta != 0.0f )
		{
			b2AppendParticleBarrierGroupDeltaBlock(
				context->blocks + blockIndex,
				(b2ParticleBarrierGroupDeltaData){
					.groupIndex = group->localIndex,
					.linearDelta = linearDelta,
					.angularDelta = angularDelta,
				} );
		}
	}
	else
	{
		b2AppendParticleBarrierVec2Delta(
			context->velocityBlocks, blockIndex, context->deltaPartitionCount, context->deltaPartitionSize, particleIndex,
			velocityDelta );
	}

	b2Vec2 forceDelta = b2MulSV( -context->invDt * context->particleMass, velocityDelta );
	b2AppendParticleBarrierVec2Delta(
		context->forceBlocks, blockIndex, context->deltaPartitionCount, context->deltaPartitionSize, particleIndex,
		forceDelta );
}

static float b2GetParticleBarrierMaxSpeed( b2ParticleSystem* system )
{
	float maxSpeedSquared = 0.0f;
	for ( int i = 0; i < system->particleCount; ++i )
	{
		b2Vec2 velocity = b2GetParticleLinearVelocity( system, i, system->positions[i] );
		maxSpeedSquared = b2MaxFloat( maxSpeedSquared, b2LengthSquared( velocity ) );
	}

	return maxSpeedSquared > 0.0f ? sqrtf( maxSpeedSquared ) : 0.0f;
}

static void b2GetParticleBarrierGridBounds( const b2ParticleCellGrid* grid, int* minCellX, int* minCellY, int* maxCellX,
										   int* maxCellY )
{
	*minCellX = INT32_MAX;
	*minCellY = INT32_MAX;
	*maxCellX = INT32_MIN;
	*maxCellY = INT32_MIN;

	for ( int i = 0; i < grid->cellCount; ++i )
	{
		const b2ParticleCell* cell = grid->cells + i;
		*minCellX = b2MinInt( *minCellX, cell->cellX );
		*minCellY = b2MinInt( *minCellY, cell->cellY );
		*maxCellX = b2MaxInt( *maxCellX, cell->cellX );
		*maxCellY = b2MaxInt( *maxCellY, cell->cellY );
	}
}

static b2AABB b2GetParticleBarrierPairAABB( b2Vec2 pa, b2Vec2 va, b2Vec2 pb, b2Vec2 vb, float tmax, float expansion )
{
	b2Vec2 pa1 = b2MulAdd( pa, tmax, va );
	b2Vec2 pb1 = b2MulAdd( pb, tmax, vb );
	b2Vec2 lower = b2Min( b2Min( pa, pb ), b2Min( pa1, pb1 ) );
	b2Vec2 upper = b2Max( b2Max( pa, pb ), b2Max( pa1, pb1 ) );
	b2Vec2 extension = { expansion, expansion };
	return (b2AABB){ b2Sub( lower, extension ), b2Add( upper, extension ) };
}

static bool b2GetParticleBarrierCellRange( const b2ParticleBarrierTaskContext* context, b2AABB aabb, int* firstCellX,
										  int* firstCellY, int* lastCellX, int* lastCellY )
{
	if ( context->invDiameter > 0.0f )
	{
		*firstCellX = (int)floorf( aabb.lowerBound.x * context->invDiameter );
		*firstCellY = (int)floorf( aabb.lowerBound.y * context->invDiameter );
		*lastCellX = (int)floorf( aabb.upperBound.x * context->invDiameter );
		*lastCellY = (int)floorf( aabb.upperBound.y * context->invDiameter );
	}
	else
	{
		*firstCellX = 0;
		*firstCellY = 0;
		*lastCellX = 0;
		*lastCellY = 0;
	}

	*firstCellX = b2MaxInt( *firstCellX, context->minCellX );
	*firstCellY = b2MaxInt( *firstCellY, context->minCellY );
	*lastCellX = b2MinInt( *lastCellX, context->maxCellX );
	*lastCellY = b2MinInt( *lastCellY, context->maxCellY );
	return *firstCellX <= *lastCellX && *firstCellY <= *lastCellY;
}

static void b2SolveParticleBarrierPair( b2ParticleBarrierTaskContext* context, int blockIndex, b2ParticleCellGrid* gridView,
									   const b2ParticlePairData* pair )
{
	if ( ( pair->flags & b2_barrierParticle ) == 0 )
	{
		return;
	}

	b2ParticleSystem* system = context->system;
	b2ParticleBarrierBlock* block = context->blocks + blockIndex;
	int indexA = pair->indexA;
	int indexB = pair->indexB;
	int groupA = system->groupIndices[indexA];
	int groupB = system->groupIndices[indexB];
	b2Vec2 pa = system->positions[indexA];
	b2Vec2 pb = system->positions[indexB];
	b2Vec2 va = b2GetParticleLinearVelocity( system, indexA, pa );
	b2Vec2 vb = b2GetParticleLinearVelocity( system, indexB, pb );
	b2Vec2 vba = b2Sub( vb, va );
	b2AABB pairAABB = b2GetParticleBarrierPairAABB( pa, va, pb, vb, context->tmax, context->queryExpansion );
	int firstCellX = 0;
	int firstCellY = 0;
	int lastCellX = 0;
	int lastCellY = 0;
	if ( b2GetParticleBarrierCellRange( context, pairAABB, &firstCellX, &firstCellY, &lastCellX, &lastCellY ) == false )
	{
		return;
	}

	for ( int cellY = firstCellY; cellY <= lastCellY; ++cellY )
	{
		for ( int cellX = firstCellX; cellX <= lastCellX; ++cellX )
		{
			int cellIndex = b2FindParticleCell( gridView, b2GetParticleCellKey( cellX, cellY ) );
			if ( cellIndex == B2_NULL_INDEX )
			{
				continue;
			}

			b2ParticleCell* cell = context->cells + cellIndex;
			for ( int indexC = cell->firstParticle; indexC != B2_NULL_INDEX; indexC = context->nextParticles[indexC] )
			{
				if ( indexC == indexA || indexC == indexB || system->groupIndices[indexC] == groupA ||
					 system->groupIndices[indexC] == groupB )
				{
					continue;
				}
				block->candidateCount += 1;

				b2Vec2 pc = system->positions[indexC];
				b2Vec2 vc = b2GetParticleLinearVelocity( system, indexC, pc );
				float s = 0.0f;
				if ( b2SolveParticleBarrierIntersection( pa, va, pb, vb, pc, vc, context->tmax, &s ) == false )
				{
					continue;
				}

				b2Vec2 dv = b2Sub( b2MulAdd( va, s, vba ), vc );
				b2AppendParticleBarrierVelocityDelta( context, blockIndex, indexC, dv );
			}
		}
	}
}

static void b2ParticleBarrierTask( int startIndex, int endIndex, uint32_t workerIndex, void* taskContext )
{
	B2_UNUSED( workerIndex );

	b2ParticleBarrierTaskContext* context = taskContext;
	b2ParticleCellGrid gridView = {
		.cells = context->cells,
		.cellCount = context->cellCount,
		.table = context->cellTable,
		.tableCapacity = context->cellTableCapacity,
	};

	for ( int blockIndex = startIndex; blockIndex < endIndex; ++blockIndex )
	{
		int firstPair = blockIndex * context->blockSize;
		int lastPair = b2MinInt( firstPair + context->blockSize, context->system->pairCount );
		for ( int pairIndex = firstPair; pairIndex < lastPair; ++pairIndex )
		{
			b2SolveParticleBarrierPair( context, blockIndex, &gridView, context->system->pairs + pairIndex );
		}
	}
}

static void b2ApplyParticleBarrierGroupDeltas( b2ParticleSystem* system, b2ParticleBarrierBlock* blocks, int blockCount )
{
	for ( int blockIndex = 0; blockIndex < blockCount; ++blockIndex )
	{
		b2ParticleBarrierBlock* block = blocks + blockIndex;
		for ( int i = 0; i < block->groupDeltaCount; ++i )
		{
			b2ParticleBarrierGroupDeltaData* delta = block->groupDeltas + i;
			b2ParticleGroup* group = b2GetParticleGroupByLocalIndex( system, delta->groupIndex );
			if ( group == NULL || ( group->groupFlags & b2_rigidParticleGroup ) == 0 )
			{
				continue;
			}

			group->linearVelocity = b2Add( group->linearVelocity, delta->linearDelta );
			group->angularVelocity += delta->angularDelta;
		}
		if ( block->groupDeltaPayload == NULL )
		{
			b2Free( block->groupDeltas, block->groupDeltaCapacity * (int)sizeof( b2ParticleBarrierGroupDeltaData ) );
		}
	}
}

static void b2SolveBarrierParticles( b2ParticleSystem* system, float dt )
{
	uint64_t ticks = b2GetTicks();
	b2World* world = b2TryGetWorld( system->worldId );
	b2ParticleRangeTaskContext wallContext = {
		.system = system,
		.type = b2_particleTaskSolveBarrierWalls,
	};
	b2RunParticleRangeTask( world, &wallContext, system->particleCount );

	bool hasBarrierPair = false;
	for ( int i = 0; i < system->pairCount; ++i )
	{
		if ( ( system->pairs[i].flags & b2_barrierParticle ) != 0 )
		{
			hasBarrierPair = true;
			break;
		}
	}

	if ( hasBarrierPair == false )
	{
		system->profile.barrier += b2GetMilliseconds( ticks );
		return;
	}

	B2_ASSERT( system->proxyCount == system->particleCount );
	int blockSize = b2MaxInt( 1, B2_PARTICLE_TASK_MIN_RANGE / 8 );
	int blockCount = ( system->pairCount + blockSize - 1 ) / blockSize;
	int partitionCount = b2GetParticleDeltaPartitionCount( world, system );
	int deltaBlockCount = blockCount * partitionCount;
	int barrierExtraBytes = 0;
	barrierExtraBytes =
		b2AddParticleScratchReserveBytes( barrierExtraBytes, blockCount, sizeof( b2ParticleBarrierBlock ) );
	barrierExtraBytes =
		b2AddParticleScratchReserveBytes( barrierExtraBytes, deltaBlockCount, sizeof( b2ParticleVec2DeltaBlock ) );
	barrierExtraBytes =
		b2AddParticleScratchReserveBytes( barrierExtraBytes, deltaBlockCount, sizeof( b2ParticleVec2DeltaBlock ) );
	b2ReserveParticleCellGridScratch( system, system->particleCount, barrierExtraBytes );

	b2ParticleCellGrid grid = { 0 };
	b2BuildParticleCellGrid( system, &grid );
	if ( grid.cellCount == 0 )
	{
		b2DestroyParticleCellGrid( &grid );
		system->profile.barrier += b2GetMilliseconds( ticks );
		return;
	}

	float invDt = dt > 0.0f ? 1.0f / dt : 0.0f;
	float particleMass = b2GetParticleMass( system );
	float tmax = B2_BARRIER_COLLISION_TIME * dt;
	float diameter = 2.0f * system->def.radius;
	float invDiameter = diameter > 0.0f ? 1.0f / diameter : 0.0f;
	float queryExpansion = system->def.radius + tmax * b2GetParticleBarrierMaxSpeed( system );
	int minCellX = 0;
	int minCellY = 0;
	int maxCellX = 0;
	int maxCellY = 0;
	b2GetParticleBarrierGridBounds( &grid, &minCellX, &minCellY, &maxCellX, &maxCellY );

	uint64_t scratchTicks = b2GetTicks();
	b2ParticleBarrierBlock* blocks = b2ParticleScratchBuffer_AllocArray( &system->scratch, blockCount, b2ParticleBarrierBlock );
	memset( blocks, 0, blockCount * sizeof( b2ParticleBarrierBlock ) );
	b2AttachParticleBarrierGroupDeltaPayloads( system, blocks, blockCount );
	system->profile.scratch += b2GetMilliseconds( scratchTicks );
	int partitionSize = b2GetParticleDeltaPartitionSize( system, partitionCount );
	b2ParticleVec2DeltaBlock* velocityBlocks = b2AllocateParticleVec2DeltaBlocks( system, deltaBlockCount, false );
	b2ParticleVec2DeltaBlock* forceBlocks = b2AllocateParticleVec2DeltaBlocks( system, deltaBlockCount, false );
	b2ParticleBarrierTaskContext context = {
		.system = system,
		.blocks = blocks,
		.velocityBlocks = velocityBlocks,
		.forceBlocks = forceBlocks,
		.cells = grid.cells,
		.nextParticles = grid.nextParticles,
		.cellTable = grid.table,
		.cellCount = grid.cellCount,
		.cellTableCapacity = grid.tableCapacity,
		.minCellX = minCellX,
		.minCellY = minCellY,
		.maxCellX = maxCellX,
		.maxCellY = maxCellY,
		.blockSize = blockSize,
		.deltaPartitionCount = partitionCount,
		.deltaPartitionSize = partitionSize,
		.invDiameter = invDiameter,
		.tmax = tmax,
		.invDt = invDt,
		.particleMass = particleMass,
		.queryExpansion = queryExpansion,
	};
	b2RunParticleReductionTask( world, system, b2ParticleBarrierTask, blockCount, 1, &context );

	int totalCandidateCount = 0;
	for ( int blockIndex = 0; blockIndex < blockCount; ++blockIndex )
	{
		totalCandidateCount += blocks[blockIndex].candidateCount;
	}
	system->barrierCandidateCount += totalCandidateCount;
	b2ApplyParticleVec2DeltaBlocks( world, system, velocityBlocks, blockCount, partitionCount, system->velocities );
	b2ApplyParticleVec2DeltaBlocks( world, system, forceBlocks, blockCount, partitionCount, system->forces );
	b2ApplyParticleBarrierGroupDeltas( system, blocks, blockCount );
	b2DestroyParticleCellGrid( &grid );
	system->profile.barrier += b2GetMilliseconds( ticks );
}

static b2Vec2 b2TransformPointWithUnnormalizedRotation( b2Transform transform, b2Vec2 point )
{
	return (b2Vec2){
		transform.p.x + transform.q.c * point.x - transform.q.s * point.y,
		transform.p.y + transform.q.s * point.x + transform.q.c * point.y,
	};
}

static void b2ParticleRigidGroupTask( int startIndex, int endIndex, uint32_t workerIndex, void* taskContext )
{
	B2_UNUSED( workerIndex );

	b2ParticleRigidGroupTaskContext* context = taskContext;
	b2ParticleSystem* system = context->system;
	for ( int i = startIndex; i < endIndex; ++i )
	{
		if ( system->groupIndices[i] == context->groupLocalIndex )
		{
			system->velocities[i] = b2TransformPointWithUnnormalizedRotation( context->velocityTransform, system->positions[i] );
		}
	}
}

static void b2SolveRigidParticles( b2ParticleSystem* system, float dt )
{
	if ( b2ParticleSystemHasGroupFlag( system, b2_rigidParticleGroup ) == false )
	{
		return;
	}

	b2RefreshGroups( system );
	b2World* world = b2TryGetWorld( system->worldId );
	float invDt = dt > 0.0f ? 1.0f / dt : 0.0f;
	for ( int groupIndex = 0; groupIndex < system->groupCount; ++groupIndex )
	{
		b2ParticleGroup* group = system->groups + groupIndex;
		if ( group->id == B2_NULL_INDEX || ( group->groupFlags & b2_rigidParticleGroup ) == 0 )
		{
			continue;
		}

		b2Rot rotation = b2MakeRot( dt * group->angularVelocity );
		b2Transform transform = {
			.p = b2Sub( b2MulAdd( group->center, dt, group->linearVelocity ), b2RotateVector( rotation, group->center ) ),
			.q = rotation,
		};
		group->transform = b2MulTransforms( transform, group->transform );

		b2Transform velocityTransform = {
			.p = b2MulSV( invDt, transform.p ),
			.q = { invDt * ( transform.q.c - 1.0f ), invDt * transform.q.s },
		};

		b2ParticleRigidGroupTaskContext context = {
			.system = system,
			.groupLocalIndex = group->localIndex,
			.velocityTransform = velocityTransform,
		};
		b2RunParticleTask( world, system, b2ParticleRigidGroupTask, system->particleCount, B2_PARTICLE_TASK_MIN_RANGE, &context );
	}
}

static void b2SolveParticleBodyCollision( b2World* world, b2ParticleSystem* system, float dt )
{
	uint64_t ticks = b2GetTicks();
	float invDt = dt > 0.0f ? 1.0f / dt : 0.0f;
	float particleMass = b2GetParticleMass( system );
	if ( system->particleCount > 0 && world->shapes.count > 0 )
	{
		int blockSize = b2GetParticleBodyTaskBlockSize( world, system );
		int blockCount = ( system->particleCount + blockSize - 1 ) / blockSize;
		uint64_t scratchTicks = b2GetTicks();
		b2ParticleScratchBuffer_Reset( &system->scratch );
		b2ParticleBodyImpulseBlock* blocks =
			b2ParticleScratchBuffer_AllocArray( &system->scratch, blockCount, b2ParticleBodyImpulseBlock );
		b2ParticleShapeCandidateBlock* shapeCandidateBlocks =
			b2ParticleScratchBuffer_AllocArray( &system->scratch, blockCount, b2ParticleShapeCandidateBlock );
		memset( blocks, 0, blockCount * sizeof( b2ParticleBodyImpulseBlock ) );
		memset( shapeCandidateBlocks, 0, blockCount * sizeof( b2ParticleShapeCandidateBlock ) );
		b2AttachParticleBodyImpulsePayloads( system, blocks, blockCount );
		b2AttachParticleShapeCandidatePayloads( system, shapeCandidateBlocks, blockCount );
		system->profile.scratch += b2GetMilliseconds( scratchTicks );
		b2ParticleBodyCollisionTaskContext collisionContext = {
			.world = world,
			.system = system,
			.blocks = blocks,
			.shapeCandidateBlocks = shapeCandidateBlocks,
			.blockSize = blockSize,
			.dt = dt,
			.invDt = invDt,
			.particleMass = particleMass,
		};

		uint64_t queryTicks = b2GetTicks();
		b2RunParticleTask( world, system, b2ParticleBodyCollisionTask, blockCount, 1, &collisionContext );
		system->profile.bodyCandidateQuery += b2GetMilliseconds( queryTicks );
		for ( int blockIndex = 0; blockIndex < blockCount; ++blockIndex )
		{
			system->bodyShapeCandidateCount += blocks[blockIndex].candidateCount;
		}
		b2ApplyParticleBodyImpulseBlocks( world, blocks, blockCount );
	}
	system->profile.collision += b2GetMilliseconds( ticks );
}

static void b2LimitParticleVelocities( b2World* world, b2ParticleSystem* system, float dt )
{
	float criticalVelocity = 2.0f * system->def.radius / dt;
	float criticalVelocitySquared = criticalVelocity * criticalVelocity;
	b2ParticleRangeTaskContext context = {
		.system = system,
		.type = b2_particleTaskLimitVelocities,
		.scalar = criticalVelocitySquared,
	};
	b2RunParticleRangeTask( world, &context, system->particleCount );
}

static void b2SolveWallParticles( b2World* world, b2ParticleSystem* system )
{
	b2ParticleRangeTaskContext context = {
		.system = system,
		.type = b2_particleTaskSolveWalls,
	};
	b2RunParticleRangeTask( world, &context, system->particleCount );
}

static void b2IntegrateParticles( b2World* world, b2ParticleSystem* system, float dt )
{
	b2ParticleRangeTaskContext context = {
		.system = system,
		.type = b2_particleTaskIntegrate,
		.dt = dt,
	};
	b2RunParticleRangeTask( world, &context, system->particleCount );
}

static void b2SolveParticleLifetimes( b2World* world, b2ParticleSystem* system, float dt )
{
	if ( system->def.destroyByAge == false )
	{
		return;
	}

	uint64_t ticks = b2GetTicks();
	b2ParticleRangeTaskContext context = {
		.system = system,
		.type = b2_particleTaskSolveLifetimes,
		.dt = dt,
	};
	b2RunParticleRangeTask( world, &context, system->particleCount );
	system->profile.lifetimes += b2GetMilliseconds( ticks );
}

static void b2AccumulateParticleSystemProfile( b2World* world, const b2ParticleSystem* system )
{
	world->profile.particleLifetimes += system->profile.lifetimes;
	world->profile.particleZombie += system->profile.zombie;
	world->profile.particleProxies += system->profile.proxies;
	world->profile.particleSpatialIndexBuild += system->profile.spatialIndexBuild;
	world->profile.particleSpatialIndexScatter += system->profile.spatialIndexScatter;
	world->profile.particleContacts += system->profile.contacts;
	world->profile.particleContactCandidates += system->profile.contactCandidates;
	world->profile.particleContactMerge += system->profile.contactMerge;
	world->profile.particleBodyContacts += system->profile.bodyContacts;
	world->profile.particleBodyCandidateQuery += system->profile.bodyCandidateQuery;
	world->profile.particleWeights += system->profile.weights;
	world->profile.particleForces += system->profile.forces;
	world->profile.particlePressure += system->profile.pressure;
	world->profile.particleDamping += system->profile.damping;
	world->profile.particleReductionGenerate += system->profile.reductionGenerate;
	world->profile.particleReductionApply += system->profile.reductionApply;
	world->profile.particleCollision += system->profile.collision;
	world->profile.particleSolidDepth += system->profile.solidDepth;
	world->profile.particleSolidEjection += system->profile.solidEjection;
	world->profile.particleGroups += system->profile.groups;
	world->profile.particleGroupRefresh += system->profile.groupRefresh;
	world->profile.particleBarrier += system->profile.barrier;
	world->profile.particleCompaction += system->profile.compaction;
	world->profile.particleCompactionMark += system->profile.compactionMark;
	world->profile.particleCompactionPrefix += system->profile.compactionPrefix;
	world->profile.particleCompactionScatter += system->profile.compactionScatter;
	world->profile.particleCompactionRemap += system->profile.compactionRemap;
	world->profile.particleScratch += system->profile.scratch;
	world->profile.particleEvents += system->profile.events;
}

static void b2StepParticleSystem( b2World* world, b2ParticleSystem* system, float timeStep )
{
	if ( timeStep <= 0.0f )
	{
		return;
	}

	if ( system->id == B2_NULL_INDEX || system->particleCount == 0 )
	{
		return;
	}

	system->profile = (b2ParticleProfile){ 0 };
	b2ResetParticleStepCounters( system );
	uint64_t stepTicks = b2GetTicks();
	b2SolveParticleLifetimes( world, system, timeStep );
	if ( system->particleCount == 0 )
	{
		system->profile.step = b2GetMilliseconds( stepTicks );
		return;
	}

	if ( system->def.iterationCount < 1 )
	{
		system->def.iterationCount = 1;
	}

	float subStep = timeStep / (float)system->def.iterationCount;
	for ( int iteration = 0; iteration < system->def.iterationCount; ++iteration )
	{
		uint64_t zombieTicks = b2GetTicks();
		b2RemoveZombieParticles( system );
		system->profile.zombie += b2GetMilliseconds( zombieTicks );

		b2UpdateParticleContacts( system );
		b2UpdateReactiveParticlePairsAndTriads( system );
		b2UpdateParticleBodyContacts( world, system );
		b2ComputeParticleWeights( world, system );
		b2SolveParticleForceBuffer( world, system, subStep );
		b2SolveParticleViscous( world, system );
		b2SolveParticleRepulsive( world, system, subStep );
		b2SolveParticlePowder( world, system, subStep );
		b2SolveParticleTensile( world, system, subStep );
		b2SolveParticleSolid( world, system, subStep );
		b2SolveParticleColorMixing( system );
		b2SolveParticleGravity( world, system, subStep );
		b2SolveParticleStaticPressure( world, system, subStep );
		b2SolveParticlePressure( world, system, subStep );
		b2SolveParticleDamping( world, system, subStep );
		b2SolveParticleSpringElastic( world, system, subStep );
		b2LimitParticleVelocities( world, system, subStep );
		b2SolveBarrierParticles( system, subStep );
		b2SolveParticleBodyCollision( world, system, subStep );
		b2SolveRigidParticles( system, subStep );
		b2SolveWallParticles( world, system );
		b2IntegrateParticles( world, system, subStep );
	}

	uint64_t groupTicks = b2GetTicks();
	b2RefreshGroups( system );
	system->profile.groups += b2GetMilliseconds( groupTicks );
	system->profile.step = b2GetMilliseconds( stepTicks );
}

typedef struct b2ParticleSystemStepTaskContext
{
	b2World* world;
	float timeStep;
} b2ParticleSystemStepTaskContext;

static void b2ParticleSystemStepTask( int startIndex, int endIndex, uint32_t workerIndex, void* taskContext )
{
	B2_UNUSED( workerIndex );

	b2ParticleSystemStepTaskContext* context = taskContext;
	for ( int i = startIndex; i < endIndex; ++i )
	{
		b2StepParticleSystem( context->world, context->world->particleSystems + i, context->timeStep );
	}
}

void b2StepParticles( b2World* world, float timeStep )
{
	if ( timeStep <= 0.0f )
	{
		return;
	}

	int activeSystemCount = 0;
	for ( int i = 0; i < world->particleSystemCount; ++i )
	{
		b2ParticleSystem* system = world->particleSystems + i;
		if ( system->id != B2_NULL_INDEX && system->particleCount > 0 )
		{
			activeSystemCount += 1;
		}
	}

	if ( activeSystemCount > 1 && world->workerCount > 1 && world->shapes.count == 0 )
	{
		b2ParticleSystemStepTaskContext context = {
			.world = world,
			.timeStep = timeStep,
		};
		world->particleSystemTaskActive = true;
		void* userTask = world->enqueueTaskFcn(
			b2ParticleSystemStepTask, world->particleSystemCount, 1, &context, world->userTaskContext );
		if ( userTask != NULL )
		{
			world->taskCount += 1;
			world->activeTaskCount += 1;
			world->finishTaskFcn( userTask, world->userTaskContext );
			world->activeTaskCount -= 1;
		}
		else
		{
			b2ParticleSystemStepTask( 0, world->particleSystemCount, 0, &context );
		}
		world->particleSystemTaskActive = false;
	}
	else
	{
		for ( int i = 0; i < world->particleSystemCount; ++i )
		{
			b2StepParticleSystem( world, world->particleSystems + i, timeStep );
		}
	}

	for ( int i = 0; i < world->particleSystemCount; ++i )
	{
		b2ParticleSystem* system = world->particleSystems + i;
		if ( system->id != B2_NULL_INDEX )
		{
			b2AccumulateParticleSystemProfile( world, system );
		}
	}
}

void b2DrawParticles( b2World* world, b2DebugDraw* draw )
{
	if ( draw->DrawPointFcn == NULL )
	{
		return;
	}

	for ( int i = 0; i < world->particleSystemCount; ++i )
	{
		b2ParticleSystem* system = world->particleSystems + i;
		if ( system->id == B2_NULL_INDEX || system->def.enableParticleDebugDraw == false )
		{
			continue;
		}

		for ( int j = 0; j < system->particleCount; ++j )
		{
			b2ParticleColor c = system->colors[j];
			b2HexColor color = c.a == 0 ? b2_colorCorePhysBlue : (b2HexColor)( ( (uint32_t)c.r << 16 ) | ( (uint32_t)c.g << 8 ) | c.b );
			draw->DrawPointFcn( system->positions[j], b2MaxFloat( 2.0f, 2.0f * system->def.radius ), color, draw->context );
		}
	}
}

void b2GetParticleWorldCounters( b2World* world, b2Counters* counters )
{
	counters->particleSystemCount = b2GetIdCount( &world->particleSystemIdPool );
	for ( int i = 0; i < world->particleSystemCount; ++i )
	{
		b2ParticleSystem* system = world->particleSystems + i;
		if ( system->id == B2_NULL_INDEX )
		{
			continue;
		}

		counters->particleGroupCount += b2GetIdCount( &system->groupIdPool );
		counters->particleCount += system->particleCount;
		counters->particleContactCount += system->contactCount;
		counters->particleBodyContactCount += system->bodyContactCount;
		counters->particleTaskCount += system->taskCount;
		counters->particleTaskRangeCount += system->taskRangeCount;
		counters->particleTaskRangeMin =
			counters->particleTaskRangeMin == 0 ? system->taskRangeMin : b2MinInt( counters->particleTaskRangeMin, system->taskRangeMin );
		counters->particleTaskRangeMax = b2MaxInt( counters->particleTaskRangeMax, system->taskRangeMax );
		counters->particleSpatialCellCount += system->spatialCellCount;
		counters->particleOccupiedCellCount += system->occupiedCellCount;
		counters->particleSpatialProxyCount += system->spatialProxyCount;
		counters->particleSpatialScatterCount += system->spatialScatterCount;
		counters->particleContactCandidateCount += system->contactCandidateCount;
		counters->particleBodyShapeCandidateCount += system->bodyShapeCandidateCount;
		counters->particleBarrierCandidateCount += system->barrierCandidateCount;
		counters->particleReductionDeltaCount += system->reductionDeltaCount;
		counters->particleReductionApplyCount += system->reductionApplyCount;
		counters->particleGroupRefreshCount += system->groupRefreshCount;
		counters->particleCompactionMoveCount += system->compactionMoveCount;
		counters->particleCompactionRemapCount += system->compactionRemapCount;
		counters->particleByteCount += system->particleCapacity * (int)( 4 * sizeof( b2Vec2 ) + sizeof( uint32_t ) +
																		 sizeof( b2ParticleColor ) + sizeof( void* ) +
																		 6 * sizeof( float ) + sizeof( int ) );
		counters->particleByteCount += system->proxyCapacity * (int)sizeof( b2ParticleProxy );
		counters->particleByteCount += ( system->contactCapacity + system->previousContactCapacity + system->contactBeginCapacity +
										  system->contactEndCapacity ) *
										(int)sizeof( b2ParticleContactData );
		counters->particleByteCount += ( system->bodyContactCapacity + system->previousBodyContactCapacity +
										  system->bodyContactBeginCapacity + system->bodyContactEndCapacity ) *
										(int)sizeof( b2ParticleBodyContactData );
		counters->particleByteCount += system->destructionEventCapacity * (int)sizeof( b2ParticleDestructionEvent );
		counters->particleByteCount += system->pairCapacity * (int)sizeof( b2ParticlePairData );
		counters->particleByteCount += system->triadCapacity * (int)sizeof( b2ParticleTriadData );
		counters->particleByteCount += system->groupCapacity * (int)sizeof( b2ParticleGroup );
		counters->particleByteCount += system->scratch.byteCapacity;
		counters->particleByteCount += b2GetParticlePayloadStorageByteCount( &system->contactBlockPayloads );
		counters->particleByteCount += b2GetParticlePayloadStorageByteCount( &system->bodyContactBlockPayloads );
		counters->particleByteCount += b2GetParticlePayloadStorageByteCount( &system->bodyImpulseBlockPayloads );
		counters->particleByteCount += b2GetParticlePayloadStorageByteCount( &system->shapeCandidatePayloads );
		counters->particleByteCount += b2GetParticlePayloadStorageByteCount( &system->floatDeltaBlockPayloads );
		counters->particleByteCount += b2GetParticlePayloadStorageByteCount( &system->vec2DeltaBlockPayloads );
		counters->particleByteCount += b2GetParticlePayloadStorageByteCount( &system->barrierGroupDeltaBlockPayloads );
		counters->particleByteCount += b2GetParticlePayloadStorageByteCount( &system->eventContactSortPayloads );
		counters->particleByteCount += b2GetParticlePayloadStorageByteCount( &system->eventBodyContactSortPayloads );
		counters->particleScratchByteCount += system->scratch.byteCount;
		counters->particleScratchHighWaterByteCount += system->scratch.byteCapacity;
	}
}

float b2ParticleSystem_GetRadius( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->def.radius : 0.0f;
}

void b2ParticleSystem_SetRadius( b2ParticleSystemId systemId, float radius )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL && radius > 0.0f )
	{
		system->def.radius = radius;
	}
}

float b2ParticleSystem_GetDensity( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->def.density : 0.0f;
}

void b2ParticleSystem_SetDensity( b2ParticleSystemId systemId, float density )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL && density > 0.0f )
	{
		system->def.density = density;
	}
}

float b2ParticleSystem_GetGravityScale( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->def.gravityScale : 0.0f;
}

void b2ParticleSystem_SetGravityScale( b2ParticleSystemId systemId, float gravityScale )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL )
	{
		system->def.gravityScale = gravityScale;
	}
}

float b2ParticleSystem_GetDamping( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->def.dampingStrength : 0.0f;
}

void b2ParticleSystem_SetDamping( b2ParticleSystemId systemId, float damping )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL )
	{
		system->def.dampingStrength = b2MaxFloat( 0.0f, damping );
	}
}

int b2ParticleSystem_GetIterationCount( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->def.iterationCount : 0;
}

void b2ParticleSystem_SetIterationCount( b2ParticleSystemId systemId, int count )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL )
	{
		system->def.iterationCount = b2MaxInt( 1, count );
	}
}

void b2ParticleSystem_SetPressureStrength( b2ParticleSystemId systemId, float value )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL )
	{
		system->def.pressureStrength = b2MaxFloat( 0.0f, value );
	}
}

void b2ParticleSystem_SetViscousStrength( b2ParticleSystemId systemId, float value )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL )
	{
		system->def.viscousStrength = b2MaxFloat( 0.0f, value );
	}
}

void b2ParticleSystem_SetPowderStrength( b2ParticleSystemId systemId, float value )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL )
	{
		system->def.powderStrength = b2MaxFloat( 0.0f, value );
	}
}

void b2ParticleSystem_SetSurfaceTensionStrengths( b2ParticleSystemId systemId, float pressure, float normal )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL )
	{
		system->def.surfaceTensionPressureStrength = b2MaxFloat( 0.0f, pressure );
		system->def.surfaceTensionNormalStrength = b2MaxFloat( 0.0f, normal );
	}
}

void b2ParticleSystem_SetStaticPressureStrengths( b2ParticleSystemId systemId, float strength, float relaxation, int iterations )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL )
	{
		system->def.staticPressureStrength = b2MaxFloat( 0.0f, strength );
		system->def.staticPressureRelaxation = b2MaxFloat( 0.0f, relaxation );
		system->def.staticPressureIterations = b2MaxInt( 0, iterations );
	}
}

void b2ParticleSystem_SetColorMixingStrength( b2ParticleSystemId systemId, float value )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL )
	{
		system->def.colorMixingStrength = b2ClampFloat( value, 0.0f, 1.0f );
	}
}

void b2ParticleSystem_SetDestructionByAge( b2ParticleSystemId systemId, bool enabled )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL )
	{
		system->def.destroyByAge = enabled;
	}
}

void b2ParticleSystem_SetContactFilter( b2ParticleSystemId systemId, b2ParticleContactFilterFcn* filter, void* context )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL )
	{
		system->contactFilterFcn = filter;
		system->contactFilterContext = context;
	}
}

void b2ParticleSystem_SetBodyContactFilter( b2ParticleSystemId systemId, b2ParticleBodyContactFilterFcn* filter, void* context )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL )
	{
		system->bodyContactFilterFcn = filter;
		system->bodyContactFilterContext = context;
	}
}

void b2ParticleSystem_SetQueryFilter( b2ParticleSystemId systemId, b2ParticleQueryFilterFcn* filter, void* context )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system != NULL )
	{
		system->queryFilterFcn = filter;
		system->queryFilterContext = context;
	}
}

const b2Vec2* b2ParticleSystem_GetPositionBuffer( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->positions : NULL;
}

b2Vec2* b2ParticleSystem_GetMutablePositionBuffer( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->positions : NULL;
}

const b2Vec2* b2ParticleSystem_GetVelocityBuffer( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->velocities : NULL;
}

b2Vec2* b2ParticleSystem_GetMutableVelocityBuffer( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->velocities : NULL;
}

const uint32_t* b2ParticleSystem_GetFlagsBuffer( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->flags : NULL;
}

uint32_t* b2ParticleSystem_GetMutableFlagsBuffer( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->flags : NULL;
}

const b2ParticleColor* b2ParticleSystem_GetColorBuffer( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->colors : NULL;
}

b2ParticleColor* b2ParticleSystem_GetMutableColorBuffer( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->colors : NULL;
}

const float* b2ParticleSystem_GetWeightBuffer( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->weights : NULL;
}

const float* b2ParticleSystem_GetLifetimeBuffer( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->lifetimes : NULL;
}

float* b2ParticleSystem_GetMutableLifetimeBuffer( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->lifetimes : NULL;
}

void* const* b2ParticleSystem_GetUserDataBuffer( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->userDataBuffer : NULL;
}

void** b2ParticleSystem_GetMutableUserDataBuffer( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->userDataBuffer : NULL;
}

const b2ParticleContactData* b2ParticleSystem_GetContacts( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->contacts : NULL;
}

int b2ParticleSystem_GetContactCount( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->contactCount : 0;
}

const b2ParticleBodyContactData* b2ParticleSystem_GetBodyContacts( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->bodyContacts : NULL;
}

int b2ParticleSystem_GetBodyContactCount( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->bodyContactCount : 0;
}

const b2ParticlePairData* b2ParticleSystem_GetPairs( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->pairs : NULL;
}

int b2ParticleSystem_GetPairCount( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->pairCount : 0;
}

const b2ParticleTriadData* b2ParticleSystem_GetTriads( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->triads : NULL;
}

int b2ParticleSystem_GetTriadCount( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->triadCount : 0;
}

b2ParticleSystemCounters b2ParticleSystem_GetCounters( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system == NULL )
	{
		return (b2ParticleSystemCounters){ 0 };
	}

	b2ParticleSystemCounters counters = {
		.particleCount = system->particleCount,
		.groupCount = b2GetIdCount( &system->groupIdPool ),
		.contactCount = system->contactCount,
		.bodyContactCount = system->bodyContactCount,
		.pairCount = system->pairCount,
		.triadCount = system->triadCount,
		.destructionEventCount = system->destructionEventCount,
		.taskRangeCount = system->taskRangeCount,
		.taskRangeMin = system->taskRangeMin,
		.taskRangeMax = system->taskRangeMax,
		.spatialCellCount = system->spatialCellCount,
		.occupiedCellCount = system->occupiedCellCount,
		.spatialProxyCount = system->spatialProxyCount,
		.spatialScatterCount = system->spatialScatterCount,
		.contactCandidateCount = system->contactCandidateCount,
		.bodyShapeCandidateCount = system->bodyShapeCandidateCount,
		.barrierCandidateCount = system->barrierCandidateCount,
		.reductionDeltaCount = system->reductionDeltaCount,
		.reductionApplyCount = system->reductionApplyCount,
		.groupRefreshCount = system->groupRefreshCount,
		.compactionMoveCount = system->compactionMoveCount,
		.compactionRemapCount = system->compactionRemapCount,
		.byteCount = system->particleCapacity * (int)( 4 * sizeof( b2Vec2 ) + sizeof( uint32_t ) + sizeof( b2ParticleColor ) +
													   sizeof( void* ) + 6 * sizeof( float ) + sizeof( int ) ) +
					 system->proxyCapacity * (int)sizeof( b2ParticleProxy ) +
					 ( system->contactCapacity + system->previousContactCapacity + system->contactBeginCapacity +
					   system->contactEndCapacity ) *
						 (int)sizeof( b2ParticleContactData ) +
					 ( system->bodyContactCapacity + system->previousBodyContactCapacity + system->bodyContactBeginCapacity +
					   system->bodyContactEndCapacity ) *
						 (int)sizeof( b2ParticleBodyContactData ) +
					 system->destructionEventCapacity * (int)sizeof( b2ParticleDestructionEvent ) +
					 system->pairCapacity * (int)sizeof( b2ParticlePairData ) +
					 system->triadCapacity * (int)sizeof( b2ParticleTriadData ) +
					 system->groupCapacity * (int)sizeof( b2ParticleGroup ) + system->scratch.byteCapacity,
		.scratchByteCount = system->scratch.byteCount,
		.scratchHighWaterByteCount = system->scratch.byteCapacity,
	};
	return counters;
}

b2ParticleProfile b2ParticleSystem_GetProfile( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	return system != NULL ? system->profile : (b2ParticleProfile){ 0 };
}

b2ParticleContactEvents b2ParticleSystem_GetContactEvents( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system == NULL )
	{
		return (b2ParticleContactEvents){ 0 };
	}

	return (b2ParticleContactEvents){
		.beginEvents = system->contactBeginEvents,
		.endEvents = system->contactEndEvents,
		.beginCount = system->contactBeginCount,
		.endCount = system->contactEndCount,
	};
}

b2ParticleBodyContactEvents b2ParticleSystem_GetBodyContactEvents( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system == NULL )
	{
		return (b2ParticleBodyContactEvents){ 0 };
	}

	return (b2ParticleBodyContactEvents){
		.beginEvents = system->bodyContactBeginEvents,
		.endEvents = system->bodyContactEndEvents,
		.beginCount = system->bodyContactBeginCount,
		.endCount = system->bodyContactEndCount,
	};
}

b2ParticleDestructionEvents b2ParticleSystem_GetDestructionEvents( b2ParticleSystemId systemId )
{
	b2ParticleSystem* system = b2TryGetParticleSystem( systemId );
	if ( system == NULL )
	{
		return (b2ParticleDestructionEvents){ 0 };
	}

	return (b2ParticleDestructionEvents){
		.events = system->destructionEvents,
		.count = system->destructionEventCount,
	};
}
