// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/IJobRunner.h"

#include <memory>

namespace MantidQt::CustomInterfaces {

class IALFAlgorithmManagerSubscriber;

class MANTIDQT_DIRECT_DLL IALFAlgorithmManager {

public:
  virtual ~IALFAlgorithmManager() = default;

  virtual void subscribe(IALFAlgorithmManagerSubscriber *subscriber) = 0;

  // The algorithms used to load and normalise the Sample
  virtual void load(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) = 0;
  virtual void normaliseByCurrent(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) = 0;
  virtual void rebinToWorkspace(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) = 0;
  virtual void divide(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) = 0;
  virtual void replaceSpecialValues(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) = 0;
  virtual void convertUnits(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) = 0;

  // The algorithms used to produce an Out of plane angle workspace
  virtual void createWorkspace(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) = 0;
  virtual void scaleX(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) = 0;
  virtual void rebunch(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) = 0;

  // The algorithms used for fitting the extracted Out of plane angle workspace
  virtual void cropWorkspace(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) = 0;
  virtual void fit(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) = 0;
};

class MANTIDQT_DIRECT_DLL ALFAlgorithmManager final : public IALFAlgorithmManager, public API::JobRunnerSubscriber {

public:
  ALFAlgorithmManager(std::unique_ptr<API::IJobRunner> jobRunner);

  void subscribe(IALFAlgorithmManagerSubscriber *subscriber) override;

  // The algorithms used to load and normalise the Sample
  void load(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) override;
  void normaliseByCurrent(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) override;
  void rebinToWorkspace(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) override;
  void divide(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) override;
  void replaceSpecialValues(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) override;
  void convertUnits(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) override;

  // The algorithms used to produce an Out of plane angle workspace
  void createWorkspace(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) override;
  void scaleX(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) override;
  void rebunch(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) override;

  // The algorithms used for fitting the extracted Out of plane angle workspace
  void cropWorkspace(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) override;
  void fit(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) override;

  void notifyBatchComplete(bool error) override { (void)error; };
  void notifyBatchCancelled() override {};
  void notifyAlgorithmStarted(API::IConfiguredAlgorithm_sptr &algorithm) override { (void)algorithm; };
  void notifyAlgorithmComplete(API::IConfiguredAlgorithm_sptr &algorithm) override;
  void notifyAlgorithmError(API::IConfiguredAlgorithm_sptr &algorithm, std::string const &message) override;

private:
  void executeAlgorithm(Mantid::API::IAlgorithm_sptr algorithm,
                        std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties);

  void notifyLoadComplete(Mantid::API::IAlgorithm_sptr const &algorithm);
  void notifyNormaliseComplete(Mantid::API::IAlgorithm_sptr const &algorithm);
  void notifyRebinToWorkspaceComplete(Mantid::API::IAlgorithm_sptr const &algorithm);
  void notifyDivideComplete(Mantid::API::IAlgorithm_sptr const &algorithm);
  void notifyReplaceSpecialValuesComplete(Mantid::API::IAlgorithm_sptr const &algorithm);
  void notifyConvertUnitsComplete(Mantid::API::IAlgorithm_sptr const &algorithm);

  void notifyCreateWorkspaceComplete(Mantid::API::IAlgorithm_sptr const &algorithm);
  void notifyScaleXComplete(Mantid::API::IAlgorithm_sptr const &algorithm);
  void notifyRebunchComplete(Mantid::API::IAlgorithm_sptr const &algorithm);

  void notifyCropWorkspaceComplete(Mantid::API::IAlgorithm_sptr const &algorithm);
  void notifyFitComplete(Mantid::API::IAlgorithm_sptr const &algorithm);

  std::unique_ptr<API::IJobRunner> m_jobRunner;
  IALFAlgorithmManagerSubscriber *m_subscriber;
};

} // namespace MantidQt::CustomInterfaces
