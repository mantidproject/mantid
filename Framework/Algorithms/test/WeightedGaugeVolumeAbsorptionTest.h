// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/AnyShapeAbsorption.h"
#include "MantidAlgorithms/WeightedGaugeVolumeAbsorption.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/V3D.h"

using Mantid::API::AnalysisDataService;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::Kernel::V3D;

class WeightedGaugeVolumeAbsorptionTest : public CxxTest::TestSuite {
private:
  // Vanadium-like, so there is something to attenuate in without the correction collapsing to zero
  static constexpr const char *ATTEN_XSECTION = "5.08";
  static constexpr const char *SCATTER_XSECTION = "5.1";
  static constexpr const char *NUMBER_DENSITY = "0.07192";

  /// A workspace with a full instrument, wavelength axis, and a cuboid sample centred at the origin.
  MatrixWorkspace_sptr makeWorkspace(const int nSpectra = 2, const double halfWidth = 0.01) {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nSpectra, 10);
    ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    auto shape = ComponentCreationHelper::createCuboid(halfWidth, halfWidth, halfWidth, V3D(0.0, 0.0, 0.0), "sample");
    ws->mutableSample().setShape(shape);
    return ws;
  }

  /// Run the plain AbsorptionCorrection for comparison.
  MatrixWorkspace_sptr runUnweighted(const MatrixWorkspace_sptr &ws, const double elementSize = 3.0) {
    Mantid::Algorithms::AnyShapeAbsorption alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", "unused");
    alg.setPropertyValue("AttenuationXSection", ATTEN_XSECTION);
    alg.setPropertyValue("ScatteringXSection", SCATTER_XSECTION);
    alg.setPropertyValue("SampleNumberDensity", NUMBER_DENSITY);
    alg.setProperty("ElementSize", elementSize);
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    return alg.getProperty("OutputWorkspace");
  }

  std::unique_ptr<Mantid::Algorithms::WeightedGaugeVolumeAbsorption> makeWeighted(const MatrixWorkspace_sptr &ws,
                                                                                  const double elementSize = 3.0) {
    auto alg = std::make_unique<Mantid::Algorithms::WeightedGaugeVolumeAbsorption>();
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", "unused");
    alg->setPropertyValue("AttenuationXSection", ATTEN_XSECTION);
    alg->setPropertyValue("ScatteringXSection", SCATTER_XSECTION);
    alg->setPropertyValue("SampleNumberDensity", NUMBER_DENSITY);
    alg->setProperty("ElementSize", elementSize);
    return alg;
  }

  static std::string cubeXML(const std::string &id, const double side, const V3D &centre) {
    std::ostringstream xml;
    xml << "<cuboid id='" << id << "'>"
        << "<height val='" << side << "' />"
        << "<width val='" << side << "' />"
        << "<depth val='" << side << "' />"
        << "<centre x='" << centre.X() << "' y='" << centre.Y() << "' z='" << centre.Z() << "' />"
        << "</cuboid><algebra val='" << id << "' />";
    return xml.str();
  }

