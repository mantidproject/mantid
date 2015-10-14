#ifndef FINDCENTEROFMASSPOSITIONTEST2_H_
#define FINDCENTEROFMASSPOSITIONTEST2_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/FindCenterOfMassPosition2.h"
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"

#include "MantidTestHelpers/SANSInstrumentCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid;

class FindCenterOfMassPosition2Test : public CxxTest::TestSuite {
public:
  static FindCenterOfMassPosition2Test *createSuite() {
    return new FindCenterOfMassPosition2Test();
  }
  static void destroySuite(FindCenterOfMassPosition2Test *suite) {
    delete suite;
  }

  /*
   * Generate fake data for which we know what the result should be
   */
  FindCenterOfMassPosition2Test() {
    inputWS = "sampledata";
    center_x = 25.5;
    center_y = 10.5;
    pixel_size = 0.005;

    ws = SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(inputWS);

    // Generate sample data as a 2D Gaussian around the defined center
    for (int ix = 0; ix < SANSInstrumentCreationHelper::nBins; ix++) {
      for (int iy = 0; iy < SANSInstrumentCreationHelper::nBins; iy++) {
        int i = ix * SANSInstrumentCreationHelper::nBins + iy +
                SANSInstrumentCreationHelper::nMonitors;
        MantidVec &X = ws->dataX(i);
        MantidVec &Y = ws->dataY(i);
        MantidVec &E = ws->dataE(i);
        X[0] = 1;
        X[1] = 2;
        double dx = (center_x - (double)ix);
        double dy = (center_y - (double)iy);
        Y[0] = exp(-(dx * dx + dy * dy));
        E[0] = 1;
        ws->getSpectrum(i)->setSpectrumNo(i);
      }
    }
  }

  ~FindCenterOfMassPosition2Test() { AnalysisDataService::Instance().clear(); }

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
    center.setPropertyValue("InputWorkspace", inputWS);
    center.setPropertyValue("Output", outputWS);
    center.setPropertyValue("CenterX", "0");
    center.setPropertyValue("CenterY", "0");

    TS_ASSERT_THROWS_NOTHING(center.execute())
    TS_ASSERT(center.isExecuted())

    // Get the resulting table workspace
    Mantid::DataObjects::TableWorkspace_sptr table =
        AnalysisDataService::Instance()
            .retrieveWS<Mantid::DataObjects::TableWorkspace>(outputWS);

    TS_ASSERT_EQUALS(table->rowCount(), 2);
    TS_ASSERT_EQUALS(table->columnCount(), 2);

    TableRow row = table->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0), "X (m)");
    TS_ASSERT_DELTA(row.Double(1), center_x * pixel_size, 0.0001);

    row = table->getRow(1);
    TS_ASSERT_EQUALS(row.String(0), "Y (m)");
    TS_ASSERT_DELTA(row.Double(1), center_y * pixel_size, 0.0001);
  }

  void testExecScatteredData() {
    Mantid::Algorithms::FindCenterOfMassPosition2 center;
    if (!center.isInitialized())
      center.initialize();

    const std::string outputWS("center_of_mass");
    center.setPropertyValue("InputWorkspace", inputWS);
    center.setPropertyValue("Output", outputWS);
    center.setPropertyValue("CenterX", "0");
    center.setPropertyValue("CenterY", "0");
    center.setPropertyValue("DirectBeam", "0");
    center.setPropertyValue(
        "BeamRadius", "0.0075"); // 1.5*0.005, now in meters, not in pixels

    TS_ASSERT_THROWS_NOTHING(center.execute())
    TS_ASSERT(center.isExecuted())

    // Get the resulting table workspace
    Mantid::DataObjects::TableWorkspace_sptr table =
        AnalysisDataService::Instance()
            .retrieveWS<Mantid::DataObjects::TableWorkspace>(outputWS);

    TS_ASSERT_EQUALS(table->rowCount(), 2);
    TS_ASSERT_EQUALS(table->columnCount(), 2);

    TableRow row = table->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0), "X (m)");
    TS_ASSERT_DELTA(row.Double(1), center_x * pixel_size, 0.0001);

    row = table->getRow(1);
    TS_ASSERT_EQUALS(row.String(0), "Y (m)");
    TS_ASSERT_DELTA(row.Double(1), center_y * pixel_size, 0.0001);
  }

  void testExecWithArrayResult() {
    Mantid::Algorithms::FindCenterOfMassPosition2 center;
    if (!center.isInitialized())
      center.initialize();

    center.setPropertyValue("InputWorkspace", inputWS);
    center.setPropertyValue("CenterX", "0");
    center.setPropertyValue("CenterY", "0");

    TS_ASSERT_THROWS_NOTHING(center.execute())
    TS_ASSERT(center.isExecuted())

    std::vector<double> list = center.getProperty("CenterOfMass");
    TS_ASSERT_EQUALS(list.size(), 2);
    TS_ASSERT_DELTA(list[0], center_x * pixel_size, 0.0001);
    TS_ASSERT_DELTA(list[1], center_y * pixel_size, 0.0001);
  }

  /*
   * Test that will load an actual data file and perform the center of mass
   * calculation. This test takes a longer time to execute so we won't include
   * it in the set of unit tests.
   */
  void validate() {
    Mantid::Algorithms::FindCenterOfMassPosition2 center;
    Mantid::DataHandling::LoadSpice2D loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "BioSANS_empty_cell.xml");
    const std::string inputWS("wav");
    loader.setPropertyValue("OutputWorkspace", inputWS);
    loader.execute();

    if (!center.isInitialized())
      center.initialize();

    TS_ASSERT_THROWS_NOTHING(center.setPropertyValue("InputWorkspace", inputWS))
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(center.setPropertyValue("Output", outputWS))
    center.setPropertyValue("CenterX", "0");
    center.setPropertyValue("CenterY", "0");
    center.setPropertyValue("Tolerance", "0.0012875");

    TS_ASSERT_THROWS_NOTHING(center.execute())
    TS_ASSERT(center.isExecuted())

    // Get the resulting table workspace
    Mantid::DataObjects::TableWorkspace_sptr table =
        AnalysisDataService::Instance()
            .retrieveWS<Mantid::DataObjects::TableWorkspace>(outputWS);

    TS_ASSERT_EQUALS(table->rowCount(), 2);
    TS_ASSERT_EQUALS(table->columnCount(), 2);

    // Check that the position is the same as obtained with the HFIR code
    TableRow row = table->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0), "X (m)");
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
    TS_ASSERT_DELTA(row.Double(1), -0.40658, 0.0001);

    row = table->getRow(1);
    TS_ASSERT_EQUALS(row.String(0), "Y (m)");
    TS_ASSERT_DELTA(row.Double(1), 0.0090835, 0.0001);
  }

private:
  std::string inputWS;
  double center_x;
  double center_y;
  double pixel_size;
  DataObjects::Workspace2D_sptr ws;
};

#endif /*FINDCENTEROFMASSPOSITIONTEST2_H_*/
