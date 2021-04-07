// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
  AlgorithmObserver(const IAlgorithm_const_sptr &alg);
  virtual ~AlgorithmObserver();

  void observeAll(const IAlgorithm_const_sptr &alg);
  void observeProgress(const IAlgorithm_const_sptr &alg);
  void observeStarting();
  void observeStart(const IAlgorithm_const_sptr &alg);
  void observeFinish(const IAlgorithm_const_sptr &alg);
  void observeError(const IAlgorithm_const_sptr &alg);

  void stopObserving(const IAlgorithm_const_sptr &alg);
  void stopObserving(const Mantid::API::IAlgorithm *alg);
  void stopObservingManager();

  virtual void progressHandle(const IAlgorithm *alg, double p, const std::string &msg, const double estimatedTime,
                              const int progressPrecision);
  virtual void startingHandle(IAlgorithm_sptr alg);
  virtual void startHandle(const IAlgorithm *alg);
  virtual void finishHandle(const IAlgorithm *alg);
  virtual void errorHandle(const IAlgorithm *alg, const std::string &what);

private:
  void _progressHandle(const Poco::AutoPtr<Algorithm::ProgressNotification> &pNf);
  /// Poco::NObserver for Algorithm::ProgressNotification.
  Poco::NObserver<AlgorithmObserver, Algorithm::ProgressNotification> m_progressObserver;

  void _startHandle(const Poco::AutoPtr<Algorithm::StartedNotification> &pNf);
  /// Poco::NObserver for Algorithm::StartedNotification.
  Poco::NObserver<AlgorithmObserver, Algorithm::StartedNotification> m_startObserver;

  void _finishHandle(const Poco::AutoPtr<Algorithm::FinishedNotification> &pNf);
  /// Poco::NObserver for Algorithm::FinishedNotification.
  Poco::NObserver<AlgorithmObserver, Algorithm::FinishedNotification> m_finishObserver;

  void _errorHandle(const Poco::AutoPtr<Algorithm::ErrorNotification> &pNf);
  /// Poco::NObserver for Algorithm::ErrorNotification.
  Poco::NObserver<AlgorithmObserver, Algorithm::ErrorNotification> m_errorObserver;

  void _startingHandle(const Poco::AutoPtr<AlgorithmStartingNotification> &pNf);
  /// Poco::NObserver for API::AlgorithmStartingNotification
  Poco::NObserver<AlgorithmObserver, AlgorithmStartingNotification> m_startingObserver;
};

} // namespace API
} // namespace Mantid
