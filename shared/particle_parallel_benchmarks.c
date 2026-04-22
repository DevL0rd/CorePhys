// SPDX-FileCopyrightText: 2026 DevL0rd
// SPDX-License-Identifier: MIT

#include "particle_parallel_benchmarks.h"

#include "corephys/math_functions.h"
#include "corephys/particle.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static b2ParticleColor MakeParticleColor( uint8_t r, uint8_t g, uint8_t b )
{
	return (b2ParticleColor){ r, g, b, 255 };
}

static b2BodyId CreateStaticBody( b2WorldId worldId )
{
	b2BodyDef bodyDef = b2DefaultBodyDef();
	return b2CreateBody( worldId, &bodyDef );
}

static void AddBoxShape( b2BodyId bodyId, float halfWidth, float halfHeight, b2Vec2 center, float angle, float density )
{
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = density;
	b2Polygon box = b2MakeOffsetBox( halfWidth, halfHeight, center, b2MakeRot( angle ) );
	b2CreatePolygonShape( bodyId, &shapeDef, &box );
}

static void CreateClosedBox( b2WorldId worldId, float halfWidth, float height, float thickness )
{
	b2BodyId groundId = CreateStaticBody( worldId );
	AddBoxShape( groundId, halfWidth + thickness, thickness, (b2Vec2){ 0.0f, -thickness }, 0.0f, 0.0f );
	AddBoxShape( groundId, thickness, 0.5f * height + thickness, (b2Vec2){ -halfWidth - thickness, 0.5f * height },
				  0.0f, 0.0f );
	AddBoxShape( groundId, thickness, 0.5f * height + thickness, (b2Vec2){ halfWidth + thickness, 0.5f * height },
				  0.0f, 0.0f );
	AddBoxShape( groundId, halfWidth + thickness, thickness, (b2Vec2){ 0.0f, height + thickness }, 0.0f, 0.0f );
}

static void CreateOpenBasin( b2WorldId worldId, float halfWidth, float wallHeight, float thickness )
{
	b2BodyId groundId = CreateStaticBody( worldId );
	AddBoxShape( groundId, halfWidth + thickness, thickness, (b2Vec2){ 0.0f, -thickness }, 0.0f, 0.0f );
	AddBoxShape( groundId, thickness, 0.5f * wallHeight + thickness,
				  (b2Vec2){ -halfWidth - thickness, 0.5f * wallHeight }, 0.0f, 0.0f );
	AddBoxShape( groundId, thickness, 0.5f * wallHeight + thickness,
				  (b2Vec2){ halfWidth + thickness, 0.5f * wallHeight }, 0.0f, 0.0f );
}

static b2BodyId CreateDynamicCircleWithVelocity( b2WorldId worldId, b2Vec2 position, float radius, float density,
												 b2Vec2 linearVelocity, float angularVelocity )
{
	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = position;
	bodyDef.linearVelocity = linearVelocity;
	bodyDef.angularVelocity = angularVelocity;
	bodyDef.enableSleep = false;
	b2BodyId bodyId = b2CreateBody( worldId, &bodyDef );

	b2Circle circle = { b2Vec2_zero, radius };
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = density;
	b2CreateCircleShape( bodyId, &shapeDef, &circle );
	return bodyId;
}

static b2BodyId CreateDynamicCircle( b2WorldId worldId, b2Vec2 position, float radius, float density )
{
	return CreateDynamicCircleWithVelocity( worldId, position, radius, density, b2Vec2_zero, 0.0f );
}

static b2BodyId CreateDynamicBoxWithVelocity( b2WorldId worldId, b2Vec2 position, b2Vec2 halfSize, float angle, float density,
											  b2Vec2 linearVelocity, float angularVelocity )
{
	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = position;
	bodyDef.rotation = b2MakeRot( angle );
	bodyDef.linearVelocity = linearVelocity;
	bodyDef.angularVelocity = angularVelocity;
	bodyDef.enableSleep = false;
	b2BodyId bodyId = b2CreateBody( worldId, &bodyDef );

	AddBoxShape( bodyId, halfSize.x, halfSize.y, b2Vec2_zero, 0.0f, density );
	return bodyId;
}

