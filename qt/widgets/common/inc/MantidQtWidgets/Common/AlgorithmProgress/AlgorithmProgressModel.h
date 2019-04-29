// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMPROGRESSMODEL_H
#define ALGORITHMPROGRESSMODEL_H

#include "MantidAPI/AlgorithmObserver.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenterBase.h"

#include <QPointer>

#include <string>

/**
 * The AlgorithmProgressModel keeps track of all active presenters that are
 * managing views with progress bars, and notifies them whenever they need to
 * update due to an algorithm's event. It tracks the starting of all algorithms,
 * and will set itself as observer on every started algorithm, tracking their
 * start, update, finish and error events.
 *
 * When an algorithm event fires it is on a separate thread from the views. An
 * action in the presenter is triggered, which then emits a signal, bringing
 * back the processing into the GUI thread, to do actual changes to GUI
 * elements.
 *
 * Note: this observer does NOT WAIT for the GUI event to finish. It triggers
 * the event, which is then queued in the Qt event loop. If the algorithm must
 * be used in the event, then the AlgorithmID can be used to retrieve a shared
 * pointer to it, which keeps the algorithm alive for the duration.
 */
namespace MantidQt {
namespace MantidWidgets {
class AlgorithmProgressPresenter;
class AlgorithmProgressDialogPresenter;
class AlgorithmProgressModel : public Mantid::API::AlgorithmObserver {
public:
  AlgorithmProgressModel(AlgorithmProgressPresenter *presenter);

  /// Observes ALL starting algorithms. Triggered when a new algorithm
  /// is constructed, and will attach itself as an observer
  /// Note: at this point the algorithm is not fully initialized
  void startingHandle(Mantid::API::IAlgorithm_sptr alg) override;
  /// Triggered when the algorithm is executed
  void startHandle(const Mantid::API::IAlgorithm *alg) override;
  /// Triggered when the algorithm is finished
  void finishHandle(const Mantid::API::IAlgorithm *alg) override;
  /// Triggered when the algorithm reports progress
  void progressHandle(const Mantid::API::IAlgorithm *alg, double progress,
                      const std::string &message) override;
  /// Triggered when the algorithm encounters an error
  void errorHandle(const Mantid::API::IAlgorithm *alg,
                   const std::string &what) override;
  /// Removes itself as an observer from the algorithm
  void removeFrom(const Mantid::API::IAlgorithm *alg);
  void setDialog(AlgorithmProgressDialogPresenter * /*presenter*/);

private:
  QPointer<AlgorithmProgressDialogPresenter> m_dialogPresenter;
  AlgorithmProgressPresenter *m_mainWindowPresenter;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSMODEL_H