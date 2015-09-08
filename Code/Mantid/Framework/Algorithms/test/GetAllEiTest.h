#ifndef GETALLEI_TEST_H_
#define GETALLEI_TEST_H_

#include <cxxtest/TestSuite.h>

class GetAllEiTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetAllEiTest *createSuite() { return new GetAllEiTest(); }
  static void destroySuite( GetAllEiTest *suite ) { delete suite; }

  GetAllEiTest(){
  }

public:
  void testName()
  {
    TS_ASSERT_EQUALS( norm.name(), "GetAllEi" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( norm.version(), 1 );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( norm.initialize() );
    TS_ASSERT( norm.isInitialized() );
  }



};

#endif
