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

//---------------------------------------------------------------------------------------
/** create a MatrixWorkspace containing integrated peak intensities for the 5
 * spectra
 *  testing workspace
 * @brief createIntensityWorkspace
 * @param name
 * @return
 */
Mantid::API::MatrixWorkspace_sptr
createIntensityWorkspace(const std::string &name) {
  Mantid::API::FrameworkManager::Instance();
  size_t num_det = 5;
  size_t n = 1;
  Mantid::DataObjects::Workspace2D_sptr ws =
      boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
          Mantid::API::WorkspaceFactory::Instance().create("Workspace2D",
                                                           num_det, n, n));
  // create unit
  std::string unitlabel("dSpacing");
  ws->getAxis(0)->unit() =
      Mantid::Kernel::UnitFactory::Instance().create(unitlabel);

  // create intensity data
  for (size_t i = 0; i < num_det; i++) {
    auto &vec_x_i = ws->mutableX(i);
    auto &vec_y_i = ws->mutableY(i);
    auto &vec_e_i = ws->mutableE(i);

    vec_x_i[0] = 1.2;
    vec_y_i[0] = (static_cast<double>(i) + 10) * 2.5;
    vec_e_i[0] = sqrt(fabs(vec_y_i[0]));
  }

  // add to ADS
  Mantid::API::AnalysisDataService::Instance().add(name, ws);

  return ws;
}

class ConvertToPoleFigureTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    Mantid::Algorithms::ConvertToPoleFigure alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //-------------------------------------------------------------------------------------------
  /** test on a 5-pixel instrument which corresponds to VULCAN's east bank
   * (center and 4 corners)
   * @brief test_Execute
   */
  void test_Execute() {

    // about input workspaces
    std::string peak_intensity_ws_name("TestPeakIntensityWorkspace");
    std::string input_ws_name("TestWithInstrumentWorkspace");
    std::string out_md_name("FiveEventsMDWorkspace");
    const std::string hrot_name("HROT");
    const std::string omega_name("Omega");

    API::MatrixWorkspace_sptr dataws = createBraggWorkspace(input_ws_name);
    API::MatrixWorkspace_sptr intensityws =
        createIntensityWorkspace(peak_intensity_ws_name);
    TS_ASSERT_EQUALS(dataws->getNumberHistograms(),
                     intensityws->getNumberHistograms())

    Mantid::Algorithms::ConvertToPoleFigure pfcalculator;
    pfcalculator.initialize();

    // set properties
    TS_ASSERT_THROWS_NOTHING(
        pfcalculator.setProperty("InputWorkspace", input_ws_name));
    TS_ASSERT_THROWS_NOTHING(
        pfcalculator.setProperty("OutputWorkspace", out_md_name));
    TS_ASSERT_THROWS_NOTHING(pfcalculator.setProperty(
        "IntegratedPeakIntensityWorkspace", peak_intensity_ws_name));
    TS_ASSERT_THROWS_NOTHING(pfcalculator.setProperty("HROTName", hrot_name));
    TS_ASSERT_THROWS_NOTHING(pfcalculator.setProperty("OmegaName", omega_name));

    TS_ASSERT_THROWS_NOTHING(pfcalculator.execute());

    // run
    TS_ASSERT(pfcalculator.isExecuted());

    // check results
    TS_ASSERT(
        API::AnalysisDataService::Instance().doesExist("TwoSpecPoleFigure"));
    API::IMDEventWorkspace_sptr outws =
        boost::dynamic_pointer_cast<API::IMDEventWorkspace>(
            API::AnalysisDataService::Instance().retrieve("TwoSpecPoleFigure"));
    TS_ASSERT(outws);

    // get the vectors out
    std::vector<double> r_td_vector = pfcalculator.getProperty("R_TD");
    std::vector<double> r_nd_vector = pfcalculator.getProperty("R_ND");
    std::vector<double> intensity_vector =
        pfcalculator.getProperty("PeakIntensity");

    // check the vectors' sizes
    TS_ASSERT_EQUALS(r_td_vector.size(), 5);
    TS_ASSERT_EQUALS(r_nd_vector.size(), 5);
    TS_ASSERT_EQUALS(intensity_vector.size(), 5);

    // set the  pre-calculated value
    std::vector<double> bench_r_td_vec = {};
    std::vector<double> bench_r_nd_vec = {};
    std::vector<double> bench_intensity_vec = {};

    // check value of R_TD
    for (size_t i = 0; i < 5; ++i) {
      TS_ASSERT_DELTA(r_td_vector[i], bench_r_td_vec[i], 0.0001);
    }

    // check value of R_ND
    for (size_t i = 0; i < 5; ++i) {
      TS_ASSERT_DELTA(r_nd_vector[i], bench_r_nd_vec[i], 0.0001);
    }

    // check value of R_ND
    for (size_t i = 0; i < 5; ++i) {
      TS_ASSERT_DELTA(intensity_vector[i], bench_intensity_vec[i], 0.0001);
    }

    // check MDWorkspaces
    TS_ASSERT(
        Mantid::API::AnalysisDataService::Instance().doesExist(out_md_name));

    API::IMDEventWorkspace_sptr out_ws =
        boost::dynamic_pointer_cast<API::IMDEventWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(out_md_name));
    TS_ASSERT_EQUALS(out_ws->getNumDims(), 3);

    // TODO - FIGURE OUT HOW TO ACCESS EACH MDEvent!

    // clean up
    Mantid::API::AnalysisDataService::Instance().remove(input_ws_name);
    Mantid::API::AnalysisDataService::Instance().remove(peak_intensity_ws_name);
    Mantid::API::AnalysisDataService::Instance().remove(out_md_name);

    return;
  }
};

#endif /* MANTID_ALGORITHMS_ConvertToPoleFigureTest_H_ */
