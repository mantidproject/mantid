// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IAsyncAlgorithmRunner.h"

#include <Poco/NObserver.h>

#include <memory>

namespace Mantid::API {

class IAsyncAlgorithmSubscriber;

class MANTID_API_DLL AsyncAlgorithmRunner final : public IAsyncAlgorithmRunner {

public:
  explicit AsyncAlgorithmRunner();
  ~AsyncAlgorithmRunner();

  void subscribe(IAsyncAlgorithmSubscriber *subscriber) override;

  void cancelRunningAlgorithm() override;

  void startAlgorithm(IAlgorithm_sptr alg) override;
  IAlgorithm_sptr getAlgorithm() const override;

private:
  void onAlgorithmProgress(const Poco::AutoPtr<Algorithm::ProgressNotification> &pNf);
  void onAlgorithmError(const Poco::AutoPtr<Algorithm::ErrorNotification> &pNf);
  void onAlgorithmFinished(const Poco::AutoPtr<Algorithm::FinishedNotification> &pNf);

  Poco::NObserver<AsyncAlgorithmRunner, Algorithm::ProgressNotification> m_progressObserver;
  Poco::NObserver<AsyncAlgorithmRunner, Algorithm::ErrorNotification> m_errorObserver;
  Poco::NObserver<AsyncAlgorithmRunner, Algorithm::FinishedNotification> m_finishedObserver;

  IAlgorithm_sptr m_algorithm;
  IAsyncAlgorithmSubscriber *m_subscriber;
};

} // namespace Mantid::API
