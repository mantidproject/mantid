// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidCrystal/FindUBUsingFFT.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/OptimizeLatticeForCellType.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/SpecialCoordinateSystem.h"

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using Mantid::DataHandling::LoadNexusProcessed;
using Mantid::Geometry::OrientedLattice;

class OptimizeLatticeForCellTypeTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    OptimizeLatticeForCellType alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Name of the output workspace.
    std::string WSName("peaks");
    LoadNexusProcessed loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", "TOPAZ_3007.peaks.nxs");
    loader.setPropertyValue("OutputWorkspace", WSName);

    TS_ASSERT(loader.execute());
    TS_ASSERT(loader.isExecuted());

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(WSName)));
    TS_ASSERT(ws);
    if (!ws)
      return;
    FindUBUsingFFT alg_fft;
    TS_ASSERT_THROWS_NOTHING(alg_fft.initialize());
    TS_ASSERT(alg_fft.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg_fft.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg_fft.setPropertyValue("MinD", "8.0"));
    TS_ASSERT_THROWS_NOTHING(alg_fft.setPropertyValue("MaxD", "13.0"));
    TS_ASSERT_THROWS_NOTHING(alg_fft.setPropertyValue("Tolerance", "0.15"));
    TS_ASSERT_THROWS_NOTHING(alg_fft.execute(););
    TS_ASSERT(alg_fft.isExecuted());
    OptimizeLatticeForCellType alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CellType", "Monoclinic"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Check that we set an oriented lattice
    TS_ASSERT(ws->mutableSample().hasOrientedLattice());
    // Check that the UB matrix is the same as in TOPAZ_3007.mat
    OrientedLattice latt = ws->mutableSample().getOrientedLattice();

    double correct_UB[] = {-0.0451, 0.0406, -0.0125, 0.0016, -0.0031, 0.1158, 0.0574, 0.0322, 0.0275};

    std::vector<double> UB_calculated = latt.getUB().getVector();

    for (size_t i = 0; i < 9; i++) {
      TS_ASSERT_DELTA(correct_UB[i], UB_calculated[i], 5e-4);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(WSName);
  }

  void test_exec_LeanElastic() {
    // Name of the output workspace.
    std::string WSName("peaks");
    LoadNexusProcessed loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", "TOPAZ_3007.peaks.nxs");
    loader.setPropertyValue("OutputWorkspace", WSName);

    TS_ASSERT(loader.execute());
    TS_ASSERT(loader.isExecuted());

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(WSName)));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Convert PeaksWorkspace to LeanElasticPeaksWorkspace
    auto lpw = std::make_shared<LeanElasticPeaksWorkspace>();
    for (auto peak : ws->getPeaks())
      lpw->addPeak(peak);
    AnalysisDataService::Instance().addOrReplace(WSName, lpw);

    FindUBUsingFFT alg_fft;
    TS_ASSERT_THROWS_NOTHING(alg_fft.initialize());
    TS_ASSERT(alg_fft.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg_fft.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg_fft.setPropertyValue("MinD", "8.0"));
    TS_ASSERT_THROWS_NOTHING(alg_fft.setPropertyValue("MaxD", "13.0"));
    TS_ASSERT_THROWS_NOTHING(alg_fft.setPropertyValue("Tolerance", "0.15"));
    TS_ASSERT_THROWS_NOTHING(alg_fft.execute(););
    TS_ASSERT(alg_fft.isExecuted());
    OptimizeLatticeForCellType alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CellType", "Monoclinic"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Check that we set an oriented lattice
    TS_ASSERT(lpw->mutableSample().hasOrientedLattice());
    // Check that the UB matrix is the same as in TOPAZ_3007.mat
    OrientedLattice latt = lpw->mutableSample().getOrientedLattice();

    double correct_UB[] = {-0.0451, 0.0406, -0.0125, 0.0016, -0.0031, 0.1158, 0.0574, 0.0322, 0.0275};

    std::vector<double> UB_calculated = latt.getUB().getVector();

    for (size_t i = 0; i < 9; i++) {
      TS_ASSERT_DELTA(correct_UB[i], UB_calculated[i], 5e-4);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(WSName);
  }

  void test_exec_ModVector() {
    auto ws = std::make_shared<LeanElasticPeaksWorkspace>();
    AnalysisDataService::Instance().addOrReplace("ws", ws);

    double a = 5;
    double b = 6;
    double c = 7;
    double alpha = 90;
    double beta = 100;
    double gamma = 90;

    auto lattice = std::make_unique<Mantid::Geometry::OrientedLattice>(a, b, c, alpha, beta, gamma);

    V3D mod_vec_0(0.01, 0.02, 0.03);
    V3D mod_vec_1(-0.02, 0.03, 0.05);
    V3D mod_vec_2(0.03, -0.02, -0.04);

    Matrix<double> modHKL(3, 3, false);
    modHKL.setColumn(0, mod_vec_0);
    modHKL.setColumn(1, mod_vec_1);
    modHKL.setColumn(2, mod_vec_2);

    Matrix<double> UB = lattice->getUB();
    Matrix<double> modUB = UB * modHKL;

    lattice->setModVec1(mod_vec_0);
    lattice->setModVec2(mod_vec_1);
    lattice->setModVec3(mod_vec_2);
    lattice->setModHKL(modHKL);
    lattice->setModUB(modUB);
    lattice->setMaxOrder(1);

    ws->mutableSample().setOrientedLattice(std::move(lattice));
    ws->addPeak(V3D(1, 0, 0), SpecialCoordinateSystem::HKL);
    ws->addPeak(V3D(0, 2, 0), SpecialCoordinateSystem::HKL);
    ws->addPeak(V3D(0, 0, 3), SpecialCoordinateSystem::HKL);
    ws->addPeak(V3D(-3, 1, 2), SpecialCoordinateSystem::HKL);
    ws->addPeak(V3D(4, -5, 6), SpecialCoordinateSystem::HKL);

    V3D mnp(1, 1, 1);

    int n_peaks = ws->getNumberPeaks();
    for (int i = 0; i < n_peaks; i++) {
      IPeak &peak = ws->getPeak(i);
      V3D hkl = peak.getHKL();
      // intentionally incorrect so it is handled by algorithm
      // peak.setHKL(hkl + modHKL * mnp);
      peak.setIntHKL(hkl);
      peak.setIntMNP(mnp);
      peak.setQSampleFrame(peak.getQSampleFrame() + modUB * mnp * 2 * M_PI, std::nullopt);
    }

    OptimizeLatticeForCellType alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CellType", "Monoclinic"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Check that we set an oriented lattice
    TS_ASSERT(ws->mutableSample().hasOrientedLattice());
    // Check that the UB matrix is the same as in TOPAZ_3007.mat
    OrientedLattice lat = ws->mutableSample().getOrientedLattice();

    TS_ASSERT_DELTA(lat.a(), a, 1e-4);
    TS_ASSERT_DELTA(lat.b(), b, 1e-4);
    TS_ASSERT_DELTA(lat.c(), c, 1e-4);
    TS_ASSERT_DELTA(lat.alpha(), alpha, 1e-4);
    TS_ASSERT_DELTA(lat.beta(), beta, 1e-4);
    TS_ASSERT_DELTA(lat.gamma(), gamma, 1e-4);

    AnalysisDataService::Instance().remove("ws");
  }
};
