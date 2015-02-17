#ifndef MANTID_MDALGORITHMS_WEIGHTEDMEANMDTEST_H_
#define MANTID_MDALGORITHMS_WEIGHTEDMEANMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidMDAlgorithms/WeightedMeanMD.h"

using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class WeightedMeanMDTest : public CxxTest::TestSuite
{

private:

  /// Helper method to create and return a matrix workspace.
  MatrixWorkspace_sptr create_matrix_workspace(const std::vector<double>& s, const std::vector<double>& e, const std::vector<double>& x, const std::string& name)
  {
    // Create and run the algorithm
    AnalysisDataServiceImpl& ADS = Mantid::API::AnalysisDataService::Instance();
    IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
    alg->initialize();
    alg->setProperty("NSpec", 1);
    alg->setProperty("DataY", s );
    alg->setProperty("DataX", x );
    alg->setProperty("DataE", e );
    alg->setPropertyValue("UnitX", "Wavelength");
    alg->setProperty("OutputWorkspace", name);
    alg->execute();
    // Return the generated MDHistoWorkspace
    return boost::dynamic_pointer_cast<MatrixWorkspace>(ADS.retrieve(name));
  }

  /// Helper method to run the WeightedMean algorithm on two matrix workspaces and return the result.
  MatrixWorkspace_sptr run_matrix_weighed_mean(MatrixWorkspace_sptr a, MatrixWorkspace_sptr b, const std::string& name)
  {
    AnalysisDataServiceImpl& ADS = Mantid::API::AnalysisDataService::Instance();
    IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("WeightedMean");
    alg->setRethrows(true);
    alg->initialize();
    alg->setProperty("InputWorkspace1", a);
    alg->setProperty("InputWorkspace2", b);
    alg->setProperty("OutputWorkspace", name);
    alg->execute();
    return boost::dynamic_pointer_cast<MatrixWorkspace>( ADS.retrieve(name) );
  }


  /// Helper method to run input type validation checks.
  void do_test_workspace_input_types(IMDWorkspace_sptr a, IMDWorkspace_sptr b)
  {
    WeightedMeanMD alg;
    alg.initialize();

    std::string outName("out_ws");
    alg.setRethrows(true);
    alg.setProperty("LHSWorkspace", a);
    alg.setProperty("RHSWorkspace", b);
    alg.setPropertyValue("OutputWorkspace", outName );
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WeightedMeanMDTest *createSuite() { return new WeightedMeanMDTest(); }
  static void destroySuite( WeightedMeanMDTest *suite ) { delete suite; }


  void test_Init()
  {
    WeightedMeanMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_LHS_mdevent_workspace_throws()
  {
    //Create an MDEventWorkspace, this is not an allowed type.
    IMDEventWorkspace_sptr a = MDEventsTestHelper::makeMDEW<2>(3, 0.0, 10.0, 1);
    //Create an MDHistoWorkspace 
    MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1, 10, 10.0, 1.0, "A");

    do_test_workspace_input_types(a, b);

  }

  void test_RHS_mdevent_workspace_throws()
  {
    //Create an MDEventWorkspace, this is not an allowed type.
    IMDEventWorkspace_sptr b = MDEventsTestHelper::makeMDEW<2>(3, 0.0, 10.0, 1);
    //Create an MDHistoWorkspace 
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1, 10, 10.0, 1.0, "A");

    do_test_workspace_input_types(a, b);
  }

  void test_RHS_and_LHS_mdevent_workspace_throws()
  {
    //Create an MDEventWorkspace, this is not an allowed type.
    IMDEventWorkspace_sptr a = MDEventsTestHelper::makeMDEW<2>(3, 0.0, 10.0, 1);
    //Create an MDEventWorkspace, this is not an allowed type.
    IMDEventWorkspace_sptr b = MDEventsTestHelper::makeMDEW<2>(3, 0.0, 10.0, 1);

    do_test_workspace_input_types(a, b);
  }

  void test_executes_1D()
  {
    WeightedMeanMD alg;
    alg.initialize();

    //Create an MDHisto workspace with signal=1, dimensionality=1, bins=10, max=10, error=1, dimname="A"
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1, 10, 10.0, 1.0, "A");
    //Create an MDHisto workspace with signal=1, dimensionality=1, bins=10, max=10, error=1, dimname="A"
    MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1, 10, 10.0, 1.0, "A");

    std::string outName("out_ws");
    alg.setRethrows(true);
    alg.setProperty("LHSWorkspace", a);
    alg.setProperty("RHSWorkspace", b);
    alg.setPropertyValue("OutputWorkspace", outName );

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    AnalysisDataServiceImpl& ADS = Mantid::API::AnalysisDataService::Instance();
    MDHistoWorkspace_sptr c = boost::dynamic_pointer_cast<MDHistoWorkspace>(ADS.retrieve(outName));
    
    TS_ASSERT(c != NULL);
    //Since A and B are equivalent, the mean Signal in C should be the same as both A and B.
    double expectedSignalResults[10]={1,1,1,1,1,1,1,1,1,1};
    //Errors = sqrt(0.5) as errors are stored squared and since error is the harmoic mean of e1 and e2. i.e. sqrt(e1 * e2 / (e1 + e2))
    double expectedErrorResults[10]={0.7071,0.7071,0.7071,0.7071,0.7071,0.7071,0.7071,0.7071,0.7071,0.7071};
    for(size_t i = 0; i < 10; ++i)
    {
      TS_ASSERT_EQUALS(expectedSignalResults[i], c->getSignalAt(i));
      TS_ASSERT_EQUALS(a->getSignalAt(i), c->getSignalAt(i));
      TS_ASSERT_EQUALS(b->getSignalAt(i), c->getSignalAt(i));
      TS_ASSERT_DELTA(expectedErrorResults[i], c->getErrorAt(i), 0.001);
    }

    ADS.remove(outName);
  }

  /// Check that things work with dimensionality other than 1 (tested above).
  void test_works_2D()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("WeightedMeanMD", "histo_A", "histo_B", "out");
    TS_ASSERT(out != NULL);
  }

  /**
  This is more of an integration test, but it's quite useful because it executes both WeightedMean and WeightedMeanMD on the same data and 
  compares the results.
  */
  MDHistoWorkspace_sptr create_md_histo_workspace(const std::vector<double>& s, const std::vector<double>& e, const std::vector<double>& x, const std::string& name)
  {
    // Fabricate some additional inputs
    std::vector<int> nbins(1, static_cast<int>(x.size()));
    std::vector<double> extents(2);
    extents[0] = x.front();
    extents[1] = x.back();

    // Create and run the algorithm
    AnalysisDataServiceImpl& ADS = Mantid::API::AnalysisDataService::Instance();
    IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("CreateMDHistoWorkspace");
    alg->initialize();
    alg->setProperty("Dimensionality", 1);
    alg->setProperty("SignalInput", s );
    alg->setProperty("ErrorInput", e );
    alg->setProperty("NumberOfBins", nbins);
    alg->setProperty("Extents", extents);
    alg->setProperty("Names", "A" );
    alg->setProperty("Units", "U");
    alg->setProperty("OutputWorkspace", name);
    alg->execute();
    // Return the generated MDHistoWorkspace
    return boost::dynamic_pointer_cast<MDHistoWorkspace>(ADS.retrieve(name));
  }

  /// Compare the outputs from this algorithm to the equivalent for MatrixWorkspaces (WeightedMean).
  void test_compare_to_matrix_weightedmean()
  {
    //Create some input data. Signal values as two offset sine waves.
    typedef std::vector<double> VecDouble;
    VecDouble s1, s2, e1, e2, x;
    double theta_shift=0.4;
    for(size_t i = 0; i < 40; ++i)
    {
      double theta = 0.02 * double(i) * M_PI;
      s1.push_back(std::sin(theta));
      e1.push_back(std::sin(theta));
      s2.push_back(std::sin(theta+theta_shift));
      e2.push_back(std::sin(theta+theta_shift));
      x.push_back(double(i));
    }

    // Create Input MDWorkspaces
    MDHistoWorkspace_sptr a_md_histo = create_md_histo_workspace(s1, e1, x, "a_md_histo");
    MDHistoWorkspace_sptr b_md_histo = create_md_histo_workspace(s2, e2, x, "b_md_histo");

    // Create the equivalent MatrixWorkspaces
    MatrixWorkspace_sptr a_matrix_ws = create_matrix_workspace(s1, e1, x, "a_matrix_workspace");
    MatrixWorkspace_sptr b_matrix_ws = create_matrix_workspace(s2, e2, x, "b_matrix_workspace");

    // Calculate the weighted mean for the matrix workspace.
    MatrixWorkspace_sptr weighted_mean_matrix = run_matrix_weighed_mean(a_matrix_ws, b_matrix_ws, "weighted_mean_matrix");

    std::string outName("weighted_mean_md");
    WeightedMeanMD alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setProperty("LHSWorkspace", a_md_histo);
    alg.setProperty("RHSWorkspace", b_md_histo);
    alg.setPropertyValue("OutputWorkspace", outName );
    alg.execute();

    AnalysisDataServiceImpl& ADS = Mantid::API::AnalysisDataService::Instance();
    MDHistoWorkspace_sptr weighted_mean_md = boost::dynamic_pointer_cast<MDHistoWorkspace>(ADS.retrieve(outName));
    
    //Check the outputs between both weighted mean algorithms.
    for(size_t j = 0; j < s1.size(); ++j)
    {
		 TS_ASSERT_DELTA(weighted_mean_matrix->readY(0)[j], weighted_mean_md->signalAt(j), 0.0001);
     TS_ASSERT_DELTA(std::pow(weighted_mean_matrix->readE(0)[j], 2) , weighted_mean_md->errorSquaredAt(j), 0.0001);
    }

    // Clean up.
    ADS.remove("a_md_histo");
    ADS.remove("b_md_histo");
    ADS.remove("a_matrix_workspace");
    ADS.remove("b_matrix_workspace");
    ADS.remove(outName);
    ADS.remove("weighted_mean_matrix");
  }

};


#endif /* MANTID_MDALGORITHMS_WEIGHTEDMEANMDTEST_H_ */
