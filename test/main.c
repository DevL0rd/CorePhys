// SPDX-FileCopyrightText: 2023 Erin Catto
// SPDX-License-Identifier: MIT

#include "test_macros.h"

#include <string.h>

#if defined( _MSC_VER )
	#include <crtdbg.h>

// int MyAllocHook(int allocType, void* userData, size_t size, int blockType, long requestNumber, const unsigned char* filename,
//	int lineNumber)
//{
//	if (size == 16416)
//	{
//		size += 0;
//	}
//
//	return 1;
// }
#endif

extern int BitSetTest( void );
extern int CollisionTest( void );
extern int ContainerTest( void );
extern int DeterminismTest( void );
extern int DistanceTest( void );
extern int DynamicTreeTest( void );
extern int IdTest( void );
extern int MathTest( void );
extern int ParticleHelperTest( void );
extern int ParticleParallelTest( void );
extern int ParticleScenarioTest( void );
extern int ParticleTest( void );
extern int ShapeTest( void );
extern int TableTest( void );
extern int WorldTest( void );

static bool ShouldRunTest( const char* testName, int argc, char** argv )
{
	return argc == 1 || strcmp( argv[1], testName ) == 0;
}

#define RUN_SELECTED_TEST( T )                                                                                                   \
	do                                                                                                                           \
	{                                                                                                                            \
		if ( ShouldRunTest( #T, argc, argv ) )                                                                                   \
		{                                                                                                                        \
			ranTest = true;                                                                                                      \
			RUN_TEST( T );                                                                                                       \
		}                                                                                                                        \
	}                                                                                                                            \
	while ( false )

int main( int argc, char** argv )
{
#if defined( _MSC_VER )
	// Enable memory-leak reports

	// How to break at the leaking allocation, in the watch window enter this variable
	// and set it to the allocation number in {}. Do this at the first line in main.
	// {,,ucrtbased.dll}_crtBreakAlloc = <allocation number> 3970
	// Note:
	// Just _crtBreakAlloc in static link
	// Tracy Profile server leaks

	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
	//_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
	//_CrtSetAllocHook(MyAllocHook);
	//_CrtSetBreakAlloc(196);
#endif

	printf( "Starting CorePhys unit tests\n" );
	printf( "======================================\n" );

	bool ranTest = false;
	RUN_SELECTED_TEST( TableTest );
	RUN_SELECTED_TEST( MathTest );
	RUN_SELECTED_TEST( ParticleTest );
	RUN_SELECTED_TEST( ParticleHelperTest );
	RUN_SELECTED_TEST( ParticleParallelTest );
	RUN_SELECTED_TEST( ParticleScenarioTest );
	RUN_SELECTED_TEST( BitSetTest );
	RUN_SELECTED_TEST( CollisionTest );
	RUN_SELECTED_TEST( ContainerTest );
	RUN_SELECTED_TEST( DeterminismTest );
	RUN_SELECTED_TEST( DistanceTest );
	RUN_SELECTED_TEST( DynamicTreeTest );
	RUN_SELECTED_TEST( IdTest );
	RUN_SELECTED_TEST( ShapeTest );
	RUN_SELECTED_TEST( WorldTest );

	if ( ranTest == false )
	{
		printf( "Unknown CorePhys test: %s\n", argc > 1 ? argv[1] : "" );
		return 1;
	}

	printf( "======================================\n" );
	printf( "All CorePhys tests passed!\n" );

#if defined( _MSC_VER )
	if ( _CrtDumpMemoryLeaks() )
	{
		return 1;
	}
#endif

	return 0;
}
