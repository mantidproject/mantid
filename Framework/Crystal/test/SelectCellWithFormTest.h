// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/SelectCellWithForm.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class SelectCellWithFormTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    SelectCellWithForm alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Name of the loader's output workspace.
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
    // set a Niggli UB for run 3007
    // (CuTCA) in the oriented lattice
    V3D row_0(0.0122354, 0.00480056, 0.0860404);
    V3D row_1(-0.1165450, 0.00178145, -0.0045884);
    V3D row_2(-0.0273738, -0.08973560, -0.0252595);

    Matrix<double> UB(3, 3, false);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);

    auto lattice = std::make_unique<OrientedLattice>();
    lattice->setUB(UB);
    ws->mutableSample().setOrientedLattice(std::move(lattice));

    // now get the UB back from the WS
    UB = ws->sample().getOrientedLattice().getUB();

    SelectCellWithForm alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FormNumber", 35));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Apply", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Tolerance", 0.12));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    int num_indexed = alg.getProperty("NumIndexed");
    TS_ASSERT_EQUALS(num_indexed, 43);
    double average_error = alg.getProperty("AverageError");
    TS_ASSERT_DELTA(average_error, 0.00972862, .0001);

    AnalysisDataService::Instance().remove(WSName);
  }

  void test_exec_LeanElasticPeak() {
    // Name of the loader's output workspace.
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

    // Convert PeaksWorkspace to LeanElasticPeaksWorkspace
    auto lpw = std::make_shared<LeanElasticPeaksWorkspace>();
    for (auto peak : ws->getPeaks())
      lpw->addPeak(peak);
    AnalysisDataService::Instance().addOrReplace(WSName, lpw);

    // set a Niggli UB for run 3007
    // (CuTCA) in the oriented lattice
    V3D row_0(0.0122354, 0.00480056, 0.0860404);
    V3D row_1(-0.1165450, 0.00178145, -0.0045884);
    V3D row_2(-0.0273738, -0.08973560, -0.0252595);

    Matrix<double> UB(3, 3, false);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);

    auto lattice = std::make_unique<OrientedLattice>();
    lattice->setUB(UB);
    lpw->mutableSample().setOrientedLattice(std::move(lattice));

    // now get the UB back from the WS
    UB = lpw->sample().getOrientedLattice().getUB();

    SelectCellWithForm alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FormNumber", 35));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Apply", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Tolerance", 0.12));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    int num_indexed = alg.getProperty("NumIndexed");
    TS_ASSERT_EQUALS(num_indexed, 43);
    double average_error = alg.getProperty("AverageError");
    TS_ASSERT_DELTA(average_error, 0.00972862, .0001);

    AnalysisDataService::Instance().remove(WSName);
  }

  void test_exec_ModVectorTransform() {
    // Name of the loader's output workspace.
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

    // Convert PeaksWorkspace to LeanElasticPeaksWorkspace
    auto lpw = std::make_shared<LeanElasticPeaksWorkspace>();
    for (auto peak : ws->getPeaks())
      lpw->addPeak(peak);
    AnalysisDataService::Instance().addOrReplace(WSName, lpw);

    // set a Niggli UB for run 3007
    // (CuTCA) in the oriented lattice
    V3D row_0(0.0122354, 0.00480056, 0.0860404);
    V3D row_1(-0.1165450, 0.00178145, -0.0045884);
    V3D row_2(-0.0273738, -0.08973560, -0.0252595);

    Matrix<double> UB(3, 3, false);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);

    V3D mod_vec_0(0.01, 0.02, 0.03);
    V3D mod_vec_1(-0.03, 0.01, -0.04);
    V3D mod_vec_2(-0.05, -0.01, -0.02);

    Matrix<double> modHKL(3, 3, false);
    modHKL.setColumn(0, mod_vec_0);
    modHKL.setColumn(1, mod_vec_1);
    modHKL.setColumn(2, mod_vec_2);

    Matrix<double> modUB = UB * modHKL;

    auto lattice = std::make_unique<OrientedLattice>();
    lattice->setModVec1(mod_vec_0);
    lattice->setModVec2(mod_vec_1);
    lattice->setModVec3(mod_vec_2);
    lattice->setUB(UB);
    lattice->setModHKL(modHKL);
    lattice->setModUB(modUB);

    // test offset mnp vector
    V3D mnp(-1, -2, 1);

    int n_peaks = lpw->getNumberPeaks();
    for (int i = 0; i < n_peaks; i++) {
      IPeak &peak = lpw->getPeak(i);
      V3D hkl = peak.getIntHKL();
      if (hkl.norm2() > 0) {
        peak.setHKL(hkl + modHKL * mnp);
        peak.setIntHKL(hkl);
        peak.setIntMNP(mnp);
        peak.setQSampleFrame(peak.getQSampleFrame() + modUB * mnp * 2 * M_PI, std::nullopt);
      }
    }

    lpw->mutableSample().setOrientedLattice(std::move(lattice));

    // now get the UB back from the WS
    UB = lpw->sample().getOrientedLattice().getUB();

    for (int i = 0; i < n_peaks; i++) {
      IPeak &peak = lpw->getPeak(i);
      if (peak.getIntHKL().norm2() > 0) {
        mnp = peak.getIntMNP();
        TS_ASSERT_DELTA(mnp[0], -1, .0001);
        TS_ASSERT_DELTA(mnp[1], -2, .0001);
        TS_ASSERT_DELTA(mnp[2], 1, .0001);
      }
    }

    SelectCellWithForm alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FormNumber", 35));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Apply", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Tolerance", 0.12));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    int num_indexed = alg.getProperty("NumIndexed");
    TS_ASSERT_EQUALS(num_indexed, 43);
    double average_error = alg.getProperty("AverageError");
    TS_ASSERT_DELTA(average_error, 0.00972862, .0001);

    Matrix<double> newUB = lpw->sample().getOrientedLattice().getUB();
    Matrix<double> newModHKL = lpw->sample().getOrientedLattice().getModHKL();

    for (int i = 0; i < n_peaks; i++) {
      IPeak &peak = lpw->getPeak(i);
      if (peak.getIntHKL().norm2() > 0) {
        V3D mnp = peak.getIntMNP();
        TS_ASSERT_DELTA(mnp[0], -1, .0001);
        TS_ASSERT_DELTA(mnp[1], -2, .0001);
        TS_ASSERT_DELTA(mnp[2], 1, .0001);

        V3D diff = peak.getIntHKL() + newModHKL * peak.getIntMNP() - peak.getHKL();
        TS_ASSERT_DELTA(diff[0], 0, 0.06);
        TS_ASSERT_DELTA(diff[1], 0, 0.06);
        TS_ASSERT_DELTA(diff[2], 0, 0.06);
      }
    }

    AnalysisDataService::Instance().remove(WSName);
  }

  void test_exec_ModVectorTransformWithImproperModUB() {
    // Name of the loader's output workspace.
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

    // Convert PeaksWorkspace to LeanElasticPeaksWorkspace
    auto lpw = std::make_shared<LeanElasticPeaksWorkspace>();
    for (auto peak : ws->getPeaks())
      lpw->addPeak(peak);
    AnalysisDataService::Instance().addOrReplace(WSName, lpw);

    // set a Niggli UB for run 3007
    // (CuTCA) in the oriented lattice
    V3D row_0(0.0122354, 0.00480056, 0.0860404);
    V3D row_1(-0.1165450, 0.00178145, -0.0045884);
    V3D row_2(-0.0273738, -0.08973560, -0.0252595);

    Matrix<double> UB(3, 3, false);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);

    V3D mod_vec_0(0.01, 0.02, 0.03);
    V3D mod_vec_1(-0.03, 0.01, -0.04);
    V3D mod_vec_2(-0.05, -0.01, -0.02);

    Matrix<double> modHKL(3, 3, false);
    modHKL.setColumn(0, mod_vec_0);
    modHKL.setColumn(1, mod_vec_1);
    modHKL.setColumn(2, mod_vec_2);

    auto lattice = std::make_unique<OrientedLattice>();
    lattice->setModVec1(mod_vec_0);
    lattice->setModVec2(mod_vec_1);
    lattice->setModVec3(mod_vec_2);
    lattice->setUB(UB);

    Matrix<double> modUB = lattice->getModUB();

    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        TS_ASSERT_DELTA(modUB[i][j], 0, 0.0001);
      }
    }

    modUB = UB * modHKL;

    // test offset mnp vector
    V3D mnp(-1, -2, 1);

    int n_peaks = lpw->getNumberPeaks();
    for (int i = 0; i < n_peaks; i++) {
      IPeak &peak = lpw->getPeak(i);
      V3D hkl = peak.getIntHKL();
      if (hkl.norm2() > 0) {
        peak.setHKL(hkl + modHKL * mnp);
        peak.setIntHKL(hkl);
        peak.setIntMNP(mnp);
        peak.setQSampleFrame(peak.getQSampleFrame() + modUB * mnp * 2 * M_PI, std::nullopt);
      }
    }

    lpw->mutableSample().setOrientedLattice(std::move(lattice));

    // now get the UB back from the WS
    UB = lpw->sample().getOrientedLattice().getUB();

    for (int i = 0; i < n_peaks; i++) {
      IPeak &peak = lpw->getPeak(i);
      if (peak.getIntHKL().norm2() > 0) {
        mnp = peak.getIntMNP();
        TS_ASSERT_DELTA(mnp[0], -1, .0001);
        TS_ASSERT_DELTA(mnp[1], -2, .0001);
        TS_ASSERT_DELTA(mnp[2], 1, .0001);
      }
    }

    SelectCellWithForm alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FormNumber", 35));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Apply", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Tolerance", 0.12));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    int num_indexed = alg.getProperty("NumIndexed");
    TS_ASSERT_EQUALS(num_indexed, 43);
    double average_error = alg.getProperty("AverageError");
    TS_ASSERT_DELTA(average_error, 0.00972862, .0001);

    Matrix<double> newUB = lpw->sample().getOrientedLattice().getUB();
    Matrix<double> newModHKL = lpw->sample().getOrientedLattice().getModHKL();

    for (int i = 0; i < n_peaks; i++) {
      IPeak &peak = lpw->getPeak(i);
      if (peak.getIntHKL().norm2() > 0) {
        V3D mnp = peak.getIntMNP();
        TS_ASSERT_DELTA(mnp[0], -1, .0001);
        TS_ASSERT_DELTA(mnp[1], -2, .0001);
        TS_ASSERT_DELTA(mnp[2], 1, .0001);

        V3D diff = peak.getIntHKL() + newModHKL * peak.getIntMNP() - peak.getHKL();
        TS_ASSERT_DELTA(diff[0], 0, 0.06);
        TS_ASSERT_DELTA(diff[1], 0, 0.06);
        TS_ASSERT_DELTA(diff[2], 0, 0.06);
      }
    }

    modUB = lpw->sample().getOrientedLattice().getModUB();

    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        TS_ASSERT_DELTA(modUB[i][j], 0, 0.0001);
      }
    }

    AnalysisDataService::Instance().remove(WSName);
  }
};
