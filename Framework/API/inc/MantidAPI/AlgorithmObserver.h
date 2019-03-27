// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_ALGORITHMOBSERVER_H_
#define MANTID_API_ALGORITHMOBSERVER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/DllConfig.h"
#include <Poco/NObserver.h>

namespace Mantid {
namespace API {
/**
Observes Algorithm notifications: start,progress,finish,error.
Hides Poco::Notification API from the user.
*/
class MANTID_API_DLL AlgorithmObserver {
public:
  AlgorithmObserver();
  AlgorithmObserver(IAlgorithm_const_sptr alg);
  virtual ~AlgorithmObserver();

  void observeAll(IAlgorithm_const_sptr alg);
  void observeProgress(IAlgorithm_const_sptr alg);
  void observeStarting();
  void observeStart(IAlgorithm_const_sptr alg);
  void observeFinish(IAlgorithm_const_sptr alg);
  void observeError(IAlgorithm_const_sptr alg);

  void stopObserving(IAlgorithm_const_sptr alg);
  void stopObserving(const Mantid::API::IAlgorithm *alg);
  void stopObservingManager();

  virtual void progressHandle(const IAlgorithm *alg, double p,
                              const std::string &msg);
  virtual void startingHandle(IAlgorithm_sptr alg);
  virtual void startHandle(const IAlgorithm *alg);
  virtual void finishHandle(const IAlgorithm *alg);
  virtual void errorHandle(const IAlgorithm *alg, const std::string &what);

private:
  void
  _progressHandle(const Poco::AutoPtr<Algorithm::ProgressNotification> &pNf);
  /// Poco::NObserver for Algorithm::ProgressNotification.
  Poco::NObserver<AlgorithmObserver, Algorithm::ProgressNotification>
      m_progressObserver;

  void _startHandle(const Poco::AutoPtr<Algorithm::StartedNotification> &pNf);
  /// Poco::NObserver for Algorithm::StartedNotification.
  Poco::NObserver<AlgorithmObserver, Algorithm::StartedNotification>
      m_startObserver;

  void _finishHandle(const Poco::AutoPtr<Algorithm::FinishedNotification> &pNf);
  /// Poco::NObserver for Algorithm::FinishedNotification.
  Poco::NObserver<AlgorithmObserver, Algorithm::FinishedNotification>
      m_finishObserver;

  void _errorHandle(const Poco::AutoPtr<Algorithm::ErrorNotification> &pNf);
  /// Poco::NObserver for Algorithm::ErrorNotification.
  Poco::NObserver<AlgorithmObserver, Algorithm::ErrorNotification>
      m_errorObserver;

  void _startingHandle(const Poco::AutoPtr<AlgorithmStartingNotification> &pNf);
  /// Poco::NObserver for API::AlgorithmStartingNotification
  Poco::NObserver<AlgorithmObserver, AlgorithmStartingNotification>
      m_startingObserver;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_ALGORITHMOBSERVER_H_*/
