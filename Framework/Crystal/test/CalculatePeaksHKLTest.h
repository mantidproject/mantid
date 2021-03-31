// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Sample.h"
#include "MantidCrystal/CalculatePeaksHKL.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using Mantid::Crystal::CalculatePeaksHKL;
using namespace Mantid::DataObjects;

class CalculatePeaksHKLTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculatePeaksHKLTest *createSuite() { return new CalculatePeaksHKLTest(); }
  static void destroySuite(CalculatePeaksHKLTest *suite) { delete suite; }

  void test_Constructor() { TS_ASSERT_THROWS_NOTHING(CalculatePeaksHKL alg); }

  void test_Init() {
    PeaksWorkspace_sptr ws = std::make_shared<PeaksWorkspace>();

    CalculatePeaksHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeaksWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OverWrite", true));
  }

  void test_throws_without_oriented_lattice() {
    auto ws = WorkspaceCreationHelper::createPeaksWorkspace(10);

    Mantid::API::AnalysisDataService::Instance().addOrReplace("ws", ws);

    CalculatePeaksHKL alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("PeaksWorkspace", "ws");
    TSM_ASSERT_THROWS("Should throw. No UB has been given.", alg.execute(), std::runtime_error &);
  }

  void test_Execute() {
    auto lattice = std::make_unique<Mantid::Geometry::OrientedLattice>();
    Mantid::Kernel::DblMatrix UB(3, 3, true);
    UB.identityMatrix();
    lattice->setUB(UB);

    auto ws = WorkspaceCreationHelper::createPeaksWorkspace(10);
    ws->mutableSample().setOrientedLattice(std::move(lattice));

    Mantid::API::AnalysisDataService::Instance().addOrReplace("ws", ws);

    CalculatePeaksHKL alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("PeaksWorkspace", "ws");
    alg.execute();
    int numberIndexed = alg.getProperty("NumIndexed");
    TS_ASSERT_EQUALS(numberIndexed, ws->getNumberPeaks());

    for (int i = 0; i < ws->getNumberPeaks(); i++) {
      Peak &peak = ws->getPeak(i);
      Mantid::Kernel::V3D expectedHKL = peak.getQSampleFrame() / (2.0 * M_PI); // Simulate the transform. UB is unity.

      TS_ASSERT_EQUALS(expectedHKL, peak.getHKL());
    }
  }

  void test_Execute_LeanElasticPeaks() {
    auto lattice = std::make_unique<Mantid::Geometry::OrientedLattice>();

    auto ws = std::make_shared<LeanElasticPeaksWorkspace>();
    ws->mutableSample().setOrientedLattice(std::move(lattice));
    ws->addPeak(LeanElasticPeak(Mantid::Kernel::V3D(2, 0, 0), 1.));
    ws->addPeak(LeanElasticPeak(Mantid::Kernel::V3D(0, 4, 0), 1.));

    Mantid::API::AnalysisDataService::Instance().addOrReplace("ws", ws);

    CalculatePeaksHKL alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("PeaksWorkspace", "ws");
    alg.execute();
    int numberIndexed = alg.getProperty("NumIndexed");
    TS_ASSERT_EQUALS(numberIndexed, ws->getNumberPeaks());

    TS_ASSERT_DELTA(ws->getPeak(0).getH(), M_1_PI, 1e-9)
    TS_ASSERT_DELTA(ws->getPeak(0).getK(), 0, 1e-9)
    TS_ASSERT_DELTA(ws->getPeak(0).getL(), 0, 1e-9)

    TS_ASSERT_DELTA(ws->getPeak(1).getH(), 0, 1e-9)
    TS_ASSERT_DELTA(ws->getPeak(1).getK(), M_2_PI, 1e-9)
    TS_ASSERT_DELTA(ws->getPeak(1).getL(), 0, 1e-9)
  }

  // Don't index peaks that are already indexed.
  void test_SkipIndexing() {
    auto lattice = std::make_unique<Mantid::Geometry::OrientedLattice>();
    Mantid::Kernel::DblMatrix UB(3, 3, true);
    UB.identityMatrix();
    lattice->setUB(UB);

    auto ws = WorkspaceCreationHelper::createPeaksWorkspace(10);
    ws->mutableSample().setOrientedLattice(std::move(lattice));
    ws->getPeak(0).setHKL(1, 1, 1); // First peak is already indexed now.

    Mantid::API::AnalysisDataService::Instance().addOrReplace("ws", ws);

    CalculatePeaksHKL alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("PeaksWorkspace", "ws");
    alg.setProperty("OverWrite", false);
    alg.execute();
    const int numberIndexed = alg.getProperty("NumIndexed");
    const int expectedNumberIndexed = ws->getNumberPeaks() - 1;
    TS_ASSERT_EQUALS(expectedNumberIndexed, numberIndexed);
  }

  // Don't index peaks that are already indexed.
  void test_OverwriteIndexed() {
    auto lattice = std::make_unique<Mantid::Geometry::OrientedLattice>();
    Mantid::Kernel::DblMatrix UB(3, 3, true);
    UB.identityMatrix();
    lattice->setUB(UB);

    auto ws = WorkspaceCreationHelper::createPeaksWorkspace(10);
    ws->mutableSample().setOrientedLattice(std::move(lattice));
    ws->getPeak(0).setHKL(1, 1, 1); // First peak is already indexed now.

    Mantid::API::AnalysisDataService::Instance().addOrReplace("ws", ws);

    CalculatePeaksHKL alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("PeaksWorkspace", "ws");
    alg.setProperty("OverWrite", true);
    alg.execute();
    const int numberIndexed = alg.getProperty("NumIndexed");
    const int expectedNumberIndexed = ws->getNumberPeaks();
    TS_ASSERT_EQUALS(expectedNumberIndexed, numberIndexed);
  }
};
