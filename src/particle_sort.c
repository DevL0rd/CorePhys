// SPDX-FileCopyrightText: 2026 DevL0rd
// SPDX-License-Identifier: MIT

#include "particle_sort.h"

#include "core.h"

#include <string.h>

enum
{
	b2_radixBits = 8,
	b2_radixSize = 1 << b2_radixBits,
	b2_radixMask = b2_radixSize - 1,
	b2_u64ByteCount = 8,
};

static uint64_t b2ReadParticleSortKey( const unsigned char* record, int keyOffset )
{
	uint64_t key;
	memcpy( &key, record + keyOffset, sizeof( key ) );
	return key;
}

static void b2ValidateParticleSortInputs( const void* records, int count, int recordSize, int keyOffset )
{
	B2_ASSERT( count >= 0 );
	B2_ASSERT( recordSize >= (int)sizeof( uint64_t ) );
	B2_ASSERT( keyOffset >= 0 );
	B2_ASSERT( keyOffset <= recordSize - (int)sizeof( uint64_t ) );
	B2_ASSERT( count == 0 || records != NULL );
	B2_ASSERT( recordSize == 0 || count <= INT32_MAX / recordSize );

	(void)records;
	(void)count;
	(void)recordSize;
	(void)keyOffset;
}

int b2GetParticleSortScratchByteCount( int count, int recordSize )
{
	B2_ASSERT( count >= 0 );
	B2_ASSERT( recordSize > 0 );
	B2_ASSERT( count <= INT32_MAX / recordSize );

	if ( count <= 1 )
	{
		return 0;
	}

	return count * recordSize;
}

bool b2ParticleRecordsAreSortedByKey( const void* records, int count, int recordSize, int keyOffset )
{
	b2ValidateParticleSortInputs( records, count, recordSize, keyOffset );

	if ( count <= 1 )
	{
		return true;
	}

	const unsigned char* bytes = records;
	uint64_t previousKey = b2ReadParticleSortKey( bytes, keyOffset );

	for ( int i = 1; i < count; ++i )
	{
		const unsigned char* record = bytes + i * recordSize;
		uint64_t key = b2ReadParticleSortKey( record, keyOffset );

		if ( key < previousKey )
		{
			return false;
		}

		previousKey = key;
	}

	return true;
}

static void b2RadixSortParticleRecordsByKey( void* records, void* scratch, int count, int recordSize, int keyOffset )
{
	B2_ASSERT( scratch != NULL );

	unsigned char* source = records;
	unsigned char* target = scratch;
	unsigned char* original = records;

	for ( int pass = 0; pass < b2_u64ByteCount; ++pass )
	{
		int counts[b2_radixSize] = { 0 };
		int offsets[b2_radixSize];
		int usedBucketCount = 0;
		int shift = pass * b2_radixBits;

		for ( int i = 0; i < count; ++i )
		{
			const unsigned char* record = source + i * recordSize;
			uint64_t key = b2ReadParticleSortKey( record, keyOffset );
			int bucket = (int)( ( key >> shift ) & b2_radixMask );

			if ( counts[bucket] == 0 )
			{
				usedBucketCount += 1;
			}

			counts[bucket] += 1;
		}

		if ( usedBucketCount <= 1 )
		{
			continue;
		}

		int offset = 0;
		for ( int i = 0; i < b2_radixSize; ++i )
		{
			offsets[i] = offset;
			offset += counts[i];
		}

		for ( int i = 0; i < count; ++i )
		{
			const unsigned char* record = source + i * recordSize;
			uint64_t key = b2ReadParticleSortKey( record, keyOffset );
			int bucket = (int)( ( key >> shift ) & b2_radixMask );
			int targetIndex = offsets[bucket]++;
			memcpy( target + targetIndex * recordSize, record, recordSize );
		}

		B2_SWAP( source, target );
	}

	if ( source != original )
	{
		memcpy( original, source, count * recordSize );
	}
}

void b2SortParticleRecordsByKeyWithScratch( void* records, void* scratch, int count, int recordSize, int keyOffset )
{
	b2ValidateParticleSortInputs( records, count, recordSize, keyOffset );

	if ( count <= 1 || b2ParticleRecordsAreSortedByKey( records, count, recordSize, keyOffset ) )
	{
		return;
	}

	b2RadixSortParticleRecordsByKey( records, scratch, count, recordSize, keyOffset );
}

void b2SortParticleRecordsByKey( void* records, int count, int recordSize, int keyOffset )
{
	b2ValidateParticleSortInputs( records, count, recordSize, keyOffset );

	if ( count <= 1 || b2ParticleRecordsAreSortedByKey( records, count, recordSize, keyOffset ) )
	{
		return;
	}

	int scratchByteCount = b2GetParticleSortScratchByteCount( count, recordSize );
	void* scratch = b2Alloc( scratchByteCount );
	b2RadixSortParticleRecordsByKey( records, scratch, count, recordSize, keyOffset );
	b2Free( scratch, scratchByteCount );
}

void b2SortParticleKeyRecords( b2ParticleSortKeyRecord* records, int count )
{
	b2SortParticleRecordsByKey( records, count, (int)sizeof( b2ParticleSortKeyRecord ), 0 );
}
