// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunView.h"

#include "MantidQtWidgets/Spectroscopy/RunWidget/RunPresenter.h"

#include <QMessageBox>

namespace {

std::string const RUN_TEXT("Run");

}

namespace MantidQt {
namespace CustomInterfaces {

RunView::RunView(QWidget *parent) : QWidget(parent), m_presenter() {
  m_uiForm.setupUi(this);

  connect(m_uiForm.pbRun, &QPushButton::clicked, this, &RunView::notifyRunClicked);
}

void RunView::subscribePresenter(IRunPresenter *presenter) { m_presenter = presenter; }

void RunView::notifyRunClicked() { m_presenter->handleRunClicked(); }

void RunView::setRunText(std::string const &text) {
  m_uiForm.pbRun->setText(QString::fromStdString(text));
  m_uiForm.pbRun->setEnabled(text == RUN_TEXT);
}

void RunView::displayWarning(std::string const &message) {
  QMessageBox::warning(this, "Warning!", QString::fromStdString(message));
}

} // namespace CustomInterfaces
} // namespace MantidQt
