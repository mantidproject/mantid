// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAlgorithms/ApplyInstrumentToPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

using Mantid::Algorithms::ApplyInstrumentToPeaks;

class ApplyInstrumentToPeaksTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ApplyInstrumentToPeaksTest *createSuite() { return new ApplyInstrumentToPeaksTest(); }
  static void destroySuite(ApplyInstrumentToPeaksTest *suite) { delete suite; }

  void test_ApplyInstrumentToPeaks() {

    // create input workspace
    const auto inputWS = prepareWorkspace(1., 1000.);

    // now set a different instrument on the peaks workspace, double the distance of everything
    const auto new_inst = prepareInstrument(2.);
    inputWS->setInstrument(new_inst);

    std::string outWSName("ApplyInstrumenttoPeaksTestWS");

    ApplyInstrumentToPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    PeaksWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(outWSName);

    TS_ASSERT_EQUALS(outWS->getNumberPeaks(), 3);

    PeaksWorkspace_sptr expectedOutWS = prepareWorkspace(2., 1000.);

    for (int n = 0; n < outWS->getNumberPeaks(); n++) {
      const auto p0 = inputWS->getPeak(n);
      const auto p = outWS->getPeak(n);
      const auto p2 = expectedOutWS->getPeak(n);

      TS_ASSERT_EQUALS(p.getDetectorID(), p2.getDetectorID());
      TS_ASSERT_EQUALS(p.getDetectorID(), p0.getDetectorID());

      TS_ASSERT_DELTA(p.getTOF(), p2.getTOF(), 1e-12);
      TS_ASSERT_DELTA(p.getTOF(), p0.getTOF(), 1e-12);

      TS_ASSERT_DELTA(p.getWavelength(), p2.getWavelength(), 1e-12);
      TS_ASSERT_DELTA(p.getWavelength(), p0.getWavelength() / 2., 1e-12);

      const auto q = p.getQSampleFrame();

      const auto q2 = p2.getQSampleFrame();
      TS_ASSERT_DELTA(q.X(), q2.X(), 1e-12);
      TS_ASSERT_DELTA(q.Y(), q2.Y(), 1e-12);
      TS_ASSERT_DELTA(q.Z(), q2.Z(), 1e-12);

      const auto q0 = p0.getQSampleFrame();
      TS_ASSERT_DELTA(q.X(), q0.X() * 2, 1e-12);
      TS_ASSERT_DELTA(q.Y(), q0.Y() * 2, 1e-12);
      TS_ASSERT_DELTA(q.Z(), q0.Z() * 2, 1e-12);
    }
  }

  void test_ApplyInstrumentToPeaks_InstrumentWorkspace() {

    // create input workspace
    const auto inputWS = prepareWorkspace(1., 1000.);

    // now set a different instrument on the peaks workspace, double the distance of everything
    const auto new_inst = prepareInstrument(2.);
    inputWS->setInstrument(new_inst);

    PeaksWorkspace_sptr expectedOutWS = prepareWorkspace(2., 1000.);

    std::string outWSName("ApplyInstrumenttoPeaksTestWS");

    ApplyInstrumentToPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InstrumentWorkspace", expectedOutWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    PeaksWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(outWSName);

    TS_ASSERT_EQUALS(outWS->getNumberPeaks(), 3);

    for (int n = 0; n < outWS->getNumberPeaks(); n++) {
      const auto p0 = inputWS->getPeak(n);
      const auto p = outWS->getPeak(n);
      const auto p2 = expectedOutWS->getPeak(n);

      TS_ASSERT_EQUALS(p.getDetectorID(), p2.getDetectorID());
      TS_ASSERT_EQUALS(p.getDetectorID(), p0.getDetectorID());

      TS_ASSERT_DELTA(p.getTOF(), p2.getTOF(), 1e-12);
      TS_ASSERT_DELTA(p.getTOF(), p0.getTOF(), 1e-12);

      TS_ASSERT_DELTA(p.getWavelength(), p2.getWavelength(), 1e-12);
      TS_ASSERT_DELTA(p.getWavelength(), p0.getWavelength() / 2., 1e-12);

      const auto q = p.getQSampleFrame();

      const auto q2 = p2.getQSampleFrame();
      TS_ASSERT_DELTA(q.X(), q2.X(), 1e-12);
      TS_ASSERT_DELTA(q.Y(), q2.Y(), 1e-12);
      TS_ASSERT_DELTA(q.Z(), q2.Z(), 1e-12);

      const auto q0 = p0.getQSampleFrame();
      TS_ASSERT_DELTA(q.X(), q0.X() * 2, 1e-12);
      TS_ASSERT_DELTA(q.Y(), q0.Y() * 2, 1e-12);
      TS_ASSERT_DELTA(q.Z(), q0.Z() * 2, 1e-12);
    }
  }

private:
  // Create instrument
  Instrument_const_sptr prepareInstrument(const double &L) {
    // create starting instrument
    // moderator is set to z=-L
    std::vector<double> L2{L, L, L};
    std::vector<double> polar{M_PI_2, M_PI_2, M_PI_4};
    std::vector<double> azimuthal{0., M_PI_2, M_PI_4};
    const auto inst = ComponentCreationHelper::createCylInstrumentWithDetInGivenPositions(L2, polar, azimuthal);
    return inst;
  }

  // Create workspace
  PeaksWorkspace_sptr prepareWorkspace(const double &L, const double &tof) {

    PeaksWorkspace_sptr ws = std::dynamic_pointer_cast<PeaksWorkspace>(WorkspaceFactory::Instance().createPeaks());
    const auto inst = prepareInstrument(L);
    ws->setInstrument(inst);

    const double wl = PhysicalConstants::h / (PhysicalConstants::NeutronMass * (2 * L * 1e6 / tof)) * 1e10;
    ws->addPeak(Peak(inst, 1, wl));
    ws->addPeak(Peak(inst, 2, wl));
    ws->addPeak(Peak(inst, 3, wl));
    return ws;
  }
};
