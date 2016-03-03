#ifndef MANTID_DATAHANDLING_LOADSPICEXML2DDETTEST_H_
#define MANTID_DATAHANDLING_LOADSPICEXML2DDETTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadSpiceXML2DDet.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
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
   * @brief test_LoadHB3AXML
   */
  void test_LoadHB3AXML() {
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
    int monitorcount =
        atoi(outws->run().getProperty("_monitor")->value().c_str());
    TS_ASSERT_EQUALS(monitorcount, 29);

    // Check motor angles
    TS_ASSERT(outws->run().hasProperty("_2theta"));
    double _2theta = atof(outws->run().getProperty("_2theta")->value().c_str());
    TS_ASSERT_DELTA(_2theta, 42.709750, 0.0000001);

    TS_ASSERT(outws->run().hasProperty("_omega"));
    double _omega = atof(outws->run().getProperty("_omega")->value().c_str());
    TS_ASSERT_DELTA(_omega, 21.354500, 0.0000001);

    TS_ASSERT(outws->run().hasProperty("_chi"));
    double _chi = atof(outws->run().getProperty("_chi")->value().c_str());
    TS_ASSERT_DELTA(_chi, 1.215250, 0.0000001);

    TS_ASSERT(outws->run().hasProperty("_phi"));
    double _phi = atof(outws->run().getProperty("_phi")->value().c_str());
    TS_ASSERT_DELTA(_phi, 144.714218, 0.0000001);

    // check start_time and end_time
    TS_ASSERT(outws->run().hasProperty("start_time"));
    std::string start_time = outws->run().getProperty("start_time")->value();
    TS_ASSERT_EQUALS(start_time, "2015-01-17 13:36:45");

    // Clean
    AnalysisDataService::Instance().remove("Exp0335_S0038");
  }

  //----------------------------------------------------------------------------------------------
  /** Test algorithm with loading HB3A with instrument and presense of SPICE
   * scan table
   *  such that it can be set to zero-2-theta position
   * @brief test_LoadHB3AXML2InstrumentedWS
   * Testing include
   * 1. 2theta = 0 degree: scattering angle of all 4 corners should be same;
   */
  void test_LoadHB3ADataZeroPosition() {
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
    loader.setProperty("PtNumber", 3);
    loader.setProperty("ShiftedDetectorDistance", 0.);

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    // Get data
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Exp0335_S0038"));
    TS_ASSERT(outws);
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 256 * 256);

    // Value
    TS_ASSERT_DELTA(outws->readY(255 * 256)[0], 1.0, 0.0001);
    TS_ASSERT_DELTA(outws->readY(9 * 256 + 253)[0], 1.0, 0.00001);

    // Instrument
    TS_ASSERT(outws->getInstrument());

    // get source and sample positions
    Kernel::V3D sample = outws->getInstrument()->getSample()->getPos();

    // check center of the detector @ (128, 115)
    size_t center_col = 128;
    size_t center_row = 115;
    size_t center_ws_index = (center_row - 1) * 256 + (center_col - 1);
    Kernel::V3D det_center = outws->getDetector(center_ws_index)->getPos();
    // distance to sample
    double dist_r = det_center.distance(sample);
    TS_ASSERT_DELTA(dist_r, 0.3750, 0.0001);
    // center of the detector must be at zero
    TS_ASSERT_DELTA(det_center.X(), 0.0, 0.0000001);
    TS_ASSERT_DELTA(det_center.Y(), 0.0, 0.0000001)

    // test the detectors with symmetric to each other
    // they should have opposite X or Y

    // ll: low-left, lr: low-right, ul: upper-left; ur: upper-right
    size_t row_ll = 0;
    size_t col_ll = 2;
    size_t ws_index_ll = row_ll * 256 + col_ll;
    Kernel::V3D det_ll_pos = outws->getDetector(ws_index_ll)->getPos();

    size_t row_lr = 0;
    size_t col_lr = 2 * 127 - 2;
    size_t ws_index_lr = row_lr * 256 + col_lr;
    Kernel::V3D det_lr_pos = outws->getDetector(ws_index_lr)->getPos();

    size_t row_ul = 114 * 2;
    size_t col_ul = 2;
    size_t ws_index_ul = row_ul * 256 + col_ul;
    Kernel::V3D det_ul_pos = outws->getDetector(ws_index_ul)->getPos();

    size_t row_ur = 114 * 2;
    size_t col_ur = 2 * 127 - 2;
    size_t ws_index_ur = row_ur * 256 + col_ur;
    Kernel::V3D det_ur_pos = outws->getDetector(ws_index_ur)->getPos();

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
  /** Test with loading instrument but without Spice scan Table.
   *  Other tests include check the positions of detectors
   *  2-theta = 42.797
   * @brief test_LoadHB3AXMLInstrumentNoTable
   */
  void test_LoadHB3AXMLInstrumentNoTable() {
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
    TS_ASSERT_DELTA(outws->readY(255 * 256)[0], 1.0, 0.0001);
    TS_ASSERT_DELTA(outws->readY(9 * 256 + 253)[0], 1.0, 0.00001);

    // Instrument
    TS_ASSERT(outws->getInstrument());

    // get 2theta from workspace
    double twotheta_raw =
        atof(outws->run().getProperty("_2theta")->value().c_str());

    Kernel::Property *raw_property = outws->run().getProperty("2theta");
    Kernel::TimeSeriesProperty<double> *twotheta_property =
        dynamic_cast<Kernel::TimeSeriesProperty<double> *>(raw_property);
    TS_ASSERT(twotheta_property);
    double twotheta_log = twotheta_property->valuesAsVector()[0];
    // 2-theta = 42.797

    TS_ASSERT_EQUALS(twotheta_raw, twotheta_log);

    // check the center of the detector
    Kernel::V3D source = outws->getInstrument()->getSource()->getPos();
    Kernel::V3D sample = outws->getInstrument()->getSample()->getPos();

    // check the center position
    size_t center_row = 115 - 1;
    size_t center_col = 128 - 1;
    size_t center_ws_index = 256 * center_row + center_col;
    Kernel::V3D center_det_pos = outws->getDetector(center_ws_index)->getPos();
    TS_ASSERT_DELTA(center_det_pos.Y(), 0., 0.00000001);
    double sample_center_distance = sample.distance(center_det_pos);
    // TS_ASSERT_DELTA(center_det_pos.X(), )
    TS_ASSERT_DELTA(sample_center_distance, 0.3750, 0.0000001);
    double sample_center_angle =
        (sample - source).angle(center_det_pos - sample);
    TS_ASSERT_DELTA(sample_center_angle * 180. / M_PI, twotheta_log, 0.0001);

    /*
    Kernel::V3D det0pos = outws->getDetector(0)->getPos();
    Kernel::V3D det255pos = outws->getDetector(255)->getPos();
    Kernel::V3D detlast0 = outws->getDetector(256 * 255)->getPos();
    Kernel::V3D detlast = outws->getDetector(256 * 256 - 1)->getPos();
    Kernel::V3D detmiddle =
        outws->getDetector(256 / 2 * 256 + 256 / 2)->getPos();
    Kernel::V3D detedgemiddle = outws->getDetector(256 * 256 / 2)->getPos();
    std::cout << "\n";
    std::cout << "Sample position: " << sample.X() << ", " << sample.Y() << ", "
              << sample.Z() << "\n";
    std::cout << "Source position: " << source.X() << ", " << source.Y() << ", "
              << source.Z() << "\n";
    std::cout << "Det Edge Middle: " << detedgemiddle.X() << ", "
              << detedgemiddle.Y() << ", " << detedgemiddle.Z() << "\n";
    std::cout << "Det (1, 1)     : " << det0pos.X() << ", " << det0pos.Y()
              << ", " << det0pos.Z() << "\n";

    // center of the detector must be negative
    TS_ASSERT_LESS_THAN(detmiddle.X(), 0.0);

    // detector distance
    double dist0 = sample.distance(det0pos);
    double dist255 = sample.distance(det255pos);
    double distlast = sample.distance(detlast);
    double distlast0 = sample.distance(detlast0);
    double distmiddle = sample.distance(detmiddle);

    TS_ASSERT_DELTA(dist0, dist255, 0.0001);
    TS_ASSERT_DELTA(dist0, distlast, 0.0001);
    TS_ASSERT_DELTA(dist0, distlast0, 0.0001);
    TS_ASSERT_DELTA(distmiddle, 0.3518, 0.000001);

    // 2theta value
    Kernel::V3D sample_source = sample - source;
    std::cout << "Sample - Source = " << sample_source.X() << ", "
              << sample_source.Y() << ", " << sample_source.Z() << "\n";

    Kernel::V3D detmid_sample = detmiddle - sample;
    double twotheta_middle =
        detmid_sample.angle(sample_source) * 180. / 3.1415926;
    TS_ASSERT_DELTA(twotheta_middle, 42.70975, 0.02);
    */

    // Clean
    AnalysisDataService::Instance().remove("Exp0335_S0038C");
  }

  //----------------------------------------------------------------------------------------------
  /** Test of loading HB3A data with calibrated distance
   * @brief test_loadHB3ACalibratedDetDistance
   */
  void Ntest_loadHB3ACalibratedDetDistance() {
    // Test 2theta = 15 degree
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
    // shift detector distance from 0.3518 to 0.3350
    loader.setProperty("ShiftedDetectorDistance", 0.0);

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    // Get data
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Exp0335_S0038"));
    TS_ASSERT(outws);
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 256 * 256);

    // Value
    TS_ASSERT_DELTA(outws->readY(255)[0], 1.0, 0.0001);
    TS_ASSERT_DELTA(outws->readY(253 * 256 + 9)[0], 1.0, 0.00001);

    // Instrument
    TS_ASSERT(outws->getInstrument());
  }

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
