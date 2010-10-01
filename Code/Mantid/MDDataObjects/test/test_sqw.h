#ifndef H_TEST_MAIN_SQW
#define H_TEST_MAIN_SQW

#include <cxxtest/TestSuite.h>
class tmain: public CxxTest::TestSuite
{
public:
      void testTMain(void){
           TS_WARN( "Main suite for sqw tests invoked" );
      }
};
#endif