static b2ParticleSystemId CreateBenchmarkParticleSystem( b2WorldId worldId, float radius, float damping, int maxCount )
{
	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = radius;
	systemDef.dampingStrength = damping;
	systemDef.maxCount = maxCount;
	systemDef.enableParticleEvents = true;
	systemDef.enableParticleBodyEvents = true;
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

static ParticleParallelBenchmark CreateLargeDamBreak( b2WorldId worldId )
{
	CreateClosedBox( worldId, 6.0f, 8.0f, 0.08f );
	CreateDynamicCircle( worldId, (b2Vec2){ 1.2f, 6.0f }, 0.6f, 0.7f );
	CreateDynamicCircle( worldId, (b2Vec2){ 3.0f, 6.8f }, 0.45f, 0.7f );

	const float radius = 0.03f;
	ParticleParallelBenchmark benchmark = { .type = ParticleParallelBenchmark_LargeDamBreak, .radius = radius };
	benchmark.systemId = CreateBenchmarkParticleSystem( worldId, radius, 0.25f, 24000 );
	benchmark.primaryGroupId = CreateBoxGroup( benchmark.systemId, (b2Vec2){ -2.6f, 3.35f }, 2.6f, 3.25f,
											   b2_waterParticle | b2_colorMixingParticle, 0, MakeParticleColor( 64, 154, 255 ) );
	benchmark.secondaryGroupId = CreateBoxGroup( benchmark.systemId, (b2Vec2){ -4.7f, 6.7f }, 0.7f, 0.7f,
												 b2_waterParticle | b2_colorMixingParticle, 0, MakeParticleColor( 255, 90, 90 ) );
	b2ParticleSystem_SetColorMixingStrength( benchmark.systemId, 0.5f );
	return benchmark;
}

static void CreateFaucetGeometry( b2WorldId worldId, float radius )
{
	const float basinHalfWidth = 3.0f;
	const float wallHeight = 2.0f;
	const float thickness = 0.08f;
	CreateOpenBasin( worldId, basinHalfWidth, wallHeight, thickness );

	b2BodyId groundId = CreateStaticBody( worldId );
	const float particleDiameter = 2.0f * radius;
	const float spoutHeight = 4.5f;
	const float spoutLength = 0.7f;
	const float spoutHalfWidth = 0.22f;
	AddBoxShape( groundId, particleDiameter, spoutLength, (b2Vec2){ -spoutHalfWidth, spoutHeight }, 0.0f, 0.0f );
	AddBoxShape( groundId, particleDiameter, spoutLength, (b2Vec2){ spoutHalfWidth, spoutHeight }, 0.0f, 0.0f );
	AddBoxShape( groundId, spoutHalfWidth - particleDiameter, particleDiameter,
				  (b2Vec2){ 0.0f, spoutHeight + spoutLength - particleDiameter }, 0.0f, 0.0f );
}

static ParticleParallelBenchmark CreateFaucet( b2WorldId worldId )
{
	const float radius = 0.025f;
	CreateFaucetGeometry( worldId, radius );

	ParticleParallelBenchmark benchmark = { .type = ParticleParallelBenchmark_Faucet, .radius = radius };
	benchmark.systemId = CreateBenchmarkParticleSystem( worldId, radius, 0.75f, 8000 );
	benchmark.primaryGroupId = CreateBoxGroup( benchmark.systemId, (b2Vec2){ 0.0f, 0.85f }, 2.2f, 0.35f,
											   b2_waterParticle | b2_colorMixingParticle, 0, MakeParticleColor( 90, 180, 255 ) );
	b2ParticleSystem_SetDestructionByAge( benchmark.systemId, true );
	return benchmark;
}

static ParticleParallelBenchmark CreateWaveMachine( b2WorldId worldId )
{
	b2BodyId groundId = CreateStaticBody( worldId );

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){ 0.0f, 2.6f };
	bodyDef.enableSleep = false;
	b2BodyId bodyId = b2CreateBody( worldId, &bodyDef );

	AddBoxShape( bodyId, 0.08f, 2.3f, (b2Vec2){ -4.0f, 0.0f }, 0.0f, 4.0f );
	AddBoxShape( bodyId, 0.08f, 2.3f, (b2Vec2){ 4.0f, 0.0f }, 0.0f, 4.0f );
	AddBoxShape( bodyId, 4.0f, 0.08f, (b2Vec2){ 0.0f, -2.3f }, 0.0f, 4.0f );
	AddBoxShape( bodyId, 4.0f, 0.08f, (b2Vec2){ 0.0f, 2.3f }, 0.0f, 4.0f );

	b2RevoluteJointDef jointDef = b2DefaultRevoluteJointDef();
	jointDef.base.bodyIdA = groundId;
	jointDef.base.bodyIdB = bodyId;
	jointDef.base.localFrameA.p = (b2Vec2){ 0.0f, 2.6f };
	jointDef.base.localFrameA.q = b2Rot_identity;
	jointDef.base.localFrameB.p = b2Vec2_zero;
	jointDef.base.localFrameB.q = b2Rot_identity;
	jointDef.motorSpeed = 0.05f * B2_PI;
	jointDef.maxMotorTorque = 1e7f;
	jointDef.enableMotor = true;

	const float radius = 0.03f;
	ParticleParallelBenchmark benchmark = { .type = ParticleParallelBenchmark_WaveMachine, .radius = radius };
	benchmark.waveJointId = b2CreateRevoluteJoint( worldId, &jointDef );
	benchmark.systemId = CreateBenchmarkParticleSystem( worldId, radius, 0.2f, 24000 );
	benchmark.primaryGroupId = CreateBoxGroup( benchmark.systemId, (b2Vec2){ 0.0f, 2.5f }, 2.9f, 1.75f,
											   b2_waterParticle | b2_colorMixingParticle, 0, MakeParticleColor( 64, 154, 255 ) );
	b2ParticleSystem_SetColorMixingStrength( benchmark.systemId, 0.35f );
	return benchmark;
}

static ParticleParallelBenchmark CreateParticleBodyCollision( b2WorldId worldId )
{
	CreateClosedBox( worldId, 5.0f, 6.0f, 0.08f );

	const float radius = 0.03f;
	ParticleParallelBenchmark benchmark = { .type = ParticleParallelBenchmark_ParticleBodyCollision, .radius = radius };
	benchmark.systemId = CreateBenchmarkParticleSystem( worldId, radius, 0.25f, 26000 );
	benchmark.primaryGroupId = CreateBoxGroup( benchmark.systemId, (b2Vec2){ 0.0f, 2.25f }, 4.4f, 2.05f,
											   b2_waterParticle | b2_colorMixingParticle, 0, MakeParticleColor( 64, 154, 255 ) );
	b2ParticleSystem_SetColorMixingStrength( benchmark.systemId, 0.35f );

	int dynamicBodyCount = 0;
	for ( int row = 0; row < 7; ++row )
	{
		for ( int column = 0; column < 12; ++column )
		{
			float x = -4.0f + 0.72f * (float)column;
			float y = 1.15f + 0.55f * (float)row;
			float lateral = ( ( column & 1 ) == 0 ? 1.0f : -1.0f ) * ( 0.25f + 0.03f * (float)row );
			float angularVelocity = ( ( row + column ) & 1 ) == 0 ? 1.5f : -1.5f;

			if ( ( row + column ) % 3 == 0 )
			{
				CreateDynamicBoxWithVelocity( worldId, (b2Vec2){ x, y + 0.08f }, (b2Vec2){ 0.18f, 0.12f }, 0.25f,
											  0.75f, (b2Vec2){ lateral, -0.15f }, angularVelocity );
			}
			else
			{
				CreateDynamicCircleWithVelocity( worldId, (b2Vec2){ x, y }, 0.16f, 0.75f, (b2Vec2){ lateral, -0.1f },
												 angularVelocity );
			}

			++dynamicBodyCount;
		}
	}

	benchmark.dynamicBodyCount = dynamicBodyCount;
	return benchmark;
}

