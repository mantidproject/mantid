// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtPreviewView.h"

#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
QtPreviewView::QtPreviewView(QWidget *parent) : QWidget(parent), m_notifyee(nullptr) {
  m_ui.setupUi(this);
  connectSignals();
}

void QtPreviewView::subscribe(PreviewViewSubscriber *notifyee) noexcept { m_notifyee = notifyee; }

void QtPreviewView::connectSignals() const {
  connect(m_ui.load_button, SIGNAL(clicked()), this, SLOT(onLoadWorkspaceRequested()));
}

void QtPreviewView::onLoadWorkspaceRequested() const { m_notifyee->notifyLoadWorkspaceRequested(); }

std::string QtPreviewView::getWorkspaceName() const { return m_ui.workspace_line_edit->text().toStdString(); }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
