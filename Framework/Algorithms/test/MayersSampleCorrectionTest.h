// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MAYERSSAMPLECORRECTIONTEST_H_
#define MANTID_ALGORITHMS_MAYERSSAMPLECORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/SampleCorrections/MayersSampleCorrection.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Material.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::API::IAlgorithm_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::Algorithms::MayersSampleCorrection;

class MayersSampleCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MayersSampleCorrectionTest *createSuite() {
    return new MayersSampleCorrectionTest();
  }
  static void destroySuite(MayersSampleCorrectionTest *suite) { delete suite; }

  // ------------------------ Success cases ----------------------------

  void test_Success_With_Both_Corrections() {
    auto sampleWS = createTestWorkspaceForCorrection();

    IAlgorithm_sptr alg;
    const bool mscatOn(true);
    TS_ASSERT_THROWS_NOTHING(alg = runAlgorithm(sampleWS, mscatOn));
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr corrected = alg->getProperty("OutputWorkspace");
    const auto &tof = corrected->x(0);
    const auto &signal = corrected->y(0);
    const auto &error = corrected->e(0);
    const double delta(1e-06);
    TS_ASSERT_DELTA(99.5, tof.front(), delta);
    TS_ASSERT_DELTA(199.5, tof.back(), delta);

    TS_ASSERT_DELTA(2.307439, signal.front(), delta);
    TS_ASSERT_DELTA(2.314956, signal.back(), delta);

    TS_ASSERT_DELTA(1.631606, error.front(), delta);
    TS_ASSERT_DELTA(1.636921, error.back(), delta);
  }

  void test_Success_With_Just_Absorption_Correction() {
    auto sampleWS = createTestWorkspaceForCorrection();

    IAlgorithm_sptr alg;
    const bool mscatOn(false);
    TS_ASSERT_THROWS_NOTHING(alg = runAlgorithm(sampleWS, mscatOn));
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr corrected = alg->getProperty("OutputWorkspace");
    const auto &tof = corrected->x(0);
    const auto &signal = corrected->y(0);
    const auto &error = corrected->e(0);
    const double delta(1e-06);
    TS_ASSERT_DELTA(99.5, tof.front(), delta);
    TS_ASSERT_DELTA(199.5, tof.back(), delta);

    TS_ASSERT_DELTA(2.3440379, signal.front(), delta);
    TS_ASSERT_DELTA(2.3489418, signal.back(), delta);

    TS_ASSERT_DELTA(1.6574851, error.front(), delta);
    TS_ASSERT_DELTA(1.6609527, error.back(), delta);
  }

  // ------------------------ Failure cases ----------------------------

  void test_Input_Workspace_With_No_Instrument_Throws_Error() {
    auto noInstWS = createTestWorkspaceWithNoInstrument();

    TS_ASSERT_THROWS(runAlgorithm(noInstWS, true),
                     const std::invalid_argument &);
  }

  void test_InputWorkspace_With_No_Sample_Shape_Throws_Error() {
    auto noSampleShapeWS = createTestWorkspaceWithNoSampleShape();

    TS_ASSERT_THROWS(runAlgorithm(noSampleShapeWS, true),
                     const std::invalid_argument &);
  }

private:
  IAlgorithm_sptr runAlgorithm(const MatrixWorkspace_sptr &inputWS,
                               bool mscatOn) {
    auto alg = boost::make_shared<MayersSampleCorrection>();
    // Don't put output in ADS by default
    alg->setChild(true);
    alg->setRethrows(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", inputWS);
    alg->setProperty("MultipleScattering", mscatOn);
    alg->setProperty("MSEvents", 2000);
    alg->setProperty("MSRuns", 5);
    alg->setPropertyValue("OutputWorkspace", "_unused_for_child");
    alg->execute();
    return alg;
  }

  MatrixWorkspace_sptr createTestWorkspaceForCorrection() {
    using ComponentCreationHelper::createCappedCylinder;
    using ComponentCreationHelper::createTestInstrumentCylindrical;
    using Mantid::Geometry::CSGObject;
    using Mantid::Geometry::ObjComponent;
    using Mantid::Kernel::Material;
    using Mantid::Kernel::V3D;
    using Mantid::PhysicalConstants::getNeutronAtom;
    using WorkspaceCreationHelper::create2DWorkspaceBinned;

    const int nhist(1), nbins(100);
    const double xstart(99.5), deltax(1.0);
    // Filled Y with 2.0 and E with sqrt(2)
    auto testWS = create2DWorkspaceBinned(nhist, nbins, xstart, deltax);

    const int nbanks(1);
    // Ids 1->9
    auto testInst = createTestInstrumentCylindrical(nbanks, V3D(0., 0., -14.));
    testWS->setInstrument(testInst);

    // Spectrum-detector mapping
    for (int i = 0; i < nhist; ++i) {
      auto &spectrum = testWS->getSpectrum(i);
      spectrum.clearDetectorIDs();
      spectrum.addDetectorID(i + 1);
    }

    // Sample properties
    const double radius(0.0025), height(0.04);
    auto cylinder =
        createCappedCylinder(radius, height, V3D(), V3D(0., 1., 0.), "sample");
    const double numberDensity(0.07261);
    cylinder->setMaterial(Material("V", getNeutronAtom(23), numberDensity));
    testWS->mutableSample().setShape(cylinder);

    // Move the detector to a known position
    const double twoTheta = 0.10821;
    const double l2 = 2.2;
    auto &detInfo = testWS->mutableDetectorInfo();
    detInfo.setPosition(0, V3D(l2 * sin(twoTheta), 0.0, l2 * cos(twoTheta)));
    return testWS;
  }

  MatrixWorkspace_sptr createTestWorkspaceWithNoInstrument() {
    const int nhist(1), nbins(1);
    const double xstart(99.5), deltax(1.0);
    return WorkspaceCreationHelper::create2DWorkspaceBinned(nhist, nbins,
                                                            xstart, deltax);
  }

  MatrixWorkspace_sptr createTestWorkspaceWithNoSampleShape() {
    using ComponentCreationHelper::createTestInstrumentCylindrical;
    using Mantid::Geometry::CSGObject;
    using Mantid::Geometry::ObjComponent;
    using Mantid::Kernel::V3D;
    using WorkspaceCreationHelper::create2DWorkspaceBinned;

    const int nhist(1), nbins(1);
    const double xstart(99.5), deltax(1.0);
    auto testWS = create2DWorkspaceBinned(nhist, nbins, xstart, deltax);

    const int nbanks(1);
    auto testInst = createTestInstrumentCylindrical(nbanks, V3D(0., 0., -14.));
    testWS->setInstrument(testInst);
    return testWS;
  }
};

#endif /* MANTID_ALGORITHMS_MAYERSSAMPLECORRECTIONTEST_H_ */
