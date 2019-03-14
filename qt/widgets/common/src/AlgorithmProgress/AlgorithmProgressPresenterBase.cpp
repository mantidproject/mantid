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

AlgorithmProgressPresenterBase::AlgorithmProgressPresenterBase(QObject *parent)
    : QObject(parent) {
  const auto connection = Qt::QueuedConnection;
  connect(this, &AlgorithmProgressPresenterBase::algorithmStartedSignal, this,
          &AlgorithmProgressPresenterBase::algorithmStartedSlot, connection);

  connect(this, &AlgorithmProgressPresenterBase::algorithmEndedSignal, this,
          &AlgorithmProgressPresenterBase::algorithmEndedSlot, connection);

  connect(this, &AlgorithmProgressPresenterBase::updateProgressBarSignal, this,
          &AlgorithmProgressPresenterBase::updateProgressBarSlot, connection);
}

/// This function is called whenever an algorithm started. It emits an algorithm
/// started signal, which is allows the GUIs to be updated, by triggering the
/// slot in the GUI thread. It is safe to call this function from any thread
/// @param alg The ID of the algorithm that has started executing
void AlgorithmProgressPresenterBase::algorithmStarted(
    Mantid::API::AlgorithmID alg) {
  emit algorithmStartedSignal(alg);
}
/// This function is called whenever an algorithm ended. It emits a signal
/// triggering the slot inside the GUI thread. It is safe to call this function
/// from any thread
/// @param alg The ID of the algorithm that has started executing
void AlgorithmProgressPresenterBase::algorithmEnded(
    Mantid::API::AlgorithmID alg) {
  emit algorithmEndedSignal(alg);
}
/// Updates the parameter progress bar to display the progress. If there is a
/// message, the progress bar format is changed to display it as well.
/// @param progressBar The progress bar that will be updated
/// @param progress The progress that an algorithm has reported
/// @param message The message that an algorithm has sent. The object will
/// already have been copied into the GUI thread, so it is safe to receive it by
/// const ref
void AlgorithmProgressPresenterBase::setProgressBar(QProgressBar *progressBar,
                                                    double progress,
                                                    const QString &message) {
  progressBar->setValue(static_cast<int>(progress * 100));

  // If the progress reporter has sent a message, format the bar to display that
  // as well as the % done
  if (!message.isEmpty()) {
    progressBar->setFormat(QString("%1 - %2").arg(message, "%p%"));
  } else {
    progressBar->setFormat("%p%");
  }
}
/// This function is called whenever an algorithm reports progress. It emits a
/// signal triggering the slot inside the GUI thread. It is safe to call this
/// function from any thread
/// @param alg The ID of the algorithm that has started executing
/// @param progress The progress that the algorithm has reported
/// @param msg The message that the algorithm has sent
void AlgorithmProgressPresenterBase::updateProgressBar(
    Mantid::API::AlgorithmID alg, double progress, const std::string &msg) {
  emit updateProgressBarSignal(alg, progress, QString::fromStdString(msg));
}
} // namespace MantidWidgets
} // namespace MantidQt