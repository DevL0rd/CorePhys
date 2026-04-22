// SPDX-FileCopyrightText: 2026 DevL0rd
// SPDX-License-Identifier: MIT

#include "particle_scenarios.h"

#include "corephys/corephys.h"
#include "corephys/math_functions.h"
#include "corephys/particle.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

static b2ParticleColor MakeParticleColor( uint8_t r, uint8_t g, uint8_t b )
{
	return (b2ParticleColor){ r, g, b, 255 };
}

static void AddBoxShape( b2BodyId bodyId, float halfWidth, float halfHeight, b2Vec2 center, float angle, float density )
{
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = density;
	b2Polygon box = b2MakeOffsetBox( halfWidth, halfHeight, center, b2MakeRot( angle ) );
	b2CreatePolygonShape( bodyId, &shapeDef, &box );
}

static void AddPolygonShape( b2BodyId bodyId, const b2Vec2* points, int count, float density )
{
	b2Hull hull = b2ComputeHull( points, count );
	if ( hull.count == 0 )
	{
		return;
	}

	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = density;
	b2Polygon polygon = b2MakePolygon( &hull, 0.0f );
	b2CreatePolygonShape( bodyId, &shapeDef, &polygon );
}

static b2BodyId CreateStaticBody( b2WorldId worldId )
{
	b2BodyDef bodyDef = b2DefaultBodyDef();
	return b2CreateBody( worldId, &bodyDef );
}

static void CreateClosedBox( b2WorldId worldId, float halfWidth, float height, float thickness )
{
	b2BodyId groundId = CreateStaticBody( worldId );
	AddBoxShape( groundId, halfWidth + thickness, thickness, (b2Vec2){ 0.0f, -thickness }, 0.0f, 0.0f );
	AddBoxShape( groundId, thickness, 0.5f * height + thickness, (b2Vec2){ -halfWidth - thickness, 0.5f * height }, 0.0f, 0.0f );
	AddBoxShape( groundId, thickness, 0.5f * height + thickness, (b2Vec2){ halfWidth + thickness, 0.5f * height }, 0.0f, 0.0f );
	AddBoxShape( groundId, halfWidth + thickness, thickness, (b2Vec2){ 0.0f, height + thickness }, 0.0f, 0.0f );
}

static void CreateFaucetStaticGeometry( b2WorldId worldId, float radius )
{
	const float containerHeight = 0.2f;
	const float containerWidth = 1.0f;
	const float containerThickness = 0.05f;
	const float faucetWidth = 0.1f;
	const float faucetHeight = 15.0f;
	const float faucetLengthFactor = 2.0f;
	const float spoutLength = 2.0f;
	const float spoutWidth = 1.1f;
	b2BodyId groundId = CreateStaticBody( worldId );

	const float height = containerHeight + containerThickness;
	AddBoxShape( groundId, containerWidth - containerThickness, containerThickness, (b2Vec2){ 0.0f, 0.0f }, 0.0f, 0.0f );
	AddBoxShape( groundId, containerThickness, height, (b2Vec2){ -containerWidth, containerHeight }, 0.0f, 0.0f );
	AddBoxShape( groundId, containerThickness, height, (b2Vec2){ containerWidth, containerHeight }, 0.0f, 0.0f );
	AddBoxShape( groundId, containerWidth * 5.0f, containerThickness, (b2Vec2){ 0.0f, -2.0f * containerThickness }, 0.0f, 0.0f );

	const float particleDiameter = 2.0f * radius;
	const float faucetLength = faucetLengthFactor * particleDiameter;
	const float length = faucetLength * spoutLength;
	const float width = containerWidth * faucetWidth * spoutWidth;
	const float spoutHeight = containerHeight * faucetHeight + 0.5f * length;

	AddBoxShape( groundId, particleDiameter, length, (b2Vec2){ -width, spoutHeight }, 0.0f, 0.0f );
	AddBoxShape( groundId, particleDiameter, length, (b2Vec2){ width, spoutHeight }, 0.0f, 0.0f );
	AddBoxShape( groundId, width - particleDiameter, particleDiameter,
				  (b2Vec2){ 0.0f, spoutHeight + length - particleDiameter }, 0.0f, 0.0f );
}

