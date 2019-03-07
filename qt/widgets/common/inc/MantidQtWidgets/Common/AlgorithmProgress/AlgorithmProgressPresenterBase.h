// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMPROGRESSPRESENTERBASE_H
#define ALGORITHMPROGRESSPRESENTERBASE_H

#include "MantidAPI/IAlgorithm_fwd.h"
#include <QObject>
#include <string>

namespace Mantid {
namespace API {
class IAlgorithm;
}
} // namespace Mantid

class QProgressBar;

namespace MantidQt {
namespace MantidWidgets {

class ProgressObserverData;
class AlgorithmProgressPresenterBase : public QObject {
  Q_OBJECT
public:
  AlgorithmProgressPresenterBase();
  virtual void updateProgressBar(Mantid::API::IAlgorithm_sptr alg, double p,
                                 const std::string &msg) = 0;

public slots:
  void setProgressBar(QProgressBar *progressBar, double progress,
                      const std::string &message);
  /// Emit an update signal to update the GUI in the Qt thread
  /// as this update is called from the AlgorithmObserver thread
  void update();

  /// Update the GUI that the presenter is currently using
  virtual void setCurrentAlgorithm() = 0;
signals:
  // When there has been a change in the observers, this signal
  // will trigger an update of the current algorithm being watched
  void updateWatchedAlgorithm();

  void progressBarNeedsUpdating(QProgressBar *, double, const std::string &);
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSPRESENTERBASE_H