#ifndef MANTID_CRYSTAL_CALCULATEPEAKSHKLTEST_H_
#define MANTID_CRYSTAL_CALCULATEPEAKSHKLTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCrystal/CalculatePeaksHKL.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Crystal::CalculatePeaksHKL;
using namespace Mantid::DataObjects;

class CalculatePeaksHKLTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculatePeaksHKLTest *createSuite() { return new CalculatePeaksHKLTest(); }
  static void destroySuite( CalculatePeaksHKLTest *suite ) { delete suite; }
  
  void test_Constructor()
  {
    TS_ASSERT_THROWS_NOTHING(CalculatePeaksHKL alg);
  }

  void test_Init()
  {
    PeaksWorkspace_sptr ws = boost::make_shared<PeaksWorkspace>();

    CalculatePeaksHKL alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("PeaksWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("OverWrite", true) );
  }

  void test_Execute()
  {
    auto ws = WorkspaceCreationHelper::createPeaksWorkspace(10);
    CalculatePeaksHKL alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("PeaksWorkspace", ws);
    alg.setProperty("OverWrite", false);
    alg.execute();
    int numberIndexed = alg.getProperty("NumIndexed");
    TS_ASSERT_EQUALS(numberIndexed, ws->getNumberPeaks());
  }


};


#endif /* MANTID_CRYSTAL_CALCULATEPEAKSHKLTEST_H_ */
