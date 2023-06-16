// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/FindCenterOfMassPosition2.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/UnitFactory.h"
#include <cxxtest/TestSuite.h>

#include "MantidFrameworkTestHelpers/SANSInstrumentCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid;

class FindCenterOfMassPosition2Test : public CxxTest::TestSuite {
public:
  static FindCenterOfMassPosition2Test *createSuite() { return new FindCenterOfMassPosition2Test(); }
  static void destroySuite(FindCenterOfMassPosition2Test *suite) { delete suite; }

  /*
   * Generate fake data for which we know what the result should be
   */
  FindCenterOfMassPosition2Test() {
    DataObjects::Workspace2D_sptr ws = SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(m_inputWSname);

    // Generate sample data as a 2D Gaussian around the defined center
    const std::size_t NUM_BINS = static_cast<std::size_t>(SANSInstrumentCreationHelper::nBins);
    for (std::size_t ix = 0; ix < NUM_BINS; ix++) {
      for (std::size_t iy = 0; iy < NUM_BINS; iy++) {
        std::size_t i = ix * NUM_BINS + iy + static_cast<std::size_t>(SANSInstrumentCreationHelper::nMonitors);
        auto &X = ws->mutableX(i);
        auto &Y = ws->mutableY(i);
        auto &E = ws->mutableE(i);
        X[0] = 1;
        X[1] = 2;
        double dx = (m_centerX - (double)ix);
        double dy = (m_centerY - (double)iy);
        Y[0] = exp(-(dx * dx + dy * dy));
        // Set tube extrema to special values
        if (iy == 0 || iy + 1 == NUM_BINS)
          Y[0] = (iy % 2) ? std::nan("") : std::numeric_limits<double>::infinity();
        E[0] = 1;
      }
    }
  }

  ~FindCenterOfMassPosition2Test() override { AnalysisDataService::Instance().remove(m_inputWSname); }

