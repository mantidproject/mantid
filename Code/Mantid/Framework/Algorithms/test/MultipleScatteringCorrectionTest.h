#ifndef MANTID_ALGORITHMS_MULTIPLESCATTERINGCORRECTIONTEST_H_
#define MANTID_ALGORITHMS_MULTIPLESCATTERINGCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MultipleScatteringCorrection.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidKernel/Material.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::MultipleScatteringCorrection;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::MatrixWorkspace_sptr;

class MultipleScatteringCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultipleScatteringCorrectionTest *createSuite() {
    return new MultipleScatteringCorrectionTest();
  }
  static void destroySuite(MultipleScatteringCorrectionTest *suite) {
    delete suite;
  }

  // ------------------------ Success cases ----------------------------

  void testValidWorkspaceProducesExpectedValues() {
    using Mantid::Geometry::ComponentHelper::Absolute;
    using Mantid::Geometry::ComponentHelper::moveComponent;
    using Mantid::Kernel::V3D;
    auto sampleWS = createTestWorkspaceForCorrection();
    auto &pmap = sampleWS->instrumentParameters();
    // Move the detector to a known position
    const double twoTheta = 0.10821;
    const double l2 = 2.2;
    auto det = sampleWS->getDetector(0);
    moveComponent(*det, pmap, V3D(l2 * sin(twoTheta), 0.0, l2 * cos(twoTheta)),
                  Absolute);

    IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = runAlgorithm(sampleWS));
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr corrected = alg->getProperty("OutputWorkspace");
    const auto & tof = corrected->readX(0);
    const auto & signal = corrected->readY(0);
    const auto & error = corrected->readE(0);
    const double delta(1e-06);
    TS_ASSERT_DELTA(99.5, tof.front(), delta);
    TS_ASSERT_DELTA(199.5, tof.back(), delta);

    TS_ASSERT_DELTA(-10.406096, signal.front(), delta);
    TS_ASSERT_DELTA(-10.366438, signal.back(), delta);

    TS_ASSERT_DELTA(-7.358221, error.front(), delta);
    TS_ASSERT_DELTA(-7.330179, error.back(), delta);
  }

  // ------------------------ Failure cases ----------------------------

  void testInputWorkspaceWithNoInstrumentThrowsError() {
    auto noInstWS = createTestWorkspaceWithNoInstrument();

    TS_ASSERT_THROWS(runAlgorithm(noInstWS), std::invalid_argument);
  }
  void testInputWorkspaceWithNoSampleShapeThrowsError() {
    auto noSampleShapeWS = createTestWorkspaceWithNoSampleShape();

    TS_ASSERT_THROWS(runAlgorithm(noSampleShapeWS), std::invalid_argument);
  }

private:
  IAlgorithm_sptr runAlgorithm(const MatrixWorkspace_sptr &inputWS) {
    auto alg = boost::make_shared<MultipleScatteringCorrection>();
    // Don't put output in ADS by default
    alg->setChild(true);
    alg->setRethrows(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", inputWS);
    alg->setPropertyValue("OutputWorkspace", "_unused_for_child");
    alg->execute();
    return alg;
  }

  MatrixWorkspace_sptr createTestWorkspaceForCorrection() {
    using ComponentCreationHelper::createCappedCylinder;
    using ComponentCreationHelper::createTestInstrumentCylindrical;
    using Mantid::Geometry::ObjComponent;
    using Mantid::Geometry::Object;
    using Mantid::Kernel::Material;
    using Mantid::Kernel::V3D;
    using Mantid::PhysicalConstants::getNeutronAtom;
    using WorkspaceCreationHelper::Create2DWorkspaceBinned;

    const int nhist(1), nbins(100);
    const double xstart(99.5), deltax(1.0);
    // Filled Y with 2.0 and E with sqrt(2)
    auto testWS = Create2DWorkspaceBinned(nhist, nbins, xstart, deltax);

    const int nbanks(1);
    // Ids 1->9
    auto testInst = createTestInstrumentCylindrical(nbanks, V3D(0., 0., -14.));
    testWS->setInstrument(testInst);

    // Spectrum-detector mapping
    for (int i = 0; i < nhist; ++i) {
      auto *spectrum = testWS->getSpectrum(i);
      spectrum->clearDetectorIDs();
      spectrum->addDetectorID(i + 1);
    }

    // Sample properties - cylinder of vanadium
    const double radius(0.0025), height(0.04);
    auto cylinder =
        createCappedCylinder(radius, height, V3D(), V3D(0., 1., 0.), "sample");
    const double numberDensity(0.07261);
    cylinder->setMaterial(Material("V", getNeutronAtom(23), numberDensity));
    testWS->mutableSample().setShape(*cylinder);

    return testWS;
  }

  MatrixWorkspace_sptr createTestWorkspaceWithNoInstrument() {
    const int nhist(1), nbins(1);
    const double xstart(99.5), deltax(1.0);
    return WorkspaceCreationHelper::Create2DWorkspaceBinned(nhist, nbins,
                                                            xstart, deltax);
  }

  MatrixWorkspace_sptr createTestWorkspaceWithNoSampleShape() {
    using ComponentCreationHelper::createTestInstrumentCylindrical;
    using Mantid::Geometry::ObjComponent;
    using Mantid::Geometry::Object;
    using Mantid::Kernel::V3D;
    using WorkspaceCreationHelper::Create2DWorkspaceBinned;

    const int nhist(1), nbins(1);
    const double xstart(99.5), deltax(1.0);
    auto testWS = Create2DWorkspaceBinned(nhist, nbins, xstart, deltax);

    const int nbanks(1);
    auto testInst = createTestInstrumentCylindrical(nbanks, V3D(0., 0., -14.));
    testWS->setInstrument(testInst);
    return testWS;
  }
};

#endif /* MANTID_ALGORITHMS_MULTIPLESCATTERINGCORRECTIONTEST_H_ */
