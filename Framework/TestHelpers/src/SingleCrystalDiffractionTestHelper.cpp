// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/* Test functions for algorithms for single crystal diffraction
 */

#include "MantidTestHelpers/SingleCrystalDiffractionTestHelper.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/normal_distribution.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <cmath>
#include <random>
#include <tuple>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::Types::Event::TofEvent;

namespace Mantid {
namespace SingleCrystalDiffractionTestHelper {

void WorkspaceBuilder::setNumPixels(const int numPixels) {
  m_numPixels = numPixels;
  m_totalNPixels = numPixels * numPixels;
}

/** Add a peak to the data set to be generated
 *
 * This will create a peak in the event workspace at the given HKL postion.
 *
 * The sigmas parameter is a tuple that controls the distribution of events in
 * the workspace. The first two elements control the x and y variance on the
 * detector bank face. The final element controls the variance in the time of
 * flight spectrum.
 *
 * @param hkl :: the HKL position of the peak
 * @param numEvents :: the number of events to create for the peak
 * @param sigmas :: tuple controlling the distribution of events
 */
void WorkspaceBuilder::addPeakByHKL(
    const V3D &hkl, const int numEvents,
    const std::tuple<double, double, double> &sigmas) {
  m_peakDescriptors.emplace_back(hkl, numEvents, sigmas);
}

/** Build a new set of diffraction data
 *
 * This will use the configured parameters supplied by the user to create a new
 * event workspace with events at the specified HKL peak positions.
 *
 * This will return a tuple where the first element is a matrix workspace
 * pointer that is either an event workspace or a histogram workspace depending
 * on the options set. The second element will be a peaks workspace.
 *
 * @return a tuple containing a matrix workspace and a peaks workspace
 */
std::tuple<MatrixWorkspace_sptr, PeaksWorkspace_sptr>
WorkspaceBuilder::build() {
  createInstrument();
  createPeaksWorkspace();
  createEventWorkspace();
  createNeighbourSearch();
  createPeaks();

  if (m_outputAsHistogram)
    rebinWorkspace();

  return std::make_tuple(m_workspace, m_peaksWorkspace);
}

/** Create a new instrument.
 *
 * This will create a simple rectangular instrument with the requested number
 * of pixels
 *
 */
void WorkspaceBuilder::createInstrument() {
  m_instrument = ComponentCreationHelper::createTestInstrumentRectangular(
      1 /*num_banks*/, m_numPixels /*pixels in each direction yields n by n*/,
      0.01, 1.0);
}

/** Create an empty peaks workspace
 *
 * This will create an empty peaks workspace with a oriented lattice and will
 * also set the instrument.
 *
 */
void WorkspaceBuilder::createPeaksWorkspace() {
  // Create a peaks workspace
  m_peaksWorkspace = boost::make_shared<PeaksWorkspace>();
  // Set the instrument to be the fake rectangular bank above.
  m_peaksWorkspace->setInstrument(m_instrument);
  // Set the oriented lattice for a cubic crystal
  OrientedLattice ol(6, 6, 6, 90, 90, 90);
  ol.setUFromVectors(V3D(6, 0, 0), V3D(0, 6, 0));
  m_peaksWorkspace->mutableSample().setOrientedLattice(&ol);
}

/** Create an empty event workspace
 *
 * This will create an empty event workspace with the instrument attached
 */
void WorkspaceBuilder::createEventWorkspace() {
  // Make an event workspace and add fake peak data
  m_eventWorkspace = boost::make_shared<EventWorkspace>();
  m_eventWorkspace->setInstrument(m_instrument);
  m_eventWorkspace->initialize(m_totalNPixels /*n spectra*/, 3 /* x-size */,
                               3 /* y-size */);
  m_eventWorkspace->getAxis(0)->setUnit("TOF");
  // Give the spectra-detector mapping for all event lists
  for (int i = 0; i < m_totalNPixels; ++i) {
    EventList &el = m_eventWorkspace->getSpectrum(i);
    el.setDetectorID(i + m_totalNPixels);
  }

  // set the output workspace to be the event workspace
  // this may or may not be converted later to a histogram
  m_workspace = m_eventWorkspace;
}

/** Create peaks for all HKL descriptors passed to the builder
 */
void WorkspaceBuilder::createPeaks() {
  int index = 0;
  for (const auto &descriptor : m_peakDescriptors) {
    createPeak(descriptor);
    if (m_useBackground)
      createBackground(index);
    ++index;
  }
}

/** Create a single peak for a given HKL descriptor
 *
 * This will create a Gaussian distributed set of events located at the TOF
 * position of a corresponding HKL value.
 *
 * This distribution of events is controlled by the sigmas parameter of the HKL
 * descriptor which describes the variance in the x, y, and TOF position.
 *
 * @param descriptor a HKLPeakDescriptor which describes the position, intensity
 * and variance in a peak
 */
void WorkspaceBuilder::createPeak(const HKLPeakDescriptor &descriptor) {
  const auto hkl = std::get<0>(descriptor);
  const auto nEvents = std::get<1>(descriptor);
  const auto sigmas = std::get<2>(descriptor);

  // Create the peak and add it to the peaks ws
  const auto peak = std::unique_ptr<Peak>(m_peaksWorkspace->createPeakHKL(hkl));
  m_peaksWorkspace->addPeak(*peak);

  // Get detector ID and TOF position of peak
  const auto detectorId = peak->getDetectorID();
  const auto tofExact = peak->getTOF();
  const auto &info = m_eventWorkspace->detectorInfo();
  const auto detPos = info.position(info.indexOf(detectorId));

  const auto xSigma = std::get<0>(sigmas);
  const auto ySigma = std::get<1>(sigmas);
  const auto tofSigma = std::get<2>(sigmas);

  // distributions for beam divergence and TOF broadening
  Kernel::normal_distribution<> xDist(0, xSigma);
  Kernel::normal_distribution<> yDist(0, ySigma);
  Kernel::normal_distribution<> tofDist(tofExact, tofSigma);

  // add events to the workspace
  for (int i = 0; i < nEvents; ++i) {
    const auto xOffset = xDist(m_generator);
    const auto yOffset = yDist(m_generator);
    const auto tof = tofDist(m_generator);

    const auto pos = V3D(detPos[0] + xOffset, detPos[1] + yOffset, detPos[2]);
    const auto result = m_detectorSearcher->findNearest(
        Eigen::Vector3d(pos[0], pos[1], pos[2]));
    const auto index = std::get<1>(result[0]);
    auto &el = m_eventWorkspace->getSpectrum(index);
    el.addEventQuickly(TofEvent(tof));
  }
}

/** Create a uniform background around each peak in the workspace
 *
 * This will NOT add background to the entire workspace as that would cause the
 * generator to take too long to be used in a unit test. Instead this will
 * generate a uniform background in a "box" around a peak.
 *
 * @param index :: index of the peak to create a uniform background for
 */
void WorkspaceBuilder::createBackground(const int index) {
  const auto &peak = m_peaksWorkspace->getPeak(index);
  const auto detectorId = peak.getDetectorID();
  const auto tofExact = peak.getTOF();
  const auto &info = m_eventWorkspace->detectorInfo();
  const auto detPos = info.position(info.indexOf(detectorId));

  const auto nBackgroundEvents = std::get<0>(m_backgroundParameters);
  const auto backgroundDetSize = std::get<1>(m_backgroundParameters);
  const auto backgroundTOFSize = std::get<2>(m_backgroundParameters);

  std::uniform_real_distribution<> backgroundXDist(-backgroundDetSize,
                                                   backgroundDetSize);
  std::uniform_real_distribution<> backgroundYDist(-backgroundDetSize,
                                                   backgroundDetSize);
  std::uniform_real_distribution<> backgroundTOFDist(
      tofExact - backgroundTOFSize, tofExact + backgroundTOFSize);

  for (int i = 0; i < nBackgroundEvents; ++i) {
    const auto xOffset = backgroundXDist(m_generator);
    const auto yOffset = backgroundYDist(m_generator);
    const auto tof = backgroundTOFDist(m_generator);

    const auto pos = V3D(detPos[0] + xOffset, detPos[1] + yOffset, detPos[2]);
    const auto result = m_detectorSearcher->findNearest(
        Eigen::Vector3d(pos[0], pos[1], pos[2]));
    const auto index = std::get<1>(result[0]);

    auto &el = m_eventWorkspace->getSpectrum(index);
    el.addEventQuickly(TofEvent(tof));
  }
}

/** Create a KD-Tree of detector positions that can be used to find the closest
 * detector to a given event position
 */
void WorkspaceBuilder::createNeighbourSearch() {
  const auto &info = m_eventWorkspace->detectorInfo();
  std::vector<Eigen::Vector3d> points;
  for (size_t i = 0; i < info.size(); ++i) {
    const auto pos = info.position(i);
    points.emplace_back(pos[0], pos[1], pos[2]);
  }
  m_detectorSearcher = std::make_unique<NearestNeighbours<3>>(points);
}

/** Rebin the event workspace using the parameters provided
 */
void WorkspaceBuilder::rebinWorkspace() {
  auto rebinAlg = AlgorithmManager::Instance().createUnmanaged("Rebin");
  rebinAlg->setChild(true);
  rebinAlg->initialize();
  rebinAlg->setProperty("InputWorkspace", m_eventWorkspace);
  rebinAlg->setProperty("Params", m_rebinParams);
  rebinAlg->setProperty("PreserveEvents", false); // Make a histo workspace
  rebinAlg->setPropertyValue("OutputWorkspace", "__SXD_test_helper_rebin");
  rebinAlg->execute();
  m_workspace = rebinAlg->getProperty("OutputWorkspace");
}
} // namespace SingleCrystalDiffractionTestHelper
} // namespace Mantid
