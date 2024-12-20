// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/LoadHKL.h"
#include "MantidCrystal/SaveHKL.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/Material.h"
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

class LoadHKLTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    LoadHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void do_test(int numRuns, size_t numBanks, size_t numPeaksPerBank, bool cosines, bool modvecs) {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(4, 10, 1.0);
    PeaksWorkspace_sptr ws(new PeaksWorkspace());
    ws->setInstrument(inst);
    double smu = 0.357;
    double amu = 0.011;
    NeutronAtom neutron(0, 0, 0.0, 0.0, smu, 0.0, smu, amu);
    auto sampleShape = std::make_shared<CSGObject>();
    sampleShape->setMaterial(Material("SetInAnvredCorrection", neutron, 1.0));
    ws->mutableSample().setShape(sampleShape);
    Mantid::Kernel::DblMatrix UBMatrix(
        {-0.009884, -0.016780, 0.115725, 0.112280, 0.002840, 0.011331, -0.005899, 0.081084, 0.023625});
    auto lattice = std::make_unique<OrientedLattice>();
    lattice->setUB(UBMatrix);
    if (modvecs) {
      lattice->setMaxOrder(1);
    }
    ws->mutableSample().setOrientedLattice(std::move(lattice));

    API::Run &mrun = ws->mutableRun();
    mrun.addProperty<double>("Radius", 0.1, true);

    for (int run = 1000; run < numRuns + 1000; run++)
      for (size_t b = 1; b <= numBanks; b++)
        for (size_t i = 0; i < numPeaksPerBank; i++) {
          V3D hkl(static_cast<double>(i), static_cast<double>(i), static_cast<double>(i));
          V3D mnp(static_cast<double>(i), static_cast<double>(i), static_cast<double>(i));
          DblMatrix gon(3, 3, true);
          Peak p(inst, static_cast<detid_t>(b * 100 + i + 1 + i * 10), static_cast<double>(i) * 1.0 + 0.5, hkl, gon);
          p.setIntHKL(hkl);
          p.setIntMNP(mnp);
          p.setRunNumber(run);
          p.setBankName("bank1");
          p.setIntensity(static_cast<double>(i) + 0.1);
          p.setSigmaIntensity(sqrt(static_cast<double>(i) + 0.1));
          p.setBinCount(static_cast<double>(i));
          ws->addPeak(p);
        }

    std::string outfile = "./LoadHKLTest.hkl";
    SaveHKL salg;
    TS_ASSERT_THROWS_NOTHING(salg.initialize())
    TS_ASSERT(salg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(salg.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(salg.setPropertyValue("Filename", outfile));
    TS_ASSERT_THROWS_NOTHING(salg.setProperty("DirectionCosines", cosines));
    TS_ASSERT_THROWS_NOTHING(salg.execute(););
    TS_ASSERT(salg.isExecuted());

    LoadHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", outfile));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "TOPAZ_peaks"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    outfile = alg.getPropertyValue("Filename");
    if (Poco::File(outfile).exists())
      Poco::File(outfile).remove();

    PeaksWorkspace_sptr wsout;
    TS_ASSERT_THROWS_NOTHING(
        wsout = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve("TOPAZ_peaks")));
    TS_ASSERT(wsout);
    if (!wsout)
      return;
    TS_ASSERT_EQUALS(wsout->getNumberPeaks(), 24);

    Peak p = wsout->getPeaks()[0];
    TS_ASSERT_EQUALS(p.getH(), 1);
    TS_ASSERT_EQUALS(p.getK(), 1);
    TS_ASSERT_EQUALS(p.getL(), 1);
    TS_ASSERT_EQUALS(p.getIntHKL(), V3D(1, 1, 1));
    if (modvecs) {
      TS_ASSERT_EQUALS(p.getIntMNP(), V3D(1, 1, 1));
    } else {
      TS_ASSERT_EQUALS(p.getIntMNP(), V3D(0, 0, 0));
    }
    TS_ASSERT_EQUALS(p.getIntensity(), 1.1);
    TS_ASSERT_EQUALS(p.getSigmaIntensity(), 1.05);
    TS_ASSERT_DELTA(p.getWavelength(), 1.5, 1e-4);
    TS_ASSERT_EQUALS(p.getRunNumber(), 1000.);
    TS_ASSERT_DELTA(p.getDSpacing(), 3.5933, 1e-4);
    double radius;
    const auto sampleMaterial = wsout->sample().getMaterial();
    if (sampleMaterial.totalScatterXSection() != 0.0) {
      double rho = sampleMaterial.numberDensity();
      smu = sampleMaterial.totalScatterXSection() * rho;
      amu = sampleMaterial.absorbXSection(NeutronAtom::ReferenceLambda) * rho;
    } else {
      throw std::invalid_argument("Could not retrieve LinearScatteringCoef from material");
    }
    const API::Run &run = wsout->run();
    if (run.hasProperty("Radius")) {
      radius = run.getPropertyValueAsType<double>("Radius");
    } else {
      throw std::invalid_argument("Could not retrieve Radius from run object");
    }

    TS_ASSERT_DELTA(smu, 0.357, 1e-3);
    TS_ASSERT_DELTA(amu, 0.011, 1e-3);
    TS_ASSERT_DELTA(radius, 0.1, 1e-3);
  }

  // Test with a few peaks
  void test_no_cosines_no_modvec() { do_test(2, 4, 4, false, false); }
  void test_cosines_no_modvec() { do_test(2, 4, 4, true, false); }
  void test_no_cosines_modvec() { do_test(2, 4, 4, false, true); }
  void test_cosines_modvec() { do_test(2, 4, 4, true, true); }
};
