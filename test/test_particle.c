// SPDX-FileCopyrightText: 2026 Erin Catto
// SPDX-License-Identifier: MIT

#include "test_macros.h"

#include "corephys/corephys.h"
#include "corephys/particle.h"

#include <float.h>
#include <stddef.h>
#include <stdint.h>

static bool WorldIdsEqual( b2WorldId a, b2WorldId b )
{
	return a.index1 == b.index1 && a.generation == b.generation;
}

static b2WorldId CreateParticleTestWorld( void )
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){ 0.0f, 0.0f };
	return b2CreateWorld( &worldDef );
}

static int ParticleFlagValues( void )
{
	ENSURE( b2_waterParticle == 0 );
	ENSURE( b2_zombieParticle == ( 1u << 1 ) );
	ENSURE( b2_wallParticle == ( 1u << 2 ) );
	ENSURE( b2_springParticle == ( 1u << 3 ) );
	ENSURE( b2_elasticParticle == ( 1u << 4 ) );
	ENSURE( b2_viscousParticle == ( 1u << 5 ) );
	ENSURE( b2_powderParticle == ( 1u << 6 ) );
	ENSURE( b2_tensileParticle == ( 1u << 7 ) );
	ENSURE( b2_colorMixingParticle == ( 1u << 8 ) );
	ENSURE( b2_destructionListenerParticle == ( 1u << 9 ) );
	ENSURE( b2_barrierParticle == ( 1u << 10 ) );
	ENSURE( b2_staticPressureParticle == ( 1u << 11 ) );
	ENSURE( b2_reactiveParticle == ( 1u << 12 ) );
	ENSURE( b2_repulsiveParticle == ( 1u << 13 ) );
	ENSURE( b2_bodyContactListenerParticle == ( 1u << 14 ) );
	ENSURE( b2_particleContactListenerParticle == ( 1u << 15 ) );
	ENSURE( b2_bodyContactFilterParticle == ( 1u << 16 ) );
	ENSURE( b2_particleContactFilterParticle == ( 1u << 17 ) );

	ENSURE( b2_solidParticleGroup == ( 1u << 0 ) );
	ENSURE( b2_rigidParticleGroup == ( 1u << 1 ) );
	ENSURE( b2_particleGroupCanBeEmpty == ( 1u << 2 ) );

	return 0;
}

static int ParticleDefaultDefs( void )
{
	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	ENSURE( systemDef.strictContactCheck == false );
	ENSURE_SMALL( systemDef.density - 1.0f, FLT_EPSILON );
	ENSURE_SMALL( systemDef.gravityScale - 1.0f, FLT_EPSILON );
	ENSURE_SMALL( systemDef.radius - 1.0f, FLT_EPSILON );
	ENSURE( systemDef.maxCount == 0 );
	ENSURE_SMALL( systemDef.pressureStrength - 0.05f, FLT_EPSILON );
	ENSURE_SMALL( systemDef.dampingStrength - 1.0f, FLT_EPSILON );
	ENSURE_SMALL( systemDef.elasticStrength - 0.25f, FLT_EPSILON );
	ENSURE_SMALL( systemDef.springStrength - 0.25f, FLT_EPSILON );
	ENSURE_SMALL( systemDef.viscousStrength - 0.25f, FLT_EPSILON );
	ENSURE_SMALL( systemDef.surfaceTensionPressureStrength - 0.2f, FLT_EPSILON );
	ENSURE_SMALL( systemDef.surfaceTensionNormalStrength - 0.2f, FLT_EPSILON );
	ENSURE_SMALL( systemDef.repulsiveStrength - 1.0f, FLT_EPSILON );
	ENSURE_SMALL( systemDef.powderStrength - 0.5f, FLT_EPSILON );
	ENSURE_SMALL( systemDef.ejectionStrength - 0.5f, FLT_EPSILON );
	ENSURE_SMALL( systemDef.staticPressureStrength - 0.2f, FLT_EPSILON );
	ENSURE_SMALL( systemDef.staticPressureRelaxation - 0.2f, FLT_EPSILON );
	ENSURE( systemDef.staticPressureIterations == 8 );
	ENSURE_SMALL( systemDef.colorMixingStrength - 0.5f, FLT_EPSILON );
	ENSURE( systemDef.destroyByAge == true );
	ENSURE_SMALL( systemDef.lifetimeGranularity - 1.0f / 60.0f, FLT_EPSILON );
	ENSURE( systemDef.iterationCount == 1 );
	ENSURE( systemDef.enableParticleEvents == false );
	ENSURE( systemDef.enableParticleBodyEvents == false );
	ENSURE( systemDef.enableParticleDebugDraw == true );

	b2ParticleDef particleDef = b2DefaultParticleDef();
	ENSURE( particleDef.flags == 0 );
	ENSURE_SMALL( particleDef.position.x, FLT_EPSILON );
	ENSURE_SMALL( particleDef.position.y, FLT_EPSILON );
	ENSURE_SMALL( particleDef.velocity.x, FLT_EPSILON );
	ENSURE_SMALL( particleDef.velocity.y, FLT_EPSILON );
	ENSURE( particleDef.color.r == 0 );
	ENSURE( particleDef.color.g == 0 );
	ENSURE( particleDef.color.b == 0 );
	ENSURE( particleDef.color.a == 0 );
	ENSURE_SMALL( particleDef.lifetime, FLT_EPSILON );
	ENSURE( particleDef.userData == NULL );
	ENSURE( B2_IS_NULL( particleDef.groupId ) );

	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	ENSURE( groupDef.flags == 0 );
	ENSURE( groupDef.groupFlags == 0 );
	ENSURE_SMALL( groupDef.position.x, FLT_EPSILON );
	ENSURE_SMALL( groupDef.position.y, FLT_EPSILON );
	ENSURE_SMALL( groupDef.rotation.c - 1.0f, FLT_EPSILON );
	ENSURE_SMALL( groupDef.rotation.s, FLT_EPSILON );
	ENSURE_SMALL( groupDef.linearVelocity.x, FLT_EPSILON );
	ENSURE_SMALL( groupDef.linearVelocity.y, FLT_EPSILON );
	ENSURE_SMALL( groupDef.angularVelocity, FLT_EPSILON );
	ENSURE( groupDef.color.r == 0 );
	ENSURE( groupDef.color.g == 0 );
	ENSURE( groupDef.color.b == 0 );
	ENSURE( groupDef.color.a == 0 );
	ENSURE_SMALL( groupDef.strength - 1.0f, FLT_EPSILON );
	ENSURE( groupDef.circle == NULL );
	ENSURE( groupDef.capsule == NULL );
	ENSURE( groupDef.segment == NULL );
	ENSURE( groupDef.polygon == NULL );
	ENSURE_SMALL( groupDef.stride, FLT_EPSILON );
	ENSURE( groupDef.particleCount == 0 );
	ENSURE( groupDef.positionData == NULL );
	ENSURE_SMALL( groupDef.lifetime, FLT_EPSILON );
	ENSURE( groupDef.userData == NULL );
	ENSURE( B2_IS_NULL( groupDef.groupId ) );

	return 0;
}

