// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressWidget.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogWidget.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenter.h"

#include <QProgressBar>

namespace MantidQt {
namespace MantidWidgets {

AlgorithmProgressWidget::AlgorithmProgressWidget(QWidget *parent)
    : QWidget(parent),
      m_progressBar{new QProgressBar(this)}, m_layout{new QHBoxLayout(this)},
      m_detailsButton{new QPushButton("Details")},
      m_presenter{std::make_unique<AlgorithmProgressPresenter>(parent, this)}

{
  m_progressBar->setAlignment(Qt::AlignHCenter);
  m_layout->addWidget(m_progressBar);
  m_layout->addWidget(m_detailsButton);
  this->setLayout(m_layout);

  connect(m_detailsButton, &QPushButton::clicked, this,
          &AlgorithmProgressWidget::showDetailsDialog);
}

void AlgorithmProgressWidget::algorithmStarted() {}

void AlgorithmProgressWidget::algorithmEnded() {
  m_progressBar->setValue(0);
  m_progressBar->setFormat("Idle.");
}

void AlgorithmProgressWidget::showDetailsDialog() {
  // If the dialog exist but is not visible, then just show it again
  if (m_details && !m_details->isVisible()) {
    m_details->show();
  } else if (!m_details) {
    // If the dialog does not exist we create it. The dialog has the attribute
    // DeleteOnClose so it will delete itself when the user closes it
    const auto parent = this->parentWidget();
    m_details = new AlgorithmProgressDialogWidget(parent, m_presenter->model());
    // the widget will be deleted when the closeEvent triggers, the
    // QPointer that is storing it will automatically set it to nullptr
    // when the widget deletes itself, so there will be no dangling pointer
    m_details->setAttribute(Qt::WA_DeleteOnClose);
    m_details->show();
  }
}

QProgressBar *AlgorithmProgressWidget::progressBar() const {
  return m_progressBar;
}

void AlgorithmProgressWidget::blockUpdates(bool block) {
	m_presenter->blockSignals(block);
}

} // namespace MantidWidgets
} // namespace MantidQt