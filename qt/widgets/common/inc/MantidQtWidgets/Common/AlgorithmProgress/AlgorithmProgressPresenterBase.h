// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMPROGRESSPRESENTERBASE_H
#define ALGORITHMPROGRESSPRESENTERBASE_H

#include "MantidAPI/Algorithm.h"

#include <QObject>

#include <string>

namespace Mantid {
namespace API {
class IAlgorithm;
}
} // namespace Mantid

class QProgressBar;

/**
 * The AlgorithmProgressPresenterBase is the base class that presenters showing
 * progress bars use. It sets up the common connections for events from the
 * model, e.g. when an algorithm has progressed and the progress bar value needs
 * to be updated.
 */

namespace MantidQt {
namespace MantidWidgets {
class ProgressObserverData;
class AlgorithmProgressPresenterBase : public QObject {
  Q_OBJECT
public:
  AlgorithmProgressPresenterBase(QObject *parent);

  /// Signals to the presenters that an algorithm has started.
  void algorithmStarted(Mantid::API::AlgorithmID /*alg*/);
  /// Signals to the presenters that an algorithm has ended.
  void algorithmEnded(Mantid::API::AlgorithmID /*alg*/);
  /// Signals to the presenters that there has been progress in one of the
  /// algorithms
  void updateProgressBar(Mantid::API::AlgorithmID /*alg*/, double /*progress*/,
                         const std::string & /*msg*/);
  /// Sets the parameter progress bar to show the progress and message
  void setProgressBar(QProgressBar * /*progressBar*/, double /*progress*/,
                      const QString & /*message*/);

public slots:
  virtual void algorithmStartedSlot(Mantid::API::AlgorithmID) = 0;
  virtual void updateProgressBarSlot(Mantid::API::AlgorithmID, double,
                                     QString) = 0;
  virtual void algorithmEndedSlot(Mantid::API::AlgorithmID) = 0;

signals:
  void algorithmStartedSignal(Mantid::API::AlgorithmID /*_t1*/);
  void updateProgressBarSignal(Mantid::API::AlgorithmID /*_t1*/, double /*_t2*/,
                               QString /*_t3*/);
  void algorithmEndedSignal(Mantid::API::AlgorithmID /*_t1*/);
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSPRESENTERBASE_H