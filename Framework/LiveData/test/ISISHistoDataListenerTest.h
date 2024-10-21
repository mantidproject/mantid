// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidFrameworkTestHelpers/FacilityHelper.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidLiveData/ISIS/FakeISISHistoDAE.h"
#include "MantidLiveData/ISIS/ISISHistoDataListener.h"

#include <cxxtest/TestSuite.h>

#include <Poco/ActiveResult.h>
#include <Poco/Thread.h>
#include <algorithm>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::LiveData;

namespace {

/**
 * Fake Algorithm.
 */
class FakeAlgorithm : public Algorithm {
public:
  void exec() override { /*Do nothing*/ }
  void init() override { /*Do nothing*/ }
  const std::string name() const override { return "FakeAlgorithm"; }
  int version() const override { return 1; }
  const std::string summary() const override { return ""; }
};
} // namespace

class ISISHistoDataListenerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ISISHistoDataListenerTest *createSuite() { return new ISISHistoDataListenerTest(); }
  static void destroySuite(ISISHistoDataListenerTest *suite) { delete suite; }

  ISISHistoDataListenerTest() { Mantid::API::FrameworkManager::Instance(); }

  void test_Receiving_data() {
// cannot make it work for linux
#ifdef _WIN32
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilities.xml", "TEST");

    FakeISISHistoDAE dae;
    dae.initialize();
    dae.setProperty("NPeriods", 1);
    auto res = dae.executeAsync();

    FakeAlgorithm alg;
    alg.declareProperty(std::make_unique<Kernel::ArrayProperty<specnum_t>>("SpectraList", ""));
    int s[] = {1, 2, 3, 10, 11, 95, 96, 97, 98, 99, 100};
    std::vector<specnum_t> specs;
    specs.assign(s, s + 11);
    alg.setProperty("SpectraList", specs);

    auto listener = Mantid::API::LiveListenerFactory::Instance().create("TESTHISTOLISTENER", true, &alg);
    TS_ASSERT(listener);
    TSM_ASSERT("Listener has failed to connect", listener->isConnected());
    if (!listener->isConnected())
      return;

    auto outWS = listener->extractData();
    auto ws = std::dynamic_pointer_cast<API::MatrixWorkspace>(outWS);
    // TS_ASSERT( ws );
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 11);
    TS_ASSERT_EQUALS(ws->blocksize(), 30);

    dae.cancel();

    TS_ASSERT_EQUALS(ws->x(0).size(), 31);
    TS_ASSERT_EQUALS(ws->x(0)[0], 10000);
    TS_ASSERT_DELTA(ws->x(0)[1], 10100, 1e-6);
    TS_ASSERT_DELTA(ws->x(0)[30], 13000.0, 1e-6);

    TS_ASSERT_EQUALS(ws->x(4).size(), 31);
    TS_ASSERT_EQUALS(ws->x(4)[0], 10000);
    TS_ASSERT_DELTA(ws->x(4)[1], 10100, 1e-6);
    TS_ASSERT_DELTA(ws->x(4)[30], 13000.0, 1e-6);

    TS_ASSERT_EQUALS(ws->y(2)[0], 3);
    TS_ASSERT_EQUALS(ws->y(2)[5], 3);
    TS_ASSERT_EQUALS(ws->y(2)[29], 3);

    TS_ASSERT_EQUALS(ws->y(4)[0], 11);
    TS_ASSERT_EQUALS(ws->y(4)[5], 11);
    TS_ASSERT_EQUALS(ws->y(4)[29], 11);

    TS_ASSERT_EQUALS(ws->y(7)[0], 97);
    TS_ASSERT_EQUALS(ws->y(7)[5], 97);
    TS_ASSERT_EQUALS(ws->y(7)[29], 97);

    TS_ASSERT_EQUALS(ws->e(2)[0], sqrt(3.0));
    TS_ASSERT_EQUALS(ws->e(2)[5], sqrt(3.0));
    TS_ASSERT_EQUALS(ws->e(2)[29], sqrt(3.0));

    TS_ASSERT_EQUALS(ws->e(4)[0], sqrt(11.0));
    TS_ASSERT_EQUALS(ws->e(4)[5], sqrt(11.0));
    TS_ASSERT_EQUALS(ws->e(4)[29], sqrt(11.0));

    TS_ASSERT_EQUALS(ws->e(7)[0], sqrt(97.0));
    TS_ASSERT_EQUALS(ws->e(7)[5], sqrt(97.0));
    TS_ASSERT_EQUALS(ws->e(7)[29], sqrt(97.0));

    auto &spec = ws->getSpectrum(0);
    TS_ASSERT_EQUALS(spec.getSpectrumNo(), 1)
    auto dets = spec.getDetectorIDs();
    TS_ASSERT_EQUALS(dets.size(), 1);
    TS_ASSERT_EQUALS(*dets.begin(), 1);

    auto &spec2 = ws->getSpectrum(3);
    TS_ASSERT_EQUALS(spec2.getSpectrumNo(), 10)
    dets = spec2.getDetectorIDs();
    TS_ASSERT_EQUALS(dets.size(), 1);
    TS_ASSERT_EQUALS(*dets.begin(), 4);

    res.wait();
