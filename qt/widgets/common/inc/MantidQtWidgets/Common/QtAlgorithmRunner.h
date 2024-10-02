// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/Algorithm.h"

#include <Poco/NObserver.h>
#include <QObject>

namespace MantidQt {
namespace API {
/** The QtAlgorithmRunner is a QObject that encapsulates
 * methods for running an algorithm asynchronously (in the background)
 * and feeds-back to a GUI widget.
 *
 * The QObject keeps track of a running algorithm.
 * Any already-running algorithm is cancelled if it gets started again.
 * Signals are emitted when the algorithm progresses or finishes.
 *
 * TO USE:
 *  - Create the QtAlgorithmRunner object.
 *  - Connect the desired signal(s) to slots on your GUI.
 *  - Call startAlgorithm() to start.

  @date 2012-04-23
*/
class EXPORT_OPT_MANTIDQT_COMMON QtAlgorithmRunner : public QObject {
  Q_OBJECT

public:
  explicit QtAlgorithmRunner(QObject *parent = nullptr);
  ~QtAlgorithmRunner() override;

  void cancelRunningAlgorithm();

  virtual void startAlgorithm(Mantid::API::IAlgorithm_sptr alg);
  virtual Mantid::API::IAlgorithm_sptr getAlgorithm() const;

signals:
  /// Signal emitted when the algorithm has completed execution/encountered an
  /// error
  void algorithmComplete(bool error);

  /// Signal emitted when the algorithm reports progress
  void algorithmProgress(double p, const std::string &msg);

protected:
  /// Algorithm notification handlers
  void handleAlgorithmFinishedNotification(const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification> &pNf);
  Poco::NObserver<QtAlgorithmRunner, Mantid::API::Algorithm::FinishedNotification> m_finishedObserver;

  void handleAlgorithmProgressNotification(const Poco::AutoPtr<Mantid::API::Algorithm::ProgressNotification> &pNf);
  Poco::NObserver<QtAlgorithmRunner, Mantid::API::Algorithm::ProgressNotification> m_progressObserver;

  void handleAlgorithmErrorNotification(const Poco::AutoPtr<Mantid::API::Algorithm::ErrorNotification> &pNf);
  Poco::NObserver<QtAlgorithmRunner, Mantid::API::Algorithm::ErrorNotification> m_errorObserver;

  /// For the asynchronous call in dynamic rebinning. Holds the result of
  /// asyncExecute() algorithm call
  Poco::ActiveResult<bool> *m_asyncResult;

  /// Reference to the algorithm executing asynchronously.
  Mantid::API::IAlgorithm_sptr m_asyncAlg;
};

} // namespace API
} // namespace MantidQt