static ParticleParallelBenchmark CreatePowder( b2WorldId worldId )
{
	CreateOpenBasin( worldId, 4.2f, 4.0f, 0.08f );

	const float radius = 0.028f;
	ParticleParallelBenchmark benchmark = { .type = ParticleParallelBenchmark_Powder, .radius = radius };
	benchmark.systemId = CreateBenchmarkParticleSystem( worldId, radius, 0.2f, 18000 );
	benchmark.primaryGroupId =
		CreateBoxGroup( benchmark.systemId, (b2Vec2){ -1.2f, 2.5f }, 1.35f, 1.25f, b2_powderParticle, 0,
						MakeParticleColor( 220, 185, 84 ) );
	benchmark.secondaryGroupId =
		CreateBoxGroup( benchmark.systemId, (b2Vec2){ 1.55f, 3.1f }, 1.1f, 1.05f, b2_powderParticle, 0,
						MakeParticleColor( 185, 140, 55 ) );
	b2ParticleSystem_SetPowderStrength( benchmark.systemId, 0.85f );
	return benchmark;
}

static ParticleParallelBenchmark CreateViscousFluid( b2WorldId worldId )
{
	CreateOpenBasin( worldId, 4.2f, 4.0f, 0.08f );
	CreateDynamicCircle( worldId, (b2Vec2){ 0.0f, 6.8f }, 0.55f, 0.7f );

	const float radius = 0.028f;
	ParticleParallelBenchmark benchmark = { .type = ParticleParallelBenchmark_ViscousFluid, .radius = radius };
	benchmark.systemId = CreateBenchmarkParticleSystem( worldId, radius, 0.2f, 18000 );
	b2ParticleSystem_SetViscousStrength( benchmark.systemId, 0.65f );
	benchmark.primaryGroupId = CreateCircleGroup( benchmark.systemId, (b2Vec2){ -1.2f, 2.5f }, 1.25f,
												  b2_viscousParticle | b2_colorMixingParticle, 0,
												  MakeParticleColor( 88, 199, 148 ) );
	benchmark.secondaryGroupId = CreateCircleGroup( benchmark.systemId, (b2Vec2){ 1.35f, 3.0f }, 1.05f,
													b2_viscousParticle | b2_colorMixingParticle, 0,
													MakeParticleColor( 48, 145, 108 ) );
	b2ParticleSystem_SetColorMixingStrength( benchmark.systemId, 0.4f );
	return benchmark;
}

static ParticleParallelBenchmark CreateBarrier( b2WorldId worldId )
{
	CreateClosedBox( worldId, 4.0f, 5.5f, 0.07f );

	const float radius = 0.03f;
	ParticleParallelBenchmark benchmark = { .type = ParticleParallelBenchmark_Barrier, .radius = radius };
	benchmark.systemId = CreateBenchmarkParticleSystem( worldId, radius, 0.25f, 22000 );
	benchmark.primaryGroupId = CreateBoxGroup( benchmark.systemId, (b2Vec2){ -1.75f, 2.2f }, 1.55f, 1.9f, b2_powderParticle, 0,
											   MakeParticleColor( 220, 185, 84 ) );
	benchmark.secondaryGroupId = CreateBoxGroup( benchmark.systemId, (b2Vec2){ 1.75f, 2.2f }, 1.55f, 1.9f,
												 b2_waterParticle | b2_colorMixingParticle, 0, MakeParticleColor( 64, 154, 255 ) );
	benchmark.tertiaryGroupId = CreateSegmentGroup( benchmark.systemId, (b2Vec2){ 0.0f, 0.35f }, (b2Vec2){ 0.0f, 5.0f },
													b2_wallParticle | b2_barrierParticle, MakeParticleColor( 255, 255, 255 ) );
	b2ParticleSystem_SetPowderStrength( benchmark.systemId, 0.85f );
	b2ParticleSystem_SetColorMixingStrength( benchmark.systemId, 0.5f );
	return benchmark;
}

static ParticleParallelBenchmark CreateSpatialIndexContacts( b2WorldId worldId )
{
	CreateClosedBox( worldId, 5.2f, 5.8f, 0.07f );

	const float radius = 0.026f;
	ParticleParallelBenchmark benchmark = { .type = ParticleParallelBenchmark_SpatialIndexContacts, .radius = radius };
	benchmark.systemId = CreateBenchmarkParticleSystem( worldId, radius, 0.12f, 30000 );
	benchmark.primaryGroupId = CreateBoxGroup( benchmark.systemId, (b2Vec2){ 0.0f, 2.8f }, 4.2f, 2.15f,
											   b2_waterParticle | b2_colorMixingParticle, 0, MakeParticleColor( 64, 154, 255 ) );
	b2ParticleSystem_SetColorMixingStrength( benchmark.systemId, 0.35f );
	return benchmark;
}