#else
    TS_ASSERT(true);
#endif
  }

  void test_Receiving_multiperiod_data() {
#ifdef _WIN32
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilities.xml", "TEST");

    FakeISISHistoDAE dae;
    dae.initialize();
    dae.setProperty("NPeriods", 2);
    auto res = dae.executeAsync();

    auto listener = Mantid::API::LiveListenerFactory::Instance().create("TESTHISTOLISTENER", true);
    TS_ASSERT(listener);
    TSM_ASSERT("Listener has failed to connect", listener->isConnected());
    if (!listener->isConnected())
      return;

    auto outWS = listener->extractData();
    auto group = std::dynamic_pointer_cast<WorkspaceGroup>(outWS);
    TS_ASSERT(group);
    TS_ASSERT_EQUALS(group->size(), 2);
    auto ws1 = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
    TS_ASSERT(ws1);
    auto ws2 = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(1));
    TS_ASSERT(ws2);

    TS_ASSERT_EQUALS(ws1->getNumberHistograms(), 100);
    TS_ASSERT_EQUALS(ws1->blocksize(), 30);

    TS_ASSERT_EQUALS(ws2->getNumberHistograms(), 100);
    TS_ASSERT_EQUALS(ws2->blocksize(), 30);

    TS_ASSERT_EQUALS(ws1->x(0).size(), 31);
    TS_ASSERT_EQUALS(ws1->x(0)[0], 10000);
    TS_ASSERT_DELTA(ws1->x(0)[1], 10100, 1e-6);
    TS_ASSERT_DELTA(ws1->x(0)[30], 13000, 1e-6);

    TS_ASSERT_EQUALS(ws1->x(4).size(), 31);
    TS_ASSERT_EQUALS(ws1->x(4)[0], 10000);
    TS_ASSERT_DELTA(ws1->x(4)[1], 10100, 1e-6);
    TS_ASSERT_DELTA(ws1->x(4)[30], 13000, 1e-6);

    TS_ASSERT_EQUALS(ws2->x(0).size(), 31);
    TS_ASSERT_EQUALS(ws2->x(0)[0], 10000);
    TS_ASSERT_DELTA(ws2->x(0)[1], 10100, 1e-6);
    TS_ASSERT_DELTA(ws2->x(0)[30], 13000, 1e-6);

    TS_ASSERT_EQUALS(ws2->x(44).size(), 31);
    TS_ASSERT_EQUALS(ws2->x(44)[0], 10000);
    TS_ASSERT_DELTA(ws2->x(44)[1], 10100, 1e-6);
    TS_ASSERT_DELTA(ws2->x(44)[30], 13000, 1e-6);

    TS_ASSERT_EQUALS(ws1->y(2)[0], 3);
    TS_ASSERT_EQUALS(ws1->y(2)[5], 3);
    TS_ASSERT_EQUALS(ws1->y(2)[29], 3);

    TS_ASSERT_EQUALS(ws1->y(44)[0], 45);
    TS_ASSERT_EQUALS(ws1->y(44)[5], 45);
    TS_ASSERT_EQUALS(ws1->y(44)[29], 45);

    TS_ASSERT_EQUALS(ws1->y(77)[0], 78);
    TS_ASSERT_EQUALS(ws1->y(77)[5], 78);
    TS_ASSERT_EQUALS(ws1->y(77)[29], 78);

    TS_ASSERT_EQUALS(ws2->y(2)[0], 1003);
    TS_ASSERT_EQUALS(ws2->y(2)[5], 1003);
    TS_ASSERT_EQUALS(ws2->y(2)[29], 1003);

    TS_ASSERT_EQUALS(ws2->y(44)[0], 1045);
    TS_ASSERT_EQUALS(ws2->y(44)[5], 1045);
    TS_ASSERT_EQUALS(ws2->y(44)[29], 1045);

    TS_ASSERT_EQUALS(ws2->y(77)[0], 1078);
    TS_ASSERT_EQUALS(ws2->y(77)[5], 1078);
    TS_ASSERT_EQUALS(ws2->y(77)[29], 1078);

    auto &spec10 = ws1->getSpectrum(0);
    TS_ASSERT_EQUALS(spec10.getSpectrumNo(), 1)
    auto dets = spec10.getDetectorIDs();
    TS_ASSERT_EQUALS(dets.size(), 1);
    TS_ASSERT_EQUALS(*dets.begin(), 1);

    auto &spec13 = ws1->getSpectrum(3);
    TS_ASSERT_EQUALS(spec13.getSpectrumNo(), 4)
    dets = spec13.getDetectorIDs();
    TS_ASSERT_EQUALS(dets.size(), 1);
    TS_ASSERT_EQUALS(*dets.begin(), 4);

    auto &spec20 = ws2->getSpectrum(0);
    TS_ASSERT_EQUALS(spec20.getSpectrumNo(), 1)
    dets = spec20.getDetectorIDs();
    TS_ASSERT_EQUALS(dets.size(), 1);
    TS_ASSERT_EQUALS(*dets.begin(), 1);

    auto &spec23 = ws2->getSpectrum(3);
    TS_ASSERT_EQUALS(spec23.getSpectrumNo(), 4)
    dets = spec23.getDetectorIDs();
    TS_ASSERT_EQUALS(dets.size(), 1);
    TS_ASSERT_EQUALS(*dets.begin(), 4);

    dae.cancel();
    res.wait();
