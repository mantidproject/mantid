#ifndef MANTID_TEST_GEOMETRY_NEARESTNEIGHBOURS
#define MANTID_TEST_GEOMETRY_NEARESTNEIGHBOURS

#include "MantidAPI/WorkspaceNearestNeighbours.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/FakeObjects.h"
#include <cxxtest/TestSuite.h>
#include <map>

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using Mantid::Kernel::V3D;

/**
* Everything must be in one test or the instrument/detector list goes AWOL.
*/

namespace {
boost::shared_ptr<MatrixWorkspace> makeWorkspace(const specnum_t start,
                                                 const specnum_t end) {
  auto ws = boost::make_shared<WorkspaceTester>();
  ws->initialize(end - start + 1, 2, 1);
  for (specnum_t i = start; i <= end; ++i) {
    ws->getSpectrum(i - start).setSpectrumNo(i);
    ws->getSpectrum(i - start).setDetectorID(i);
  }
  return ws;
}

std::vector<specnum_t> getSpectrumNumbers(const MatrixWorkspace &workspace) {
  std::vector<specnum_t> spectrumNumbers;
  for (size_t i = 0; i < workspace.getNumberHistograms(); ++i)
    spectrumNumbers.push_back(workspace.getSpectrum(i).getSpectrumNo());
  return spectrumNumbers;
}
}

//=====================================================================================
// Functional tests
//=====================================================================================
class WorkspaceNearestNeighboursTest : public CxxTest::TestSuite {
private:
  /// Helper type giving access to protected methods. Makes testing of NN
  /// internals possible.
  class ExposedNearestNeighbours
      : public Mantid::API::WorkspaceNearestNeighbours {
  public:
    ExposedNearestNeighbours(const SpectrumInfo &spectrumInfo,
                             const std::vector<specnum_t> spectrumNumbers,
                             bool ignoreMasked = false)
        : WorkspaceNearestNeighbours(8, spectrumInfo, spectrumNumbers,
                                     ignoreMasked) {}

    // Direct access to intermdiate spectra detectors
    std::vector<size_t> getSpectraDetectors() {
      return WorkspaceNearestNeighbours::getSpectraDetectors();
    }
  };

public:
  void doTestWithNeighbourNumbers(int actualNeighboursNumber,
                                  int expectedNeighboursNumber) {
    const auto ws = makeWorkspace(1, 18);
    ws->setInstrument(
        ComponentCreationHelper::createTestInstrumentCylindrical(2));

    // Create the NearestNeighbours object directly.
    WorkspaceNearestNeighbours nn(actualNeighboursNumber, ws->spectrumInfo(),
                                  getSpectrumNumbers(*ws));

    // Check distances calculated in NearestNeighbours compare with those using
    // getDistance on component
    std::map<specnum_t, V3D> distances = nn.neighbours(14);

    // We should have 8 neighbours when not specifying a range.
    TS_ASSERT_EQUALS(expectedNeighboursNumber, distances.size());
  }

  void testNeighbourFindingWithRadius() {
    const auto ws = makeWorkspace(1, 18);
    ws->setInstrument(
        ComponentCreationHelper::createTestInstrumentCylindrical(2));

    // Create the NearestNeighbours object directly.
    WorkspaceNearestNeighbours nn(8, ws->spectrumInfo(),
                                  getSpectrumNumbers(*ws));

    detid2det_map m_detectors;
    ws->getInstrument()->getDetectors(m_detectors);

    // Need scaling vector since changes to NN ( 22/12/10 )
    Mantid::Geometry::BoundingBox bbox = Mantid::Geometry::BoundingBox();
    boost::shared_ptr<const Detector> det =
        boost::dynamic_pointer_cast<const Detector>(m_detectors[3]);
    det->getBoundingBox(bbox);
    V3D scale((bbox.xMax() - bbox.xMin()), (bbox.yMax() - bbox.yMin()),
              (bbox.zMax() - bbox.zMin()));

    // Check instrument was created to our expectations
    TS_ASSERT_EQUALS(m_detectors.size(), 18);

    // Check distances calculated in NearestNeighbours compare with those using
    // getDistance on component
    std::map<specnum_t, V3D> distances = nn.neighbours(5);
    std::map<specnum_t, V3D>::iterator distIt;

    // We should have 8 neighbours when not specifying a range.
    TS_ASSERT_EQUALS(distances.size(), 8);

    for (distIt = distances.begin(); distIt != distances.end(); ++distIt) {
      double nnDist = distIt->second.norm();
      V3D delta =
          m_detectors[distIt->first]->getPos() - m_detectors[5]->getPos();
      double gmDist = delta.norm();
      TS_ASSERT_DELTA(nnDist, gmDist, 1e-12);
    }

    // Check that the 'radius' option works as expected
    // Lower radius
    distances = nn.neighboursInRadius(14, 0.008);
    TS_ASSERT_EQUALS(distances.size(), 4);

    // Higher than currently computed
    distances = nn.neighboursInRadius(14, 6.0);
    TS_ASSERT_EQUALS(distances.size(), 17);
  }

