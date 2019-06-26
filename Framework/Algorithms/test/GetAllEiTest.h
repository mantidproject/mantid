// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef GETALLEI_TEST_H_
#define GETALLEI_TEST_H_

#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/GetAllEi.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <memory>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace HistogramData;

namespace {
DataObjects::Workspace2D_sptr createTestingWS(bool noLogs = false) {
  double delay(2000), chopSpeed(100), inital_chop_phase(-3000);
  auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
      2, 1000, true);
  auto pInstrument = ws->getInstrument();
  auto chopper = pInstrument->getComponentByName("chopper-position");

  // add chopper parameters
  auto &paramMap = ws->instrumentParameters();
  const std::string description(
      "The initial rotation phase of the disk used to calculate the time"
      " for neutrons arriving at the chopper according to the formula time = "
      "delay + initial_phase/Speed");
  paramMap.add<double>("double", chopper.get(), "initial_phase",
                       inital_chop_phase, &description);
  paramMap.add<std::string>("string", chopper.get(), "ChopperDelayLog",
                            "fermi_delay");
  paramMap.add<std::string>("string", chopper.get(), "ChopperSpeedLog",
                            "fermi_speed");
  paramMap.add<std::string>("string", chopper.get(), "FilterBaseLog",
                            "is_running");
  paramMap.add<bool>("bool", chopper.get(), "filter_with_derivative", false);

  // test instrument parameters (obtained from workspace):
  auto moderatorPosition = pInstrument->getSource()->getPos();
  auto &spectrumInfo = ws->spectrumInfo();
  double l_chop = chopper->getPos().distance(moderatorPosition);
  double l_mon1 = spectrumInfo.position(0).distance(moderatorPosition);
  double l_mon2 = spectrumInfo.position(1).distance(moderatorPosition);
  //,l_mon1(20-9),l_mon2(20-2);
  double t_chop(delay + inital_chop_phase / chopSpeed);
  double Period =
      (0.5 * 1.e+6) / chopSpeed; // 0.5 because some choppers open twice.

  ws->setBinEdges(0, BinEdges(ws->x(0).size(), LinearGenerator(5, 10)));

  // signal at first monitor
  double t1 = t_chop * l_mon1 / l_chop;
  double t2 = (t_chop + Period) * l_mon1 / l_chop;

  // temporary vars, to avoid redeclaring
  double tm1(0.0);
  double tm2(0.0);

  auto t = ws->points(0);
  std::transform(t.cbegin(), t.cend(), ws->mutableY(0).begin(),
                 [t1, t2, &tm1, &tm2](const double t) {
                   tm1 = t - t1;
                   tm2 = t - t2;
                   return (10000 * std::exp(-tm1 * tm1 / 1000.) +
                           20000 * std::exp(-tm2 * tm2 / 1000.));
                 });

  // signal at second monitor
  t1 = t_chop * l_mon2 / l_chop;
  t2 = (t_chop + Period) * l_mon2 / l_chop;

  std::transform(t.cbegin(), t.cend(), ws->mutableY(1).begin(),
                 [t1, t2, &tm1, &tm2](const double t) {
                   tm1 = t - t1;
                   tm2 = t - t2;
                   return (100 * std::exp(-tm1 * tm1 / 1000.) +
                           200 * std::exp(-tm2 * tm2 / 1000.));
                 });

  if (noLogs)
    return ws;

  auto chopDelayLog =
      std::make_unique<Kernel::TimeSeriesProperty<double>>("Chopper_Delay");
  auto chopSpeedLog =
      std::make_unique<Kernel::TimeSeriesProperty<double>>("Chopper_Speed");
  auto isRunning =
      std::make_unique<Kernel::TimeSeriesProperty<double>>("is_running");

  for (int i = 0; i < 10; i++) {
    auto time = Types::Core::DateAndTime(10 * i, 0);
    chopDelayLog->addValue(time, delay);
    chopSpeedLog->addValue(time, chopSpeed);
    isRunning->addValue(time, 1.);
  }

  ws->mutableRun().addLogData(chopSpeedLog.release());
  ws->mutableRun().addLogData(chopDelayLog.release());
  ws->mutableRun().addLogData(isRunning.release());