  void validateCenterAndRemoveTableWS(const std::string &tableWSname, const double centerX, const double centerY,
                                      const double tolerance) {
    Mantid::DataObjects::TableWorkspace_sptr table =
        AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::TableWorkspace>(tableWSname);
    TS_ASSERT(table);

    TS_ASSERT_EQUALS(table->rowCount(), 2);
    TS_ASSERT_EQUALS(table->columnCount(), 2);

    TableRow row = table->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0), "X (m)");
    TS_ASSERT_DELTA(row.Double(1), centerX, tolerance);

    row = table->getRow(1);
    TS_ASSERT_EQUALS(row.String(0), "Y (m)");
    TS_ASSERT_DELTA(row.Double(1), centerY, tolerance);

    // remove the TableWorkspace from the ADS
    AnalysisDataService::Instance().remove(tableWSname);
  }

  void testParameters() {
    Mantid::Algorithms::FindCenterOfMassPosition2 center;
    TS_ASSERT_EQUALS(center.name(), "FindCenterOfMassPosition")

    TS_ASSERT_EQUALS(center.version(), 2)

    TS_ASSERT_EQUALS(center.category(), "SANS")
  }

  void testExec() {
    Mantid::Algorithms::FindCenterOfMassPosition2 center;
    if (!center.isInitialized())
      center.initialize();

    const std::string outputWS("center_of_mass");
    center.setPropertyValue("InputWorkspace", m_inputWSname);
    center.setPropertyValue("Output", outputWS);
    center.setPropertyValue("CenterX", "0");
    center.setPropertyValue("CenterY", "0");

    TS_ASSERT_THROWS_NOTHING(center.execute())
    TS_ASSERT(center.isExecuted())

    validateCenterAndRemoveTableWS(outputWS, m_centerX * m_pixel_size, m_centerY * m_pixel_size, 0.0001);
  }

  void testExecScatteredData() {
    Mantid::Algorithms::FindCenterOfMassPosition2 center;
    if (!center.isInitialized())
      center.initialize();

    const std::string outputWS("center_of_mass");
    center.setPropertyValue("InputWorkspace", m_inputWSname);
    center.setPropertyValue("Output", outputWS);
    center.setProperty("CenterX", 0.);
    center.setProperty("CenterY", 0.);
    center.setProperty("DirectBeam", false);
    center.setProperty("BeamRadius", 0.0075); // 1.5*0.005, now in meters, not in pixels

    TS_ASSERT_THROWS_NOTHING(center.execute())
    TS_ASSERT(center.isExecuted())

    validateCenterAndRemoveTableWS(outputWS, m_centerX * m_pixel_size, m_centerY * m_pixel_size, 0.0001);
  }

  void testExecWithArrayResult() {
    Mantid::Algorithms::FindCenterOfMassPosition2 center;
    if (!center.isInitialized())
      center.initialize();

    center.setPropertyValue("InputWorkspace", m_inputWSname);
    center.setProperty("CenterX", 0.);
    center.setProperty("CenterY", 0.);

    TS_ASSERT_THROWS_NOTHING(center.execute())
    TS_ASSERT(center.isExecuted())

    std::vector<double> list = center.getProperty("CenterOfMass");
    TS_ASSERT_EQUALS(list.size(), 2);
    TS_ASSERT_DELTA(list[0], m_centerX * m_pixel_size, 0.0001);
    TS_ASSERT_DELTA(list[1], m_centerY * m_pixel_size, 0.0001);
  }

  void testCG3Data() {
    const double CENTER_TOL{0.00125}; // algorithm default
    // values estimated by eye
    const double X_EXP{-0.0078};
    const double Y_EXP{-0.0143};
    const std::string IN_WKSP_NAME("testCG3DataInputWorkspace");

    Mantid::Algorithms::FindCenterOfMassPosition2 center;
    center.initialize();

    // load the data
    auto loader = center.createChildAlgorithm("LoadNexusProcessed");
    loader->initialize();
    loader->setPropertyValue("Filename", "CG3_beamcenter_input.nxs");
    loader->setPropertyValue("OutputWorkspace", IN_WKSP_NAME);
    loader->setAlwaysStoreInADS(true); // required to retrieve later the workspace by its name
    loader->execute();

    center.setPropertyValue("InputWorkspace", "testCG3DataInputWorkspace");
    const std::string outputWSname("testCG3DataOutputWorkspace");
    center.setPropertyValue("Output", outputWSname);
    center.setProperty("CenterX", 0.);
    center.setProperty("CenterY", 0.);
    center.setProperty("Tolerance", CENTER_TOL);
    center.setProperty("BeamRadius", 0.0155); // meters

    TS_ASSERT_THROWS_NOTHING(center.execute())
    TS_ASSERT(center.isExecuted())

    validateCenterAndRemoveTableWS(outputWSname, X_EXP, Y_EXP, 0.5 * CENTER_TOL);

    AnalysisDataService::Instance().remove(IN_WKSP_NAME);
  }

  /*
   * Test that will load an actual data file and perform the center of mass
   * calculation. This test takes a longer time to execute so we won't include
   * it in the set of unit tests.
   */
  void test_biosans_empty_cell() {
    const std::string IN_WKSP_NAME("wav");

    // load in the data
    Mantid::DataHandling::LoadSpice2D loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "BioSANS_empty_cell.xml");
    loader.setPropertyValue("OutputWorkspace", IN_WKSP_NAME);
    loader.execute();

    // run the centering algorithm
    Mantid::Algorithms::FindCenterOfMassPosition2 center;
    center.initialize();

    TS_ASSERT_THROWS_NOTHING(center.setPropertyValue("InputWorkspace", IN_WKSP_NAME))
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(center.setPropertyValue("Output", outputWS))
    center.setProperty("CenterX", 0.);
    center.setProperty("CenterY", 0.);
    center.setProperty("Tolerance", 0.0012875);

    TS_ASSERT_THROWS_NOTHING(center.execute())
    TS_ASSERT(center.isExecuted())

    // Check that the position is the same as obtained with the HFIR code
    validateCenterAndRemoveTableWS(outputWS, -0.40658, 0.0090835, 0.0001);

    // NOTE: Version 1 (from original IGOR HFIR code) computes everything in
    // pixels, where
    // the counts in a pixel is effectively put at the center of the pixel. In
    // the BIOSANS geometry
    // description, the pixels are offset by half a pixel so that 0,0 is right
    // in the middle of the detector.
    // This gives us an offset of half a pixel when transforming from pixel
    // coordinate to real space.

    // NOTE: The HFIR algorithm masked one pixel around the edge of the
    // detector, so the
    // answer is not exactly the same. It was checked that the correct output
    // comes out
    // of the algorithm if the one-pixel mask is applied. See python unit tests.
    // For this test we simply compare to the correct output _without_ masking.

    AnalysisDataService::Instance().remove(IN_WKSP_NAME);
  }

private:
  const std::string m_inputWSname{"FindCenterOfMassPosition2Test_engineered_input"};
  const double m_centerX{25.5};
  const double m_centerY{10.5};
  const double m_pixel_size{0.005};
};
