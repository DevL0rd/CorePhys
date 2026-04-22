// SPDX-FileCopyrightText: 2026 DevL0rd
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stdint.h>

// Internal deterministic sort helpers for particle subsystem data.
// Records are sorted by a uint64_t key stored at keyOffset bytes from the start of each record.

typedef struct b2ParticleSortKeyRecord
{
	uint64_t key;
	int index;
} b2ParticleSortKeyRecord;

int b2GetParticleSortScratchByteCount( int count, int recordSize );

bool b2ParticleRecordsAreSortedByKey( const void* records, int count, int recordSize, int keyOffset );

void b2SortParticleRecordsByKey( void* records, int count, int recordSize, int keyOffset );

void b2SortParticleRecordsByKeyWithScratch( void* records, void* scratch, int count, int recordSize, int keyOffset );

void b2SortParticleKeyRecords( b2ParticleSortKeyRecord* records, int count );
