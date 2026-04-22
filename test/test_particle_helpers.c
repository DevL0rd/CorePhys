// SPDX-FileCopyrightText: 2026 DevL0rd
// SPDX-License-Identifier: MIT

#include "test_macros.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if defined( __has_include )
	#if __has_include( "particle_sort.h" )
		#include "particle_sort.h"
		#define B2_HAS_PARTICLE_SORT_HELPERS 1
	#else
		#define B2_HAS_PARTICLE_SORT_HELPERS 0
	#endif
	#if __has_include( "particle_scratch.h" )
		#include "particle_scratch.h"
		#define B2_HAS_PARTICLE_SCRATCH_HELPERS 1
	#else
		#define B2_HAS_PARTICLE_SCRATCH_HELPERS 0
	#endif
#else
	#include "particle_sort.h"
	#include "particle_scratch.h"
	#define B2_HAS_PARTICLE_SORT_HELPERS 1
	#define B2_HAS_PARTICLE_SCRATCH_HELPERS 1
#endif

#if B2_HAS_PARTICLE_SORT_HELPERS

typedef struct ParticleSortTestRecord
{
	int payload;
	uint64_t key;
	int stableIndex;
} ParticleSortTestRecord;

static int ParticleSortOrdersRecordsByOffsetKey( void )
{
	ParticleSortTestRecord records[] = {
		{ 11, 5, 0 },
		{ 12, 1, 1 },
		{ 13, 3, 2 },
		{ 14, 1, 3 },
		{ 15, 0, 4 },
		{ 16, UINT64_C( 0x100000000 ), 5 },
		{ 17, 5, 6 },
	};

	const int count = ARRAY_COUNT( records );
	const int recordSize = (int)sizeof( records[0] );
	const int keyOffset = (int)offsetof( ParticleSortTestRecord, key );

	ENSURE( b2GetParticleSortScratchByteCount( 0, recordSize ) == 0 );
	ENSURE( b2GetParticleSortScratchByteCount( 1, recordSize ) == 0 );
	ENSURE( b2GetParticleSortScratchByteCount( count, recordSize ) == count * recordSize );
	ENSURE( b2ParticleRecordsAreSortedByKey( records, count, recordSize, keyOffset ) == false );

	unsigned char scratch[sizeof( records )];
	b2SortParticleRecordsByKeyWithScratch( records, scratch, count, recordSize, keyOffset );

	ENSURE( b2ParticleRecordsAreSortedByKey( records, count, recordSize, keyOffset ) );
	ENSURE( records[0].key == 0 && records[0].stableIndex == 4 );
	ENSURE( records[1].key == 1 && records[1].stableIndex == 1 );
	ENSURE( records[2].key == 1 && records[2].stableIndex == 3 );
	ENSURE( records[3].key == 3 && records[3].stableIndex == 2 );
	ENSURE( records[4].key == 5 && records[4].stableIndex == 0 );
	ENSURE( records[5].key == 5 && records[5].stableIndex == 6 );
	ENSURE( records[6].key == UINT64_C( 0x100000000 ) && records[6].stableIndex == 5 );

	return 0;
}

static int ParticleSortKeyRecords( void )
{
	b2ParticleSortKeyRecord records[] = {
		{ 7, 70 },
		{ 2, 20 },
		{ 2, 21 },
		{ 9, 90 },
		{ 0, 0 },
	};

	b2SortParticleKeyRecords( records, ARRAY_COUNT( records ) );

	ENSURE( records[0].key == 0 && records[0].index == 0 );
	ENSURE( records[1].key == 2 && records[1].index == 20 );
	ENSURE( records[2].key == 2 && records[2].index == 21 );
	ENSURE( records[3].key == 7 && records[3].index == 70 );
	ENSURE( records[4].key == 9 && records[4].index == 90 );

	return 0;
}

#endif

#if B2_HAS_PARTICLE_SCRATCH_HELPERS