#else
    TS_ASSERT(true);
#endif
  }

  void test_Receiving_selected_periods() {
#ifdef _WIN32
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilities.xml", "TEST");

    FakeISISHistoDAE dae;
    dae.initialize();
    dae.setProperty("NSpectra", 30);
    dae.setProperty("NPeriods", 4);
    auto res = dae.executeAsync();

    FakeAlgorithm alg;
    alg.declareProperty(std::make_unique<Kernel::ArrayProperty<int>>("PeriodList"));
    std::vector<int> periods(2);
    periods[0] = 2;
    periods[1] = 3;
    alg.setProperty("PeriodList", periods);

    auto listener = Mantid::API::LiveListenerFactory::Instance().create("TESTHISTOLISTENER", true, &alg);
    TS_ASSERT(listener);
    TSM_ASSERT("Listener has failed to connect", listener->isConnected());
    if (!listener->isConnected())
      return;

    auto outWS = listener->extractData();
    auto group = std::dynamic_pointer_cast<WorkspaceGroup>(outWS);
    TS_ASSERT(group);
    TS_ASSERT_EQUALS(group->size(), 2);

    auto ws = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->y(2)[0], 1003);
    TS_ASSERT_EQUALS(ws->y(2)[5], 1003);
    TS_ASSERT_EQUALS(ws->y(2)[29], 1003);

    ws = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(1));
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->y(2)[0], 2003);
    TS_ASSERT_EQUALS(ws->y(2)[5], 2003);
    TS_ASSERT_EQUALS(ws->y(2)[29], 2003);

    dae.cancel();
    res.wait();
#else
    TS_ASSERT(true);
