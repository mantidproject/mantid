// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_APPLYDETAILEDBALANCETEST_H_
#define MANTID_ALGORITHMS_APPLYDETAILEDBALANCETEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/ApplyDetailedBalance.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using Mantid::HistogramData::HistogramX;

class ApplyDetailedBalanceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ApplyDetailedBalanceTest *createSuite() {
    return new ApplyDetailedBalanceTest();
  }
  static void destroySuite(ApplyDetailedBalanceTest *suite) { delete suite; }
  ApplyDetailedBalanceTest()
      : inputWSname("testADBInput"), outputWSname("testADBOutput") {}

  void test_Init() {
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    createWorkspace2D(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", inputWSname));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outputWSname));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Temperature", "300.0"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    Workspace2D_sptr inws, outws;
    TS_ASSERT_THROWS_NOTHING(
        outws = AnalysisDataService::Instance().retrieveWS<Workspace2D>(
            outputWSname));
    TS_ASSERT(outws);
    TS_ASSERT_THROWS_NOTHING(
        inws = AnalysisDataService::Instance().retrieveWS<Workspace2D>(
            inputWSname));
    TS_ASSERT(inws);
    if (!outws)
      return;

    for (std::size_t i = 0; i < 5; ++i) {
      TS_ASSERT_DELTA(
          outws->readY(0)[i],
          M_PI *
              (1 - std::exp(-11.604519 *
                            (inws->readX(0)[i] + inws->readX(0)[i + 1]) / 2. /
                            300.)) *
              inws->readY(0)[i],
          1e-8);
    }
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);
  }

  void test_failTemp() {
    createWorkspace2D(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", inputWSname));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outputWSname));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Temperature", "x"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(!alg.isExecuted());
    Workspace2D_sptr outws;
    TS_ASSERT_THROWS_ANYTHING(
        outws = AnalysisDataService::Instance().retrieveWS<Workspace2D>(
            outputWSname));

    // AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);
  }

  void test_event() {
    EventWorkspace_sptr evin = WorkspaceCreationHelper::createEventWorkspace(
                            1, 5, 10, 0, 1, 3),
                        evout;
    evin->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");
    AnalysisDataService::Instance().add(inputWSname, evin);

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", inputWSname));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outputWSname));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Temperature", "100"));

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        evout = boost::dynamic_pointer_cast<EventWorkspace>(
            AnalysisDataService::Instance().retrieve(outputWSname)));

    double temp = 100.;
    TS_ASSERT(evout); // should be an event workspace
    for (size_t i = 0; i < 5; ++i) {
      double en = static_cast<double>(i) + 0.5;
      double w = M_PI * (1 - std::exp(-en * 11.604519 / temp));
      TS_ASSERT_DELTA(evout->getSpectrum(0).getEvent(i).m_weight, w, w * 1e-6);
    }
    AnalysisDataService::Instance().remove(outputWSname);

    AnalysisDataService::Instance().remove(inputWSname);
  }

  void test_units() {
    createWorkspace2D(true);
    Workspace2D_sptr inws =
        AnalysisDataService::Instance().retrieveWS<Workspace2D>(inputWSname);
    TS_ASSERT_EQUALS(inws->getAxis(0)->unit()->unitID(), "DeltaE");
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", inputWSname);
    alg.setPropertyValue("OutputWorkspace", outputWSname);
    alg.setPropertyValue("Temperature", "300.0");
    alg.setPropertyValue("OutputUnits", "Frequency");
    alg.execute();
    Workspace2D_sptr outws =
        AnalysisDataService::Instance().retrieveWS<Workspace2D>(outputWSname);
    TS_ASSERT_EQUALS(outws->getAxis(0)->unit()->unitID(), "DeltaE_inFrequency");
  }

private:
  ApplyDetailedBalance alg;
  std::string inputWSname;
  std::string outputWSname;

  void createWorkspace2D(bool isHistogram) {
    const int nspecs(1);
    const int nbins(5);
    double h = 0;

    if (isHistogram)
      h = 0.5;

    Workspace2D_sptr ws2D(new Workspace2D);
    ws2D->initialize(nspecs, nbins + 1, nbins);
    ws2D->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");

    Mantid::MantidVec xv;
    if (isHistogram) {
      xv.resize(nbins + 1, 0.0);
    } else {
      xv.resize(nbins, 0.0);
    }
    for (int i = 0; i < nbins; ++i) {
      xv[i] = static_cast<double>((i - 2. - h) * 5.);
    }
    if (isHistogram) {
      xv[nbins] = static_cast<double>((nbins - 2.5) * 5.);
    }

    auto cow_xv = make_cow<HistogramX>(std::move(xv));
    for (int i = 0; i < nspecs; i++) {
      ws2D->setX(i, cow_xv);
      ws2D->dataY(i) = {1, 2, 3, 4, 5};
      ws2D->dataE(i) = {sqrt(1), sqrt(2), sqrt(3), sqrt(4), sqrt(5)};
    }

    AnalysisDataService::Instance().add(inputWSname, ws2D);
  }
};

#endif /* MANTID_ALGORITHMS_APPLYDETAILEDBALANCETEST_H_ */
