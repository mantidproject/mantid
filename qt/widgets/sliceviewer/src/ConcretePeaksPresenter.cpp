// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/SliceViewer/ConcretePeaksPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/PeakShapeBase.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/V3D.h"
#include "MantidQtWidgets/SliceViewer/PeakEditMode.h"
#include "MantidQtWidgets/SliceViewer/UpdateableOnDemand.h"
#include "MantidQtWidgets/SliceViewer/ZoomableOnDemand.h"
#include <boost/regex.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using Mantid::Geometry::IMDDimension_const_sptr;

namespace MantidQt {
namespace SliceViewer {
namespace {
/// static logger
Mantid::Kernel::Logger g_log("PeaksPresenter");
} // namespace

/**
 * Convert from a SpecialCoordinateSystem enum to a correpsonding enum name.
 * @param coordSystem : enum option
 * @return coordinate system as a string
 */
std::string
coordinateToString(Mantid::Kernel::SpecialCoordinateSystem coordSystem) {
  switch (coordSystem) {
  case Mantid::Kernel::QLab:
    return "QLab";
  case Mantid::Kernel::QSample:
    return "QSample";
  case Mantid::Kernel::HKL:
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

  PeakOverlayView_sptr newView = m_viewFactory->createView(this, m_transform);
  PeakOverlayView_sptr oldView = m_viewPeaks;
  if (oldView) {
    newView->takeSettingsFrom(oldView.get());
  }
  m_viewPeaks = newView;

  // We reapply any cached edit mode settings we had before.
  this->peakEditMode(m_editMode);
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
      ss << '\n';
      ss << "According to the dimension names in your MDWorkspace, this "
            "work-space is determined to be in: ";
      ss << m_transform->getFriendlyName() << " in the PeaksViewer. ";
      ss << "However, the MDWorkspace has properties indicating that it's "
            "coordinates are in: "
         << coordinateToString(coordSystMD);
      ss << " To resolve the conflict, the MDWorkspace will be treated as "
            "though it has coordinates in: "
         << m_transform->getFriendlyName();
      g_log.notice(ss.str());
    }
    // If the peaks work-space has been integrated. check cross-work-space
    // compatibility.
    if (coordSystDim != coordSystPK && m_peaksWS->hasIntegratedPeaks()) {
      std::stringstream ss;
      ss << '\n';
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
    Mantid::Geometry::PeakTransformFactory_sptr transformFactory)
    : m_viewFactory(viewFactory), m_peaksWS(peaksWS),
      m_transformFactory(transformFactory),
      m_transform(transformFactory->createDefaultTransform()), m_slicePoint(),
      m_owningPresenter(nullptr), m_isHidden(false),
      m_editMode(SliceViewer::None), m_nonOrthogonalMode(false) {

  m_axisData.dimX = 0;
  m_axisData.dimY = 1;
  m_axisData.dimMissing = 2;

  m_axisData.fromHklToXyz[0] = 1.0;
  m_axisData.fromHklToXyz[1] = 0.0;
  m_axisData.fromHklToXyz[2] = 0.0;
  m_axisData.fromHklToXyz[3] = 0.0;
  m_axisData.fromHklToXyz[4] = 1.0;
  m_axisData.fromHklToXyz[5] = 0.0;
  m_axisData.fromHklToXyz[6] = 0.0;
  m_axisData.fromHklToXyz[7] = 0.0;
  m_axisData.fromHklToXyz[8] = 1.0;

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

  // Make and register each peak widget.
  produceViews();
  // in case dimensions shown are not those expected by
  // default transformation
  changeShownDim(m_axisData.dimX, m_axisData.dimY);
}

/**
 Force each view to re-paint.
 */
void ConcretePeaksPresenter::update() { m_viewPeaks->updateView(); }

/**
 * Set/update the internal visible peak mask list.
 * @param indexes : Indexes of peaks that can be seen.
 */
void ConcretePeaksPresenter::setVisiblePeaks(
    const std::vector<size_t> &indexes) {
  std::vector<bool> visible(this->m_peaksWS->getNumberPeaks(),
                            false); // assume all invisible
  for (unsigned long long indexe : indexes) {
    visible[indexe] =
        true; // make the visible indexes visible. Masking type operation.
  }
  m_viewablePeaks = visible;

  m_viewPeaks->setSlicePoint(m_slicePoint.slicePoint(), m_viewablePeaks);
}

/**
 * Find the peaks in the region.
 * Update the view with all those peaks that could be viewable.
 *
 * Also takes into account the peak radius, so needs to be re-executed when the
 *user changes the effective radius of the peaks markers.
 */
void ConcretePeaksPresenter::doFindPeaksInRegion() {

  auto indexes = findVisiblePeakIndexes(m_slicePoint);
  setVisiblePeaks(indexes);
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
bool ConcretePeaksPresenter::changeShownDim(size_t dimX, size_t dimY) {
  m_axisData.dimX = dimX;
  m_axisData.dimY = dimY;

  // Reconfigure the mapping tranform.
  const bool transformSucceeded = this->configureMappingTransform();
  // Apply the mapping tranform to move each peak overlay object.

  if (transformSucceeded) {
    if (m_nonOrthogonalMode) {
      m_viewFactory->getNonOrthogonalInfo(m_axisData);
      m_viewPeaks->movePositionNonOrthogonal(m_transform, m_axisData);
    } else {
      m_viewPeaks->movePosition(m_transform);
    }
  }
  return transformSucceeded;
}

void ConcretePeaksPresenter::setNonOrthogonal(bool nonOrthogonalEnabled) {
  m_nonOrthogonalMode = nonOrthogonalEnabled;
  if (m_nonOrthogonalMode) {
    m_viewFactory->getNonOrthogonalInfo(m_axisData);
    changeShownDim(m_axisData.dimX, m_axisData.dimY);
  }
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
  } catch (Mantid::Geometry::PeakTransformException &) {
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
  if (m_viewPeaks != nullptr)
    m_viewPeaks->showView();
}

/**
 Request that each owned view makes its self  NOT visible.
 */
void ConcretePeaksPresenter::hideAll() {
  // Hide all views.
  if (m_viewPeaks != nullptr)
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

PeakViewColor ConcretePeaksPresenter::getBackgroundPeakViewColor() const {
  return m_viewPeaks->getBackgroundPeakViewColor();
}

PeakViewColor ConcretePeaksPresenter::getForegroundPeakViewColor() const {
  return m_viewPeaks->getForegroundPeakViewColor();
}

void ConcretePeaksPresenter::setForegroundColor(const PeakViewColor color) {
  // Change foreground colors
  if (m_viewPeaks != nullptr) {
    m_viewPeaks->changeForegroundColour(color);
    m_viewPeaks->updateView();
  }
  // For the case that this has been performed outside the GUI.
  informOwnerUpdate();
}

void ConcretePeaksPresenter::setBackgroundColor(const PeakViewColor color) {
  // Change background colours
  if (m_viewPeaks != nullptr) {
    m_viewPeaks->changeBackgroundColour(color);
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
  if (m_viewPeaks != nullptr) {
    m_viewPeaks->showBackgroundRadius(show);
    doFindPeaksInRegion();
  }
  // For the case that this has been performed outside the GUI.
  informOwnerUpdate();
}

void ConcretePeaksPresenter::setShown(const bool shown) {
  m_isHidden = !shown;
  if (m_viewPeaks != nullptr) {
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
 * Determine if the contents of the other presenter are different from this one.
 * @param other
 * @return
 */
bool ConcretePeaksPresenter::contentsDifferent(
    const PeaksPresenter *other) const {
  const SetPeaksWorkspaces otherWorkspaces = other->presentedWorkspaces();

  // Look for this workspace in the others workspace list.
  auto iterator = otherWorkspaces.find(this->m_peaksWS);

  const bool different = (iterator == otherWorkspaces.end());
  return different;
}

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
  if (m_viewPeaks != nullptr && m_peaksWS->getNumberPeaks() > 0) {
    result = m_viewPeaks->getOccupancyInView();
  }
  return result;
}

double ConcretePeaksPresenter::getPeakSizeIntoProjection() const {
  double result = 0;
  if (m_viewPeaks != nullptr && m_peaksWS->getNumberPeaks() > 0) {
    result = m_viewPeaks->getOccupancyIntoView();
  }
  return result;
}

void ConcretePeaksPresenter::registerOwningPresenter(
    UpdateableOnDemand *owner) {
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

void ConcretePeaksPresenter::peakEditMode(EditMode mode) {
  if (mode == DeletePeaks) {
    m_viewPeaks->peakDeletionMode();
  } else if (mode == AddPeaks) {
    m_viewPeaks->peakAdditionMode();
  } else {
    m_viewPeaks->peakDisplayMode();
  }
  // Cache the current edit mode.
  m_editMode = mode;
}

bool ConcretePeaksPresenter::deletePeaksIn(PeakBoundingBox box) {

  Left left(box.left());
  Right right(box.right());
  Bottom bottom(box.bottom());
  Top top(box.top());
  SlicePoint slicePoint(box.slicePoint());
  if (slicePoint() < 0) { // indicates that it should not be used.
    slicePoint = SlicePoint(m_slicePoint.slicePoint());
  }

  PeakBoundingBox accurateBox(
      left, right, top, bottom,
      slicePoint /*Use the current slice position, previously unknown.*/);

  // Tranform box from plot coordinates into orderd HKL, Qx,Qy,Qz etc, then find
  // the visible peaks.
  std::vector<size_t> deletionIndexList = findVisiblePeakIndexes(accurateBox);

  // If we have things to remove, do that in one-step.
  if (!deletionIndexList.empty()) {

    Mantid::API::IPeaksWorkspace_sptr peaksWS =
        boost::const_pointer_cast<Mantid::API::IPeaksWorkspace>(
            this->m_peaksWS);
    // Sort the Peaks in-place.
    Mantid::API::IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("DeleteTableRows");
    alg->setChild(true);
    alg->setRethrows(true);
    alg->initialize();
    alg->setProperty("TableWorkspace", peaksWS);
    alg->setProperty("Rows", deletionIndexList);
    alg->execute();

    // Reproduce the views. Proxy representations recreated for all peaks.
    this->produceViews();

    // Refind visible peaks and Set the proxy representations to be visible or
    // not.
    doFindPeaksInRegion();

    // Upstream controls need to be regenerated.
    this->informOwnerUpdate();
  }
  return !deletionIndexList.empty();
}

bool ConcretePeaksPresenter::addPeakAt(double plotCoordsPointX,
                                       double plotCoordsPointY) {
  V3D plotCoordsPoint(plotCoordsPointX, plotCoordsPointY,
                      m_slicePoint.slicePoint());
  V3D position = m_transform->transformBack(plotCoordsPoint);

  Mantid::API::IPeaksWorkspace_sptr peaksWS =
      boost::const_pointer_cast<Mantid::API::IPeaksWorkspace>(this->m_peaksWS);

  const auto frame = m_transform->getCoordinateSystem();
  try {
    peaksWS->addPeak(position, frame);
  } catch (const std::invalid_argument &e) {
    g_log.warning(e.what());
  }

  // Reproduce the views. Proxy representations recreated for all peaks.
  this->produceViews();

  // Refind visible peaks and Set the proxy representations to be visible or
  // not.
  doFindPeaksInRegion();

  // Upstream controls need to be regenerated.
  this->informOwnerUpdate();

  return true;
}

std::vector<size_t>
ConcretePeaksPresenter::findVisiblePeakIndexes(const PeakBoundingBox &box) {
  std::vector<size_t> indexes;
  // Don't bother to find peaks in the region if there are no peaks to find.
  if (this->m_peaksWS->getNumberPeaks() >= 1) {

    double radius =
        m_viewPeaks
            ->getRadius(); // Effective radius of each peak representation.

    Mantid::API::IPeaksWorkspace_sptr peaksWS =
        boost::const_pointer_cast<Mantid::API::IPeaksWorkspace>(
            this->m_peaksWS);

    PeakBoundingBox transformedViewableRegion = box.makeSliceBox(radius);

    transformedViewableRegion.transformBox(m_transform);

    Mantid::API::IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("PeaksInRegion");
    alg->setChild(true);
    alg->setRethrows(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", peaksWS);
    alg->setProperty("OutputWorkspace",
                     peaksWS->getName() + "_peaks_in_region");
    alg->setProperty("Extents", transformedViewableRegion.toExtents());
    alg->setProperty("CheckPeakExtents", false); // consider all peaks as points
    alg->setProperty("PeakRadius", radius);
    alg->setPropertyValue("CoordinateFrame", m_transform->getFriendlyName());
    alg->execute();
    ITableWorkspace_sptr outTable = alg->getProperty("OutputWorkspace");

    for (size_t i = 0; i < outTable->rowCount(); ++i) {
      const bool insideRegion = outTable->cell<Boolean>(i, 1);
      if (insideRegion) {
        indexes.push_back(i);
      }
    }
  }
  return indexes;
}
} // namespace SliceViewer
} // namespace MantidQt
