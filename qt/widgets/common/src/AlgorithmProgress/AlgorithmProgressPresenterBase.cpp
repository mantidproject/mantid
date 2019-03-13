// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenterBase.h"
#include <QProgressBar>

namespace MantidQt {
namespace MantidWidgets {

AlgorithmProgressPresenterBase::AlgorithmProgressPresenterBase(QObject*parent)
    : QObject(parent) {
  const auto connection = Qt::QueuedConnection;
  connect(this, &AlgorithmProgressPresenterBase::algorithmStartedSignal, this,
          &AlgorithmProgressPresenterBase::algorithmStartedSlot, connection);

  connect(this, &AlgorithmProgressPresenterBase::algorithmEndedSignal, this,
          &AlgorithmProgressPresenterBase::algorithmEndedSlot, connection);

  connect(this, &AlgorithmProgressPresenterBase::updateProgressBarSignal, this,
          &AlgorithmProgressPresenterBase::updateProgressBarSlot, connection);
}

void AlgorithmProgressPresenterBase::algorithmStarted(
    Mantid::API::AlgorithmID alg) {
  // all the parameters are copied into the slot
  emit algorithmStartedSignal(alg);
}

void AlgorithmProgressPresenterBase::algorithmEnded(
    Mantid::API::AlgorithmID alg) {
  // all the parameters are copied into the slot
  emit algorithmEndedSignal(alg);
}

void AlgorithmProgressPresenterBase::setProgressBar(QProgressBar *progressBar,
                                                    double progress,
                                                    QString message) {
  progressBar->setValue(static_cast<int>(progress * 100));

  // If the progress reporter has sent a message, format the bar to display that
  // as well as the % done
  if (!message.isEmpty()) {
    progressBar->setFormat(QString("%1 - %2").arg(message, "%p%"));
  } else {
    progressBar->setFormat("%p%");
  }
}

void AlgorithmProgressPresenterBase::updateProgressBar(
    Mantid::API::AlgorithmID alg, double progress, const std::string &msg) {
  emit updateProgressBarSignal(alg, progress, QString::fromStdString(msg));
}
} // namespace MantidWidgets
} // namespace MantidQt