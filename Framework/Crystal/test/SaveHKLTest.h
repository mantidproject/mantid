// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/SaveHKL.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Timer.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>
#include <fstream>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::PhysicalConstants;

class SaveHKLTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveHKLTest *createSuite() { return new SaveHKLTest(); }
  static void destroySuite(SaveHKLTest *suite) { delete suite; }

  // create FrameworkManager so that MantidAlgorithms.dll is loaded and
  // AddAbsorptionWeightedPathLengths algorithm is registered
  SaveHKLTest() { FrameworkManager::Instance(); }

  void test_Init() {
    SaveHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_empty_workspace() {
    const bool expectEmptyFile{true};
    assertFileContent(assertSaveExec(createTestPeaksWorkspace(0, 0, 0)), expectEmptyFile);
  }

  void test_save_using_sample_shape_object_and_radius_log_entry() {
    auto ws = createTestPeaksWorkspace(4, 4, 2);
    const double smu{0.357}, amu{0.011};
    NeutronAtom neutron(0, 0, 0.0, 0.0, smu, 0.0, smu, amu);
    auto sampleShape = std::make_shared<CSGObject>();
    sampleShape->setMaterial(Material("SetInSaveHKLTest", neutron, 1.0));
    ws->mutableSample().setShape(sampleShape);
    API::Run &mrun = ws->mutableRun();
    mrun.addProperty<double>("Radius", 0.1, true);

    const bool expectEmptyFile{false};
    const double expectedTbar{0.14888}, expectedTransmission{0.9469};

    assertFileContent(assertSaveExec(ws), expectEmptyFile, expectedTbar, expectedTransmission);
  }

  void test_save_using_sample_shape_object_and_sample_material() {
    auto ws = createTestPeaksWorkspace(4, 4, 2);
    const double smu{0.357}, amu{0.011};
    const double radius{0.1};
    NeutronAtom neutron(0, 0, 0.0, 0.0, smu, 0.0, smu, amu);
    auto sampleShape = ShapeFactory::createSphere(V3D(), radius * 0.01);
    sampleShape->setMaterial(Material("SetInSaveHKLTest", neutron, 1.0));
    ws->mutableSample().setShape(sampleShape);

    const bool expectEmptyFile{false};
    const double expectedTbar{0.2}, expectedTransmission{0.9294};

    assertFileContent(assertSaveExec(ws), expectEmptyFile, expectedTbar, expectedTransmission);
  }

  void test_save_using_sample_properties_on_algorithm() {
    auto ws = createTestPeaksWorkspace(4, 4, 2);
    const double smu{0.357}, amu{0.011};
    const double radius{0.1};

    const bool expectEmptyFile{false};
    const double expectedTbar{0.14888}, expectedTransmission{0.9469};
    assertFileContent(assertSaveExec(ws, radius, smu, amu), expectEmptyFile, expectedTbar, expectedTransmission);
  }

  void test_save_with_direction_cosines_DEMAND() {
    // This test compares the direction cosines calculated with
    // SaveHKL to values from other software
    auto ws = std::make_shared<PeaksWorkspace>();

    // create dummy workspace to use with LoadInstrument
    auto dummyWS = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    auto expinfo = std::dynamic_pointer_cast<ExperimentInfo>(dummyWS);
    auto &run = expinfo->mutableRun();
    Types::Core::DateAndTime startTime(0);
    auto twotheta = std::make_unique<TimeSeriesProperty<double>>("2theta");
    twotheta->addValue(startTime, 58.0595);
    run.addLogData(std::move(twotheta));
    auto det_trans = std::make_unique<TimeSeriesProperty<double>>("det_trans");
    det_trans->addValue(startTime, 399.9955);
    run.addLogData(std::move(det_trans));

    AnalysisDataService::Instance().addOrReplace("dummy_direction_cosine_test", dummyWS);
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    loader.setPropertyValue("InstrumentName", "HB3A");
    loader.setPropertyValue("Workspace", "dummy_direction_cosine_test");
    loader.setPropertyValue("RewriteSpectraMap", "False");
    loader.execute();

    Goniometer gon;
    gon.pushAxis("omega", 0, 1, 0, 29.0295, -1);
    gon.pushAxis("chi", 0, 0, 1, 15.1168, -1);
    gon.pushAxis("phi", 0, 1, 0, 4.7395, -1);
    run.setGoniometer(gon, false);

    ws->setInstrument(dummyWS->getInstrument());
    ws->mutableRun().setGoniometer(run.getGoniometer().getR(), false);

    Mantid::Kernel::DblMatrix UBMatrix(
        {-0.009884, -0.016780, 0.115725, 0.112280, 0.002840, 0.011331, -0.005899, 0.081084, 0.023625});
    auto lattice = std::make_unique<OrientedLattice>();
    lattice->setUB(UBMatrix);
    ExperimentInfo_sptr ei = std::dynamic_pointer_cast<ExperimentInfo>(ws);
    ei->mutableSample().setOrientedLattice(std::move(lattice));

    V3D hkl(1, -2, 5);
    V3D qSampleFrame = UBMatrix * hkl * 2 * M_PI;
    Peak p(ws->getInstrument(), qSampleFrame, run.getGoniometer().getR());
    p.setHKL(hkl);
    p.setIntHKL(hkl);
    p.setRunNumber(1000);
    p.setBankName("bank1");
    p.setIntensity(1.0);
    p.setSigmaIntensity(1.0);
    p.setBinCount(1.0);
    ws->addPeak(p);

    std::string outfile = "./SaveHKLTest_direction_cosine.hkl";
    SaveHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", outfile));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DirectionCosines", true));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Get the file
    outfile = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(outfile).exists());

    std::ifstream in(outfile.c_str());
    double h, k, l, fsw, sigmafsq, histnum, wl, tbar, dir_cos_1_x, dir_cos_2_x, dir_cos_1_y, dir_cos_2_y, dir_cos_1_z,
        dir_cos_2_z, dsp, col, row;
    in >> h >> k >> l >> fsw >> sigmafsq >> histnum >> wl >> tbar >> dir_cos_1_x >> dir_cos_2_x >> dir_cos_1_y >>
        dir_cos_2_y >> dir_cos_1_z >> dir_cos_2_z >> dsp >> col >> row;
    in.close();

    TS_ASSERT_EQUALS(h, -1);
    TS_ASSERT_EQUALS(k, 2);
    TS_ASSERT_EQUALS(l, -5);
    TS_ASSERT_EQUALS(fsw, 1);
    TS_ASSERT_EQUALS(sigmafsq, 1);
    TS_ASSERT_EQUALS(histnum, 1);
    TS_ASSERT_DELTA(wl, 1.55025, 1e-4);
    TS_ASSERT_EQUALS(tbar, 0);
    // compare to direction cosines produced with other software
    TS_ASSERT_DELTA(dir_cos_1_x, -0.03516, 1e-3);
    TS_ASSERT_DELTA(dir_cos_1_y, -0.71007, 1e-3);
    TS_ASSERT_DELTA(dir_cos_1_z, -0.70368, 1e-3);
    TS_ASSERT_DELTA(dir_cos_2_x, -0.13889, 1e-3);
    TS_ASSERT_DELTA(dir_cos_2_y, 0.96627, 1e-3);
    TS_ASSERT_DELTA(dir_cos_2_z, -0.21574, 1e-3);
    TS_ASSERT_EQUALS(dsp, 1000);
    TS_ASSERT_EQUALS(col, 0);
    TS_ASSERT_EQUALS(row, 0);

    // compare direction cosine to direct calculation
    auto RU = ws->run().getGoniometer().getR() * ws->sample().getOrientedLattice().getU();
    RU.Transpose();
    V3D dir_cos_1 = RU * V3D(0, 0, -1);
    auto peaks_pos = ws->getPeak(0).getDetPos();
    peaks_pos.normalize();
    V3D dir_cos_2 = RU * peaks_pos;
    TS_ASSERT_DELTA(dir_cos_1_x, dir_cos_1[0], 1e-3);
    TS_ASSERT_DELTA(dir_cos_1_y, dir_cos_1[1], 1e-3);
    TS_ASSERT_DELTA(dir_cos_1_z, dir_cos_1[2], 1e-3);
    TS_ASSERT_DELTA(dir_cos_2_x, dir_cos_2[0], 1e-3);
    TS_ASSERT_DELTA(dir_cos_2_y, dir_cos_2[1], 1e-3);
    TS_ASSERT_DELTA(dir_cos_2_z, dir_cos_2[2], 1e-3);

    if (Poco::File(outfile).exists())
      Poco::File(outfile).remove();
  }

