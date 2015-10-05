#ifndef MANTID_DATAHANDLING_LOADSPICEXML2DDETTEST_H_
#define MANTID_DATAHANDLING_LOADSPICEXML2DDETTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadSpiceXML2DDet.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"

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

  void test_Init() {
    LoadSpiceXML2DDet testalg;
    testalg.initialize();
    TS_ASSERT(testalg.isInitialized());
  }

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
    TS_ASSERT_DELTA(outws->readY(135)[120], 4., 0.000001);

    // Check the sample logs
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
  /** Test algorithm with loading HB3A
   * @brief test_LoadHB3AXML2InstrumentedWS
   * Testing include
   * (1) 2theta = -15 degree (15 degree in SPICE): distance of 4 corners should
   * be same. scattering
   *     angles should be paired;
   * (2) 2theta = 0 degree: scattering angle of all 4 corners should be same;
   * (3) 2theta = 15 degree: scattering angles should be symmetric to case 1
   */
  void test_LoadHB3AXML2InstrumentedWS() {

    // Set up Spice table workspace for log value
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
    loader.setProperty("SpiceTableWorkspace", "SpiceDataTable");
    loader.setProperty("PtNumber", 1);

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

    Kernel::V3D source = outws->getInstrument()->getSource()->getPos();
    Kernel::V3D sample = outws->getInstrument()->getSample()->getPos();
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

    Kernel::V3D det0_sample = det0pos - sample;
    double twotheta0 = det0_sample.angle(sample_source) * 180. / 3.14159265;
    TS_ASSERT_DELTA(twotheta0, 11.6252, 0.0001);

    Kernel::V3D detTL_sample = detlast0 - sample;
    double twotheta_tl = detTL_sample.angle(sample_source) * 180. / 3.14159265;
    TS_ASSERT_DELTA(twotheta0, twotheta_tl, 0.00001);

    Kernel::V3D det255_sample = det255pos - sample;
    double twotheta255 = det255_sample.angle(sample_source) * 180. / 3.14159265;
    TS_ASSERT_DELTA(twotheta255, 19.5328, 0.0001);

    Kernel::V3D detlast_sample = detlast - sample;
    double twothetalast =
        detlast_sample.angle(sample_source) * 180. / 3.14159265;

    TS_ASSERT_DELTA(twotheta255, twothetalast, 0.00001);

    Kernel::V3D detmid_sample = detmiddle - sample;
    double twotheta_middle =
        detmid_sample.angle(sample_source) * 180. / 3.1415926;
    TS_ASSERT_DELTA(twotheta_middle, 15.0, 0.02);

    Kernel::V3D detedgemid_sample = detedgemiddle - sample;
    double twotheta_edgemiddle =
        detedgemid_sample.angle(sample_source) * 180. / 3.14159265;
    TS_ASSERT_DELTA(twotheta_edgemiddle, 10.8864, 0.0001);

    TS_ASSERT_LESS_THAN(twotheta0, twotheta_middle);
    TS_ASSERT_LESS_THAN(twotheta_middle, twothetalast);

    //-------------------------------------------------------------------------------
    // Case: 2theta = 0
    //-------------------------------------------------------------------------------
    LoadSpiceXML2DDet loader2;
    loader2.initialize();

    TS_ASSERT_THROWS_NOTHING(loader2.setProperty("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        loader2.setProperty("OutputWorkspace", "Exp0335_S0038B"));
    loader2.setProperty("DetectorGeometry", sizelist);
    loader2.setProperty("LoadInstrument", true);
    loader2.setProperty("SpiceTableWorkspace", "SpiceDataTable");
    loader2.setProperty("PtNumber", 3);

    loader2.execute();
    TS_ASSERT(loader2.isExecuted());

    // Get data
    MatrixWorkspace_sptr outws2 = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Exp0335_S0038B"));
    TS_ASSERT(outws2);

    // Check instrument
    Kernel::V3D source2 = outws2->getInstrument()->getSource()->getPos();
    Kernel::V3D sample2 = outws2->getInstrument()->getSample()->getPos();
    Kernel::V3D det0pos2 = outws2->getDetector(0)->getPos();
    Kernel::V3D det255pos2 = outws2->getDetector(255)->getPos();
    Kernel::V3D detlast0_2 = outws2->getDetector(256 * 255)->getPos();
    Kernel::V3D detlast2 = outws2->getDetector(256 * 256 - 1)->getPos();
    Kernel::V3D detmiddle2 =
        outws2->getDetector(256 / 2 * 256 + 256 / 2)->getPos();
    std::cout << "Case 2: Middle Det " << detmiddle2.X() << ", "
              << detmiddle2.Y() << ", " << detmiddle2.Z() << "\n";

    // detector distance
    double dist0b = sample2.distance(det0pos2);
    double dist255b = sample2.distance(det255pos2);
    double distlastb = sample2.distance(detlast2);
    // double distlast0b = sample2.distance(detlast0_2);
    double distmiddleb = sample2.distance(detmiddle2);

    TS_ASSERT_DELTA(dist0b, dist0, 0.000001);
    TS_ASSERT_DELTA(distmiddleb, distmiddle, 0.00001);
    TS_ASSERT_DELTA(distlastb, distlast, 0.00001);
    TS_ASSERT_DELTA(dist255b, dist255, 0.00001);

    // 2theta
    sample_source = sample2 - source2;

    detmid_sample = detmiddle2 - sample2;
    twotheta_middle = detmid_sample.angle(sample_source) * 180. / 3.1415926;
    TS_ASSERT_DELTA(twotheta_middle, 0.0228, 0.0001);

    det0_sample = det0pos2 - sample2;
    double twotheta0_2 = det0_sample.angle(sample_source) * 180. / 3.14159265;
    std::cout << "Case 2: 2theta at (1, 1) " << twotheta0_2 << "\n";

    detTL_sample = detlast0_2 - sample2;
    twotheta_tl = detTL_sample.angle(sample_source) * 180. / 3.14159265;

    det255_sample = det255pos2 - sample2;
    double twotheta255_2 =
        det255_sample.angle(sample_source) * 180. / 3.14159265;
    std::cout << "Case 2: 2theta at (1, 256) " << twotheta255_2 << "\n";

    detlast_sample = detlast2 - sample2;
    twothetalast = detlast_sample.angle(sample_source) * 180. / 3.14159265;
    std::cout << "Case 2: 2theta at (256, 256) " << twothetalast << "\n";

    Kernel::V3D det_edgemiddel_left =
        outws2->getDetector(256 * 256 / 2)->getPos();
    Kernel::V3D det_edgemiddel_right =
        outws2->getDetector(256 * 256 / 2 + 255)->getPos();
    Kernel::V3D det_edgemiddle_left_sample = det_edgemiddel_left - sample;
    Kernel::V3D det_edgemiddle_right_sample = det_edgemiddel_right - sample;
    double inplane1 = det_edgemiddle_left_sample.angle(sample_source);
    double inplane2 = det_edgemiddle_right_sample.angle(sample_source);
    std::cout << "In plain left  side = " << inplane1 * 180 / 3.1415926 << "\n";
    std::cout << "In plain right side = " << inplane2 * 180 / 3.1415926 << "\n";

    TS_ASSERT_DELTA(twotheta0_2, twothetalast, 0.00001);
    TS_ASSERT_DELTA(twotheta0_2, twotheta_tl, 0.00001);
    TS_ASSERT_DELTA(twotheta0_2, twotheta255_2, 0.00001);

    // Case 3: symmetry test
    LoadSpiceXML2DDet loader3;
    loader3.initialize();

    TS_ASSERT_THROWS_NOTHING(loader3.setProperty("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        loader3.setProperty("OutputWorkspace", "Exp0335_S0038C"));
    loader3.setProperty("DetectorGeometry", sizelist);
    loader3.setProperty("LoadInstrument", true);
    loader3.setProperty("SpiceTableWorkspace", "SpiceDataTable");
    loader3.setProperty("PtNumber", 2);

    loader3.execute();
    TS_ASSERT(loader3.isExecuted());

    // Get data
    MatrixWorkspace_sptr outws3 = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Exp0335_S0038C"));
    TS_ASSERT(outws3);

    // Check instrument
    Kernel::V3D source3 = outws3->getInstrument()->getSource()->getPos();
    Kernel::V3D sample3 = outws3->getInstrument()->getSample()->getPos();
    Kernel::V3D det0pos3 = outws3->getDetector(0)->getPos();
    Kernel::V3D det255pos3 = outws3->getDetector(255)->getPos();

    // 2theta
    sample_source = sample3 - source3;

    det0_sample = det0pos3 - sample3;
    double twotheta0_3 = det0_sample.angle(sample_source) * 180. / 3.14159265;
    TS_ASSERT_DELTA(twotheta0_3, twotheta255, 0.00001);

    det255_sample = det255pos3 - sample3;
    double twotheta255_3 =
        det255_sample.angle(sample_source) * 180. / 3.14159265;
    TS_ASSERT_DELTA(twotheta255_3, twotheta0, 0.00001);

    // Clean
    AnalysisDataService::Instance().remove("SpiceDataTable");
    AnalysisDataService::Instance().remove("Exp0335_S0038B");
    AnalysisDataService::Instance().remove("Exp0335_S0038");
  }

  void test_LoadHB3AXMLInstrumentNoTable() {
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

    Kernel::V3D source = outws->getInstrument()->getSource()->getPos();
    Kernel::V3D sample = outws->getInstrument()->getSample()->getPos();
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
  }
};

#endif /* MANTID_DATAHANDLING_LOADSPICEXML2DDETTEST_H_ */