static ParticleParallelBenchmark CreateBarrierContacts( b2WorldId worldId )
{
	CreateClosedBox( worldId, 4.5f, 5.8f, 0.07f );

	const float radius = 0.03f;
	ParticleParallelBenchmark benchmark = { .type = ParticleParallelBenchmark_BarrierContacts, .radius = radius };
	benchmark.systemId = CreateBenchmarkParticleSystem( worldId, radius, 0.25f, 24000 );
	benchmark.primaryGroupId =
		CreateBoxGroup( benchmark.systemId, (b2Vec2){ -1.65f, 2.35f }, 1.45f, 1.95f, b2_powderParticle, 0,
						MakeParticleColor( 220, 185, 84 ) );
	benchmark.secondaryGroupId = CreateBoxGroup( benchmark.systemId, (b2Vec2){ 1.65f, 2.35f }, 1.45f, 1.95f,
												 b2_waterParticle | b2_colorMixingParticle, 0, MakeParticleColor( 64, 154, 255 ) );
	benchmark.tertiaryGroupId = CreateSegmentGroup( benchmark.systemId, (b2Vec2){ -0.9f, 0.45f }, (b2Vec2){ 0.9f, 5.1f },
													b2_wallParticle | b2_barrierParticle, MakeParticleColor( 255, 255, 255 ) );
	CreateSegmentGroup( benchmark.systemId, (b2Vec2){ 0.9f, 0.45f }, (b2Vec2){ -0.9f, 5.1f },
						b2_wallParticle | b2_barrierParticle, MakeParticleColor( 255, 255, 255 ) );
	CreateSegmentGroup( benchmark.systemId, (b2Vec2){ 0.0f, 0.35f }, (b2Vec2){ 0.0f, 5.25f },
						b2_wallParticle | b2_barrierParticle, MakeParticleColor( 255, 255, 255 ) );
	b2ParticleSystem_SetPowderStrength( benchmark.systemId, 0.85f );
	b2ParticleSystem_SetColorMixingStrength( benchmark.systemId, 0.5f );
	return benchmark;
}

static ParticleParallelBenchmark CreateSolidGroups( b2WorldId worldId )
{
	CreateOpenBasin( worldId, 4.2f, 4.0f, 0.08f );
	CreateDynamicCircle( worldId, (b2Vec2){ 0.0f, 7.0f }, 0.55f, 0.7f );

	const float radius = 0.028f;
	ParticleParallelBenchmark benchmark = { .type = ParticleParallelBenchmark_SolidGroups, .radius = radius };
	benchmark.systemId = CreateBenchmarkParticleSystem( worldId, radius, 0.25f, 18000 );

	uint32_t groupFlags = b2_solidParticleGroup;
	benchmark.primaryGroupId = CreateCircleGroup( benchmark.systemId, (b2Vec2){ -2.3f, 2.7f }, 0.75f,
												  b2_waterParticle | b2_colorMixingParticle, groupFlags,
												  MakeParticleColor( 255, 90, 90 ) );
	benchmark.secondaryGroupId = CreateCircleGroup( benchmark.systemId, (b2Vec2){ -0.7f, 3.2f }, 0.85f,
													b2_waterParticle | b2_colorMixingParticle, groupFlags,
													MakeParticleColor( 90, 220, 130 ) );
	benchmark.tertiaryGroupId = CreateRotatedBoxGroup( benchmark.systemId, (b2Vec2){ 1.5f, 3.1f }, 1.1f, 0.8f, -0.45f,
													   0.0f, b2_waterParticle | b2_colorMixingParticle, groupFlags,
													   MakeParticleColor( 90, 160, 255 ) );
	CreateRotatedBoxGroup( benchmark.systemId, (b2Vec2){ 2.9f, 4.1f }, 0.7f, 0.7f, 0.35f, 0.0f,
						   b2_waterParticle | b2_colorMixingParticle, groupFlags, MakeParticleColor( 255, 220, 90 ) );
	b2ParticleSystem_SetColorMixingStrength( benchmark.systemId, 0.5f );
	return benchmark;
}

static ParticleParallelBenchmark CreateSolidGroupRefresh( b2WorldId worldId )
{
	CreateOpenBasin( worldId, 4.8f, 4.4f, 0.08f );

	const float radius = 0.028f;
	ParticleParallelBenchmark benchmark = { .type = ParticleParallelBenchmark_SolidGroupRefresh, .radius = radius };
	benchmark.systemId = CreateBenchmarkParticleSystem( worldId, radius, 0.25f, 22000 );

	uint32_t groupFlags = b2_solidParticleGroup;
	for ( int i = 0; i < 8; ++i )
	{
		float x = -3.2f + 0.9f * (float)i;
		float y = 2.4f + 0.35f * (float)( i % 3 );
		b2ParticleColor color = MakeParticleColor( (uint8_t)( 80 + 20 * i ), (uint8_t)( 220 - 15 * i ), (uint8_t)( 120 + 10 * i ) );
		CreateCircleGroup( benchmark.systemId, (b2Vec2){ x, y }, 0.55f, b2_waterParticle | b2_colorMixingParticle, groupFlags, color );
	}

	b2ParticleSystem_SetColorMixingStrength( benchmark.systemId, 0.5f );
	return benchmark;
}

static ParticleParallelBenchmark CreateRigidGroups( b2WorldId worldId )
{
	CreateOpenBasin( worldId, 4.2f, 4.0f, 0.08f );
	CreateDynamicCircle( worldId, (b2Vec2){ 0.0f, 7.0f }, 0.55f, 0.7f );

	const float radius = 0.03f;
	ParticleParallelBenchmark benchmark = { .type = ParticleParallelBenchmark_RigidGroups, .radius = radius };
	benchmark.systemId = CreateBenchmarkParticleSystem( worldId, radius, 0.2f, 16000 );

	uint32_t groupFlags = b2_solidParticleGroup | b2_rigidParticleGroup;
	benchmark.primaryGroupId = CreateCircleGroup( benchmark.systemId, (b2Vec2){ -2.1f, 2.7f }, 0.7f, b2_waterParticle,
												  groupFlags, MakeParticleColor( 255, 90, 90 ) );
	benchmark.secondaryGroupId = CreateRotatedBoxGroup( benchmark.systemId, (b2Vec2){ -0.2f, 3.25f }, 1.1f, 0.65f, -0.35f,
														1.8f, b2_waterParticle, groupFlags, MakeParticleColor( 90, 220, 130 ) );
	benchmark.tertiaryGroupId = CreateCircleGroup( benchmark.systemId, (b2Vec2){ 2.2f, 3.2f }, 0.8f, b2_waterParticle,
												   groupFlags, MakeParticleColor( 90, 160, 255 ) );
	CreateRotatedBoxGroup( benchmark.systemId, (b2Vec2){ 0.9f, 4.6f }, 0.85f, 0.55f, 0.6f, -2.0f, b2_waterParticle,
						   groupFlags, MakeParticleColor( 255, 220, 90 ) );
	return benchmark;
}

