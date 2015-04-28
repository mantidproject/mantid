#ifndef MANTID_MDALGORITHMS_INTEGRATEMDHISTOWORKSPACETEST_H_
#define MANTID_MDALGORITHMS_INTEGRATEMDHISTOWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include <boost/assign/list_of.hpp>

#include "MantidMDAlgorithms/IntegrateMDHistoWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using Mantid::MDAlgorithms::IntegrateMDHistoWorkspace;
using namespace Mantid::API;

//=====================================================================================
// Functional Tests
//=====================================================================================
class IntegrateMDHistoWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegrateMDHistoWorkspaceTest *createSuite() { return new IntegrateMDHistoWorkspaceTest(); }
  static void destroySuite( IntegrateMDHistoWorkspaceTest *suite ) { delete suite; }


  void test_Init()
  {
    IntegrateMDHistoWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );
  }

  void test_throw_if_new_steps_in_binning()
  {
    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1 /*nd*/, 10);

    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    const double step = 0.1;
    alg.setProperty("P1Bin", boost::assign::list_of(0.0)(step)(1.0).convert_to_container<std::vector<double> >());
    alg.setPropertyValue("OutputWorkspace", "dummy");
    TSM_ASSERT("Expect validation errors", alg.validateInputs().size() > 0);
    TSM_ASSERT_THROWS("No new steps allowed", alg.execute(), std::runtime_error&);
  }

  void test_throw_if_incorrect_binning_limits_when_integrating()
  {
    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1 /*nd*/, 10);

    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", "dummy");

    const double min = 3;

    // Test equal to
    double max = min;
    alg.setProperty("P1Bin", boost::assign::list_of(min)(max).convert_to_container<std::vector<double> >());
    TSM_ASSERT("Expect validation errors", alg.validateInputs().size() > 0);
    TSM_ASSERT_THROWS("Incorrect limits", alg.execute(), std::runtime_error&);

    // Test less than
    max = min - 0.01;
    alg.setProperty("P1Bin", boost::assign::list_of(min)(max).convert_to_container<std::vector<double> >());
    TSM_ASSERT("Expect validation errors", alg.validateInputs().size() > 0);
    TSM_ASSERT_THROWS("Incorrect limits", alg.execute(), std::runtime_error&);
  }

  void test_throw_if_incorrect_binning_limits_when_similar()
  {
    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1 /*nd*/, 10);

    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", "dummy");

    const double min = 3;
    double step = 0;
    // Test equal to
    double max = min;
    alg.setProperty("P1Bin", boost::assign::list_of(min)(step)(max).convert_to_container<std::vector<double> >());
    TSM_ASSERT("Expect validation errors", alg.validateInputs().size() > 0);
    TSM_ASSERT_THROWS("Incorrect limits", alg.execute(), std::runtime_error&);

    // Test less than
    max = min - 0.01;
    alg.setProperty("P1Bin", boost::assign::list_of(min)(step)(max).convert_to_container<std::vector<double> >());
    TSM_ASSERT("Expect validation errors", alg.validateInputs().size() > 0);
    TSM_ASSERT_THROWS("Incorrect limits", alg.execute(), std::runtime_error&);

    // Test non-zero step. ZERO means copy!
    max = min - 0.01;
    alg.setProperty("P1Bin", boost::assign::list_of(min)(1.0)(max).convert_to_container<std::vector<double> >());
    TSM_ASSERT("Expect validation errors", alg.validateInputs().size() > 0);
    TSM_ASSERT_THROWS("Step has been specified", alg.execute(), std::runtime_error&);
  }

  // Users may set all binning parameter to [] i.e. direct copy, no integration.
  void test_exec_do_nothing_but_clone()
  {
    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1 /*nd*/, 10);

    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS=alg.getProperty("OutputWorkspace");

    // Quick check that output seems to be a copy of input.
    TS_ASSERT_EQUALS(outWS->getNPoints(), ws->getNPoints());
    TS_ASSERT_EQUALS(outWS->getNumDims(), ws->getNumDims());
    TS_ASSERT_EQUALS(outWS->getSignalAt(0), ws->getSignalAt(0));
    TS_ASSERT_EQUALS(outWS->getSignalAt(1), ws->getSignalAt(1));
  }

  void test_1D_integration_exact_binning()
  {

      /*

                           input
      (x = 0) *|--|--|--|--|--|--|--|--|--|--|* (x = 10)
                1  1  1  1  1  1  1  1  1  1

                  output requested

      (x = 0) *|--------------|* (x = 5)
                1 + 1 + 1 + 1 + 1 = 5 counts

      */

      using namespace Mantid::DataObjects;
      MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0 /*signal*/, 1 /*nd*/, 10 /*nbins*/, 10 /*max*/, 1.0 /*error sq*/);

      IntegrateMDHistoWorkspace alg;
      alg.setChild(true);
      alg.setRethrows(true);
      alg.initialize();
      alg.setProperty("InputWorkspace", ws);
      const double min = 0;
      const double max = 5;
      alg.setProperty("P1Bin", boost::assign::list_of(min)(max).convert_to_container<std::vector<double> >());
      alg.setPropertyValue("OutputWorkspace", "dummy");
      alg.execute();
      IMDHistoWorkspace_sptr outWS=alg.getProperty("OutputWorkspace");

      // Quick check that output seems to have the right shape.
      TSM_ASSERT_EQUALS("All integrated", 1, outWS->getNPoints());
      auto dim = outWS->getDimension(0);
      TS_ASSERT_EQUALS(min, dim->getMinimum());
      TS_ASSERT_EQUALS(max, dim->getMaximum());

      // Check the data.
      TSM_ASSERT_DELTA("Wrong integrated value", 5.0, outWS->getSignalAt(0), 1e-4);
      TSM_ASSERT_DELTA("Wrong error value", std::sqrt(5 * (ws->getErrorAt(0) * ws->getErrorAt(0))), outWS->getErrorAt(0), 1e-4);
  }


  void test_1D_integration_partial_binning_complex(){

      /*

                           input
      (x = 0) *|--|--|--|--|--|--|--|--|--|--|* (x = 10)
                1  1  1  1  1  1  1  1  1  1

                  output requested

      (x = 0.75) *|--------------|* (x = 4.25)
                1/4 + 1 + 1 + 1 + 1/4 = 3.5 counts

      */


      using namespace Mantid::DataObjects;
      MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0 /*signal*/, 1 /*nd*/, 10 /*nbins*/, 10 /*max*/, 1.0 /*error sq*/);

      IntegrateMDHistoWorkspace alg;
      alg.setChild(true);
      alg.setRethrows(true);
      alg.initialize();
      alg.setProperty("InputWorkspace", ws);
      const double min = 0.75;
      const double max = 4.25;
      alg.setProperty("P1Bin", boost::assign::list_of(min)(max).convert_to_container<std::vector<double> >());
      alg.setPropertyValue("OutputWorkspace", "dummy");
      alg.execute();
      IMDHistoWorkspace_sptr outWS=alg.getProperty("OutputWorkspace");

      // Quick check that output seems to have the right shape.
      TSM_ASSERT_EQUALS("All integrated", 1, outWS->getNPoints());
      auto dim = outWS->getDimension(0);
      TS_ASSERT_EQUALS(min, dim->getMinimum());
      TS_ASSERT_EQUALS(max, dim->getMaximum());

      // Check the data.
      TSM_ASSERT_DELTA("Wrong integrated value", 3.5, outWS->getSignalAt(0), 1e-4);
      TSM_ASSERT_DELTA("Wrong error value", std::sqrt(3.5 * (ws->getErrorAt(0) * ws->getErrorAt(0))), outWS->getErrorAt(0), 1e-4);
  }

  void test_1D_integration_with_original_step_and_forbidden_partial_binning(){

      /*

                           input
      (x = 0) *|--|--|--|--|--|--|--|--|--|--|* (x = 10)
                1  1  1  1  1  1  1  1  1  1

                  output requested, but partial bins are forbidden so round to the nearest bin edges

      (x = 0.75) *|--------------|* (x = 4.25)
                1/4 , 1 , 1 , 1 , 1/4

                  output with rounding (maintain closest possible bin boundaries. no partial binning)

      (x = 0) *|--------------------|* (x = 5)
                 1 , 1 , 1 , 1 , 1, 1

      */


      using namespace Mantid::DataObjects;
      MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0 /*signal*/, 1 /*nd*/, 10 /*nbins*/, 10 /*max*/, 1.0 /*error sq*/);

      IntegrateMDHistoWorkspace alg;
      alg.setChild(true);
      alg.setRethrows(true);
      alg.initialize();
      alg.setProperty("InputWorkspace", ws);
      const double min = 0.75;
      const double max = 4.25;
      alg.setProperty("P1Bin", boost::assign::list_of(min)(0.0)(max).convert_to_container<std::vector<double> >());
      alg.setPropertyValue("OutputWorkspace", "dummy");
      alg.execute();
      IMDHistoWorkspace_sptr outWS=alg.getProperty("OutputWorkspace");

      // Quick check that output seems to have the right shape.
      TSM_ASSERT_EQUALS("Should have rounded to whole widths.", 5, outWS->getNPoints());
      auto outDim = outWS->getDimension(0);
      auto inDim = ws->getDimension(0);
      TS_ASSERT_EQUALS(0.0f, outDim->getMinimum());
      TS_ASSERT_EQUALS(5.0f, outDim->getMaximum());
      TSM_ASSERT_EQUALS("Bin width should be unchanged", inDim->getBinWidth(), outDim->getBinWidth());

      // Check the data.
      TSM_ASSERT_DELTA("Wrong value", 1.0, outWS->getSignalAt(0), 1e-4);
      TSM_ASSERT_DELTA("Wrong value", 1.0, outWS->getSignalAt(1), 1e-4);
      TSM_ASSERT_DELTA("Wrong value", 1.0, outWS->getSignalAt(2), 1e-4);
      TSM_ASSERT_DELTA("Wrong value", 1.0, outWS->getSignalAt(3), 1e-4);
      TSM_ASSERT_DELTA("Wrong value", 1.0, outWS->getSignalAt(4), 1e-4);

   }

  void test_2d_partial_binning() {

    /*

      Input filled with 1's binning = 1 in each dimension
      ----------------------------- (10, 10)
      |                           |
      |                           |
      |                           |
      |                           |
      |                           |
      |                           |
      |                           |
      |                           |
      |                           |
      |                           |
      -----------------------------
    (0, 0)


      Slice. Two vertical columns. Each 1 in width.

      ----------------------------- (10, 10)
      |                           |
      |                           |
      |__________________________ | (10, 7.1)
      |    |    |   ...           |
      |    |    |                 |
      |    |    |                 |
      |    |    |                 |
      |    |    |                 |
      |__________________________ | (10, 1.1)
      |                           |
      -----------------------------
    (0, 0)

    */

    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0 /*signal*/, 2 /*nd*/, 10 /*nbins*/, 10 /*max*/, 1.0 /*error sq*/);

    IntegrateMDHistoWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    const double min = 1.1;
    const double max = 7.1; // 7.1 - 1.1 = 6
    alg.setProperty("P1Bin", std::vector<double>(0)); // Pass through. Do not change binning.
    alg.setProperty("P2Bin", boost::assign::list_of(min)(max).convert_to_container<std::vector<double> >());
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS=alg.getProperty("OutputWorkspace");

    // Quick check that output seems to have the right shape.
    TSM_ASSERT_EQUALS("All integrated", 10, outWS->getNPoints()); // one dimension unchanged the other integrated
    auto intdim = outWS->getDimension(1);
    TS_ASSERT_DELTA(min, intdim->getMinimum(), 1e-4);
    TS_ASSERT_DELTA(max, intdim->getMaximum(), 1e-4);
    TS_ASSERT_EQUALS(1, intdim->getNBins());
    auto dim = outWS->getDimension(0);
    TSM_ASSERT_DELTA("Not integrated binning should be the same as the original dimension", 0, dim->getMinimum(), 1e-4);
    TSM_ASSERT_DELTA("Not integrated binning should be the same as the original dimension", 10, dim->getMaximum(), 1e-4);
    TSM_ASSERT_EQUALS("Not integrated binning should be the same as the original dimension", 10, dim->getNBins());

    // Check the data.
    TSM_ASSERT_DELTA("Wrong integrated value", 6.0, outWS->getSignalAt(0), 1e-4);
    TSM_ASSERT_DELTA("Wrong error value", std::sqrt(6.0 * (ws->getErrorAt(0) * ws->getErrorAt(0))), outWS->getErrorAt(0), 1e-4);
  }

  void test_update_n_events_for_normalization() {

      /*
                    input
      (x = 0) *|--|--|--|--|--|--|--|--|--|--|* (x = 10)
                1  2  3  4  5  6  7  8  9  10    (signal values in bins)
                1  2  3  4  5  6  7  8  9  10    (n_events in bins)

                  output requested

      (x = 0.75) *|--------------|* (x = 4.25)
                1/4 , 1 , 1 , 1 , 1/4 = weights based on fraction overlap
                1/4 + 2 + 3 + 4 + 5/4  (signal values in bins)
                1/4 + 2 + 3 + 4 + 5/4  (n_events in bins)


      */


      using namespace Mantid::DataObjects;
      MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0 /*signal*/, 1 /*nd*/, 10 /*nbins*/, 10 /*max*/, 1.0 /*error sq*/);
      // Fill signal and n-events as above
      for(size_t i = 0; i < ws->getNPoints(); ++i) {
        ws->setSignalAt(i, Mantid::signal_t(i+1));
        ws->setNumEventsAt(i, Mantid::signal_t(i+1));
        std::cout << "signal " << i+1 <<  "\t" << "nevents" <<  "\t" << "at" << "\t" << i << std::endl;
      }

      IntegrateMDHistoWorkspace alg;
      alg.setChild(true);
      alg.setRethrows(true);
      alg.initialize();
      alg.setProperty("InputWorkspace", ws);
      const double min = 0.75;
      const double max = 4.25;
      alg.setProperty("P1Bin", boost::assign::list_of(min)(max).convert_to_container<std::vector<double> >());
      alg.setPropertyValue("OutputWorkspace", "dummy");
      alg.execute();
      IMDHistoWorkspace_sptr outWS=alg.getProperty("OutputWorkspace");

      // Quick check that output seems to have the right shape.
      TSM_ASSERT_EQUALS("All integrated", 1, outWS->getNPoints());
      auto dim = outWS->getDimension(0);
      TS_ASSERT_EQUALS(min, dim->getMinimum());
      TS_ASSERT_EQUALS(max, dim->getMaximum());

      // Check the data. No accounting for normalization.
      TSM_ASSERT_DELTA("Wrong integrated value", 1.0/4 + 2 + 3 + 4 + 5.0/4, outWS->getSignalAt(0), 1e-4);

      Mantid::coord_t point[1] = {3.0}; // Roughly centre of the single output bin
      TSM_ASSERT_DELTA("Number of events normalization. Weights for n-events used incorrectly.", 1.0, outWS->getSignalAtCoord(point, Mantid::API::NumEventsNormalization), 1e-4);
      
  }


};

