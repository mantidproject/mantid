#ifndef MANTID_ALGORITHMS_MASKDETECTORBINSTEST_H_
#define MANTID_ALGORITHMS_MASKDETECTORBINSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/MaskBinsFromTable.h"
#include "MantidAPI/TableRow.h"
#include "MantidGeometry/Instrument.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

using namespace std;

class MaskBinsFromTableTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaskBinsFromTableTest *createSuite() {
    return new MaskBinsFromTableTest();
  }
  static void destroySuite(MaskBinsFromTableTest *suite) { delete suite; }

  /** In-place single mask test.
   * Same as the test in MaskBins()
   */
  void test_MaskBinWithSingleLine() {
    // 1. Create a dummy workspace
    const std::string workspaceName("raggedMask");
    int nBins = 10;
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(5, nBins, 0.0);
    AnalysisDataService::Instance().add(workspaceName, WS);

    // 2. Generate a TableWorskpace
    DataObjects::TableWorkspace_sptr tablews =
        boost::shared_ptr<DataObjects::TableWorkspace>(
            new DataObjects::TableWorkspace());
    tablews->addColumn("double", "XMin");
    tablews->addColumn("double", "XMax");
    tablews->addColumn("str", "SpectraList");

    API::TableRow row0 = tablews->appendRow();
    row0 << 3.0 << 6.0 << "1-3";

    // 3. Execute
    MaskBinsFromTable maskalg;
    TS_ASSERT_THROWS_NOTHING(maskalg.initialize());
    maskalg.setPropertyValue("InputWorkspace", workspaceName);
    maskalg.setPropertyValue("OutputWorkspace", workspaceName);
    maskalg.setProperty("MaskingInformation", tablews);
    TS_ASSERT_THROWS_NOTHING(maskalg.execute());
    TS_ASSERT(maskalg.isExecuted());

    // 4. Check
    WS = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(workspaceName));
    TS_ASSERT(WS);
    for (int wi = 1; wi <= 3; wi++) {
      for (int bin = 3; bin < 6; bin++) {
        TS_ASSERT_EQUALS(WS->y(wi)[bin], 0.0);
      }
    }

    // 5. Clean
    AnalysisDataService::Instance().remove(workspaceName);

    return;
  }

  /** Out-of-place single mask test.
   * Same as the test in MaskBins()
   */
  void test_MaskBinWithSingleLineOutPlace() {
    // 1. Create a dummy workspace
    const std::string workspaceName("raggedMask");
    const std::string opWSName("maskedWorkspace");
    int nBins = 10;
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(5, nBins, 0.0);
    AnalysisDataService::Instance().add(workspaceName, WS);

    // 2. Generate a TableWorskpace
    DataObjects::TableWorkspace_sptr tablews =
        boost::shared_ptr<DataObjects::TableWorkspace>(
            new DataObjects::TableWorkspace());
    tablews->addColumn("double", "XMin");
    tablews->addColumn("double", "XMax");
    tablews->addColumn("str", "SpectraList");

    API::TableRow row0 = tablews->appendRow();
    row0 << 3.0 << 6.0 << "1-3";

    // 3. Execute
    MaskBinsFromTable maskalg;
    TS_ASSERT_THROWS_NOTHING(maskalg.initialize());
    maskalg.setPropertyValue("InputWorkspace", workspaceName);
    maskalg.setPropertyValue("OutputWorkspace", opWSName);
    maskalg.setProperty("MaskingInformation", tablews);
    TS_ASSERT_THROWS_NOTHING(maskalg.execute());
    TS_ASSERT(maskalg.isExecuted());

    // 4. Check
    MatrixWorkspace_sptr outWS =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(opWSName));
    TS_ASSERT(outWS);
    for (int wi = 1; wi <= 3; wi++) {
      for (int bin = 3; bin < 6; bin++) {
        TS_ASSERT_EQUALS(outWS->y(wi)[bin], 0.0);
      }
    }

    // 5. Clean
    AnalysisDataService::Instance().remove(workspaceName);
    AnalysisDataService::Instance().remove(opWSName);

    return;
  }

  /** Multiple lines out-of-place test.
   * This is a real test
   */
  void test_MaskBinWithMultiLines() {
    // 1. Create a dummy workspace
    const std::string workspaceName("raggedMask");
    int nBins = 10;
    int nHist = 12;
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(nHist, nBins, 0.0);
    AnalysisDataService::Instance().add(workspaceName, WS);

    // 2. Generate a TableWorskpace
    DataObjects::TableWorkspace_sptr tablews =
        boost::shared_ptr<DataObjects::TableWorkspace>(
            new DataObjects::TableWorkspace());
    tablews->addColumn("double", "XMin");
    tablews->addColumn("double", "XMax");
    tablews->addColumn("str", "SpectraList");

    API::TableRow row0 = tablews->appendRow();
    row0 << 3.0 << 6.0 << "1-3";
    API::TableRow row1 = tablews->appendRow();
    row1 << 4.0 << 7.0 << "5, 6-8";
    API::TableRow row2 = tablews->appendRow();
    row2 << 0.0 << 1.0 << "9";

    // 3. Execute
    MaskBinsFromTable maskalg;
    TS_ASSERT_THROWS_NOTHING(maskalg.initialize());
    maskalg.setPropertyValue("InputWorkspace", workspaceName);
    maskalg.setPropertyValue("OutputWorkspace", workspaceName);
    maskalg.setProperty("MaskingInformation", tablews);
    TS_ASSERT_THROWS_NOTHING(maskalg.execute());
    TS_ASSERT(maskalg.isExecuted());

    // 4. Check
    WS = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(workspaceName));
    TS_ASSERT(WS);

    // a) Table Line 0
    for (int wi = 1; wi <= 3; wi++) {
      for (size_t bin = 0; bin < WS->y(wi).size(); ++bin) {
        if (bin >= 3 && bin < 6) {
          TS_ASSERT_EQUALS(WS->y(wi)[bin], 0.0);
        } else {
          TS_ASSERT_EQUALS(WS->y(wi)[bin], 2.0);
        }
      }
    }

    // b) Table Line 1
    std::vector<int> speclist;
    speclist.push_back(5);
    speclist.push_back(6);
    speclist.push_back(7);
    speclist.push_back(8);
    for (size_t iws = 0; iws < speclist.size(); ++iws) {
      auto &yvec = WS->y(speclist[iws]);
      for (size_t bin = 0; bin < yvec.size(); ++bin) {
        if (bin >= 4 && bin < 7) {
          TS_ASSERT_EQUALS(yvec[bin], 0.0);
        } else {
          TS_ASSERT_EQUALS(yvec[bin], 2.0);
        }
      }
    }

    // c) Table Line 2
    for (size_t iws = 9; iws < 10; ++iws) {
      auto &yvec = WS->y(iws);
      for (size_t bin = 0; bin < yvec.size(); ++bin) {
        if (bin == 0) {
          TS_ASSERT_EQUALS(yvec[bin], 0.0);
        } else {
          TS_ASSERT_EQUALS(yvec[bin], 2.0);
        }
      }
    }

    // 5. Clean
    AnalysisDataService::Instance().remove(workspaceName);

    return;
  }

  /** In-place single mask test.
   * Same as the test in MaskBins()
   * With TableWorkspace of column in different order
   */
  void test_MaskBinWithSingleLine2() {
    // 1. Create a dummy workspace
    const std::string workspaceName("raggedMask");
    int nBins = 10;
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(5, nBins, 0.0);
    AnalysisDataService::Instance().add(workspaceName, WS);

    // 2. Generate a TableWorskpace
    DataObjects::TableWorkspace_sptr tablews =
        boost::shared_ptr<DataObjects::TableWorkspace>(
            new DataObjects::TableWorkspace());
    tablews->addColumn("str", "SpectraList");
    tablews->addColumn("double", "XMin");
    tablews->addColumn("double", "XMax");

    API::TableRow row0 = tablews->appendRow();
    row0 << "1-3" << 3.0 << 6.0;

    // 3. Execute
    MaskBinsFromTable maskalg;
    TS_ASSERT_THROWS_NOTHING(maskalg.initialize());
    maskalg.setPropertyValue("InputWorkspace", workspaceName);
    maskalg.setPropertyValue("OutputWorkspace", workspaceName);
    maskalg.setProperty("MaskingInformation", tablews);
    TS_ASSERT_THROWS_NOTHING(maskalg.execute());
    TS_ASSERT(maskalg.isExecuted());

    // 4. Check
    WS = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(workspaceName));
    TS_ASSERT(WS);
    for (int wi = 1; wi <= 3; wi++) {
      for (int bin = 3; bin < 6; bin++) {
        TS_ASSERT_EQUALS(WS->y(wi)[bin], 0.0);
      }
    }

    // 5. Clean
    AnalysisDataService::Instance().remove(workspaceName);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test to mask detectors by detectors IDs
    */
  void test_maskBinWithDetectorIDsList() {
    // Create a workspace to mask: 5 spectra, 10 bins
    const std::string workspaceName("raggedMask5");
    int nBins = 10;
    MatrixWorkspace_sptr dataws =
        WorkspaceCreationHelper::create2DWorkspaceBinned(5, nBins, 0.0);
    AnalysisDataService::Instance().add(workspaceName, dataws);

    // Find out mapping between spectra/workspace indexes and detectors IDs
    for (size_t i = 0; i < 5; ++i) {
      for (const auto &id : dataws->getSpectrum(i).getDetectorIDs())
        cout << "WorkspaceIndex = " << i << ":  Detector ID = " << id << ".\n";
    }

    // Generate a TableWorksapce
    auto tablews = boost::make_shared<TableWorkspace>();
    tablews->addColumn("str", "DetectorIDsList");
    tablews->addColumn("double", "XMin");
    tablews->addColumn("double", "XMax");
    AnalysisDataService::Instance().addOrReplace("MaskInfoTable", tablews);

    API::TableRow row0 = tablews->appendRow();
    row0 << "2-4" << 3.0 << 6.0;

    // Call the algorithm
    MaskBinsFromTable maskalg;
    TS_ASSERT_THROWS_NOTHING(maskalg.initialize());
    maskalg.setPropertyValue("InputWorkspace", workspaceName);
    maskalg.setPropertyValue("OutputWorkspace", workspaceName);
    maskalg.setProperty("MaskingInformation", "MaskInfoTable");
    TS_ASSERT_THROWS_NOTHING(maskalg.execute());
    TS_ASSERT(maskalg.isExecuted());

    // Check
    MatrixWorkspace_sptr outws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(workspaceName));
    TS_ASSERT(outws);
    for (int wi = 1; wi <= 3; wi++) {
      for (int bin = 3; bin < 6; bin++) {
        TS_ASSERT_EQUALS(outws->y(wi)[bin], 0.0);
      }
    }

    // Clean
    AnalysisDataService::Instance().remove("raggedMask5");
    AnalysisDataService::Instance().remove("MaskInfoTable");
  }
};

#endif /* MANTID_ALGORITHMS_MASKDETECTORBINSTEST_H_ */
