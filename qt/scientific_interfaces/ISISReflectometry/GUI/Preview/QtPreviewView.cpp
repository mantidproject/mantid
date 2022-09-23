// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtPreviewView.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtIcons/Icon.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedCylinder.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"
#include "ROIType.h"

#include <QAction>
#include <QMenu>

#include <string>

using namespace Mantid::Kernel;
using MantidQt::MantidWidgets::IPlotView;
using MantidQt::MantidWidgets::ProjectionSurface;

namespace MantidQt::CustomInterfaces::ISISReflectometry {
QtPreviewView::QtPreviewView(QWidget *parent) : QWidget(parent) {
  m_ui.setupUi(this);

  resetInstView();
  loadToolbarIcons();
  setupSelectRegionTypes();
  connectSignals();
}

void QtPreviewView::loadToolbarIcons() {
  m_ui.iv_zoom_button->setIcon(MantidQt::Icons::getIcon("mdi.magnify", "black", 1.3));
  m_ui.iv_edit_button->setIcon(MantidQt::Icons::getIcon("mdi.pencil", "black", 1.3));
  m_ui.iv_rect_select_button->setIcon(MantidQt::Icons::getIcon("mdi.selection", "black", 1.3));
  m_ui.rs_ads_export_button->setIcon(MantidQt::Icons::getIcon("mdi.file-export", "black", 1.3));
  m_ui.rs_edit_button->setIcon(MantidQt::Icons::getIcon("mdi.pencil", "black", 1.3));
  m_ui.lp_ads_export_button->setIcon(MantidQt::Icons::getIcon("mdi.file-export", "black", 1.3));
}

void QtPreviewView::setupSelectRegionTypes() {
  QMenu *menu = new QMenu();
  QAction *signalAction = new QAction(
      MantidQt::Icons::getIcon("mdi.selection", QString::fromStdString(roiTypeToColor(ROIType::Signal)), 1.3),
      QString::fromStdString(roiTypeToString(ROIType::Signal)));
  QAction *backgroundAction = new QAction(
      MantidQt::Icons::getIcon("mdi.selection", QString::fromStdString(roiTypeToColor(ROIType::Background)), 1.3),
      QString::fromStdString(roiTypeToString(ROIType::Background)));
  QAction *transmissionAction = new QAction(
      MantidQt::Icons::getIcon("mdi.selection", QString::fromStdString(roiTypeToColor(ROIType::Transmission)), 1.3),
      QString::fromStdString(roiTypeToString(ROIType::Transmission)));

  signalAction->setToolTip("Add rectangular signal region");
  backgroundAction->setToolTip("Add rectangular background region");
  transmissionAction->setToolTip("Add rectangular transmission region");

  menu->addAction(signalAction);
  menu->addAction(backgroundAction);
  menu->addAction(transmissionAction);

  m_ui.rs_rect_select_button->setMenu(menu);
  m_ui.rs_rect_select_button->setDefaultAction(signalAction);

  connect(menu, SIGNAL(triggered(QAction *)), this, SLOT(onAddRectangularROIClicked(QAction *)));
}

void QtPreviewView::subscribe(PreviewViewSubscriber *notifyee) noexcept { m_notifyee = notifyee; }

void QtPreviewView::enableApplyButton() { m_ui.apply_button->setEnabled(true); }

void QtPreviewView::disableApplyButton() { m_ui.apply_button->setEnabled(false); }

void QtPreviewView::connectSignals() const {
  // Loading section
  connect(m_ui.load_button, SIGNAL(clicked()), this, SLOT(onLoadWorkspaceRequested()));
  connect(m_ui.update_button, SIGNAL(clicked()), this, SLOT(onUpdateClicked()));
  connect(m_ui.angle_spin_box, SIGNAL(valueChanged(double)), this, SLOT(onAngleEdited()));
  // Instrument viewer toolbar
  connect(m_ui.iv_zoom_button, SIGNAL(clicked()), this, SLOT(onInstViewZoomClicked()));
  connect(m_ui.iv_edit_button, SIGNAL(clicked()), this, SLOT(onInstViewEditClicked()));
  connect(m_ui.iv_rect_select_button, SIGNAL(clicked()), this, SLOT(onInstViewSelectRectClicked()));
  // Region selector toolbar
  connect(m_ui.rs_ads_export_button, SIGNAL(clicked()), this, SLOT(onRegionSelectorExportToAdsClicked()));
  connect(m_ui.rs_edit_button, SIGNAL(clicked()), this, SLOT(onEditROIClicked()));
  // Line plot toolbar
  connect(m_ui.lp_ads_export_button, SIGNAL(clicked()), this, SLOT(onLinePlotExportToAdsClicked()));
  // Apply button
  connect(m_ui.apply_button, SIGNAL(clicked()), this, SLOT(onApplyClicked()));
}

void QtPreviewView::onLoadWorkspaceRequested() const { m_notifyee->notifyLoadWorkspaceRequested(); }
void QtPreviewView::onUpdateClicked() const { m_notifyee->notifyUpdateAngle(); }

void QtPreviewView::onInstViewZoomClicked() const { m_notifyee->notifyInstViewZoomRequested(); }
void QtPreviewView::onInstViewEditClicked() const { m_notifyee->notifyInstViewEditRequested(); }
void QtPreviewView::onInstViewSelectRectClicked() const { m_notifyee->notifyInstViewSelectRectRequested(); }
void QtPreviewView::onInstViewShapeChanged() const { m_notifyee->notifyInstViewShapeChanged(); }

void QtPreviewView::onRegionSelectorExportToAdsClicked() const { m_notifyee->notifyRegionSelectorExportAdsRequested(); }

void QtPreviewView::onEditROIClicked() const { m_notifyee->notifyEditROIModeRequested(); }

void QtPreviewView::onAddRectangularROIClicked(QAction *regionType) const {
  m_ui.rs_rect_select_button->setDefaultAction(regionType);
  m_notifyee->notifyRectangularROIModeRequested();
}

void QtPreviewView::onLinePlotExportToAdsClicked() const { m_notifyee->notifyLinePlotExportAdsRequested(); }

void QtPreviewView::onAngleEdited() { m_ui.update_button->setEnabled(true); }

void QtPreviewView::onApplyClicked() const { m_notifyee->notifyApplyRequested(); }

std::string QtPreviewView::getWorkspaceName() const { return m_ui.workspace_line_edit->text().toStdString(); }

double QtPreviewView::getAngle() const { return m_ui.angle_spin_box->value(); }

void QtPreviewView::resetInstView() {
  if (m_instDisplay) {
    // Destruct the previous instance
    m_instDisplay = nullptr;
  }
  m_instDisplay = std::make_unique<MantidWidgets::InstrumentDisplay>(m_ui.inst_view_placeholder);
}

void QtPreviewView::plotInstView(MantidWidgets::InstrumentActor *instActor, V3D const &samplePos, V3D const &axis) {
  // We need to recreate the surface so disconnect any existing signals first
  if (m_instDisplay->getSurface()) {
    disconnect(m_instDisplay->getSurface().get(), SIGNAL(shapeChangeFinished()));
  }
  m_instDisplay->setSurface(std::make_shared<MantidWidgets::UnwrappedCylinder>(instActor, samplePos, axis));
  connect(m_instDisplay->getSurface().get(), SIGNAL(shapeChangeFinished()), this, SLOT(onInstViewShapeChanged()));
}

QLayout *QtPreviewView::getRegionSelectorLayout() const { return m_ui.rs_plot_layout; }

IPlotView *QtPreviewView::getLinePlotView() const { return m_ui.line_plot; }

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

void QtPreviewView::setRegionSelectorToolbarEnabled(bool enable) {
  m_ui.rs_ads_export_button->setEnabled(enable);
  m_ui.rs_edit_button->setEnabled(enable);
  m_ui.rs_rect_select_button->setEnabled(enable);
}

void QtPreviewView::setAngle(double angle) {
  m_ui.angle_spin_box->blockSignals(true);
  m_ui.angle_spin_box->setValue(angle);
  m_ui.angle_spin_box->blockSignals(false);
}

void QtPreviewView::setUpdateAngleButtonEnabled(bool enabled) { m_ui.update_button->setEnabled(enabled); }

void QtPreviewView::setEditROIState(bool state) { m_ui.rs_edit_button->setDown(state); }

void QtPreviewView::setRectangularROIState(bool state) { m_ui.rs_rect_select_button->setDown(state); }

std::vector<size_t> QtPreviewView::getSelectedDetectors() const {
  std::vector<size_t> result;
  // The name is confusing here but "masked" detectors refers to those selected by a "mask shape"
  // (although weather it's treated as a mask or not is up to the caller)
  m_instDisplay->getSurface()->getMaskedDetectors(result);
  return result;
}

std::string QtPreviewView::getRegionType() const {
  return m_ui.rs_rect_select_button->defaultAction()->text().toStdString();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
