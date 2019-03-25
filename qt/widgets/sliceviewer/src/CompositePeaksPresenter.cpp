// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/SliceViewer/CompositePeaksPresenter.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include <stdexcept>

using Mantid::Geometry::PeakTransform_sptr;

namespace MantidQt {
namespace SliceViewer {
/**
Constructor
*/
CompositePeaksPresenter::CompositePeaksPresenter(
    ZoomablePeaksView *const zoomablePlottingWidget,
    PeaksPresenter_sptr defaultPresenter)
    : m_zoomablePlottingWidget(zoomablePlottingWidget),
      m_default(defaultPresenter), m_owner(nullptr), m_zoomedPeakIndex(-1) {
  if (m_zoomablePlottingWidget == nullptr) {
    throw std::runtime_error("Zoomable Plotting Widget is NULL");
  }
}

/**
Destructor
*/
CompositePeaksPresenter::~CompositePeaksPresenter() {}

/**
Overrriden update method
*/
void CompositePeaksPresenter::update() {
  if (useDefault()) {
    m_default->update();
    return;
  }
  for (auto &subject : m_subjects) {
    subject->update();
  }
}

/**
Overriden updateWithSlicePoint
@param point : Slice point to update with
*/
void CompositePeaksPresenter::updateWithSlicePoint(
    const PeakBoundingBox &point) {
  if (useDefault()) {
    m_default->updateWithSlicePoint(point);
    return;
  }
  for (auto &subject : m_subjects) {
    subject->updateWithSlicePoint(point);
  }
}

/**
Handle dimension display changing.
*/
bool CompositePeaksPresenter::changeShownDim(size_t dimX, size_t dimY) {
  if (useDefault()) {
    return m_default->changeShownDim(dimX, dimY);
  }
  bool result = true;
  for (auto &subject : m_subjects) {
    result &= subject->changeShownDim(dimX, dimY);
  }
  return result;
}

void CompositePeaksPresenter::setNonOrthogonal(bool nonOrthogonalEnabled) {
  if (useDefault()) {
    m_default->setNonOrthogonal(nonOrthogonalEnabled);
  }

  for (auto &subject : m_subjects) {
    subject->setNonOrthogonal(nonOrthogonalEnabled);
  }
}
/**
Determine wheter a given axis label correponds to the free peak axis.
@return True only if the label is that of the free peak axis.
*/
bool CompositePeaksPresenter::isLabelOfFreeAxis(
    const std::string &label) const {
  if (useDefault()) {
    return m_default->isLabelOfFreeAxis(label);
  }
  bool result = true;
  for (const auto &subject : m_subjects) {
    result &= subject->isLabelOfFreeAxis(label);
  }
  return result;
}

/**
Clear all peaks
*/
void CompositePeaksPresenter::clear() {

  if (!m_subjects.empty()) {
    for (auto &i : m_subjects) {
      i->setShown(false);
    }
    m_subjects.clear();
    this->m_zoomablePlottingWidget->detach();
    PeakPalette<PeakViewColor> tempPeakViewColor;
    m_palettePeakViewColor = tempPeakViewColor;
  }
}

/**
 * @brief Determine wheter the workspace contents is different with another
 * presenter.
 * @param other
 * @return
 */
bool CompositePeaksPresenter::contentsDifferent(
    PeaksPresenter const *const other) const {
  /*
   What we are doing here is looking through all workspaces associated with the
   incoming presenter
   and seeing if ANY of the exising subjects of this composite is already
   presenting one of these workspaces.
   ONLY if there is no intesection between the two sets will we allow addition.
   */
  const SetPeaksWorkspaces otherWorkspaces = other->presentedWorkspaces();
  const SetPeaksWorkspaces existingWorkspaces = this->presentedWorkspaces();

  SetPeaksWorkspaces difference;
  std::set_difference(existingWorkspaces.begin(), existingWorkspaces.end(),
                      otherWorkspaces.begin(), otherWorkspaces.end(),
                      std::inserter(difference, difference.end()));

  // Is the candidate set the same as the difference set (i.e. no intesection)
  // and also non-zero in size
  const bool different = (difference.size() == existingWorkspaces.size());
  return different;
}

/**
Add peaks presenter
@param presenter : Subject peaks presenter
*/
void CompositePeaksPresenter::addPeaksPresenter(PeaksPresenter_sptr presenter) {
  if (this->size() == 10) {
    throw std::invalid_argument("Maximum number of PeaksWorkspaces that can be "
                                "simultaneously displayed is 10.");
  }

  // Look for the same presenter added twice.
  auto result_it = std::find(m_subjects.begin(), m_subjects.end(), presenter);

  // Only add a peaks presenter if the contents are different. The presenter may
  // be different, but manage the same workspace set.
  if (result_it == m_subjects.end() && presenter->contentsDifferent(this)) {
    m_subjects.push_back(presenter);
    presenter->registerOwningPresenter(this);
  }
}

/**
@return the number of subjects in the composite
*/
size_t CompositePeaksPresenter::size() const { return m_subjects.size(); }

/**
Return a collection of all referenced workspaces on demand.
*/
SetPeaksWorkspaces CompositePeaksPresenter::presentedWorkspaces() const {
  SetPeaksWorkspaces allWorkspaces;
  for (const auto &subject : m_subjects) {
    auto workspacesToAppend = subject->presentedWorkspaces();
    allWorkspaces.insert(workspacesToAppend.begin(), workspacesToAppend.end());
  }
  return allWorkspaces;
}

/**
@param ws : Peaks Workspace to look for on sub-presenters.
@return the identified sub-presenter for the workspace, or a NullPeaksPresenter.
*/
CompositePeaksPresenter::SubjectContainer::iterator
CompositePeaksPresenter::getPresenterIteratorFromWorkspace(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) {
  SubjectContainer::iterator presenterFound = m_subjects.end();
  for (auto presenterIterator = m_subjects.begin();
       presenterIterator != m_subjects.end(); ++presenterIterator) {
    auto workspacesOfSubject = (*presenterIterator)->presentedWorkspaces();
    SetPeaksWorkspaces::iterator iteratorFound = workspacesOfSubject.find(ws);
    if (iteratorFound != workspacesOfSubject.end()) {
      presenterFound = presenterIterator;
      break;
    }
  }
  return presenterFound;
}

/**
@param ws : Peaks Workspace to look for on sub-presenters.
@return the identified sub-presenter for the workspace, or a NullPeaksPresenter.
*/
CompositePeaksPresenter::SubjectContainer::const_iterator
CompositePeaksPresenter::getPresenterIteratorFromWorkspace(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const {
  SubjectContainer::const_iterator presenterFound = m_subjects.end();
  for (auto presenterIterator = m_subjects.begin();
       presenterIterator != m_subjects.end(); ++presenterIterator) {
    auto workspacesOfSubject = (*presenterIterator)->presentedWorkspaces();
    SetPeaksWorkspaces::iterator iteratorFound = workspacesOfSubject.find(ws);
    if (iteratorFound != workspacesOfSubject.end()) {
      presenterFound = presenterIterator;
      break;
    }
  }
  return presenterFound;
}

/**
Set the foreground colour of the peaks.
@ workspace containing the peaks to re-colour
@ colour to use for re-colouring
*/
void CompositePeaksPresenter::setForegroundColor(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws,
    const PeakViewColor color) {
  SubjectContainer::iterator iterator = getPresenterIteratorFromWorkspace(ws);

  // Update the palette the foreground colour
  const int pos = static_cast<int>(std::distance(m_subjects.begin(), iterator));
  m_palettePeakViewColor.setForegroundColour(pos, color);

  // Apply the foreground colour
  (*iterator)->setForegroundColor(color);
}

/**
Set the background colour of the peaks.
@ workspace containing the peaks to re-colour
@ colour to use for re-colouring
*/
void CompositePeaksPresenter::setBackgroundColor(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws,
    const PeakViewColor color) {
  SubjectContainer::iterator iterator = getPresenterIteratorFromWorkspace(ws);

  // Update the palette background colour.
  const int pos = static_cast<int>(std::distance(m_subjects.begin(), iterator));
  m_palettePeakViewColor.setBackgroundColour(pos, color);

  // Apply the background colour
  (*iterator)->setBackgroundColor(color);
}

/**
Getter for the name of the transform.
@return transform name.
*/
std::string CompositePeaksPresenter::getTransformName() const {
  if (useDefault()) {
    return m_default->getTransformName();
  }
  return (*m_subjects.begin())->getTransformName();
}

/**
@return a copy of the peaks palette.
*/
PeakPalette<PeakViewColor> CompositePeaksPresenter::getPalette() const {
  return this->m_palettePeakViewColor;
}

/**
@param ws: PeakWorkspace to get the colour for.
@return the foreground colour corresponding to the peaks workspace.
*/
PeakViewColor CompositePeaksPresenter::getForegroundPeakViewColor(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const {
  if (useDefault()) {
    throw std::runtime_error("Foreground colours from palette cannot be "
                             "fetched until nested presenters are added.");
  }
  SubjectContainer::const_iterator iterator =
      getPresenterIteratorFromWorkspace(ws);
  const int pos = static_cast<int>(std::distance(m_subjects.begin(), iterator));
  return m_palettePeakViewColor.foregroundIndexToColour(pos);
}

/**
@param ws: PeakWorkspace to get the colour for.
@return the background colour corresponding to the peaks workspace.
*/
PeakViewColor CompositePeaksPresenter::getBackgroundPeakViewColor(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const {
  if (useDefault()) {
    throw std::runtime_error("Background colours from palette cannot be "
                             "fetched until nested presenters are added.");
  }
  SubjectContainer::const_iterator iterator =
      getPresenterIteratorFromWorkspace(ws);
  const int pos = static_cast<int>(std::distance(m_subjects.begin(), iterator));
  return m_palettePeakViewColor.backgroundIndexToColour(pos);
}

/**
 * Set to show the background radius.
 * @param ws : Workspace upon which the backgoround radius should be
 * shown/hidden.
 * @param shown : True to show.
 */
void CompositePeaksPresenter::setBackgroundRadiusShown(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws,
    const bool shown) {
  if (useDefault()) {
    return m_default->showBackgroundRadius(shown);
  }
  auto iterator = getPresenterIteratorFromWorkspace(ws);
  (*iterator)->showBackgroundRadius(shown);
}

/**
 * Remove a peaks list altogether from the reporting and peaks overlays.
 * @param peaksWS : Peaks list to remove.
 */
void CompositePeaksPresenter::remove(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS) {
  if (useDefault()) {
    return;
  }
  auto iterator = getPresenterIteratorFromWorkspace(peaksWS);
  if (iterator != m_subjects.end()) {
    m_subjects.erase(iterator);
  }
  if (m_subjects.empty()) {
    this->m_zoomablePlottingWidget->detach();
  }
}

/**
 * Allow the peaks list to be hidden or visible.
 * @param peaksWS : Peaks list to show/hide.
 * @param shown : True to show.
 */
void CompositePeaksPresenter::setShown(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS,
    const bool shown) {
  if (useDefault()) {
    return m_default->setShown(shown);
  }
  auto iterator = getPresenterIteratorFromWorkspace(peaksWS);
  if (iterator == m_subjects.end())
    return;

  auto presenter = *iterator;
  if (presenter)
    presenter->setShown(shown);
}

/**
 * Zoom in on a given peak in a given peaks list according to the current
 * viewing dimensions.
 * @param peaksWS : Peaks list from which a choosen peak will be zoomed into.
 * @param peakIndex : Index of the peak in the peaks list to zoom into.
 */
void CompositePeaksPresenter::zoomToPeak(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS,
    const int peakIndex) {
  auto iterator = getPresenterIteratorFromWorkspace(peaksWS);
  auto subjectPresenter = *iterator;
  auto boundingBox = subjectPresenter->getBoundingBox(peakIndex);
  m_zoomablePlottingWidget->zoomToRectangle(boundingBox);
  m_zoomedPeakIndex = peakIndex;
  m_zoomedPresenter = subjectPresenter;
}

/**
 * Set the peaks size on the current projection using the supplied fraction.
 * @param fraction of the view width to use as the peak radius.
 */
void CompositePeaksPresenter::setPeakSizeOnProjection(const double fraction) {
  if (useDefault()) {
    return m_default->setPeakSizeOnProjection(fraction);
  }
  for (auto &subject : m_subjects) {
    subject->setPeakSizeOnProjection(fraction);
  }
}

/**
 * Enter a peak edit mode
 * @param mode : Mode to enter.
 */
void CompositePeaksPresenter::peakEditMode(EditMode mode) {
  if (useDefault()) {
    return m_default->peakEditMode(mode);
  }
  for (auto &subject : m_subjects) {
    subject->peakEditMode(mode);
  }
}

/**
 * Fraction of the z-range to use as the peak radius.
 * @param fraction to use as the peak radius
 */
void CompositePeaksPresenter::setPeakSizeIntoProjection(const double fraction) {
  if (useDefault()) {
    return m_default->setPeakSizeIntoProjection(fraction);
  }
  for (auto &subject : m_subjects) {
    subject->setPeakSizeIntoProjection(fraction);
  }
}

/**
 * Get the peak size on the projection plane
 * @return size
 */
double CompositePeaksPresenter::getPeakSizeOnProjection() const {
  if (useDefault()) {
    return m_default->getPeakSizeOnProjection();
  }
  double result = 0;
  for (const auto &subject : m_subjects) {
    double temp = subject->getPeakSizeOnProjection();
    if (temp > 0) {
      result = temp;
      break;
    }
  }
  return result;
}

/**
 * Get peak size into the projection
 * @return size
 */
double CompositePeaksPresenter::getPeakSizeIntoProjection() const {
  if (useDefault()) {
    return m_default->getPeakSizeIntoProjection();
  }
  double result = 0;
  for (const auto &subject : m_subjects) {
    double temp = subject->getPeakSizeIntoProjection();
    if (temp > 0) {
      result = temp;
      break;
    }
  }
  return result;
}

/**
 * Determine if the background is to be shown for a particular workspace.
 * @param ws
 * @return
 */
bool CompositePeaksPresenter::getShowBackground(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const {
  if (useDefault()) {
    throw std::runtime_error("Get show background cannot be fetched until "
                             "nested presenters are added.");
  }
  SubjectContainer::const_iterator iterator =
      getPresenterIteratorFromWorkspace(ws);
  return (*iterator)->getShowBackground();
}

namespace {
// Helper comparitor type.
class MatchWorkspaceName
    : public std::unary_function<SetPeaksWorkspaces::value_type, bool> {
private:
  const QString m_wsName;

public:
  explicit MatchWorkspaceName(const QString &name) : m_wsName(name) {}
  bool operator()(SetPeaksWorkspaces::value_type ws) {
    const std::string &wsName = ws->getName();
    const std::string toMatch = m_wsName.toStdString();
    const bool result = (wsName == toMatch);
    return result;
  }
};
} // namespace

CompositePeaksPresenter::SubjectContainer::iterator
CompositePeaksPresenter::getPresenterIteratorFromName(const QString &name) {
  MatchWorkspaceName comparitor(name);
  SubjectContainer::iterator presenterFound = m_subjects.end();
  for (auto presenterIterator = m_subjects.begin();
       presenterIterator != m_subjects.end(); ++presenterIterator) {
    auto wsOfSubject = (*presenterIterator)->presentedWorkspaces();
    SetPeaksWorkspaces::iterator iteratorFound =
        std::find_if(wsOfSubject.begin(), wsOfSubject.end(), comparitor);
    if (iteratorFound != wsOfSubject.end()) {
      presenterFound = presenterIterator;
      break;
    }
  }
  return presenterFound;
}

/**
 * Get the peaks presenter corresponding to a peaks workspace name.
 * @param name
 * @return Peaks presenter.
 */
PeaksPresenter *
CompositePeaksPresenter::getPeaksPresenter(const QString &name) {
  SubjectContainer::iterator presenterFound =
      this->getPresenterIteratorFromName(name);
  if (presenterFound == m_subjects.end()) {
    throw std::invalid_argument("Cannot find peaks workspace called :" +
                                name.toStdString());
  }
  return (*presenterFound).get();
}

/**
 * Register an owning presenter for this object.
 * @param owner
 */
void CompositePeaksPresenter::registerOwningPresenter(
    UpdateableOnDemand *owner) {
  m_owner = owner;
}

/**
 * Perform steps associated with an update. Driven by nested presenters.
 */
void CompositePeaksPresenter::performUpdate() {
  for (auto presenterIterator = m_subjects.begin();
       presenterIterator != m_subjects.end(); ++presenterIterator) {
    auto presenter = (*presenterIterator);
    const int pos =
        static_cast<int>(std::distance(m_subjects.begin(), presenterIterator));
    m_palettePeakViewColor.setBackgroundColour(
        pos, presenter->getBackgroundPeakViewColor());
    m_palettePeakViewColor.setForegroundColour(
        pos, presenter->getForegroundPeakViewColor());

    if (m_owner) {
      m_owner->performUpdate();
    }
  }
}

namespace {
// Private helper class
class MatchPointer : public std::unary_function<bool, PeaksPresenter_sptr> {
private:
  PeaksPresenter *m_toFind;

public:
  explicit MatchPointer(PeaksPresenter *toFind) : m_toFind(toFind) {}
  bool operator()(PeaksPresenter_sptr candidate) {
    return candidate.get() == m_toFind;
  }
};
} // namespace

/**
 * Zoom to a peak
 * @param presenter: Holds the peaks workspace.
 * @param peakIndex: The peak index.
 */
void CompositePeaksPresenter::zoomToPeak(PeaksPresenter *const presenter,
                                         const int peakIndex) {

  MatchPointer comparitor(presenter);
  m_zoomedPeakIndex = peakIndex;
  SubjectContainer::iterator it =
      std::find_if(m_subjects.begin(), m_subjects.end(), comparitor);
  if (it == m_subjects.end()) {
    throw std::invalid_argument(
        "Cannot file subject presenter at CompositePeaksPresenter::zoomToPeak");
  }
  const PeakBoundingBox &box = presenter->getBoundingBox(peakIndex);
  m_zoomablePlottingWidget->zoomToRectangle(box);
  m_zoomedPresenter = *it;
  m_zoomedPeakIndex = peakIndex;
  if (m_owner) {
    m_owner->performUpdate();
  }
}

/**
 * Determine if the presenter is hidden.
 * @param peaksWS to use to find the right presenter.
 * @return True if hidden.
 */
bool CompositePeaksPresenter::getIsHidden(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS) const {
  auto iterator = getPresenterIteratorFromWorkspace(peaksWS);
  auto subjectPresenter = *iterator;
  return subjectPresenter->isHidden();
}

/**
 * Reset the zoom.
 * Forget any optional zoom extents.
 */
void CompositePeaksPresenter::resetZoom() {
  m_zoomedPeakIndex = -1;
  m_zoomedPresenter.reset();
  if (m_owner) {
    m_owner->performUpdate(); // This tells any 'listening GUIs' to sort
                              // themselves out.
  }
}

/**
 * @return an optional zoomed peak presenter.
 */
boost::optional<PeaksPresenter_sptr>
CompositePeaksPresenter::getZoomedPeakPresenter() const {
  return m_zoomedPresenter;
}

/**
 * @return a zoomed peak index.
 */
int CompositePeaksPresenter::getZoomedPeakIndex() const {
  return m_zoomedPeakIndex;
}

void CompositePeaksPresenter::editCommand(
    EditMode editMode,
    boost::weak_ptr<const Mantid::API::IPeaksWorkspace> target) {
  if (auto ws = target.lock()) {

    // Change the right subject to the desired edit mode.
    auto targetIterator = this->getPresenterIteratorFromWorkspace(ws);
    if (targetIterator != m_subjects.end()) {
      (*targetIterator)->peakEditMode(editMode);
    }
    // Reset everything else.
    for (auto it = m_subjects.begin(); it != m_subjects.end(); ++it) {
      if (it != targetIterator) {
        // All other subjects must be in a neutral edit mode.
        (*it)->peakEditMode(None);
      }
    }
  }
}

void CompositePeaksPresenter::updatePeaksWorkspace(
    const std::string &toName,
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> toWorkspace) {
  if (m_owner) {
    m_owner->updatePeaksWorkspace(toName, toWorkspace);
  }
}

void CompositePeaksPresenter::notifyWorkspaceChanged(
    const std::string &wsName,
    Mantid::API::IPeaksWorkspace_sptr &changedPeaksWS) {
  // Try to find the peaks workspace via location, in the case of changes
  // involving in-place operations.
  auto iterator = this->getPresenterIteratorFromWorkspace(changedPeaksWS);

  // If the above strategy fails. Look for a presenter via a name.
  if (iterator == m_subjects.end()) {
    // Try to find the peaks workspace via name in the case of changes involving
    // changes to ADS values, but no changes to keys (copy-swap type
    // operations).

    // The peaks workspace location may have changed, but the workspace name has
    // not.
    iterator = this->getPresenterIteratorFromName(
        wsName.c_str()); // TODO. don't ask the peaks workspace for it's name
                         // directly as it gets it from the workspace object,
                         // and that object would have been changed!
    if (iterator == m_subjects.end()) {
      return;
    }
  }
  const int pos = static_cast<int>(std::distance(m_subjects.begin(), iterator));
  m_subjects[pos]->reInitialize(changedPeaksWS);

  auto presentedWorkspaces = m_subjects[pos]->presentedWorkspaces();
  // We usually only have a single workspace, but just incase there are more.
  if (m_owner) {
    for (const auto & presentedWorkspace : presentedWorkspaces) {
      m_owner->updatePeaksWorkspace(wsName, presentedWorkspace);
    }
  }
}

bool CompositePeaksPresenter::deletePeaksIn(PeakBoundingBox box) {
  if (useDefault()) {
    return m_default->deletePeaksIn(box);
  }
  // Forward the request onwards
  bool result = false;
  for (auto &subject : m_subjects) {
    result |= subject->deletePeaksIn(box);
  }
  return result;
}

bool CompositePeaksPresenter::addPeakAt(double plotCoordsPointX,
                                        double plotCoordsPointY) {
  if (useDefault()) {
    return m_default->addPeakAt(plotCoordsPointX, plotCoordsPointY);
  }
  // Forward the request onwards
  bool result = false;
  for (auto &subject : m_subjects) {
    result |= subject->addPeakAt(plotCoordsPointX, plotCoordsPointY);
  }
  return result;
}
} // namespace SliceViewer
} // namespace MantidQt
