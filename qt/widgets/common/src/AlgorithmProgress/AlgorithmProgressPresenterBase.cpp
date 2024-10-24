// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenterBase.h"
#include <QProgressBar>

namespace MantidQt::MantidWidgets {

AlgorithmProgressPresenterBase::AlgorithmProgressPresenterBase(QObject *parent) : QObject(parent) {
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
void AlgorithmProgressPresenterBase::algorithmStarted(Mantid::API::AlgorithmID alg) {
  emit algorithmStartedSignal(alg);
}
/// This function is called whenever an algorithm ended. It emits a signal
/// triggering the slot inside the GUI thread. It is safe to call this function
/// from any thread
/// @param alg The ID of the algorithm that has started executing
void AlgorithmProgressPresenterBase::algorithmEnded(Mantid::API::AlgorithmID alg) { emit algorithmEndedSignal(alg); }
/// Updates the parameter progress bar to display the progress. If there is a
/// message, the progress bar format is changed to display it as well.
/// @param progressBar The progress bar that will be updated
/// @param progress The progress that an algorithm has reported
/// @param message The message that an algorithm has sent. The object will
/// already have been copied into the GUI thread, so it is safe to receive it by
/// @param estimatedTime :: estimated time to completion in seconds
/// @param progressPrecision :: number of digits after the decimal
void AlgorithmProgressPresenterBase::setProgressBar(QProgressBar *progressBar, const double progress,
                                                    const QString &message, const double estimatedTime,
                                                    const int progressPrecision) {
  progressBar->setValue(static_cast<int>(progress * 100));
  // Make the progress string
  std::ostringstream mess;
  if (!message.isEmpty()) {
    mess << message.toStdString() << " ";
  }
  mess.precision(progressPrecision);
  mess << std::fixed << progress * 100 << "%";
  if (estimatedTime > 0.5) {
    mess.precision(0);
    mess << " (~";
    if (estimatedTime < 60)
      mess << static_cast<int>(estimatedTime) << "s";
    else if (estimatedTime < 60 * 60) {
      int min = static_cast<int>(estimatedTime / 60);
      int sec = static_cast<int>(estimatedTime - min * 60);
      mess << min << "m" << std::setfill('0') << std::setw(2) << sec << "s";
    } else {
      int hours = static_cast<int>(estimatedTime / 3600);
      int min = static_cast<int>((estimatedTime - hours * 3600) / 60);
      mess << hours << "h" << std::setfill('0') << std::setw(2) << min << "m";
    }
    mess << ")";
  }
  QString formatStr = QString::fromStdString(mess.str());
  progressBar->setFormat(formatStr);
}
/// This function is called whenever an algorithm reports progress. It emits a
/// signal triggering the slot inside the GUI thread. It is safe to call this
/// function from any thread
/// @param alg The ID of the algorithm that has started executing
/// @param progress The progress that the algorithm has reported
/// @param msg The message that the algorithm has sent
/// @param estimatedTime :: estimated time to completion in seconds
/// @param progressPrecision :: number of digits after the decimal
void AlgorithmProgressPresenterBase::updateProgressBar(Mantid::API::AlgorithmID alg, double progress,
                                                       const std::string &msg, const double estimatedTime,
                                                       const int progressPrecision) {
  emit updateProgressBarSignal(alg, progress, QString::fromStdString(msg), estimatedTime, progressPrecision);
}
} // namespace MantidQt::MantidWidgets
