// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/CreateGroupingWorkspace.h"
#include "MantidAlgorithms/DiffractionFocussing2.h"
#include "MantidAlgorithms/EstimateResolutionDiffraction.h"
#include "MantidDataHandling/LoadDetectorsGroupingFile.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"

using Mantid::Algorithms::CreateGroupingWorkspace;
using Mantid::Algorithms::DiffractionFocussing2;
using Mantid::Algorithms::EstimateResolutionDiffraction;
using Mantid::DataHandling::LoadEmptyInstrument;

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using Mantid::Types::Core::DateAndTime;

namespace {
double resolution(const double deltaT_overT, const double deltaL, const double l_total, const double deltaTheta,
                  const double theta) {
  // std::cout << "Lt=" << l_total << " theta=" << theta * 180 / M_PI << "\n";
  double termTOF = deltaT_overT * deltaT_overT;
  double termL = (deltaL * deltaL) / (l_total * l_total);
  double termTheta = (deltaTheta * deltaTheta) / (tan(theta) * tan(theta));
  return sqrt(termTOF + termL + termTheta);
}
} // namespace

class EstimateResolutionDiffractionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EstimateResolutionDiffractionTest *createSuite() { return new EstimateResolutionDiffractionTest(); }
  static void destroySuite(EstimateResolutionDiffractionTest *suite) { delete suite; }

  /** Test init
   */
  void test_Init() {
    EstimateResolutionDiffraction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  /** Test POWGEN
   */
  void test_EmptyPG3() {
    // Create an empty PG3 workspace
    MatrixWorkspace_sptr ws = createPG3Instrument();

    // Set up and run
    EstimateResolutionDiffraction alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", ws->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "PG3_Resolution"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PartialResolutionWorkspaces", "PG3_partial"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DeltaTOF", 40.0));

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outputws =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("PG3_Resolution"));
    TS_ASSERT(outputws);
    if (!outputws)
      return;

    size_t numspec = outputws->getNumberHistograms();
    TS_ASSERT_EQUALS(numspec, 25873);

    for (size_t i = 0; i < numspec; ++i)
      TS_ASSERT(outputws->y(i)[0] < 0.03);

    TS_ASSERT_DELTA(outputws->y(numspec / 2)[0], 0.0057, 1e-4); // copied value from test)
  }

  void test_SourceTerms() {
    // Create an empty PG3 workspace
    MatrixWorkspace_sptr ws = createPG3Instrument();

    // Set up and run
    EstimateResolutionDiffraction alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", ws->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "PG3_Resolution"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PartialResolutionWorkspaces", "PG3_partial"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DeltaTOF", 40.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SourceDeltaTheta", .002));

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outputws =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("PG3_Resolution"));
    TS_ASSERT(outputws);
    if (!outputws)
      return;

    size_t numspec = outputws->getNumberHistograms();
    TS_ASSERT_EQUALS(numspec, 25873);

    for (size_t i = 0; i < numspec; ++i)
      TS_ASSERT(outputws->y(i)[0] < 0.04);

    TS_ASSERT_DELTA(outputws->y(numspec / 2)[0], 0.0061, 1e-4); // copied value from test)
  }

  /*
   * The tests using this are meant to duplicate a workflow where data from SNAP_57514 is time focussed before
   * calculating the resolution of the resulting focussed spectra. The values checked against are from observations from
   * doing individual peak fits.
   */
  void run_focusSNAPtest(const std::string &groupBy, const std::vector<double> &tolerances) {
    auto ws = createSNAPLiteInstrument(groupBy);
    const auto WS_IN = ws->getName();
    std::string WS_OUT = "SNAP" + groupBy + "Bank_Resolution";
    // source resolution dominates the calculation for SNAP
    constexpr double SNAP_deltaTOFOverTOF{0.002};
    constexpr double SNAP_deltaL{0.001};
    constexpr double SNAP_deltaTheta{(0.01 / 15) * 4.8};

    // Set up and run the algorithm we're interested in
    EstimateResolutionDiffraction alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", WS_IN));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", WS_OUT));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PartialResolutionWorkspaces", "SNAP_partial"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DeltaTOFOverTOF", SNAP_deltaTOFOverTOF));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SourceDeltaL", SNAP_deltaL));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SourceDeltaTheta", SNAP_deltaTheta));

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // get the output
    MatrixWorkspace_sptr wsOut =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(WS_OUT));
    const size_t numHist = wsOut->getNumberHistograms();
    TS_ASSERT_EQUALS(numHist, tolerances.size());
    const auto &spectrumInfo = wsOut->spectrumInfo();
    const auto l1 = spectrumInfo.l1(); // 15m
    // assume all spectra are approximately at the focus position
    for (size_t i = 0; i < numHist; ++i) {
      const double twoTheta = spectrumInfo.twoTheta(i);
      const double l2 = spectrumInfo.l2(i);
      const double res = resolution(SNAP_deltaTOFOverTOF, SNAP_deltaL, l1 + l2, SNAP_deltaTheta, 0.5 * twoTheta);

      // compare values with relative tolerance
      const double abs_tol = abs(wsOut->readY(i)[0] - res);
      const double rel_tol = 100. * abs_tol / res;
      TS_ASSERT_LESS_THAN(rel_tol, tolerances[i]);
    }

    // delete the workspaces
    AnalysisDataService::Instance().remove(WS_IN);
    AnalysisDataService::Instance().remove(WS_OUT);
  }

  void test_focusSNAPByColumn() {
    // these are the relative toleraces that work
    const std::vector<double> tolerances{3., 2., 4., 4., 2., 6.};
    run_focusSNAPtest("Column", tolerances);
  }

  void test_focusSNAPByPanel() {
    // these are the relative toleraces that work
    const std::vector<double> tolerances{3., 3., 3., 2., 2., 2., 4., 4., 4., 5., 6., 5., 2., 3., 2., 4., 4., 4.};
    run_focusSNAPtest("bank", tolerances);
  }

  /** Create an instrument
   */
  API::MatrixWorkspace_sptr createPG3Instrument() {
    // Create empty workspace
    LoadEmptyInstrument loader;
    loader.initialize();

    loader.setProperty("Filename", "POWGEN_Definition_2013-06-01.xml");
    loader.setProperty("OutputWorkspace", "PG3_Scratch");

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    // Time series property
    TimeSeriesProperty<double> *lambda = new TimeSeriesProperty<double>("LambdaRequest");
    lambda->setUnits("Angstrom");
    DateAndTime time0(0);
    lambda->addValue(time0, 1.066);

    // Add log to workspace
    MatrixWorkspace_sptr ws =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("PG3_Scratch"));
    ws->mutableRun().addProperty(lambda);

    return ws;
  }

  API::MatrixWorkspace_sptr createSNAPLiteInstrument(const std::string &groupDetectorsBy) {
    const std::string WS_IN("SNAP_Scratch");
    // Use lite instrument based on one valid starting 2018-05-01
    const std::string IDF_FILE("SNAPLite_Definition.xml");
    //    const std::string IDF_FILE("SNAP_Definition.xml");

    // Create empty instrument
    LoadEmptyInstrument loadEmptyInstr;
    loadEmptyInstr.initialize();
    loadEmptyInstr.setProperty("Filename", IDF_FILE);
    loadEmptyInstr.setProperty("OutputWorkspace", WS_IN);

    loadEmptyInstr.execute();
    if (!loadEmptyInstr.isExecuted())
      throw std::runtime_error("Failed to execute LoadEmptyInstrument");

    // add logs - values taken from SNAP_57514
    // for some reason, the units aren't in the logs
    { // reduce variable scope
      TimeSeriesProperty<double> *ang1 = new TimeSeriesProperty<double>("det_arc1");
      ang1->addValue(DateAndTime(0), -65.3);
      TimeSeriesProperty<double> *ang2 = new TimeSeriesProperty<double>("det_arc2");
      ang2->addValue(DateAndTime(0), 104.95);
      TimeSeriesProperty<double> *len1 = new TimeSeriesProperty<double>("det_lin1");
      len1->addValue(DateAndTime(0), 0.045);
      TimeSeriesProperty<double> *len2 = new TimeSeriesProperty<double>("det_lin2");
      len2->addValue(DateAndTime(0), 0.043);

      MatrixWorkspace_sptr wsIn =
          std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(WS_IN));
      wsIn->mutableRun().addProperty(ang1);
      wsIn->mutableRun().addProperty(ang2);
      wsIn->mutableRun().addProperty(len1);
      wsIn->mutableRun().addProperty(len2);

      // reload instrument so the logs are used
      LoadInstrument loadInstr;
      loadInstr.initialize();
      loadInstr.setProperty("Workspace", WS_IN);
      loadInstr.setProperty("Filename", IDF_FILE);
      loadInstr.setProperty("MonitorList", "-2--1");
      loadInstr.setProperty("RewriteSpectraMap", "False");
      loadInstr.execute();
      if (!loadInstr.isExecuted())
        throw std::runtime_error("Failed to execute LoadInstrument");

      // set the units so DiffractionFocussing will do its job
      auto xAxis = wsIn->getAxis(0);
      xAxis->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
    }

    const std::string WS_GRP = "SNAP_group" + groupDetectorsBy;
    if (groupDetectorsBy == "bank" || groupDetectorsBy == "Column") {
      CreateGroupingWorkspace groupAlg;
      groupAlg.initialize();
      groupAlg.setProperty("InputWorkspace", WS_IN);
      groupAlg.setProperty("GroupDetectorsBy", groupDetectorsBy);
      groupAlg.setProperty("OutputWorkspace", WS_GRP);
      groupAlg.execute();
      if (!groupAlg.isExecuted())
        throw std::runtime_error("Failed to execute CreateGroupingWorkspace");
    } else {
      // assume groupDetectorsBy is a grouping filename
      LoadDetectorsGroupingFile groupAlg;
      groupAlg.initialize();
      groupAlg.setProperty("InputWorkspace", WS_IN);
      groupAlg.setProperty("InputFile", groupDetectorsBy);
      groupAlg.setProperty("OutputWorkspace", WS_GRP);
      groupAlg.execute();
      if (!groupAlg.isExecuted())
        throw std::runtime_error("Failed to execute CreateGroupingWorkspace");
    }

    DiffractionFocussing2 focusAlg;
    focusAlg.initialize();
    focusAlg.setProperty("InputWorkspace", WS_IN);
    focusAlg.setProperty("OutputWorkspace", WS_IN);
    focusAlg.setProperty("GroupingWorkspace", WS_GRP);
    focusAlg.execute();
    if (!focusAlg.isExecuted())
      throw std::runtime_error("Failed to execute DiffractionFocussing");

    MatrixWorkspace_sptr ws =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(WS_IN));
    return ws;
  }
};
