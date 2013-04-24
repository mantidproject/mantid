#ifndef MANTID_ALGORITHMS_EXTRACTMASKTOTABLETEST_H_
#define MANTID_ALGORITHMS_EXTRACTMASKTOTABLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ExtractMaskToTable.h"
#include "MantidAPI/TableRow.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"

using Mantid::Algorithms::ExtractMaskToTable;

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid;

using namespace std;

class ExtractMaskToTableTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractMaskToTableTest *createSuite() { return new ExtractMaskToTableTest(); }
  static void destroySuite( ExtractMaskToTableTest *suite ) { delete suite; }

  /** Test initialization of the algorithm
    */
  void test_Init()
  {
    ExtractMaskToTable alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  /** Test for writing a new line to a new table workspace
    */
  void test_writeToNewTable()
  {
    // Create a workspace with some detectors masked
    const int nvectors(50), nbins(10);
    Workspace2D_sptr inputws = WorkspaceCreationHelper::Create2DWorkspace(nvectors, nbins);

    //   Mask every 10th spectra
    std::set<int64_t> maskedIndices;
    for( int i = 0; i < 50; i += 10 )
    {
      maskedIndices.insert(i);
    }
    //   Mask some consecutive detids
    maskedIndices.insert(5);
    maskedIndices.insert(6);
    maskedIndices.insert(7);
    inputws = WorkspaceCreationHelper::maskSpectra(inputws, maskedIndices);

    AnalysisDataService::Instance().addOrReplace("TestWorkspace1", inputws);

    /*
    for (size_t i = 0; i < inputws->getNumberHistograms(); ++i)
    {
      IDetector_const_sptr det = inputws->getDetector(i);
      cout << "WorkspaceIndex = " << i << "  X Size = " << inputws->readX(i).size()
           << "; Detector ID = " << det->getID() << ".\n";
    }
    Instrument_const_sptr instrument = inputws->getInstrument();
    vector<detid_t> detids = instrument->getDetectorIDs();
    cout << "Number of detectors = " << detids.size() << ".\n";
    for (size_t i = 0; i < detids.size(); ++i)
      cout << "Detector " << i << ":  ID = " << detids[i] << ".\n";
     */

    // Call algorithms
    ExtractMaskToTable alg;
    alg.initialize();

    // Set up properties
    alg.setProperty("InputWorkspace", "TestWorkspace1");
    alg.setProperty("OutputWorkspace", "MaskTable1");
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

    cout << "XMin = " << xxmin << ", XMax = " << xxmax << ", Spec list = " << specstr << ".\n";
    string expectedspec(" 1,  6-8,  11,  21,  31,  41");
    TS_ASSERT_EQUALS(specstr, expectedspec);
    TS_ASSERT_DELTA(xxmin, 1234.0, 0.0001);
    TS_ASSERT_DELTA(xxmax, 12345.6, 0.0001);

    // Clean
    AnalysisDataService::Instance().remove("TestWorkspace1");
    AnalysisDataService::Instance().remove("MaskTable1");

    return;
  }

  /** Test for appending a new line to an existing table workspace
    */
  void test_appendToExistingTable()
  {
    // Create a workspace with some detectors masked
    const int nvectors(50), nbins(10);
    Workspace2D_sptr inputws = WorkspaceCreationHelper::Create2DWorkspace(nvectors, nbins);

    //   Mask every 10th spectra
    std::set<int64_t> maskedIndices;
    for( int i = 0; i < 50; i += 10 )
    {
      maskedIndices.insert(i);
    }
    //   Mask some consecutive detids
    maskedIndices.insert(5);
    maskedIndices.insert(6);
    maskedIndices.insert(7);
    inputws = WorkspaceCreationHelper::maskSpectra(inputws, maskedIndices);

    AnalysisDataService::Instance().addOrReplace("TestWorkspace2", inputws);

    // Create a table workspace to append to
    TableWorkspace_sptr existtablews(new TableWorkspace());
    existtablews->addColumn("double", "XMin");
    existtablews->addColumn("double", "XMax");
    existtablews->addColumn("str", "SpectraList");
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
    cout << "XMin = " << xxmin << ", XMax = " << xxmax << ", Spec list = " << specstr << ".\n";
    string expectedspec(" 1,  6-8,  11,  21,  31,  41");
    TS_ASSERT_EQUALS(specstr, expectedspec);
    TS_ASSERT_DELTA(xxmin, 1234.0, 0.0001);
    TS_ASSERT_DELTA(xxmax, 12345.6, 0.0001);

    TableRow therow1 = outws->getRow(1);
    therow1 >> xxmin >> xxmax >> specstr;
    cout << "XMin = " << xxmin << ", XMax = " << xxmax << ", Spec list = " << specstr << ".\n";
    string expectedspec2("43");
    TS_ASSERT_DELTA(xxmin, 2345.1, 0.0001);
    TS_ASSERT_DELTA(xxmax, 78910.5, 0.0001);
    TS_ASSERT_EQUALS(specstr, expectedspec2);

    // Clean
    AnalysisDataService::Instance().remove("TestWorkspace2");
    AnalysisDataService::Instance().remove("MaskTable2");

  }

};


#endif /* MANTID_ALGORITHMS_EXTRACTMASKTOTABLETEST_H_ */