static int ParticleSystemCreateDestroy( void )
{
	b2ParticleSystemDef invalidSystemDef = b2DefaultParticleSystemDef();
	ENSURE( B2_IS_NULL( b2CreateParticleSystem( b2_nullWorldId, &invalidSystemDef ) ) );

	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.125f;
	systemDef.density = 2.0f;
	systemDef.gravityScale = 0.5f;
	systemDef.dampingStrength = 0.25f;
	systemDef.maxCount = 32;
	systemDef.iterationCount = 3;

	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );
	ENSURE( WorldIdsEqual( b2ParticleSystem_GetWorld( systemId ), worldId ) );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 0 );
	ENSURE( b2ParticleSystem_GetMaxParticleCount( systemId ) == 32 );
	ENSURE_SMALL( b2ParticleSystem_GetRadius( systemId ) - 0.125f, FLT_EPSILON );
	ENSURE_SMALL( b2ParticleSystem_GetDensity( systemId ) - 2.0f, FLT_EPSILON );
	ENSURE_SMALL( b2ParticleSystem_GetGravityScale( systemId ) - 0.5f, FLT_EPSILON );
	ENSURE_SMALL( b2ParticleSystem_GetDamping( systemId ) - 0.25f, FLT_EPSILON );
	ENSURE( b2ParticleSystem_GetIterationCount( systemId ) == 3 );

	b2Counters counters = b2World_GetCounters( worldId );
	ENSURE( counters.particleSystemCount == 1 );
	ENSURE( counters.particleGroupCount == 0 );
	ENSURE( counters.particleCount == 0 );

	b2DestroyParticleSystem( systemId );
	ENSURE( b2ParticleSystem_IsValid( systemId ) == false );

	counters = b2World_GetCounters( worldId );
	ENSURE( counters.particleSystemCount == 0 );
	ENSURE( counters.particleGroupCount == 0 );
	ENSURE( counters.particleCount == 0 );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );

	return 0;
}

static int ParticleCreateAndBuffers( void )
{
	static int userDataA = 17;
	static int userDataB = 23;

	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.05f;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.flags = b2_viscousParticle;
	particleDef.position = (b2Vec2){ 1.0f, 2.0f };
	particleDef.velocity = (b2Vec2){ 3.0f, 4.0f };
	particleDef.color = (b2ParticleColor){ 10, 20, 30, 40 };
	particleDef.userData = &userDataA;

	int particleA = b2ParticleSystem_CreateParticle( systemId, &particleDef );
	ENSURE( particleA == 0 );

	particleDef.flags = b2_tensileParticle | b2_colorMixingParticle;
	particleDef.position = (b2Vec2){ -2.0f, 5.0f };
	particleDef.velocity = (b2Vec2){ -7.0f, 8.0f };
	particleDef.color = (b2ParticleColor){ 200, 180, 160, 140 };
	particleDef.userData = &userDataB;

	int particleB = b2ParticleSystem_CreateParticle( systemId, &particleDef );
	ENSURE( particleB == 1 );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 2 );

	const b2Vec2* positions = b2ParticleSystem_GetPositionBuffer( systemId );
	const b2Vec2* velocities = b2ParticleSystem_GetVelocityBuffer( systemId );
	const uint32_t* flags = b2ParticleSystem_GetFlagsBuffer( systemId );
	const b2ParticleColor* colors = b2ParticleSystem_GetColorBuffer( systemId );
	const float* weights = b2ParticleSystem_GetWeightBuffer( systemId );
	const float* lifetimes = b2ParticleSystem_GetLifetimeBuffer( systemId );
	void* const* userData = b2ParticleSystem_GetUserDataBuffer( systemId );

	ENSURE( positions != NULL );
	ENSURE( velocities != NULL );
	ENSURE( flags != NULL );
	ENSURE( colors != NULL );
	ENSURE( weights != NULL );
	ENSURE( lifetimes != NULL );
	ENSURE( userData != NULL );
	ENSURE( b2ParticleSystem_GetMutableFlagsBuffer( systemId ) != NULL );
	ENSURE( b2ParticleSystem_GetMutableColorBuffer( systemId ) != NULL );
	ENSURE( b2ParticleSystem_GetMutableLifetimeBuffer( systemId ) != NULL );
	ENSURE( b2ParticleSystem_GetMutableUserDataBuffer( systemId ) != NULL );

	ENSURE_SMALL( positions[particleA].x - 1.0f, FLT_EPSILON );
	ENSURE_SMALL( positions[particleA].y - 2.0f, FLT_EPSILON );
	ENSURE_SMALL( velocities[particleA].x - 3.0f, FLT_EPSILON );
	ENSURE_SMALL( velocities[particleA].y - 4.0f, FLT_EPSILON );
	ENSURE( flags[particleA] == b2_viscousParticle );
	ENSURE( colors[particleA].r == 10 );
	ENSURE( colors[particleA].g == 20 );
	ENSURE( colors[particleA].b == 30 );
	ENSURE( colors[particleA].a == 40 );
	ENSURE( userData[particleA] == &userDataA );
	ENSURE_SMALL( weights[particleA], FLT_EPSILON );

	ENSURE_SMALL( positions[particleB].x + 2.0f, FLT_EPSILON );
	ENSURE_SMALL( positions[particleB].y - 5.0f, FLT_EPSILON );
	ENSURE_SMALL( velocities[particleB].x + 7.0f, FLT_EPSILON );
	ENSURE_SMALL( velocities[particleB].y - 8.0f, FLT_EPSILON );
	ENSURE( flags[particleB] == ( b2_tensileParticle | b2_colorMixingParticle ) );
	ENSURE( colors[particleB].r == 200 );
	ENSURE( colors[particleB].g == 180 );
	ENSURE( colors[particleB].b == 160 );
	ENSURE( colors[particleB].a == 140 );
	ENSURE( userData[particleB] == &userDataB );
	ENSURE_SMALL( weights[particleB], FLT_EPSILON );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );

	return 0;
}

static int ParticleMaxCount( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.maxCount = 2;
	systemDef.destroyByAge = false;

	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );
	ENSURE( b2ParticleSystem_GetMaxParticleCount( systemId ) == 2 );

	b2ParticleDef particleDef = b2DefaultParticleDef();
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 0 );
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 1 );
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) < 0 );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 2 );

	b2ParticleSystem_SetMaxParticleCount( systemId, 3 );
	ENSURE( b2ParticleSystem_GetMaxParticleCount( systemId ) == 3 );
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 2 );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 3 );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );

	return 0;
}

