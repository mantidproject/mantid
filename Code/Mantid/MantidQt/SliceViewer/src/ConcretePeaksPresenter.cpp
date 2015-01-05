#include "MantidQtSliceViewer/ConcretePeaksPresenter.h"
#include "MantidQtSliceViewer/UpdateableOnDemand.h"
#include "MantidQtSliceViewer/ZoomableOnDemand.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Logger.h"
#include <boost/scoped_ptr.hpp>
#include <boost/regex.hpp>

using namespace Mantid::API;
using Mantid::Geometry::IMDDimension_const_sptr;

namespace MantidQt {
namespace SliceViewer {
namespace {
/// static logger
Mantid::Kernel::Logger g_log("PeaksPresenter");
}

/**
 * Convert from a SpecialCoordinateSystem enum to a correpsonding enum name.
 * @param coordSystem : enum option
 * @return coordinate system as a string
 */
std::string
coordinateToString(Mantid::API::SpecialCoordinateSystem coordSystem) {
  switch (coordSystem) {
  case Mantid::API::QLab:
    return "QLab";
  case Mantid::API::QSample:
    return "QSample";
  case Mantid::API::HKL:
    return "HKL";
  default:
    return "Unknown";
  }
}

/**
 * Produce the views for the internally held peaks workspace.
 * Indexes to peaks in the peaks workspace are used to reference the
 * corresponding PeaksOverlayView, so when the PeaksWorkspace is reordered,
 * All the views must be recreated.
 */
void ConcretePeaksPresenter::produceViews() {
  m_viewPeaks = m_viewFactory->createView(m_transform);
}

/**
 * Check the work-space compatibilities.
 *
 * @param mdWS : MDWorkspace currently plotted.
 */
void ConcretePeaksPresenter::checkWorkspaceCompatibilities(
    boost::shared_ptr<Mantid::API::MDGeometry> mdWS) {
  if (auto imdWS =
          boost::dynamic_pointer_cast<Mantid::API::IMDWorkspace>(mdWS)) {
    const SpecialCoordinateSystem coordSystMD =
        imdWS->getSpecialCoordinateSystem();
    const SpecialCoordinateSystem coordSystDim =
        m_transform->getCoordinateSystem();
    const SpecialCoordinateSystem coordSystPK =
        m_peaksWS->getSpecialCoordinateSystem();
    // Check that the MDWorkspace is self-consistent.
    if (coordSystMD != coordSystDim) {
      std::stringstream ss;
      ss << std::endl;
      ss << "According to the dimension names in your MDWorkspace, this "
            "work-space is determined to be in: ";
      ss << m_transform->getFriendlyName() << " in the PeaksViewer. ";
      ss << "However, the MDWorkspace has properties indicating that it's "
            "coordinates are in: " << coordinateToString(coordSystMD);
      ss << " To resolve the conflict, the MDWorkspace will be treated as "
            "though it has coordinates in: " << m_transform->getFriendlyName();
      g_log.notice(ss.str());
    }
    // If the peaks work-space has been integrated. check cross-work-space
    // compatibility.
    if (coordSystDim != coordSystPK && m_peaksWS->hasIntegratedPeaks()) {
      std::stringstream ss;
      ss << std::endl;
      ss << "You appear to be plotting your PeaksWorkspace in a different "
            "coordinate system from the one in which integration was "
            "performed. ";
      ss << "This will distort the integrated peak shape on the PeaksViewer. ";
      ss << "PeaksWorkspace was integrated against a MDWorkspace in the "
            "coordinate system: "
         << coordinateToString(m_peaksWS->getSpecialCoordinateSystem());
      ss << "MDWorkspace is displayed in coordinate system: "
         << m_transform->getFriendlyName();
      g_log.notice(ss.str());
    }
  }
}

/**
 Constructor.

 1 First check that the arguments provided are valid.
 2 Then iterate over the MODEL and use it to construct VIEWs via the factory.
 3 A collection of views is stored internally

 @param viewFactory : View Factory (THE VIEW via factory)
 @param peaksWS : IPeaksWorkspace to visualise (THE MODEL)
 @param mdWS : IMDWorkspace also being visualised (THE MODEL)
 @param transformFactory : Peak Transformation Factory. This is about
 interpreting the MODEL.
 */
ConcretePeaksPresenter::ConcretePeaksPresenter(
    PeakOverlayViewFactory_sptr viewFactory, IPeaksWorkspace_sptr peaksWS,
    boost::shared_ptr<MDGeometry> mdWS,
    Mantid::API::PeakTransformFactory_sptr transformFactory)
    : m_viewFactory(viewFactory), m_peaksWS(peaksWS),
      m_transformFactory(transformFactory),
      m_transform(transformFactory->createDefaultTransform()), m_slicePoint(),
      m_owningPresenter(NULL), m_isHidden(false) {
  // Check that the workspaces appear to be compatible. Log if otherwise.
  checkWorkspaceCompatibilities(mdWS);

  this->initialize();
}

/**
 * reInitialize the setup. Reuse the same presenter around a new peaks
 * workspace.
 * @param peaksWS : re-initialize around a peaks workspace
 */
void ConcretePeaksPresenter::reInitialize(IPeaksWorkspace_sptr peaksWS) {
  m_peaksWS = peaksWS;

  // The view factory also needs an updated reference.
  m_viewFactory->swapPeaksWorkspace(peaksWS);

  // Make and register each peak widget.
  produceViews();

  doFindPeaksInRegion();
}

/**
 * @brief initialize inner components. Produces the views.
 */
void ConcretePeaksPresenter::initialize() {
  const bool transformSucceeded = this->configureMappingTransform();

  // Make and register each peak widget.
  produceViews();

  if (!transformSucceeded) {
    hideAll();
  }
}

/**
 Force each view to re-paint.
 */
void ConcretePeaksPresenter::update() { m_viewPeaks->updateView(); }

/**
 * Find the peaks in the region.
 * Update the view with all those peaks that could be viewable.
 *
 * Also takes into account the peak radius, so needs to be re-executed when the
 *user changes the effective radius of the peaks markers.
 */
void ConcretePeaksPresenter::doFindPeaksInRegion() {
  PeakBoundingBox transformedViewableRegion =
      m_slicePoint.makeSliceBox(1e-6); // TODO, could actually be calculated as
                                       // a single plane with z = 0 thickness.
  transformedViewableRegion.transformBox(m_transform);

  double effectiveRadius =
      m_viewPeaks->getRadius(); // Effective radius of each peak representation.

  Mantid::API::IPeaksWorkspace_sptr peaksWS =
      boost::const_pointer_cast<Mantid::API::IPeaksWorkspace>(this->m_peaksWS);

  Mantid::API::IAlgorithm_sptr alg =
      AlgorithmManager::Instance().create("PeaksInRegion");
  alg->setChild(true);
  alg->setRethrows(true);
  alg->initialize();
  alg->setProperty("InputWorkspace", peaksWS);
  alg->setProperty("OutputWorkspace", peaksWS->name() + "_peaks_in_region");
  alg->setProperty("Extents", transformedViewableRegion.toExtents());
  alg->setProperty("CheckPeakExtents", true);
  alg->setProperty("PeakRadius", effectiveRadius);
  alg->setPropertyValue("CoordinateFrame", m_transform->getFriendlyName());
  alg->execute();
  ITableWorkspace_sptr outTable = alg->getProperty("OutputWorkspace");
  std::vector<bool> viewablePeaks(outTable->rowCount());
  for (size_t i = 0; i < outTable->rowCount(); ++i) {
    viewablePeaks[i] = outTable->cell<Boolean>(i, 1);
  }
  m_viewablePeaks = viewablePeaks;

  m_viewPeaks->setSlicePoint(m_slicePoint.slicePoint(), m_viewablePeaks);
}

/**
 Allow all view to redraw themselves following an update to the slice point
 intersecting plane.
 @param viewableRegion : The new slice position (z) against the x-y plot of
 data.
 */
void ConcretePeaksPresenter::updateWithSlicePoint(
    const PeakBoundingBox &viewableRegion) {
  if (m_slicePoint != viewableRegion) // only update if required.
  {
    m_slicePoint = viewableRegion;
    doFindPeaksInRegion();
  }
}

/**
 Destructor. Hide all owned views.
 */
ConcretePeaksPresenter::~ConcretePeaksPresenter() { hideAll(); }

/**
 Respond to changes in the shown dimension.
 @ return True only if this succeeds.
 */
bool ConcretePeaksPresenter::changeShownDim() {
  // Reconfigure the mapping tranform.
  const bool transformSucceeded = this->configureMappingTransform();
  // Apply the mapping tranform to move each peak overlay object.

  if (transformSucceeded) {
    m_viewPeaks->movePosition(m_transform);
  }
  return transformSucceeded;
}

/**
 This method looks at the plotted dimensions (XY) , and work out what indexes
 into the vector HKL, these XYZ dimensions correpond to.
 The indexes can then be used for any future transformation, where the user
 changes the chosen dimensions to plot.
 @return True if the mapping has succeeded.
 */
bool ConcretePeaksPresenter::configureMappingTransform() {
  bool transformSucceeded = false;
  try {
    std::string xLabel = m_viewFactory->getPlotXLabel();
    std::string yLabel = m_viewFactory->getPlotYLabel();
    auto temp = m_transformFactory->createTransform(xLabel, yLabel);
    m_transform = temp;
    showAll();
    transformSucceeded = true;
  } catch (PeakTransformException &) {
    hideAll();
  }
  return transformSucceeded;
}

/**
 Determine whether the candidate label is the label of the free axis.
 @param label: The candidate axis label to consider.
 @return True if it matches the label of the free axis accoring to the current
 peaks transform.
 */
bool ConcretePeaksPresenter::isLabelOfFreeAxis(const std::string &label) const {
  return isDimensionNameOfFreeAxis(label);
}

/**
 Determine whether the candidate dimension name is the name of the free axis.
 @param name: The candidate dimension name to consider.
 @return True if it matches the label of the free axis accoring to the current
 peaks transform.
 */
bool ConcretePeaksPresenter::isDimensionNameOfFreeAxis(
    const std::string &name) const {
  return boost::regex_match(name, m_transform->getFreePeakAxisRegex());
}

/**
 Request that each owned view makes its self visible.
 */
void ConcretePeaksPresenter::showAll() {
  if (m_viewPeaks != NULL)
    m_viewPeaks->showView();
}

/**
 Request that each owned view makes its self  NOT visible.
 */
void ConcretePeaksPresenter::hideAll() {
  // Hide all views.
  if (m_viewPeaks != NULL)
    m_viewPeaks->hideView();
}

/**
 @return a reference to the held peaks workspace.
 */
SetPeaksWorkspaces ConcretePeaksPresenter::presentedWorkspaces() const {
  // There is only one workspace to return.
  SetPeaksWorkspaces workspaces;
  workspaces.insert(m_peaksWS);
  return workspaces;
}

QColor ConcretePeaksPresenter::getBackgroundColor() const {
  return m_viewPeaks->getBackgroundColour();
}

QColor ConcretePeaksPresenter::getForegroundColor() const {
  return m_viewPeaks->getForegroundColour();
}

void ConcretePeaksPresenter::setForegroundColor(const QColor colour) {
  // Change foreground colours
  if (m_viewPeaks != NULL) {
    m_viewPeaks->changeForegroundColour(colour);
    m_viewPeaks->updateView();
  }
  // For the case that this has been performed outside the GUI.
  informOwnerUpdate();
}

void ConcretePeaksPresenter::setBackgroundColor(const QColor colour) {
  // Change background colours
  if (m_viewPeaks != NULL) {
    m_viewPeaks->changeBackgroundColour(colour);
    m_viewPeaks->updateView();
  }
  // For the case that this has been performed outside the GUI.
  informOwnerUpdate();
}

std::string ConcretePeaksPresenter::getTransformName() const {
  return m_transform->getFriendlyName();
}

void ConcretePeaksPresenter::showBackgroundRadius(const bool show) {
  // Change background colours
  if (m_viewPeaks != NULL) {
    m_viewPeaks->showBackgroundRadius(show);
    doFindPeaksInRegion();
  }
  // For the case that this has been performed outside the GUI.
  informOwnerUpdate();
}

void ConcretePeaksPresenter::setShown(const bool shown) {
  m_isHidden = !shown;
  if (m_viewPeaks != NULL) {
    if (shown) {
      m_viewPeaks->showView();
    } else {
      m_viewPeaks->hideView();
    }
    m_viewPeaks->updateView();
  }
  // For the case that this has been performed outside the GUI.
  informOwnerUpdate();
}

/**
 * Determine whether the presenter is hidden or not.
 * @return
 */
bool ConcretePeaksPresenter::isHidden() const { return m_isHidden; }

/**
 @param peakIndex: index into contained peaks workspace.
 @return the bounding box corresponding to the peakIndex.
 */
PeakBoundingBox
ConcretePeaksPresenter::getBoundingBox(const int peakIndex) const {
  if (peakIndex < 0 || peakIndex > static_cast<int>(m_peaksWS->rowCount())) {
    throw std::out_of_range("Index given to "
                            "ConcretePeaksPresenter::getBoundingBox() is out "
                            "of range.");
  }
  return m_viewPeaks->getBoundingBox(peakIndex);
}

void ConcretePeaksPresenter::sortPeaksWorkspace(const std::string &byColumnName,
                                                const bool ascending) {
  Mantid::API::IPeaksWorkspace_sptr peaksWS =
      boost::const_pointer_cast<Mantid::API::IPeaksWorkspace>(this->m_peaksWS);

  // Sort the Peaks in-place.
  Mantid::API::IAlgorithm_sptr alg =
      AlgorithmManager::Instance().create("SortPeaksWorkspace");
  alg->setChild(true);
  alg->setRethrows(true);
  alg->initialize();
  alg->setProperty("InputWorkspace", peaksWS);
  alg->setPropertyValue("OutputWorkspace", "SortedPeaksWorkspace");
  alg->setProperty("OutputWorkspace", peaksWS);
  alg->setProperty("SortAscending", ascending);
  alg->setPropertyValue("ColumnNameToSortBy", byColumnName);
  alg->execute();

  // Reproduce the views.
  this->produceViews();

  // Give the new views the current slice point.
  m_viewPeaks->setSlicePoint(this->m_slicePoint.slicePoint(), m_viewablePeaks);
}

void ConcretePeaksPresenter::setPeakSizeOnProjection(const double fraction) {
  m_viewPeaks->changeOccupancyInView(fraction);
  m_viewPeaks->updateView();
  // For the case that this has been performed outside the GUI.
  informOwnerUpdate();
}

void ConcretePeaksPresenter::setPeakSizeIntoProjection(const double fraction) {
  m_viewPeaks->changeOccupancyIntoView(fraction);
  doFindPeaksInRegion();
  // For the case that this has been performed outside the GUI.
  informOwnerUpdate();
}

double ConcretePeaksPresenter::getPeakSizeOnProjection() const {
  double result = 0;
  if (m_viewPeaks != NULL && m_viewPeaks->positionOnly()) {
    result = m_viewPeaks->getOccupancyInView();
  }
  return result;
}

double ConcretePeaksPresenter::getPeakSizeIntoProjection() const {
  double result = 0;
  if (m_viewPeaks != NULL && m_viewPeaks->positionOnly()) {
    result = m_viewPeaks->getOccupancyIntoView();
  }
  return result;
}

void
ConcretePeaksPresenter::registerOwningPresenter(UpdateableOnDemand *owner) {
  m_owningPresenter = owner;
}

void ConcretePeaksPresenter::informOwnerUpdate() {
  if (m_owningPresenter) {
    m_owningPresenter->performUpdate();
  }
}

bool ConcretePeaksPresenter::getShowBackground() const {
  return m_viewPeaks->isBackgroundShown();
}

void ConcretePeaksPresenter::zoomToPeak(const int peakIndex) {
  if (auto zoomable = dynamic_cast<ZoomableOnDemand *>(m_owningPresenter)) {
    zoomable->zoomToPeak(this, peakIndex);
  }
}
}
}
