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
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadSampleEnvironment.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include <cxxtest/TestSuite.h>

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
};
