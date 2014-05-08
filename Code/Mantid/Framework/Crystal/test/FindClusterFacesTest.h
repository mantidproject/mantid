#ifndef MANTID_CRYSTAL_FINDCLUSTERFACESTEST_H_
#define MANTID_CRYSTAL_FINDCLUSTERFACESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/FindClusterFaces.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace Mantid::API;
using namespace Mantid::MDEvents;
using Mantid::Crystal::FindClusterFaces;

class FindClusterFacesTest: public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindClusterFacesTest *createSuite()
  {
    return new FindClusterFacesTest();
  }
  static void destroySuite(FindClusterFacesTest *suite)
  {
    delete suite;
  }

  void test_Init()
  {
    FindClusterFaces alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
  }

  ITableWorkspace_sptr doExecute(IMDHistoWorkspace_sptr& inWS)
  {
    FindClusterFaces alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    ITableWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    return outWS;
  }

  void test_find_no_edges_1D()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 3); // Makes a 1 by 3 md ws with identical signal values.

    ITableWorkspace_const_sptr outWS = doExecute(inWS);

    TSM_ASSERT_EQUALS("There are no edge faces", outWS->rowCount(), 0);
  }

  void test_find_one_edges_1D()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 3); // Makes a 1 by 3 md ws with identical signal values.
    inWS->setSignalAt(2, 0); // Now we have a single edge!

    ITableWorkspace_sptr outWS = doExecute(inWS);

    TSM_ASSERT_EQUALS("One face should be identified", outWS->rowCount(), 1);

    auto clusterId = outWS->cell<int>(0,  0);
    auto wsIndex = outWS->cell<double>(0,  1);
    auto normalDimension = outWS->cell<int>(0,  2);
    auto max = outWS->cell<Mantid::API::Boolean>(0,  3);

    TSM_ASSERT_EQUALS("Wrong clusterId", 1, clusterId);
    TSM_ASSERT_EQUALS("Wrong workspace index", 1, wsIndex);
    TSM_ASSERT_EQUALS("Wrong normal dimension", 0, normalDimension);
    TSM_ASSERT_EQUALS("Wrong max min. Face is greater than current liner index.", true, max);
  }

};

#endif /* MANTID_CRYSTAL_FINDCLUSTERFACESTEST_H_ */