static void CreateLiquidFunParticleBasin( b2WorldId worldId )
{
	b2BodyId groundId = CreateStaticBody( worldId );

	const b2Vec2 floorPoints[4] = {
		{ -4.0f, -1.0f },
		{ 4.0f, -1.0f },
		{ 4.0f, 0.0f },
		{ -4.0f, 0.0f },
	};
	AddPolygonShape( groundId, floorPoints, 4, 0.0f );

	const b2Vec2 leftPoints[4] = {
		{ -4.0f, -0.1f },
		{ -2.0f, -0.1f },
		{ -2.0f, 2.0f },
		{ -4.0f, 3.0f },
	};
	AddPolygonShape( groundId, leftPoints, 4, 0.0f );

	const b2Vec2 rightPoints[4] = {
		{ 2.0f, -0.1f },
		{ 4.0f, -0.1f },
		{ 4.0f, 3.0f },
		{ 2.0f, 2.0f },
	};
	AddPolygonShape( groundId, rightPoints, 4, 0.0f );
}

static void CreateStraightParticleBasin( b2WorldId worldId )
{
	b2BodyId groundId = CreateStaticBody( worldId );

	AddBoxShape( groundId, 4.0f, 0.5f, (b2Vec2){ 0.0f, -0.5f }, 0.0f, 0.0f );
	AddBoxShape( groundId, 0.5f, 1.05f, (b2Vec2){ -3.0f, 1.0f }, 0.0f, 0.0f );
	AddBoxShape( groundId, 0.5f, 1.05f, (b2Vec2){ 3.0f, 1.0f }, 0.0f, 0.0f );
}

static void CreateDynamicCircle( b2WorldId worldId, b2Vec2 position, float radius, float density )
{
	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = position;
	b2BodyId bodyId = b2CreateBody( worldId, &bodyDef );

	b2Circle circle = { b2Vec2_zero, radius };
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = density;
	b2CreateCircleShape( bodyId, &shapeDef, &circle );
}

static b2ParticleSystemId CreateLiquidFunParticleSystem( b2WorldId worldId, float radius, float damping, int maxCount )
{
	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = radius;
	systemDef.dampingStrength = damping;
	systemDef.maxCount = maxCount;
	return b2CreateParticleSystem( worldId, &systemDef );
}

static b2ParticleGroupId CreateBoxGroup( b2ParticleSystemId systemId, b2Vec2 center, float halfWidth, float halfHeight,
										 uint32_t flags, uint32_t groupFlags, b2ParticleColor color )
{
	b2Polygon box = b2MakeOffsetBox( halfWidth, halfHeight, center, b2Rot_identity );
	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	groupDef.flags = flags;
	groupDef.groupFlags = groupFlags;
	groupDef.color = color;
	groupDef.polygon = &box;
	return b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
}

static b2ParticleGroupId CreateCircleGroup( b2ParticleSystemId systemId, b2Vec2 center, float radius, uint32_t flags,
											uint32_t groupFlags, b2ParticleColor color )
{
	b2Circle circle = { center, radius };
	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	groupDef.flags = flags;
	groupDef.groupFlags = groupFlags;
	groupDef.color = color;
	groupDef.circle = &circle;
	return b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
}

static b2ParticleGroupId CreateRotatedBoxGroup( b2ParticleSystemId systemId, b2Vec2 position, float halfWidth, float halfHeight,
												float angle, float angularVelocity, uint32_t flags, uint32_t groupFlags,
												b2ParticleColor color )
{
	b2Polygon box = b2MakeBox( halfWidth, halfHeight );
	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	groupDef.flags = flags;
	groupDef.groupFlags = groupFlags;
	groupDef.position = position;
	groupDef.rotation = b2MakeRot( angle );
	groupDef.angularVelocity = angularVelocity;
	groupDef.color = color;
	groupDef.polygon = &box;
	return b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
}

