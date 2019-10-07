// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PREDICTFRACTIONALPEAKSTEST_H_
#define PREDICTFRACTIONALPEAKSTEST_H_

#include "MantidAPI/Sample.h"
#include "MantidCrystal/IndexPeaks.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/PredictFractionalPeaks.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/FacilityHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using Mantid::API::Workspace_sptr;
using Mantid::Crystal::PredictFractionalPeaks;
using Mantid::DataObjects::PeaksWorkspace_sptr;
using Mantid::Geometry::OrientedLattice;
using Mantid::Kernel::V3D;

using namespace Mantid::DataHandling;
using namespace Mantid::Crystal;

namespace {

PeaksWorkspace_sptr createIndexedPeaksWorkspace() {
  LoadNexusProcessed loader;
  loader.setChild(true);
  loader.initialize();
  loader.isInitialized();
  loader.setPropertyValue("Filename", "TOPAZ_3007.peaks.nxs");
  loader.setPropertyValue("OutputWorkspace", "__unused__");
  loader.execute();
  Workspace_sptr loadedWS = loader.getProperty("OutputWorkspace");

  LoadIsawUB ubLoader;
  ubLoader.setChild(true);
  ubLoader.initialize();
  ubLoader.setProperty("InputWorkspace", loadedWS);
  ubLoader.setProperty("FileName", "TOPAZ_3007.mat");
  ubLoader.execute();

  IndexPeaks indexer;
  indexer.setChild(true);
  indexer.setLogging(false);
  indexer.initialize();
  indexer.setProperty("PeaksWorkspace", loadedWS);
  indexer.execute();

  return boost::dynamic_pointer_cast<PeaksWorkspace_sptr::element_type>(
      loadedWS);
}

PeaksWorkspace_sptr runPredictFractionalPeaks(
    const PeaksWorkspace_sptr &inputPeaks,
    const std::unordered_map<const char *, const char *> &args) {
  PredictFractionalPeaks alg;
  alg.setChild(true);
  alg.initialize();
  alg.setProperty("Peaks", inputPeaks);
  alg.setPropertyValue("FracPeaks", "__unused__");
  for (const auto &nameValue : args) {
    alg.setProperty(nameValue.first, nameValue.second);
  }
  alg.execute();
  TS_ASSERT(alg.isExecuted())

  return alg.getProperty("FracPeaks");
}

} // namespace

class PredictFractionalPeaksTest : public CxxTest::TestSuite {

public:
  void test_Init() {
    PredictFractionalPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void setUp() override {
    if (!m_indexedPeaks) {
      m_indexedPeaks = createIndexedPeaksWorkspace();
    }
  }

  void test_exec_without_hkl_range_uses_input_peaks() {
    const auto fracPeaks = runPredictFractionalPeaks(
        m_indexedPeaks,
        {{"HOffset", "-0.5,0,0.5"}, {"KOffset", "0.0"}, {"LOffset", "0.2"}});

    TS_ASSERT_EQUALS(117, fracPeaks->getNumberPeaks())
    const auto &peak0 = fracPeaks->getPeak(0);
    TS_ASSERT_DELTA(peak0.getH(), -5.5, .0001)
    TS_ASSERT_DELTA(peak0.getK(), 7.0, .0001)
    TS_ASSERT_DELTA(peak0.getL(), -3.8, .0001)

    const auto &peak3 = fracPeaks->getPeak(3);
    TS_ASSERT_DELTA(peak3.getH(), -5.5, .0001)
    TS_ASSERT_DELTA(peak3.getK(), 3.0, .0001)
    TS_ASSERT_DELTA(peak3.getL(), -2.8, .0001)

    const auto &peak6 = fracPeaks->getPeak(6);
    TS_ASSERT_DELTA(peak6.getH(), -6.5, .0001)
    TS_ASSERT_DELTA(peak6.getK(), 4.0, .0001)
    TS_ASSERT_DELTA(peak6.getL(), -3.8, .0001)
  }

  void test_exec_with_include_in_range() {
    const auto fracPeaks = runPredictFractionalPeaks(
        m_indexedPeaks, {{"HOffset", "-0.5,0,0.5"},
                         {"KOffset", "0.0"},
                         {"LOffset", "0.2"},
                         {"IncludeAllPeaksInRange", "1"},
                         {"Hmin", "-1"},
                         {"Hmax", "1"},
                         {"Kmin", "-2"},
                         {"Kmax", "2"},
                         {"Lmin", "-3"},
                         {"Lmax", "3"}

                        });
    TS_ASSERT_EQUALS(9, fracPeaks->getNumberPeaks())
    const auto &peak0 = fracPeaks->getPeak(0);
    TS_ASSERT_DELTA(peak0.getH(), -0.5, .0001)
    TS_ASSERT_DELTA(peak0.getK(), -2.0, .0001)
    TS_ASSERT_DELTA(peak0.getL(), -1.8, .0001)

    const auto &peak3 = fracPeaks->getPeak(3);
    TS_ASSERT_DELTA(peak3.getH(), -1.0, .0001)
    TS_ASSERT_DELTA(peak3.getK(), 1.0, .0001)
    TS_ASSERT_DELTA(peak3.getL(), -0.8, .0001)

    const auto &peak6 = fracPeaks->getPeak(6);
    TS_ASSERT_DELTA(peak6.getH(), -0.5, .0001)
    TS_ASSERT_DELTA(peak6.getK(), 0.0, .0001)
    TS_ASSERT_DELTA(peak6.getL(), 0.2, .0001)

    const auto &peak8 = fracPeaks->getPeak(8);
    TS_ASSERT_DELTA(peak8.getH(), -1.5, .0001)
    TS_ASSERT_DELTA(peak8.getK(), 2.0, .0001)
    TS_ASSERT_DELTA(peak8.getL(), 0.2, .0001)
  }

  // ---------------- Failure tests -----------------------------
  void test_empty_peaks_workspace_is_not_allowed() {
    PredictFractionalPeaks alg;
    alg.initialize();
    alg.setProperty("Peaks", WorkspaceCreationHelper::createPeaksWorkspace(0));

    auto helpMsgs = alg.validateInputs();

    const auto valueIter = helpMsgs.find("Peaks");
    TS_ASSERT(valueIter != helpMsgs.cend())
  }

private:
  PeaksWorkspace_sptr m_indexedPeaks;
};

#endif /* PredictFRACTIONALPEAKSTEST_H_ */