static int ParticleLifetimesAfterStepping( void )
{
	static int expiredUserData = 1;
	static int persistentUserData = 2;

	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.05f;
	systemDef.lifetimeGranularity = 1.0f / 60.0f;

	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.position = (b2Vec2){ 0.0f, 0.0f };
	particleDef.lifetime = 1.0f / 60.0f;
	particleDef.userData = &expiredUserData;
	int expiringParticle = b2ParticleSystem_CreateParticle( systemId, &particleDef );
	ENSURE( expiringParticle == 0 );

	particleDef.position = (b2Vec2){ 1.0f, 0.0f };
	particleDef.lifetime = 0.0f;
	particleDef.userData = &persistentUserData;
	int persistentParticle = b2ParticleSystem_CreateParticle( systemId, &particleDef );
	ENSURE( persistentParticle == 1 );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 2 );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 1 );

	void* const* userData = b2ParticleSystem_GetUserDataBuffer( systemId );
	ENSURE( userData != NULL );
	ENSURE( userData[0] == &persistentUserData );

	const b2Vec2* positions = b2ParticleSystem_GetPositionBuffer( systemId );
	ENSURE( positions != NULL );
	ENSURE_SMALL( positions[0].x - 1.0f, FLT_EPSILON );
	ENSURE_SMALL( positions[0].y, FLT_EPSILON );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );

	return 0;
}

static int ParticleContactsRemainValidAfterCompaction( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.1f;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.position = (b2Vec2){ 0.0f, 0.0f };
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 0 );
	particleDef.position = (b2Vec2){ 0.05f, 0.0f };
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 1 );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	ENSURE( b2ParticleSystem_GetContactCount( systemId ) > 0 );

	b2ParticleSystem_DestroyParticle( systemId, 0 );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 1 );
	ENSURE( b2ParticleSystem_GetContactCount( systemId ) == 0 );
	ENSURE( b2ParticleSystem_GetBodyContactCount( systemId ) == 0 );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );

	return 0;
}

static int ParticleContactEvents( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.1f;
	systemDef.enableParticleEvents = true;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.position = (b2Vec2){ 0.0f, 0.0f };
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 0 );
	particleDef.position = (b2Vec2){ 0.05f, 0.0f };
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 1 );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	b2ParticleContactEvents events = b2ParticleSystem_GetContactEvents( systemId );
	ENSURE( events.beginCount > 0 );
	ENSURE( events.beginEvents != NULL );
	ENSURE( events.endCount == 0 );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	events = b2ParticleSystem_GetContactEvents( systemId );
	ENSURE( events.beginCount == 0 );
	ENSURE( events.endCount == 0 );

	b2Vec2* positions = b2ParticleSystem_GetMutablePositionBuffer( systemId );
	ENSURE( positions != NULL );
	positions[1] = (b2Vec2){ 10.0f, 0.0f };

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	events = b2ParticleSystem_GetContactEvents( systemId );
	ENSURE( events.beginCount == 0 );
	ENSURE( events.endCount > 0 );
	ENSURE( events.endEvents != NULL );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	events = b2ParticleSystem_GetContactEvents( systemId );
	ENSURE( events.beginCount == 0 );
	ENSURE( events.endCount == 0 );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );

	return 0;
}

static int ParticleBodyContactEvents( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2BodyDef bodyDef = b2DefaultBodyDef();
	b2BodyId bodyId = b2CreateBody( worldId, &bodyDef );
	ENSURE( b2Body_IsValid( bodyId ) );
	b2Polygon box = b2MakeBox( 0.5f, 0.5f );
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2ShapeId shapeId = b2CreatePolygonShape( bodyId, &shapeDef, &box );
	ENSURE( b2Shape_IsValid( shapeId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.1f;
	systemDef.enableParticleBodyEvents = true;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.position = (b2Vec2){ 0.0f, 0.0f };
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 0 );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	b2ParticleBodyContactEvents events = b2ParticleSystem_GetBodyContactEvents( systemId );
	ENSURE( events.beginCount > 0 );
	ENSURE( events.beginEvents != NULL );
	ENSURE( events.endCount == 0 );
	ENSURE( B2_ID_EQUALS( events.beginEvents[0].shapeId, shapeId ) );

	b2Vec2* positions = b2ParticleSystem_GetMutablePositionBuffer( systemId );
	ENSURE( positions != NULL );
	positions[0] = (b2Vec2){ 5.0f, 0.0f };

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	events = b2ParticleSystem_GetBodyContactEvents( systemId );
	ENSURE( events.beginCount == 0 );
	ENSURE( events.endCount > 0 );
	ENSURE( events.endEvents != NULL );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );

	return 0;
}

static int ParticleGroupCreationFromShapes( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.05f;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2Circle circle = { { 0.0f, 0.0f }, 0.25f };
	b2ParticleGroupDef circleGroupDef = b2DefaultParticleGroupDef();
	circleGroupDef.flags = b2_tensileParticle;
	circleGroupDef.groupFlags = b2_solidParticleGroup;
	circleGroupDef.circle = &circle;
	circleGroupDef.stride = 0.08f;

	b2ParticleGroupId circleGroupId = b2ParticleSystem_CreateParticleGroup( systemId, &circleGroupDef );
	ENSURE( b2ParticleGroup_IsValid( circleGroupId ) );
	int circleCount = b2ParticleGroup_GetParticleCount( circleGroupId );
	ENSURE( circleCount > 0 );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == circleCount );
	ENSURE( b2ParticleGroup_GetMass( circleGroupId ) > 0.0f );
	int circleStartIndex = -1;
	int circleRangeCount = -1;
	b2ParticleGroup_GetParticleRange( circleGroupId, &circleStartIndex, &circleRangeCount );
	ENSURE( circleStartIndex >= 0 );
	ENSURE( circleRangeCount == circleCount );

	b2Polygon polygon = b2MakeBox( 0.25f, 0.15f );
	b2ParticleGroupDef polygonGroupDef = b2DefaultParticleGroupDef();
	polygonGroupDef.flags = b2_viscousParticle;
	polygonGroupDef.groupFlags = b2_rigidParticleGroup;
	polygonGroupDef.polygon = &polygon;
	polygonGroupDef.position = (b2Vec2){ 1.0f, 0.0f };
	polygonGroupDef.stride = 0.08f;

	b2ParticleGroupId polygonGroupId = b2ParticleSystem_CreateParticleGroup( systemId, &polygonGroupDef );
	ENSURE( b2ParticleGroup_IsValid( polygonGroupId ) );
	int polygonCount = b2ParticleGroup_GetParticleCount( polygonGroupId );
	ENSURE( polygonCount > 0 );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == circleCount + polygonCount );
	ENSURE( b2ParticleGroup_GetMass( polygonGroupId ) > 0.0f );
	int polygonStartIndex = -1;
	int polygonRangeCount = -1;
	b2ParticleGroup_GetParticleRange( polygonGroupId, &polygonStartIndex, &polygonRangeCount );
	ENSURE( polygonStartIndex >= circleStartIndex + circleRangeCount );
	ENSURE( polygonRangeCount == polygonCount );

	b2Vec2 polygonCenter = b2ParticleGroup_GetCenter( polygonGroupId );
	ENSURE_SMALL( polygonCenter.x - 1.0f, 0.1f );
	ENSURE_SMALL( polygonCenter.y, 0.1f );

	b2Counters counters = b2World_GetCounters( worldId );
	ENSURE( counters.particleSystemCount == 1 );
	ENSURE( counters.particleGroupCount == 2 );
	ENSURE( counters.particleCount == circleCount + polygonCount );

	b2ParticleSystemCounters systemCounters = b2ParticleSystem_GetCounters( systemId );
	ENSURE( systemCounters.particleCount == circleCount + polygonCount );
	ENSURE( systemCounters.groupCount == 2 );
	ENSURE( systemCounters.contactCount >= 0 );
	ENSURE( systemCounters.bodyContactCount == 0 );
	ENSURE( systemCounters.byteCount > 0 );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );

	return 0;
}

