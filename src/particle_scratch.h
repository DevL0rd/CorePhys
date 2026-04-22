// SPDX-FileCopyrightText: 2026 DevL0rd
// SPDX-License-Identifier: MIT

#pragma once

#include "core.h"

#include <stddef.h>
#include <stdint.h>

typedef struct b2ParticleScratchBuffer
{
	uint8_t* data;
	int byteCount;
	int byteCapacity;
} b2ParticleScratchBuffer;

typedef struct b2ParticleScratchArray
{
	void* data;
	int count;
	int capacity;
	int elementSize;
} b2ParticleScratchArray;

void b2ParticleScratchBuffer_Init( b2ParticleScratchBuffer* buffer );
void b2ParticleScratchBuffer_Destroy( b2ParticleScratchBuffer* buffer );
void b2ParticleScratchBuffer_Reset( b2ParticleScratchBuffer* buffer );
void b2ParticleScratchBuffer_Reserve( b2ParticleScratchBuffer* buffer, int byteCapacity );
void* b2ParticleScratchBuffer_Resize( b2ParticleScratchBuffer* buffer, int byteCount );
void* b2ParticleScratchBuffer_Alloc( b2ParticleScratchBuffer* buffer, int byteCount, int alignment );

void b2ParticleScratchArray_Init( b2ParticleScratchArray* array, int elementSize );
void b2ParticleScratchArray_Destroy( b2ParticleScratchArray* array );
void b2ParticleScratchArray_Reset( b2ParticleScratchArray* array );
void b2ParticleScratchArray_Reserve( b2ParticleScratchArray* array, int capacity );
void* b2ParticleScratchArray_Resize( b2ParticleScratchArray* array, int count );
void* b2ParticleScratchArray_Add( b2ParticleScratchArray* array );

static inline int b2ParticleScratchByteCount( int count, size_t elementSize )
{
	B2_ASSERT( count >= 0 );
	B2_ASSERT( elementSize > 0 );
	B2_ASSERT( elementSize <= (size_t)INT32_MAX );
	B2_ASSERT( count <= INT32_MAX / (int)elementSize );
	return count * (int)elementSize;
}

#define b2ParticleScratchBuffer_AllocArray( buffer, count, type )                                                                \
	( (type*)b2ParticleScratchBuffer_Alloc( ( buffer ), b2ParticleScratchByteCount( ( count ), sizeof( type ) ),                 \
											(int)_Alignof( type ) ) )

#define b2ParticleScratchBuffer_ResizeArray( buffer, count, type )                                                               \
	( (type*)b2ParticleScratchBuffer_Resize( ( buffer ), b2ParticleScratchByteCount( ( count ), sizeof( type ) ) ) )

#define b2ParticleScratchArray_InitTyped( array, type ) b2ParticleScratchArray_Init( ( array ), (int)sizeof( type ) )

#define b2ParticleScratchArray_Data( array, type ) ( (type*)( array )->data )

#define b2ParticleScratchArray_ResizeTyped( array, count, type )                                                                 \
	( B2_ASSERT( ( array )->elementSize == (int)sizeof( type ) ), (type*)b2ParticleScratchArray_Resize( ( array ), ( count ) ) )

#define b2ParticleScratchArray_AddTyped( array, type )                                                                           \
	( B2_ASSERT( ( array )->elementSize == (int)sizeof( type ) ), (type*)b2ParticleScratchArray_Add( ( array ) ) )
