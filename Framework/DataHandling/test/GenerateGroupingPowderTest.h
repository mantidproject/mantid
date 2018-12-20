#ifndef MANTID_DATAHANDLING_GENERATEGROUPINGPOWDERTEST_H_
#define MANTID_DATAHANDLING_GENERATEGROUPINGPOWDERTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/GenerateGroupingPowder.h"
#include "MantidDataHandling/LoadDetectorsGroupingFile.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <fstream>

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class GenerateGroupingPowderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GenerateGroupingPowderTest *createSuite() {
    return new GenerateGroupingPowderTest();
  }
  static void destroySuite(GenerateGroupingPowderTest *suite) { delete suite; }

  void test_Init() {
    GenerateGroupingPowder alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    LoadEmptyInstrument lei;
    lei.initialize();
    lei.setPropertyValue("Filename", "CNCS_Definition.xml");
    std::string wsName = "LoadEmptyInstrumentCNCS";
    lei.setPropertyValue("OutputWorkspace", wsName);
    TS_ASSERT_THROWS_NOTHING(lei.execute(););
    TS_ASSERT(lei.isExecuted());

    GenerateGroupingPowder alg;
    std::string xmlFile("PowderGrouping.xml");
    double step = 10;

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("GroupingFilename", xmlFile));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AngleStep", "10"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    xmlFile = alg.getPropertyValue("GroupingFilename");
    std::string parFile = xmlFile;
    parFile.replace(parFile.end() - 3, parFile.end(), "par");

    // Check the results
    // par file
    std::ifstream pf(parFile.c_str());
    std::size_t nDet;
    pf >> nDet;
    TS_ASSERT_EQUALS(nDet, 14);
    const auto &detectorInfo = ws->detectorInfo();
    for (std::size_t i = 0; i < nDet; ++i) {
      double r, th, phi, dx, dy, tth;
      detid_t detID;
      pf >> r >> th >> phi >> dx >> dy >> detID;
      TS_ASSERT_DELTA(r, 3.5, 0.2);
      TS_ASSERT_DELTA(th, step * (static_cast<double>(i) + 0.5), 0.5 * step);
      TS_ASSERT_EQUALS(phi, 0);
      TS_ASSERT_DELTA(dx, r * step * Geometry::deg2rad, 0.01);
      TS_ASSERT_EQUALS(dy, 0.01);
      tth = detectorInfo.twoTheta(detectorInfo.indexOf(detID)) *
            Geometry::rad2deg;
      TS_ASSERT_LESS_THAN(tth, static_cast<double>(i + 1) * step);
      TS_ASSERT_LESS_THAN(static_cast<double>(i) * step, tth);
    }
    pf.close();

    LoadDetectorsGroupingFile load2;
    load2.initialize();

    TS_ASSERT(load2.setProperty("InputFile", xmlFile));
    TS_ASSERT(load2.setProperty("OutputWorkspace", "GroupPowder"));

    load2.execute();
    TS_ASSERT(load2.isExecuted());

    DataObjects::GroupingWorkspace_sptr gws2 =
        boost::dynamic_pointer_cast<DataObjects::GroupingWorkspace>(
            API::AnalysisDataService::Instance().retrieve("GroupPowder"));

    TS_ASSERT_DELTA(gws2->dataY(0)[0], 13.0, 1.0E-5);    // 130.6 degrees
    TS_ASSERT_DELTA(gws2->dataY(10000)[0], 9.0, 1.0E-5); // 97.4 degrees
    TS_ASSERT_DELTA(gws2->dataY(20000)[0], 6.0, 1.0E-5); // 62.9 degrees
    TS_ASSERT_DELTA(gws2->dataY(30000)[0], 2.0, 1.0E-5); // 27.8 degrees
    TS_ASSERT_DELTA(gws2->dataY(40000)[0], 1.0, 1.0E-5); // 14.5 degrees
    TS_ASSERT_DELTA(gws2->dataY(50000)[0], 4.0, 1.0E-5); // 49.7 degrees

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(wsName);
    AnalysisDataService::Instance().remove("GroupPowder");
    // Delete files
    unlink(xmlFile.c_str());
    unlink(parFile.c_str());
  }
};

#endif /* MANTID_DATAHANDLING_GENERATEGROUPINGPOWDERTEST_H_ */
