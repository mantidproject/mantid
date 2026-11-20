// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/LabFrameSample.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadSampleEnvironment.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>
#include <filesystem>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;

class LoadSampleEnvironmentTest : public CxxTest::TestSuite {
public:
  static LoadSampleEnvironmentTest *createSuite() { return new LoadSampleEnvironmentTest(); }
  static void destroySuite(LoadSampleEnvironmentTest *suite) { delete suite; }

  void testInit() {

    LoadSampleEnvironment alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void testWithoutSetMaterial() {
    LoadSampleEnvironment alg;
    alg.initialize();
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl").string();
    alg.setProperty("Filename", path);
    alg.setPropertyValue("EnvironmentName", "testName");
    alg.setProperty("SetMaterial", false);
    const int nvectors(2), nbins(10);
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nvectors, nbins);
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", "outputWorkspace");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr ws = alg.getProperty("OutputWorkspace");
    const auto &sample(ws->sample());
    const Geometry::SampleEnvironment environment = sample.getEnvironment();
    const auto &can = environment.getContainer();
    TSM_ASSERT_EQUALS(("expected elements"), environment.nelements(), 1);
    TS_ASSERT(can.hasValidShape());
    TS_ASSERT_EQUALS(environment.name(), "testName");
  }

  void testByDefaultOnlyTheContainerIsOnTheGoniometer() {
    const auto ws = loadCanThenPart("Unset", "Unset");
    const auto &environment = ws->sample().getEnvironment();
    TS_ASSERT(environment.isOnGoniometer(0));
    TS_ASSERT(!environment.isOnGoniometer(1));
  }

  void testOnGoniometerSaysWhetherTheLoadedPartTurns() {
    const auto ws = loadCanThenPart("False", "True");
    const auto &environment = ws->sample().getEnvironment();
    TS_ASSERT(!environment.isOnGoniometer(0));
    TS_ASSERT(environment.isOnGoniometer(1));
  }

  void testAddingToALabFrameViewAddsToTheEnvironmentAsDefined() {
    auto ws = loadPart(WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10), false, "Unset");
    ws->mutableRun().mutableGoniometer().setR(Kernel::Matrix<double>(std::vector<double>{0, -1, 0, 1, 0, 0, 0, 0, 1}));
    // as ws.setSample(ws.getLabFrameSample()) leaves it in Python
    ws->mutableSample() = labFrameSample(*ws);

    ws = loadPart(ws, true, "Unset");

    const auto &environment = ws->sample().getEnvironment();
    TS_ASSERT_EQUALS(environment.nelements(), 2);
    TS_ASSERT_EQUALS(&environment.ownFrame(), &environment);
  }

  void testSetMaterial() {
    LoadSampleEnvironment alg;
    alg.initialize();
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl").string();
    alg.setProperty("Filename", path);
    alg.setPropertyValue("EnvironmentName", "testName");
    alg.setProperty("SetMaterial", true);
    alg.setProperty("AtomicNumber", 1);
    alg.setProperty("MassNumber", 1);
    alg.setProperty("SampleNumberDensity", 1.0);
    const int nvectors(2), nbins(10);
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nvectors, nbins);
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", "outputWorkspace");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr ws = alg.getProperty("OutputWorkspace");
    const auto &sample(ws->sample());
    const Geometry::SampleEnvironment environment = sample.getEnvironment();
    const auto &can = environment.getContainer();
    const auto &material = can.material();
    TSM_ASSERT_EQUALS(("expected elements"), environment.nelements(), 1);
    TS_ASSERT(can.hasValidShape());
    TS_ASSERT_EQUALS(environment.name(), "testName");
    TS_ASSERT_EQUALS(material.numberDensity(), 1);
    TS_ASSERT_EQUALS(material.name(), "");
  }

  void testSetMaterialNumberDensityInFormulaUnits() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl").string();
    constexpr int nvectors{2}, nbins{10};
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nvectors, nbins);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", path))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "outputWorkspace"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("EnvironmentName", "testName"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SetMaterial", true))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ChemicalFormula", "Al2 O3"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SampleNumberDensity", 0.23))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberDensityUnit", "Formula Units"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr ws = alg.getProperty("OutputWorkspace");
    const auto &material = ws->sample().getEnvironment().getContainer().material();
    TS_ASSERT_DELTA(material.numberDensity(), 0.23 * (2. + 3.), 1e-12);
  }

  void test3MF() {
#ifdef ENABLE_LIB3MF
    LoadSampleEnvironment alg;
    alg.initialize();
    std::string path = FileFinder::Instance().getFullPath("box.3mf").string();
    alg.setProperty("Filename", path);
    alg.setPropertyValue("EnvironmentName", "testName");
    alg.setProperty("SetMaterial", false);
    const int nvectors(2), nbins(10);
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nvectors, nbins);
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", "outputWorkspace");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr ws = alg.getProperty("OutputWorkspace");
    const auto &sample(ws->sample());
    const Geometry::SampleEnvironment environment = sample.getEnvironment();
    const auto &can = environment.getContainer();
    const auto &material = can.material();
    TSM_ASSERT_EQUALS(("expected elements"), environment.nelements(), 1);
    TS_ASSERT(can.hasValidShape());
    TS_ASSERT_EQUALS(environment.name(), "testName");
