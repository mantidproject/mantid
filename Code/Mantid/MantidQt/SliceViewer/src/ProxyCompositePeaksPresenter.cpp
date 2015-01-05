#include "MantidQtSliceViewer/ProxyCompositePeaksPresenter.h"

namespace MantidQt {
namespace SliceViewer {
/**
Constructor
*/
ProxyCompositePeaksPresenter::ProxyCompositePeaksPresenter(
    boost::shared_ptr<CompositePeaksPresenter> composite)
    : m_compositePresenter(composite), m_updateableView(NULL) {
  m_compositePresenter->registerOwningPresenter(this);
}

ProxyCompositePeaksPresenter::ProxyCompositePeaksPresenter()
    : m_updateableView(NULL) {}

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
@ colour to use for re-colouring
*/
void ProxyCompositePeaksPresenter::setForegroundColour(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, QColor colour) {
  m_compositePresenter->setForegroundColour(ws, colour);
}

/**
Set the background colour of the peaks.
@ workspace containing the peaks to re-colour
@ colour to use for re-colouring
*/
void ProxyCompositePeaksPresenter::setBackgroundColour(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, QColor colour) {
  m_compositePresenter->setBackgroundColour(ws, colour);
}

QColor ProxyCompositePeaksPresenter::getBackgroundColour(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const {
  return m_compositePresenter->getBackgroundColour(ws);
}

QColor ProxyCompositePeaksPresenter::getForegroundColour(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const {
  return m_compositePresenter->getForegroundColour(ws);
}

bool ProxyCompositePeaksPresenter::getShowBackground(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const {
  return m_compositePresenter->getShowBackground(ws);
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
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws,
    const bool shown) {
  m_compositePresenter->setBackgroundRadiusShown(ws, shown);
}

void ProxyCompositePeaksPresenter::remove(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS) {
  m_compositePresenter->remove(peaksWS);
}

void ProxyCompositePeaksPresenter::hideInPlot(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS,
    const bool hide) {
  m_compositePresenter->setShown(peaksWS, !hide);
}

void ProxyCompositePeaksPresenter::zoomToPeak(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS,
    const int peakIndex) {
  m_compositePresenter->zoomToPeak(peaksWS, peakIndex);
}

void ProxyCompositePeaksPresenter::sortPeaksWorkspace(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS,
    const std::string &columnToSortBy, const bool sortedAscending) {
  m_compositePresenter->sortPeaksWorkspace(peaksWS, columnToSortBy,
                                           sortedAscending);
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
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> toWorkspace) {
  if (m_updateableView) {
    m_updateableView->updatePeaksWorkspace(toName, toWorkspace);
  }
}

bool ProxyCompositePeaksPresenter::getIsHidden(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS) const {
  return m_compositePresenter->getIsHidden(peaksWS);
}

void
ProxyCompositePeaksPresenter::registerView(UpdateableOnDemand *updateableView) {
  m_updateableView = updateableView;
}

boost::optional<PeaksPresenter_sptr>
ProxyCompositePeaksPresenter::getZoomedPeakPresenter() const {
  return m_compositePresenter->getZoomedPeakPresenter();
}

int ProxyCompositePeaksPresenter::getZoomedPeakIndex() const {
  return m_compositePresenter->getZoomedPeakIndex();
}
}
}