  return ws;
}
} // namespace

class GetAllEiTester : public GetAllEi {
public:
  void find_chop_speed_and_delay(const API::MatrixWorkspace_sptr &inputWS,
                                 double &chop_speed, double &chop_delay) {
    GetAllEi::findChopSpeedAndDelay(inputWS, chop_speed, chop_delay);
  }
  void findGuessOpeningTimes(const std::pair<double, double> &TOF_range,
                             double ChopDelay, double Period,
                             std::vector<double> &guess_opening_times) {
    GetAllEi::findGuessOpeningTimes(TOF_range, ChopDelay, Period,
                                    guess_opening_times);
  }
  bool filterLogProvided() const { return (m_pFilterLog != nullptr); }
  double getAvrgLogValue(const API::MatrixWorkspace_sptr &inputWS,
                         const std::string &propertyName) {
    std::vector<Kernel::SplittingInterval> splitter;
    return GetAllEi::getAvrgLogValue(inputWS, propertyName, splitter);
  }
  API::MatrixWorkspace_sptr
  buildWorkspaceToFit(const API::MatrixWorkspace_sptr &inputWS,
                      size_t &wsIndex0) {
    return GetAllEi::buildWorkspaceToFit(inputWS, wsIndex0);
  }
  void findBinRanges(const HistogramX &eBins, const HistogramY &signal,
                     const std::vector<double> &guess_energies,
                     double Eresolution, std::vector<size_t> &irangeMin,
                     std::vector<size_t> &irangeMax,
                     std::vector<bool> &guessValid) {
    GetAllEi::findBinRanges(eBins, signal, guess_energies, Eresolution,
                            irangeMin, irangeMax, guessValid);
  }
  void setResolution(double newResolution) {
    this->m_max_Eresolution = newResolution;
  }
  size_t calcDerivativeAndCountZeros(const std::vector<double> &bins,
                                     const std::vector<double> &signal,
                                     std::vector<double> &deriv,
                                     std::vector<double> &zeros) {
    return GetAllEi::calcDerivativeAndCountZeros(bins, signal, deriv, zeros);
  }
};

class GetAllEiTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetAllEiTest *createSuite() { return new GetAllEiTest(); }
  static void destroySuite(GetAllEiTest *suite) { delete suite; }

  GetAllEiTest() {}

