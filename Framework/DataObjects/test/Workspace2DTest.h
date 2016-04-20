#ifndef WORKSPACE2DTEST_H_
#define WORKSPACE2DTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidKernel/CPUTimer.h"
#include "PropertyManagerHelper.h"

using namespace std;
using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

class Workspace2DTest : public CxxTest::TestSuite {
public:
  int nbins, nhist;
  Workspace2D_sptr ws;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Workspace2DTest *createSuite() { return new Workspace2DTest(); }
  static void destroySuite(Workspace2DTest *suite) { delete suite; }

  Workspace2DTest() {
    nbins = 5;
    nhist = 10;
    ws = Create2DWorkspaceBinned(nhist, nbins);
  }

  static Workspace2D_sptr Create2DWorkspaceBinned(int nhist, int nbins,
                                                  double x0 = 0.0,
                                                  double deltax = 1.0) {
    MantidVecPtr x, y, e;
    x.access().resize(nbins + 1);
    y.access().resize(nbins, 2); // Value of 2.0 in all ys
    e.access().resize(nbins, sqrt(2.0));
    for (int i = 0; i < nbins + 1; ++i) {
      x.access()[i] = x0 + i * deltax;
    }
    Workspace2D_sptr retVal(new Workspace2D());
    retVal->initialize(nhist, nbins + 1, nbins);
    for (int i = 0; i < nhist; i++) {
      retVal->setX(i, x);
      retVal->setData(i, y, e);
    }

    return retVal;
  }

  void testClone() {
    Workspace2D_sptr cloned(ws->clone());

    // Swap ws with cloned pointer, such that we can reuse existing tests.
    ws.swap(cloned);

    // Run all other (non-destructive) tests on ws.
    testInit();
    testId();
    testDataDx();

    // Undo swap, to avoid possible interferences.
    ws.swap(cloned);
  }