static ParticleParallelBenchmark CreateRigidGroupRefresh( b2WorldId worldId )
{
	CreateOpenBasin( worldId, 4.8f, 4.4f, 0.08f );

	const float radius = 0.03f;
	ParticleParallelBenchmark benchmark = { .type = ParticleParallelBenchmark_RigidGroupRefresh, .radius = radius };
	benchmark.systemId = CreateBenchmarkParticleSystem( worldId, radius, 0.2f, 18000 );

	uint32_t groupFlags = b2_solidParticleGroup | b2_rigidParticleGroup;
	for ( int i = 0; i < 7; ++i )
	{
		float x = -3.0f + 1.0f * (float)i;
		float y = 2.5f + 0.42f * (float)( i % 2 );
		float angle = -0.45f + 0.15f * (float)i;
		float angularVelocity = ( ( i & 1 ) == 0 ? 1.0f : -1.0f ) * ( 1.0f + 0.2f * (float)i );
		b2ParticleColor color = MakeParticleColor( (uint8_t)( 255 - 18 * i ), (uint8_t)( 90 + 18 * i ), (uint8_t)( 110 + 12 * i ) );
		CreateRotatedBoxGroup( benchmark.systemId, (b2Vec2){ x, y }, 0.55f, 0.4f, angle, angularVelocity, b2_waterParticle,
							   groupFlags, color );
	}

	return benchmark;
}

static ParticleParallelBenchmark CreateElasticGroups( b2WorldId worldId )
{
	CreateOpenBasin( worldId, 4.2f, 4.0f, 0.08f );
	CreateDynamicCircle( worldId, (b2Vec2){ 0.0f, 7.0f }, 0.55f, 0.7f );

	const float radius = 0.03f;
	ParticleParallelBenchmark benchmark = { .type = ParticleParallelBenchmark_ElasticGroups, .radius = radius };
	benchmark.systemId = CreateBenchmarkParticleSystem( worldId, radius, 0.25f, 18000 );

	uint32_t groupFlags = b2_solidParticleGroup;
	benchmark.primaryGroupId = CreateCircleGroup( benchmark.systemId, (b2Vec2){ -2.1f, 2.7f }, 0.75f,
												  b2_springParticle | b2_colorMixingParticle, groupFlags,
												  MakeParticleColor( 255, 90, 90 ) );
	benchmark.secondaryGroupId = CreateRotatedBoxGroup( benchmark.systemId, (b2Vec2){ -0.1f, 3.25f }, 1.1f, 0.65f, -0.35f,
														0.0f, b2_elasticParticle | b2_colorMixingParticle, groupFlags,
														MakeParticleColor( 90, 220, 130 ) );
	benchmark.tertiaryGroupId = CreateCircleGroup( benchmark.systemId, (b2Vec2){ 2.2f, 3.2f }, 0.8f,
												   b2_elasticParticle | b2_colorMixingParticle, groupFlags,
												   MakeParticleColor( 90, 160, 255 ) );
	CreateRotatedBoxGroup( benchmark.systemId, (b2Vec2){ 1.0f, 4.6f }, 0.85f, 0.55f, 0.6f, 0.0f,
						   b2_springParticle | b2_colorMixingParticle, groupFlags, MakeParticleColor( 255, 220, 90 ) );
	b2ParticleSystem_SetColorMixingStrength( benchmark.systemId, 0.5f );
	return benchmark;
}

const char* GetParticleParallelBenchmarkName( ParticleParallelBenchmarkType type )
{
	static const char* names[] = {
		"Large Dam Break",
		"Faucet",
		"Wave Machine",
		"Particle Body Collision",
		"Powder",
		"Viscous Fluid",
		"Barrier",
		"Solid Groups",
		"Rigid Groups",
		"Elastic Groups",
		"Spatial Index Contacts",
		"Barrier Contacts",
		"Solid Group Refresh",
		"Rigid Group Refresh",
	};

	if ( type < 0 || ParticleParallelBenchmark_Count <= type )
	{
		return "Unknown";
	}

	return names[type];
}

ParticleParallelBenchmarkRunDef ParticleParallelBenchmark_DefaultRunDef( ParticleParallelBenchmarkType type )
{
	ParticleParallelBenchmarkRunDef def = {
		.type = type,
		.warmupStepCount = 60,
		.measuredStepCount = 240,
		.timeStep = 1.0f / 60.0f,
		.subStepCount = 4,
	};

	if ( type == ParticleParallelBenchmark_Faucet )
	{
		def.warmupStepCount = 180;
		def.measuredStepCount = 300;
	}
	else if ( type == ParticleParallelBenchmark_SpatialIndexContacts || type == ParticleParallelBenchmark_BarrierContacts )
	{
		def.warmupStepCount = 45;
		def.measuredStepCount = 180;
	}
	else if ( type == ParticleParallelBenchmark_ParticleBodyCollision )
	{
		def.warmupStepCount = 90;
		def.measuredStepCount = 240;
	}

	return def;
}

