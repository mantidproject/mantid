// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Axis.h"
#include "MantidCrystal/NormaliseVanadium.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/FacilityHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Timer.h"
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <random>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::LinearGenerator;
using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;

namespace {
/** Create an EventWorkspace containing fake data of single-crystal diffraction.
 *
 * @return EventWorkspace_sptr
 */
EventWorkspace_sptr createDiffractionEventWorkspace(int numEvents) {
  FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilities.xml", "TEST");

  int numPixels = 10000;
  int numBins = 16;
  double binDelta = 0.10;

  EventWorkspace_sptr retVal(new EventWorkspace);
  retVal->initialize(numPixels, 1, 1);

  // --------- Load the instrument -----------
  LoadInstrument *loadInst = new LoadInstrument();
  loadInst->initialize();
  loadInst->setPropertyValue("Filename", "unit_testing/MINITOPAZ_Definition.xml");
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", retVal);
  loadInst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
  loadInst->execute();
  delete loadInst;
  // Populate the instrument parameters in this workspace - this works around
  // a bug
  retVal->populateInstrumentParameters();

  DateAndTime run_start("2010-01-01T00:00:00");
  std::mt19937 rng(1);
  std::uniform_real_distribution<double> flat(-1.0, 1.0);
  for (int pix = 0; pix < numPixels; pix++) {
    EventList &el = retVal->getSpectrum(pix);
    el.setSpectrumNo(pix);
    el.addDetectorID(pix);
    // Background
    for (int i = 0; i < numBins; i++) {
      // Two events per bin
      el += TofEvent((i + 0.5) * binDelta, run_start + double(i));
      el += TofEvent((i + 0.5) * binDelta, run_start + double(i));
    }

    // Peak
    int r = static_cast<int>(
        numEvents / std::sqrt((pix / 100 - 50.5) * (pix / 100 - 50.5) + (pix % 100 - 50.5) * (pix % 100 - 50.5)));
    for (int i = 0; i < r; i++) {
      el += TofEvent(0.75 + binDelta * ((flat(rng) + flat(rng) + flat(rng)) * 2. - 3.), run_start + double(i));
    }
  }

  // Set all the histograms at once.
  retVal->setAllX(BinEdges(numBins, LinearGenerator(0.0, binDelta)));

  // Some sanity checks
  TS_ASSERT_EQUALS(retVal->getInstrument()->getName(), "MINITOPAZ");
  std::map<int, Geometry::IDetector_const_sptr> dets;
  retVal->getInstrument()->getDetectors(dets);
  TS_ASSERT_EQUALS(dets.size(), 100 * 100 + 2);

  return retVal;
}

/** Creates an instance of the NormaliseVanadium algorithm and sets its
 * properties.
 *
 * @return NormaliseVanadium alg
 */
IAlgorithm_sptr createAlgorithm() {
  int numEventsPer = 100;
  MatrixWorkspace_sptr inputW = createDiffractionEventWorkspace(numEventsPer);
  inputW->getAxis(0)->setUnit("Wavelength");

  auto alg = std::make_shared<NormaliseVanadium>();
  TS_ASSERT_THROWS_NOTHING(alg->initialize())
  TS_ASSERT(alg->isInitialized())
  alg->setProperty("InputWorkspace", inputW);
  alg->setProperty("OutputWorkspace", "TOPAZ");
  alg->setProperty("Wavelength", 1.0);

  return alg;
}
} // namespace

class NormaliseVanadiumTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    NormaliseVanadium alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_MINITOPAZ() {

    IAlgorithm_sptr alg = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->execute();)
    TS_ASSERT(alg->isExecuted())

    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("TOPAZ"));
    TS_ASSERT(ws);
    if (!ws)
      return;
    TS_ASSERT_DELTA(ws->y(5050)[5], 17, 0.0001);
    AnalysisDataService::Instance().remove("TOPAZ");
  }
};

class NormaliseVanadiumTestPerformance : public CxxTest::TestSuite {
public:
  static NormaliseVanadiumTestPerformance *createSuite() { return new NormaliseVanadiumTestPerformance(); }
  static void destroySuite(NormaliseVanadiumTestPerformance *suite) { delete suite; }

  void setUp() override { normaliseVanadiumAlg = createAlgorithm(); }

  void tearDown() override { AnalysisDataService::Instance().remove("TOPAZ"); }

  void testNormaliseVanadiumPerformance() { TS_ASSERT_THROWS_NOTHING(normaliseVanadiumAlg->execute()); }

private:
  IAlgorithm_sptr normaliseVanadiumAlg;
};
