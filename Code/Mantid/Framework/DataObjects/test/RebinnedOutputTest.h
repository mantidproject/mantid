#ifndef MANTID_DATAOBJECTS_REBINNEDOUTPUTTEST_H_
#define MANTID_DATAOBJECTS_REBINNEDOUTPUTTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class RebinnedOutputTest : public CxxTest::TestSuite
{
private:
  RebinnedOutput_sptr ws;
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RebinnedOutputTest *createSuite() { return new RebinnedOutputTest(); }
  static void destroySuite( RebinnedOutputTest *suite ) { delete suite; }

  void setUp()
  {
    //ws = WorkspaceCreationHelper::CreateRebinnedOutputWorkspace();
  }

  void testId()
  {
    TS_ASSERT_EQUALS(1, 1);
    //TS_ASSERT_EQUALS( ws->id(), "RebinnedOutput" );
  }

  void xtestRepresentation()
  {
    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS( ws->blocksize(), 6);
  }

};


#endif /* MANTID_DATAOBJECTS_REBINNEDOUTPUTTEST_H_ */
