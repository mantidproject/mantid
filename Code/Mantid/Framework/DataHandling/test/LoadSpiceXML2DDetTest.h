#ifndef MANTID_DATAHANDLING_LOADSPICEXML2DDETTEST_H_
#define MANTID_DATAHANDLING_LOADSPICEXML2DDETTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadSpiceXML2DDet.h"
#include "MantidAPI/MatrixWorkspace.h"

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

    // Clean
    AnalysisDataService::Instance().remove("xp0335_S0038");
  }
};

#endif /* MANTID_DATAHANDLING_LOADSPICEXML2DDETTEST_H_ */
