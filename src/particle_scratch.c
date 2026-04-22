// SPDX-FileCopyrightText: 2026 DevL0rd
// SPDX-License-Identifier: MIT

#include "particle_scratch.h"

#include <stdbool.h>

static int b2ParticleScratchGrowCapacity( int oldCapacity, int minCapacity, int initialCapacity )
{
	B2_ASSERT( minCapacity >= 0 );
	B2_ASSERT( initialCapacity > 0 );

	if ( minCapacity <= oldCapacity )
	{
		return oldCapacity;
	}

	int newCapacity = oldCapacity < initialCapacity ? initialCapacity : oldCapacity + ( oldCapacity >> 1 );
	while ( newCapacity < minCapacity )
	{
		int previousCapacity = newCapacity;
		newCapacity = newCapacity + ( newCapacity >> 1 );
		B2_ASSERT( newCapacity > previousCapacity );
	}

	return newCapacity;
}

static bool b2IsPowerOfTwo( int value )
{
	return value > 0 && ( value & ( value - 1 ) ) == 0;
}

void b2ParticleScratchBuffer_Init( b2ParticleScratchBuffer* buffer )
{
	B2_ASSERT( buffer != NULL );
	*buffer = (b2ParticleScratchBuffer){ 0 };
}

void b2ParticleScratchBuffer_Destroy( b2ParticleScratchBuffer* buffer )
{
	if ( buffer == NULL )
	{
		return;
	}

	b2Free( buffer->data, buffer->byteCapacity );
	*buffer = (b2ParticleScratchBuffer){ 0 };
}

void b2ParticleScratchBuffer_Reset( b2ParticleScratchBuffer* buffer )
{
	B2_ASSERT( buffer != NULL );
	buffer->byteCount = 0;
}

void b2ParticleScratchBuffer_Reserve( b2ParticleScratchBuffer* buffer, int byteCapacity )
{
	B2_ASSERT( buffer != NULL );
	B2_ASSERT( byteCapacity >= 0 );

	if ( byteCapacity <= buffer->byteCapacity )
	{
		return;
	}

	int newCapacity = b2ParticleScratchGrowCapacity( buffer->byteCapacity, byteCapacity, 256 );
	buffer->data = b2GrowAlloc( buffer->data, buffer->byteCapacity, newCapacity );
	buffer->byteCapacity = newCapacity;
}

void* b2ParticleScratchBuffer_Resize( b2ParticleScratchBuffer* buffer, int byteCount )
{
	B2_ASSERT( buffer != NULL );
	B2_ASSERT( byteCount >= 0 );

	b2ParticleScratchBuffer_Reserve( buffer, byteCount );
	buffer->byteCount = byteCount;
	return buffer->data;
}

void* b2ParticleScratchBuffer_Alloc( b2ParticleScratchBuffer* buffer, int byteCount, int alignment )
{
	B2_ASSERT( buffer != NULL );
	B2_ASSERT( byteCount >= 0 );
	B2_ASSERT( b2IsPowerOfTwo( alignment ) );

	if ( byteCount == 0 )
	{
		return NULL;
	}

	B2_ASSERT( buffer->byteCount <= INT32_MAX - byteCount - alignment );
	b2ParticleScratchBuffer_Reserve( buffer, buffer->byteCount + byteCount + alignment - 1 );

	uintptr_t start = (uintptr_t)( buffer->data + buffer->byteCount );
	uintptr_t aligned = ( start + (uintptr_t)( alignment - 1 ) ) & ~(uintptr_t)( alignment - 1 );
	int alignedOffset = (int)( aligned - (uintptr_t)buffer->data );
	buffer->byteCount = alignedOffset + byteCount;

	return buffer->data + alignedOffset;
}

void b2ParticleScratchArray_Init( b2ParticleScratchArray* array, int elementSize )
{
	B2_ASSERT( array != NULL );
	B2_ASSERT( elementSize > 0 );

	*array = (b2ParticleScratchArray){ .elementSize = elementSize };
}

void b2ParticleScratchArray_Destroy( b2ParticleScratchArray* array )
{
	if ( array == NULL )
	{
		return;
	}

	b2Free( array->data, array->capacity * array->elementSize );
	*array = (b2ParticleScratchArray){ 0 };
}

void b2ParticleScratchArray_Reset( b2ParticleScratchArray* array )
{
	B2_ASSERT( array != NULL );
	array->count = 0;
}

void b2ParticleScratchArray_Reserve( b2ParticleScratchArray* array, int capacity )
{
	B2_ASSERT( array != NULL );
	B2_ASSERT( array->elementSize > 0 );
	B2_ASSERT( capacity >= 0 );

	if ( capacity <= array->capacity )
	{
		return;
	}

	int newCapacity = b2ParticleScratchGrowCapacity( array->capacity, capacity, 8 );
	array->data = b2GrowAlloc( array->data, array->capacity * array->elementSize, newCapacity * array->elementSize );
	array->capacity = newCapacity;
}

void* b2ParticleScratchArray_Resize( b2ParticleScratchArray* array, int count )
{
	B2_ASSERT( array != NULL );
	B2_ASSERT( count >= 0 );

	b2ParticleScratchArray_Reserve( array, count );
	array->count = count;
	return array->data;
}

void* b2ParticleScratchArray_Add( b2ParticleScratchArray* array )
{
	B2_ASSERT( array != NULL );

	if ( array->count == array->capacity )
	{
		b2ParticleScratchArray_Reserve( array, array->count + 1 );
	}

	uint8_t* item = (uint8_t*)array->data + array->count * array->elementSize;
	array->count += 1;
	return item;
}