static int ParticleGroupBatchCreationPreservesParticleState( void )
{
	static int userData = 31;

	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.05f;
	systemDef.lifetimeGranularity = 0.25f;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2Vec2 positions[] = { { -0.35f, 0.0f }, { -0.25f, 0.0f } };
	b2Circle circle = { { 0.0f, 0.0f }, 0.16f };
	b2Segment segment = { { 0.25f, -0.1f }, { 0.25f, 0.15f } };

	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	groupDef.flags = b2_tensileParticle | b2_colorMixingParticle | b2_destructionListenerParticle;
	groupDef.groupFlags = b2_solidParticleGroup;
	groupDef.positionData = positions;
	groupDef.particleCount = ARRAY_COUNT( positions );
	groupDef.circle = &circle;
	groupDef.segment = &segment;
	groupDef.stride = 0.08f;
	groupDef.linearVelocity = (b2Vec2){ 1.25f, -0.5f };
	groupDef.angularVelocity = 0.75f;
	groupDef.color = (b2ParticleColor){ 30, 80, 220, 255 };
	groupDef.lifetime = 0.6f;
	groupDef.userData = &userData;

	b2ParticleGroupId groupId = b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
	ENSURE( b2ParticleGroup_IsValid( groupId ) );
	int groupCount = b2ParticleGroup_GetParticleCount( groupId );
	ENSURE( groupCount > ARRAY_COUNT( positions ) );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == groupCount );

	int startIndex = -1;
	int rangeCount = -1;
	b2ParticleGroup_GetParticleRange( groupId, &startIndex, &rangeCount );
	ENSURE( startIndex == 0 );
	ENSURE( rangeCount == groupCount );

	const b2Vec2* particlePositions = b2ParticleSystem_GetPositionBuffer( systemId );
	const b2Vec2* velocities = b2ParticleSystem_GetVelocityBuffer( systemId );
	const uint32_t* flags = b2ParticleSystem_GetFlagsBuffer( systemId );
	const b2ParticleColor* colors = b2ParticleSystem_GetColorBuffer( systemId );
	const float* lifetimes = b2ParticleSystem_GetLifetimeBuffer( systemId );
	void* const* userDataBuffer = b2ParticleSystem_GetUserDataBuffer( systemId );
	ENSURE( particlePositions != NULL );
	ENSURE( velocities != NULL );
	ENSURE( flags != NULL );
	ENSURE( colors != NULL );
	ENSURE( lifetimes != NULL );
	ENSURE( userDataBuffer != NULL );

	uint32_t expectedFlags = groupDef.flags;
	for ( int i = startIndex; i < startIndex + rangeCount; ++i )
	{
		ENSURE( flags[i] == expectedFlags );
		ENSURE( colors[i].r == groupDef.color.r );
		ENSURE( colors[i].g == groupDef.color.g );
		ENSURE( colors[i].b == groupDef.color.b );
		ENSURE( colors[i].a == groupDef.color.a );
		ENSURE_SMALL( lifetimes[i] - 0.75f, FLT_EPSILON );
		ENSURE( userDataBuffer[i] == &userData );

		b2Vec2 r = b2Sub( particlePositions[i], groupDef.position );
		b2Vec2 expectedVelocity = {
			groupDef.linearVelocity.x - groupDef.angularVelocity * r.y,
			groupDef.linearVelocity.y + groupDef.angularVelocity * r.x,
		};
		ENSURE_SMALL( velocities[i].x - expectedVelocity.x, FLT_EPSILON );
		ENSURE_SMALL( velocities[i].y - expectedVelocity.y, FLT_EPSILON );
	}

	b2ParticleSystem_DestroyParticle( systemId, 0 );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == groupCount - 1 );
	ENSURE( b2ParticleGroup_IsValid( groupId ) );
	ENSURE( b2ParticleGroup_GetParticleCount( groupId ) == groupCount - 1 );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );
	return 0;
}

static int ParticleGroupBatchAppendKeepsSingleGroupRange( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.05f;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2Vec2 positionsA[] = { { 0.0f, 0.0f }, { 0.06f, 0.0f } };
	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	groupDef.flags = b2_viscousParticle;
	groupDef.positionData = positionsA;
	groupDef.particleCount = ARRAY_COUNT( positionsA );
	b2ParticleGroupId groupId = b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
	ENSURE( b2ParticleGroup_IsValid( groupId ) );
	ENSURE( b2ParticleGroup_GetParticleCount( groupId ) == ARRAY_COUNT( positionsA ) );

	b2Circle circle = { { 0.35f, 0.0f }, 0.13f };
	groupDef = b2DefaultParticleGroupDef();
	groupDef.flags = b2_tensileParticle;
	groupDef.groupId = groupId;
	groupDef.circle = &circle;
	groupDef.stride = 0.08f;
	b2ParticleGroupId sameGroupId = b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
	ENSURE( B2_ID_EQUALS( sameGroupId, groupId ) );
	ENSURE( b2World_GetCounters( worldId ).particleGroupCount == 1 );

	int totalCount = b2ParticleSystem_GetParticleCount( systemId );
	ENSURE( totalCount > ARRAY_COUNT( positionsA ) );
	ENSURE( b2ParticleGroup_GetParticleCount( groupId ) == totalCount );

	int startIndex = -1;
	int rangeCount = -1;
	b2ParticleGroup_GetParticleRange( groupId, &startIndex, &rangeCount );
	ENSURE( startIndex == 0 );
	ENSURE( rangeCount == totalCount );

	const uint32_t* flags = b2ParticleSystem_GetFlagsBuffer( systemId );
	ENSURE( flags != NULL );
	int viscousCount = 0;
	int tensileCount = 0;
	for ( int i = 0; i < totalCount; ++i )
	{
		viscousCount += ( flags[i] & b2_viscousParticle ) != 0 ? 1 : 0;
		tensileCount += ( flags[i] & b2_tensileParticle ) != 0 ? 1 : 0;
	}
	ENSURE( viscousCount == ARRAY_COUNT( positionsA ) );
	ENSURE( tensileCount == totalCount - ARRAY_COUNT( positionsA ) );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );
	return 0;
}

