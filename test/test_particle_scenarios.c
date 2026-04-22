// SPDX-FileCopyrightText: 2026 DevL0rd
// SPDX-License-Identifier: MIT

#include "particle_scenarios.h"
#include "test_macros.h"

#include "corephys/corephys.h"
#include "corephys/particle.h"

#include <math.h>

static b2WorldId CreateScenarioWorld( void )
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){ 0.0f, -10.0f };
	return b2CreateWorld( &worldDef );
}

static int ValidateParticleBuffers( b2ParticleSystemId systemId )
{
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	int particleCount = b2ParticleSystem_GetParticleCount( systemId );
	ENSURE( particleCount > 0 );

	const b2Vec2* positions = b2ParticleSystem_GetPositionBuffer( systemId );
	const b2Vec2* velocities = b2ParticleSystem_GetVelocityBuffer( systemId );
	const uint32_t* flags = b2ParticleSystem_GetFlagsBuffer( systemId );
	const b2ParticleColor* colors = b2ParticleSystem_GetColorBuffer( systemId );
	const float* weights = b2ParticleSystem_GetWeightBuffer( systemId );
	ENSURE( positions != NULL );
	ENSURE( velocities != NULL );
	ENSURE( flags != NULL );
	ENSURE( colors != NULL );
	ENSURE( weights != NULL );

	for ( int i = 0; i < particleCount; ++i )
	{
		ENSURE( isfinite( positions[i].x ) );
		ENSURE( isfinite( positions[i].y ) );
		ENSURE( isfinite( velocities[i].x ) );
		ENSURE( isfinite( velocities[i].y ) );
		ENSURE( isfinite( weights[i] ) );
		(void)flags[i];
		(void)colors[i];
	}

	return 0;
}

static void StepScenario( b2WorldId worldId, ParticleScenario* scenario, int stepCount )
{
	float timeStep = 1.0f / 60.0f;
	for ( int i = 0; i < stepCount; ++i )
	{
		StepParticleScenario( worldId, scenario, timeStep );
		b2World_Step( worldId, timeStep, 4 );
	}
}

static int ValidateGroup( b2ParticleGroupId groupId )
{
	ENSURE( b2ParticleGroup_IsValid( groupId ) );
	ENSURE( b2ParticleGroup_GetParticleCount( groupId ) > 0 );
	ENSURE( b2ParticleGroup_GetMass( groupId ) > 0.0f );
	b2Vec2 center = b2ParticleGroup_GetCenter( groupId );
	ENSURE( isfinite( center.x ) );
	ENSURE( isfinite( center.y ) );
	ENSURE( isfinite( b2ParticleGroup_GetAngularVelocity( groupId ) ) );
	return 0;
}