  void testInit() {
    ws->setTitle("testInit");
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), nhist);
    ;
    TS_ASSERT_EQUALS(ws->blocksize(), nbins);
    ;
    TS_ASSERT_EQUALS(ws->size(), nbins * nhist);
    ;

    for (int i = 0; i < nhist; ++i) {
      TS_ASSERT_EQUALS(ws->dataX(i).size(), nbins + 1);
      ;
      TS_ASSERT_EQUALS(ws->dataY(i).size(), nbins);
      ;
      TS_ASSERT_EQUALS(ws->dataE(i).size(), nbins);
      ;
    }
  }

  void testId() { TS_ASSERT_EQUALS(ws->id(), "Workspace2D"); }

  void testSetX() {
    double aNumber = 5.3;
    boost::shared_ptr<MantidVec> v =
        boost::make_shared<MantidVec>(nbins, aNumber);
    TS_ASSERT_THROWS_NOTHING(ws->setX(0, v));
    TS_ASSERT_EQUALS(ws->dataX(0)[0], aNumber);
    TS_ASSERT_THROWS(ws->setX(-1, v), std::range_error);
    TS_ASSERT_THROWS(ws->setX(nhist + 5, v), std::range_error);
  }

  void testSetX_cowptr() {
    double aNumber = 5.4;
    MantidVecPtr v;
    v.access() = MantidVec(nbins, aNumber);
    TS_ASSERT_THROWS_NOTHING(ws->setX(0, v));
    TS_ASSERT_EQUALS(ws->dataX(0)[0], aNumber);
    TS_ASSERT_THROWS(ws->setX(-1, v), std::range_error);
    TS_ASSERT_THROWS(ws->setX(nhist + 5, v), std::range_error);
  }

  void testSetData_cowptr() {
    double aNumber = 5.5;
    MantidVecPtr v;
    v.access() = MantidVec(nbins, aNumber);
    TS_ASSERT_THROWS_NOTHING(ws->setData(0, v));
    TS_ASSERT_EQUALS(ws->dataY(0)[0], aNumber);
    TS_ASSERT_DIFFERS(ws->dataY(1)[0], aNumber);
  }

  void testSetData_cowptr2() {
    double aNumber = 5.6;
    MantidVecPtr v, e;
    v.access() = MantidVec(nbins, aNumber);
    e.access() = MantidVec(nbins, aNumber * 2);
    TS_ASSERT_THROWS_NOTHING(ws->setData(0, v, e));
    TS_ASSERT_EQUALS(ws->dataY(0)[0], aNumber);
    TS_ASSERT_EQUALS(ws->dataE(0)[0], aNumber * 2);
    TS_ASSERT_DIFFERS(ws->dataY(1)[0], aNumber);
    TS_ASSERT_DIFFERS(ws->dataE(1)[0], aNumber * 2);
  }

  void testSetData() {
    double aNumber = 5.7;
    const boost::shared_ptr<MantidVec> v =
        boost::make_shared<MantidVec>(nbins, aNumber);
    const boost::shared_ptr<MantidVec> e =
        boost::make_shared<MantidVec>(nbins, aNumber * 2);
    TS_ASSERT_THROWS_NOTHING(ws->setData(0, v, e));
    TS_ASSERT_EQUALS(ws->dataY(0)[0], aNumber);
    TS_ASSERT_EQUALS(ws->dataE(0)[0], aNumber * 2);
    TS_ASSERT_DIFFERS(ws->dataY(1)[0], aNumber);
    TS_ASSERT_DIFFERS(ws->dataE(1)[0], aNumber * 2);
  }

  void testIntegrateSpectra_entire_range() {
    ws = Create2DWorkspaceBinned(nhist, nbins);
    MantidVec sums;
    ws->getIntegratedSpectra(sums, 10, 5, true);
    for (int i = 0; i < nhist; ++i) {
      TS_ASSERT_EQUALS(sums[i], nbins * 2.0);
      ;
    }
  }
  void testIntegrateSpectra_empty_range() {
    ws = Create2DWorkspaceBinned(nhist, nbins);
    MantidVec sums;
    ws->getIntegratedSpectra(sums, 10, 5, false);
    for (int i = 0; i < nhist; ++i) {
      TS_ASSERT_EQUALS(sums[i], 0.0);
      ;
    }
  }

  void testIntegrateSpectra_partial_range() {
    ws = Create2DWorkspaceBinned(nhist, nbins);
    MantidVec sums;
    ws->getIntegratedSpectra(sums, 1.9, 3.2, false);
    for (int i = 0; i < nhist; ++i) {
      TS_ASSERT_EQUALS(sums[i], 4.0);
      ;
    }
  }

  void test_generateHistogram() {
    Workspace2D_sptr ws = Create2DWorkspaceBinned(2, 5);
    MantidVec X, Y, E;
    X.push_back(0.0);
    X.push_back(0.5);
    X.push_back(1.0);
    TS_ASSERT_THROWS_ANYTHING(ws->generateHistogram(2, X, Y, E););
    TS_ASSERT_THROWS_NOTHING(ws->generateHistogram(0, X, Y, E););
    TS_ASSERT_EQUALS(Y.size(), 2);
    TS_ASSERT_EQUALS(E.size(), 2);
    TS_ASSERT_DELTA(Y[0], 1.0, 1e-5);
    TS_ASSERT_DELTA(Y[1], 1.0, 1e-5);
    TS_ASSERT_DELTA(E[0], 1.0, 1e-5);
    TS_ASSERT_DELTA(E[1], 1.0, 1e-5);
  }

  void testDataDx() {
    TS_ASSERT_EQUALS(ws->readDx(0).size(), 6);
    TS_ASSERT_EQUALS(ws->readDx(6)[3], 0.0);

    TS_ASSERT_THROWS_NOTHING(ws->dataDx(6)[3] = 9.9);
    TS_ASSERT_EQUALS(ws->readDx(6)[3], 9.9);
  }

  void test_getMemorySizeForXAxes() {
    ws = Create2DWorkspaceBinned(nhist, nbins);
    // Here they are shared, so only 1 X axis
    TS_ASSERT_EQUALS(ws->getMemorySizeForXAxes(),
                     1 * (nbins + 1) * sizeof(double));
    for (int i = 0; i < nhist; i++) {
      ws->dataX(i)[0] +=
          1; // This modifies the X axis in-place, creatign a copy of it.
    }
    // Now there is a different one for each
    TS_ASSERT_EQUALS(ws->getMemorySizeForXAxes(),
                     nhist * (nbins + 1) * sizeof(double));
  }

  /** Refs #3003: very odd bug when getting detector in parallel only!
   * This does not reproduce it :( */
  void test_getDetector_parallel() {
    int numpixels = 10000;
    Workspace2D_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(numpixels,
                                                                     200);

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < numpixels; i++) {
      IDetector_const_sptr det = ws->getDetector(i);
      TS_ASSERT(det);
    }
  }

  /** Get spectrum() */
  void testGetSpectrum() {
    boost::shared_ptr<MatrixWorkspace> ws = boost::make_shared<Workspace2D>();
    ws->initialize(4, 1, 1);
    ISpectrum *spec = NULL;
    TS_ASSERT_THROWS_NOTHING(spec = ws->getSpectrum(0));
    TS_ASSERT(spec);
    TS_ASSERT_THROWS_NOTHING(spec = ws->getSpectrum(3));
    TS_ASSERT(spec);
    TS_ASSERT_THROWS_ANYTHING(spec = ws->getSpectrum(4));
  }

  /**
   * Test that a Workspace2D_sptr can be held as a property and
   * retrieved as const or non-const sptr,
   * and that the cast from TypedValue works properly
   */
  void testGetProperty_const_sptr() {
    const std::string wsName = "InputWorkspace";
    Workspace2D_sptr wsInput(new Workspace2D());
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Mantid::Kernel::Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    Workspace2D_const_sptr wsConst;
    Workspace2D_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(
        wsConst = manager.getValue<Workspace2D_const_sptr>(wsName));
    TS_ASSERT(wsConst != NULL);
    TS_ASSERT_THROWS_NOTHING(wsNonConst =
                                 manager.getValue<Workspace2D_sptr>(wsName));
    TS_ASSERT(wsNonConst != NULL);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    Workspace2D_const_sptr wsCastConst;
    Workspace2D_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst = (Workspace2D_const_sptr)val);
    TS_ASSERT(wsCastConst != NULL);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (Workspace2D_sptr)val);
    TS_ASSERT(wsCastNonConst != NULL);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }
};

