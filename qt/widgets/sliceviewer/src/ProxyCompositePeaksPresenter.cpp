// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidQtWidgets/SliceViewer/ProxyCompositePeaksPresenter.h"

namespace MantidQt {
namespace SliceViewer {
/**
Constructor
*/
ProxyCompositePeaksPresenter::ProxyCompositePeaksPresenter(
    std::shared_ptr<CompositePeaksPresenter> composite)
    : m_compositePresenter(std::move(composite)), m_updateableView(nullptr) {
  m_compositePresenter->registerOwningPresenter(this);
}

ProxyCompositePeaksPresenter::ProxyCompositePeaksPresenter()
    : m_updateableView(nullptr) {}

/**
Destructor
*/
ProxyCompositePeaksPresenter::~ProxyCompositePeaksPresenter() {}

/**
Update method
*/
void ProxyCompositePeaksPresenter::update() { m_compositePresenter->update(); }

/**
@return the number of subjects in the composite
*/
size_t ProxyCompositePeaksPresenter::size() const {
  return m_compositePresenter->size();
}

/**
Set the foreground colour of the peaks.
@ workspace containing the peaks to re-colour
@ color to use for re-colouring
*/
void ProxyCompositePeaksPresenter::setForegroundColor(
    std::shared_ptr<const Mantid::API::IPeaksWorkspace> ws,
    const PeakViewColor &color) {
  m_compositePresenter->setForegroundColor(std::move(ws), std::move(color));
}

/**
Set the background colour of the peaks.
@ workspace containing the peaks to re-colour
@ colour to use for re-colouring
*/
void ProxyCompositePeaksPresenter::setBackgroundColor(
    std::shared_ptr<const Mantid::API::IPeaksWorkspace> ws,
    const PeakViewColor &color) {
  m_compositePresenter->setBackgroundColor(std::move(ws), std::move(color));
}

PeakViewColor ProxyCompositePeaksPresenter::getBackgroundPeakViewColor(
    std::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const {
  return m_compositePresenter->getBackgroundPeakViewColor(std::move(ws));
}

PeakViewColor ProxyCompositePeaksPresenter::getForegroundPeakViewColor(
    std::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const {
  return m_compositePresenter->getForegroundPeakViewColor(std::move(ws));
}

bool ProxyCompositePeaksPresenter::getShowBackground(
    std::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const {
  return m_compositePresenter->getShowBackground(std::move(ws));
}

/**
Get all the presented workspaces.
*/
SetPeaksWorkspaces ProxyCompositePeaksPresenter::presentedWorkspaces() const {
  return m_compositePresenter->presentedWorkspaces();
}

/**
Getter for the transform name.
*/
std::string ProxyCompositePeaksPresenter::getTransformName() const {
  return m_compositePresenter->getTransformName();
}

void ProxyCompositePeaksPresenter::setBackgroundRadiusShown(
    std::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, const bool shown) {
  m_compositePresenter->setBackgroundRadiusShown(std::move(ws), shown);
}

void ProxyCompositePeaksPresenter::remove(
    std::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS) {
  m_compositePresenter->remove(std::move(peaksWS));
}

void ProxyCompositePeaksPresenter::hideInPlot(
    std::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS,
    const bool hide) {
  m_compositePresenter->setShown(std::move(peaksWS), !hide);
}

void ProxyCompositePeaksPresenter::zoomToPeak(
    std::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS,
    const int peakIndex) {
  m_compositePresenter->zoomToPeak(std::move(peaksWS), peakIndex);
}

PeaksPresenter *
ProxyCompositePeaksPresenter::getPeaksPresenter(const QString &name) {
  return m_compositePresenter->getPeaksPresenter(name);
}

void ProxyCompositePeaksPresenter::performUpdate() {
  if (m_updateableView) {
    m_updateableView->performUpdate();
  }
}

void ProxyCompositePeaksPresenter::updatePeaksWorkspace(
    const std::string &toName,
    std::shared_ptr<const Mantid::API::IPeaksWorkspace> toWorkspace) {
  if (m_updateableView) {
    m_updateableView->updatePeaksWorkspace(toName, toWorkspace);
  }
}

bool ProxyCompositePeaksPresenter::getIsHidden(
    std::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS) const {
  return m_compositePresenter->getIsHidden(std::move(peaksWS));
}

void ProxyCompositePeaksPresenter::registerView(
    UpdateableOnDemand *updateableView) {
  m_updateableView = updateableView;
}

boost::optional<PeaksPresenter_sptr>
ProxyCompositePeaksPresenter::getZoomedPeakPresenter() const {
  return m_compositePresenter->getZoomedPeakPresenter();
}

int ProxyCompositePeaksPresenter::getZoomedPeakIndex() const {
  return m_compositePresenter->getZoomedPeakIndex();
}

void ProxyCompositePeaksPresenter::editCommand(
    EditMode editMode,
    const std::weak_ptr<const Mantid::API::IPeaksWorkspace> &target) {
  m_compositePresenter->editCommand(editMode, std::move(target));
}

void ProxyCompositePeaksPresenter::setPeakSizeOnProjection(
    const double fraction) {
  m_compositePresenter->setPeakSizeOnProjection(fraction);
}

void ProxyCompositePeaksPresenter::setPeakSizeIntoProjection(
    const double fraction) {
  m_compositePresenter->setPeakSizeIntoProjection(fraction);
}

double ProxyCompositePeaksPresenter::getPeakSizeOnProjection() const {
  return m_compositePresenter->getPeakSizeOnProjection();
}

double ProxyCompositePeaksPresenter::getPeakSizeIntoProjection() const {
  return m_compositePresenter->getPeakSizeIntoProjection();
}
} // namespace SliceViewer
} // namespace MantidQt
