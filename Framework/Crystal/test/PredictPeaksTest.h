// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/PredictPeaks.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

#include <utility>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::V3D;

class PredictPeaksTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    PredictPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  /** Make a HKL peaks workspace */
  PeaksWorkspace_sptr getHKLpw(const Instrument_sptr &inst, const std::vector<V3D> &hkls, detid_t detid) {
    PeaksWorkspace_sptr hklPW;
    if (hkls.size() > 0) {
      hklPW = PeaksWorkspace_sptr(new PeaksWorkspace());
      for (const auto &hkl : hkls) {
        Peak p(inst, detid, 1.0);
        p.setHKL(hkl);
        hklPW->addPeak(p);
      }
    }
    return hklPW;
  }

  void do_test_exec(const std::string &reflectionCondition, size_t expectedNumber, const std::vector<V3D> &hkls,
                    int convention = 1, bool useExtendedDetectorSpace = false,
                    bool addExtendedDetectorDefinition = false, int edge = 0) {
    // Name of the output workspace.
    std::string outWSName("PredictPeaksTest_OutputWS");

    // Make the fake input workspace
    MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::create2DWorkspace(10000, 1);
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100);

    if (addExtendedDetectorDefinition) {
      auto extendedSpaceObj = ComponentCreationHelper::createCuboid(5., 5., 5.);
      auto extendedSpace = new ObjComponent("extended-detector-space", extendedSpaceObj, inst.get());
      extendedSpace->setPos(V3D(0.0, 0.0, 0.0));
      inst->add(extendedSpace);
    }

    inWS->setInstrument(inst);

    // Set ub and Goniometer rotation
    WorkspaceCreationHelper::setOrientedLattice(inWS, 12.0, 12.0, 12.0);
    WorkspaceCreationHelper::setGoniometer(inWS, 0., 0., 0.);

    PeaksWorkspace_sptr hklPW = getHKLpw(inst, std::move(hkls), 10000);

    PredictPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<Workspace>(inWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("WavelengthMin", "0.1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("WavelengthMax", "10.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MinDSpacing", "1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ReflectionCondition", reflectionCondition));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HKLPeaksWorkspace", hklPW));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PredictPeaksOutsideDetectors", useExtendedDetectorSpace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EdgePixels", edge));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), expectedNumber);
    V3D hklTest = {-10, -6, 1};
    hklTest *= convention;
    if (expectedNumber > 5 && !addExtendedDetectorDefinition) {
      TS_ASSERT_EQUALS(ws->getPeak(0).getHKL(), hklTest);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_exec() { do_test_exec("Primitive", 10, std::vector<V3D>()); }

  /** Fewer HKLs if they are not allowed */
  void test_exec_withReflectionCondition() { do_test_exec("C-face centred", 6, std::vector<V3D>()); }

  void test_exec_withExtendedDetectorSpace() { do_test_exec("Primitive", 3350, std::vector<V3D>(), 1, true, true); }

  void test_exec_withReflectionConditionAndExtendedDetectorSpace() {
    do_test_exec("C-face centred", 1690, std::vector<V3D>(), 1, true, true);
  }

  void test_exec_withExtendedDetectorSpaceOptionCheckedNoDefinition() {
    std::string outWSName("PredictPeaksTest_OutputWS");
    // Make the fake input workspace
    auto inWS = WorkspaceCreationHelper::create2DWorkspace(10000, 1);
    auto inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100);
    inWS->setInstrument(inst);

    // Set ub and Goniometer rotation
    WorkspaceCreationHelper::setOrientedLattice(inWS, 12.0, 12.0, 12.0);
    WorkspaceCreationHelper::setGoniometer(inWS, 0., 0., 0.);

    PredictPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<Workspace>(inWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("WavelengthMin", "0.1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("WavelengthMax", "10.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MinDSpacing", "1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ReflectionCondition", "Primitive"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PredictPeaksOutsideDetectors", true));
    alg.execute();

    // should fail to execute and throw a runtime error
    TS_ASSERT(!alg.isExecuted());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_exec_withInputHKLList() {
    std::vector<V3D> hkls{{-6, -9, 1}};
    do_test_exec("Primitive", 1, hkls);
  }

  /** More manual test of predict peaks where we build a simple UB
   * and see that the peak falls where it should.
   * In this case, hkl 1,0,0 on a crystal rotated 45 deg. relative to +Y
   * should fall on a detector towards (+1.0, 0.0, 0.0)
   */
  void do_test_manual(double Urotation, double GonioRotation) {
    // Name of the output workspace.
    std::string outWSName("PredictPeaksTest_OutputWS");

    // Make the fake input workspace
    MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::create2DWorkspace(10000, 1);
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 100);
    inWS->setInstrument(inst);

    // Set ub and Goniometer rotation
    WorkspaceCreationHelper::setOrientedLattice(inWS, 10.0, 10.0, 10.0);

    // Make a U matrix of 22.5 degree rotation around +Y
    DblMatrix u(3, 3);
    Goniometer gon;
    gon.makeUniversalGoniometer();
    gon.setRotationAngle("phi", Urotation);
    u = gon.getR();
    inWS->mutableSample().getOrientedLattice().setU(u);

    // Final rotation should add up to 45 degrees around +Y so that hkl 1,0,0
    // goes to +X
    WorkspaceCreationHelper::setGoniometer(inWS, GonioRotation, 0., 0.);

    DblMatrix ub = inWS->sample().getOrientedLattice().getUB();
    PeaksWorkspace_sptr hklPW = getHKLpw(inst, {{-1, 0, 0}}, 0);

    PredictPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<Workspace>(inWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HKLPeaksWorkspace", hklPW));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 1);
    // Center of the panel
    TS_ASSERT_EQUALS(ws->getPeak(0).getDetectorID(), 5050);
    // Expected wavelength
    TS_ASSERT_DELTA(ws->getPeak(0).getWavelength(), 14.14, 0.01);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_manual_goniometer_alone() { do_test_manual(0, 45.0); }

  void test_manual_U_and_gonio() { do_test_manual(22.5, 22.5); }

  void test_crystallography() {
    using Kernel::ConfigService;
    auto origQConv = ConfigService::Instance().getString("Q.convention");
    ConfigService::Instance().setString("Q.convention", "Crystallography");
    do_test_exec("Primitive", 10, std::vector<V3D>(), -1);
    ConfigService::Instance().setString("Q.convention", origQConv);
  }
  void test_edge() { do_test_exec("Primitive", 5, std::vector<V3D>(), 1, false, false, 10); }

  void test_exec_with_CalculateGoniometerForCW() {
    using Kernel::ConfigService;
    auto origQConv = ConfigService::Instance().getString("Q.convention");
    ConfigService::Instance().setString("Q.convention", "Crystallography");

    // Name of the output workspace.
    std::string outWSName("PredictPeaksTest_OutputWS");

    // Make the fake input workspace
    MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::create2DWorkspace(10000, 1);
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100);
    inWS->setInstrument(inst);

    // Set ub and Goniometer rotation
    WorkspaceCreationHelper::setOrientedLattice(inWS, 12.0, 12.0, 12.0);
    WorkspaceCreationHelper::setGoniometer(inWS, 0., 0., 0.);

    PredictPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<Workspace>(inWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CalculateGoniometerForCW", true));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Wavelength", "1.5"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MinAngle", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaxAngle", "5.0"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 1);
    TS_ASSERT_EQUALS(ws->getPeak(0).getHKL(), V3D(1, 0, 0));
    TS_ASSERT_DELTA(ws->getPeak(0).getWavelength(), 1.5, 1e-6);
    TS_ASSERT_DELTA(ws->getPeak(0).getQSampleFrame().X(), 0.52, 1e-2); // ~2pi/12
    TS_ASSERT_DELTA(ws->getPeak(0).getQSampleFrame().Y(), 0.0, 1e-2);
    TS_ASSERT_DELTA(ws->getPeak(0).getQSampleFrame().Z(), 0.0, 1e-2);
    std::vector<double> angles = Quat(ws->getPeak(0).getGoniometerMatrix()).getEulerAngles("YZY");
    TS_ASSERT_DELTA(angles[0], 3.58, 1e-2); // omega
    TS_ASSERT_DELTA(angles[1], 0, 1e-2);    // chi
    TS_ASSERT_DELTA(angles[2], 0, 1e-2);    // phi
    TS_ASSERT_DELTA(ws->getPeak(0).getQLabFrame().X(), 0.52, 1e-2);
    TS_ASSERT_DELTA(ws->getPeak(0).getQLabFrame().Y(), 0.00, 1e-2);
    TS_ASSERT_DELTA(ws->getPeak(0).getQLabFrame().Z(), -0.03, 1e-2);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);

    // now set the goniometer to YXZ convention
    // This should have the same q_sample for the hkl=100 peak but different q_lab
    WorkspaceCreationHelper::addTSPEntry(inWS->mutableRun(), "axis0", 10.);
    WorkspaceCreationHelper::addTSPEntry(inWS->mutableRun(), "axis1", 20.);
    WorkspaceCreationHelper::addTSPEntry(inWS->mutableRun(), "axis2", 30.);

    Mantid::Geometry::Goniometer gm;
    gm.pushAxis("axis0", 0, 1, 0, 0, 1); // Y
    gm.pushAxis("axis1", 1, 0, 0, 0, 1); // X
    gm.pushAxis("axis2", 0, 0, 1, 0, 1); // Z

    inWS->mutableRun().setGoniometer(gm, true);

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<Workspace>(inWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CalculateGoniometerForCW", true));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Wavelength", "1.5"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MinAngle", "14"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaxAngle", "16"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 1);
    TS_ASSERT_EQUALS(ws->getPeak(0).getHKL(), V3D(1, 0, 0));
    TS_ASSERT_DELTA(ws->getPeak(0).getWavelength(), 1.5, 1e-6);
    TS_ASSERT_DELTA(ws->getPeak(0).getQSampleFrame().X(), 0.52, 1e-2); // ~2pi/12
    TS_ASSERT_DELTA(ws->getPeak(0).getQSampleFrame().Y(), 0.0, 1e-2);
    TS_ASSERT_DELTA(ws->getPeak(0).getQSampleFrame().Z(), 0.0, 1e-2);
    angles = Quat(ws->getPeak(0).getGoniometerMatrix()).getEulerAngles("YXZ");
    TS_ASSERT_DELTA(angles[0], 15.23, 1e-2); // axis0
    TS_ASSERT_DELTA(angles[1], 20.0, 1e-2);  // axis1
    TS_ASSERT_DELTA(angles[2], 30.0, 1e-2);  // axis2
    TS_ASSERT_DELTA(ws->getPeak(0).getQLabFrame().X(), 0.46, 1e-2);
    TS_ASSERT_DELTA(ws->getPeak(0).getQLabFrame().Y(), 0.25, 1e-2);
    TS_ASSERT_DELTA(ws->getPeak(0).getQLabFrame().Z(), -0.03, 1e-2);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);

    ConfigService::Instance().setString("Q.convention", origQConv);
  }

  void test_exec_with_CalculateGoniometerForCW_LeanElasticPeak() {
    using Kernel::ConfigService;
    auto origQConv = ConfigService::Instance().getString("Q.convention");
    ConfigService::Instance().setString("Q.convention", "Crystallography");

    // Name of the output workspace.
    std::string outWSName("PredictPeaksTest_OutputWS");

    // Make the fake input workspace
    MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::create2DWorkspace(10000, 1);
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100);
    inWS->setInstrument(inst);

    // Set ub and Goniometer rotation
    WorkspaceCreationHelper::setOrientedLattice(inWS, 12.0, 12.0, 12.0);
    WorkspaceCreationHelper::setGoniometer(inWS, 0., 0., 0.);

    PredictPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<Workspace>(inWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CalculateGoniometerForCW", true));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Wavelength", "1.5"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MinAngle", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaxAngle", "5.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputType", "LeanElasticPeak"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    LeanElasticPeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<LeanElasticPeaksWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 1);
    TS_ASSERT_EQUALS(ws->getPeak(0).getHKL(), V3D(1, 0, 0));
    TS_ASSERT_DELTA(ws->getPeak(0).getWavelength(), 1.5, 1e-6);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);

    ConfigService::Instance().setString("Q.convention", origQConv);
  }

  void test_exec_LeanElasticPeak_no_instrument() {
    auto inWS = WorkspaceFactory::Instance().createPeaks();
    inWS->mutableSample().setOrientedLattice(std::make_unique<OrientedLattice>(2., 2., 2., 90., 90., 90.));
    PredictPeaks alg;
    std::string outWSName("PredictPeaksTest_OutputWS");
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<Workspace>(inWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputType", "LeanElasticPeak"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CalculateWavelength", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    LeanElasticPeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<LeanElasticPeaksWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 32);
    TS_ASSERT_EQUALS(ws->getPeak(0).getHKL(), V3D(-2, 0, 0));
    TS_ASSERT_EQUALS(ws->getPeak(31).getHKL(), V3D(2, 0, 0));

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_missing_instrument_sample_pos() {
    // PredictPeaks was segfaulting when the instrument in the
    // InputWorkspace didn't have the sample position set, this checks
    // that it no longer segfaults
    FrameworkManager::Instance().exec("CreatePeaksWorkspace", 2, "OutputWorkspace", "peaks_ws");
    FrameworkManager::Instance().exec("SetUB", 2, "Workspace", "peaks_ws");

    PredictPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "peaks_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "PredictPeaksTest_OutputWS"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););

    // check that the algorithm didn't run
    TS_ASSERT(!alg.isExecuted());

    AnalysisDataService::Instance().remove("peaks_ws");
  }
};