#endif
  }

  void test3MFSampleMeshIsBakedIntoTheLabFrame() {
#ifdef ENABLE_LIB3MF
    // The only fixture with both a <sample> object and environment components, so the only one that
    // reaches bakeGoniometerRotation. box_sample.3mf has no components, and exec() asks the sample
    // for its environment unconditionally afterwards.
    const auto unrotated = load3MFSampleMesh(Kernel::Matrix<double>(3, 3, true));
    const auto rotated = load3MFSampleMesh(ninetyAboutZ());

    // the bake is recorded, so a later reader knows the mesh is already in the lab frame
    TS_ASSERT_EQUALS(unrotated->getAppliedRotation(), Kernel::Matrix<double>(3, 3, true));
    TS_ASSERT_EQUALS(rotated->getAppliedRotation(), ninetyAboutZ());

    // and the vertices actually moved with it
    const auto &before = unrotated->getV3Ds();
    const auto &after = rotated->getV3Ds();
    TS_ASSERT_EQUALS(before.size(), after.size());
    for (size_t i = 0; i < before.size(); ++i) {
      Kernel::V3D expected(before[i]);
      expected.rotate(ninetyAboutZ());
      TS_ASSERT_DELTA(expected.X(), after[i].X(), 1e-12);
      TS_ASSERT_DELTA(expected.Y(), after[i].Y(), 1e-12);
      TS_ASSERT_DELTA(expected.Z(), after[i].Z(), 1e-12);
    }
#endif
  }

private:
  /// A workspace whose environment is a cube loaded as the can, then a cube added to it.
  MatrixWorkspace_sptr loadCanThenPart(const std::string &canOnGoniometer, const std::string &partOnGoniometer) {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);
    return loadPart(loadPart(ws, false, canOnGoniometer), true, partOnGoniometer);
  }

  /// ws with a cube loaded into its environment, as the can or added to it.
  MatrixWorkspace_sptr loadPart(const MatrixWorkspace_sptr &ws, const bool add, const std::string &onGoniometer) {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("Filename", FileFinder::Instance().getFullPath("cubeBin.stl").string());
    alg.setProperty("Add", add);
    alg.setPropertyValue("OnGoniometer", onGoniometer);
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", "unused_for_child");
    alg.execute();
    return alg.getProperty("OutputWorkspace");
  }

#ifdef ENABLE_LIB3MF
  static Kernel::Matrix<double> ninetyAboutZ() {
    return Kernel::Matrix<double>(std::vector<double>{0, -1, 0, 1, 0, 0, 0, 0, 1});
  }

  /// The sample shape left by loading the Pearl 3MF onto a workspace whose run carries goniometerR.
  std::shared_ptr<const MeshObject> load3MFSampleMesh(const Kernel::Matrix<double> &goniometerR) {
    const auto path = std::filesystem::path(Kernel::ConfigService::Instance().getInstrumentDirectory()) /
                      "sampleenvironments" / "ISIS" / "PearlSampleAndEnvironment.3mf";
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("Filename", path.string());
    alg.setPropertyValue("EnvironmentName", "testName");
    alg.setProperty("SetMaterial", false);
    const int nvectors(2), nbins(10);
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nvectors, nbins);
    inputWS->mutableRun().mutableGoniometer().setR(goniometerR);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", "outputWorkspace");
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr ws = alg.getProperty("OutputWorkspace");
    auto mesh = std::dynamic_pointer_cast<const MeshObject>(ws->sample().getShapePtr());
    TS_ASSERT(mesh);
    return mesh;
  }
#endif
};