static int ParticleSingleCreateIntoExistingGroupReorders( void )
{
	static int userData = 41;

	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.05f;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2Vec2 positions[] = { { 0.0f, 0.0f }, { 0.08f, 0.0f } };
	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	groupDef.flags = b2_tensileParticle;
	groupDef.positionData = positions;
	groupDef.particleCount = ARRAY_COUNT( positions );
	b2ParticleGroupId groupId = b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
	ENSURE( b2ParticleGroup_IsValid( groupId ) );

	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.flags = b2_viscousParticle;
	particleDef.position = (b2Vec2){ -0.1f, 0.0f };
	particleDef.velocity = (b2Vec2){ 2.0f, -3.0f };
	particleDef.color = (b2ParticleColor){ 7, 11, 13, 17 };
	particleDef.userData = &userData;
	particleDef.groupId = groupId;
	int particleIndex = b2ParticleSystem_CreateParticle( systemId, &particleDef );
	ENSURE( particleIndex >= 0 );
	ENSURE( b2ParticleGroup_GetParticleCount( groupId ) == 3 );

	int startIndex = -1;
	int rangeCount = -1;
	b2ParticleGroup_GetParticleRange( groupId, &startIndex, &rangeCount );
	ENSURE( startIndex == 0 );
	ENSURE( rangeCount == 3 );

	const uint32_t* flags = b2ParticleSystem_GetFlagsBuffer( systemId );
	const b2Vec2* velocities = b2ParticleSystem_GetVelocityBuffer( systemId );
	const b2ParticleColor* colors = b2ParticleSystem_GetColorBuffer( systemId );
	void* const* userDataBuffer = b2ParticleSystem_GetUserDataBuffer( systemId );
	ENSURE( flags != NULL );
	ENSURE( velocities != NULL );
	ENSURE( colors != NULL );
	ENSURE( userDataBuffer != NULL );
	ENSURE( flags[particleIndex] == b2_viscousParticle );
	ENSURE_SMALL( velocities[particleIndex].x - 2.0f, FLT_EPSILON );
	ENSURE_SMALL( velocities[particleIndex].y + 3.0f, FLT_EPSILON );
	ENSURE( colors[particleIndex].r == 7 );
	ENSURE( colors[particleIndex].g == 11 );
	ENSURE( colors[particleIndex].b == 13 );
	ENSURE( colors[particleIndex].a == 17 );
	ENSURE( userDataBuffer[particleIndex] == &userData );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );
	return 0;
}

static int ParticleGroupDestroyMiddleCompactsAndRemapsBuffers( void )
{
	static int userData[] = { 51, 52, 53, 54, 55 };

	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.05f;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2Vec2 positions[] = { { 0.0f, 0.0f }, { 0.06f, 0.0f }, { 0.12f, 0.0f }, { 0.18f, 0.0f }, { 0.24f, 0.0f } };
	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	groupDef.positionData = positions;
	groupDef.particleCount = ARRAY_COUNT( positions );
	b2ParticleGroupId groupId = b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
	ENSURE( b2ParticleGroup_IsValid( groupId ) );

	void** userDataBuffer = b2ParticleSystem_GetMutableUserDataBuffer( systemId );
	b2ParticleColor* colors = b2ParticleSystem_GetMutableColorBuffer( systemId );
	uint32_t* flags = b2ParticleSystem_GetMutableFlagsBuffer( systemId );
	ENSURE( userDataBuffer != NULL );
	ENSURE( colors != NULL );
	ENSURE( flags != NULL );

	for ( int i = 0; i < ARRAY_COUNT( positions ); ++i )
	{
		userDataBuffer[i] = userData + i;
		colors[i] = (b2ParticleColor){ (uint8_t)( 10 + i ), (uint8_t)( 20 + i ), (uint8_t)( 30 + i ), 255 };
		flags[i] = i == 2 ? b2_viscousParticle : b2_tensileParticle;
	}

	b2ParticleSystem_DestroyParticle( systemId, 2 );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 4 );
	ENSURE( b2ParticleGroup_IsValid( groupId ) );
	ENSURE( b2ParticleGroup_GetParticleCount( groupId ) == 4 );

	int startIndex = -1;
	int rangeCount = -1;
	b2ParticleGroup_GetParticleRange( groupId, &startIndex, &rangeCount );
	ENSURE( startIndex == 0 );
	ENSURE( rangeCount == 4 );

	userDataBuffer = b2ParticleSystem_GetMutableUserDataBuffer( systemId );
	colors = b2ParticleSystem_GetMutableColorBuffer( systemId );
	flags = b2ParticleSystem_GetMutableFlagsBuffer( systemId );
	ENSURE( userDataBuffer[0] == userData + 0 );
	ENSURE( userDataBuffer[1] == userData + 1 );
	ENSURE( userDataBuffer[2] == userData + 3 );
	ENSURE( userDataBuffer[3] == userData + 4 );
	ENSURE( colors[2].r == 13 );
	ENSURE( colors[3].r == 14 );
	ENSURE( flags[0] == b2_tensileParticle );
	ENSURE( flags[1] == b2_tensileParticle );
	ENSURE( flags[2] == b2_tensileParticle );
	ENSURE( flags[3] == b2_tensileParticle );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );
	return 0;
}

static int ParticleGroupMaxCountDestroyByAgeIsDeterministic( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.05f;
	systemDef.maxCount = 2;
	systemDef.destroyByAge = true;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2Vec2 positions[] = { { 0.0f, 0.0f }, { 0.1f, 0.0f }, { 0.2f, 0.0f } };
	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	groupDef.positionData = positions;
	groupDef.particleCount = ARRAY_COUNT( positions );
	b2ParticleGroupId groupId = b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
	ENSURE( b2ParticleGroup_IsValid( groupId ) );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 2 );
	ENSURE( b2ParticleGroup_GetParticleCount( groupId ) == 2 );

	const b2Vec2* particlePositions = b2ParticleSystem_GetPositionBuffer( systemId );
	ENSURE( particlePositions != NULL );
	ENSURE_SMALL( particlePositions[0].x - 0.1f, FLT_EPSILON );
	ENSURE_SMALL( particlePositions[1].x - 0.2f, FLT_EPSILON );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );
	return 0;
}

