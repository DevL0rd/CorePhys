// SPDX-FileCopyrightText: 2026 DevL0rd
// SPDX-License-Identifier: MIT

#pragma once

#include "corephys/id.h"
#include "corephys/types.h"

#include <stdint.h>

// LiquidFun-inspired particle validation scenarios shared by tests, samples, and benchmarks.

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum ParticleScenarioType
{
	ParticleScenario_DamBreak,
	ParticleScenario_Faucet,
	ParticleScenario_WaveMachine,
	ParticleScenario_Particles,
	ParticleScenario_RigidGroups,
	ParticleScenario_ElasticGroups,
	ParticleScenario_ViscousFluid,
	ParticleScenario_Powder,
	ParticleScenario_Barrier,
	ParticleScenario_Count,
} ParticleScenarioType;

typedef struct ParticleScenario
{
	ParticleScenarioType type;
	b2ParticleSystemId systemId;
	b2ParticleGroupId primaryGroupId;
	b2ParticleGroupId secondaryGroupId;
	b2ParticleGroupId tertiaryGroupId;
	b2JointId waveJointId;
	float time;
	float emitAccumulator;
	float radius;
	int emittedCount;
} ParticleScenario;

const char* GetParticleScenarioName( ParticleScenarioType type );
ParticleScenario CreateParticleScenario( b2WorldId worldId, ParticleScenarioType type );
void StepParticleScenario( b2WorldId worldId, ParticleScenario* scenario, float timeStep );
uint32_t HashParticleScenario( const ParticleScenario* scenario );

#ifdef __cplusplus
}
#endif
