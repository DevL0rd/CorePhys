// SPDX-FileCopyrightText: 2026 DevL0rd
// SPDX-License-Identifier: MIT

#pragma once

#include "corephys/corephys.h"

#include <stdint.h>

// Internal particle benchmark helpers shared by CorePhys tests, samples, and benchmark drivers.

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum ParticleParallelBenchmarkType
{
	ParticleParallelBenchmark_LargeDamBreak,
	ParticleParallelBenchmark_Faucet,
	ParticleParallelBenchmark_WaveMachine,
	ParticleParallelBenchmark_ParticleBodyCollision,
	ParticleParallelBenchmark_Powder,
	ParticleParallelBenchmark_ViscousFluid,
	ParticleParallelBenchmark_Barrier,
	ParticleParallelBenchmark_SolidGroups,
	ParticleParallelBenchmark_RigidGroups,
	ParticleParallelBenchmark_ElasticGroups,
	ParticleParallelBenchmark_SpatialIndexContacts,
	ParticleParallelBenchmark_BarrierContacts,
	ParticleParallelBenchmark_SolidGroupRefresh,
	ParticleParallelBenchmark_RigidGroupRefresh,
	ParticleParallelBenchmark_Count,
} ParticleParallelBenchmarkType;

typedef struct ParticleParallelBenchmark
{
	ParticleParallelBenchmarkType type;
	b2ParticleSystemId systemId;
	b2ParticleGroupId primaryGroupId;
	b2ParticleGroupId secondaryGroupId;
	b2ParticleGroupId tertiaryGroupId;
	b2JointId waveJointId;
	float time;
	float emitAccumulator;
	float radius;
	int emittedCount;
	int dynamicBodyCount;
} ParticleParallelBenchmark;

typedef struct ParticleParallelBenchmarkRunDef
{
	ParticleParallelBenchmarkType type;
	int warmupStepCount;
	int measuredStepCount;
	float timeStep;
	int subStepCount;
} ParticleParallelBenchmarkRunDef;

typedef struct ParticleParallelBenchmarkResult
{
	ParticleParallelBenchmarkType type;
	int workerCount;
	int warmupStepCount;
	int measuredStepCount;
	int particleCount;
	int dynamicBodyCount;
	int particleGroupCount;
	int particleContactCount;
	int particleBodyContactCount;
	int particlePairCount;
	int particleTriadCount;
	float elapsedMilliseconds;
	float stepMilliseconds;
	uint32_t hash;
	b2Counters worldCounters;
	b2ParticleSystemCounters particleCounters;
	b2Profile averageWorldProfile;
	b2ParticleProfile averageParticleProfile;
} ParticleParallelBenchmarkResult;

typedef void ParticleParallelConfigureWorldFcn( int workerCount, b2WorldDef* worldDef, void* context );
typedef void ParticleParallelFinishWorldFcn( int workerCount, void* context );

typedef struct ParticleParallelWorkerComparisonDef
{
	ParticleParallelBenchmarkRunDef runDef;
	int minWorkerCount;
	int maxWorkerCount;
	ParticleParallelConfigureWorldFcn* configureWorld;
	ParticleParallelFinishWorldFcn* finishWorld;
	void* context;
} ParticleParallelWorkerComparisonDef;

const char* GetParticleParallelBenchmarkName( ParticleParallelBenchmarkType type );
ParticleParallelBenchmarkRunDef ParticleParallelBenchmark_DefaultRunDef( ParticleParallelBenchmarkType type );
ParticleParallelBenchmark CreateParticleParallelBenchmark( b2WorldId worldId, ParticleParallelBenchmarkType type );
void StepParticleParallelBenchmark( b2WorldId worldId, ParticleParallelBenchmark* benchmark, float timeStep, int subStepCount );
uint32_t HashParticleParallelBenchmark( const ParticleParallelBenchmark* benchmark );
ParticleParallelBenchmarkResult RunParticleParallelBenchmark( b2WorldId worldId, ParticleParallelBenchmark* benchmark,
															  const ParticleParallelBenchmarkRunDef* runDef, int workerCount );
int CompareParticleParallelBenchmarkWorkers( const ParticleParallelWorkerComparisonDef* comparisonDef,
											 ParticleParallelBenchmarkResult* results, int resultCapacity );

#ifdef __cplusplus
}
#endif
