#ifndef H_TEST_MAIN
#define H_TEST_MAIN

#include <cxxtest/TestSuite.h>
class tmain: public CxxTest::TestSuite
{
public:
      void testTMain(void){
           TS_WARN( "Main suite for sqw tests invoked" );
      }
};
#endif