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
#include "MantidAlgorithms/CreateGroupingWorkspace.h"
#include "MantidAlgorithms/DiffractionFocussing2.h"
#include "MantidAlgorithms/EstimateResolutionDiffraction.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
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
    MatrixWorkspace_sptr ws = createInstrument();

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

    TS_ASSERT_DELTA(outputws->y(numspec / 2)[0], 0.0057154, 1e-7); // copied value from test)
  }

  void test_SourceTerms() {
    // Create an empty PG3 workspace
    MatrixWorkspace_sptr ws = createInstrument();

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

    TS_ASSERT_DELTA(outputws->y(numspec / 2)[0], 0.0061078, 1e-7); // copied value from test)
  }

  /*
   * This test is meant to duplicate a workflow where data from SNAP_57514 is time focussed before calculating the
   * resolution of the resulting 6 spectra. The values checked against are from observations from doing individual peak
   * fits.
   */
  void test_focusedInstrument() {
    const std::string WS_IN("SNAP_Scratch");

    // Create empty instrument
    LoadEmptyInstrument loader;
    loader.initialize();
    // Use instrument valid starting 2018-05-01
    loader.setProperty("Filename", "SNAP_Definition.xml");
    loader.setProperty("OutputWorkspace", WS_IN);

    loader.execute();
    if (!loader.isExecuted())
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
          std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("SNAP_Scratch"));
      wsIn->mutableRun().addProperty(ang1);
      wsIn->mutableRun().addProperty(ang2);
      wsIn->mutableRun().addProperty(len1);
      wsIn->mutableRun().addProperty(len2);

      // set the units so DiffractionFocussing will do its job
      auto xAxis = wsIn->getAxis(0);
      xAxis->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
    }

    const std::string WS_GRP("SNAP_group");
    CreateGroupingWorkspace groupAlg;
    groupAlg.initialize();
    groupAlg.setProperty("InputWorkspace", WS_IN);
    groupAlg.setProperty("GroupDetectorsBy", "Column");
    groupAlg.setProperty("OutputWorkspace", WS_GRP);
    groupAlg.execute();
    if (!groupAlg.isExecuted())
      throw std::runtime_error("Failed to execute CreateGroupingWorkspace");

    DiffractionFocussing2 focusAlg;
    focusAlg.initialize();
    focusAlg.setProperty("InputWorkspace", WS_IN);
    focusAlg.setProperty("OutputWorkspace", WS_IN);
    focusAlg.setProperty("GroupingWorkspace", WS_GRP);
    focusAlg.execute();
    if (!focusAlg.isExecuted())
      throw std::runtime_error("Failed to execute DiffractionFocussing");

    // Set up and run the algorithm we're interested in
    EstimateResolutionDiffraction alg;
    alg.initialize();
    std::string WS_OUT("SNAP_Resolution");
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", WS_IN));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", WS_OUT));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PartialResolutionWorkspaces", "SNAP_partial"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DeltaTOFOverTOF", 0.001 / 15));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SourceDeltaL", 0.001));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SourceDeltaTheta", (0.01 / 15) * 4.8));

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // get the output
    MatrixWorkspace_sptr wsOut =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(WS_OUT));

    // observed values as observed by Guthrie fitting gaussians and dividing observed width by position
    TS_ASSERT_EQUALS(wsOut->getNumberHistograms(), 6);
    TS_ASSERT_DELTA(wsOut->readY(0)[0], 0.0026, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(1)[0], 0.0032, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(2)[0], 0.0039, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(3)[0], 0.0041, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(4)[0], 0.0054, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(5)[0], 0.0071, 0.0001);
  }

  /** Create an instrument
   */
  API::MatrixWorkspace_sptr createInstrument() {
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
};