ParticleParallelBenchmark CreateParticleParallelBenchmark( b2WorldId worldId, ParticleParallelBenchmarkType type )
{
	if ( b2World_IsValid( worldId ) == false )
	{
		return (ParticleParallelBenchmark){ .type = type };
	}

	b2World_EnableSleeping( worldId, false );

	switch ( type )
	{
		case ParticleParallelBenchmark_LargeDamBreak:
			return CreateLargeDamBreak( worldId );

		case ParticleParallelBenchmark_Faucet:
			return CreateFaucet( worldId );

		case ParticleParallelBenchmark_WaveMachine:
			return CreateWaveMachine( worldId );

		case ParticleParallelBenchmark_ParticleBodyCollision:
			return CreateParticleBodyCollision( worldId );

		case ParticleParallelBenchmark_Powder:
			return CreatePowder( worldId );

		case ParticleParallelBenchmark_ViscousFluid:
			return CreateViscousFluid( worldId );

		case ParticleParallelBenchmark_Barrier:
			return CreateBarrier( worldId );

		case ParticleParallelBenchmark_SpatialIndexContacts:
			return CreateSpatialIndexContacts( worldId );

		case ParticleParallelBenchmark_BarrierContacts:
			return CreateBarrierContacts( worldId );

		case ParticleParallelBenchmark_SolidGroups:
			return CreateSolidGroups( worldId );

		case ParticleParallelBenchmark_SolidGroupRefresh:
			return CreateSolidGroupRefresh( worldId );

		case ParticleParallelBenchmark_RigidGroups:
			return CreateRigidGroups( worldId );

		case ParticleParallelBenchmark_RigidGroupRefresh:
			return CreateRigidGroupRefresh( worldId );

		case ParticleParallelBenchmark_ElasticGroups:
			return CreateElasticGroups( worldId );

		case ParticleParallelBenchmark_Count:
		default:
			break;
	}

	return (ParticleParallelBenchmark){ .type = type };
}

static void EmitFaucetParticles( ParticleParallelBenchmark* benchmark, float timeStep )
{
	const b2Vec2 emitterPosition = { 0.0f, 4.5f };
	const float emitRate = 480.0f;
	const float emitWidth = 0.36f;

	benchmark->emitAccumulator += timeStep * emitRate;
	int emitCount = (int)benchmark->emitAccumulator;
	benchmark->emitAccumulator -= (float)emitCount;

	static const b2ParticleColor colors[] = {
		{ 255, 255, 255, 255 },
		{ 255, 90, 90, 255 },
		{ 255, 220, 90, 255 },
		{ 90, 180, 255, 255 },
		{ 120, 255, 180, 255 },
	};

	for ( int i = 0; i < emitCount; ++i )
	{
		int index = benchmark->emittedCount++;
		float phase = (float)( ( index * 37 ) % 97 ) / 96.0f;
		float velocityPhase = (float)( ( index * 53 ) % 101 ) / 100.0f;

		b2ParticleDef particleDef = b2DefaultParticleDef();
		particleDef.flags = b2_waterParticle | b2_colorMixingParticle;
		particleDef.position = (b2Vec2){ emitterPosition.x + ( phase - 0.5f ) * emitWidth, emitterPosition.y };
		particleDef.velocity = (b2Vec2){ ( velocityPhase - 0.5f ) * 0.25f, -0.25f };
		particleDef.color = colors[index % (int)( sizeof( colors ) / sizeof( colors[0] ) )];
		particleDef.lifetime = 18.0f + 16.0f * velocityPhase;

		b2ParticleSystem_CreateParticle( benchmark->systemId, &particleDef );
	}
}

