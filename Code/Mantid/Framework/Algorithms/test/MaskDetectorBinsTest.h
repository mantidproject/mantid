#ifndef MANTID_ALGORITHMS_MASKDETECTORBINSTEST_H_
#define MANTID_ALGORITHMS_MASKDETECTORBINSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/MaskDetectorBins.h"
#include "MantidAPI/TableRow.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class MaskDetectorBinsTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaskDetectorBinsTest *createSuite() { return new MaskDetectorBinsTest(); }
  static void destroySuite( MaskDetectorBinsTest *suite ) { delete suite; }

  /*
   * In-place single mask test.
   * Same as the test in MaskBins()
   */
  void test_MaskBinWithSingleLine()
  {
    // 1. Create a dummy workspace
    const std::string workspaceName("raggedMask");
    int nBins = 10;
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::Create2DWorkspaceBinned(5, nBins, 0.0);
    AnalysisDataService::Instance().add(workspaceName,WS);

    // 2. Generate a TableWorskpace
    DataObjects::TableWorkspace_sptr tablews = boost::shared_ptr<DataObjects::TableWorkspace>(new DataObjects::TableWorkspace());
    tablews->addColumn("double", "XMin");
    tablews->addColumn("double", "XMax");
    tablews->addColumn("str", "SpectraList");

    API::TableRow row0 = tablews->appendRow();
    row0 << 3.0 << 6.0 << "1-3";

    // 3. Execute
    MaskDetectorBins maskalg;
    TS_ASSERT_THROWS_NOTHING(maskalg.initialize());
    maskalg.setPropertyValue("InputWorkspace", workspaceName);
    maskalg.setPropertyValue("OutputWorkspace",workspaceName);
    maskalg.setProperty("MaskingInformation", tablews);
    TS_ASSERT_THROWS_NOTHING(maskalg.execute());
    TS_ASSERT(maskalg.isExecuted());

    // 4. Check
    WS = boost::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve(workspaceName));
    TS_ASSERT(WS);
    for (int wi=1; wi<=3; wi++)
    {
      for (int bin=3; bin<6;bin++)
      {
        TS_ASSERT_EQUALS( WS->dataY(wi)[bin], 0.0 );
      }
    }

    // 5. Clean
    AnalysisDataService::Instance().remove(workspaceName);

    return;
  }

  /*
   * Out-of-place single mask test.
   * Same as the test in MaskBins()
   */
  void test_MaskBinWithSingleLineOutPlace()
  {
    // 1. Create a dummy workspace
    const std::string workspaceName("raggedMask");
    const std::string opWSName("maskedWorkspace");
    int nBins = 10;
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::Create2DWorkspaceBinned(5, nBins, 0.0);
    AnalysisDataService::Instance().add(workspaceName,WS);

    // 2. Generate a TableWorskpace
    DataObjects::TableWorkspace_sptr tablews = boost::shared_ptr<DataObjects::TableWorkspace>(new DataObjects::TableWorkspace());
    tablews->addColumn("double", "XMin");
    tablews->addColumn("double", "XMax");
    tablews->addColumn("str", "SpectraList");

    API::TableRow row0 = tablews->appendRow();
    row0 << 3.0 << 6.0 << "1-3";

    // 3. Execute
    MaskDetectorBins maskalg;
    TS_ASSERT_THROWS_NOTHING(maskalg.initialize());
    maskalg.setPropertyValue("InputWorkspace", workspaceName);
    maskalg.setPropertyValue("OutputWorkspace",opWSName);
    maskalg.setProperty("MaskingInformation", tablews);
    TS_ASSERT_THROWS_NOTHING(maskalg.execute());
    TS_ASSERT(maskalg.isExecuted());

    // 4. Check
    MatrixWorkspace_sptr outWS =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve(opWSName));
    TS_ASSERT(outWS);
    for (int wi=1; wi<=3; wi++)
    {
      for (int bin=3; bin<6;bin++)
      {
        TS_ASSERT_EQUALS( outWS->dataY(wi)[bin], 0.0 );
      }
    }

    // 5. Clean
    AnalysisDataService::Instance().remove(workspaceName);
    AnalysisDataService::Instance().remove(opWSName);

    return;
  }


  /*
   * Multiple lines out-of-place test.
   * This is a real test
   */
  void test_MaskBinWithMultiLines()
  {
    // 1. Create a dummy workspace
    const std::string workspaceName("raggedMask");
    int nBins = 10;
    int nHist = 12;
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::Create2DWorkspaceBinned(nHist, nBins, 0.0);
    AnalysisDataService::Instance().add(workspaceName,WS);

    // 2. Generate a TableWorskpace
    DataObjects::TableWorkspace_sptr tablews = boost::shared_ptr<DataObjects::TableWorkspace>(new DataObjects::TableWorkspace());
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
    MaskDetectorBins maskalg;
    TS_ASSERT_THROWS_NOTHING(maskalg.initialize());
    maskalg.setPropertyValue("InputWorkspace", workspaceName);
    maskalg.setPropertyValue("OutputWorkspace",workspaceName);
    maskalg.setProperty("MaskingInformation", tablews);
    TS_ASSERT_THROWS_NOTHING(maskalg.execute());
    TS_ASSERT(maskalg.isExecuted());

    // 4. Check
    WS = boost::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve(workspaceName));
    TS_ASSERT(WS);

    // a) Table Line 0
    for (int wi=1; wi<=3; wi++)
    {
      for (size_t bin = 0; bin < WS->dataY(wi).size(); ++bin)
      {
        if (bin >= 3 && bin < 6)
        {
          TS_ASSERT_EQUALS( WS->dataY(wi)[bin], 0.0 );
        }
        else
        {
          TS_ASSERT_EQUALS( WS->dataY(wi)[bin], 2.0 );
        }
      }
    }

    // b) Table Line 1
    std::vector<int> speclist;
    speclist.push_back(5);
    speclist.push_back(6);
    speclist.push_back(7);
    speclist.push_back(8);
    for (size_t iws = 0; iws < speclist.size(); ++iws)
    {
      const MantidVec& yvec = WS->readY(speclist[iws]);
      for (size_t bin = 0; bin < yvec.size(); ++bin)
      {
        if (bin >= 4 && bin < 7)
        {
          TS_ASSERT_EQUALS(yvec[bin], 0.0);
        }
        else
        {
          TS_ASSERT_EQUALS(yvec[bin], 2.0);
        }
      }
    }

    // c) Table Line 2
    for (size_t iws = 9; iws < 10; ++iws)
    {
      const MantidVec& yvec = WS->readY(iws);
      for (size_t bin = 0; bin < yvec.size(); ++bin)
      {
        if (bin == 0)
        {
          TS_ASSERT_EQUALS(yvec[bin], 0.0);
        }
        else
        {
          TS_ASSERT_EQUALS(yvec[bin], 2.0);
        }
      }
    }

    // 5. Clean
    AnalysisDataService::Instance().remove(workspaceName);

    return;
  }

};


#endif /* MANTID_ALGORITHMS_MASKDETECTORBINSTEST_H_ */
