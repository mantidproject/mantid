#ifndef MANTID_CRYSTAL_FINDCLUSTERFACESTEST_H_
#define MANTID_CRYSTAL_FINDCLUSTERFACESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/FindClusterFaces.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidGeometry/Instrument.h"
#include <boost/assign/list_of.hpp>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::MDEvents;
using Mantid::Crystal::FindClusterFaces;

namespace
{
  // Helper function to create a peaks workspace.
  IPeaksWorkspace_sptr create_peaks_WS(Instrument_sptr inst)
  {
    PeaksWorkspace* pPeaksWS = new PeaksWorkspace();
    pPeaksWS->setCoordinateSystem(Mantid::Kernel::HKL);
    IPeaksWorkspace_sptr peakWS(pPeaksWS);
    peakWS->setInstrument(inst);
    return peakWS;
  }

  // Helper function to create a MD Image workspace of labels.
  IMDHistoWorkspace_sptr create_HKL_MDWS(double min = -10, double max = 10, int numberOfBins = 3,
      double signalValue = 1, double errorValue = 1)
  {
    const int dimensionality = 3;
    int totalBins = 1;
    for (int i = 0; i < dimensionality; ++i)
    {
      totalBins *= numberOfBins;
    }
    auto mdworkspaceAlg = AlgorithmManager::Instance().createUnmanaged("CreateMDHistoWorkspace");
    mdworkspaceAlg->setChild(true);
    mdworkspaceAlg->initialize();
    mdworkspaceAlg->setProperty("Dimensionality", dimensionality);
    std::vector<int> numbersOfBins(dimensionality, numberOfBins);
    mdworkspaceAlg->setProperty("NumberOfBins", numbersOfBins);
    std::vector<double> extents =
        boost::assign::list_of(min)(max)(min)(max)(min)(max).convert_to_container<std::vector<double> >();
    mdworkspaceAlg->setProperty("Extents", extents);
    std::vector<double> signalValues(totalBins, signalValue);
    mdworkspaceAlg->setProperty("SignalInput", signalValues);
    std::vector<double> errorValues(totalBins, errorValue);
    mdworkspaceAlg->setProperty("ErrorInput", errorValues);
    mdworkspaceAlg->setPropertyValue("Names", "H,K,L");
    mdworkspaceAlg->setPropertyValue("Units", "-,-,-");
    mdworkspaceAlg->setPropertyValue("OutputWorkspace", "IntegratePeaksMDTest_MDEWS");
    mdworkspaceAlg->execute();
    IMDHistoWorkspace_sptr inWS = mdworkspaceAlg->getProperty("OutputWorkspace");

    // --- Set speical coordinates on fake mdworkspace --
    auto coordsAlg = AlgorithmManager::Instance().createUnmanaged("SetSpecialCoordinates");
    coordsAlg->setChild(true);
    coordsAlg->initialize();
    coordsAlg->setProperty("InputWorkspace", inWS);
    coordsAlg->setProperty("SpecialCoordinates", "HKL");
    coordsAlg->execute();
    return inWS;
  }

  ITableWorkspace_sptr doExecute(IMDHistoWorkspace_sptr& inWS)
  {
    FindClusterFaces alg;
    //alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("LimitRows", false);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    ITableWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    return outWS;
  }

  ITableWorkspace_sptr doExecuteWithFilter(IMDHistoWorkspace_sptr& inWS, IPeaksWorkspace_sptr& filterWS)
  {
    FindClusterFaces alg;
   // alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("FilterWorkspace", filterWS);
    alg.setProperty("LimitRows", false);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    ITableWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    return outWS;
  }
}

