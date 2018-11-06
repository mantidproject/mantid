// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef HE3TUBEEFFICIENCYTEST_H_
#define HE3TUBEEFFICIENCYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/He3TubeEfficiency.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Kernel;
using namespace Mantid::Kernel::Exception;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace std;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Counts;

namespace He3TubeEffeciencyHelper {
void createWorkspace2DInADS(const std::string inputWS) {
  const int nspecs(4);
  const int nbins(5);

  auto space2D = createWorkspace<Workspace2D>(nspecs, nbins + 1, nbins);
  space2D->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");

  BinEdges x{0.1, 0.2, 0.3, 0.4, 0.5, 0.6};
  Counts y(nbins, 10.0);
  CountStandardDeviations e(nbins, sqrt(5.0));

  for (int i = 0; i < nspecs; i++) {
    space2D->setBinEdges(i, x);
    space2D->setCounts(i, y);
    space2D->setCountStandardDeviations(i, e);
    space2D->getSpectrum(i).setSpectrumNo(i);
  }

  AnalysisDataService::Instance().add(inputWS, space2D);

  LoadInstrument loader;
  loader.initialize();
  loader.setPropertyValue("Filename", "unit_testing/DUM_Definition.xml");
  loader.setPropertyValue("Workspace", inputWS);
  loader.setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
  loader.execute();
}

void createEventWorkspaceInADS(const std::string inputEvWS) {
  EventWorkspace_sptr event =
      WorkspaceCreationHelper::createEventWorkspace(4, 5, 5, 0, 0.9, 3, 0);
  event->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
  AnalysisDataService::Instance().add(inputEvWS, event);

  LoadInstrument loader;
  loader.initialize();
  loader.setPropertyValue("Filename", "unit_testing/DUM_Definition.xml");
  loader.setPropertyValue("Workspace", inputEvWS);
  loader.setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
  loader.execute();
}
} // namespace He3TubeEffeciencyHelper
class He3TubeEfficiencyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static He3TubeEfficiencyTest *createSuite() {
    return new He3TubeEfficiencyTest();
  }
  static void destroySuite(He3TubeEfficiencyTest *suite) { delete suite; }

  He3TubeEfficiencyTest() : inputWS("testInput"), inputEvWS("testEvInput") {}

  void testCorrection() {
    He3TubeEffeciencyHelper::createWorkspace2DInADS(inputWS);
    He3TubeEfficiency alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", inputWS);

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr result =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWS);

    // Monitor should be untouched
    TS_ASSERT_DELTA(result->y(0).front(), 10.0, 1e-6);
    // Check some detector values
    TS_ASSERT_DELTA(result->y(1).back(), 15.989063, 1e-6);
    TS_ASSERT_DELTA(result->y(2)[2], 21.520201, 1e-6);
    TS_ASSERT_DELTA(result->y(3).front(), 31.716197, 1e-6);

    AnalysisDataService::Instance().remove(inputWS);
  }

  void testEventCorrection() {
    He3TubeEffeciencyHelper::createEventWorkspaceInADS(inputEvWS);
    He3TubeEfficiency alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("InputWorkspace", inputEvWS);
    alg.setPropertyValue("OutputWorkspace", inputEvWS);

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr result =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputEvWS);
    EventWorkspace_sptr ev_result =
        boost::dynamic_pointer_cast<EventWorkspace>(result);

    // Monitor events should be untouched
    EventList mon_ev = ev_result->getSpectrum(0);
    TS_ASSERT_DELTA(mon_ev.getEvent(1).m_weight, 1.0, 1e-6);
    // Check some detector events
    EventList det1_ev = ev_result->getSpectrum(1);
    TS_ASSERT_DELTA(det1_ev.getEvent(1).m_weight, 1.098646, 1e-6);
    TS_ASSERT_DELTA(det1_ev.getEvent(1).m_errorSquared, 1.207024, 1e-6);
    EventList det3_ev = ev_result->getSpectrum(3);
    TS_ASSERT_DELTA(det3_ev.getEvent(4).m_weight, 1.000036, 1e-6);

    AnalysisDataService::Instance().remove(inputEvWS);
  }

  void testBadOverrideParameters() {
    He3TubeEffeciencyHelper::createWorkspace2DInADS(inputWS);
    He3TubeEfficiency alg;
    alg.initialize();

    TS_ASSERT_THROWS(alg.setPropertyValue("TubePressure", "-10"),
                     invalid_argument);
    TS_ASSERT_THROWS(alg.setPropertyValue("TubeThickness", "-0.08"),
                     invalid_argument);
    TS_ASSERT_THROWS(alg.setPropertyValue("TubeTemperature", "-100"),
                     invalid_argument);

    AnalysisDataService::Instance().remove(inputWS);
  }

  void testBadTubeThickness() {
    He3TubeEffeciencyHelper::createWorkspace2DInADS(inputWS);
    He3TubeEfficiency alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", inputWS);
    alg.setPropertyValue("TubeThickness", "0.0127");

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr result =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWS);

    // Monitor should be untouched
    TS_ASSERT_DELTA(result->y(0).front(), 10.0, 1e-6);
    // Check that detector values should be zero
    TS_ASSERT_DELTA(result->y(1).back(), 0.0, 1e-6);
    TS_ASSERT_DELTA(result->y(2)[2], 0.0, 1e-6);
    TS_ASSERT_DELTA(result->y(3).front(), 0.0, 1e-6);

    AnalysisDataService::Instance().remove(inputWS);
  }

  void testBadTubeThicknessEvents() {
    He3TubeEffeciencyHelper::createEventWorkspaceInADS(inputEvWS);
    He3TubeEfficiency alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("InputWorkspace", inputEvWS);
    alg.setPropertyValue("OutputWorkspace", inputEvWS);
    alg.setPropertyValue("TubeThickness", "0.0127");

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr result =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputEvWS);
    EventWorkspace_sptr ev_result =
        boost::dynamic_pointer_cast<EventWorkspace>(result);

    // Monitor should be untouched
    EventList mon_ev = ev_result->getSpectrum(0);
    TS_ASSERT_DELTA(mon_ev.getEvent(1).m_weight, 1.0, 1e-6);
    // Check that detectors have no events
    EventList det1_ev = ev_result->getSpectrum(1);
    TS_ASSERT_EQUALS(det1_ev.getNumberEvents(), 0);
    // Check that the total number of events is just the monitor
    TS_ASSERT_EQUALS(ev_result->getNumberEvents(), 5);

    AnalysisDataService::Instance().remove(inputWS);
  }

private:
  const std::string inputWS;
  const std::string inputEvWS;
};
#endif // HE3TUBEEFFICIENCYTEST_H_
