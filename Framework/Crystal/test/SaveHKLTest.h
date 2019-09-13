// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_SAVEHKLTEST_H_
#define MANTID_CRYSTAL_SAVEHKLTEST_H_

#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/SaveHKL.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
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
  void test_Init() {
    SaveHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_empty_workspace() {
    const bool expectEmptyFile{true};
    assertFileContent(assertSaveExec(createTestPeaksWorkspace(0, 0, 0)),
                      expectEmptyFile);
  }

  void test_save_using_sample_shape_object_and_radius_log_entry() {
    auto ws = createTestPeaksWorkspace(4, 4, 2);
    const double smu{0.357}, amu{0.011};
    NeutronAtom neutron(0, 0, 0.0, 0.0, smu, 0.0, smu, amu);
    auto sampleShape = boost::make_shared<CSGObject>();
    sampleShape->setMaterial(Material("SetInSaveHKLTest", neutron, 1.0));
    ws->mutableSample().setShape(sampleShape);
    API::Run &mrun = ws->mutableRun();
    mrun.addProperty<double>("Radius", 0.1, true);

    const bool expectEmptyFile{false};
    const double expectedTbar{0.1591}, expectedTransmission{0.9434};

    assertFileContent(assertSaveExec(ws), expectEmptyFile, expectedTbar,
                      expectedTransmission);
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

    assertFileContent(assertSaveExec(ws), expectEmptyFile, expectedTbar,
                      expectedTransmission);
  }

  void test_save_using_sample_properties_on_algorithm() {
    auto ws = createTestPeaksWorkspace(4, 4, 2);
    const double smu{0.357}, amu{0.011};
    const double radius{0.1};

    const bool expectEmptyFile{false};
    const double expectedTbar{0.1591}, expectedTransmission{0.9434};
    assertFileContent(assertSaveExec(ws, radius, smu, amu), expectEmptyFile,
                      expectedTbar, expectedTransmission);
  }

private:
  PeaksWorkspace_sptr createTestPeaksWorkspace(int numRuns, size_t numBanks,
                                               size_t numPeaksPerBank) {
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular(4, 10, 1.0);
    auto ws = boost::make_shared<PeaksWorkspace>();
    ws->setInstrument(inst);

    for (int run = 1000; run < numRuns + 1000; run++) {
      for (size_t b = 1; b <= numBanks; b++) {
        for (size_t i = 0; i < numPeaksPerBank; i++) {
          V3D hkl(static_cast<double>(i), static_cast<double>(i),
                  static_cast<double>(i));
          DblMatrix gon(3, 3, true);
          Peak p(inst, static_cast<detid_t>(b * 100 + i + 1 + i * 10),
                 static_cast<double>(i) * 1.0 + 0.5, hkl, gon);
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

  std::string assertSaveExec(const PeaksWorkspace_sptr &peaksWS,
                             const double radius = -1.0,
                             const double smu = -1.0, const double amu = -1.0) {

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

  void assertFileContent(const std::string &filepath, const bool expectedEmpty,
                         const double expectedTbar = -1.0,
                         const double expectedTransmission = -1.0) {

    if (expectedEmpty)
      return;

    std::ifstream in(filepath.c_str());
    double d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14;
    in >> d1 >> d2 >> d3 >> d4 >> d5 >> d6 >> d7 >> d8 >> d9 >> d10 >> d11 >>
        d12 >> d13 >> d14;
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

#endif /* MANTID_CRYSTAL_SAVEHKLTEST_H_ */