public:
  void testName() {
    Mantid::Algorithms::WeightedGaugeVolumeAbsorption alg;
    TS_ASSERT_EQUALS(alg.name(), "WeightedGaugeVolumeAbsorption");
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void testInit() {
    Mantid::Algorithms::WeightedGaugeVolumeAbsorption alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    // inherited from AbsorptionCorrection, plus its own
    TS_ASSERT(alg.existsProperty("ScatterFrom"));
    TS_ASSERT(alg.existsProperty("NumberOfWavelengthPoints"));
    TS_ASSERT(alg.existsProperty("ElementSize"));
    TS_ASSERT(alg.existsProperty("ElementUnits"));
    TS_ASSERT(alg.existsProperty("ScatteringCentres"));
    TS_ASSERT(alg.existsProperty("IlluminatedVolumeFraction"));
  }

  /// The load-bearing test. With no beam profile and no collimator every weight is unity, so the
  /// weighted quadrature must reproduce the ordinary absorption correction exactly. This is what
  /// establishes that the SRF weighting is layered on top of the existing integral rather than
  /// replacing it with something subtly different.
  void testReducesToAbsorptionCorrectionWithoutBeamOrCollimator() {
    const auto expected = runUnweighted(makeWorkspace());

    auto alg = makeWeighted(makeWorkspace());
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    MatrixWorkspace_sptr actual = alg->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(actual->getNumberHistograms(), expected->getNumberHistograms());
    for (size_t i = 0; i < expected->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < expected->blocksize(); ++j) {
        TS_ASSERT_DELTA(actual->y(i)[j], expected->y(i)[j], 1e-12);
      }
    }
  }

  /// With nothing to attenuate in, the correction is unity everywhere. This pins the normalisation:
  /// the divisor has to be the weighted volume, not the whole sample volume, or the answer would
  /// come out as the illuminated fraction instead of one.
  void testNegligibleAttenuationGivesUnity() {
    auto ws = makeWorkspace();
    Mantid::Algorithms::WeightedGaugeVolumeAbsorption alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", "unused");
    alg.setPropertyValue("AttenuationXSection", "0.0");
    alg.setPropertyValue("ScatteringXSection", "0.0");
    alg.setPropertyValue("SampleNumberDensity", "1e-10");
    alg.setProperty("ElementSize", 3.0);
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr out = alg.getProperty("OutputWorkspace");
    for (size_t i = 0; i < out->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < out->blocksize(); ++j) {
        TS_ASSERT_DELTA(out->y(i)[j], 1.0, 1e-8);
      }
    }
  }

  /// A gauge volume smaller than the sample restricts the integration but must not change what the
  /// correction means: it is still an attenuation factor between zero and one.
  void testGaugeVolumeRestrictsTheIntegrationVolume() {
    auto ws = makeWorkspace();
    ws->mutableRun().addProperty("GaugeVolume", cubeXML("gv", 0.01, V3D(0.0, 0.0, 0.0)));

    auto alg = makeWeighted(ws, 1.0);
    alg->setPropertyValue("IlluminatedVolumeFraction", "fraction");
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr out = alg->getProperty("OutputWorkspace");
    for (size_t i = 0; i < out->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < out->blocksize(); ++j) {
        TS_ASSERT_LESS_THAN(0.0, out->y(i)[j]);
        TS_ASSERT_LESS_THAN_EQUALS(out->y(i)[j], 1.0);
      }
    }

    // The gauge is a 10 mm cube inside a 20 mm cube, so an eighth of the sample volume
    MatrixWorkspace_sptr fraction = alg->getProperty("IlluminatedVolumeFraction");
    TS_ASSERT_EQUALS(fraction->getNumberHistograms(), out->getNumberHistograms());
    for (size_t i = 0; i < fraction->getNumberHistograms(); ++i) {
      TS_ASSERT_DELTA(fraction->y(i)[0], 0.125, 0.02);
    }
  }

  /// A gauge volume centred on the sample is symmetric, so the centre of gravity stays at the
  /// origin however the attenuation weights the elements.
  void testScatteringCentreOfSymmetricGaugeIsAtTheCentre() {
    auto ws = makeWorkspace();
    ws->mutableRun().addProperty("GaugeVolume", cubeXML("gv", 0.01, V3D(0.0, 0.0, 0.0)));

    auto alg = makeWeighted(ws, 1.0);
    alg->setPropertyValue("ScatteringCentres", "centres");
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    ITableWorkspace_sptr centres = alg->getProperty("ScatteringCentres");
    TS_ASSERT(centres);
    TS_ASSERT_LESS_THAN(0, centres->rowCount());
    TS_ASSERT_EQUALS(centres->columnCount(), 5);
    for (size_t row = 0; row < centres->rowCount(); ++row) {
      TS_ASSERT_DELTA(centres->cell<double>(row, 1), 0.0, 1e-3);
      TS_ASSERT_DELTA(centres->cell<double>(row, 2), 0.0, 1e-3);
      TS_ASSERT_DELTA(centres->cell<double>(row, 3), 0.0, 1e-3);
      TS_ASSERT_LESS_THAN(0.0, centres->cell<double>(row, 4));
    }
  }

  /// The point of the algorithm. A gauge volume pushed half out of the sample is truncated by the
  /// surface, so the volume that actually scatters - and its centre of gravity - is displaced back
  /// inside the sample rather than sitting where the gauge was aimed.
  void testPartiallyImmersedGaugeCentreLiesInsideTheSample() {
    auto ws = makeWorkspace();
    // sample spans -0.01..0.01 in x; put the gauge centre on the surface so half of it is outside
    ws->mutableRun().addProperty("GaugeVolume", cubeXML("gv", 0.01, V3D(0.01, 0.0, 0.0)));

    auto alg = makeWeighted(ws, 1.0);
    alg->setPropertyValue("ScatteringCentres", "centres");
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    ITableWorkspace_sptr centres = alg->getProperty("ScatteringCentres");
    TS_ASSERT(centres);
    TS_ASSERT_LESS_THAN(0, centres->rowCount());
    for (size_t row = 0; row < centres->rowCount(); ++row) {
      const double x = centres->cell<double>(row, 1);
      // inside the sample, and pulled back from where the gauge was aimed
      TS_ASSERT_LESS_THAN(x, 0.01);
      TS_ASSERT_LESS_THAN(0.0, x);
    }
  }

  /// The centre of gravity has to come from the elements that survived the gauge intersection, so a
  /// gauge shifted along +x must give a centre shifted along +x relative to a centred one.
  void testShiftingTheGaugeShiftsTheCentre() {
    auto centredWS = makeWorkspace();
    centredWS->mutableRun().addProperty("GaugeVolume", cubeXML("gv", 0.008, V3D(0.0, 0.0, 0.0)));
    auto centred = makeWeighted(centredWS, 1.0);
    centred->setPropertyValue("ScatteringCentres", "c1");
    centred->execute();
    ITableWorkspace_sptr centredTable = centred->getProperty("ScatteringCentres");

    auto shiftedWS = makeWorkspace();
    shiftedWS->mutableRun().addProperty("GaugeVolume", cubeXML("gv", 0.008, V3D(0.005, 0.0, 0.0)));
    auto shifted = makeWeighted(shiftedWS, 1.0);
    shifted->setPropertyValue("ScatteringCentres", "c2");
    shifted->execute();
    ITableWorkspace_sptr shiftedTable = shifted->getProperty("ScatteringCentres");

    TS_ASSERT_EQUALS(centredTable->rowCount(), shiftedTable->rowCount());
    for (size_t row = 0; row < centredTable->rowCount(); ++row) {
      TS_ASSERT_LESS_THAN(centredTable->cell<double>(row, 1), shiftedTable->cell<double>(row, 1));
    }
  }

  /// Weighting by attenuation needs a material; failing early is better than returning zeros.
  void testRejectsSampleWithoutMaterial() {
    auto ws = makeWorkspace();
    Mantid::Algorithms::WeightedGaugeVolumeAbsorption alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", "unused");
    alg.setProperty("ElementSize", 3.0);
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  /// ElementUnits exists so a gauge volume of a few millimetres can be diced without expressing the
  /// element size as a fraction of a millimetre. The same physical size must give the same answer
  /// however it is spelled.
  void testElementUnitsAreEquivalent() {
    auto mmWS = makeWorkspace();
    auto inMillimetres = makeWeighted(mmWS, 2.0);
    inMillimetres->setPropertyValue("ElementUnits", "mm");
    inMillimetres->execute();
    MatrixWorkspace_sptr fromMM = inMillimetres->getProperty("OutputWorkspace");

    auto mWS = makeWorkspace();
    auto inMetres = makeWeighted(mWS, 0.002);
    inMetres->setPropertyValue("ElementUnits", "m");
    inMetres->execute();
    MatrixWorkspace_sptr fromM = inMetres->getProperty("OutputWorkspace");

    for (size_t i = 0; i < fromMM->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < fromMM->blocksize(); ++j) {
        TS_ASSERT_DELTA(fromMM->y(i)[j], fromM->y(i)[j], 1e-12);
      }
    }
  }
};
