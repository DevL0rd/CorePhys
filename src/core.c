// SPDX-FileCopyrightText: 2023 Erin Catto
// SPDX-License-Identifier: MIT

#include "core.h"

#include "corephys/math_functions.h"

#if defined( B2_COMPILER_MSVC )
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>
#else
#include <stdlib.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef COREPHYS_PROFILE

#if defined( B2_PLATFORM_WINDOWS )
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <tracy/TracyC.h>
#define b2TracyCAlloc( ptr, size ) TracyCAllocNS( ptr, size, 4, "CorePhys Heap" )
#define b2TracyCFree( ptr ) TracyCFreeNS( ptr, 4, "CorePhys Heap" )

typedef struct b2TracyMemoryTracker
{
	void** items;
	size_t count;
	size_t capacity;
} b2TracyMemoryTracker;

static b2TracyMemoryTracker b2_tracyMemoryTracker = { 0 };

#if defined( B2_PLATFORM_WINDOWS )
static INIT_ONCE b2_tracyMemoryTrackerOnce = INIT_ONCE_STATIC_INIT;
static CRITICAL_SECTION b2_tracyMemoryTrackerMutex;

static BOOL CALLBACK b2TracyMemoryTrackerInitOnce( PINIT_ONCE initOnce, PVOID parameter, PVOID* context )
{
	(void)initOnce;
	(void)parameter;
	(void)context;
	InitializeCriticalSection( &b2_tracyMemoryTrackerMutex );
	return TRUE;
}

static void b2TracyMemoryTrackerLock( void )
{
	InitOnceExecuteOnce( &b2_tracyMemoryTrackerOnce, b2TracyMemoryTrackerInitOnce, NULL, NULL );
	EnterCriticalSection( &b2_tracyMemoryTrackerMutex );
}

static void b2TracyMemoryTrackerUnlock( void )
{
	LeaveCriticalSection( &b2_tracyMemoryTrackerMutex );
}
#else
static pthread_mutex_t b2_tracyMemoryTrackerMutex = PTHREAD_MUTEX_INITIALIZER;

static void b2TracyMemoryTrackerLock( void )
{
	pthread_mutex_lock( &b2_tracyMemoryTrackerMutex );
}

static void b2TracyMemoryTrackerUnlock( void )
{
	pthread_mutex_unlock( &b2_tracyMemoryTrackerMutex );
}
#endif

static bool b2TracyMemoryTrackerAdd( void* ptr )
{
	size_t i = 0;
	if ( ptr == NULL )
	{
		return false;
	}

	b2TracyMemoryTrackerLock();
	for ( i = 0; i < b2_tracyMemoryTracker.count; ++i )
	{
		if ( b2_tracyMemoryTracker.items[i] == ptr )
		{
			b2TracyMemoryTrackerUnlock();
			return false;
		}
	}

	if ( b2_tracyMemoryTracker.count == b2_tracyMemoryTracker.capacity )
	{
		size_t nextCapacity = b2_tracyMemoryTracker.capacity > 0 ? b2_tracyMemoryTracker.capacity * 2 : 256;
		void** nextItems = realloc( b2_tracyMemoryTracker.items, nextCapacity * sizeof( *nextItems ) );
		if ( nextItems == NULL )
		{
			b2TracyMemoryTrackerUnlock();
			return false;
		}

		b2_tracyMemoryTracker.items = nextItems;
		b2_tracyMemoryTracker.capacity = nextCapacity;
	}

	b2_tracyMemoryTracker.items[b2_tracyMemoryTracker.count++] = ptr;
	b2TracyMemoryTrackerUnlock();
	return true;
}

static bool b2TracyMemoryTrackerRemove( void* ptr )
{
	size_t i = 0;
	if ( ptr == NULL )
	{
		return false;
	}

	b2TracyMemoryTrackerLock();
	for ( i = 0; i < b2_tracyMemoryTracker.count; ++i )
	{
		if ( b2_tracyMemoryTracker.items[i] == ptr )
		{
			b2_tracyMemoryTracker.items[i] = b2_tracyMemoryTracker.items[b2_tracyMemoryTracker.count - 1];
			b2_tracyMemoryTracker.count -= 1;
			b2TracyMemoryTrackerUnlock();
			return true;
		}
	}

	b2TracyMemoryTrackerUnlock();
	return false;
}

#else

#define b2TracyCAlloc( ptr, size )
#define b2TracyCFree( ptr )

#endif

#include "atomic.h"

// This allows the user to change the length units at runtime
static float b2_lengthUnitsPerMeter = 1.0f;

void b2SetLengthUnitsPerMeter( float lengthUnits )
{
	B2_ASSERT( b2IsValidFloat( lengthUnits ) && lengthUnits > 0.0f );
	b2_lengthUnitsPerMeter = lengthUnits;
}

float b2GetLengthUnitsPerMeter( void )
{
	return b2_lengthUnitsPerMeter;
}

static int b2DefaultAssertFcn( const char* condition, const char* fileName, int lineNumber )
{
	printf( "COREPHYS ASSERTION: %s, %s, line %d\n", condition, fileName, lineNumber );

	// return non-zero to break to debugger
	return 1;
}

static b2AssertFcn* b2AssertHandler = b2DefaultAssertFcn;

