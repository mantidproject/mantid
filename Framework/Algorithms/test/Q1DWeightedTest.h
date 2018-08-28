#ifndef Q1DWEIGHTEDTEST_H_
#define Q1DWEIGHTEDTEST_H_

#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/Q1DWeighted.h"
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

class Q1DWeightedTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(radial_average.name(), "Q1DWeighted") }

  void testVersion() { TS_ASSERT_EQUALS(radial_average.version(), 1) }

  void testCategory() { TS_ASSERT_EQUALS(radial_average.category(), "SANS") }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(radial_average.initialize())
    TS_ASSERT(radial_average.isInitialized())
  }

  void setUp() override {
      loadAndMove();
  }

  void testExec() {

    if (!radial_average.isInitialized())
      radial_average.initialize();

    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("InputWorkspace", m_inputWS))
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("OutputBinning", "0.01,0.001,0.11"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("NPixelDivision", "3"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("ErrorWeighting", "1"))

    TS_ASSERT_THROWS_NOTHING(radial_average.execute())

    TS_ASSERT(radial_average.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))
    TS_ASSERT_EQUALS(result->getNumberHistograms(), 1)

    // Timer is 3600.0 for this test data file
    double tolerance(1e-03);

    // The points we are checking were computed using the HFIR IGOR package
    // For NPixelDivision = 1
    //   Y[1] = 0.0398848*3600; Y[2] = 0.0371762*3600; Y[30] = 0.030971*3600;
    //   Y[80] = 0.0275545*3600; Y[90] = 0.0270528*3600
    TS_ASSERT_EQUALS(result->x(0)[0], 0.01);
    TS_ASSERT_DELTA(result->y(0)[30], 110.9651, tolerance);
    TS_ASSERT_DELTA(result->y(0)[1], 143.2190, tolerance);
    TS_ASSERT_DELTA(result->y(0)[2], 134.2864, tolerance);
    TS_ASSERT_DELTA(result->y(0)[80], 98.3834, tolerance);
    TS_ASSERT_DELTA(result->y(0)[90], 95.9322, tolerance);

    Mantid::API::AnalysisDataService::Instance().remove(m_inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  // Test whether the WedgeOffset parameter works correctly.
  void testWedgeOffset() {

    const std::string outputWS("result");
    const std::string wedgeWS1("wedge1");
    const std::string wedgeWS2("wedge2");

    // Test method:
    // We use two wedges, which implies that they have an offset of 90 degree.
    // We then call the algorithm twice, once with offset 0, once with offset
    // 90. With offset 90 the wedges are thus logically "swapped", so we check
    // if their values match.

    if (!radial_average.isInitialized())
      radial_average.initialize();
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("InputWorkspace", m_inputWS))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("OutputBinning", "0.01,0.001,0.11"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("NPixelDivision", "3"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("ErrorWeighting", "1"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("WedgeWorkspace", wedgeWS1))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("NumberOfWedges", "2"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("WedgeAngle", "30"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("WedgeOffset", "0"))

    TS_ASSERT_THROWS_NOTHING(radial_average.execute())
    TS_ASSERT(radial_average.isExecuted())

    if (!radial_average.isInitialized())
      radial_average.initialize();
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("InputWorkspace", m_inputWS))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("OutputBinning", "0.01,0.001,0.11"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("NPixelDivision", "3"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("ErrorWeighting", "1"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("WedgeWorkspace", wedgeWS2))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("NumberOfWedges", "2"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("WedgeAngle", "30"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("WedgeOffset", "90"))

    TS_ASSERT_THROWS_NOTHING(radial_average.execute())
    TS_ASSERT(radial_average.isExecuted())

    // Get wedge 0 of the result with offset 0.
    auto result1 = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(
        Mantid::API::AnalysisDataService::Instance().retrieve(wedgeWS1));
    auto wedge1 = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        result1->getItem(0));

    // Get wedge 1 of the result with offset 90.
    auto result2 = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(
        Mantid::API::AnalysisDataService::Instance().retrieve(wedgeWS2));
    auto wedge2 = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        result2->getItem(1));

    double tolerance = 1e-12;

    // The two wedges should be identical.
    for (size_t i = 0; i < wedge1->y(0).size(); ++i)
      TS_ASSERT_DELTA(wedge1->y(0)[i], wedge2->y(0)[i], tolerance);

    Mantid::API::AnalysisDataService::Instance().remove(m_inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
    Mantid::API::AnalysisDataService::Instance().remove(wedgeWS1);
    Mantid::API::AnalysisDataService::Instance().remove(wedgeWS2);
  }

  // Test the asymmetric wedges option
  void testWedgeAsymm() {

    const std::string outputWS("result");
    const std::string wedgeWS1("wedge1");
    const std::string wedgeWS2("wedge2");

    // We will call the algorithm twice:
    // once with symmetric wedges, once with twice as much of asymmetric wedges,
    // such that they are back-to-back in azimuthal plane. The pair-wise average
    // of asymmetric wedges should match the corresponding symmetric ones.

    if (!radial_average.isInitialized())
      radial_average.initialize();
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("InputWorkspace", m_inputWS))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("OutputBinning", "0.01,0.001,0.08"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("NPixelDivision", "3"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("ErrorWeighting", "0"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("WedgeWorkspace", wedgeWS1))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("NumberOfWedges", "2"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("WedgeAngle", "30"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("WedgeOffset", "0"))

    TS_ASSERT_THROWS_NOTHING(radial_average.execute())
    TS_ASSERT(radial_average.isExecuted())

    if (!radial_average.isInitialized())
      radial_average.initialize();
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("InputWorkspace", m_inputWS))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("OutputBinning", "0.01,0.001,0.08"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("NPixelDivision", "3"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("ErrorWeighting", "0"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("WedgeWorkspace", wedgeWS2))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("NumberOfWedges", "4"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("WedgeAngle", "30"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("WedgeOffset", "0"))
    TS_ASSERT_THROWS_NOTHING(
        radial_average.setPropertyValue("AsymmetricWedges", "1"))

    TS_ASSERT_THROWS_NOTHING(radial_average.execute())
    TS_ASSERT(radial_average.isExecuted())

    // Get the results of symmetric wedges.
    auto result1 = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(
        Mantid::API::AnalysisDataService::Instance().retrieve(wedgeWS1));
    TS_ASSERT(result1)
    auto wedge1 = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        result1->getItem(0));
    auto wedge2 = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        result1->getItem(1));

    TS_ASSERT(wedge1)
    TS_ASSERT(wedge2)

    // Get the results of asymmetric wedges.
    auto result2 = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(
        Mantid::API::AnalysisDataService::Instance().retrieve(wedgeWS2));
    TS_ASSERT(result2)
    auto wedgeA1 = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        result2->getItem(0));
    auto wedgeA2 = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        result2->getItem(1));
    auto wedgeA3 = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        result2->getItem(2));
    auto wedgeA4 = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        result2->getItem(3));

    TS_ASSERT(wedgeA1)
    TS_ASSERT(wedgeA2)
    TS_ASSERT(wedgeA3)
    TS_ASSERT(wedgeA4)

    double tolerance = 1e-12;

    // The average of A2 and A4 should be similar to wedge 2.
    for (size_t i = 0; i < wedge1->y(0).size(); ++i) {
      TS_ASSERT_DELTA(wedge2->y(0)[i],
                      (wedgeA2->y(0)[i] + wedgeA4->y(0)[i]) / 2, tolerance);
    }

    Mantid::API::AnalysisDataService::Instance().remove(m_inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
    Mantid::API::AnalysisDataService::Instance().remove(wedgeWS1);
    Mantid::API::AnalysisDataService::Instance().remove(wedgeWS2);
  }

private:
  void loadAndMove() {
    // This generates an appropriate real life workspace for testing.
    Mantid::DataHandling::LoadSpice2D loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "BioSANS_exp61_scan0004_0001.xml");
    m_inputWS = "wav";
    loader.setPropertyValue("OutputWorkspace", m_inputWS);
    loader.execute();

    // Move detector to its correct position
    Mantid::DataHandling::MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace", m_inputWS);
    mover.setPropertyValue("ComponentName", "detector1");

    // According to the instrument geometry, the center of the detector is
    // located at N_pixel / 2 + 0.5
    // X = (16-192.0/2.0+0.5)*5.15/1000.0 = -0.409425
    // Y = (95-192.0/2.0+0.5)*5.15/1000.0 = -0.002575
    mover.setPropertyValue("X", "0.409425");
    mover.setPropertyValue("Y", "0.002575");
    mover.execute();
  }

  Mantid::Algorithms::Q1DWeighted radial_average;
  std::string m_inputWS;
};

#endif /*Q1DWEIGHTEDTEST_H_*/