private:
  PeaksWorkspace_sptr createTestPeaksWorkspace(int numRuns, size_t numBanks, size_t numPeaksPerBank) {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(4, 10, 1.0);
    auto ws = std::make_shared<PeaksWorkspace>();
    ws->setInstrument(inst);

    for (int run = 1000; run < numRuns + 1000; run++) {
      for (size_t b = 1; b <= numBanks; b++) {
        for (size_t i = 0; i < numPeaksPerBank; i++) {
          V3D hkl(static_cast<double>(i), static_cast<double>(i), static_cast<double>(i));
          DblMatrix gon(3, 3, true);
          Peak p(inst, static_cast<detid_t>(b * 100 + i + 1 + i * 10), static_cast<double>(i) * 1.0 + 0.5, hkl, gon);
          p.setRunNumber(run);
          p.setBankName("bank1");
          p.setIntensity(static_cast<double>(i) + 0.1);
          p.setSigmaIntensity(sqrt(static_cast<double>(i)));
          p.setBinCount(static_cast<double>(i));
          ws->addPeak(p);
        }
      }
    }
    return ws;
  }

  std::string assertSaveExec(const PeaksWorkspace_sptr &peaksWS, const double radius = -1.0, const double smu = -1.0,
                             const double amu = -1.0) {

    std::string outfile = "./SaveHKLTest.hkl";
    SaveHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", peaksWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", outfile));
    if (radius > 0.0)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("Radius", radius));
    if (smu > 0.0)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("LinearScatteringCoef", smu));
    if (amu > 0.0)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("LinearAbsorptionCoef", amu));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Get the file
    outfile = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(outfile).exists());
    return outfile;
  }

  void assertFileContent(const std::string &filepath, const bool expectedEmpty, const double expectedTbar = -1.0,
                         const double expectedTransmission = -1.0) {

    if (expectedEmpty)
      return;

    std::ifstream in(filepath.c_str());
    double d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14;
    in >> d1 >> d2 >> d3 >> d4 >> d5 >> d6 >> d7 >> d8 >> d9 >> d10 >> d11 >> d12 >> d13 >> d14;
    TS_ASSERT_EQUALS(d1, -1);
    TS_ASSERT_EQUALS(d2, -1);
    TS_ASSERT_EQUALS(d3, -1);
    TS_ASSERT_EQUALS(d4, 1.1);
    TS_ASSERT_EQUALS(d5, 1);
    TS_ASSERT_EQUALS(d6, 1);
    TS_ASSERT_EQUALS(d7, 1.5);
    TS_ASSERT_EQUALS(d8, expectedTbar);
    TS_ASSERT_EQUALS(d9, 1000.);
    TS_ASSERT_EQUALS(d10, 2);
    TS_ASSERT_EQUALS(d11, expectedTransmission);
    TS_ASSERT_EQUALS(d12, 1);
    TS_ASSERT_DELTA(d13, 0.4205, 1e-4);
    TS_ASSERT_EQUALS(d14, 3.5933);
    in.close();

    if (Poco::File(filepath).exists())
      Poco::File(filepath).remove();
  }
};