//=====================================================================================
// Performance Tests
//=====================================================================================
using namespace Mantid::DataObjects;
class IntegrateMDHistoWorkspaceTestPerformance : public CxxTest::TestSuite
{

private:

  MDHistoWorkspace_sptr m_ws;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegrateMDHistoWorkspaceTestPerformance *createSuite() { return new IntegrateMDHistoWorkspaceTestPerformance(); }
  static void destroySuite( IntegrateMDHistoWorkspaceTestPerformance *suite ) { delete suite; }

  IntegrateMDHistoWorkspaceTestPerformance() {
      // Create a 4D workspace.
      m_ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0 /*signal*/, 4 /*nd*/, 100 /*nbins*/, 10 /*max*/, 1.0 /*error sq*/);
  }

  void test_execute_4d()
  {
      IntegrateMDHistoWorkspace alg;
      alg.setChild(true);
      alg.setRethrows(true);
      alg.initialize();
      const double min = 0;
      const double max = 1;
      alg.setProperty("InputWorkspace", m_ws);
      alg.setProperty("P1Bin", boost::assign::list_of(min)(max).convert_to_container<std::vector<double> >());
      alg.setProperty("P2Bin", boost::assign::list_of(min)(max).convert_to_container<std::vector<double> >());
      alg.setProperty("P3Bin", boost::assign::list_of(min)(max).convert_to_container<std::vector<double> >());
      alg.setProperty("P4Bin", boost::assign::list_of(min)(max).convert_to_container<std::vector<double> >());
      alg.setPropertyValue("OutputWorkspace", "dummy");
      alg.execute();
      IMDHistoWorkspace_sptr outWS=alg.getProperty("OutputWorkspace");
      TS_ASSERT(outWS);
  }

};

#endif /* MANTID_MDALGORITHMS_INTEGRATEMDHISTOWORKSPACETEST_H_ */
