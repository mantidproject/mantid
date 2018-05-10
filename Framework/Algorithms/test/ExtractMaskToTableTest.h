#ifndef MANTID_ALGORITHMS_EXTRACTMASKTOTABLETEST_H_
#define MANTID_ALGORITHMS_EXTRACTMASKTOTABLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/ExtractMask.h"
#include "MantidAlgorithms/ExtractMaskToTable.h"
#include "MantidAlgorithms/MaskDetectorsIf.h"
#include "MantidAlgorithms/SumNeighbours.h"
#include "MantidDataHandling/MaskDetectors.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::ExtractMaskToTable;

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid;

using namespace std;

class ExtractMaskToTableTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractMaskToTableTest *createSuite() {
    return new ExtractMaskToTableTest();
  }
  static void destroySuite(ExtractMaskToTableTest *suite) { delete suite; }

  //----------------------------------------------------------------------------------------------
  /** Test method 'subtractVector'
   */
  void test_method() {
    ExtractMaskToTable alg;

    vector<int> vecA;

    // Case: A constains B
    vecA.reserve(20);
    for (size_t i = 0; i < 20; ++i) {
      vecA.push_back(static_cast<int>(i) + 5);
    }

    vector<int> vecB{6, 10, 14, 18};

    vector<int> vecC = alg.subtractVector(vecA, vecB);

    TS_ASSERT_EQUALS(vecC.size(), vecA.size() - vecB.size());

    // Case: A does not contain B; but the intersection between A and B is not
    // empty
    vecA.clear();
    vecB.clear();

    for (int i = 0; i < 10; ++i)
      vecA.push_back(i * 3);

    for (int i = 0; i < 10; ++i)
      vecB.push_back(i + 10);

    vecC = alg.subtractVector(vecA, vecB);

    TS_ASSERT_EQUALS(vecC.size(), 7);

    // Case: B has a larger range than A
    vecA.clear();
    vecB.clear();

    for (int i = 0; i < 10; ++i)
      vecA.push_back(5 + i * 2);

    for (int i = 0; i < 3; ++i)
      vecB.push_back(i + 1);
    for (int i = 0; i < 10; ++i)
      vecB.push_back(i + 10);
    vecB.push_back(25);
    vecB.push_back(30);

    vecC = alg.subtractVector(vecA, vecB);

    TS_ASSERT_EQUALS(vecC.size(), 5);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test initialization of the algorithm
   */
  void test_Init() {
    ExtractMaskToTable alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  //----------------------------------------------------------------------------------------------
  /** Test for writing a new line to a new table workspace
   */
  void test_writeToNewTable() {
    // Create a workspace with some detectors masked
    const int nvectors(50), nbins(10);
    Workspace2D_sptr inputws =
        WorkspaceCreationHelper::create2DWorkspace(nvectors, nbins);

    //   Mask every 10th spectra
    std::set<int64_t> maskedIndices;
    for (int i = 0; i < 50; i += 10) {
      maskedIndices.insert(i);
    }
    //   Mask some consecutive detids
    maskedIndices.insert(5);
    maskedIndices.insert(6);
    maskedIndices.insert(7);
    inputws = WorkspaceCreationHelper::maskSpectra(inputws, maskedIndices);

    AnalysisDataService::Instance().addOrReplace("TestWorkspace1", inputws);

    // Call algorithms
    ExtractMaskToTable alg;
    alg.initialize();

    // Set up properties
    alg.setPropertyValue("InputWorkspace", "TestWorkspace1");
    alg.setPropertyValue("OutputWorkspace", "MaskTable1");
    alg.setProperty("XMin", 1234.0);
    alg.setProperty("XMax", 12345.6);

    // Execute
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Validate
    TableWorkspace_sptr outws = boost::dynamic_pointer_cast<TableWorkspace>(
        AnalysisDataService::Instance().retrieve("MaskTable1"));
    TS_ASSERT(outws);
    if (!outws)
      return;

    cout << "Number of row in table workspace : " << outws->rowCount() << ".\n";
    TS_ASSERT_EQUALS(outws->rowCount(), 1);

    TableRow therow = outws->getRow(0);
    double xxmin, xxmax;
    string specstr;
    therow >> xxmin >> xxmax >> specstr;

    string expectedspec(" 1,  6-8,  11,  21,  31,  41");
    TS_ASSERT_EQUALS(specstr, expectedspec);
    TS_ASSERT_DELTA(xxmin, 1234.0, 0.0001);
    TS_ASSERT_DELTA(xxmax, 12345.6, 0.0001);

    // Clean
    AnalysisDataService::Instance().remove("TestWorkspace1");
    AnalysisDataService::Instance().remove("MaskTable1");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test for appending a new line to an existing table workspace
   */
  void test_appendToExistingTable() {
    // Create a workspace with some detectors masked
    const int nvectors(50), nbins(10);
    Workspace2D_sptr inputws =
        WorkspaceCreationHelper::create2DWorkspace(nvectors, nbins);

    //   Mask every 10th spectra
    std::set<int64_t> maskedIndices;
    for (int i = 0; i < 50; i += 10) {
      maskedIndices.insert(i);
    }
    //   Mask some consecutive detids
    maskedIndices.insert(5);
    maskedIndices.insert(6);
    maskedIndices.insert(7);
    inputws = WorkspaceCreationHelper::maskSpectra(inputws, maskedIndices);

    AnalysisDataService::Instance().addOrReplace("TestWorkspace2", inputws);

    // Create a table workspace to append to
    auto existtablews = boost::make_shared<TableWorkspace>();
    existtablews->addColumn("double", "XMin");
    existtablews->addColumn("double", "XMax");
    existtablews->addColumn("str", "DetectorIDsList");
    TableRow row0 = existtablews->appendRow();
    row0 << 2345.0 << 78910.3 << "23-24, 33";
    TableRow row1 = existtablews->appendRow();
    row1 << 2345.1 << 78910.5 << "43";

    AnalysisDataService::Instance().addOrReplace("MaskTable2", existtablews);

    // Call algorithms
    ExtractMaskToTable alg;
    alg.initialize();

    // Set up properties
    alg.setProperty("InputWorkspace", "TestWorkspace2");
    alg.setProperty("MaskTableWorkspace", "MaskTable2");
    alg.setProperty("OutputWorkspace", "MaskTable2");
    alg.setProperty("XMin", 1234.0);
    alg.setProperty("XMax", 12345.6);

    // Execute
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Validate
    TableWorkspace_sptr outws = boost::dynamic_pointer_cast<TableWorkspace>(
        AnalysisDataService::Instance().retrieve("MaskTable2"));
    TS_ASSERT(outws);
    if (!outws)
      return;

    cout << "Number of row in table workspace : " << outws->rowCount() << ".\n";
    TS_ASSERT_EQUALS(outws->rowCount(), 3);

    double xxmin, xxmax;
    string specstr;

    TableRow therow = outws->getRow(2);
    therow >> xxmin >> xxmax >> specstr;

    string expectedspec(" 1,  6-8,  11,  21,  31,  41");
    TS_ASSERT_EQUALS(specstr, expectedspec);
    TS_ASSERT_DELTA(xxmin, 1234.0, 0.0001);
    TS_ASSERT_DELTA(xxmax, 12345.6, 0.0001);

    TableRow therow1 = outws->getRow(1);
    therow1 >> xxmin >> xxmax >> specstr;

    string expectedspec2("43");
    TS_ASSERT_DELTA(xxmin, 2345.1, 0.0001);
    TS_ASSERT_DELTA(xxmax, 78910.5, 0.0001);
    TS_ASSERT_EQUALS(specstr, expectedspec2);

    // Call algorithm second time with the same arguments
    ExtractMaskToTable alg1;
    alg1.initialize();

    // Set up properties
    alg1.setProperty("InputWorkspace", "TestWorkspace2");
    alg1.setProperty("MaskTableWorkspace", "MaskTable2");
    alg1.setProperty("OutputWorkspace", "MaskTable2");
    alg1.setProperty("XMin", 1234.0);
    alg1.setProperty("XMax", 12345.6);

    // Execute
    alg1.execute();
    // Returns not executed but doesn't crash
    TS_ASSERT(!alg1.isExecuted());

    // Clean
    AnalysisDataService::Instance().remove("TestWorkspace2");
    AnalysisDataService::Instance().remove("MaskTable2");

    return;
  }

  /** Test for appending a new line to an existing table workspace
   * Some masked detectors are in the input table workspace
   */
  void test_appendToPreviousTable() {
    // Create a workspace with some detectors masked
    const int nvectors(50), nbins(10);
    Workspace2D_sptr inputws =
        WorkspaceCreationHelper::create2DWorkspace(nvectors, nbins);

    //   Mask every 10th spectra
    std::set<int64_t> maskedIndices;
    for (int i = 0; i < 50; i += 10) {
      maskedIndices.insert(i);
    }
    for (int i = 22; i < 25; ++i) {
      maskedIndices.insert(i);
    }
    maskedIndices.insert(42);

    //   Mask some consecutive detids
    maskedIndices.insert(5);
    maskedIndices.insert(6);
    maskedIndices.insert(7);
    inputws = WorkspaceCreationHelper::maskSpectra(inputws, maskedIndices);

    AnalysisDataService::Instance().addOrReplace("TestWorkspace2", inputws);

    // Create a table workspace to append to
    auto existtablews = boost::make_shared<TableWorkspace>();
    existtablews->addColumn("double", "XMin");
    existtablews->addColumn("double", "XMax");
    existtablews->addColumn("str", "DetectorIDsList");
    TableRow row0 = existtablews->appendRow();
    row0 << 2345.0 << 78910.3 << "23-25, 33";
    TableRow row1 = existtablews->appendRow();
    row1 << 2345.1 << 78910.5 << "43";

    AnalysisDataService::Instance().addOrReplace("MaskTable2", existtablews);

    // Call algorithms
    ExtractMaskToTable alg;
    alg.initialize();

    // Set up properties
    alg.setProperty("InputWorkspace", "TestWorkspace2");
    alg.setProperty("MaskTableWorkspace", "MaskTable2");
    alg.setProperty("OutputWorkspace", "MaskTable2");
    alg.setProperty("XMin", 1234.0);
    alg.setProperty("XMax", 12345.6);

    // Execute
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Validate
    TableWorkspace_sptr outws = boost::dynamic_pointer_cast<TableWorkspace>(
        AnalysisDataService::Instance().retrieve("MaskTable2"));
    TS_ASSERT(outws);
    if (!outws)
      return;

    cout << "Number of row in table workspace : " << outws->rowCount() << ".\n";
    TS_ASSERT_EQUALS(outws->rowCount(), 3);

    double xxmin, xxmax;
    string specstr;

    TableRow therow = outws->getRow(2);
    therow >> xxmin >> xxmax >> specstr;
    cout << "XMin = " << xxmin << ", XMax = " << xxmax
         << ", Spec list = " << specstr << ".\n";
    string expectedspec(" 1,  6-8,  11,  21,  31,  41");
    TS_ASSERT_EQUALS(specstr, expectedspec);
    TS_ASSERT_DELTA(xxmin, 1234.0, 0.0001);
    TS_ASSERT_DELTA(xxmax, 12345.6, 0.0001);

    TableRow therow1 = outws->getRow(1);
    therow1 >> xxmin >> xxmax >> specstr;
    cout << "XMin = " << xxmin << ", XMax = " << xxmax
         << ", Spec list = " << specstr << ".\n";
    string expectedspec2("43");
    TS_ASSERT_DELTA(xxmin, 2345.1, 0.0001);
    TS_ASSERT_DELTA(xxmax, 78910.5, 0.0001);
    TS_ASSERT_EQUALS(specstr, expectedspec2);

    // Clean
    AnalysisDataService::Instance().remove("TestWorkspace2");
    AnalysisDataService::Instance().remove("MaskTable2");

    return;
  }

  /** Test for extracting masks from a MaskWorkspace
   */
  void test_extractFromMaskWorkspace() {
    // Create a workspace with Mask
    const int nvectors(50), nbins(10);
    Workspace2D_sptr inputws =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nvectors,
                                                                     nbins);
    AnalysisDataService::Instance().addOrReplace("TestWorkspace3", inputws);

    //   Mask detectors of spectra 1, 6, 11, ..., 31
    DataHandling::MaskDetectors maskalg;
    maskalg.initialize();
    maskalg.setProperty("Workspace", "TestWorkspace3");
    maskalg.setPropertyValue("SpectraList", "1-3, 5, 20, 34");
    maskalg.execute();
    if (!maskalg.isExecuted())
      throw runtime_error("MaskDetectors fails to execute.");
    else
      cout << "Workspace masked."
           << "\n";

    const auto &detectorInfo = inputws->detectorInfo();
    for (size_t i = 0; i < detectorInfo.size(); ++i) {
      if (detectorInfo.isMasked(i))
        cout << "Detector : " << detectorInfo.detectorIDs()[i]
             << " is masked.\n";
    }

    /*
    //   Sum spectra
    Algorithms::SumNeighbours sumalg;
    sumalg.initialize();
    sumalg.setProperty("InputWorkspace", "TestWorkspace3");
    sumalg.setProperty("OutputWorkspace", "TestWorkspace3");
    sumalg.setProperty("SumX", 2);
    sumalg.setProperty("SumY", 1);

    sumalg.execute();
    if (!sumalg.isExecuted())
      throw runtime_error("Unable to sum neighbors");
    else
      cout << "Workspace " << inputws->name() << " has " <<
    inputws->getNumberHistograms() << " spectra.\n";
    */

    //   Extract to take workspace
    Algorithms::ExtractMask extractalg;
    extractalg.initialize();
    extractalg.setProperty("InputWorkspace", "TestWorkspace3");
    extractalg.setProperty("OutputWorkspace", "MaskWorkspace3");
    extractalg.execute();

    // Call algorithms
    ExtractMaskToTable alg;
    alg.initialize();

    // Set up properties
    alg.setProperty("InputWorkspace", "MaskWorkspace3");
    alg.setProperty("OutputWorkspace", "MaskTable3");
    alg.setProperty("XMin", 1234.0);
    alg.setProperty("XMax", 12345.6);

    // Execute
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Validate
    TableWorkspace_sptr outws = boost::dynamic_pointer_cast<TableWorkspace>(
        AnalysisDataService::Instance().retrieve("MaskTable3"));
    TS_ASSERT(outws);
    if (!outws)
      return;

    cout << "Number of row in table workspace : " << outws->rowCount() << ".\n";
    TS_ASSERT_EQUALS(outws->rowCount(), 1);

    double xxmin, xxmax;
    string specstr;

    TableRow therow = outws->getRow(0);
    therow >> xxmin >> xxmax >> specstr;
    cout << "XMin = " << xxmin << ", XMax = " << xxmax
         << ", Spec list = " << specstr << ".\n";
    string expectedspec(" 1-3,  5,  20,  34");
    TS_ASSERT_EQUALS(specstr, expectedspec);
    TS_ASSERT_DELTA(xxmin, 1234.0, 0.0001);
    TS_ASSERT_DELTA(xxmax, 12345.6, 0.0001);

    // Clean
    AnalysisDataService::Instance().remove("TestWorkspace3");
    AnalysisDataService::Instance().remove("MaskWorkspace3");
    AnalysisDataService::Instance().remove("MaskTable3");

    return;
  }
};

#endif /* MANTID_ALGORITHMS_EXTRACTMASKTOTABLETEST_H_ */