static b2ParticleGroupId CreateSegmentGroup( b2ParticleSystemId systemId, b2Vec2 point1, b2Vec2 point2, uint32_t flags,
											 b2ParticleColor color )
{
	b2Segment segment = { point1, point2 };
	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	groupDef.flags = flags;
	groupDef.color = color;
	groupDef.segment = &segment;
	return b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
}

static ParticleScenario CreateDamBreak( b2WorldId worldId )
{
	CreateClosedBox( worldId, 2.0f, 4.0f, 0.05f );

	float radius = 0.025f;
	ParticleScenario scenario = { .type = ParticleScenario_DamBreak, .radius = radius };
	scenario.systemId = CreateLiquidFunParticleSystem( worldId, radius, 0.2f, 0 );
	scenario.primaryGroupId = CreateBoxGroup( scenario.systemId, (b2Vec2){ -1.2f, 1.01f }, 0.8f, 1.0f, b2_waterParticle, 0,
											  MakeParticleColor( 64, 154, 255 ) );
	return scenario;
}

static ParticleScenario CreateFaucet( b2WorldId worldId )
{
	float radius = 0.035f;
	CreateFaucetStaticGeometry( worldId, radius );

	ParticleScenario scenario = { .type = ParticleScenario_Faucet, .radius = radius };
	scenario.systemId = CreateLiquidFunParticleSystem( worldId, radius, 1.0f, 1000 );
	b2ParticleSystem_SetDestructionByAge( scenario.systemId, true );
	return scenario;
}

static ParticleScenario CreateWaveMachine( b2WorldId worldId )
{
	b2BodyId groundId = CreateStaticBody( worldId );

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){ 0.0f, 1.0f };
	bodyDef.enableSleep = false;
	b2BodyId bodyId = b2CreateBody( worldId, &bodyDef );

	AddBoxShape( bodyId, 0.05f, 1.0f, (b2Vec2){ 2.0f, 0.0f }, 0.0f, 5.0f );
	AddBoxShape( bodyId, 0.05f, 1.0f, (b2Vec2){ -2.0f, 0.0f }, 0.0f, 5.0f );
	AddBoxShape( bodyId, 2.0f, 0.05f, (b2Vec2){ 0.0f, 1.0f }, 0.0f, 5.0f );
	AddBoxShape( bodyId, 2.0f, 0.05f, (b2Vec2){ 0.0f, -1.0f }, 0.0f, 5.0f );

	b2RevoluteJointDef jointDef = b2DefaultRevoluteJointDef();
	jointDef.base.bodyIdA = groundId;
	jointDef.base.bodyIdB = bodyId;
	jointDef.base.localFrameA.p = (b2Vec2){ 0.0f, 1.0f };
	jointDef.base.localFrameA.q = b2Rot_identity;
	jointDef.base.localFrameB.p = b2Vec2_zero;
	jointDef.base.localFrameB.q = b2Rot_identity;
	jointDef.motorSpeed = 0.05f * B2_PI;
	jointDef.maxMotorTorque = 1e7f;
	jointDef.enableMotor = true;

	float radius = 0.025f;
	ParticleScenario scenario = { .type = ParticleScenario_WaveMachine, .radius = radius };
	scenario.waveJointId = b2CreateRevoluteJoint( worldId, &jointDef );
	scenario.systemId = CreateLiquidFunParticleSystem( worldId, radius, 0.2f, 0 );
	scenario.primaryGroupId = CreateBoxGroup( scenario.systemId, (b2Vec2){ 0.0f, 1.0f }, 0.9f, 0.9f, b2_waterParticle, 0,
											  MakeParticleColor( 64, 154, 255 ) );
	return scenario;
}

static ParticleScenario CreateParticles( b2WorldId worldId )
{
	CreateLiquidFunParticleBasin( worldId );
	CreateDynamicCircle( worldId, (b2Vec2){ 0.0f, 8.0f }, 0.5f, 0.5f );

	float radius = 0.035f;
	ParticleScenario scenario = { .type = ParticleScenario_Particles, .radius = radius };
	scenario.systemId = CreateLiquidFunParticleSystem( worldId, radius, 0.2f, 0 );
	scenario.primaryGroupId = CreateCircleGroup( scenario.systemId, (b2Vec2){ 0.0f, 3.0f }, 2.0f, b2_waterParticle, 0,
												 MakeParticleColor( 64, 154, 255 ) );
	return scenario;
}