static int RunParticleScenario( ParticleScenarioType type, int stepCount )
{
	b2WorldId worldId = CreateScenarioWorld();
	ParticleScenario scenario = CreateParticleScenario( worldId, type );
	ENSURE( b2ParticleSystem_IsValid( scenario.systemId ) );

	StepScenario( worldId, &scenario, stepCount );

	ENSURE( ValidateParticleBuffers( scenario.systemId ) == 0 );

	b2ParticleSystemCounters particleCounters = b2ParticleSystem_GetCounters( scenario.systemId );
	b2Counters worldCounters = b2World_GetCounters( worldId );
	ENSURE( worldCounters.particleSystemCount == 1 );
	ENSURE( worldCounters.particleCount == particleCounters.particleCount );
	ENSURE( HashParticleScenario( &scenario ) != 0 );

	switch ( type )
	{
		case ParticleScenario_DamBreak:
			ENSURE( particleCounters.groupCount == 1 );
			ENSURE( particleCounters.contactCount > 0 );
			ENSURE( particleCounters.bodyContactCount > 0 );
			ENSURE( ValidateGroup( scenario.primaryGroupId ) == 0 );
			break;

		case ParticleScenario_Faucet:
			ENSURE( particleCounters.groupCount == 0 );
			ENSURE( particleCounters.particleCount <= b2ParticleSystem_GetMaxParticleCount( scenario.systemId ) );
			ENSURE( particleCounters.bodyContactCount > 0 );
			break;

		case ParticleScenario_WaveMachine:
			ENSURE( particleCounters.groupCount == 1 );
			ENSURE( particleCounters.contactCount > 0 );
			ENSURE( particleCounters.bodyContactCount > 0 );
			ENSURE( worldCounters.jointCount == 1 );
			ENSURE( B2_IS_NON_NULL( scenario.waveJointId ) );
			ENSURE( b2Joint_IsValid( scenario.waveJointId ) );
			ENSURE( ValidateGroup( scenario.primaryGroupId ) == 0 );
			break;

		case ParticleScenario_Particles:
			ENSURE( particleCounters.groupCount == 1 );
			ENSURE( particleCounters.contactCount > 0 );
			ENSURE( particleCounters.bodyContactCount > 0 );
			ENSURE( worldCounters.bodyCount >= 2 );
			ENSURE( ValidateGroup( scenario.primaryGroupId ) == 0 );
			break;

		case ParticleScenario_RigidGroups:
			ENSURE( particleCounters.groupCount == 3 );
			ENSURE( particleCounters.contactCount > 0 );
			ENSURE( ValidateGroup( scenario.primaryGroupId ) == 0 );
			ENSURE( ValidateGroup( scenario.secondaryGroupId ) == 0 );
			ENSURE( ValidateGroup( scenario.tertiaryGroupId ) == 0 );
			break;

		case ParticleScenario_ElasticGroups:
			ENSURE( particleCounters.groupCount == 3 );
			ENSURE( particleCounters.contactCount > 0 );
			ENSURE( particleCounters.pairCount > 0 );
			ENSURE( particleCounters.triadCount > 0 );
			ENSURE( ValidateGroup( scenario.primaryGroupId ) == 0 );
			ENSURE( ValidateGroup( scenario.secondaryGroupId ) == 0 );
			ENSURE( ValidateGroup( scenario.tertiaryGroupId ) == 0 );
			break;

		case ParticleScenario_ViscousFluid:
			ENSURE( particleCounters.groupCount == 2 );
			ENSURE( particleCounters.contactCount > 0 );
			ENSURE( ValidateGroup( scenario.primaryGroupId ) == 0 );
			ENSURE( ValidateGroup( scenario.secondaryGroupId ) == 0 );
			break;

		case ParticleScenario_Powder:
			ENSURE( particleCounters.groupCount == 2 );
			ENSURE( particleCounters.contactCount > 0 );
			ENSURE( ValidateGroup( scenario.primaryGroupId ) == 0 );
			ENSURE( ValidateGroup( scenario.secondaryGroupId ) == 0 );
			break;

		case ParticleScenario_Barrier:
			ENSURE( particleCounters.groupCount == 3 );
			ENSURE( particleCounters.contactCount > 0 );
			ENSURE( particleCounters.pairCount > 0 );
			ENSURE( ValidateGroup( scenario.primaryGroupId ) == 0 );
			ENSURE( ValidateGroup( scenario.secondaryGroupId ) == 0 );
			ENSURE( ValidateGroup( scenario.tertiaryGroupId ) == 0 );
			break;

		case ParticleScenario_Count:
		default:
			ENSURE( false );
			break;
	}

	b2DestroyWorld( worldId );
	return 0;
}

static int DamBreakScenario( void )
{
	return RunParticleScenario( ParticleScenario_DamBreak, 10 );
}

static int FaucetScenario( void )
{
	return RunParticleScenario( ParticleScenario_Faucet, 90 );
}

static int WaveMachineScenario( void )
{
	return RunParticleScenario( ParticleScenario_WaveMachine, 30 );
}

static int ParticlesScenario( void )
{
	return RunParticleScenario( ParticleScenario_Particles, 30 );
}

static int RigidGroupsScenario( void )
{
	return RunParticleScenario( ParticleScenario_RigidGroups, 10 );
}

static int ElasticGroupsScenario( void )
{
	return RunParticleScenario( ParticleScenario_ElasticGroups, 10 );
}

static int ViscousFluidScenario( void )
{
	return RunParticleScenario( ParticleScenario_ViscousFluid, 10 );
}

static int PowderScenario( void )
{
	return RunParticleScenario( ParticleScenario_Powder, 10 );
}

static int BarrierScenario( void )
{
	return RunParticleScenario( ParticleScenario_Barrier, 10 );
}

int ParticleScenarioTest( void )
{
	RUN_SUBTEST( DamBreakScenario );
	RUN_SUBTEST( FaucetScenario );
	RUN_SUBTEST( WaveMachineScenario );
	RUN_SUBTEST( ParticlesScenario );
	RUN_SUBTEST( RigidGroupsScenario );
	RUN_SUBTEST( ElasticGroupsScenario );
	RUN_SUBTEST( ViscousFluidScenario );
	RUN_SUBTEST( PowderScenario );
	RUN_SUBTEST( BarrierScenario );

	return 0;
}
