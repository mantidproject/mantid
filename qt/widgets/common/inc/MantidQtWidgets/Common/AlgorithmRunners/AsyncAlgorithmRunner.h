// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidQtWidgets/Common/AlgorithmRunners/IAsyncAlgorithmRunner.h"
#include "MantidQtWidgets/Common/DllOption.h"

#include <Poco/NObserver.h>

#include <memory>

namespace MantidQt::API {

class IAsyncAlgorithmSubscriber;

class EXPORT_OPT_MANTIDQT_COMMON AsyncAlgorithmRunner final : public IAsyncAlgorithmRunner {

public:
  explicit AsyncAlgorithmRunner();
  ~AsyncAlgorithmRunner();

  void subscribe(IAsyncAlgorithmSubscriber *subscriber) override;

  virtual void cancelRunningAlgorithm();

  virtual void startAlgorithm(Mantid::API::IAlgorithm_sptr alg);
  virtual Mantid::API::IAlgorithm_sptr getAlgorithm() const;

private:
  void onAlgorithmFinished(const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification> &pNf);
  Poco::NObserver<AsyncAlgorithmRunner, Mantid::API::Algorithm::FinishedNotification> m_finishedObserver;

  void onAlgorithmProgress(const Poco::AutoPtr<Mantid::API::Algorithm::ProgressNotification> &pNf);
  Poco::NObserver<AsyncAlgorithmRunner, Mantid::API::Algorithm::ProgressNotification> m_progressObserver;

  void onAlgorithmError(const Poco::AutoPtr<Mantid::API::Algorithm::ErrorNotification> &pNf);
  Poco::NObserver<AsyncAlgorithmRunner, Mantid::API::Algorithm::ErrorNotification> m_errorObserver;

  Mantid::API::IAlgorithm_sptr m_algorithm;
  IAsyncAlgorithmSubscriber *m_subscriber;
};

} // namespace MantidQt::API