class PredictPeaksTestPerformance : public CxxTest::TestSuite {
public:
  void test_manyPeaksRectangular() {
    MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::create2DWorkspace(10000, 1);
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 100);
    inWS->setInstrument(inst);

    // Set ub and Goniometer rotation
    WorkspaceCreationHelper::setOrientedLattice(inWS, 12.0, 12.0, 12.0);
    WorkspaceCreationHelper::setGoniometer(inWS, 0., 0., 0.);

    PredictPeaks alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<Workspace>(inWS));
    alg.setPropertyValue("OutputWorkspace", "predict_peaks_performance");
    alg.setPropertyValue("WavelengthMin", ".5");
    alg.setPropertyValue("WavelengthMax", "15.0");
    alg.setPropertyValue("MinDSpacing", ".1");
    alg.setPropertyValue("ReflectionCondition", "Primitive");
    alg.execute();
  }

  void test_manyPeaks() {
    MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::create2DWorkspace(10000, 1);
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(3, V3D(0, 0, -1), V3D(0, 0, 0), 1.6, 1.0);
    inWS->setInstrument(inst);

    // Set UB matrix and Goniometer rotation
    WorkspaceCreationHelper::setOrientedLattice(inWS, 12.0, 12.0, 12.0);
    WorkspaceCreationHelper::setGoniometer(inWS, 0., 0., 0.);

    PredictPeaks alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<Workspace>(inWS));
    alg.setPropertyValue("OutputWorkspace", "predict_peaks_performance");
    alg.setPropertyValue("WavelengthMin", ".5");
    alg.setPropertyValue("WavelengthMax", "15.0");
    alg.setPropertyValue("MinDSpacing", ".1");
    alg.setPropertyValue("ReflectionCondition", "Primitive");
    alg.execute();
  }
};