static ParticleScenario CreateRigidGroups( b2WorldId worldId )
{
	CreateStraightParticleBasin( worldId );
	CreateDynamicCircle( worldId, (b2Vec2){ 0.0f, 8.0f }, 0.5f, 0.5f );

	float radius = 0.035f;
	ParticleScenario scenario = { .type = ParticleScenario_RigidGroups, .radius = radius };
	scenario.systemId = CreateLiquidFunParticleSystem( worldId, radius, 0.2f, 0 );
	uint32_t groupFlags = b2_rigidParticleGroup | b2_solidParticleGroup;
	scenario.primaryGroupId = CreateCircleGroup( scenario.systemId, (b2Vec2){ 0.0f, 3.0f }, 0.5f, b2_waterParticle, groupFlags,
												 MakeParticleColor( 255, 0, 0 ) );
	scenario.secondaryGroupId =
		CreateCircleGroup( scenario.systemId, (b2Vec2){ -1.0f, 3.0f }, 0.5f, b2_waterParticle, groupFlags, MakeParticleColor( 0, 255, 0 ) );
	scenario.tertiaryGroupId = CreateRotatedBoxGroup( scenario.systemId, (b2Vec2){ 1.0f, 4.0f }, 1.0f, 0.5f, -0.5f, 2.0f,
													  b2_waterParticle, groupFlags, MakeParticleColor( 0, 0, 255 ) );
	return scenario;
}

static ParticleScenario CreateElasticGroups( b2WorldId worldId )
{
	CreateStraightParticleBasin( worldId );
	CreateDynamicCircle( worldId, (b2Vec2){ 0.0f, 8.0f }, 0.5f, 0.5f );

	float radius = 0.035f;
	ParticleScenario scenario = { .type = ParticleScenario_ElasticGroups, .radius = radius };
	scenario.systemId = CreateLiquidFunParticleSystem( worldId, radius, 0.2f, 0 );
	scenario.primaryGroupId =
		CreateCircleGroup( scenario.systemId, (b2Vec2){ 0.0f, 3.0f }, 0.5f, b2_springParticle, b2_solidParticleGroup,
						   MakeParticleColor( 255, 0, 0 ) );
	scenario.secondaryGroupId =
		CreateCircleGroup( scenario.systemId, (b2Vec2){ -1.0f, 3.0f }, 0.5f, b2_elasticParticle, b2_solidParticleGroup,
						   MakeParticleColor( 0, 255, 0 ) );
	scenario.tertiaryGroupId = CreateRotatedBoxGroup( scenario.systemId, (b2Vec2){ 1.0f, 4.0f }, 1.0f, 0.5f, -0.5f, 2.0f,
													  b2_elasticParticle, b2_solidParticleGroup, MakeParticleColor( 0, 0, 255 ) );
	return scenario;
}

static ParticleScenario CreateViscousFluid( b2WorldId worldId )
{
	CreateStraightParticleBasin( worldId );

	float radius = 0.035f;
	ParticleScenario scenario = { .type = ParticleScenario_ViscousFluid, .radius = radius };
	scenario.systemId = CreateLiquidFunParticleSystem( worldId, radius, 0.2f, 0 );
	b2ParticleSystem_SetViscousStrength( scenario.systemId, 0.5f );
	scenario.primaryGroupId = CreateCircleGroup( scenario.systemId, (b2Vec2){ -0.5f, 2.0f }, 0.75f, b2_viscousParticle, 0,
												 MakeParticleColor( 88, 199, 148 ) );
	scenario.secondaryGroupId = CreateCircleGroup( scenario.systemId, (b2Vec2){ 0.75f, 2.3f }, 0.55f, b2_viscousParticle, 0,
												   MakeParticleColor( 48, 145, 108 ) );
	return scenario;
}