void StepParticleParallelBenchmark( b2WorldId worldId, ParticleParallelBenchmark* benchmark, float timeStep, int subStepCount )
{
	if ( benchmark == NULL || b2World_IsValid( worldId ) == false )
	{
		return;
	}

	if ( timeStep <= 0.0f )
	{
		timeStep = 1.0f / 60.0f;
	}

	if ( subStepCount <= 0 )
	{
		subStepCount = 4;
	}

	if ( benchmark->type == ParticleParallelBenchmark_Faucet && b2ParticleSystem_IsValid( benchmark->systemId ) )
	{
		EmitFaucetParticles( benchmark, timeStep );
	}
	else if ( benchmark->type == ParticleParallelBenchmark_WaveMachine && B2_IS_NON_NULL( benchmark->waveJointId ) &&
			  b2Joint_IsValid( benchmark->waveJointId ) )
	{
		b2RevoluteJoint_SetMotorSpeed( benchmark->waveJointId, 0.05f * cosf( benchmark->time ) * B2_PI );
	}

	benchmark->time += timeStep;
	b2World_Step( worldId, timeStep, subStepCount );
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

uint32_t HashParticleParallelBenchmark( const ParticleParallelBenchmark* benchmark )
{
	if ( benchmark == NULL || b2ParticleSystem_IsValid( benchmark->systemId ) == false )
	{
		return 0;
	}

	uint32_t hash = 2166136261u;
	b2ParticleSystemCounters counters = b2ParticleSystem_GetCounters( benchmark->systemId );
	hash = HashWord( hash, (uint32_t)benchmark->type );
	hash = HashWord( hash, (uint32_t)counters.particleCount );
	hash = HashWord( hash, (uint32_t)counters.groupCount );
	hash = HashWord( hash, (uint32_t)counters.contactCount );
	hash = HashWord( hash, (uint32_t)counters.bodyContactCount );
	hash = HashWord( hash, (uint32_t)counters.pairCount );
	hash = HashWord( hash, (uint32_t)counters.triadCount );
	hash = HashWord( hash, (uint32_t)benchmark->emittedCount );
	hash = HashWord( hash, (uint32_t)benchmark->dynamicBodyCount );

	const b2Vec2* positions = b2ParticleSystem_GetPositionBuffer( benchmark->systemId );
	const b2Vec2* velocities = b2ParticleSystem_GetVelocityBuffer( benchmark->systemId );
	const uint32_t* flags = b2ParticleSystem_GetFlagsBuffer( benchmark->systemId );
	const b2ParticleColor* colors = b2ParticleSystem_GetColorBuffer( benchmark->systemId );
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

static void AddParticleProfile( b2ParticleProfile* total, b2ParticleProfile profile )
{
	total->step += profile.step;
	total->lifetimes += profile.lifetimes;
	total->zombie += profile.zombie;
	total->proxies += profile.proxies;
	total->spatialIndexBuild += profile.spatialIndexBuild;
	total->spatialIndexScatter += profile.spatialIndexScatter;
	total->contacts += profile.contacts;
	total->contactCandidates += profile.contactCandidates;
	total->contactMerge += profile.contactMerge;
	total->bodyContacts += profile.bodyContacts;
	total->bodyCandidateQuery += profile.bodyCandidateQuery;
	total->weights += profile.weights;
	total->forces += profile.forces;
	total->pressure += profile.pressure;
	total->damping += profile.damping;
	total->reductionGenerate += profile.reductionGenerate;
	total->reductionApply += profile.reductionApply;
	total->collision += profile.collision;
	total->solidDepth += profile.solidDepth;
	total->solidEjection += profile.solidEjection;
	total->groups += profile.groups;
	total->groupRefresh += profile.groupRefresh;
	total->barrier += profile.barrier;
	total->compaction += profile.compaction;
	total->compactionMark += profile.compactionMark;
	total->compactionPrefix += profile.compactionPrefix;
	total->compactionScatter += profile.compactionScatter;
	total->compactionRemap += profile.compactionRemap;
	total->scratch += profile.scratch;
	total->events += profile.events;
}

static void ScaleParticleProfile( b2ParticleProfile* profile, float scale )
{
	profile->step *= scale;
	profile->lifetimes *= scale;
	profile->zombie *= scale;
	profile->proxies *= scale;
	profile->spatialIndexBuild *= scale;
	profile->spatialIndexScatter *= scale;
	profile->contacts *= scale;
	profile->contactCandidates *= scale;
	profile->contactMerge *= scale;
	profile->bodyContacts *= scale;
	profile->bodyCandidateQuery *= scale;
	profile->weights *= scale;
	profile->forces *= scale;
	profile->pressure *= scale;
	profile->damping *= scale;
	profile->reductionGenerate *= scale;
	profile->reductionApply *= scale;
	profile->collision *= scale;
	profile->solidDepth *= scale;
	profile->solidEjection *= scale;
	profile->groups *= scale;
	profile->groupRefresh *= scale;
	profile->barrier *= scale;
	profile->compaction *= scale;
	profile->compactionMark *= scale;
	profile->compactionPrefix *= scale;
	profile->compactionScatter *= scale;
	profile->compactionRemap *= scale;
	profile->scratch *= scale;
	profile->events *= scale;
}

static void AddWorldProfile( b2Profile* total, b2Profile profile )
{
	total->step += profile.step;
	total->pairs += profile.pairs;
	total->collide += profile.collide;
	total->solve += profile.solve;
	total->prepareStages += profile.prepareStages;
	total->solveConstraints += profile.solveConstraints;
	total->prepareConstraints += profile.prepareConstraints;
	total->integrateVelocities += profile.integrateVelocities;
	total->warmStart += profile.warmStart;
	total->solveImpulses += profile.solveImpulses;
	total->integratePositions += profile.integratePositions;
	total->relaxImpulses += profile.relaxImpulses;
	total->applyRestitution += profile.applyRestitution;
	total->storeImpulses += profile.storeImpulses;
	total->splitIslands += profile.splitIslands;
	total->transforms += profile.transforms;
	total->sensorHits += profile.sensorHits;
	total->jointEvents += profile.jointEvents;
	total->hitEvents += profile.hitEvents;
	total->refit += profile.refit;
	total->bullets += profile.bullets;
	total->sleepIslands += profile.sleepIslands;
	total->sensors += profile.sensors;
	total->particles += profile.particles;
	total->particleLifetimes += profile.particleLifetimes;
	total->particleZombie += profile.particleZombie;
	total->particleProxies += profile.particleProxies;
	total->particleSpatialIndexBuild += profile.particleSpatialIndexBuild;
	total->particleSpatialIndexScatter += profile.particleSpatialIndexScatter;
	total->particleContacts += profile.particleContacts;
	total->particleContactCandidates += profile.particleContactCandidates;
	total->particleContactMerge += profile.particleContactMerge;
	total->particleBodyContacts += profile.particleBodyContacts;
	total->particleBodyCandidateQuery += profile.particleBodyCandidateQuery;
	total->particleWeights += profile.particleWeights;
	total->particleForces += profile.particleForces;
	total->particlePressure += profile.particlePressure;
	total->particleDamping += profile.particleDamping;
	total->particleReductionGenerate += profile.particleReductionGenerate;
	total->particleReductionApply += profile.particleReductionApply;
	total->particleCollision += profile.particleCollision;
	total->particleSolidDepth += profile.particleSolidDepth;
	total->particleSolidEjection += profile.particleSolidEjection;
	total->particleGroups += profile.particleGroups;
	total->particleGroupRefresh += profile.particleGroupRefresh;
	total->particleBarrier += profile.particleBarrier;
	total->particleCompaction += profile.particleCompaction;
	total->particleCompactionMark += profile.particleCompactionMark;
	total->particleCompactionPrefix += profile.particleCompactionPrefix;
	total->particleCompactionScatter += profile.particleCompactionScatter;
	total->particleCompactionRemap += profile.particleCompactionRemap;
	total->particleScratch += profile.particleScratch;
	total->particleEvents += profile.particleEvents;
}

static void ScaleWorldProfile( b2Profile* profile, float scale )
{
	profile->step *= scale;
	profile->pairs *= scale;
	profile->collide *= scale;
	profile->solve *= scale;
	profile->prepareStages *= scale;
	profile->solveConstraints *= scale;
	profile->prepareConstraints *= scale;
	profile->integrateVelocities *= scale;
	profile->warmStart *= scale;
	profile->solveImpulses *= scale;
	profile->integratePositions *= scale;
	profile->relaxImpulses *= scale;
	profile->applyRestitution *= scale;
	profile->storeImpulses *= scale;
	profile->splitIslands *= scale;
	profile->transforms *= scale;
	profile->sensorHits *= scale;
	profile->jointEvents *= scale;
	profile->hitEvents *= scale;
	profile->refit *= scale;
	profile->bullets *= scale;
	profile->sleepIslands *= scale;
	profile->sensors *= scale;
	profile->particles *= scale;
	profile->particleLifetimes *= scale;
	profile->particleZombie *= scale;
	profile->particleProxies *= scale;
	profile->particleSpatialIndexBuild *= scale;
	profile->particleSpatialIndexScatter *= scale;
	profile->particleContacts *= scale;
	profile->particleContactCandidates *= scale;
	profile->particleContactMerge *= scale;
	profile->particleBodyContacts *= scale;
	profile->particleBodyCandidateQuery *= scale;
	profile->particleWeights *= scale;
	profile->particleForces *= scale;
	profile->particlePressure *= scale;
	profile->particleDamping *= scale;
	profile->particleReductionGenerate *= scale;
	profile->particleReductionApply *= scale;
	profile->particleCollision *= scale;
	profile->particleSolidDepth *= scale;
	profile->particleSolidEjection *= scale;
	profile->particleGroups *= scale;
	profile->particleGroupRefresh *= scale;
	profile->particleBarrier *= scale;
	profile->particleCompaction *= scale;
	profile->particleCompactionMark *= scale;
	profile->particleCompactionPrefix *= scale;
	profile->particleCompactionScatter *= scale;
	profile->particleCompactionRemap *= scale;
	profile->particleScratch *= scale;
	profile->particleEvents *= scale;
}

ParticleParallelBenchmarkResult RunParticleParallelBenchmark( b2WorldId worldId, ParticleParallelBenchmark* benchmark,
															  const ParticleParallelBenchmarkRunDef* runDef, int workerCount )
{
	ParticleParallelBenchmarkRunDef def =
		runDef != NULL ? *runDef : ParticleParallelBenchmark_DefaultRunDef( benchmark != NULL ? benchmark->type : 0 );

	if ( def.timeStep <= 0.0f )
	{
		def.timeStep = 1.0f / 60.0f;
	}

	if ( def.subStepCount <= 0 )
	{
		def.subStepCount = 4;
	}

	if ( def.warmupStepCount < 0 )
	{
		def.warmupStepCount = 0;
	}

	if ( def.measuredStepCount < 0 )
	{
		def.measuredStepCount = 0;
	}

	ParticleParallelBenchmarkResult result = {
		.type = def.type,
		.workerCount = workerCount,
		.warmupStepCount = def.warmupStepCount,
		.measuredStepCount = def.measuredStepCount,
	};

	if ( benchmark == NULL || b2World_IsValid( worldId ) == false )
	{
		return result;
	}

	for ( int i = 0; i < def.warmupStepCount; ++i )
	{
		StepParticleParallelBenchmark( worldId, benchmark, def.timeStep, def.subStepCount );
	}

	uint64_t ticks = b2GetTicks();
	for ( int i = 0; i < def.measuredStepCount; ++i )
	{
		StepParticleParallelBenchmark( worldId, benchmark, def.timeStep, def.subStepCount );
		AddWorldProfile( &result.averageWorldProfile, b2World_GetProfile( worldId ) );

		if ( b2ParticleSystem_IsValid( benchmark->systemId ) )
		{
			AddParticleProfile( &result.averageParticleProfile, b2ParticleSystem_GetProfile( benchmark->systemId ) );
		}
	}
	result.elapsedMilliseconds = b2GetMilliseconds( ticks );

	if ( def.measuredStepCount > 0 )
	{
		float invStepCount = 1.0f / (float)def.measuredStepCount;
		result.stepMilliseconds = result.elapsedMilliseconds * invStepCount;
		ScaleWorldProfile( &result.averageWorldProfile, invStepCount );
		ScaleParticleProfile( &result.averageParticleProfile, invStepCount );
	}

	if ( b2World_IsValid( worldId ) )
	{
		result.worldCounters = b2World_GetCounters( worldId );
	}

	if ( b2ParticleSystem_IsValid( benchmark->systemId ) )
	{
		result.particleCounters = b2ParticleSystem_GetCounters( benchmark->systemId );
	}

	result.particleCount = result.particleCounters.particleCount;
	result.dynamicBodyCount = benchmark->dynamicBodyCount;
	result.particleBodyContactCount = result.particleCounters.bodyContactCount;
	result.hash = HashParticleParallelBenchmark( benchmark );
	return result;
}

int CompareParticleParallelBenchmarkWorkers( const ParticleParallelWorkerComparisonDef* comparisonDef,
											 ParticleParallelBenchmarkResult* results, int resultCapacity )
{
	if ( comparisonDef == NULL || results == NULL || resultCapacity <= 0 )
	{
		return 0;
	}

	int minWorkerCount = comparisonDef->minWorkerCount > 0 ? comparisonDef->minWorkerCount : 1;
	int maxWorkerCount = comparisonDef->maxWorkerCount >= minWorkerCount ? comparisonDef->maxWorkerCount : minWorkerCount;
	int resultCount = 0;

	for ( int workerCount = minWorkerCount; workerCount <= maxWorkerCount && resultCount < resultCapacity; ++workerCount )
	{
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = (b2Vec2){ 0.0f, -10.0f };
		worldDef.workerCount = workerCount;

		if ( comparisonDef->configureWorld != NULL )
		{
			comparisonDef->configureWorld( workerCount, &worldDef, comparisonDef->context );
		}

		b2WorldId worldId = b2CreateWorld( &worldDef );
		ParticleParallelBenchmark benchmark = CreateParticleParallelBenchmark( worldId, comparisonDef->runDef.type );
		results[resultCount] = RunParticleParallelBenchmark( worldId, &benchmark, &comparisonDef->runDef, workerCount );
		++resultCount;

		b2DestroyWorld( worldId );

		if ( comparisonDef->finishWorld != NULL )
		{
			comparisonDef->finishWorld( workerCount, comparisonDef->context );
		}
	}

	return resultCount;
}
