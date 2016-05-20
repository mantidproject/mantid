#ifndef MANTID_ALGORITHMS_CLEARMASKFLAGTEST_H_
#define MANTID_ALGORITHMS_CLEARMASKFLAGTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/ClearMaskFlag.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::Algorithms::ClearMaskFlag;
using Mantid::MantidVecPtr;
using Mantid::HistogramData::BinEdges;

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
    Detector *d = new Detector("det", 0, 0);
    instr->markAsDetector(d);

    // create the workspace
    MatrixWorkspace_sptr space =
        WorkspaceFactory::Instance().create("Workspace2D", numspec, 6, 5);
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    BinEdges x(6, 10.0);
    MantidVecPtr vec;
    vec.access().resize(5, 1.0);
    for (int j = 0; j < numspec; ++j) {
      space2D->setBinEdges(j, x);
      space2D->setData(j, vec, vec);
      space2D->getSpectrum(j)->setSpectrumNo(j);
      space2D->getSpectrum(j)->setDetectorID(j);
    }
    space->setInstrument(instr);

    // set the mask on a bunch of spectra
    Mantid::Geometry::ParameterMap &pmap = space->instrumentParameters();
    for (int j = 0; j < nummask; ++j) {
      pmap.addBool(instr->getDetector(j)->getComponentID(), "masked", true);
    }

    // register the workspace in the data service
    std::string wsName("ClearMaskFlagTest_WS");
    AnalysisDataService::Instance().addOrReplace(wsName, space);

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
    Instrument_const_sptr out_instr = ws->getInstrument();
    for (int j = 0; j < numspec; ++j) {
      TS_ASSERT(!out_instr->isDetectorMasked(j));
    }

    // remove workspace from the data service.
    AnalysisDataService::Instance().remove(wsName);
  }
};

#endif /* MANTID_ALGORITHMS_CLEARMASKFLAGTEST_H_ */