static ParticleScenario CreatePowder( b2WorldId worldId )
{
	CreateStraightParticleBasin( worldId );

	float radius = 0.035f;
	ParticleScenario scenario = { .type = ParticleScenario_Powder, .radius = radius };
	scenario.systemId = CreateLiquidFunParticleSystem( worldId, radius, 0.2f, 0 );
	b2ParticleSystem_SetPowderStrength( scenario.systemId, 0.8f );
	scenario.primaryGroupId = CreateBoxGroup( scenario.systemId, (b2Vec2){ -0.4f, 2.0f }, 0.65f, 0.65f, b2_powderParticle, 0,
											  MakeParticleColor( 220, 185, 84 ) );
	scenario.secondaryGroupId = CreateBoxGroup( scenario.systemId, (b2Vec2){ 0.8f, 2.7f }, 0.45f, 0.45f, b2_powderParticle, 0,
												MakeParticleColor( 185, 140, 55 ) );
	return scenario;
}

static ParticleScenario CreateBarrier( b2WorldId worldId )
{
	CreateClosedBox( worldId, 2.0f, 3.0f, 0.05f );

	float radius = 0.035f;
	ParticleScenario scenario = { .type = ParticleScenario_Barrier, .radius = radius };
	scenario.systemId = CreateLiquidFunParticleSystem( worldId, radius, 0.2f, 0 );
	scenario.primaryGroupId = CreateBoxGroup( scenario.systemId, (b2Vec2){ -0.9f, 1.2f }, 0.75f, 0.75f, b2_powderParticle, 0,
											  MakeParticleColor( 220, 185, 84 ) );
	scenario.secondaryGroupId = CreateBoxGroup( scenario.systemId, (b2Vec2){ 0.9f, 1.2f }, 0.75f, 0.75f, b2_waterParticle, 0,
												MakeParticleColor( 64, 154, 255 ) );
	scenario.tertiaryGroupId = CreateSegmentGroup( scenario.systemId, (b2Vec2){ 0.0f, 0.3f }, (b2Vec2){ 0.0f, 2.7f },
												   b2_wallParticle | b2_barrierParticle, MakeParticleColor( 255, 255, 255 ) );
	return scenario;
}

const char* GetParticleScenarioName( ParticleScenarioType type )
{
	static const char* names[] = {
		"Dam Break",
		"Faucet",
		"Wave Machine",
		"Particles",
		"Rigid Groups",
		"Elastic Groups",
		"Viscous Fluid",
		"Powder",
		"Barrier",
	};

	if ( type < 0 || ParticleScenario_Count <= type )
	{
		return "Unknown";
	}

	return names[type];
}

ParticleScenario CreateParticleScenario( b2WorldId worldId, ParticleScenarioType type )
{
	b2World_EnableSleeping( worldId, false );

	switch ( type )
	{
		case ParticleScenario_DamBreak:
			return CreateDamBreak( worldId );

		case ParticleScenario_Faucet:
			return CreateFaucet( worldId );

		case ParticleScenario_WaveMachine:
			return CreateWaveMachine( worldId );

		case ParticleScenario_Particles:
			return CreateParticles( worldId );

		case ParticleScenario_RigidGroups:
			return CreateRigidGroups( worldId );

		case ParticleScenario_ElasticGroups:
			return CreateElasticGroups( worldId );

		case ParticleScenario_ViscousFluid:
			return CreateViscousFluid( worldId );

		case ParticleScenario_Powder:
			return CreatePowder( worldId );

		case ParticleScenario_Barrier:
			return CreateBarrier( worldId );

		case ParticleScenario_Count:
		default:
			break;
	}

	return (ParticleScenario){ .type = type };
}

