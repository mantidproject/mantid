#ifndef MANTID_ALGORITHMS_POLARISATIONCORRECTIONTEST_H_
#define MANTID_ALGORITHMS_POLARISATIONCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/PolarisationCorrection.h"
#include "MantidDataObjects/Workspace2D.h"
#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using Mantid::DataObjects::Workspace2D;

class PolarisationCorrectionTest: public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PolarisationCorrectionTest *createSuite()
  {
    return new PolarisationCorrectionTest();
  }
  static void destroySuite(PolarisationCorrectionTest *suite)
  {
    delete suite;
  }

  void test_Init()
  {
    PolarisationCorrection alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
  }

  void test_set_wrong_workspace_type_throws()
  {
    MatrixWorkspace_sptr ws = boost::make_shared<Workspace2D>();
    PolarisationCorrection alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize());
    TS_ASSERT_THROWS( alg.setProperty("InputWorkspace", ws), std::invalid_argument& );
  }

  void test_set_analysis_to_PA()
  {
    PolarisationCorrection alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize());
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("PolarisationAnalysis", "PA"));
  }

  void test_set_analysis_to_PNR()
  {
    PolarisationCorrection alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("PolarisationAnalysis", "PNR"));
  }

  void test_set_analysis_to_invalid_throws()
  {
    PolarisationCorrection alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT_THROWS( alg.setProperty("PolarisationAnalysis", "_"), std::invalid_argument&);
  }

  Mantid::API::WorkspaceGroup_sptr makeWorkspaceGroup(const size_t n_workspaces)
  {
    Mantid::API::WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    return group;
  }

  void test_throw_if_PA_and_group_is_wrong_size_throws()
  {
    Mantid::API::WorkspaceGroup_sptr inWS = boost::make_shared<WorkspaceGroup>(); // Empty group ws.

    // Name of the output workspace.
    std::string outWSName("PolarisationCorrectionTest_OutputWS");

    PolarisationCorrection alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("PolarisationAnalysis", "PA");
    alg.setPropertyValue("OutputWorkspace", outWSName);
    TSM_ASSERT_THROWS("Wrong number of grouped workspaces, should throw", alg.execute(),
        std::invalid_argument&);
  }

  void test_throw_if_PNR_and_group_is_wrong_size_throws()
  {
    Mantid::API::WorkspaceGroup_sptr inWS = boost::make_shared<WorkspaceGroup>(); // Empty group ws.

    // Name of the output workspace.
    std::string outWSName("PolarisationCorrectionTest_OutputWS");

    PolarisationCorrection alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("PolarisationAnalysis", "PNR");
    alg.setPropertyValue("OutputWorkspace", outWSName);
    TSM_ASSERT_THROWS("Wrong number of grouped workspaces, should throw", alg.execute(),
        std::invalid_argument&);
  }

};

#endif /* MANTID_ALGORITHMS_POLARISATIONCORRECTIONTEST_H_ */
