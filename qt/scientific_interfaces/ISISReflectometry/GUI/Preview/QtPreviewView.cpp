// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtPreviewView.h"
#include "MantidQtIcons/Icon.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedCylinder.h"

#include <string>

using namespace Mantid::Kernel;
using MantidQt::MantidWidgets::ProjectionSurface;

namespace MantidQt::CustomInterfaces::ISISReflectometry {
QtPreviewView::QtPreviewView(QWidget *parent) : QWidget(parent) {
  m_ui.setupUi(this);
  m_instDisplay = std::make_unique<MantidWidgets::InstrumentDisplay>(m_ui.inst_view_placeholder);
  loadToolbarIcons();
  connectSignals();
}

void QtPreviewView::loadToolbarIcons() {
  m_ui.iv_zoom_button->setIcon(MantidQt::Icons::getIcon("mdi.magnify", "black", 1.3));
  m_ui.iv_edit_button->setIcon(MantidQt::Icons::getIcon("mdi.pencil", "black", 1.3));
  m_ui.iv_rect_select_button->setIcon(MantidQt::Icons::getIcon("mdi.selection", "black", 1.3));
  m_ui.contour_ads_export_button->setIcon(MantidQt::Icons::getIcon("mdi.file-export", "black", 1.3));
}

void QtPreviewView::subscribe(PreviewViewSubscriber *notifyee) noexcept { m_notifyee = notifyee; }

void QtPreviewView::connectSignals() const {
  connect(m_ui.load_button, SIGNAL(clicked()), this, SLOT(onLoadWorkspaceRequested()));

  connect(m_ui.iv_zoom_button, SIGNAL(clicked()), this, SLOT(onInstViewZoomClicked()));
  connect(m_ui.iv_edit_button, SIGNAL(clicked()), this, SLOT(onInstViewEditClicked()));
  connect(m_ui.iv_rect_select_button, SIGNAL(clicked()), this, SLOT(onInstViewSelectRectClicked()));
  connect(m_ui.contour_ads_export_button, SIGNAL(clicked()), this, SLOT(onContourExportToAdsClicked()));
}

void QtPreviewView::onLoadWorkspaceRequested() const { m_notifyee->notifyLoadWorkspaceRequested(); }

void QtPreviewView::onInstViewZoomClicked() const { m_notifyee->notifyInstViewZoomRequested(); }
void QtPreviewView::onInstViewEditClicked() const { m_notifyee->notifyInstViewEditRequested(); }
void QtPreviewView::onInstViewSelectRectClicked() const { m_notifyee->notifyInstViewSelectRectRequested(); }
void QtPreviewView::onInstViewShapeChanged() const { m_notifyee->notifyInstViewShapeChanged(); }

void QtPreviewView::onContourExportToAdsClicked() const { m_notifyee->notifyContourExportAdsRequested(); }

std::string QtPreviewView::getWorkspaceName() const { return m_ui.workspace_line_edit->text().toStdString(); }

void QtPreviewView::plotInstView(MantidWidgets::InstrumentActor *instActor, V3D const &samplePos, V3D const &axis) {
  // We need to recreate the surface so disconnect any existing signals first
  if (m_instDisplay->getSurface()) {
    disconnect(m_instDisplay->getSurface().get(), SIGNAL(shapeChangeFinished()));
  }
  m_instDisplay->setSurface(std::make_shared<MantidWidgets::UnwrappedCylinder>(instActor, samplePos, axis));
  connect(m_instDisplay->getSurface().get(), SIGNAL(shapeChangeFinished()), this, SLOT(onInstViewShapeChanged()));
}
void QtPreviewView::setInstViewZoomState(bool isChecked) { m_ui.iv_zoom_button->setDown(isChecked); }
void QtPreviewView::setInstViewEditState(bool isChecked) { m_ui.iv_edit_button->setDown(isChecked); }

void QtPreviewView::setInstViewSelectRectState(bool isChecked) { m_ui.iv_rect_select_button->setDown(isChecked); }

void QtPreviewView::setInstViewZoomMode() {
  m_instDisplay->getSurface()->setInteractionMode(ProjectionSurface::MoveMode);
}

void QtPreviewView::setInstViewEditMode() {
  m_instDisplay->getSurface()->setInteractionMode(ProjectionSurface::EditShapeMode);
}

void QtPreviewView::setInstViewSelectRectMode() {
  m_instDisplay->getSurface()->setInteractionMode(ProjectionSurface::EditShapeMode);
  m_instDisplay->getSurface()->startCreatingShape2D("rectangle", Qt::green, QColor(255, 255, 255, 80));
}

void QtPreviewView::setInstViewToolbarEnabled(bool enable) {
  m_ui.iv_zoom_button->setEnabled(enable);
  m_ui.iv_edit_button->setEnabled(enable);
  m_ui.iv_rect_select_button->setEnabled(enable);
}

std::vector<size_t> QtPreviewView::getSelectedDetectors() const {
  std::vector<size_t> result;
  // The name is confusing here but "masked" detectors refers to those selected by a "mask shape"
  // (although weather it's treated as a mask or not is up to the caller)
  m_instDisplay->getSurface()->getMaskedDetectors(result);
  return result;
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
