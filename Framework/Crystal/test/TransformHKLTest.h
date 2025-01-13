// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/TransformHKL.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/SpecialCoordinateSystem.h"

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class TransformHKLTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    TransformHKL alg;
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
    // make a reasonable UB and
    // put it in the workspace
    V3D row_0(-0.0122354, 0.00480056, -0.0860404);
    V3D row_1(0.1165450, 0.00178145, 0.0045884);
    V3D row_2(0.0273738, -0.08973560, 0.0252595);

    Matrix<double> UB(3, 3, false);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);

    auto lattice = std::make_unique<OrientedLattice>();
    lattice->setUB(UB);
    ws->mutableSample().setOrientedLattice(std::move(lattice));

    Matrix<double> UB_inverse(UB);
    UB_inverse.Invert();
    // index the peaks with UB_inverse
    std::vector<Peak> &peaks = ws->getPeaks();
    size_t n_peaks = ws->getNumberPeaks();
    for (size_t i = 0; i < n_peaks; i++) {
      V3D q_vec = (peaks[i].getQSampleFrame()) * (0.5 / M_PI);
      V3D hkl_vec = UB_inverse * q_vec;
      peaks[i].setHKL(hkl_vec);
    }
    // save original indexes, so we can
    // verify they were properly changed
    std::vector<V3D> original_hkl;
    for (size_t i = 0; i < n_peaks; i++) {
      original_hkl.emplace_back(peaks[i].getHKL());
    }

    TransformHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Tolerance", "0.1"));

    // specify a matrix that will swap H and K and negate L
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("HKLTransform", "0,1,0,1,0,0,0,0,-1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Check that the peaks were all indexed
    double tolerance = alg.getProperty("Tolerance");
    peaks = ws->getPeaks();
    for (size_t i = 0; i < n_peaks; i++) {
      TS_ASSERT(IndexingUtils::ValidIndex(peaks[i].getHKL(), tolerance));
    }

    // Check that the transform actually
    // did swap H & K and negate L.
    for (size_t i = 0; i < n_peaks; i++) {
      TS_ASSERT_DELTA(original_hkl[i][0], peaks[i].getK(), 1.0e-5);
      TS_ASSERT_DELTA(original_hkl[i][1], peaks[i].getH(), 1.0e-5);
      TS_ASSERT_DELTA(original_hkl[i][2], -peaks[i].getL(), 1.0e-5);
    }
    // Check the output properties
    int numIndexed = alg.getProperty("NumIndexed");
    TS_ASSERT_EQUALS(numIndexed, 43);
    double averageError = alg.getProperty("AverageError");
    TS_ASSERT_DELTA(averageError, 0.0097, 1e-3);

    AnalysisDataService::Instance().remove(WSName);
  }

  void test_exec_LeanElasticPeak() {
    auto ws = std::make_shared<LeanElasticPeaksWorkspace>();
    AnalysisDataService::Instance().addOrReplace("ws", ws);
    auto lattice = std::make_unique<Mantid::Geometry::OrientedLattice>(5, 6, 7, 90, 90, 120);
    ws->mutableSample().setOrientedLattice(std::move(lattice));
    ws->addPeak(V3D(1, 0, 0), SpecialCoordinateSystem::HKL);
    ws->addPeak(V3D(0, 2, 0), SpecialCoordinateSystem::HKL);
    ws->addPeak(V3D(0, 0, 3), SpecialCoordinateSystem::HKL);

    TransformHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Tolerance", "0.1"));

    // specify a matrix that will swap H and K and negate L
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("HKLTransform", "0,1,0,1,0,0,0,0,-1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_EQUALS(ws->getPeak(0).getHKL(), V3D(0, 1, 0));
    TS_ASSERT_EQUALS(ws->getPeak(1).getHKL(), V3D(2, 0, 0));
    TS_ASSERT_EQUALS(ws->getPeak(2).getHKL(), V3D(0, 0, -3));
  }

  void test_exec_ModVectorTransform() {
    auto ws = std::make_shared<LeanElasticPeaksWorkspace>();
    AnalysisDataService::Instance().addOrReplace("ws", ws);
    auto lattice = std::make_unique<Mantid::Geometry::OrientedLattice>(5, 6, 7, 90, 90, 120);

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

    ws->mutableSample().setOrientedLattice(std::move(lattice));
    ws->addPeak(V3D(1, 0, 0), SpecialCoordinateSystem::HKL);
    ws->addPeak(V3D(0, 2, 0), SpecialCoordinateSystem::HKL);
    ws->addPeak(V3D(0, 0, 3), SpecialCoordinateSystem::HKL);

    V3D mnp(1, 1, 1);

    int n_peaks = ws->getNumberPeaks();
    for (int i = 0; i < n_peaks; i++) {
      IPeak &peak = ws->getPeak(i);
      V3D hkl = peak.getHKL();
      peak.setHKL(hkl + modHKL * mnp);
      peak.setIntHKL(hkl);
      peak.setIntMNP(mnp);
      peak.setQSampleFrame(peak.getQSampleFrame() + modUB * mnp * 2 * M_PI, std::nullopt);
    }

    TransformHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Tolerance", "0.1"));

    // specify a matrix that will swap H and K and negate L
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("HKLTransform", "0,1,0,1,0,0,0,0,-1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_EQUALS(ws->getPeak(0).getHKL(), V3D(0.03, 1.02, -0.04));
    TS_ASSERT_EQUALS(ws->getPeak(1).getHKL(), V3D(2.03, 0.02, -0.04));
    TS_ASSERT_EQUALS(ws->getPeak(2).getHKL(), V3D(0.03, 0.02, -3.04));
  }

  void test_exec_ModVectorTransformImproperModUB() {
    auto ws = std::make_shared<LeanElasticPeaksWorkspace>();
    AnalysisDataService::Instance().addOrReplace("ws", ws);
    auto lattice = std::make_unique<Mantid::Geometry::OrientedLattice>(5, 6, 7, 90, 90, 120);

    V3D mod_vec_0(0.01, 0.02, 0.03);
    V3D mod_vec_1(-0.02, 0.03, 0.05);
    V3D mod_vec_2(0.03, -0.02, -0.04);

    Matrix<double> modHKL(3, 3, false);
    modHKL.setColumn(0, mod_vec_0);
    modHKL.setColumn(1, mod_vec_1);
    modHKL.setColumn(2, mod_vec_2);

    Matrix<double> UB = lattice->getUB();

    lattice->setModVec1(mod_vec_0);
    lattice->setModVec2(mod_vec_1);
    lattice->setModVec3(mod_vec_2);
    lattice->setModHKL(modHKL);

    Matrix<double> modUB = lattice->getModUB();

    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        TS_ASSERT_DELTA(modUB[i][j], 0, 0.0001);
      }
    }

    modUB = UB * modHKL;

    ws->mutableSample().setOrientedLattice(std::move(lattice));
    ws->addPeak(V3D(1, 0, 0), SpecialCoordinateSystem::HKL);
    ws->addPeak(V3D(0, 2, 0), SpecialCoordinateSystem::HKL);
    ws->addPeak(V3D(0, 0, 3), SpecialCoordinateSystem::HKL);

    V3D mnp(1, 1, 1);

    int n_peaks = ws->getNumberPeaks();
    for (int i = 0; i < n_peaks; i++) {
      IPeak &peak = ws->getPeak(i);
      V3D hkl = peak.getHKL();
      peak.setHKL(hkl + modHKL * mnp);
      peak.setIntHKL(hkl);
      peak.setIntMNP(mnp);
      peak.setQSampleFrame(peak.getQSampleFrame() + modUB * mnp * 2 * M_PI, std::nullopt);
    }

    TransformHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Tolerance", "0.1"));

    // specify a matrix that will swap H and K and negate L
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("HKLTransform", "0,1,0,1,0,0,0,0,-1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_EQUALS(ws->getPeak(0).getHKL(), V3D(0.03, 1.02, -0.04));
    TS_ASSERT_EQUALS(ws->getPeak(1).getHKL(), V3D(2.03, 0.02, -0.04));
    TS_ASSERT_EQUALS(ws->getPeak(2).getHKL(), V3D(0.03, 0.02, -3.04));

    modUB = ws->sample().getOrientedLattice().getModUB();

    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        TS_ASSERT_DELTA(modUB[i][j], 0, 0.0001);
      }
    }
  }

  void test_exec_skip_FindError() {
    auto ws = std::make_shared<LeanElasticPeaksWorkspace>();
    AnalysisDataService::Instance().addOrReplace("ws", ws);
    auto lattice = std::make_unique<Mantid::Geometry::OrientedLattice>(5, 6, 7, 90, 90, 120);
    ws->mutableSample().setOrientedLattice(std::move(lattice));
    ws->addPeak(V3D(1, 2, 0), SpecialCoordinateSystem::HKL);
    ws->addPeak(V3D(0, 0, 3), SpecialCoordinateSystem::HKL);

    TransformHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Tolerance", "0.1"));

    // skip error calculation for lattice parameters
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FindError", false));

    // specify a matrix that will swap H and K and negate L
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("HKLTransform", "0,1,0,1,0,0,0,0,-1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_EQUALS(ws->getPeak(0).getHKL(), V3D(2, 1, 0));
    TS_ASSERT_EQUALS(ws->getPeak(1).getHKL(), V3D(0, 0, -3));

    auto lat = ws->sample().getOrientedLattice();

    TS_ASSERT_DELTA(lat.errora(), 0.0, 1e-6);
    TS_ASSERT_DELTA(lat.errorb(), 0.0, 1e-6);
    TS_ASSERT_DELTA(lat.errorc(), 0.0, 1e-6);
    TS_ASSERT_DELTA(lat.erroralpha(), 0.0, 1e-6);
    TS_ASSERT_DELTA(lat.errorbeta(), 0.0, 1e-6);
    TS_ASSERT_DELTA(lat.errorgamma(), 0.0, 1e-6);
  }
};