static int ParticleShapeGroupPairsTriadsAfterBatchCreation( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.05f;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2Circle circle = { { 0.0f, 0.0f }, 0.16f };
	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	groupDef.flags = b2_springParticle | b2_elasticParticle | b2_barrierParticle;
	groupDef.circle = &circle;
	groupDef.stride = 0.08f;
	groupDef.strength = 0.8f;
	b2ParticleGroupId groupId = b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
	ENSURE( b2ParticleGroup_IsValid( groupId ) );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) >= 4 );
	ENSURE( b2ParticleSystem_GetPairCount( systemId ) > 0 );
	ENSURE( b2ParticleSystem_GetPairs( systemId ) != NULL );
	ENSURE( b2ParticleSystem_GetTriadCount( systemId ) > 0 );
	ENSURE( b2ParticleSystem_GetTriads( systemId ) != NULL );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	ENSURE( b2ParticleSystem_GetPairCount( systemId ) > 0 );
	ENSURE( b2ParticleSystem_GetTriadCount( systemId ) > 0 );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );
	return 0;
}

static int ParticleColorMixingMatchesLiquidFunPairStep( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.1f;
	systemDef.gravityScale = 0.0f;
	systemDef.pressureStrength = 0.0f;
	systemDef.dampingStrength = 0.0f;
	systemDef.colorMixingStrength = 0.5f;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.flags = b2_colorMixingParticle;
	particleDef.position = (b2Vec2){ 0.0f, 0.0f };
	particleDef.color = (b2ParticleColor){ 255, 0, 0, 255 };
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 0 );
	particleDef.position = (b2Vec2){ 0.1f, 0.0f };
	particleDef.color = (b2ParticleColor){ 0, 0, 255, 255 };
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 1 );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	ENSURE( b2ParticleSystem_GetContactCount( systemId ) == 1 );

	const b2ParticleColor* colors = b2ParticleSystem_GetColorBuffer( systemId );
	ENSURE( colors != NULL );
	ENSURE( colors[0].r == 191 );
	ENSURE( colors[0].g == 0 );
	ENSURE( colors[0].b == 63 );
	ENSURE( colors[0].a == 255 );
	ENSURE( colors[1].r == 64 );
	ENSURE( colors[1].g == 0 );
	ENSURE( colors[1].b == 192 );
	ENSURE( colors[1].a == 255 );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );
	return 0;
}

static int ParticleBatchGroupBodyContactsEmitEvents( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2BodyDef bodyDef = b2DefaultBodyDef();
	b2BodyId bodyId = b2CreateBody( worldId, &bodyDef );
	ENSURE( b2Body_IsValid( bodyId ) );

	b2Polygon box = b2MakeBox( 1.0f, 1.0f );
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	b2ShapeId shapeId = b2CreatePolygonShape( bodyId, &shapeDef, &box );
	ENSURE( b2Shape_IsValid( shapeId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.05f;
	systemDef.gravityScale = 0.0f;
	systemDef.enableParticleBodyEvents = true;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2Circle circle = { { 0.0f, 0.0f }, 0.18f };
	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	groupDef.flags = b2_bodyContactListenerParticle;
	groupDef.circle = &circle;
	groupDef.stride = 0.08f;
	b2ParticleGroupId groupId = b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
	ENSURE( b2ParticleGroup_IsValid( groupId ) );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	b2ParticleBodyContactEvents events = b2ParticleSystem_GetBodyContactEvents( systemId );
	ENSURE( b2ParticleSystem_GetBodyContactCount( systemId ) == b2ParticleSystem_GetParticleCount( systemId ) );
	ENSURE( events.beginCount == b2ParticleSystem_GetParticleCount( systemId ) );
	ENSURE( events.endCount == 0 );
	ENSURE( B2_ID_EQUALS( events.beginEvents[0].shapeId, shapeId ) );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );
	return 0;
}

static int ParticleGroupJoinAndSplitRanges( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.1f;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2Vec2 groupAPositions[] = { { 0.0f, 0.0f }, { 0.05f, 0.0f } };
	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	groupDef.positionData = groupAPositions;
	groupDef.particleCount = 2;
	b2ParticleGroupId groupA = b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
	ENSURE( b2ParticleGroup_IsValid( groupA ) );

	b2Vec2 groupBPositions[] = { { 0.1f, 0.0f }, { 0.15f, 0.0f } };
	groupDef = b2DefaultParticleGroupDef();
	groupDef.positionData = groupBPositions;
	groupDef.particleCount = 2;
	b2ParticleGroupId groupB = b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
	ENSURE( b2ParticleGroup_IsValid( groupB ) );

	b2ParticleGroup_Join( groupA, groupB );
	ENSURE( b2ParticleGroup_IsValid( groupA ) );
	ENSURE( b2ParticleGroup_IsValid( groupB ) == false );
	ENSURE( b2ParticleGroup_GetParticleCount( groupA ) == 4 );
	int startIndex = -1;
	int rangeCount = -1;
	b2ParticleGroup_GetParticleRange( groupA, &startIndex, &rangeCount );
	ENSURE( startIndex == 0 );
	ENSURE( rangeCount == 4 );

	b2Vec2 splitPositions[] = { { 2.0f, 0.0f }, { 2.05f, 0.0f }, { 4.0f, 0.0f }, { 4.05f, 0.0f } };
	groupDef = b2DefaultParticleGroupDef();
	groupDef.positionData = splitPositions;
	groupDef.particleCount = 4;
	b2ParticleGroupId splitGroup = b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
	ENSURE( b2ParticleGroup_IsValid( splitGroup ) );
	ENSURE( b2ParticleGroup_GetParticleCount( splitGroup ) == 4 );

	b2ParticleGroup_Split( splitGroup );
	ENSURE( b2ParticleGroup_IsValid( splitGroup ) );
	ENSURE( b2ParticleGroup_GetParticleCount( splitGroup ) == 2 );
	b2Counters counters = b2World_GetCounters( worldId );
	ENSURE( counters.particleGroupCount == 3 );

	b2ParticleGroup_GetParticleRange( splitGroup, &startIndex, &rangeCount );
	ENSURE( startIndex >= 0 );
	ENSURE( rangeCount == 2 );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );

	return 0;
}

