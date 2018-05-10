#ifndef HFIRLOADTEST_H_
#define HFIRLOADTEST_H_

#include "MantidWorkflowAlgorithms/HFIRLoad.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

class HFIRLoadTest : public CxxTest::TestSuite {
public:
  /**
   * No distances given in the input!
   * It get's everything from the data file
   */
  void testDefaults() {

    Mantid::WorkflowAlgorithms::HFIRLoad algorithm;
    TS_ASSERT_EQUALS(algorithm.name(), "HFIRLoad")
    TS_ASSERT_EQUALS(algorithm.version(), 1)
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())

    TS_ASSERT_THROWS_NOTHING(algorithm.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setPropertyValue("OutputWorkspace", "output_ws"));

    if (!algorithm.isInitialized())
      algorithm.initialize();

    TS_ASSERT_THROWS_NOTHING(algorithm.execute())

    TS_ASSERT(algorithm.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve("output_ws")))

    TS_ASSERT_EQUALS(result->run().getPropertyValueAsType<double>(
                         "sample-detector-distance-offset"),
                     665.400000);
    TS_ASSERT_EQUALS(result->run().getPropertyValueAsType<double>(
                         "sample-detector-distance"),
                     6.000000 * 1000);
    TS_ASSERT_EQUALS(result->run().getPropertyValueAsType<double>(
                         "sample-si-window-distance"),
                     146.000000);

    TS_ASSERT_EQUALS(result->run().getPropertyValueAsType<double>(
                         "total-sample-detector-distance"),
                     665.4 + 6.000000 * 1000 + 146.000000);

    TS_ASSERT_EQUALS(result->run().getPropertyValueAsType<double>(
                         "sample_detector_distance"),
                     6.0 * 1000);

    TS_ASSERT_EQUALS(result->getAxis(0)->unit()->unitID(), "Wavelength")

    Mantid::API::AnalysisDataService::Instance().remove("output_ws");
  }

  void testSampleDetectorDistanceGiven() {

    Mantid::WorkflowAlgorithms::HFIRLoad algorithm;
    TS_ASSERT_EQUALS(algorithm.name(), "HFIRLoad")
    TS_ASSERT_EQUALS(algorithm.version(), 1)
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())

    TS_ASSERT_THROWS_NOTHING(algorithm.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setPropertyValue("OutputWorkspace", "output_ws"));
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setPropertyValue("SampleDetectorDistance", "19534"));

    if (!algorithm.isInitialized())
      algorithm.initialize();

    TS_ASSERT_THROWS_NOTHING(algorithm.execute())

    TS_ASSERT(algorithm.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve("output_ws")))

    TS_ASSERT_EQUALS(result->run().getPropertyValueAsType<double>(
                         "sample_detector_distance"),
                     19534);

    TS_ASSERT_EQUALS(result->getAxis(0)->unit()->unitID(), "Wavelength")

    Mantid::API::AnalysisDataService::Instance().remove("output_ws");
  }

  void testSampleDetectorDistanceOffsetGiven() {

    Mantid::WorkflowAlgorithms::HFIRLoad algorithm;
    TS_ASSERT_EQUALS(algorithm.name(), "HFIRLoad")
    TS_ASSERT_EQUALS(algorithm.version(), 1)
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())

    TS_ASSERT_THROWS_NOTHING(algorithm.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setPropertyValue("OutputWorkspace", "output_ws"));
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setPropertyValue("SampleDetectorDistanceOffset", "749"));

    if (!algorithm.isInitialized())
      algorithm.initialize();

    TS_ASSERT_THROWS_NOTHING(algorithm.execute())

    TS_ASSERT(algorithm.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve("output_ws")))

    TS_ASSERT_EQUALS(result->run().getPropertyValueAsType<double>(
                         "sample_detector_distance"),
                     6.0 * 1000 + 749);

    TS_ASSERT_EQUALS(result->getAxis(0)->unit()->unitID(), "Wavelength")

    Mantid::API::AnalysisDataService::Instance().remove("output_ws");
  }

  /**
   * Offset is ignored!
   * sample_detector_distance is SampleDetectorDistance given!
   */
  void testBothSampleDistancesGiven() {

    Mantid::WorkflowAlgorithms::HFIRLoad algorithm;
    TS_ASSERT_EQUALS(algorithm.name(), "HFIRLoad")
    TS_ASSERT_EQUALS(algorithm.version(), 1)
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())

    TS_ASSERT_THROWS_NOTHING(algorithm.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setPropertyValue("OutputWorkspace", "output_ws"));
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setPropertyValue("SampleDetectorDistance", "19100"));
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setPropertyValue("SampleDetectorDistanceOffset", "749"));

    if (!algorithm.isInitialized())
      algorithm.initialize();

    TS_ASSERT_THROWS_NOTHING(algorithm.execute())

    TS_ASSERT(algorithm.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve("output_ws")))
    // In the fire but ignored for the final property!
    TS_ASSERT_EQUALS(result->run().getPropertyValueAsType<double>(
                         "sample-detector-distance-offset"),
                     665.400000);
    TS_ASSERT_EQUALS(result->run().getPropertyValueAsType<double>(
                         "sample-detector-distance"),
                     6.000000 * 1000);
    TS_ASSERT_EQUALS(result->run().getPropertyValueAsType<double>(
                         "sample-si-window-distance"),
                     146.000000);

    TS_ASSERT_EQUALS(result->run().getPropertyValueAsType<double>(
                         "sample_detector_distance"),
                     19100);

    TS_ASSERT_EQUALS(result->getAxis(0)->unit()->unitID(), "Wavelength")

    Mantid::API::AnalysisDataService::Instance().remove("output_ws");
  }

private:
  std::string filename = "BioSANS_exp61_scan0004_0001.xml";
};

#endif /*HFIRLOADTEST_H_*/