public:
  void testName() { TS_ASSERT_EQUALS(m_getAllEi.name(), "GetAllEi"); }

  void testVersion() { TS_ASSERT_EQUALS(m_getAllEi.version(), 1); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(m_getAllEi.initialize());
    TS_ASSERT(m_getAllEi.isInitialized());
  }
  //
  void test_validators_work() {

    MatrixWorkspace_sptr ws = createTestingWS(true);

    m_getAllEi.initialize();
    m_getAllEi.setProperty("Workspace", ws);
    m_getAllEi.setProperty("OutputWorkspace", "monitor_peaks");
    TSM_ASSERT_THROWS(
        "should throw runtime error on as spectra ID should be positive",
        m_getAllEi.setProperty("Monitor1SpecID", -1),
        const std::invalid_argument &);

    m_getAllEi.setProperty("Monitor1SpecID", 1);
    m_getAllEi.setProperty("Monitor2SpecID", 2);
    m_getAllEi.setProperty("ChopperSpeedLog", "Chopper_Speed");
    m_getAllEi.setProperty("ChopperDelayLog", "Chopper_Delay");
    m_getAllEi.setProperty("FilterBaseLog", "proton_charge");
    m_getAllEi.setProperty("FilterWithDerivative", false);

    TSM_ASSERT_THROWS("should throw runtime error on validation as no "
                      "appropriate logs are defined",
                      m_getAllEi.execute(), const std::runtime_error &);
    auto log_messages = m_getAllEi.validateInputs();
    TSM_ASSERT_EQUALS("Two logs should fail", log_messages.size(), 2);
    // add invalid property type
    ws->mutableRun().addLogData(
        new Kernel::PropertyWithValue<double>("Chopper_Speed", 10.));
    auto log_messages2 = m_getAllEi.validateInputs();
    TSM_ASSERT_EQUALS("Two logs should fail", log_messages2.size(), 2);

    TSM_ASSERT_DIFFERS("should fail for different reason ",
                       log_messages["ChopperSpeedLog"],
                       log_messages2["ChopperSpeedLog"]);
    // add correct property type:
    ws->mutableRun().clearLogs();
    ws->mutableRun().addLogData(
        new Kernel::TimeSeriesProperty<double>("Chopper_Speed"));
    log_messages = m_getAllEi.validateInputs();
    TSM_ASSERT_EQUALS("One log should fail", log_messages.size(), 1);
    TSM_ASSERT("Filter log is not provided ", !m_getAllEi.filterLogProvided());
    ws->mutableRun().addLogData(
        new Kernel::TimeSeriesProperty<double>("Chopper_Delay"));
    ws->mutableRun().addLogData(
        new Kernel::TimeSeriesProperty<double>("proton_charge"));
    log_messages = m_getAllEi.validateInputs();

    TSM_ASSERT_EQUALS("All logs are defined", log_messages.size(), 0);
    TSM_ASSERT("Filter log is provided ", m_getAllEi.filterLogProvided());

    m_getAllEi.setProperty("Monitor1SpecID", 3);
    log_messages = m_getAllEi.validateInputs();
    TSM_ASSERT_EQUALS("Workspace should not have spectra with ID=3",
                      log_messages.size(), 1);
  }
  //
  void test_get_chopper_speed() {

    MatrixWorkspace_sptr ws = createTestingWS(true);

    m_getAllEi.initialize();
    m_getAllEi.setProperty("Workspace", ws);
    m_getAllEi.setProperty("OutputWorkspace", "monitor_peaks");
    m_getAllEi.setProperty("Monitor1SpecID", 1);
    m_getAllEi.setProperty("Monitor2SpecID", 2);
    m_getAllEi.setProperty("ChopperSpeedLog", "Chopper_Speed");
    m_getAllEi.setProperty("ChopperDelayLog", "Chopper_Delay");
    m_getAllEi.setProperty("FilterBaseLog", "proton_charge");
    m_getAllEi.setProperty("FilterWithDerivative", false);

    auto chopSpeed =
        std::make_unique<Kernel::TimeSeriesProperty<double>>("Chopper_Speed");
    for (int i = 0; i < 10; i++) {
      chopSpeed->addValue(Types::Core::DateAndTime(10000 + 10 * i, 0), 1.);
    }
    for (int i = 0; i < 10; i++) {
      chopSpeed->addValue(Types::Core::DateAndTime(100 + 10 * i, 0), 10.);
    }
    for (int i = 0; i < 10; i++) {
      chopSpeed->addValue(Types::Core::DateAndTime(10 * i, 0), 100.);
    }
    ws->mutableRun().addLogData(chopSpeed.release());

    // Test sort log by run time.
    TSM_ASSERT_THROWS(
        "Attempt to get log without start/stop time set should fail",
        m_getAllEi.getAvrgLogValue(ws, "ChopperSpeedLog"),
        const std::runtime_error &);

    ws->mutableRun().setStartAndEndTime(Types::Core::DateAndTime(90, 0),
                                        Types::Core::DateAndTime(10000, 0));
    double val = m_getAllEi.getAvrgLogValue(ws, "ChopperSpeedLog");
    TS_ASSERT_DELTA(val, (10 * 10 + 100.) / 11., 1.e-6);

    ws->mutableRun().setStartAndEndTime(Types::Core::DateAndTime(100, 0),
                                        Types::Core::DateAndTime(10000, 0));
    val = m_getAllEi.getAvrgLogValue(ws, "ChopperSpeedLog");
    TS_ASSERT_DELTA(val, 10., 1.e-6);

    // Test sort log by log.
    auto chopDelay =
        std::make_unique<Kernel::TimeSeriesProperty<double>>("Chopper_Delay");
    auto goodFram =
        std::make_unique<Kernel::TimeSeriesProperty<double>>("proton_charge");

    for (int i = 0; i < 10; i++) {
      auto time = Types::Core::DateAndTime(200 + 10 * i, 0);
      chopDelay->addValue(time, 10.);
      if (i < 2) {
        goodFram->addValue(time, 1);
      } else {
        goodFram->addValue(time, 0);
      }
    }
    for (int i = 0; i < 10; i++) {
      auto time = Types::Core::DateAndTime(100 + 10 * i, 0);
      chopDelay->addValue(time, 0.1);
      goodFram->addValue(time, 1);
    }
    for (int i = 0; i < 10; i++) {
      auto time = Types::Core::DateAndTime(10 * i, 0);
      chopDelay->addValue(time, 1.);
      goodFram->addValue(time, 0);
    }
    ws->mutableRun().addLogData(chopDelay.release());
    ws->mutableRun().addLogData(goodFram.release());

    // Run validate as this will set up property, which indicates filter log
    // presence
    auto errors = m_getAllEi.validateInputs();
    TSM_ASSERT_EQUALS("All logs are defined now", errors.size(), 0);

    double chop_speed, chop_delay;
    m_getAllEi.find_chop_speed_and_delay(ws, chop_speed, chop_delay);
    TSM_ASSERT_DELTA("Chopper delay should have special speed ",
                     (10 * 0.1 + 20) / 12., chop_delay, 1.e-6);

    goodFram =
        std::make_unique<Kernel::TimeSeriesProperty<double>>("proton_charge");
    for (int i = 0; i < 10; i++) {
      auto time = Types::Core::DateAndTime(100 + 10 * i, 0);
      goodFram->addValue(time, 1);
    }

    ws->mutableRun().addProperty(goodFram.release(), true);
    errors = m_getAllEi.validateInputs();
    TSM_ASSERT_EQUALS("All logs are defined now", errors.size(), 0);

    m_getAllEi.find_chop_speed_and_delay(ws, chop_speed, chop_delay);
    TSM_ASSERT_DELTA("Chopper delay should have special speed", 0.1, chop_delay,
                     1.e-6);
  }
  void test_get_chopper_speed_filter_derivative() {

    MatrixWorkspace_sptr ws = createTestingWS(true);

    m_getAllEi.initialize();
    m_getAllEi.setProperty("Workspace", ws);
    m_getAllEi.setProperty("OutputWorkspace", "monitor_peaks");
    m_getAllEi.setProperty("Monitor1SpecID", 1);
    m_getAllEi.setProperty("Monitor2SpecID", 2);
    m_getAllEi.setProperty("ChopperSpeedLog", "Chopper_Speed");
    m_getAllEi.setProperty("ChopperDelayLog", "Chopper_Delay");
    m_getAllEi.setProperty("FilterBaseLog", "proton_charge");
    m_getAllEi.setProperty("FilterWithDerivative", true);

    // Test select log by log derivative
    auto chopDelay =
        std::make_unique<Kernel::TimeSeriesProperty<double>>("Chopper_Delay");
    auto chopSpeed =
        std::make_unique<Kernel::TimeSeriesProperty<double>>("Chopper_Speed");
    auto protCharge =
        std::make_unique<Kernel::TimeSeriesProperty<double>>("proton_charge");

    double gf(0);
    for (int i = 0; i < 50; i++) {
      auto time = Types::Core::DateAndTime(10 * i, 0);
      if (i > 10 && i < 20) {
        chopDelay->addValue(time, 100.);
        chopSpeed->addValue(time, 0.);
        protCharge->addValue(time, gf);
      } else {
        chopDelay->addValue(time, 10.);
        chopSpeed->addValue(time, 50.);
        protCharge->addValue(time, gf);
        gf++;
      }
    }
    ws->mutableRun().addLogData(chopSpeed.release());
    ws->mutableRun().addLogData(chopDelay.release());
    ws->mutableRun().addLogData(protCharge.release());

    // Run validate as this will set up property, which indicates filter log
    // presence
    auto errors = m_getAllEi.validateInputs();
    TSM_ASSERT_EQUALS("All logs are defined now", errors.size(), 0);

    double chop_speed, chop_delay;
    m_getAllEi.find_chop_speed_and_delay(ws, chop_speed, chop_delay);
    TSM_ASSERT_DELTA("Chopper delay should have defined value ", 10.,
                     chop_delay, 1.e-6);
    TSM_ASSERT_DELTA("Chopper speed should have defined speed", 50., chop_speed,
                     1.e-6);
  }

  void test_guess_opening_times() {

    std::pair<double, double> TOF_range(5, 100);
    double t0(6), Period(10);
    std::vector<double> guess_tof;
    m_getAllEi.findGuessOpeningTimes(TOF_range, t0, Period, guess_tof);
    TSM_ASSERT_EQUALS("should have 10 periods within the specified interval",
                      guess_tof.size(), 10);

    guess_tof.resize(0);
    t0 = TOF_range.first;
    m_getAllEi.findGuessOpeningTimes(TOF_range, t0, Period, guess_tof);
    TSM_ASSERT_EQUALS(
        "Still should be 10 periods within the specified interval",
        guess_tof.size(), 10);

    t0 = TOF_range.second;
    TSM_ASSERT_THROWS(
        "Should throw out of range",
        m_getAllEi.findGuessOpeningTimes(TOF_range, t0, Period, guess_tof),
        const std::runtime_error &);

    t0 = 1;
    guess_tof.resize(0);
    m_getAllEi.findGuessOpeningTimes(TOF_range, t0, Period, guess_tof);
    TSM_ASSERT_EQUALS(" should be 9 periods within the specified interval",
                      guess_tof.size(), 9);

    guess_tof.resize(0);
    t0 = 21;
    TOF_range.first = 20;
    m_getAllEi.findGuessOpeningTimes(TOF_range, t0, Period, guess_tof);
    TSM_ASSERT_EQUALS(" should be 8 periods within the specified interval",
                      guess_tof.size(), 8);
  }
  //
  void test_internalWS_to_fit() {
    Mantid::DataObjects::Workspace2D_sptr tws =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(5, 100,
                                                                     true);
    auto &spectrumInfoT = tws->spectrumInfo();
    auto det1TPosition = spectrumInfoT.position(0);
    auto det2TPosition = spectrumInfoT.position(4);
    auto detID1 = tws->getSpectrum(0).getDetectorIDs();
    auto detID2 = tws->getSpectrum(4).getDetectorIDs();

    m_getAllEi.initialize();
    m_getAllEi.setProperty("Workspace", tws);
    m_getAllEi.setProperty("OutputWorkspace", "monitor_peaks");
    m_getAllEi.setProperty("Monitor1SpecID", 1);
    m_getAllEi.setProperty("Monitor2SpecID", 5);

    size_t wsIndex0;
    auto wws = m_getAllEi.buildWorkspaceToFit(tws, wsIndex0);

    auto &spectrumInfoW = wws->spectrumInfo();
    auto det1WPosition = spectrumInfoW.position(0);
    auto det2WPosition = spectrumInfoW.position(1);
    TSM_ASSERT_EQUALS("should be the same first detector position",
                      det1WPosition, det1TPosition);
    TSM_ASSERT_EQUALS("should be the same second detector position",
                      det2WPosition, det2TPosition);

    TSM_ASSERT_EQUALS("Detector's ID for the first spectrum and new workspace "
                      "should coincide",
                      *(detID1.begin()),
                      (*wws->getSpectrum(0).getDetectorIDs().begin()));
    TSM_ASSERT_EQUALS("Detector's ID for the second spectrum and new workspace "
                      "should coincide",
                      *(detID2.begin()),
                      (*wws->getSpectrum(1).getDetectorIDs().begin()));
    auto Xsp1 = wws->getSpectrum(0).mutableX();
    auto Xsp2 = wws->getSpectrum(1).mutableX();
    size_t nSpectra = Xsp2.size();
    TS_ASSERT_EQUALS(nSpectra, 101);
    TS_ASSERT(std::isinf(Xsp1[nSpectra - 1]));
    TS_ASSERT(std::isinf(Xsp2[nSpectra - 1]));

    // for(size_t i=0;i<Xsp1.size();i++){
    //  TS_ASSERT_DELTA(Xsp1[i],Xsp2[i],1.e-6);
    //}
  }
  void test_calcDerivative() {
    double sig[] = {1, 2, 3, 4, 5, 6};
    std::vector<double> signal(sig, sig + sizeof(sig) / sizeof(double));
    double bin[] = {2, 3, 4, 5, 6, 7, 8};
    std::vector<double> bins(bin, bin + sizeof(bin) / sizeof(double));
    std::vector<double> zeros;

    std::vector<double> deriv;
    size_t nZer =
        m_getAllEi.calcDerivativeAndCountZeros(bins, signal, deriv, zeros);
    TS_ASSERT_EQUALS(nZer, 0);
    TS_ASSERT_DELTA(deriv[0], deriv[1], 1.e-9);
    TS_ASSERT_DELTA(deriv[0], deriv[5], 1.e-9);
    TS_ASSERT_DELTA(deriv[0], deriv[2], 1.e-9);
    TS_ASSERT_DELTA(deriv[0], 1., 1.e-9);

    double bin1[] = {0, 1, 3, 6, 10, 15, 21};
    std::vector<double> bins1(bin1, bin1 + sizeof(bin1) / sizeof(double));
    nZer = m_getAllEi.calcDerivativeAndCountZeros(bins1, signal, deriv, zeros);
    TS_ASSERT_EQUALS(nZer, 0);
    TS_ASSERT_DELTA(deriv[0], deriv[1], 1.e-9);
    TS_ASSERT_DELTA(deriv[0], deriv[5], 1.e-9);
    TS_ASSERT_DELTA(deriv[0], deriv[2], 1.e-9);
    TS_ASSERT_DELTA(deriv[0], 0, 1.e-9);

    bins.resize(101);
    signal.resize(100);
    for (size_t i = 0; i < 101; i++) {
      bins[i] = double(i) * 0.1;
    }
    for (size_t i = 0; i < 100; i++) {
      signal[i] = std::sin(0.5 * (bins[i] + bins[i + 1]));
    }
    nZer = m_getAllEi.calcDerivativeAndCountZeros(bins, signal, deriv, zeros);
    TS_ASSERT_EQUALS(nZer, 3);
    for (size_t i = 0; i < 99; i++) { // intentionally left boundary point --
                                      // its accuracy is much lower
      TSM_ASSERT_DELTA("At i=" + boost::lexical_cast<std::string>(i), deriv[i],
                       10. * std::cos(0.5 * (bins[i] + bins[i + 1])), 1.e-1);
    }
    TS_ASSERT_DELTA(zeros[0], 1.55, 1.e-3);
    TS_ASSERT_DELTA(zeros[1], 4.65, 1.e-3);
    TS_ASSERT_DELTA(zeros[2], 7.85, 1.e-3);
  }
  void test_binRanges() {
    std::vector<size_t> bin_min, bin_max, zeros;
    // Index           0 1 2 3 4 5 6 7 8 9  10 11 12 13
    double debin[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15};
    std::vector<double> ebin(debin, debin + sizeof(debin) / sizeof(double));
    // Not yet supported in VC 2012
    // std::vector<double> ebin={1,2,3,4,5,6,7,8,9,10,11,12,13,15};
    // Ind          0 1 2 3 4 5 6 7 8 9 10 11 12 13
    double sig[] = {0, 0, 0, 3, 0, 0, 4, 0, 0, 0, 11, 0, 0};
    std::vector<double> signal(sig, sig + sizeof(sig) / sizeof(double));

    double dguess[] = {1, 6, 10, 12};
    std::vector<double> guess(dguess, dguess + sizeof(dguess) / sizeof(double));
    std::vector<bool> guessValid;

    m_getAllEi.findBinRanges(ebin, signal, guess, 0.1, bin_min, bin_max,
                             guessValid);

    TS_ASSERT_EQUALS(bin_min.size(), 2)
    TS_ASSERT_EQUALS(bin_max.size(), 2)
    TS_ASSERT_EQUALS(guessValid.size(), 4)
    TS_ASSERT_EQUALS(bin_min[0], 4)
    TS_ASSERT_EQUALS(bin_max[0], 9)
    TS_ASSERT_EQUALS(bin_min[1], 7)
    TS_ASSERT_EQUALS(bin_max[1], 13)

    signal[10] = 0;
    signal[11] = 11;
    guess[1] = 3;
    guess[2] = 6;
    guess[3] = 11;
    m_getAllEi.findBinRanges(ebin, signal, guess, 0.01, bin_min, bin_max,
                             guessValid);
    TS_ASSERT_EQUALS(bin_min.size(), 3)
    TS_ASSERT_EQUALS(bin_max.size(), 3)
    TS_ASSERT_EQUALS(guessValid.size(), 4)

    TS_ASSERT_EQUALS(bin_min[0], 3);
    TS_ASSERT_EQUALS(bin_max[0], 4);
    TS_ASSERT(guessValid[1]);

    TS_ASSERT_EQUALS(bin_min[1], 6);
    TS_ASSERT_EQUALS(bin_max[1], 7);
    TS_ASSERT(guessValid[2]);

    TS_ASSERT_EQUALS(bin_min[2], 11);
    TS_ASSERT_EQUALS(bin_max[2], 12);
    TS_ASSERT(guessValid[3]);

    TS_ASSERT(!guessValid[0]);
  }

  void test_getAllEi() {
    auto ws = createTestingWS(false);
    API::MatrixWorkspace_sptr out_ws;

    TS_ASSERT_THROWS_NOTHING(m_getAllEi.initialize());
    m_getAllEi.setProperty("Workspace", ws);
    m_getAllEi.setProperty("OutputWorkspace", "monitor_peaks");
    m_getAllEi.setProperty("Monitor1SpecID", 1);
    m_getAllEi.setProperty("Monitor2SpecID", 2);
    m_getAllEi.setProperty("ChopperSpeedLog", "Chopper_Speed");
    m_getAllEi.setProperty("ChopperDelayLog", "Chopper_Delay");
    m_getAllEi.setProperty("FilterBaseLog", "is_running");
    m_getAllEi.setProperty("FilterWithDerivative", false);
    m_getAllEi.setProperty("OutputWorkspace", "allEiWs");

    TS_ASSERT_THROWS_NOTHING(m_getAllEi.execute());
    TSM_ASSERT_EQUALS("GetAllEi Algorithms should be executed",
                      m_getAllEi.isExecuted(), true);
    TS_ASSERT_THROWS_NOTHING(
        out_ws = API::AnalysisDataService::Instance()
                     .retrieveWS<API::MatrixWorkspace>("allEiWs"));

    TSM_ASSERT("Should be able to retrieve workspace", out_ws);
    auto wso = dynamic_cast<DataObjects::Workspace2D *>(out_ws.get());
    TS_ASSERT(wso);
    if (!wso)
      return;

    auto &x = wso->mutableX(0);
    TSM_ASSERT_EQUALS("Second peak should be filtered by monitor ranges",
                      x.size(), 1);
    TS_ASSERT_DELTA(x[0], 134.316, 1.e-3)
  }

private:
  GetAllEiTester m_getAllEi;
};

class GetAllEiTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetAllEiTestPerformance *createSuite() {
    return new GetAllEiTestPerformance();
  }
  static void destroySuite(GetAllEiTestPerformance *suite) { delete suite; }

  void setUp() override { inputMatrix = createTestingWS(false); }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().remove("monitor_peaks");
  }

  void testPerformance() {
    GetAllEi getAllEi;
    getAllEi.initialize();

    getAllEi.setProperty("Workspace", inputMatrix);
    getAllEi.setProperty("OutputWorkspace", "monitor_peaks");
    getAllEi.setProperty("Monitor1SpecID", 1);
    getAllEi.setProperty("Monitor2SpecID", 2);
    getAllEi.setPropertyValue("ChopperSpeedLog", "Chopper_Speed");
    getAllEi.setPropertyValue("ChopperDelayLog", "Chopper_Delay");
    getAllEi.setPropertyValue("FilterBaseLog", "is_running");
    getAllEi.setProperty("FilterWithDerivative", false);

    TS_ASSERT_THROWS_NOTHING(getAllEi.execute());
  }

private:
  Mantid::API::MatrixWorkspace_sptr inputMatrix;
};
#endif