static int ParticleScratchBufferKeepsHighWaterCapacity( void )
{
	b2ParticleScratchBuffer buffer;
	b2ParticleScratchBuffer_Init( &buffer );

	b2ParticleScratchBuffer_Reserve( &buffer, 13 );
	ENSURE( buffer.byteCapacity >= 13 );

	int initialCapacity = buffer.byteCapacity;
	int* ints = b2ParticleScratchBuffer_AllocArray( &buffer, 4, int );
	ENSURE( ints != NULL );
	for ( int i = 0; i < 4; ++i )
	{
		ints[i] = i * 3;
	}

	uint64_t* aligned = b2ParticleScratchBuffer_AllocArray( &buffer, 1, uint64_t );
	ENSURE( aligned != NULL );
	ENSURE( ( (uintptr_t)aligned & (uintptr_t)( _Alignof( uint64_t ) - 1 ) ) == 0 );
	*aligned = UINT64_C( 0x123456789ABCDEF0 );

	ENSURE( ints[0] == 0 );
	ENSURE( ints[1] == 3 );
	ENSURE( ints[2] == 6 );
	ENSURE( ints[3] == 9 );
	ENSURE( *aligned == UINT64_C( 0x123456789ABCDEF0 ) );

	int highWaterCapacity = buffer.byteCapacity;
	b2ParticleScratchBuffer_Reset( &buffer );
	ENSURE( buffer.byteCount == 0 );
	ENSURE( buffer.byteCapacity == highWaterCapacity );
	ENSURE( buffer.byteCapacity >= initialCapacity );

	float* floats = b2ParticleScratchBuffer_ResizeArray( &buffer, 3, float );
	ENSURE( floats != NULL );
	ENSURE( buffer.byteCount == 3 * (int)sizeof( float ) );

	b2ParticleScratchBuffer_Destroy( &buffer );
	ENSURE( buffer.data == NULL );
	ENSURE( buffer.byteCount == 0 );
	ENSURE( buffer.byteCapacity == 0 );

	return 0;
}

static int ParticleScratchArrayKeepsTypedStorage( void )
{
	b2ParticleScratchArray array;
	b2ParticleScratchArray_InitTyped( &array, uint64_t );

	b2ParticleScratchArray_Reserve( &array, 3 );
	ENSURE( array.capacity >= 3 );
	ENSURE( array.count == 0 );
	ENSURE( array.elementSize == (int)sizeof( uint64_t ) );

	uint64_t* first = b2ParticleScratchArray_AddTyped( &array, uint64_t );
	uint64_t* second = b2ParticleScratchArray_AddTyped( &array, uint64_t );
	*first = 42;
	*second = 84;

	ENSURE( array.count == 2 );
	ENSURE( b2ParticleScratchArray_Data( &array, uint64_t )[0] == 42 );
	ENSURE( b2ParticleScratchArray_Data( &array, uint64_t )[1] == 84 );

	int highWaterCapacity = array.capacity;
	b2ParticleScratchArray_Reset( &array );
	ENSURE( array.count == 0 );
	ENSURE( array.capacity == highWaterCapacity );

	uint64_t* resized = b2ParticleScratchArray_ResizeTyped( &array, 5, uint64_t );
	ENSURE( resized != NULL );
	ENSURE( array.count == 5 );
	ENSURE( array.capacity >= 5 );

	resized[4] = 168;
	ENSURE( b2ParticleScratchArray_Data( &array, uint64_t )[4] == 168 );

	b2ParticleScratchArray_Destroy( &array );
	ENSURE( array.data == NULL );
	ENSURE( array.count == 0 );
	ENSURE( array.capacity == 0 );
	ENSURE( array.elementSize == 0 );

	return 0;
}

#endif

int ParticleHelperTest( void )
{
#if B2_HAS_PARTICLE_SORT_HELPERS
	RUN_SUBTEST( ParticleSortOrdersRecordsByOffsetKey );
	RUN_SUBTEST( ParticleSortKeyRecords );
#else
	printf( "  subtest skipped: particle_sort helpers not available\n" );
#endif

#if B2_HAS_PARTICLE_SCRATCH_HELPERS
	RUN_SUBTEST( ParticleScratchBufferKeepsHighWaterCapacity );
	RUN_SUBTEST( ParticleScratchArrayKeepsTypedStorage );
#else
	printf( "  subtest skipped: particle_scratch helpers not available\n" );
#endif

	return 0;
}
