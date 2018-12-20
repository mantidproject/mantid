#ifndef LOADSNSSPECTEST_H_
#define LOADSNSSPECTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadSNSspec.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;

class LoadSNSspecTest : public CxxTest::TestSuite {
public:
  void testConfidence() {
    Mantid::DataHandling::LoadSNSspec loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "LoadSNSspec.txt");

    Mantid::Kernel::FileDescriptor descriptor(
        loader.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(80, loader.confidence(descriptor));
  }

  void testName() { TS_ASSERT_EQUALS(loader.name(), "LoadSNSspec"); }

  void testVersion() { TS_ASSERT_EQUALS(loader.version(), 1); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
  }

  void testExec() {
    if (!loader.isInitialized())
      loader.initialize();

    std::string outWS("outWS");

    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("Filename", "LoadSNSspec.txt"));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", outWS));

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(outWS)));

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 4); // number of spectrum
    TS_ASSERT_EQUALS(ws->blocksize(), 39);
    TS_ASSERT(!ws->isDistribution())
    TS_ASSERT(ws->isHistogramData())

    TS_ASSERT_EQUALS(ws->x(0)[1], 148.294676917);
    TS_ASSERT_EQUALS(ws->x(2)[38], 314.564466187);
    TS_ASSERT_EQUALS(ws->x(3)[10], 188.738679712);

    TS_ASSERT_EQUALS(ws->y(0)[4], 2.63040177974e-5);
    TS_ASSERT_EQUALS(ws->y(2)[10], 8.80816679672e-5);
    TS_ASSERT_EQUALS(ws->y(3)[38], 1.85253847513e-5);

    TS_ASSERT_EQUALS(ws->e(0)[14], 8.03084255786e-6);
    TS_ASSERT_EQUALS(ws->e(1)[5], 1.42117480748e-5);
    TS_ASSERT_EQUALS(ws->e(3)[36], 5.76084468445e-5);

    AnalysisDataService::Instance().remove(outWS);
  }

  void testExecPoint() {
    if (!loader.isInitialized())
      loader.initialize();

    std::string outWS("outWS");

    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("Filename", "LoadSpecPoint.txt"));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", outWS));

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(outWS)));

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 4); // number of spectrum
    TS_ASSERT_EQUALS(ws->blocksize(), 39);
    TS_ASSERT(!ws->isDistribution())
    TS_ASSERT(!ws->isHistogramData())

    TS_ASSERT_EQUALS(ws->x(0)[1], 148.294676917);
    TS_ASSERT_EQUALS(ws->x(2)[38], 314.564466187);
    TS_ASSERT_EQUALS(ws->x(3)[10], 188.738679712);

    TS_ASSERT_EQUALS(ws->y(0)[4], 2.63040177974e-5);
    TS_ASSERT_EQUALS(ws->y(2)[10], 8.80816679672e-5);
    TS_ASSERT_EQUALS(ws->y(3)[38], 1.85253847513e-5);

    TS_ASSERT_EQUALS(ws->e(0)[14], 8.03084255786e-6);
    TS_ASSERT_EQUALS(ws->e(1)[5], 1.42117480748e-5);
    TS_ASSERT_EQUALS(ws->e(3)[36], 5.76084468445e-5);

    AnalysisDataService::Instance().remove(outWS);
  }

private:
  Mantid::DataHandling::LoadSNSspec loader;
};

#endif /*LoadSNSSPECTEST_H_*/
