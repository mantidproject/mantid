#ifndef LOADVTK_TEST_H_
#define LOADVTK_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/LoadVTK.h"


using namespace Mantid::API;
using namespace Mantid::VATES;

class LoadVTKTest: public CxxTest::TestSuite
{
public:

  void test_catagory()
  {
    LoadVTK loadVTK;
    Algorithm& alg = loadVTK;
    TS_ASSERT_EQUALS("MDAlgorithms", alg.category());
  }

  void test_version()
  {
    LoadVTK loadVTK;
    TS_ASSERT_EQUALS(1, loadVTK.version());
  }

};

#endif