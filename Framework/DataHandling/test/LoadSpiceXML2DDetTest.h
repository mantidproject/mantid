// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADSPICEXML2DDETTEST_H_
#define MANTID_DATAHANDLING_LOADSPICEXML2DDETTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/LoadSpiceXML2DDet.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid;
using namespace Mantid::API;
using Mantid::DataHandling::LoadSpiceXML2DDet;

class LoadSpiceXML2DDetTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadSpiceXML2DDetTest *createSuite() {
    return new LoadSpiceXML2DDetTest();
  }
  static void destroySuite(LoadSpiceXML2DDetTest *suite) { delete suite; }

  ITableWorkspace_sptr scantablews;

  //----------------------------------------------------------------------------------------------
  /** Test initialization of algorithm
   * @brief test_Init
   */
  void test_Init() {
    LoadSpiceXML2DDet testalg;
    testalg.initialize();
    TS_ASSERT(testalg.isInitialized());

    // create SPICE scan table workspace
    scantablews = createSpiceScanTable();
  }

  //----------------------------------------------------------------------------------------------
  /** Sample test load data without instrument
   * test data: HB3A_exp355_scan0001_0522
   *   2theta = 42.70975 degree
   * check:
   *   sample logs: including run_start, monitor, omega, chi, phi and 2theta
   * @brief Load data without instrument
   */
  void test_LoadDataNoInstrument() {
    LoadSpiceXML2DDet loader;
    loader.initialize();

    const std::string filename("HB3A_exp355_scan0001_0522.xml");
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        loader.setProperty("OutputWorkspace", "Exp0335_S0038"));
    std::vector<size_t> sizelist(2);
    sizelist[0] = 256;
    sizelist[1] = 256;
    loader.setProperty("DetectorGeometry", sizelist);
    loader.setProperty("LoadInstrument", false);

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    // Get data
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Exp0335_S0038"));
    TS_ASSERT(outws);

    size_t numspec = outws->getNumberHistograms();
    TS_ASSERT_EQUALS(numspec, 256);
    size_t numx = outws->readX(0).size();
    size_t numy = outws->readY(0).size();
    TS_ASSERT_EQUALS(numx, 256);
    TS_ASSERT_EQUALS(numy, 256);

    // Check the value
    double totalcounts = 0;
    for (size_t i = 0; i < numspec; ++i) {
      const MantidVec &vecY = outws->readY(i);
      size_t numy = vecY.size();
      for (size_t j = 0; j < numy; ++j)
        totalcounts += vecY[j];
    }
    TS_ASSERT_DELTA(totalcounts, 8049.00, 0.000001);

    // check max count
    TS_ASSERT_DELTA(outws->readY(120)[135], 4., 0.000001);

    // Check the sample logs loaded from XML node
    // monitor counts
    TS_ASSERT(outws->run().hasProperty("_monitor"));
    int monitorcount = std::stoi(outws->run().getProperty("_monitor")->value());
    TS_ASSERT_EQUALS(monitorcount, 29);

    // Check motor angles
    TS_ASSERT(outws->run().hasProperty("_2theta"));
    double _2theta = std::stod(outws->run().getProperty("_2theta")->value());
    TS_ASSERT_DELTA(_2theta, 42.709750, 0.0000001);

    TS_ASSERT(outws->run().hasProperty("_omega"));
    double _omega = std::stod(outws->run().getProperty("_omega")->value());
    TS_ASSERT_DELTA(_omega, 21.354500, 0.0000001);

    TS_ASSERT(outws->run().hasProperty("_chi"));
    double _chi = std::stod(outws->run().getProperty("_chi")->value());
    TS_ASSERT_DELTA(_chi, 1.215250, 0.0000001);

    TS_ASSERT(outws->run().hasProperty("_phi"));
    double _phi = std::stod(outws->run().getProperty("_phi")->value());
    TS_ASSERT_DELTA(_phi, 144.714218, 0.0000001);

    // check start_time and end_time
    TS_ASSERT(outws->run().hasProperty("start_time"));
    std::string start_time = outws->run().getProperty("start_time")->value();
    TS_ASSERT_EQUALS(start_time, "2015-01-17T13:36:45");

    // Clean
    AnalysisDataService::Instance().remove("Exp0335_S0038");
  }

  //----------------------------------------------------------------------------------------------
  /** Test algorithm with loading HB3A with instrument and presense of SPICE
   * scan table such that it will override 2theta value in the XML file
   * Test: Set 2theta = 0
   *  1. Detector should be symmetric along the X-axis and about the center of
   * detector;
   *  2. All pixels on the X-axis will have position Y value zero;
   *  3. All pixels on the Y-axis will have position X value zero
   * @brief test: load data with instrument whose detector's 2theta value is 0.
   */
  void test_LoadDataOverwrite2ThetaZero() {
    // Test 2theta at 0 degree
    LoadSpiceXML2DDet loader;
    loader.initialize();

    const std::string filename("HB3A_exp355_scan0001_0522.xml");
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        loader.setProperty("OutputWorkspace", "Exp0335_S0038"));
    std::vector<size_t> sizelist(2);
    sizelist[0] = 256;
    sizelist[1] = 256;
    loader.setProperty("DetectorGeometry", sizelist);
    loader.setProperty("LoadInstrument", true);
    loader.setProperty("SpiceTableWorkspace", scantablews);
    loader.setProperty("PtNumber", 3); // pt number 3 has 2theta value as 0.0
    loader.setProperty("ShiftedDetectorDistance", 0.);

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    // Get data
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Exp0335_S0038"));
    TS_ASSERT(outws);
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 256 * 256);

    for (auto property : outws->run().getProperties()) {
      std::cout << property->name() << ": " << property->value() << "\n";
    }

    // test signal value on various pixels
    // pixel at (256, 1): column 1
    TS_ASSERT_DELTA(outws->readY(255)[0], 1.0, 0.0001);
    // pixel at (254, 256): colun 256
    TS_ASSERT_DELTA(outws->readY(255 * 256 + 138)[0], 2.0, 0.00001);

    // Instrument
    TS_ASSERT(outws->getInstrument());

    // check center of the detector @ (128, 115)
    size_t center_col = 128;
    size_t center_row = 115;
    size_t center_ws_index = (center_row - 1) + (center_col - 1) * 256;

    const auto &spectrumInfo = outws->spectrumInfo();
    const auto det_center = spectrumInfo.position(center_ws_index);

    // distance to sample
    TS_ASSERT_DELTA(spectrumInfo.l2(center_ws_index), 0.3750, 0.0001);
    // center of the detector must be at zero
    TS_ASSERT_DELTA(det_center.X(), 0.0, 0.0000001);
    TS_ASSERT_DELTA(det_center.Y(), 0.0, 0.0000001);

    // check the sequence of the detector to each ws index
    const auto &detector0 = spectrumInfo.detector(0);
    const auto det0id = detector0.getID();
    TS_ASSERT_EQUALS(det0id, 0);
    const auto &detector1 = spectrumInfo.detector(1);
    const auto det1id = detector1.getID();
    TS_ASSERT_EQUALS(det1id, 1);
    const auto &detectorLast = spectrumInfo.detector(256 * 255 + 255);
    const auto detlastid = detectorLast.getID();
    TS_ASSERT_EQUALS(detlastid, 255 * 256 + 255);
    // test the whole sequence
    for (size_t irow = 1; irow < 250; ++irow)
      for (size_t jcol = 10; jcol < 20; ++jcol) {
        size_t iws = irow + jcol * 256;
        const auto &detector = spectrumInfo.detector(iws);
        const auto detid = detector.getID();
        TS_ASSERT_EQUALS(detid, static_cast<detid_t>(iws));
      }

    // test the geometry position whether det ID is from lower right corner and
    // move along positive Y direction
    // right most column
    const auto det0pos = spectrumInfo.position(0);
    TS_ASSERT_DELTA(det0pos.X(), 0.0252015625, 0.000001);
    TS_ASSERT_DELTA(det0pos.Y(), -0.022621875, 0.000001);
    TS_ASSERT_DELTA(det0pos.Z(), 0.375, 0.0001);

    double dY = 0.0001984375;

    const auto det1pos = spectrumInfo.position(1);
    TS_ASSERT_DELTA(det1pos.X(), 0.0252015625, 0.000001);
    TS_ASSERT_DELTA(det1pos.Y(), -0.022621875 + dY, 0.000001);
    TS_ASSERT_DELTA(det1pos.Z(), 0.375, 0.0001);

    // center is tested before

    // lower left column
    size_t i_wsll = 255 * 256 + 1;
    double dX = -0.0001984375;
    const auto detllpos = spectrumInfo.position(i_wsll);
    TS_ASSERT_DELTA(detllpos.X(), 0.0252015625 + 255 * dX, 0.000001);
    TS_ASSERT_DELTA(detllpos.Y(), -0.022621875 + dY, 0.000001);
    TS_ASSERT_DELTA(detllpos.Z(), 0.375, 0.0001);

    // test the detectors with symmetric to each other
    // they should have opposite X or Y

    // ll: low-left, lr: low-right, ul: upper-left; ur: upper-right
    size_t row_ll = 0;
    size_t col_ll = 2;
    size_t ws_index_ll = row_ll + col_ll * 256;
    const auto det_ll_pos = spectrumInfo.position(ws_index_ll);

    size_t row_lr = 0;
    size_t col_lr = 2 * 127 - 2;
    size_t ws_index_lr = row_lr + col_lr * 256;
    const auto det_lr_pos = spectrumInfo.position(ws_index_lr);

    size_t row_ul = 114 * 2;
    size_t col_ul = 2;
    size_t ws_index_ul = row_ul + col_ul * 256;
    const auto det_ul_pos = spectrumInfo.position(ws_index_ul);

    size_t row_ur = 114 * 2;
    size_t col_ur = 2 * 127 - 2;
    size_t ws_index_ur = row_ur + col_ur * 256;
    const auto det_ur_pos = spectrumInfo.position(ws_index_ur);

    double det_size = 0.0508; // meter
    int num_pixel = 256;
    double pixel_size = det_size / static_cast<double>(num_pixel);

    // Check symmetricity
    TS_ASSERT_DELTA(det_ll_pos.X() + det_lr_pos.X(), 0., 0.0000001);
    TS_ASSERT_DELTA(det_ll_pos.X(), (127 - 2) * pixel_size, 0.000001);
    TS_ASSERT_DELTA(det_ll_pos.Y(), det_lr_pos.Y(), 0.000001);
    TS_ASSERT_DELTA(det_ll_pos.Y(), -114 * pixel_size, 0.0000001);

    TS_ASSERT_DELTA(det_ll_pos.X(), det_ul_pos.X(), 0.00001);
    TS_ASSERT_DELTA(det_ll_pos.Y() + det_ul_pos.Y(), 0., 0.000001);

    TS_ASSERT_DELTA(det_lr_pos.X(), det_ur_pos.X(), 0.00001);
    TS_ASSERT_DELTA(det_lr_pos.Y() + det_ur_pos.Y(), 0., 0.00001);

    // Clean
    AnalysisDataService::Instance().remove("Exp0335_S0038");
  }

  //----------------------------------------------------------------------------------------------
  /** Test with loading instrument without Spice scan Table, while the 2theta
   * value is from sample
   *    sample log
   *  Testing includes:
   *  1. Load the instrument without Spice Table;
   *  2. Check the positions of detectors.
   *    (a) at center pixel, 2-theta = 42.797
   * @brief Load data and instrument with sample log value
   */
  void test_loadDataUsingSampleLogValue() {
    // initialize the algorithm
    LoadSpiceXML2DDet loader;
    loader.initialize();

    // set up properties
    const std::string filename("HB3A_exp355_scan0001_0522.xml");
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        loader.setProperty("OutputWorkspace", "Exp0335_S0038C"));
    std::vector<size_t> sizelist(2);
    sizelist[0] = 256;
    sizelist[1] = 256;
    loader.setProperty("DetectorGeometry", sizelist);
    loader.setProperty("LoadInstrument", true);
    loader.setProperty("ShiftedDetectorDistance", 0.);

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    // Get data
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Exp0335_S0038C"));
    TS_ASSERT(outws);
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 256 * 256);

    // Value
    // test signal value on various pixels
    // pixel at (256, 1): column 1
    TS_ASSERT_DELTA(outws->readY(255)[0], 1.0, 0.0001);
    // pixel at (254, 256): colun 256
    TS_ASSERT_DELTA(outws->readY(255 * 256 + 138)[0], 2.0, 0.00001);

    // Instrument
    TS_ASSERT(outws->getInstrument());

    // get 2theta from workspace
    double twotheta_raw =
        std::stod(outws->run().getProperty("_2theta")->value());

    Kernel::Property *raw_property = outws->run().getProperty("2theta");
    Kernel::TimeSeriesProperty<double> *twotheta_property =
        dynamic_cast<Kernel::TimeSeriesProperty<double> *>(raw_property);
    TS_ASSERT(twotheta_property);
    double twotheta_log = twotheta_property->valuesAsVector()[0];
    // 2-theta = 42.797

    TS_ASSERT_EQUALS(twotheta_raw, twotheta_log);

    // check the center of the detector
    const auto &spectrumInfo = outws->spectrumInfo();

    // check the center position
    size_t center_row = 115 - 1;
    size_t center_col = 128 - 1;
    size_t center_ws_index = 256 * center_col + center_row;
    const auto center_det_pos = spectrumInfo.position(center_ws_index);
    TS_ASSERT_DELTA(center_det_pos.Y(), 0., 0.00000001);
    double sample_center_distance = spectrumInfo.l2(center_ws_index);
    // TS_ASSERT_DELTA(center_det_pos.X(), )
    TS_ASSERT_DELTA(sample_center_distance, 0.3750, 0.0000001);
    double sample_center_angle = spectrumInfo.twoTheta(center_ws_index);
    TS_ASSERT_DELTA(sample_center_angle * 180. / M_PI, twotheta_log, 0.0001);

    size_t ll_ws_index = 0;
    const auto ll_det_pos = spectrumInfo.position(ll_ws_index);
    double ll_sample_r = spectrumInfo.l2(ll_ws_index);
    TS_ASSERT_DELTA(ll_sample_r, 0.37597, 0.001);

    size_t lu_ws_index = 255; // row = 255, col = 1
    const auto lu_det_pos = spectrumInfo.position(lu_ws_index);
    double lu_sample_r = spectrumInfo.l2(lu_ws_index);
    TS_ASSERT_DELTA(lu_sample_r, 0.37689, 0.001);

    TS_ASSERT_DELTA(ll_det_pos.X(), lu_det_pos.X(), 0.000001);
    TS_ASSERT(ll_det_pos.Y() + lu_det_pos.Y() > 0);

    TS_ASSERT(ll_det_pos.X() > center_det_pos.X());

    // Clean
    AnalysisDataService::Instance().remove("Exp0335_S0038C");
  }

  //----------------------------------------------------------------------------------------------
  /** Test of loading HB3A data with calibrated distance
   * scan table
   *  such that it can be set to zero-2-theta position
   * @brief test_loadHB3ACalibratedDetDistance
   * Testing include
   * 1. 2theta = 15 degree: scattering angle of all 4 corners should be same;
   */
  void test_loadHB3ACalibratedDetDistance() {
    // Test 2theta at 15 degree with sample-detector distance shift
    LoadSpiceXML2DDet loader;
    loader.initialize();

    const std::string filename("HB3A_exp355_scan0001_0522.xml");
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        loader.setProperty("OutputWorkspace", "Exp0335_S0038D"));
    std::vector<size_t> sizelist(2);
    sizelist[0] = 256;
    sizelist[1] = 256;
    loader.setProperty("DetectorGeometry", sizelist);
    loader.setProperty("LoadInstrument", true);
    loader.setProperty("SpiceTableWorkspace", scantablews);
    loader.setProperty("PtNumber", 1);
    loader.setProperty("ShiftedDetectorDistance", 0.1);

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    // Get data
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Exp0335_S0038D"));
    TS_ASSERT(outws);
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 256 * 256);

    // Value
    // test signal value on various pixels
    // pixel at (256, 1): column 1
    TS_ASSERT_DELTA(outws->readY(255)[0], 1.0, 0.0001);
    // pixel at (254, 256): colun 256
    TS_ASSERT_DELTA(outws->readY(255 * 256 + 138)[0], 2.0, 0.00001);

    // Instrument
    TS_ASSERT(outws->getInstrument());

    // get source and sample positions
    const auto &spectrumInfo = outws->spectrumInfo();

    // check center of the detector @ (128, 115)
    size_t center_col = 128;
    size_t center_row = 115;
    size_t center_ws_index = (center_row - 1) + (center_col - 1) * 256;
    // distance to sample
    double dist_r = spectrumInfo.l2(center_ws_index);
    TS_ASSERT_DELTA(dist_r, 0.3750 + 0.1, 0.0001);
    // center of the detector at 15 degree
    const auto sample_center_angle = spectrumInfo.twoTheta(center_ws_index);
    TS_ASSERT_DELTA(sample_center_angle * 180. / M_PI, 15., 0.0001);

    // test the detectors with symmetric to each other
    // they should have opposite X or Y

    // ll: low-left, lr: low-right, ul: upper-left; ur: upper-right
    size_t row_ll = 0;
    size_t col_ll = 2;
    size_t ws_index_ll = row_ll + col_ll * 256;

    size_t row_lr = 0;
    size_t col_lr = 2 * 127 - 2;
    size_t ws_index_lr = row_lr + col_lr * 256;

    size_t row_ul = 114 * 2;
    size_t col_ul = 2;
    size_t ws_index_ul = row_ul + col_ul * 256;

    size_t row_ur = 114 * 2;
    size_t col_ur = 2 * 127 - 2;
    size_t ws_index_ur = row_ur + col_ur * 256;

    // Check symmetry
    TS_ASSERT_DELTA(spectrumInfo.l2(ws_index_ll), spectrumInfo.l2(ws_index_lr),
                    0.0000001);
    TS_ASSERT_DELTA(spectrumInfo.l2(ws_index_ll), spectrumInfo.l2(ws_index_ul),
                    0.0000001);
    TS_ASSERT_DELTA(spectrumInfo.l2(ws_index_ll), spectrumInfo.l2(ws_index_ur),
                    0.0000001);

    // Clean
    AnalysisDataService::Instance().remove("Exp0335_S0038D");
  }

  //----------------------------------------------------------------------------------------------
  /** Test with loading instrument without Spice scan Table and detector is
   *shifted from original
   *  center
   *
   *  Testing includes:
   *  1. Load the instrument without Spice Table;
   *  2. Check the positions of shifted detector: from (115, 128) to (127,137)
   *    (a) at center pixel, 2-theta = 42.797 and pixel ID
   *  3. Check the symmetry of the peak positions
   * @brief Load data and instrument with sample log value
   */
  void test_loadDataShiftDetectorCenter() {
    // initialize the algorithm
    LoadSpiceXML2DDet loader;
    loader.initialize();

    // calculate shift of the detector center from (115, 128) to (127, 127)
    double det_step_x = -0.0001984375;
    double shift_x = static_cast<double>(137 - 128) * det_step_x *
                     -1.; // shift x comes from column
    double det_step_y = 0.0001984375;
    double shift_y = static_cast<double>(127 - 115) * det_step_y *
                     -1; // shift y comes from row

    // set up properties
    const std::string filename("HB3A_exp355_scan0001_0522.xml");
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        loader.setProperty("OutputWorkspace", "Exp0335_S0038C"));
    std::vector<size_t> sizelist(2);
    sizelist[0] = 256;
    sizelist[1] = 256;
    loader.setProperty("DetectorGeometry", sizelist);
    loader.setProperty("LoadInstrument", true);
    loader.setProperty("ShiftedDetectorDistance", 0.);
    loader.setProperty("DetectorCenterXShift", shift_x);
    loader.setProperty("DetectorCenterYShift", shift_y);

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    // Get data
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Exp0335_S0038C"));
    TS_ASSERT(outws);
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 256 * 256);

    // Value
    // test signal value on various pixels
    // pixel at (256, 1): column 1
    TS_ASSERT_DELTA(outws->readY(255)[0], 1.0, 0.0001);
    // pixel at (254, 256): column 256
    TS_ASSERT_DELTA(outws->readY(255 * 256 + 138)[0], 2.0, 0.00001);

    // Instrument
    TS_ASSERT(outws->getInstrument());

    // get 2theta from workspace
    double twotheta_raw =
        std::stod(outws->run().getProperty("_2theta")->value());

    Kernel::Property *raw_property = outws->run().getProperty("2theta");
    Kernel::TimeSeriesProperty<double> *twotheta_property =
        dynamic_cast<Kernel::TimeSeriesProperty<double> *>(raw_property);
    TS_ASSERT(twotheta_property);
    double twotheta_log = twotheta_property->valuesAsVector()[0];
    TS_ASSERT_DELTA(twotheta_log, 42.70975, 0.0001);

    TS_ASSERT_EQUALS(twotheta_raw, twotheta_log);

    // check the center of the detector
    const auto &spectrumInfo = outws->spectrumInfo();

    // check the center position
    size_t center_row = 127 - 1;
    size_t center_col = 137 - 1;
    size_t center_ws_index = 256 * center_col + center_row;
    // y should be 0. in the Z-Y plane
    const auto center_det_pos = spectrumInfo.position(center_ws_index);

    TS_ASSERT_DELTA(center_det_pos.Y(), 0., 0.00000001);
    double sample_center_distance = spectrumInfo.l2(center_ws_index);
    // distance
    std::cout << "Sample center distance: " << sample_center_distance << "\n";
    TS_ASSERT_DELTA(sample_center_distance, 0.3750, 0.0000001);
    const auto sample_center_angle = spectrumInfo.twoTheta(center_ws_index);
    TS_ASSERT_DELTA(sample_center_angle * 180. / M_PI, twotheta_log, 0.0001);

    // symmetry from now on!
    size_t ws_d_row = 10;
    size_t ws_d_col = 15;

    size_t ll_ws_index =
        (center_row - ws_d_row) + (center_col - ws_d_col) * 256;

    double ll_sample_r = spectrumInfo.l2(ll_ws_index);

    size_t lr_ws_index =
        (center_row + ws_d_row) + (center_col - ws_d_col) * 256;

    double lr_sample_r = spectrumInfo.l2(lr_ws_index);

    TS_ASSERT_DELTA(ll_sample_r, lr_sample_r, 0.0000001);

    size_t ur_ws_index =
        (center_row + ws_d_row) + (center_col + ws_d_col) * 256;

    double ur_sample_r = spectrumInfo.l2(ur_ws_index);

    TS_ASSERT_DELTA(ll_sample_r, ur_sample_r, 0.0000001);

    size_t ul_ws_index =
        (center_row - ws_d_row) + (center_col + ws_d_col) * 256;
    double ul_sample_r = spectrumInfo.l2(ul_ws_index);

    TS_ASSERT_DELTA(ul_sample_r, ur_sample_r, 0.0000001);

    // Clean
    AnalysisDataService::Instance().remove("Exp0335_S0038C");
  }

  //----------------------------------------------------------------------------------------------
  /** Test with loading instrument without Spice scan Table and WITHOUT
   * user-specified detector
   * geometry, while the 2theta value is from sample
   *    sample log
   *  Testing includes:
   *  1. Load the instrument without Spice Table;
   *  2. Check the positions of detectors.
   *    (a) at center pixel, 2-theta = 42.797
   * This should have the same result as unti test
   * "test_loadDataUsingSampleLogValue"
   * @brief Load data and instrument with sample log value
   */
  void test_loadDataWithoutSpecifyingDetectorGeometry() {
    // initialize the algorithm
    LoadSpiceXML2DDet loader;
    loader.initialize();

    // set up properties
    const std::string filename("HB3A_exp355_scan0001_0522.xml");
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        loader.setProperty("OutputWorkspace", "Exp0335_S0038F"));
    std::vector<size_t> geometryvec;
    geometryvec.push_back(0);
    geometryvec.push_back(0);
    loader.setProperty("DetectorGeometry", geometryvec);
    loader.setProperty("LoadInstrument", true);
    loader.setProperty("ShiftedDetectorDistance", 0.);

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    // Get data
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Exp0335_S0038F"));
    TS_ASSERT(outws);
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 256 * 256);

    // Value
    // test signal value on various pixels
    // pixel at (256, 1): column 1
    TS_ASSERT_DELTA(outws->readY(255)[0], 1.0, 0.0001);
    // pixel at (254, 256): colun 256
    TS_ASSERT_DELTA(outws->readY(255 * 256 + 138)[0], 2.0, 0.00001);

    // Instrument
    TS_ASSERT(outws->getInstrument());

    // get 2theta from workspace
    double twotheta_raw =
        std::stod(outws->run().getProperty("_2theta")->value());

    Kernel::Property *raw_property = outws->run().getProperty("2theta");
    Kernel::TimeSeriesProperty<double> *twotheta_property =
        dynamic_cast<Kernel::TimeSeriesProperty<double> *>(raw_property);
    TS_ASSERT(twotheta_property);
    double twotheta_log = twotheta_property->valuesAsVector()[0];
    // 2-theta = 42.797

    TS_ASSERT_EQUALS(twotheta_raw, twotheta_log);

    // check the center of the detector
    const auto &spectrumInfo = outws->spectrumInfo();

    // check the center position
    size_t center_row = 115 - 1;
    size_t center_col = 128 - 1;
    size_t center_ws_index = 256 * center_col + center_row;
    const auto center_det_pos = spectrumInfo.position(center_ws_index);
    TS_ASSERT_DELTA(center_det_pos.Y(), 0., 0.00000001);
    double sample_center_distance = spectrumInfo.l2(center_ws_index);
    // TS_ASSERT_DELTA(center_det_pos.X(), )
    TS_ASSERT_DELTA(sample_center_distance, 0.3750, 0.0000001);
    double sample_center_angle = spectrumInfo.twoTheta(center_ws_index);
    TS_ASSERT_DELTA(sample_center_angle * 180. / M_PI, twotheta_log, 0.0001);

    size_t ll_ws_index = 0;
    const auto ll_det_pos = spectrumInfo.position(ll_ws_index);
    double ll_sample_r = spectrumInfo.l2(ll_ws_index);
    TS_ASSERT_DELTA(ll_sample_r, 0.37597, 0.001);

    size_t lu_ws_index = 255; // row = 255, col = 1
    const auto lu_det_pos = spectrumInfo.position(lu_ws_index);
    double lu_sample_r = spectrumInfo.l2(lu_ws_index);
    TS_ASSERT_DELTA(lu_sample_r, 0.37689, 0.001);

    TS_ASSERT_DELTA(ll_det_pos.X(), lu_det_pos.X(), 0.000001);
    TS_ASSERT(ll_det_pos.Y() + lu_det_pos.Y() > 0);

    TS_ASSERT(ll_det_pos.X() > center_det_pos.X());

    // Clean
    AnalysisDataService::Instance().remove("Exp0335_S0038F");
  }

  //----
  void test_loadBinaryFile() {
    // HB3A_exp685_scan0248_0004.bin
    // initialize the algorithm
    LoadSpiceXML2DDet loader;
    loader.initialize();

    // set up properties
    const std::string filename("LaB6_10kev_35deg.bin");
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        loader.setProperty("OutputWorkspace", "Exp0335_S0038F"));
    std::vector<size_t> geometryvec;
    geometryvec.push_back(0);
    geometryvec.push_back(0);
    loader.setProperty("LoadInstrument", false);

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    // Get data
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Exp0335_S0038F"));
    TS_ASSERT(outws);
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 1024 * 1024);

    // Value
    // test signal value on various pixels
    // pixel at (256, 1): column 1
    TS_ASSERT_DELTA(outws->readY(0)[0], 60000., 0.0001);
    TS_ASSERT_DELTA(outws->readY(1)[0], 50000., 0.0001);
    TS_ASSERT_DELTA(outws->readY(2)[0], 40000., 0.0001);

    // Clean
    AnalysisDataService::Instance().remove("Exp0335_S0038F");
  }

  //----------------------------------------------------------------------------------------------
  /** Create SPICE scan table workspace
   * @brief createSpiceScanTable
   * @return
   */
  ITableWorkspace_sptr createSpiceScanTable() {
    ITableWorkspace_sptr datatablews =
        boost::dynamic_pointer_cast<ITableWorkspace>(
            boost::make_shared<DataObjects::TableWorkspace>());
    datatablews->addColumn("int", "Pt.");
    datatablews->addColumn("double", "2theta");
    datatablews->addColumn("double", "m1");
    AnalysisDataService::Instance().addOrReplace("SpiceDataTable", datatablews);

    TableRow row0 = datatablews->appendRow();
    row0 << 1 << 15.0 << -25.8;
    TableRow row1 = datatablews->appendRow();
    row1 << 2 << -15.0 << -25.8;
    TableRow row2 = datatablews->appendRow();
    row2 << 3 << 0.0 << -25.8;

    return datatablews;
  }
};

#endif /* MANTID_DATAHANDLING_LOADSPICEXML2DDETTEST_H_ */
