#ifndef MANTID_ALGORITHMS_CALC_COUNTRATE_TEST_H_
#define MANTID_ALGORITHMS_CALC_COUNTRATE_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidAlgorithms/CalculateCountRate.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidHistogramData/HistogramX.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <numeric>

using namespace Mantid;
using Mantid::DataObjects::Workspace2D_sptr;
using namespace Mantid::API;
using namespace Mantid::Algorithms;

class CalculateCountRateTester : public CalculateCountRate {
public:
  CalculateCountRateTester() { this->setChild(true); }
  void setSearchRanges(DataObjects::EventWorkspace_sptr &InputWorkspace) {
    CalculateCountRate::setSourceWSandXRanges(InputWorkspace);
  }
  std::tuple<double, double, bool> getXRanges() const {
    return std::tuple<double, double, bool>(m_XRangeMin, m_XRangeMax,
                                            m_rangeExplicit);
  }
  void
  setOutLogParameters(const DataObjects::EventWorkspace_sptr &InputWorkspace) {
    CalculateCountRate::setOutLogParameters(InputWorkspace);
  }
  void getAlgLogSettings(size_t &numLogSteps,
                         Kernel::TimeSeriesProperty<double> const *&pNormLog) {
    pNormLog = m_pNormalizationLog;
    numLogSteps = m_numLogSteps;
  }

  DataObjects::EventWorkspace *getWorkingWS() { return m_workingWS.get(); }
  void setVisWS(const std::string &wsName) {
    this->setProperty("VisualizationWs", wsName);
    this->checkAndInitVisWorkspace();
  }
  const std::vector<double> &getVisNormLog() { return m_visNorm; }
};

class CalculateCountRateTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateCountRateTest *createSuite() {
    return new CalculateCountRateTest();
  }
  static void destroySuite(CalculateCountRateTest *suite) { delete suite; }

  void test_Init() {
    CalculateCountRate alg;

    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }
  void test_ranges() {

    auto sws = build_test_ws(false);

    double XRangeMin, XRangeMax;

    CalculateCountRateTester alg;

    alg.initialize();
    alg.setProperty("Workspace", sws);
    alg.setProperty("RangeUnits", "dSpacing");

    alg.setSearchRanges(sws); // explicit ranges:
    //-- real workspace ranges returned

    auto ranges = alg.getXRanges();
    TS_ASSERT_DELTA(std::get<0>(ranges), 0.5, 1.e-8);
    TS_ASSERT_DELTA(std::get<1>(ranges), 99.5, 1.e-8);
    TS_ASSERT(!std::get<2>(ranges));

    alg.getWorkingWS()->getEventXMinMax(XRangeMin, XRangeMax);
    TS_ASSERT_EQUALS(XRangeMin, std::get<0>(ranges));
    TS_ASSERT_DELTA(XRangeMax, std::get<1>(ranges), 1.e-8);

    //--------------------------------------------------------------------
    // right crop range is specified. Top range is within the right
    // limit
    alg.setProperty("Workspace", sws);
    alg.setProperty("XMax", 20.);
    alg.setProperty("RangeUnits", "dSpacing");

    alg.setSearchRanges(sws);

    ranges = alg.getXRanges();
    TS_ASSERT_DELTA(std::get<0>(ranges), 0.5, 1.e-8); // left range is real
    // range as not specified
    TS_ASSERT_DELTA(std::get<1>(ranges), 20.,
                    1.e-8); // Right range is specified
    TS_ASSERT(std::get<2>(ranges));

    sws->getEventXMinMax(XRangeMin, XRangeMax);
    TS_ASSERT_DELTA(XRangeMin, 0.5, 1.e-5);
    TS_ASSERT_DELTA(XRangeMax, 99.5, 1.e-5);

    //--------------------------------------------------------------------
    // both crop ranges are specified. Result lies within the crop
    // ranges in energy units.

    sws = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(
        2, 10, false);

    alg.setProperty("XMin", 1.);
    alg.setProperty("XMax", 30.);
    alg.setProperty("RangeUnits", "Energy");

    alg.setSearchRanges(sws);

    ranges = alg.getXRanges();
    TS_ASSERT_DELTA(std::get<0>(ranges), 19.9301, 1.e-4);
    TS_ASSERT_DELTA(std::get<1>(ranges), 30., 1.e-8);
    TS_ASSERT(std::get<2>(ranges));

    // units have been converted
    alg.getWorkingWS()->getEventXMinMax(XRangeMin, XRangeMax);

    TS_ASSERT_DELTA(std::get<0>(ranges), XRangeMin, 1.e-4);
    TS_ASSERT(std::isinf(XRangeMax));
  }

  void test_log_params() {

    auto sws = build_test_ws(false);

    size_t numLogSteps;
    Kernel::TimeSeriesProperty<double> const *pNormLog;

    CalculateCountRateTester alg;
    alg.initialize();

    //-------- check defaults
    alg.setOutLogParameters(sws);

    alg.getAlgLogSettings(numLogSteps, pNormLog);
    TS_ASSERT_EQUALS(numLogSteps, 200);
    TS_ASSERT(!pNormLog);

    //-------- check numLogSteps
    alg.setProperty("NumTimeSteps", 100);

    alg.setOutLogParameters(sws);

    alg.getAlgLogSettings(numLogSteps, pNormLog);
    TS_ASSERT_EQUALS(numLogSteps, 100);
    TS_ASSERT(!pNormLog);

    //-------- check numLogSteps, normalization log ignored
    alg.setProperty("NumTimeSteps", 120);
    alg.setProperty("NormalizeTheRate", true);
    alg.setProperty("UseLogDerivative", false);
    alg.setProperty("UseNormLogGranularity", true);

    alg.setOutLogParameters(sws);
    alg.getAlgLogSettings(numLogSteps, pNormLog);
    TS_ASSERT_EQUALS(numLogSteps, 120);
    TS_ASSERT(!pNormLog);
    TS_ASSERT(!alg.normalizeCountRate());

    // Check time series log outside of the data range
    auto pTime_log = new Kernel::TimeSeriesProperty<double>("proton_charge");
    Kernel::DateAndTime first("2015-11-30T16:17:10");
    std::vector<Kernel::DateAndTime> times(140);
    std::vector<double> values(140);

    for (size_t i = 0; i < 140; ++i) {
      times[i] = first + double(i);
      values[i] = double(i);
    }

    // DateAndTime("2010-01-01T00:00:00")

    pTime_log->addValues(times, values);
    sws->mutableRun().addProperty(pTime_log, true);

    alg.setOutLogParameters(sws);
    alg.getAlgLogSettings(numLogSteps, pNormLog);
    TS_ASSERT_EQUALS(numLogSteps, 120);
    TS_ASSERT(!pNormLog);
    TS_ASSERT(!alg.normalizeCountRate());
    TS_ASSERT(!alg.useLogDerivative());

    // Check correct date and time
    first = Kernel::DateAndTime("2010-01-01T00:00:00");
    times.resize(240);
    values.resize(240);
    for (size_t i = 0; i < 240; ++i) {
      times[i] = first - 20. + double(i);
      values[i] = double(i);
    }
    pTime_log->replaceValues(times, values);

    alg.setOutLogParameters(sws);
    alg.getAlgLogSettings(numLogSteps, pNormLog);
    TS_ASSERT_EQUALS(numLogSteps, 99);
    TS_ASSERT(pNormLog);
    TS_ASSERT(alg.normalizeCountRate());
    TS_ASSERT(!alg.useLogDerivative());

    // check useLogDerivative
    alg.setProperty("UseLogDerivative", true);
    //
    alg.setOutLogParameters(sws);
    alg.getAlgLogSettings(numLogSteps, pNormLog);
    TS_ASSERT_EQUALS(numLogSteps, 100);
    TS_ASSERT(pNormLog);
    TS_ASSERT(alg.normalizeCountRate());
    TS_ASSERT(alg.useLogDerivative());
  }

  void test_processing() {

    auto sws = build_test_ws(true);

    CalculateCountRateTester alg;
    alg.initialize();

    alg.setProperty("NumTimeSteps", 120);
    alg.setProperty("NormalizeTheRate", true);
    alg.setProperty("UseLogDerivative", true);
    alg.setProperty("UseNormLogGranularity", true);

    alg.setProperty("Workspace", sws);

    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(sws->run().hasProperty("block_count_rate"));

    auto newLog = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
        sws->run().getLogData("block_count_rate"));

    TS_ASSERT(newLog);
    if (!newLog)
      return;

    TS_ASSERT_EQUALS(newLog->realSize(), 100);
    TS_ASSERT_EQUALS(newLog->size(), 100);
    auto val_vec = newLog->valuesAsVector();

    for (size_t i = 0; i < val_vec.size(); i++) {
      TS_ASSERT_DELTA(val_vec[i], 198., 1.e-4);
    }
  }

  void test_visWS_creation() {

    auto sws = build_test_ws(false);

    CalculateCountRateTester alg;
    alg.initialize();

    alg.setProperty("NumTimeSteps", 120);
    alg.setProperty("XResolution", 200);
    alg.setProperty("XMin", 10.);
    alg.setProperty("XMax", 50.);

    alg.setProperty("Workspace", sws);
    alg.setSearchRanges(sws);

    TS_ASSERT_THROWS_NOTHING(alg.setVisWS("testVisWSName"));

    API::MatrixWorkspace_sptr testVisWS = alg.getProperty("VisualizationWs");
    TS_ASSERT(testVisWS);
    if (!testVisWS)
      return;

    TS_ASSERT_EQUALS(testVisWS->getNumberHistograms(), 120);
    auto X = testVisWS->readX(0);
    auto Y = testVisWS->readY(0);
    TS_ASSERT_EQUALS(X.size(), 201);
    TS_ASSERT_EQUALS(Y.size(), 200);
  }

  void test_visWS_noNormalization() {

    DataObjects::EventWorkspace_sptr sws =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 10,
                                                                        false);

    CalculateCountRateTester alg;
    alg.initialize();

    alg.setProperty("NumTimeSteps", 100);
    alg.setProperty("XResolution", 200);

    alg.setProperty("RangeUnits", "dSpacing");

    alg.setProperty("NormalizeTheRate", false);
    alg.setProperty("UseLogDerivative", true);
    alg.setProperty("UseNormLogGranularity", true);

    alg.setProperty("Workspace", sws);
    alg.setProperty("VisualizationWs", "testVisWSNoNorm");

    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    API::MatrixWorkspace_sptr testVisWS = alg.getProperty("VisualizationWs");
    TS_ASSERT(testVisWS);
    TS_ASSERT_EQUALS(testVisWS->getNumberHistograms(), 100);
    const MantidVec &X = testVisWS->readX(0);
    const MantidVec &Y = testVisWS->readY(0);
    TS_ASSERT_EQUALS(X.size(), 201);
    TS_ASSERT_EQUALS(Y.size(), 200);
    auto Yax = dynamic_cast<API::NumericAxis *>(testVisWS->getAxis(1));
    TS_ASSERT(Yax);
    if (!Yax)
      return;

    auto newLog = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
        sws->run().getLogData("block_count_rate"));
    TS_ASSERT(newLog);
    if (!newLog)
      return;

    MantidVec counts = newLog->valuesAsVector();
    TS_ASSERT_EQUALS(counts.size(), testVisWS->getNumberHistograms());
    if (counts.size() != testVisWS->getNumberHistograms())
      return;

    for (size_t i = 0; i < testVisWS->getNumberHistograms(); ++i) {
      const MantidVec &Y = testVisWS->readY(i);
      double sum = std::accumulate(Y.begin(), Y.end(), 0.);
      TSM_ASSERT_DELTA("Incorrect counts at index: " +
                           boost::lexical_cast<std::string>(i),
                       counts[i], sum, 1.e-6);
    }
  }

  void test_visWS_NormalizationFine() {

    auto sws = build_test_ws(true);

    CalculateCountRateTester alg;
    alg.initialize();

    alg.setProperty("NumTimeSteps", 300);
    alg.setProperty("XResolution", 200);

    alg.setProperty("RangeUnits", "dSpacing");

    alg.setProperty("NormalizeTheRate", true);
    alg.setProperty("UseLogDerivative", true);
    alg.setProperty("UseNormLogGranularity", true);

    alg.setProperty("Workspace", sws);

    alg.setOutLogParameters(sws);
    alg.setSearchRanges(sws);
    TS_ASSERT_THROWS_NOTHING(alg.setVisWS("testVisWSNorm"));

    auto visNormLog = alg.getVisNormLog();
    TS_ASSERT_EQUALS(visNormLog.size(), 100);
    TS_ASSERT_DELTA(visNormLog[10], 2, 1.e-5);
    auto sum = std::accumulate(visNormLog.begin(), visNormLog.end(), 0.);

    TS_ASSERT_DELTA(sum, 200., 1.e-4);
  }

  void test_visWS_NormalizationCoarce() {

    auto sws = build_test_ws(true);

    CalculateCountRateTester alg;
    alg.initialize();

    alg.setProperty("NumTimeSteps", 50);
    alg.setProperty("XResolution", 200);

    alg.setProperty("RangeUnits", "dSpacing");

    alg.setProperty("NormalizeTheRate", true);
    alg.setProperty("UseLogDerivative", true);
    alg.setProperty("UseNormLogGranularity", true);

    alg.setProperty("Workspace", sws);

    alg.setOutLogParameters(sws);
    alg.setSearchRanges(sws);
    TS_ASSERT_THROWS_NOTHING(alg.setVisWS("testVisWSNorm"));

    auto visNormLog = alg.getVisNormLog();
    TS_ASSERT_EQUALS(visNormLog.size(), 50);
    TS_ASSERT_DELTA(visNormLog[10], 4, 1.e-5);
    auto sum = std::accumulate(visNormLog.begin(), visNormLog.end(), 0.);

    TS_ASSERT_DELTA(sum, 200., 1.e-4);
  }

  void test_visWS_Normalized() {

    auto sws = build_test_ws(true);

    CalculateCountRateTester alg;
    alg.initialize();

    alg.setProperty("NumTimeSteps", 50);
    alg.setProperty("XResolution", 200);
    alg.setProperty("VisualizationWs", "testVisWSNormalized");

    alg.setProperty("RangeUnits", "dSpacing");

    alg.setProperty("NormalizeTheRate", true);
    alg.setProperty("UseLogDerivative", true);
    alg.setProperty("UseNormLogGranularity", true);

    alg.setProperty("Workspace", sws);

    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    API::MatrixWorkspace_sptr testVisWS = alg.getProperty("VisualizationWs");
    TS_ASSERT(testVisWS);
    if (!testVisWS)
      return;
    TS_ASSERT_EQUALS(testVisWS->getNumberHistograms(), 50);
    const HistogramData::HistogramX &X = testVisWS->x(0);
    const HistogramData::HistogramY &Y = testVisWS->y(0);
    TS_ASSERT_EQUALS(X.size(), 201);
    TS_ASSERT_EQUALS(Y.size(), 200);
    auto Yax = dynamic_cast<API::NumericAxis *>(testVisWS->getAxis(1));
    TS_ASSERT(Yax);
    if (!Yax)
      return;

    auto newLog = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
        sws->run().getLogData("block_count_rate"));
    TS_ASSERT(newLog);
    if (!newLog)
      return;

    MantidVec counts = newLog->valuesAsVector();

    // verify everywhere except boundaries, where round-off errors and
    // different time steps make results unstable
    for (size_t i = 1; i < testVisWS->getNumberHistograms() - 1; ++i) {
      const HistogramData::HistogramY &Y = testVisWS->y(i);
      // const MantidVec &Y = testVisWS->readY(i); // -- better for debugging as
      // one can see what is inside
      double sum = std::accumulate(Y.begin(), Y.end(), 0.);
      TSM_ASSERT_DELTA("Incorrect counts at index: " + std::to_string(i),
                       counts[i], sum, 1.e-6);
    }
  }
  //----------------------------------------------------------------------
  DataObjects::EventWorkspace_sptr build_test_ws(bool addLog = false) {

    DataObjects::EventWorkspace_sptr sws =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 10,
                                                                        false);

    if (!addLog) {
      return sws;
    }

    auto pTime_log = new Kernel::TimeSeriesProperty<double>("proton_charge");
    Kernel::DateAndTime first("2010-01-01T00:00:00");
    std::vector<Kernel::DateAndTime> times(240);
    std::vector<double> values(240);

    for (size_t i = 0; i < values.size(); ++i) {
      times[i] = first - 10. + double(i);
      values[i] = 2 * double(i);
    }
    pTime_log->addValues(times, values);
    sws->mutableRun().addProperty(pTime_log, true);

    return sws;
  }
};

#endif /* MANTID_ALGORITHMS_CALC_COUNTRATE_TEST_H_ */
