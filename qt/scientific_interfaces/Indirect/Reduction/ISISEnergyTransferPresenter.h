// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/IAlgorithmRunnerSubscriber.h"
#include "DllConfig.h"
#include "ISISEnergyTransferData.h"
#include "ISISEnergyTransferModel.h"
#include "ISISEnergyTransferView.h"

#include "DataReductionTab.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IIETPresenter {
public:
  virtual void notifySaveClicked() = 0;
  virtual void notifyRunClicked() = 0;
  virtual void notifyPlotRawClicked() = 0;
  virtual void notifySaveCustomGroupingClicked(std::string const &customGrouping) = 0;
  virtual void notifyRunFinished() = 0;
};

class MANTIDQT_INDIRECT_DLL IETPresenter : public DataReductionTab,
                                           public IIETPresenter,
                                           public IAlgorithmRunnerSubscriber {
  Q_OBJECT

public:
  IETPresenter(IDataReduction * idrUI, IIETView *view, std::unique_ptr<IIETModel> model,
               std::unique_ptr<IAlgorithmRunner> algorithmRunner);

  void setup() override;
  void run() override;
  bool validate() override;

  void notifySaveClicked() override;
  void notifyRunClicked() override;
  void notifyPlotRawClicked() override;
  void notifySaveCustomGroupingClicked(std::string const &customGrouping) override;
  void notifyRunFinished() override;

  void notifyAlgorithmComplete(API::IConfiguredAlgorithm_sptr &algorithm) override;
  void notifyAlgorithmError(API::IConfiguredAlgorithm_sptr &algorithm, std::string const &message) override;

private slots:
  void plotRawComplete(bool error);

private:
  bool validateInstrumentDetails();
  void updateInstrumentConfiguration() override;

  InstrumentData getInstrumentData();

  void setFileExtensionsByName(bool filter) override;

  IIETView *m_view;
  std::unique_ptr<IIETModel> m_model;
};
} // namespace CustomInterfaces
} // namespace MantidQt