static int ParticleCounters( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2Counters counters = b2World_GetCounters( worldId );
	ENSURE( counters.particleSystemCount == 0 );
	ENSURE( counters.particleGroupCount == 0 );
	ENSURE( counters.particleCount == 0 );
	ENSURE( counters.particleContactCount == 0 );
	ENSURE( counters.particleBodyContactCount == 0 );
	ENSURE( counters.particleTaskCount == 0 );
	ENSURE( counters.particleByteCount == 0 );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.05f;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.position = (b2Vec2){ -1.0f, 0.0f };
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 0 );
	particleDef.position = (b2Vec2){ 1.0f, 0.0f };
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 1 );

	counters = b2World_GetCounters( worldId );
	ENSURE( counters.particleSystemCount == 1 );
	ENSURE( counters.particleGroupCount == 0 );
	ENSURE( counters.particleCount == 2 );
	ENSURE( counters.particleContactCount == 0 );
	ENSURE( counters.particleBodyContactCount == 0 );
	ENSURE( counters.particleTaskCount >= 0 );
	ENSURE( counters.particleByteCount > 0 );

	b2ParticleSystemCounters systemCounters = b2ParticleSystem_GetCounters( systemId );
	ENSURE( systemCounters.particleCount == 2 );
	ENSURE( systemCounters.groupCount == 0 );
	ENSURE( systemCounters.contactCount == 0 );
	ENSURE( systemCounters.bodyContactCount == 0 );
	ENSURE( systemCounters.byteCount > 0 );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );

	counters = b2World_GetCounters( worldId );
	ENSURE( counters.particleSystemCount == 1 );
	ENSURE( counters.particleGroupCount == 0 );
	ENSURE( counters.particleCount == 2 );
	ENSURE( counters.particleContactCount >= 0 );
	ENSURE( counters.particleBodyContactCount == 0 );
	ENSURE( counters.particleTaskCount >= 0 );
	ENSURE( counters.particleByteCount > 0 );

	systemCounters = b2ParticleSystem_GetCounters( systemId );
	ENSURE( systemCounters.particleCount == 2 );
	ENSURE( systemCounters.groupCount == 0 );
	ENSURE( systemCounters.contactCount >= 0 );
	ENSURE( systemCounters.bodyContactCount == 0 );
	ENSURE( systemCounters.byteCount > 0 );

	b2DestroyParticleSystem( systemId );

	counters = b2World_GetCounters( worldId );
	ENSURE( counters.particleSystemCount == 0 );
	ENSURE( counters.particleGroupCount == 0 );
	ENSURE( counters.particleCount == 0 );
	ENSURE( counters.particleContactCount == 0 );
	ENSURE( counters.particleBodyContactCount == 0 );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );

	return 0;
}

static bool RejectAllParticleContacts( b2ParticleSystemId systemId, int indexA, int indexB, void* context )
{
	(void)systemId;
	(void)indexA;
	(void)indexB;
	(void)context;
	return false;
}

static bool RejectAllBodyContacts( b2ParticleSystemId systemId, b2ShapeId shapeId, int particleIndex, void* context )
{
	(void)systemId;
	(void)shapeId;
	(void)particleIndex;
	(void)context;
	return false;
}

static bool KeepProtectedParticle( b2ParticleSystemId systemId, int particleIndex, void* context )
{
	(void)systemId;
	int protectedIndex = *(int*)context;
	return particleIndex != protectedIndex;
}

static int ParticlePairsTriadsAndGroupAccessors( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.1f;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2Vec2 positions[] = { { 0.0f, 0.0f }, { 0.1f, 0.0f }, { 0.0f, 0.1f } };
	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	groupDef.flags = b2_springParticle | b2_elasticParticle | b2_barrierParticle;
	groupDef.positionData = positions;
	groupDef.particleCount = 3;
	groupDef.strength = 0.75f;

	b2ParticleGroupId groupId = b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
	ENSURE( b2ParticleGroup_IsValid( groupId ) );
	ENSURE( b2ParticleSystem_GetPairCount( systemId ) > 0 );
	ENSURE( b2ParticleSystem_GetPairs( systemId ) != NULL );
	ENSURE( b2ParticleSystem_GetTriadCount( systemId ) > 0 );
	ENSURE( b2ParticleSystem_GetTriads( systemId ) != NULL );
	ENSURE( b2ParticleGroup_GetInertia( groupId ) >= 0.0f );
	ENSURE_SMALL( b2ParticleGroup_GetAngularVelocity( groupId ), FLT_EPSILON );
	b2Transform transform = b2ParticleGroup_GetTransform( groupId );
	ENSURE_SMALL( transform.q.c - 1.0f, FLT_EPSILON );
	ENSURE_SMALL( b2ParticleGroup_GetAngle( groupId ), FLT_EPSILON );
	b2Vec2 pointVelocity = b2ParticleGroup_GetLinearVelocityFromWorldPoint( groupId, positions[0] );
	ENSURE_SMALL( pointVelocity.x, FLT_EPSILON );
	ENSURE_SMALL( pointVelocity.y, FLT_EPSILON );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	ENSURE( b2ParticleSystem_GetPairCount( systemId ) > 0 );
	ENSURE( b2ParticleSystem_GetTriadCount( systemId ) > 0 );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );
	return 0;
}

static int ParticleDestructionEventsAndQueryFilter( void )
{
	static int userDataA = 31;
	static int userDataB = 37;

	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.1f;
	systemDef.enableParticleEvents = true;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.flags = b2_destructionListenerParticle;
	particleDef.position = (b2Vec2){ 0.0f, 0.0f };
	particleDef.userData = &userDataA;
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 0 );
	particleDef.position = (b2Vec2){ 0.5f, 0.0f };
	particleDef.userData = &userDataB;
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 1 );

	int protectedIndex = 1;
	b2ParticleSystem_SetQueryFilter( systemId, KeepProtectedParticle, &protectedIndex );
	b2Segment segment = { { -0.2f, 0.0f }, { 0.7f, 0.0f } };
	ENSURE( b2ParticleSystem_DestroyParticlesInSegment( systemId, &segment, b2Transform_identity ) == 1 );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 1 );
	void* const* userData = b2ParticleSystem_GetUserDataBuffer( systemId );
	ENSURE( userData != NULL );
	ENSURE( userData[0] == &userDataB );

	b2ParticleDestructionEvents events = b2ParticleSystem_GetDestructionEvents( systemId );
	ENSURE( events.count == 1 );
	ENSURE( events.events != NULL );
	ENSURE( events.events[0].userData == &userDataA );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );
	return 0;
}

static int ParticleContactFilters( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.1f;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );
	b2ParticleSystem_SetContactFilter( systemId, RejectAllParticleContacts, NULL );
	b2ParticleSystem_SetBodyContactFilter( systemId, RejectAllBodyContacts, NULL );

	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.flags = b2_particleContactFilterParticle;
	particleDef.position = (b2Vec2){ 0.0f, 0.0f };
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 0 );
	particleDef.position = (b2Vec2){ 0.05f, 0.0f };
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 1 );

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.position = (b2Vec2){ 2.0f, 0.0f };
	b2BodyId bodyId = b2CreateBody( worldId, &bodyDef );
	ENSURE( b2Body_IsValid( bodyId ) );
	b2Polygon box = b2MakeBox( 0.5f, 0.5f );
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	ENSURE( B2_IS_NON_NULL( b2CreatePolygonShape( bodyId, &shapeDef, &box ) ) );

	particleDef.flags = b2_bodyContactFilterParticle;
	particleDef.position = (b2Vec2){ 2.0f, 0.0f };
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 2 );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	ENSURE( b2ParticleSystem_GetContactCount( systemId ) == 0 );
	ENSURE( b2ParticleSystem_GetBodyContactCount( systemId ) == 0 );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );
	return 0;
}

