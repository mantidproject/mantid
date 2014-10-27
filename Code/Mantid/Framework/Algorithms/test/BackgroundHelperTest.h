#ifndef BACKGROUDHELPER_TEST_H_
#define BACKGROUDHELPER_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/BackgroundHelper.h"



class BackgroundHelperTest : public CxxTest::TestSuite
{
public:

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BackgroundHelperTest *createSuite() { return new BackgroundHelperTest(); }
  static void destroySuite( BackgroundHelperTest *suite ) { delete suite; }

  BackgroundHelperTest()
  {
  }

  ~BackgroundHelperTest()
  {
  }
  
  void testBackgroundInit()
  {
  }

private:

 
};

 

#endif /*ALGORITHMTEST_H_*/