static void EmitFaucetParticles( ParticleScenario* scenario, float timeStep )
{
	const float containerHeight = 0.2f;
	const float containerWidth = 1.0f;
	const float faucetWidth = 0.1f;
	const float faucetHeight = 15.0f;
	const float faucetLength = 2.0f * 2.0f * scenario->radius;
	const b2Vec2 emitterPosition = { containerWidth * faucetWidth, containerHeight * faucetHeight + 0.5f * faucetLength };
	const float emitRate = 120.0f;

	scenario->emitAccumulator += timeStep * emitRate;
	int emitCount = (int)scenario->emitAccumulator;
	scenario->emitAccumulator -= (float)emitCount;

	static const b2ParticleColor colors[] = {
		{ 255, 255, 255, 255 },
		{ 255, 90, 90, 255 },
		{ 255, 220, 90, 255 },
		{ 90, 180, 255, 255 },
		{ 120, 255, 180, 255 },
	};

	for ( int i = 0; i < emitCount; ++i )
	{
		int index = scenario->emittedCount++;
		float phase = (float)( ( index * 37 ) % 97 ) / 96.0f;
		float lifetimePhase = (float)( ( index * 53 ) % 101 ) / 100.0f;

		b2ParticleDef particleDef = b2DefaultParticleDef();
		particleDef.flags = b2_waterParticle;
		particleDef.position = (b2Vec2){ emitterPosition.x, emitterPosition.y + ( phase - 0.5f ) * faucetLength };
		particleDef.velocity = b2Vec2_zero;
		particleDef.color = colors[index % (int)( sizeof( colors ) / sizeof( colors[0] ) )];
		particleDef.lifetime = 30.0f + 20.0f * lifetimePhase;

		b2ParticleSystem_CreateParticle( scenario->systemId, &particleDef );
	}
}

void StepParticleScenario( b2WorldId worldId, ParticleScenario* scenario, float timeStep )
{
	(void)worldId;

	if ( scenario == NULL || timeStep <= 0.0f )
	{
		return;
	}

	if ( scenario->type == ParticleScenario_Faucet )
	{
		EmitFaucetParticles( scenario, timeStep );
	}
	else if ( scenario->type == ParticleScenario_WaveMachine && B2_IS_NON_NULL( scenario->waveJointId ) &&
			  b2Joint_IsValid( scenario->waveJointId ) )
	{
		scenario->time += timeStep;
		b2RevoluteJoint_SetMotorSpeed( scenario->waveJointId, 0.05f * cosf( scenario->time ) * B2_PI );
	}
}

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

uint32_t HashParticleScenario( const ParticleScenario* scenario )
{
	if ( scenario == NULL || b2ParticleSystem_IsValid( scenario->systemId ) == false )
	{
		return 0;
	}

	uint32_t hash = 2166136261u;
	b2ParticleSystemCounters counters = b2ParticleSystem_GetCounters( scenario->systemId );
	hash = HashWord( hash, (uint32_t)scenario->type );
	hash = HashWord( hash, (uint32_t)counters.particleCount );
	hash = HashWord( hash, (uint32_t)counters.groupCount );
	hash = HashWord( hash, (uint32_t)counters.contactCount );
	hash = HashWord( hash, (uint32_t)counters.bodyContactCount );
	hash = HashWord( hash, (uint32_t)counters.pairCount );
	hash = HashWord( hash, (uint32_t)counters.triadCount );

	const b2Vec2* positions = b2ParticleSystem_GetPositionBuffer( scenario->systemId );
	const b2Vec2* velocities = b2ParticleSystem_GetVelocityBuffer( scenario->systemId );
	const uint32_t* flags = b2ParticleSystem_GetFlagsBuffer( scenario->systemId );
	const b2ParticleColor* colors = b2ParticleSystem_GetColorBuffer( scenario->systemId );
	for ( int i = 0; i < counters.particleCount; ++i )
	{
		hash = HashVec2( hash, positions[i] );
		hash = HashVec2( hash, velocities[i] );
		hash = HashWord( hash, flags[i] );
		hash = HashWord( hash, (uint32_t)colors[i].r | ( (uint32_t)colors[i].g << 8 ) | ( (uint32_t)colors[i].b << 16 ) |
								 ( (uint32_t)colors[i].a << 24 ) );
	}

	return hash;
}
