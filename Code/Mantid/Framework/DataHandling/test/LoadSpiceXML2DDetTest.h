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

  void test_LoadHB3AXML2InstrumentedWS() {
    std::string idffname(
        "/home/wzz/Mantid/Code/Mantid/instrument/HB3A_Definition.xml");

    // Table workspace
    ITableWorkspace_sptr datatablews =
        boost::dynamic_pointer_cast<ITableWorkspace>(
            boost::make_shared<DataObjects::TableWorkspace>());
    datatablews->addColumn("int", "Pt.");
    datatablews->addColumn("double", "2theta");
    AnalysisDataService::Instance().addOrReplace("SpiceDataTable", datatablews);

    TableRow row0 = datatablews->appendRow();
    row0 << 1 << 15.0;
    TableRow row1 = datatablews->appendRow();
    row1 << 2 << 42.709750;

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
    // loader.setProperty("InstrumentFilename", idffname);
    loader.setProperty("SpiceTableWorkspace", "SpiceDataTable");

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    // Get data
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Exp0335_S0038"));
    TS_ASSERT(outws);
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 256 * 256);

    // Instrument
    TS_ASSERT(outws->getInstrument());

    // Detector distance
    Kernel::V3D sample = outws->getInstrument()->getSample()->getPos();
    Kernel::V3D det0pos = outws->getDetector(0)->getPos();
    Kernel::V3D det255pos = outws->getDetector(255)->getPos();
    Kernel::V3D detlast = outws->getDetector(256 * 256 - 1)->getPos();
    double dist0 = sample.distance(det0pos);
    double dist255 = sample.distance(det255pos);
    double distlast = sample.distance(detlast);

    TS_ASSERT_DELTA(dist0, dist255, 0.0001);
    TS_ASSERT_DELTA(dist0, distlast, 0.0001);

    TS_ASSERT_DELTA(outws->readY(255)[0], 1.0, 0.0001);
    TS_ASSERT_DELTA(outws->readY(253 * 256 + 9)[0], 1.0, 0.00001);
  }
};

#endif /* MANTID_DATAHANDLING_LOADSPICEXML2DDETTEST_H_ */