class Workspace2DTestPerformance : public CxxTest::TestSuite {
public:
  int nbins, nhist;
  Workspace2D_sptr ws1;
  Workspace2D_sptr ws2;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Workspace2DTestPerformance *createSuite() {
    return new Workspace2DTestPerformance();
  }
  static void destroySuite(Workspace2DTestPerformance *suite) { delete suite; }

  Workspace2DTestPerformance() {
    nhist = 1000000; // 1 million
    ws1 = WorkspaceCreationHelper::Create2DWorkspaceBinned(nhist, 5);
    ws2 = WorkspaceCreationHelper::Create2DWorkspaceBinned(10, 5);
    for (size_t i = 0; i < 10; i++) {
      ISpectrum *spec = ws2->getSpectrum(i);
      for (detid_t j = detid_t(i) * 100000; j < detid_t(i + 1) * 100000; j++) {
        spec->addDetectorID(j);
      }
    }
  }

  void test_ISpectrum_getDetectorIDs() {
    CPUTimer tim;
    for (size_t i = 0; i < ws1->getNumberHistograms(); i++) {
      const ISpectrum *spec = ws1->getSpectrum(i);
      const auto &detIDs = spec->getDetectorIDs();
      detid_t oneDetId = *detIDs.begin();
      UNUSED_ARG(oneDetId)
    }
    std::cout << tim << " to get detector ID's for " << nhist
              << " spectra using the ISpectrum method." << std::endl;
  }

  void test_ISpectrum_changeDetectorIDs() {
    CPUTimer tim;
    for (size_t i = 0; i < ws1->getNumberHistograms(); i++) {
      ISpectrum *spec = ws1->getSpectrum(i);
      spec->setDetectorID(detid_t(i));
    }
    std::cout << tim << " to set all detector IDs for " << nhist
              << " spectra, using the ISpectrum method (serial)." << std::endl;

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < (int)ws1->getNumberHistograms(); i++) {
      ISpectrum *spec = ws1->getSpectrum(i);
      spec->setDetectorID(detid_t(i));
    }
    std::cout << tim << " to set all detector IDs for " << nhist
              << " spectra, using the ISpectrum method (in parallel)."
              << std::endl;
  }
};

#endif
