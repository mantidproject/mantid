// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/Axis.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/Muscat.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class MuscatTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuscatTest *createSuite() { return new MuscatTest(); }
  static void destroySuite(MuscatTest *suite) { delete suite; }

  void test_flat_plate_sample_vs_analytic_result() {
    // generate a result corresponding to Figure 4 in the Mancinelli paper (flat
    // plate sample for once scattered neutrons)
    const double WAVELENGTH = 1.;
    Mantid::API::MatrixWorkspace_sptr inputWorkspace =
        WorkspaceCreationHelper::create2DWorkspaceBinned(46, 1, 0.5);
    inputWorkspace->getAxis(0)->unit() =
        UnitFactory::Instance().create("Wavelength");
    Mantid::API::MatrixWorkspace_sptr SofQWorkspace =
        WorkspaceCreationHelper::create2DWorkspace(1, 1);
    SofQWorkspace->mutableY(0)[0] = 1.;
    SofQWorkspace->getAxis(0)->unit() =
        UnitFactory::Instance().create("MomentumTransfer");
    V3D samplePosition(0., 0., 0.);
    V3D sourcePosition(0., 0., -14.);

    Instrument_sptr instrument = std::make_shared<Instrument>();
    instrument->setReferenceFrame(std::make_shared<ReferenceFrame>(
        Mantid::Geometry::Y, Mantid::Geometry::Z, Right, "0,0,0"));

    InstrumentCreationHelper::addSource(instrument, sourcePosition, "source");
    InstrumentCreationHelper::addSample(instrument, samplePosition, "sample");

    for (int i = 0; i < 46; ++i) {
      std::stringstream buffer;
      buffer << "detector_" << i;
      V3D detPos;
      detPos.spherical(1.0, i, 0.);
      InstrumentCreationHelper::addDetector(instrument, detPos, i,
                                            buffer.str());

      // Link it to the workspace
      inputWorkspace->getSpectrum(i).addDetectorID(i);
    }
    inputWorkspace->setInstrument(instrument);

    // create flat plate that is 1mm thick
    const double THICKNESS = 0.001; // metres
    auto flatPlateShape = ComponentCreationHelper::createCuboid(
        (10 * THICKNESS) / 2, (10 * THICKNESS) / 2, THICKNESS / 2, 0,
        V3D{0, 0, 1});
    auto mat = Mantid::Kernel::Material(
        "Ni", Mantid::PhysicalConstants::getNeutronAtom(28, 0), 0.091337537);
    flatPlateShape->setMaterial(mat);
    inputWorkspace->mutableSample().setShape(flatPlateShape);

    Mantid::Algorithms::Muscat alg;
    alg.setAlwaysStoreInADS(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SofqWorkspace", SofQWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronEventsSingle", 10000));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("OutputWorkspace", "MuscatResults"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    using Mantid::API::WorkspaceGroup_sptr;
    Mantid::API::WorkspaceGroup_sptr output =
        alg.getProperty("OutputWorkspace");
    Mantid::API::Workspace_sptr wsPtr = output->getItem("Scatter_1");
    auto singleScatterResult =
        std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr);
    // calculate result analytically
    double totalXSection =
        mat.totalScatterXSection() + mat.absorbXSection(WAVELENGTH);
    double alpha = mat.absorbXSection(WAVELENGTH) / totalXSection;
    double mfp = 0.01 / (mat.numberDensity() * totalXSection);
    double tau = THICKNESS / mfp;
    const int SPECTRUMINDEXTOTEST = 1;
    double secangle =
        1 / cos(inputWorkspace->spectrumInfo().twoTheta(SPECTRUMINDEXTOTEST));
    double analyticResult = (1 - alpha) * (exp(-tau * secangle) - exp(-tau)) /
                            (4 * M_PI * (1 - secangle));
    const double delta(1e-05);
    TS_ASSERT_DELTA(singleScatterResult->y(SPECTRUMINDEXTOTEST)[0],
                    analyticResult, delta);
  }
};