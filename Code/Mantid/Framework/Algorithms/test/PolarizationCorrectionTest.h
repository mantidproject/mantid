#ifndef MANTID_ALGORITHMS_POLARIZATIONCORRECTION_TEST_H_
#define MANTID_ALGORITHMS_POLARIZATIONCORRECTION_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/PolarizationCorrection.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include <boost/make_shared.hpp>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace WorkspaceCreationHelper;

class PolarizationCorrectionTest: public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PolarizationCorrectionTest *createSuite()
  {
    return new PolarizationCorrectionTest();
  }
  static void destroySuite(PolarizationCorrectionTest *suite)
  {
    delete suite;
  }

  void test_Init()
  {
    PolarizationCorrection alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
  }

  void test_set_wrong_workspace_type_throws()
  {
    MatrixWorkspace_sptr ws = boost::make_shared<Workspace2D>();
    PolarizationCorrection alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize());
    TS_ASSERT_THROWS( alg.setProperty("InputWorkspace", ws), std::invalid_argument&);
  }

  void test_set_analysis_to_PA()
  {
    PolarizationCorrection alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize());
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("PolarizationAnalysis", "PA"));
  }

  void test_set_analysis_to_PNR()
  {
    PolarizationCorrection alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("PolarizationAnalysis", "PNR"));
  }

  void test_set_analysis_to_invalid_throws()
  {
    PolarizationCorrection alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT_THROWS( alg.setProperty("PolarizationAnalysis", "_"), std::invalid_argument&);
  }

  Mantid::API::WorkspaceGroup_sptr makeWorkspaceGroup()
  {
    Mantid::API::WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    return group;
  }

  void test_throw_if_PA_and_group_is_wrong_size_throws()
  {
    Mantid::API::WorkspaceGroup_sptr inWS = boost::make_shared<WorkspaceGroup>(); // Empty group ws.

    // Name of the output workspace.
    std::string outWSName("PolarizationCorrectionTest_OutputWS");

    PolarizationCorrection alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("PolarizationAnalysis", "PA");
    alg.setPropertyValue("CRho", "1,1,1,1");
    alg.setPropertyValue("CAlpha", "1,1,1,1");
    alg.setPropertyValue("CAp", "1,1,1,1");
    alg.setPropertyValue("CPp", "1,1,1,1");

    alg.setPropertyValue("OutputWorkspace", outWSName);
    TSM_ASSERT_THROWS("Wrong number of grouped workspaces, should throw", alg.execute(),
        std::invalid_argument&);
  }

  void test_throw_if_PNR_and_group_is_wrong_size_throws()
  {
    Mantid::API::WorkspaceGroup_sptr inWS = boost::make_shared<WorkspaceGroup>(); // Empty group ws.

    // Name of the output workspace.
    std::string outWSName("PolarizationCorrectionTest_OutputWS");

    PolarizationCorrection alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("PolarizationAnalysis", "PNR");
    alg.setPropertyValue("OutputWorkspace", outWSName);
    alg.setPropertyValue("CRho", "1,1,1,1");
    alg.setPropertyValue("CAlpha", "1,1,1,1");
    alg.setPropertyValue("CAp", "1,1,1,1");
    alg.setPropertyValue("CPp", "1,1,1,1");
    TSM_ASSERT_THROWS("Wrong number of grouped workspaces, should throw", alg.execute(),
        std::invalid_argument&);
  }

  void test_throw_group_contains_other_workspace_types()
  {
    Mantid::API::WorkspaceGroup_sptr inWS = boost::make_shared<WorkspaceGroup>(); // Empty group ws.

    inWS->addWorkspace(boost::make_shared<TableWorkspace>()); // Group now contains a table workspace

    // Name of the output workspace.
    std::string outWSName("PolarizationCorrectionTest_OutputWS");

    PolarizationCorrection alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("PolarizationAnalysis", "PNR");
    alg.setPropertyValue("OutputWorkspace", outWSName);
    alg.setPropertyValue("CRho", "1,1,1,1");
    alg.setPropertyValue("CAlpha", "1,1,1,1");
    alg.setPropertyValue("CAp", "1,1,1,1");
    alg.setPropertyValue("CPp", "1,1,1,1");
    TSM_ASSERT_THROWS("Wrong workspace types in group", alg.execute(), std::invalid_argument&);
  }

  MatrixWorkspace_sptr create1DWorkspace(int size, double signal, double error)
  {
    auto ws = Create1DWorkspaceConstant(size, signal, error);
    ws->getAxis(0)->setUnit("Wavelength");
    return ws;
  }

  // If polynomials are unity, no changes should be made.
  void test_run_PA_unity()
  {
    auto groupWS = boost::make_shared<WorkspaceGroup>(); // Empty group ws.
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));

    PolarizationCorrection alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", groupWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setProperty("PolarizationAnalysis", "PA");
    alg.setPropertyValue("CRho", "1,0,0,0");
    alg.setPropertyValue("CAlpha", "1,0,0,0");
    alg.setPropertyValue("CAp", "1,0,0,0");
    alg.setPropertyValue("CPp", "1,0,0,0");
    alg.execute();
    WorkspaceGroup_sptr outWS = alg.getProperty("OutputWorkspace");

    TSM_ASSERT_EQUALS("Wrong number of output workspaces", outWS->size(), groupWS->size());

    for (size_t i = 0; i < outWS->size(); ++i)
    {
      std::cout << "Checking equivalent workspaces at index : " << i << std::endl;
      auto checkAlg = AlgorithmManager::Instance().createUnmanaged("CheckWorkspacesMatch");
      checkAlg->initialize();
      checkAlg->setChild(true);
      checkAlg->setProperty("Workspace1", groupWS->getItem(i));
      checkAlg->setProperty("Workspace2", outWS->getItem(i));
      checkAlg->setProperty("Tolerance", 3e-16);
      checkAlg->execute();
      const std::string result = checkAlg->getProperty("Result");
      TS_ASSERT_EQUALS("Success!", result);
    }

  }

  void test_run_PNR_unity()
  {
    auto groupWS = boost::make_shared<WorkspaceGroup>(); // Empty group ws.
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));
    groupWS->addWorkspace(create1DWorkspace(4, 1, 1));

    PolarizationCorrection alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", groupWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setProperty("PolarizationAnalysis", "PNR");
    alg.setPropertyValue("CRho", "1,0,0,0");
    alg.setPropertyValue("CPp", "1,0,0,0");
    alg.execute();
    WorkspaceGroup_sptr outWS = alg.getProperty("OutputWorkspace");

    TSM_ASSERT_EQUALS("Wrong number of output workspaces", outWS->size(), groupWS->size());

    for (size_t i = 0; i < outWS->size(); ++i)
    {
      std::cout << "Checking equivalent workspaces at index : " << i << std::endl;
      auto checkAlg = AlgorithmManager::Instance().createUnmanaged("CheckWorkspacesMatch");
      checkAlg->initialize();
      checkAlg->setChild(true);
      checkAlg->setProperty("Workspace1", groupWS->getItem(i));
      checkAlg->setProperty("Workspace2", outWS->getItem(i));
      checkAlg->execute();
      const std::string result = checkAlg->getProperty("Result");
      TS_ASSERT_EQUALS("Success!", result);
    }

  }

};

#endif /* MANTID_ALGORITHMS_POLARIZATIONCORRECTION_TEST_H_ */
