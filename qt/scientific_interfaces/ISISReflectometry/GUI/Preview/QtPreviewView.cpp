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

namespace MantidQt::CustomInterfaces::ISISReflectometry {
QtPreviewView::QtPreviewView(QWidget *parent)
    : QWidget(parent), m_imageInfo(new MantidQt::MantidWidgets::ImageInfoWidgetMini(this)) {
  m_ui.setupUi(this);
  m_ui.statusbar_layout->addWidget(m_imageInfo);
  m_ui.title_display_label->setWordWrap(true);
  connectSignals();
}

void QtPreviewView::subscribe(PreviewViewSubscriber *notifyee) noexcept { m_notifyee = notifyee; }

QLayout *QtPreviewView::getDockedWidgetsLayout() noexcept { return m_ui.dockable_widgets_layout; }

MantidWidgets::IImageInfoWidget *QtPreviewView::getImageInfo() noexcept { return m_imageInfo; }

void QtPreviewView::enableMainWidget() { this->setEnabled(true); }

void QtPreviewView::disableMainWidget() { this->setEnabled(false); }

void QtPreviewView::connectSignals() const {
  // Loading section
  connect(m_ui.load_button, SIGNAL(clicked()), this, SLOT(onLoadWorkspaceRequested()));
  connect(m_ui.update_button, SIGNAL(clicked()), this, SLOT(onUpdateClicked()));
  connect(m_ui.angle_spin_box, SIGNAL(valueChanged(double)), this, SLOT(onAngleEdited()));
  // Apply button
  connect(m_ui.apply_button, SIGNAL(clicked()), this, SLOT(onApplyClicked()));
}

void QtPreviewView::onLoadWorkspaceRequested() const { m_notifyee->notifyLoadWorkspaceRequested(); }
void QtPreviewView::onUpdateClicked() const { m_notifyee->notifyUpdateAngle(); }

void QtPreviewView::onAngleEdited() { m_ui.update_button->setEnabled(true); }

void QtPreviewView::onApplyClicked() const { m_notifyee->notifyApplyRequested(); }

std::string QtPreviewView::getWorkspaceName() const { return m_ui.workspace_line_edit->text().toStdString(); }

double QtPreviewView::getAngle() const { return m_ui.angle_spin_box->value(); }

void QtPreviewView::setAngle(double angle) {
  m_ui.angle_spin_box->blockSignals(true);
  m_ui.angle_spin_box->setValue(angle);
  m_ui.angle_spin_box->blockSignals(false);
}

void QtPreviewView::setUpdateAngleButtonEnabled(bool enabled) { m_ui.update_button->setEnabled(enabled); }

void QtPreviewView::setTitle(const std::string &title) {
  m_ui.title_display_label->setText(QString::fromStdString(title));
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