//=====================================================================================
// Functional Tests
//=====================================================================================
class FindClusterFacesTest: public CxxTest::TestSuite
{

private:

void verify_table_row(ITableWorkspace_sptr& outWS, int expectedClusterId, size_t expectedWorkspaceIndex, int expectedNormalDimensionIndex, bool expectedMaxExtent, double expectedRadius=-1)
  {
    for (size_t rowIndex = 0; rowIndex < outWS->rowCount(); ++rowIndex)
    {
      auto clusterId = outWS->cell<int>(rowIndex, 0);
      auto wsIndex = outWS->cell<double>(rowIndex, 1);
      auto normalDimension = outWS->cell<int>(rowIndex, 2);
      auto maxExtent = outWS->cell<Mantid::API::Boolean>(rowIndex, 3);
      auto radius = outWS->cell<double>(rowIndex, 4);
      if (expectedClusterId == clusterId && expectedWorkspaceIndex == wsIndex
          && expectedNormalDimensionIndex == normalDimension 
          && expectedMaxExtent == maxExtent && radius==expectedRadius)
      {
        return;
      }
    }
    TSM_ASSERT("Expected row does not exist in the output table workspace", false);
  }


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

  FindClusterFacesTest()
  {
    Mantid::API::FrameworkManager::Instance();
  }

  void test_throws_with_non_cluster_mdhistoworkspace()
  {
    const double nonIntegerSignalValue = 1.2;
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(nonIntegerSignalValue, 1, 1);
    TS_ASSERT_THROWS(doExecute(inWS), std::runtime_error&);
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

    bool maxExtent = true;
    verify_table_row(outWS, 1, 1, 0, maxExtent);
  }

  void test_find_two_edges_1D()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 3); // Makes a 1 by 3 md ws with identical signal values.
    inWS->setSignalAt(2, 0); // Now we have a single edge!
    inWS->setSignalAt(0, 0); // Now we have another edge!

    ITableWorkspace_sptr outWS = doExecute(inWS);

    TSM_ASSERT_EQUALS("Two faces should be identified", outWS->rowCount(), 2);

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

  void test_find_cluster_faces_throws_if_peaks_workspace_and_dimensionality_less_than_three()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(0, 2, 1);

    IPeaksWorkspace_sptr filterWS = boost::make_shared<PeaksWorkspace>();
    TS_ASSERT_THROWS(doExecuteWithFilter(inWS, filterWS), std::invalid_argument&);
  }

  void test_only_create_faces_for_clusters_corresponding_to_peaks()
  {
    const double min = 0; // HKL
    const double max = 10; // HKL

    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100, 0.05);
    IPeaksWorkspace_sptr filterWS = create_peaks_WS(inst);

    Peak peak(inst, 15050, 1.0);
    peak.setHKL(5, 5, 5); // Set HKL of peak
    filterWS->addPeak(peak); // Add a single peak.

    const int nBins = 10;
    const double bulkSignalValue = 0;
    auto inWS = create_HKL_MDWS(min, max, nBins, bulkSignalValue);
    inWS->setSignalAt(0, 2); // Cluster at linear index 0. No corresponding peak position.
    inWS->setSignalAt(555, 1); // Cluster at linear index 125. Corresponds with peak position

    ITableWorkspace_sptr faces = doExecuteWithFilter(inWS, filterWS);
    TSM_ASSERT_EQUALS(
        "Should have exactly 6 entries in the table. One cluster with 6 neighbours. The other cluster should be ignored as has no corresponding peak.",
        6, faces->rowCount());

    const double binWidth = (max-min)/nBins;
    const double binCenter = binWidth / 2;
    const double expectedRadius = std::sqrt(3 *  (binCenter * binCenter) ) ;

    verify_table_row(faces, 1, 555, 0, true, expectedRadius);
  }

  void test_complex_filtering()
  {
    const double min = 0; // HKL
    const double max = 10; // HKL

    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100, 0.05);
    IPeaksWorkspace_sptr filterWS = create_peaks_WS(inst);

    Peak centerPeak(inst, 15050, 1.0);
    centerPeak.setHKL(5, 5, 5); // Set HKL of peak
    filterWS->addPeak(centerPeak); // Add a valid center peak. Produces 6 faces.

    Peak cornerPeak(inst, 15050, 1.0);
    cornerPeak.setHKL(0, 0, 0); // Set HKL of peak
    filterWS->addPeak(cornerPeak); // Add a valid corner peak. Produces 3 faces.

    Peak outOfBoundsPeak(inst, 15050, 1.0);
    outOfBoundsPeak.setHKL(20, 20, 20); // Set HKL of peak
    filterWS->addPeak(outOfBoundsPeak); // Add an out of bounds peak. Produces 0 faces.

    const int nBins = 10;
    const double bulkSignalValue = 0;
    auto inWS = create_HKL_MDWS(min, max, nBins, bulkSignalValue);
    inWS->setSignalAt(0, 2); // Cluster at linear index 0. No corresponding peak position.
    inWS->setSignalAt(555, 1); // Cluster at linear index 125. Corresponds with peak position

    ITableWorkspace_sptr faces = doExecuteWithFilter(inWS, filterWS);
    TSM_ASSERT_EQUALS(
        "Should have exactly 3+6 entries in the table. One cluster with 6 neighbours, another with 3. The other cluster should be ignored as has no corresponding peak.",
        9, faces->rowCount());
  }

  void test_ignore_row_limit()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 3); // Makes a 1 by 3 md ws with identical signal values.
    inWS->setSignalAt(2, 0); // Now we have a single edge!
    inWS->setSignalAt(0, 0); // Now we have another edge!

    const int rowMaximumLimit = 1;

    FindClusterFaces alg;
    //alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("LimitRows", false); // IGNORE ROW LIMITS IF SUPPLIED
    alg.setProperty("MaximumRows", rowMaximumLimit);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    ITableWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    const bool isTruncated = alg.getProperty("TruncatedOutput");

    TSM_ASSERT("Result should NOT be truncated", !isTruncated);
    TSM_ASSERT_EQUALS("Two faces should be identified", 2, outWS->rowCount());

  }

  void test_limit_rows()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 3); // Makes a 1 by 3 md ws with identical signal values.
    inWS->setSignalAt(2, 0); // Now we have a single edge!
    inWS->setSignalAt(0, 0); // Now we have another edge!

    const int rowMaximumLimit = 1;

    FindClusterFaces alg;
    //alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("LimitRows", true);
    alg.setProperty("MaximumRows", rowMaximumLimit);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    ITableWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    const bool isTruncated = alg.getProperty("TruncatedOutput");

    TSM_ASSERT("Result should be truncated", isTruncated);
    TSM_ASSERT_EQUALS("Although there are actually two faces, only one face should be identified", outWS->rowCount(), rowMaximumLimit);

  }

};