static int ParticleListenerFlagsEmitEvents( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.1f;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.flags = b2_particleContactListenerParticle;
	particleDef.position = (b2Vec2){ 0.0f, 0.0f };
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 0 );
	particleDef.position = (b2Vec2){ 0.05f, 0.0f };
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 1 );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	ENSURE( b2ParticleSystem_GetContactEvents( systemId ).beginCount > 0 );

	b2DestroyWorld( worldId );

	worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );
	b2BodyDef bodyDef = b2DefaultBodyDef();
	b2BodyId bodyId = b2CreateBody( worldId, &bodyDef );
	ENSURE( b2Body_IsValid( bodyId ) );
	b2Polygon box = b2MakeBox( 0.5f, 0.5f );
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	ENSURE( B2_IS_NON_NULL( b2CreatePolygonShape( bodyId, &shapeDef, &box ) ) );

	systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );
	particleDef = b2DefaultParticleDef();
	particleDef.flags = b2_bodyContactListenerParticle;
	particleDef.position = (b2Vec2){ 0.0f, 0.0f };
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 0 );

	b2World_Step( worldId, 1.0f / 60.0f, 1 );
	ENSURE( b2ParticleSystem_GetBodyContactEvents( systemId ).beginCount > 0 );

	b2DestroyWorld( worldId );
	return 0;
}

static int ParticleLifetimeGranularityAndOldestAdmission( void )
{
	static int oldestUserData = 11;
	static int middleUserData = 13;
	static int newestUserData = 17;

	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.1f;
	systemDef.maxCount = 2;
	systemDef.lifetimeGranularity = 1.0f;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2ParticleDef particleDef = b2DefaultParticleDef();
	particleDef.lifetime = 10.0f;
	particleDef.userData = &oldestUserData;
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 0 );
	b2World_Step( worldId, 0.25f, 1 );

	particleDef.userData = &middleUserData;
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 1 );
	b2World_Step( worldId, 0.25f, 1 );

	particleDef.userData = &newestUserData;
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 1 );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 2 );
	void* const* userData = b2ParticleSystem_GetUserDataBuffer( systemId );
	ENSURE( userData != NULL );
	ENSURE( userData[0] == &middleUserData );
	ENSURE( userData[1] == &newestUserData );

	b2ParticleSystem_DestroyParticle( systemId, 0 );
	b2ParticleSystem_DestroyParticle( systemId, 0 );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 0 );

	particleDef.lifetime = 0.25f;
	particleDef.userData = NULL;
	ENSURE( b2ParticleSystem_CreateParticle( systemId, &particleDef ) == 0 );
	ENSURE_SMALL( b2ParticleSystem_GetParticleLifetime( systemId, 0 ) - 1.0f, FLT_EPSILON );
	b2World_Step( worldId, 0.5f, 1 );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 1 );
	b2World_Step( worldId, 0.5f, 1 );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 0 );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );
	return 0;
}

static int ParticleGroupCreationRollsBackOnAdmissionFailure( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.1f;
	systemDef.maxCount = 1;
	systemDef.destroyByAge = false;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2Vec2 positions[] = { { 0.0f, 0.0f }, { 0.05f, 0.0f } };
	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	groupDef.positionData = positions;
	groupDef.particleCount = 2;
	b2ParticleGroupId groupId = b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
	ENSURE( B2_IS_NULL( groupId ) );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 0 );
	ENSURE( b2World_GetCounters( worldId ).particleGroupCount == 0 );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );
	return 0;
}

static int ParticleGroupShapeCreationRollsBackOnAdmissionFailure( void )
{
	b2WorldId worldId = CreateParticleTestWorld();
	ENSURE( b2World_IsValid( worldId ) );

	b2ParticleSystemDef systemDef = b2DefaultParticleSystemDef();
	systemDef.radius = 0.05f;
	systemDef.maxCount = 2;
	systemDef.destroyByAge = false;
	b2ParticleSystemId systemId = b2CreateParticleSystem( worldId, &systemDef );
	ENSURE( b2ParticleSystem_IsValid( systemId ) );

	b2Circle circle = { { 0.0f, 0.0f }, 0.2f };
	b2ParticleGroupDef groupDef = b2DefaultParticleGroupDef();
	groupDef.circle = &circle;
	groupDef.stride = 0.07f;
	b2ParticleGroupId groupId = b2ParticleSystem_CreateParticleGroup( systemId, &groupDef );
	ENSURE( B2_IS_NULL( groupId ) );
	ENSURE( b2ParticleSystem_GetParticleCount( systemId ) == 0 );
	ENSURE( b2World_GetCounters( worldId ).particleGroupCount == 0 );

	b2DestroyWorld( worldId );
	ENSURE( b2World_IsValid( worldId ) == false );
	return 0;
}

int ParticleTest( void )
{
	RUN_SUBTEST( ParticleFlagValues );
	RUN_SUBTEST( ParticleDefaultDefs );
	RUN_SUBTEST( ParticleSystemCreateDestroy );
	RUN_SUBTEST( ParticleCreateAndBuffers );
	RUN_SUBTEST( ParticleMaxCount );
	RUN_SUBTEST( ParticleLifetimesAfterStepping );
	RUN_SUBTEST( ParticleContactsRemainValidAfterCompaction );
	RUN_SUBTEST( ParticleContactEvents );
	RUN_SUBTEST( ParticleBodyContactEvents );
	RUN_SUBTEST( ParticleGroupCreationFromShapes );
	RUN_SUBTEST( ParticleGroupBatchCreationPreservesParticleState );
	RUN_SUBTEST( ParticleGroupBatchAppendKeepsSingleGroupRange );
	RUN_SUBTEST( ParticleSingleCreateIntoExistingGroupReorders );
	RUN_SUBTEST( ParticleGroupDestroyMiddleCompactsAndRemapsBuffers );
	RUN_SUBTEST( ParticleGroupMaxCountDestroyByAgeIsDeterministic );
	RUN_SUBTEST( ParticleShapeGroupPairsTriadsAfterBatchCreation );
	RUN_SUBTEST( ParticleColorMixingMatchesLiquidFunPairStep );
	RUN_SUBTEST( ParticleBatchGroupBodyContactsEmitEvents );
	RUN_SUBTEST( ParticleGroupJoinAndSplitRanges );
	RUN_SUBTEST( ParticleCounters );
	RUN_SUBTEST( ParticlePairsTriadsAndGroupAccessors );
	RUN_SUBTEST( ParticleDestructionEventsAndQueryFilter );
	RUN_SUBTEST( ParticleContactFilters );
	RUN_SUBTEST( ParticleListenerFlagsEmitEvents );
	RUN_SUBTEST( ParticleLifetimeGranularityAndOldestAdmission );
	RUN_SUBTEST( ParticleGroupCreationRollsBackOnAdmissionFailure );
	RUN_SUBTEST( ParticleGroupShapeCreationRollsBackOnAdmissionFailure );

	return 0;
}
