// SPDX-FileCopyrightText: 2026 DevL0rd
// SPDX-License-Identifier: MIT

#include "particle_scenarios.h"
#include "sample.h"

#include "corephys/particle.h"

class ParticleScenarioSample : public Sample
{
public:
	ParticleScenarioSample( SampleContext* context, ParticleScenarioType type )
		: Sample( context )
	{
		m_type = type;
		m_camera->center = { 0.0f, 2.0f };
		m_camera->zoom = 0.1f;
		m_scenario = CreateParticleScenario( m_worldId, type );
	}

	void Step() override
	{
		float timeStep = m_context->hertz > 0.0f ? 1.0f / m_context->hertz : 0.0f;
		bool shouldStep = timeStep > 0.0f && ( m_context->pause == false || m_context->singleStep );
		if ( shouldStep )
		{
			StepParticleScenario( m_worldId, &m_scenario, timeStep );
		}

		Sample::Step();

		if ( b2ParticleSystem_IsValid( m_scenario.systemId ) )
		{
			b2ParticleSystemCounters counters = b2ParticleSystem_GetCounters( m_scenario.systemId );
			DrawTextLine( "%s", GetParticleScenarioName( m_type ) );
			DrawTextLine( "particles/groups/contacts/body = %d/%d/%d/%d", counters.particleCount, counters.groupCount,
						  counters.contactCount, counters.bodyContactCount );
			DrawTextLine( "pairs/triads/destruction = %d/%d/%d", counters.pairCount, counters.triadCount,
						  counters.destructionEventCount );
		}
	}

private:
	ParticleScenarioType m_type = ParticleScenario_DamBreak;
	ParticleScenario m_scenario = {};
};

#define DEFINE_PARTICLE_SAMPLE( className, scenarioType )                                                                        \
	class className : public ParticleScenarioSample                                                                               \
	{                                                                                                                            \
	public:                                                                                                                      \
		explicit className( SampleContext* context )                                                                              \
			: ParticleScenarioSample( context, scenarioType )                                                                     \
		{                                                                                                                        \
		}                                                                                                                        \
		static Sample* Create( SampleContext* context )                                                                           \
		{                                                                                                                        \
			return new className( context );                                                                                      \
		}                                                                                                                        \
	}

DEFINE_PARTICLE_SAMPLE( ParticleDamBreak, ParticleScenario_DamBreak );
DEFINE_PARTICLE_SAMPLE( ParticleFaucet, ParticleScenario_Faucet );
DEFINE_PARTICLE_SAMPLE( ParticleWaveMachine, ParticleScenario_WaveMachine );
DEFINE_PARTICLE_SAMPLE( ParticleParticles, ParticleScenario_Particles );
DEFINE_PARTICLE_SAMPLE( ParticleRigidGroups, ParticleScenario_RigidGroups );
DEFINE_PARTICLE_SAMPLE( ParticleElasticGroups, ParticleScenario_ElasticGroups );
DEFINE_PARTICLE_SAMPLE( ParticleViscousFluid, ParticleScenario_ViscousFluid );
DEFINE_PARTICLE_SAMPLE( ParticlePowder, ParticleScenario_Powder );
DEFINE_PARTICLE_SAMPLE( ParticleBarrier, ParticleScenario_Barrier );

static int sampleParticleDamBreak = RegisterSample( "Particles", "Dam Break", ParticleDamBreak::Create );
static int sampleParticleFaucet = RegisterSample( "Particles", "Faucet", ParticleFaucet::Create );
static int sampleParticleWaveMachine = RegisterSample( "Particles", "Wave Machine", ParticleWaveMachine::Create );
static int sampleParticleParticles = RegisterSample( "Particles", "Particles", ParticleParticles::Create );
static int sampleParticleRigidGroups = RegisterSample( "Particles", "Rigid Groups", ParticleRigidGroups::Create );
static int sampleParticleElasticGroups = RegisterSample( "Particles", "Elastic Groups", ParticleElasticGroups::Create );
static int sampleParticleViscousFluid = RegisterSample( "Particles", "Viscous Fluid", ParticleViscousFluid::Create );
static int sampleParticlePowder = RegisterSample( "Particles", "Powder", ParticlePowder::Create );
static int sampleParticleBarrier = RegisterSample( "Particles", "Barrier", ParticleBarrier::Create );
