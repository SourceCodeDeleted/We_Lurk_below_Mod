//-----------------------------------------------------------------------------
// ec_assert.h
//
// Asserts stay active in the shipped build. A failed assert is written to the debug log once per
// (file, line) and the function bails out; assert_failed() always returns false in this build, so
// the __debugbreak() is never reached.
//
// assert_return / assert_return_false / assert_return_zero are the original macro names (2019
// source). assert_exp is a name chosen here for the form that reports and carries on.

#ifndef _EC_ASSERT_H_
#define _EC_ASSERT_H_

void assert_system_init();
bool assert_exists( int line, const char * file );
bool assert_failed( const char * expression, const char * file, const char * function, int line, bool show_stack );

#define assert_exp( exp ) \
    do { if ( !( exp ) ) { if ( assert_failed( #exp, __FILE__, __FUNCSIG__, __LINE__, false ) ) __debugbreak(); } } while ( 0 )

#define assert_return( exp ) \
    do { if ( !( exp ) ) { if ( assert_failed( #exp, __FILE__, __FUNCSIG__, __LINE__, false ) ) __debugbreak(); return; } } while ( 0 )

#define assert_return_false( exp ) \
    do { if ( !( exp ) ) { if ( assert_failed( #exp, __FILE__, __FUNCSIG__, __LINE__, false ) ) __debugbreak(); return false; } } while ( 0 )

#define assert_return_zero( exp ) \
    do { if ( !( exp ) ) { if ( assert_failed( #exp, __FILE__, __FUNCSIG__, __LINE__, false ) ) __debugbreak(); return 0; } } while ( 0 )

#define assert_return_null( exp ) \
    do { if ( !( exp ) ) { if ( assert_failed( #exp, __FILE__, __FUNCSIG__, __LINE__, false ) ) __debugbreak(); return NULL; } } while ( 0 )

#endif
