// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressWidget.h"

namespace MantidQt {
namespace MantidWidgets {

AlgorithmProgressWidget::AlgorithmProgressWidget(QWidget *parent)
    : QWidget(parent), presenter{
                           std::make_unique<AlgorithmProgressPresenter>(this)} {

  m_detailsButton = new QPushButton("Details");
  layout = new QHBoxLayout(this);
  pb = new QProgressBar(this);
  pb->setAlignment(Qt::AlignHCenter);
  layout->addWidget(pb);
  layout->addStretch();
  layout->addWidget(m_detailsButton);
  this->setLayout(layout);

  // Connect the signal to update the progress bar, this signal can be
  // triggered from any thread and will correctly update the progress bar
  // that is inside the Qt GUI thread
  connect(this, SIGNAL(updateProgressBar(double)), this,
          SLOT(setProgressBarValue(double)));
}
void AlgorithmProgressWidget::setProgressBarValue(const double progress) {
  pb->setValue(progress * 100);
}

} // namespace MantidWidgets
} // namespace MantidQt