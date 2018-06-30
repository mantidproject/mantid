#include "MantidVatesAPI/ConcretePeaksPresenterVsi.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidDataObjects/NoShape.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidVatesAPI/ViewFrustum.h"
namespace Mantid {
namespace VATES {

using Mantid::DataObjects::PeaksWorkspace;

/**
 * Constructor
 * @param peaksWorkspace The peaks workspace.
 * @param frustum The view frustum
 * @param frame The coordinate frame
 */
ConcretePeaksPresenterVsi::ConcretePeaksPresenterVsi(
    Mantid::API::IPeaksWorkspace_sptr peaksWorkspace,
    ViewFrustum_const_sptr frustum, std::string frame)
    : m_viewableRegion(frustum), m_peaksWorkspace(peaksWorkspace),
      m_frame(frame) {}

/// Destructor
ConcretePeaksPresenterVsi::~ConcretePeaksPresenterVsi() {}

/**
 * Update the view frustum
 * @param frustum The view frustum.
 */
void ConcretePeaksPresenterVsi::updateViewFrustum(
    ViewFrustum_const_sptr frustum) {
  m_viewableRegion = frustum;
}

/**
 * Get the viewable peaks. Essentially copied from the slice viewer.
 * @returns A vector indicating which of the peaks are viewable.
 */
std::vector<bool> ConcretePeaksPresenterVsi::getViewablePeaks() const {
  // Need to apply a transform.
  // Don't bother to find peaks in the region if there are no peaks to find.
  Mantid::API::ITableWorkspace_sptr outTable;

  if (this->m_peaksWorkspace->getNumberPeaks() >= 1) {
    double effectiveRadius = 1e-2;
    std::string viewable = m_viewableRegion->toExtentsAsString();
    auto peaksWS =
        boost::dynamic_pointer_cast<PeaksWorkspace>(m_peaksWorkspace);

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("PeaksInRegion");
    alg->setChild(true);
    alg->setRethrows(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", peaksWS);
    alg->setProperty("OutputWorkspace",
                     peaksWS->getName() + "_peaks_in_region");
    alg->setProperty("Extents", viewable);
    alg->setProperty("CheckPeakExtents", true);
    alg->setProperty("PeakRadius", effectiveRadius);
    alg->setPropertyValue("CoordinateFrame", m_frame);
    alg->execute();
    outTable = alg->getProperty("OutputWorkspace");
    std::vector<bool> viewablePeaks(outTable->rowCount());
    for (size_t i = 0; i < outTable->rowCount(); ++i) {
      viewablePeaks[i] = outTable->cell<Mantid::API::Boolean>(i, 1);
    }
    m_viewablePeaks = viewablePeaks;
  } else {
    // No peaks will be viewable
    m_viewablePeaks = std::vector<bool>();
  }

  return m_viewablePeaks;
}

/**
 * Get the underlying peaks workspace
 * @returns A pointer to the underlying peaks workspace.
 */
Mantid::API::IPeaksWorkspace_sptr
ConcretePeaksPresenterVsi::getPeaksWorkspace() const {
  return m_peaksWorkspace;
}

/**
 * Get the frame
 * @returns The frame.
 */
std::string ConcretePeaksPresenterVsi::getFrame() const { return m_frame; }

/**
 * Get the name of the underlying peaks workspace.
 * @returns The name of the peaks workspace.
 */
std::string ConcretePeaksPresenterVsi::getPeaksWorkspaceName() const {
  return m_peaksWorkspace->getName();
}

/**
 * Get the peaks info for a single peak, defined by the row in the peaks table.
 * @param peaksWorkspace A pointer to a peaks workspace.
 * @param row The row in the peaks table.
 * @param position A reference which holds the position of the peak.
 * @param radius A reference which holds the radius of the peak.
 * @param specialCoordinateSystem The coordinate system.
 */
void ConcretePeaksPresenterVsi::getPeaksInfo(
    Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, int row,
    Mantid::Kernel::V3D &position, double &radius,
    Mantid::Kernel::SpecialCoordinateSystem specialCoordinateSystem) const {

  switch (specialCoordinateSystem) {
  case (Mantid::Kernel::SpecialCoordinateSystem::QLab):
    position = peaksWorkspace->getPeak(row).getQLabFrame();
    break;
  case (Mantid::Kernel::SpecialCoordinateSystem::QSample):
    position = peaksWorkspace->getPeak(row).getQSampleFrame();
    break;
  case (Mantid::Kernel::SpecialCoordinateSystem::HKL):
    position = peaksWorkspace->getPeak(row).getHKL();
    break;
  default:
    throw std::invalid_argument("The coordinate system is invalid.\n");
  }

  // Peak radius
  Mantid::Geometry::PeakShape_sptr shape(
      peaksWorkspace->getPeakPtr(row)->getPeakShape().clone());
  radius = getMaxRadius(*shape);
}

/**
 * Get the maximal radius
 * @param shape The shape of a peak.
 * @returns The maximal radius of the peak.
 */
double ConcretePeaksPresenterVsi::getMaxRadius(
    const Mantid::Geometry::PeakShape &shape) const {
  const double defaultRadius = 1.0;

  boost::optional<double> radius =
      shape.radius(Mantid::Geometry::PeakShape::Radius);
  if (radius) {
    return radius.get();
  } else {
    return defaultRadius;
  }
}

/**
 * Sorts the peak workspace by a specified column name in ascending or
 * descending order.
 * @param byColumnName The column by which the workspace is to be sorted.
 * @param ascending If the workspace is to be sorted in a ascending or
 * descending manner.
 */
void ConcretePeaksPresenterVsi::sortPeaksWorkspace(
    const std::string &byColumnName, const bool ascending) {
  auto peaksWS = boost::dynamic_pointer_cast<PeaksWorkspace>(m_peaksWorkspace);

  // Sort the Peaks in-place.
  Mantid::API::IAlgorithm_sptr alg =
      Mantid::API::AlgorithmManager::Instance().create("SortPeaksWorkspace");
  alg->setChild(true);
  alg->setRethrows(true);
  alg->initialize();
  alg->setProperty("InputWorkspace", peaksWS);
  alg->setPropertyValue("OutputWorkspace", "SortedPeaksWorkspace");
  alg->setProperty("OutputWorkspace", peaksWS);
  alg->setProperty("SortAscending", ascending);
  alg->setPropertyValue("ColumnNameToSortBy", byColumnName);
  alg->execute();
}
} // namespace VATES
} // namespace Mantid
