#ifndef MANTID_ALGORITHMS_CLEARMASKFLAGTEST_H_
#define MANTID_ALGORITHMS_CLEARMASKFLAGTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/ClearMaskFlag.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::Algorithms::ClearMaskFlag;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::LinearGenerator;
using Mantid::MantidVecPtr;

class ClearMaskFlagTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ClearMaskFlagTest *createSuite() { return new ClearMaskFlagTest(); }
  static void destroySuite(ClearMaskFlagTest *suite) { delete suite; }

  void test_Init() {
    ClearMaskFlag alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // create a workspace
    const int numspec = 9;
    const int nummask = 5;
    Instrument_sptr instr = boost::dynamic_pointer_cast<Instrument>(
        ComponentCreationHelper::createTestInstrumentCylindrical(1));
    Detector *d = new Detector("det", 0, nullptr);
    instr->add(d);
    instr->markAsDetector(d);

    // create the workspace
    auto space2D = createWorkspace<Workspace2D>(numspec, 6, 5);
    BinEdges x(6, LinearGenerator(10.0, 1.0));
    Counts y(5, 1.0);
    CountStandardDeviations e(5, 1.0);
    for (int j = 0; j < numspec; ++j) {
      space2D->setBinEdges(j, x);
      space2D->setCounts(j, y);
      space2D->setCountStandardDeviations(j, e);
      space2D->getSpectrum(j).setSpectrumNo(j);
      space2D->getSpectrum(j).setDetectorID(j);
    }
    space2D->setInstrument(instr);

    // set the mask on a bunch of spectra
    auto &detectorInfo = space2D->mutableDetectorInfo();
    for (int j = 0; j < nummask; ++j) {
      detectorInfo.setMasked(j, true);
    }

    // register the workspace in the data service
    std::string wsName("ClearMaskFlagTest_WS");
    AnalysisDataService::Instance().addOrReplace(wsName, space2D);

    // run the algorithm with nothing masked
    ClearMaskFlag alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", wsName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // retrieve the workspace from data service. TODO: Change to your desired
    // type
    Workspace2D_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<Workspace2D>(wsName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // check the results
    const auto &resultDetInfo = ws->detectorInfo();
    for (int j = 0; j < numspec; ++j) {
      TS_ASSERT(!resultDetInfo.isMasked(j));
    }

    // remove workspace from the data service.
    AnalysisDataService::Instance().remove(wsName);
  }
};

#endif /* MANTID_ALGORITHMS_CLEARMASKFLAGTEST_H_ */