#endif
  }

  void test_Receiving_selected_monitors() {
#ifdef _WIN32
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilities.xml", "TEST");

    FakeISISHistoDAE dae;
    dae.initialize();
    dae.setProperty("NSpectra", 10);
    dae.setProperty("NPeriods", 4);
    dae.setProperty("NBins", 20);
    auto res = dae.executeAsync();

    FakeAlgorithm alg;
    alg.declareProperty(std::make_unique<Kernel::ArrayProperty<int>>("SpectraList"));
    alg.declareProperty(std::make_unique<Kernel::ArrayProperty<int>>("PeriodList"));
    alg.setProperty("PeriodList", "1,3");
    // FakeISISHistoDAE has 3 monitors with spectra numbers NSpectra+1,
    // NSpectra+2, NSpectra+2
    alg.setProperty("SpectraList", "11-13");

    auto listener = Mantid::API::LiveListenerFactory::Instance().create("TESTHISTOLISTENER", true, &alg);
    TS_ASSERT(listener);
    TSM_ASSERT("Listener has failed to connect", listener->isConnected());
    if (!listener->isConnected())
      return;

    auto outWS = listener->extractData();
    auto group = std::dynamic_pointer_cast<WorkspaceGroup>(outWS);
    TS_ASSERT(group);
    TS_ASSERT_EQUALS(group->size(), 2);

    auto ws = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
    TS_ASSERT(ws);
    // monitors in FakeISISHistoDAE have twice the number of bins of normal
    // spectra
    TS_ASSERT_EQUALS(ws->y(2).size(), 40);
    TS_ASSERT_EQUALS(ws->y(2)[0], 13);
    TS_ASSERT_EQUALS(ws->y(2)[5], 13);
    TS_ASSERT_EQUALS(ws->y(2)[29], 13);

    ws = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(1));
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->y(2).size(), 40);
    TS_ASSERT_EQUALS(ws->y(2)[0], 2013);
    TS_ASSERT_EQUALS(ws->y(2)[5], 2013);
    TS_ASSERT_EQUALS(ws->y(2)[29], 2013);

    dae.cancel();
    res.wait();
#else
    TS_ASSERT(true);
#endif
  }

  void test_invalid_spectra_numbers() {
#ifdef _WIN32
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilities.xml", "TEST");

    FakeISISHistoDAE dae;
    dae.initialize();
    dae.setProperty("NSpectra", 10);
    dae.setProperty("NPeriods", 4);
    dae.setProperty("NBins", 20);
    auto res = dae.executeAsync();

    FakeAlgorithm alg;
    alg.declareProperty(std::make_unique<Kernel::ArrayProperty<int>>("SpectraList"));
    alg.declareProperty(std::make_unique<Kernel::ArrayProperty<int>>("PeriodList"));
    alg.setProperty("PeriodList", "1,3");
    // FakeISISHistoDAE has 3 monitors with spectra numbers NSpectra+1,
    // NSpectra+2, NSpectra+2
    alg.setProperty("SpectraList", "14-17");

    auto listener = Mantid::API::LiveListenerFactory::Instance().create("TESTHISTOLISTENER", true, &alg);
    TS_ASSERT(listener);
    TSM_ASSERT("Listener has failed to connect", listener->isConnected());
    if (!listener->isConnected())
      return;

    TS_ASSERT_THROWS(auto outWS = listener->extractData(), const std::invalid_argument &);

    dae.cancel();
    res.wait();
#else
    TS_ASSERT(true);
#endif
  }

  void test_no_period() {
#ifdef _WIN32
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilities.xml", "TEST");

    FakeISISHistoDAE dae;
    dae.initialize();
    dae.setProperty("NPeriods", 4);
    auto res = dae.executeAsync();

    FakeAlgorithm alg;
    alg.declareProperty(std::make_unique<Kernel::ArrayProperty<int>>("PeriodList"));
    std::vector<int> periods(2);
    periods[0] = 2;
    periods[1] = 5; // this period doesn't exist in dae
    alg.setProperty("PeriodList", periods);

    TS_ASSERT_THROWS(auto listener =
                         Mantid::API::LiveListenerFactory::Instance().create("TESTHISTOLISTENER", true, &alg),
                     const std::invalid_argument &);

    dae.cancel();
    res.wait();
#else
    TS_ASSERT(true);
#endif
  }
};
