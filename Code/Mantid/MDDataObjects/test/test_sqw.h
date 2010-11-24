#ifndef H_TEST_MAIN_SQW
#define H_TEST_MAIN_SQW
#include "find_mantid.h"
#include "MantidKernel/System.h"


#include <cxxtest/TestSuite.h>
class tmain: public CxxTest::TestSuite
{
public:
      void testTMain(void){
           TS_WARN( "Main suite for sqw tests invoked" );
      }
};



#endif