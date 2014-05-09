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

  void verify_table_row(ITableWorkspace_sptr& outWS, int expectedClusterId,
      size_t expectedWorkspaceIndex, int expectedNormalDimensionIndex, bool expectedMaxExtent)
  {
    for (size_t rowIndex = 0; rowIndex < outWS->rowCount(); ++rowIndex)
    {
      auto clusterId = outWS->cell<int>(rowIndex, 0);
      auto wsIndex = outWS->cell<double>(rowIndex, 1);
      auto normalDimension = outWS->cell<int>(rowIndex, 2);
      auto maxExtent = outWS->cell<Mantid::API::Boolean>(rowIndex, 3);
      if (expectedClusterId == clusterId && expectedWorkspaceIndex == wsIndex
          && expectedNormalDimensionIndex == normalDimension && expectedMaxExtent == maxExtent)
      {
        return;
      }
    }
    TSM_ASSERT("Expected row does not exist in the output table workspace", false);
  }

  void test_find_one_edges_1D()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 3); // Makes a 1 by 3 md ws with identical signal values.
    inWS->setSignalAt(2, 0); // Now we have a single edge!

    ITableWorkspace_sptr outWS = doExecute(inWS);

    TSM_ASSERT_EQUALS("One face should be identified", outWS->rowCount(), 1);

    bool maxExtent = true;
    verify_table_row(outWS, 1, 1, 0, maxExtent);
  }

  void test_find_two_edges_1D()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 3); // Makes a 1 by 3 md ws with identical signal values.
    inWS->setSignalAt(2, 0); // Now we have a single edge!
    inWS->setSignalAt(0, 0); // Now we have another edge!

    ITableWorkspace_sptr outWS = doExecute(inWS);

    TSM_ASSERT_EQUALS("One face should be identified", outWS->rowCount(), 2);

    int clusterId = 1;
    size_t expectedWorkspaceIndex = 1;
    int expectedNormalDimensionIndex = 0;
    bool maxExtent = true;
    verify_table_row(outWS, clusterId, expectedWorkspaceIndex, expectedNormalDimensionIndex, maxExtent);
    verify_table_row(outWS, clusterId, expectedWorkspaceIndex, expectedNormalDimensionIndex, !maxExtent);
  }

  void test_find_three_edges_1D()
  {
    /*-------------

     signal at 0 and 2 is not empty.

     0  1  2  3
     |--|__|--|__|

     ^  ^  ^  ^
     |  |  |  |
     |Edge Edge Edge
     |
     Edge
     off


     --------------*/

    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 4); // Makes a 1 by 3 md ws with identical signal values.

    // This really creates four faces, with two non-zero label ids.
    inWS->setSignalAt(1, 0); // Now we have a single edge!
    inWS->setSignalAt(3, 0); // Now we have another edge!

    ITableWorkspace_sptr outWS = doExecute(inWS);

    TSM_ASSERT_EQUALS("Wrong number of faces", outWS->rowCount(), 3);

    int clusterId = 1;
    int expectedNormalDimensionIndex = 0;
    const bool maxExtent = true;
    verify_table_row(outWS, clusterId, 0/*workspace index*/, expectedNormalDimensionIndex, maxExtent);
    verify_table_row(outWS, clusterId, 2/*workspace index*/, expectedNormalDimensionIndex, maxExtent);
    verify_table_row(outWS, clusterId, 2/*workspace index*/, expectedNormalDimensionIndex, !maxExtent);
  }

  void test_find_four_edges_2D()
  {

    /*-------------

     Single non-empty cluster point. Should produce four faces.

     0 -  1  - 2
     3 - |4| - 5
     6 -  7  - 8

     --------------*/

    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(0, 2, 3); // Makes a 2 by 3 md ws with identical signal values of zero.
    inWS->setSignalAt(4, 1); // Central point is non-zero

    ITableWorkspace_sptr outWS = doExecute(inWS);

    TSM_ASSERT_EQUALS("Wrong number of faces", outWS->rowCount(), 4);
    int clusterId = 1;
    const bool maxExtent = true;

    verify_table_row(outWS, clusterId, 4 /*workspace index*/, 0 /*expectedNormalDimensionIndex*/,
        !maxExtent);
    verify_table_row(outWS, clusterId, 4 /*workspace index*/, 0 /*expectedNormalDimensionIndex*/,
        maxExtent);
    verify_table_row(outWS, clusterId, 4 /*workspace index*/, 1 /*expectedNormalDimensionIndex*/,
        !maxExtent);
    verify_table_row(outWS, clusterId, 4 /*workspace index*/, 1 /*expectedNormalDimensionIndex*/,
        maxExtent);
  }

  void test_find_two_edges_2D()
  {

    /*-------------

     Single non-empty cluster point.

     0 -  1  - 2
     3 -  4  - 5
     6 -  7  -|8|

     --------------*/

    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(0, 2, 3); // Makes a 2 by 3 md ws with identical signal values of zero.
    inWS->setSignalAt(8, 1); // last point is non-zero

    ITableWorkspace_sptr outWS = doExecute(inWS);

    TSM_ASSERT_EQUALS("Wrong number of faces", outWS->rowCount(), 2);
    int clusterId = 1;
    const bool maxExtent = true;

    verify_table_row(outWS, clusterId, 8 /*workspace index*/, 0 /*expectedNormalDimensionIndex*/,
        !maxExtent);
    verify_table_row(outWS, clusterId, 8 /*workspace index*/, 1 /*expectedNormalDimensionIndex*/,
        !maxExtent);
  }

  void test_find_six_edges_3D()
  {
    /*-------------

     Single non-empty cluster point.

     0 -  1  - 2
     3 -  4  - 5
     6 -  7  - 8

     9 -  10 - 11
     12- |13| - 14
     15-  16 - 17

     18-  19 - 20
     21-  22 - 23
     24-  25 - 26

     --------------*/

    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(0, 3, 3); // Makes a 3 by 3 md ws with identical signal values of zero.
    inWS->setSignalAt(13, 1); // central point is non-zero

    ITableWorkspace_sptr outWS = doExecute(inWS);

    TSM_ASSERT_EQUALS("Wrong number of faces", outWS->rowCount(), 6);
    int clusterId = 1;
    const bool maxExtent = true;

    verify_table_row(outWS, clusterId, 13 /*workspace index*/, 0 /*expectedNormalDimensionIndex*/,
        !maxExtent);
    verify_table_row(outWS, clusterId, 13 /*workspace index*/, 0 /*expectedNormalDimensionIndex*/,
        maxExtent);
    verify_table_row(outWS, clusterId, 13 /*workspace index*/, 1 /*expectedNormalDimensionIndex*/,
        !maxExtent);
    verify_table_row(outWS, clusterId, 13 /*workspace index*/, 1 /*expectedNormalDimensionIndex*/,
        maxExtent);
    verify_table_row(outWS, clusterId, 13 /*workspace index*/, 2 /*expectedNormalDimensionIndex*/,
        !maxExtent);
    verify_table_row(outWS, clusterId, 13 /*workspace index*/, 2 /*expectedNormalDimensionIndex*/,
        maxExtent);
  }

  void test_find_three_edges_3D()
  {
    /*-------------

     Single non-empty cluster point.

     0 -  1  - 2
     3 -  4  - 5
     6 -  7  - 8

     9 -  10 - 11
     12-  13 - 14
     15-  16 - 17

     18-  19 - 20
     21-  22 - 23
     24-  25 -|26|

     --------------*/

    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(0, 3, 3); // Makes a 3 by 3 md ws with identical signal values of zero.
    inWS->setSignalAt(26, 1); // central point is non-zero

    ITableWorkspace_sptr outWS = doExecute(inWS);

    TSM_ASSERT_EQUALS("Wrong number of faces", outWS->rowCount(), 3);
    int clusterId = 1;
    const bool maxExtent = true;

    verify_table_row(outWS, clusterId, 26 /*workspace index*/, 1 /*expectedNormalDimensionIndex*/,
        !maxExtent);
    verify_table_row(outWS, clusterId, 26 /*workspace index*/, 0 /*expectedNormalDimensionIndex*/,
        !maxExtent);
    verify_table_row(outWS, clusterId, 26 /*workspace index*/, 2 /*expectedNormalDimensionIndex*/,
        !maxExtent);

  }

};

#endif /* MANTID_CRYSTAL_FINDCLUSTERFACESTEST_H_ */