  void testNeighbourFindingWithNeighbourNumberSpecified() {
    doTestWithNeighbourNumbers(1, 1);
    doTestWithNeighbourNumbers(2, 2);
    doTestWithNeighbourNumbers(3, 3);
  }

  // Let's try it with a rectangular detector.
  void testNeighbours_RectangularDetector() {
    const auto ws = makeWorkspace(256, 767);
    // 2 Rectangular detectors, 16x16
    ws->setInstrument(
        ComponentCreationHelper::createTestInstrumentRectangular(2, 16));

    // Create the NearestNeighbours object directly.
    WorkspaceNearestNeighbours nn(8, ws->spectrumInfo(),
                                  getSpectrumNumbers(*ws));

    const auto &m_instrument = ws->getInstrument();
    // Correct # of detectors
    TS_ASSERT_EQUALS(m_instrument->getDetectorIDs().size(), 512);

    RectangularDetector_const_sptr bank1 =
        boost::dynamic_pointer_cast<const RectangularDetector>(
            m_instrument->getComponentByName("bank1"));
    boost::shared_ptr<const Detector> det = bank1->getAtXY(2, 3);
    TS_ASSERT(det);
    std::map<specnum_t, V3D> nb;

    // Too close!
    specnum_t spec =
        256 + 2 * 16 + 3; // This gives the spectrum number for this detector
    nb = nn.neighboursInRadius(spec, 0.003);
    TS_ASSERT_EQUALS(nb.size(), 0);

    // The ones above below and next to it
    nb = nn.neighboursInRadius(spec, 0.016);
    TS_ASSERT_EQUALS(nb.size(), 4);
  }

  void testIgnoreAndApplyMasking() {
    const auto ws = makeWorkspace(1, 18);
    ws->setInstrument(
        ComponentCreationHelper::createTestInstrumentCylindrical(2));

    // Mask the first 2 detectors
    auto &spectrumInfo = ws->mutableSpectrumInfo();
    spectrumInfo.setMasked(0, true);
    spectrumInfo.setMasked(1, true);

    // Create the NearestNeighbours object directly. Ignore any masking.
    ExposedNearestNeighbours ignoreMaskedNN(ws->spectrumInfo(),
                                            getSpectrumNumbers(*ws), true);
    // Create the NearestNeighbours object directly. Account for any masking.
    ExposedNearestNeighbours accountForMaskedNN(ws->spectrumInfo(),
                                                getSpectrumNumbers(*ws), false);

    size_t sizeWithoutMasked = ignoreMaskedNN.getSpectraDetectors().size();
    size_t sizeWithMasked = accountForMaskedNN.getSpectraDetectors().size();

    TSM_ASSERT_EQUALS("With masked should get 18 spectra back", 18,
                      sizeWithMasked);
    TSM_ASSERT_EQUALS("Without masked should get 16 spectra back", 16,
                      sizeWithoutMasked);
    TSM_ASSERT("Must have less detectors available after applying masking",
               sizeWithoutMasked < sizeWithMasked);
  }
};

//=====================================================================================
// Performance tests
//=====================================================================================
class NearestNeighboursTestPerformance : public CxxTest::TestSuite {

public:
  void testUsingRadius() {
    const auto ws = makeWorkspace(1, 18);
    ws->setInstrument(
        ComponentCreationHelper::createTestInstrumentCylindrical(2));

    // Create the NearestNeighbours object directly.
    WorkspaceNearestNeighbours nn(8, ws->spectrumInfo(),
                                  getSpectrumNumbers(*ws));
    for (size_t i = 0; i < 2000; i++) {
      nn.neighboursInRadius(1, 5.0);
    }
  }

  void testUsingNumberOfNeighbours() {
    const auto ws = makeWorkspace(1, 18);
    ws->setInstrument(
        ComponentCreationHelper::createTestInstrumentCylindrical(2));

    // Create the NearestNeighbours object directly.
    const auto &spectrumInfo = ws->spectrumInfo();
    const auto spectrumNumbers = getSpectrumNumbers(*ws);
    for (size_t i = 0; i < 2000; i++) {
      WorkspaceNearestNeighbours nn(8, spectrumInfo, spectrumNumbers);
      nn.neighbours(1);
    }
  }
};

#endif /* MANTID_TEST_GEOMETRY_NEARESTNEIGHBOURS */