void b2SetAssertFcn( b2AssertFcn* assertFcn )
{
	B2_ASSERT( assertFcn != NULL );
	b2AssertHandler = assertFcn;
}

#if !defined( NDEBUG ) || defined( B2_ENABLE_ASSERT )
int b2InternalAssert( const char* condition, const char* fileName, int lineNumber )
{
	int result = b2AssertHandler( condition, fileName, lineNumber );
	if ( result )
	{
		B2_BREAKPOINT;
	}
	return result;
}
#endif

static void b2DefaultLogFcn( const char* message )
{
	printf( "CorePhys: %s\n", message );
}

static b2LogFcn* b2LogHandler = b2DefaultLogFcn;

void b2SetLogFcn( b2LogFcn* logFcn )
{
	B2_ASSERT( logFcn != NULL );
	b2LogHandler = logFcn;
}

void b2Log( const char* format, ... )
{
	va_list args;
	va_start( args, format );
	char buffer[512];
	vsnprintf( buffer, sizeof( buffer ), format, args );
	b2LogHandler( buffer );
	va_end( args );
}

b2Version b2GetVersion( void )
{
	return (b2Version){
		.major = 3,
		.minor = 2,
		.revision = 0,
	};
}

static b2AllocFcn* b2_allocFcn = NULL;
static b2FreeFcn* b2_freeFcn = NULL;

static b2AtomicInt b2_byteCount;

void b2SetAllocator( b2AllocFcn* allocFcn, b2FreeFcn* freeFcn )
{
	b2_allocFcn = allocFcn;
	b2_freeFcn = freeFcn;
}

// Use 32 byte alignment for everything. Works with 256bit SIMD.
#define B2_ALIGNMENT 32

void* b2Alloc( int size )
{
	if ( size == 0 )
	{
		return NULL;
	}

	// This could cause some sharing issues, however CorePhys rarely calls b2Alloc.
	b2AtomicFetchAddInt( &b2_byteCount, size );

	// Allocation must be a multiple of 32 or risk a seg fault
	// https://en.cppreference.com/w/c/memory/aligned_alloc
	int size32 = ( ( size - 1 ) | 0x1F ) + 1;

	if ( b2_allocFcn != NULL )
	{
		void* ptr = b2_allocFcn( size32, B2_ALIGNMENT );
#ifdef COREPHYS_PROFILE
		if ( TracyCIsConnected != 0 && b2TracyMemoryTrackerAdd( ptr ) )
		{
			b2TracyCAlloc( ptr, size );
		}
#endif

		B2_ASSERT( ptr != NULL );
		B2_ASSERT( ( (uintptr_t)ptr & 0x1F ) == 0 );

		return ptr;
	}

#ifdef B2_PLATFORM_WINDOWS
	void* ptr = _aligned_malloc( size32, B2_ALIGNMENT );
#elif defined( B2_PLATFORM_ANDROID )
	void* ptr = NULL;
	if ( posix_memalign( &ptr, B2_ALIGNMENT, size32 ) != 0 )
	{
		// allocation failed, exit the application
		exit( EXIT_FAILURE );
	}
#else
	void* ptr = aligned_alloc( B2_ALIGNMENT, size32 );
#endif

#ifdef COREPHYS_PROFILE
	if ( TracyCIsConnected != 0 && b2TracyMemoryTrackerAdd( ptr ) )
	{
		b2TracyCAlloc( ptr, size );
	}
#endif

	B2_ASSERT( ptr != NULL );
	B2_ASSERT( ( (uintptr_t)ptr & 0x1F ) == 0 );

	return ptr;
}

void* b2AllocZeroInit( int size )
{
	void* memory = b2Alloc( size );
	memset( memory, 0, size );
	return memory;
}

void b2Free( void* mem, int size )
{
	if ( mem == NULL )
	{
		return;
	}

#ifdef COREPHYS_PROFILE
	if ( b2TracyMemoryTrackerRemove( mem ) )
	{
		b2TracyCFree( mem );
	}
#endif

	if ( b2_freeFcn != NULL )
	{
		b2_freeFcn( mem, size );
	}
	else
	{
#ifdef B2_PLATFORM_WINDOWS
		_aligned_free( mem );
#else
		free( mem );
#endif
	}

	b2AtomicFetchAddInt( &b2_byteCount, -size );
}

void* b2GrowAlloc( void* oldMem, int oldSize, int newSize )
{
	B2_ASSERT( newSize > oldSize );
	void* newMem = b2Alloc( newSize );
	if ( oldSize > 0 )
	{
		memcpy( newMem, oldMem, oldSize );
		b2Free( oldMem, oldSize );
	}
	return newMem;
}

void* b2GrowAllocZeroInit( void* oldMem, int oldSize, int newSize )
{
	B2_ASSERT( newSize > oldSize );
	void* newMem = b2Alloc( newSize );
	if ( oldSize > 0 )
	{
		memcpy( newMem, oldMem, oldSize );
		b2Free( oldMem, oldSize );
	}

	memset( (char*)newMem + oldSize, 0, newSize - oldSize );
	return newMem;
}

int b2GetByteCount( void )
{
	return b2AtomicLoadInt( &b2_byteCount );
}
