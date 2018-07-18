#ifndef MANTID_ALGORITHMS_ConvertToPoleFigureTest_H_
#define MANTID_ALGORITHMS_ConvertToPoleFigureTest_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/ConvertToPoleFigure.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTypes/Core/DateAndTime.h"

#include <cmath>
#include <numeric>

using Mantid::Algorithms::ConvertToPoleFigure;

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid;

namespace {

//-----------------------------------------------------
/** create a Bragg workspace containg 2 spectra.
 * Each has 100 data points containing a Gaussian peak
 * between d = 1.2 and 1.5
 * More on test case
 * X    Y           Z
 * 2    -0.16625    -0.3825
   2    0.16625    -0.3825
   2    0    0
   2    -0.16625    0.3825
   2    0.16625    0.3825


Omega = -45    HROT = 30
TD ND
   pt1 0.823814639    1.619696448
   pt2 0.990790951    1.523292629
   pt3 1       1.732050808
   pt4 -0.808630193    -1.63434472
   pt5 -1.01106895    -1.51746665
 * @brief createBraggWorkspace
 * @return
 */
Mantid::API::MatrixWorkspace_sptr
createBraggWorkspace(const std::string &name) {

  Mantid::API::FrameworkManager::Instance();
  size_t num_det = 5;
  size_t n = 100;
  Mantid::DataObjects::Workspace2D_sptr ws =
      boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
          Mantid::API::WorkspaceFactory::Instance().create("Workspace2D",
                                                           num_det, n, n));
  // create unit
  std::string unitlabel("dSpacing");
  ws->getAxis(0)->unit() =
      Mantid::Kernel::UnitFactory::Instance().create(unitlabel);

  // set instrument as reduced VULCAN east and west bank
  Instrument_sptr testInst(new Instrument);

  // Define a source component: [0,0,-43.754]
  Mantid::Geometry::ObjComponent *source = new Mantid::Geometry::ObjComponent(
      "moderator", IObject_sptr(), testInst.get());
  source->setPos(V3D(0, 0.0, -43.754));
  testInst->add(source);
  testInst->markAsSource(source);

  // Define a sample as a simple sphere
  Mantid::Geometry::ObjComponent *sample = new Mantid::Geometry::ObjComponent(
      "samplePos", IObject_sptr(), testInst.get());
  testInst->setPos(0.0, 0.0, 0.0);
  testInst->add(sample);
  testInst->markAsSamplePos(sample);

  // add 5 pixels to simulate VULCAN's east bank's 4 corners and center
  std::vector<std::vector<double>> pixel_pos_vec(num_det);
  pixel_pos_vec[0] = {2, -0.16625, -0.3825};
  pixel_pos_vec[1] = {2, 0.16625, -0.3825};
  pixel_pos_vec[2] = {2, 0, 0};
  pixel_pos_vec[3] = {2, -0.16625, 0.3825};
  pixel_pos_vec[4] = {2, 0.16625, 0.3825};

  std::vector<Mantid::Geometry::Detector *> pixel_vec(num_det);
  for (size_t i = 0; i < num_det; ++i) {
    int det_id_i = static_cast<int>(i) + 1;
    Mantid::Geometry::Detector *physicalPixel_i =
        new Detector("pixel", det_id_i, testInst.get());
    physicalPixel_i->setPos(pixel_pos_vec[i][0], pixel_pos_vec[i][1],
                            pixel_pos_vec[i][2]);
    testInst->add(physicalPixel_i);
    testInst->markAsDetector(physicalPixel_i);
    pixel_vec[i] = physicalPixel_i;
  }

  // set instrument
  ws->setInstrument(testInst);
  for (size_t i = 0; i < num_det; ++i) {
    ws->getSpectrum(i).addDetectorID(pixel_vec[i]->getID());
  }

  // set data
  auto &X = ws->mutableX(0);
  auto &Y = ws->mutableY(0);
  auto &E = ws->mutableE(0);

  // create Gaussian peak
  double dx = 0.01;
  for (size_t i = 0; i < n; i++) {
    X[i] = double(i) * dx + 1.2;
    Y[i] = exp(-(X[i] - 1.5) * (X[i] - 1.5) / (0.02)) * (1.0 + double(i));
    E[i] = sqrt(fabs(Y[i]));
  }

  // add properties
  // add HROT
  Kernel::TimeSeriesProperty<double> *hrotprop =
      new Kernel::TimeSeriesProperty<double>("HROT");
  Types::Core::DateAndTime time0(1000000);
  hrotprop->addValue(time0, -0.003857142);
  ws->mutableRun().addProperty(hrotprop);

  // add Omega
  Kernel::TimeSeriesProperty<double> *omegaprop =
      new Kernel::TimeSeriesProperty<double>("OMEGA");
  omegaprop->addValue(time0, 89.998);
  API::Run &run = ws->mutableRun();
  run.addProperty(omegaprop);

  // add to ADS
  Mantid::API::AnalysisDataService::Instance().add(name, ws);

  return ws;
}
}

class ConvertToPoleFigureTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    Mantid::Algorithms::ConvertToPoleFigure alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //-------------------------------------------------------------------------------------------
  /**
   * @brief test_Execute
   */
  void New_test_Execute() {

    // about input workspaces
    std::string peak_intensity_ws_name("TestPeakIntensityWorkspace");
    std::string input_ws_name("TestWithInstrumentWorkspace");

    API::MatrixWorkspace_sptr dataws = createBraggWorkspace(input_ws_name);
    // TODO API::MatrixWorkspace_sptr intensityws =
    // createIntensityWorkspace(peak_intensity_ws_name);

    Mantid::Algorithms::ConvertToPoleFigure pfcalculator;
    pfcalculator.initialize();

    // set properties
    TS_ASSERT_THROWS_NOTHING(
        pfcalculator.setProperty("InputWorkspace", input_ws_name));
    pfcalculator.setProperty("OutputWorkspace", "TwoSpecPoleFigure");
    pfcalculator.setProperty("IntegratedPeakIntensityWorkspace",
                             peak_intensity_ws_name);
    // TODO pfcalculator.setProperty("HROTName", hrot_name);
    // TODO pfcalculator.setProperty("OmegaName", omega_name);

    pfcalculator.execute();

    // run
    TS_ASSERT(pfcalculator.isExecuted());

    // check results
    TS_ASSERT(
        API::AnalysisDataService::Instance().doesExist("TwoSpecPoleFigure"));
    API::IMDEventWorkspace_sptr outws =
        boost::dynamic_pointer_cast<API::IMDEventWorkspace>(
            API::AnalysisDataService::Instance().retrieve("TwoSpecPoleFigure"));
    TS_ASSERT(outws);

    // get the vectors out - TODO
    std::vector<double> r_td_vector = pfcalculator.getProperty("R_TD");

    // compare the vetors - TODO

    // check MDWorkspaces

    //    TS_ASSERT_EQUALS(wsindex, 0);
    //    TS_ASSERT_EQUALS(r_td, r_td1);

    // clean up

    return;
  }
};

#endif /* MANTID_ALGORITHMS_ConvertToPoleFigureTest_H_ */
