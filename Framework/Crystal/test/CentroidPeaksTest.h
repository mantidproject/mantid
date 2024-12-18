// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Axis.h"
#include "MantidCrystal/CentroidPeaks.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
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
using namespace Mantid::HistogramData;
using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;

class CentroidPeaksTest : public CxxTest::TestSuite {
public:
  /** Create an EventWorkspace containing fake data
   * of single-crystal diffraction.
   *
   * @return EventWorkspace_sptr
   */
  EventWorkspace_sptr createDiffractionEventWorkspace(int numEvents) {
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilities.xml", "TEST");

    int numPixels = 10000;
    int numBins = 16;
    double binDelta = 10.0;
    std::mt19937 rng(1);
    std::uniform_real_distribution<double> flat(-1.0, 1.0);

    std::shared_ptr<EventWorkspace> retVal =
        create<EventWorkspace>(numPixels, BinEdges(numBins, LinearGenerator(0.0, binDelta)));

    // --------- Load the instrument -----------
    LoadInstrument *loadInst = new LoadInstrument();
    loadInst->initialize();
    loadInst->setPropertyValue("Filename", "unit_testing/MINITOPAZ_Definition.xml");
    loadInst->setProperty("Workspace", retVal);
    loadInst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
    loadInst->execute();
    delete loadInst;
    // Populate the instrument parameters in this workspace - this works around
    // a bug
    retVal->populateInstrumentParameters();

    DateAndTime run_start("2010-01-01T00:00:00");

    for (int pix = 0; pix < numPixels; ++pix) {
      auto &el = retVal->getSpectrum(pix);
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
        el += TofEvent(5844. + 10. * ((flat(rng) + flat(rng) + flat(rng)) * 2. - 3.), run_start + double(i));
      }
    }

    // Some sanity checks
    TS_ASSERT_EQUALS(retVal->getInstrument()->getName(), "MINITOPAZ");
    std::map<int, Geometry::IDetector_const_sptr> dets;
    retVal->getInstrument()->getDetectors(dets);
    TS_ASSERT_EQUALS(dets.size(), 100 * 100 + 2);

    return retVal;
  }

  void test_Init() {
    CentroidPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void do_test_MINITOPAZ(bool ev) {
    int numEventsPer = 100;
    MatrixWorkspace_sptr inputW = createDiffractionEventWorkspace(numEventsPer);
    EventWorkspace_sptr in_ws = std::dynamic_pointer_cast<EventWorkspace>(inputW);
    inputW->getAxis(0)->setUnit("TOF");
    // Register the workspace in the data service

    // Create the peaks workspace
    PeaksWorkspace_sptr pkws(new PeaksWorkspace());
    // pkws->setName("TOPAZ");

    // Create two peaks on that particular detector
    // First peak is at edge to check EdgePixels option
    Peak PeakObj(in_ws->getInstrument(), 0, 2., V3D(1, 1, 1));
    PeakObj.setRunNumber(3007);
    pkws->addPeak(PeakObj);
    Peak PeakObj2(in_ws->getInstrument(), 5050, 2., V3D(1, 1, 1));
    PeakObj2.setRunNumber(3007);
    pkws->addPeak(PeakObj2);
    AnalysisDataService::Instance().addOrReplace("TOPAZ", pkws);

    inputW->mutableRun().addProperty("run_number", 3007);

    std::shared_ptr<Mantid::API::Algorithm> algu =
        Mantid::API::AlgorithmFactory::Instance().create(std::string("LoadIsawUB"), 1);
    algu->initialize();
    algu->setProperty<Workspace_sptr>("InputWorkspace", inputW);
    algu->setPropertyValue("Filename", "TOPAZ_3007.mat");
    algu->execute();

    if (ev) {
      std::shared_ptr<Mantid::API::Algorithm> algb =
          Mantid::API::AlgorithmFactory::Instance().create(std::string("Rebin"), 1);
      algb->initialize();
      algb->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputW);
      algb->setPropertyValue("OutputWorkspace", "RebinResult");
      algb->setPropertyValue("Params", "5760.,10.0,5920.");
      algb->setProperty("PreserveEvents", ev);
      algb->execute();
      inputW = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("RebinResult");
    }

    CentroidPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspace", inputW);
    alg.setProperty("InPeaksWorkspace", "TOPAZ");
    alg.setProperty("OutPeaksWorkspace", "TOPAZ");
    alg.setProperty("PeakRadius", 5);
    alg.setProperty("EdgePixels", 24);
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted())

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>("TOPAZ"));
    TS_ASSERT(ws);
    if (!ws)
      return;
    Peak &peak = ws->getPeak(0);

    double row = peak.getRow();
    TS_ASSERT_DELTA(row, 50, 1.0);

    double col = peak.getCol();
    TS_ASSERT_DELTA(col, 50, 1.0);

    double tof = peak.getTOF();
    TS_ASSERT_DELTA(tof, 5814, 1.0);

    AnalysisDataService::Instance().remove("TOPAZ");
  }

  void test_MINITOPAZ() {
    do_test_MINITOPAZ(true);
    do_test_MINITOPAZ(false);
  }
};
