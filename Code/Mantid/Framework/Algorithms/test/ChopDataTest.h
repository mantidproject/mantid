#ifndef ALG_CHOPDATA_TEST_H_
#define ALG_CHOPDATA_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ChopData.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;

class ChopDataTest : public CxxTest::TestSuite
{

public:
  ChopDataTest() {}
  ~ChopDataTest() {}

  void testMetaInfo()
  {
    alg = new ChopData();
    TS_ASSERT_EQUALS(alg->name(), "ChopData");
    TS_ASSERT_EQUALS(alg->version(), 1);
    TS_ASSERT_EQUALS(alg->category(), "General");
    delete alg;
  }

  void testInit()
  {
    alg = new ChopData();
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT(alg->isInitialized());
    delete alg;
  }

  void testExec()
  {
    Mantid::DataObjects::Workspace2D_sptr inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(149, 24974, 5, 4);

    for ( int i = 0; i < 4995; i++ )
    {
      inputWS->dataX(140)[i+19980] = 0.2;
    }

    inputWS->getAxis(0)->setUnit("TOF");

    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().add("chopdatatest_input", inputWS));

    const int nHist = inputWS->getNumberHistograms();

    TS_ASSERT_EQUALS(nHist, 149);

    alg = new ChopData();
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("InputWorkspace", "chopdatatest_input"));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", "chopdatatest_output"));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // Get workspace group output by algorithm
    Mantid::API::WorkspaceGroup_sptr wsgroup;
    TS_ASSERT_THROWS_NOTHING(wsgroup = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(Mantid::API::AnalysisDataService::Instance().retrieve("chopdatatest_output")));

    TS_ASSERT_EQUALS(wsgroup->getNumberOfEntries(), 4);

    std::vector<std::string> wsnames = wsgroup->getNames();

    Mantid::API::MatrixWorkspace_const_sptr output1;
    Mantid::API::MatrixWorkspace_const_sptr output4;

    TS_ASSERT_THROWS_NOTHING(
      output1 = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
      (Mantid::API::AnalysisDataService::Instance().retrieve(wsnames[0]))
      );

    TS_ASSERT_THROWS_NOTHING(
      output4 = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
      (Mantid::API::AnalysisDataService::Instance().retrieve(wsnames[3]))
      );

    
    TS_ASSERT_EQUALS(output1->getNumberHistograms(), nHist);
    TS_ASSERT_EQUALS(output4->getNumberHistograms(), nHist);

    TS_ASSERT_EQUALS(output1->blocksize(), 4998);
    TS_ASSERT_EQUALS(output4->blocksize(), 9975);

    TS_ASSERT(output1->readX(0)[4997] < output4->readX(0)[9975]);

    delete alg;

    // Cleanup
    AnalysisDataService::Instance().remove("chopdatatest_input");
    wsgroup->deepRemoveAll();
    AnalysisDataService::Instance().remove("chopdatatest_output");
  }

private:
  ChopData* alg;

};

#endif