//=====================================================================================
// Performance Tests
//=====================================================================================
class FindClusterFacesTestPerformance: public CxxTest::TestSuite
{
private:

  IMDHistoWorkspace_sptr m_inWS;
  IPeaksWorkspace_sptr m_filterWS;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindClusterFacesTestPerformance *createSuite()
  {
    return new FindClusterFacesTestPerformance();
  }
  static void destroySuite(FindClusterFacesTestPerformance *suite)
  {
    delete suite;
  }

  void test_Init()
  {
    FindClusterFaces alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
  }

  FindClusterFacesTestPerformance()
  {
    const double min = 0; // HKL
    const double max = 10; // HKL

    Mantid::API::FrameworkManager::Instance();
    const int nBins = 100;
    const double bulkSignalValue = 0;
    m_inWS = create_HKL_MDWS(min, max, nBins, bulkSignalValue);

    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100, 0.05);
    m_filterWS = create_peaks_WS(inst);

    //Add 50 cluster points and correspoinding peaks.
    for (int i = 0; i < nBins; i += 2)
    {
      m_inWS->setSignalAt(i, static_cast<Mantid::signal_t>(i));

      Peak peak(inst, 15050, 1.0);
      peak.setHKL(double(i), 0, 0); // Set HKL of peak
      m_filterWS->addPeak(peak); // Add a valid center peak. Produces 6 faces.
    }

  }

  void test_execution_unfiltered()
  {
    auto outTable = doExecute(m_inWS);
    TS_ASSERT(outTable->rowCount() > 0);
  }

  void test_execution_filtered()
  {
    auto outTable = doExecuteWithFilter(m_inWS, m_filterWS);
    TS_ASSERT(outTable->rowCount() > 0);
  }

};

#endif /* MANTID_CRYSTAL_FINDCLUSTERFACESTEST_H_ */
