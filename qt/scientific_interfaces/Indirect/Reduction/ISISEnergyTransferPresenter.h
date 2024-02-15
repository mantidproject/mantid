// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "ISISEnergyTransferData.h"
#include "ISISEnergyTransferModel.h"
#include "ISISEnergyTransferView.h"

#include "IndirectDataReductionTab.h"

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_INDIRECT_DLL IETPresenter : public IndirectDataReductionTab, public IETViewSubscriber {
  Q_OBJECT

public:
  IETPresenter(IndirectDataReduction *idrUI, QWidget *parent = nullptr);
  ~IETPresenter() override;

  void setup() override;
  void run() override;

  void notifyNewMessage(const QString &message) override;
  void notifySaveClicked() override;
  void notifyRunClicked() override;
  void notifyPlotRawClicked() override;
  void notifySaveCustomGroupingClicked(std::string const &customGrouping) override;
  void notifyRunFinished() override;

public slots:
  bool validate() override;

private slots:
  void algorithmComplete(bool error);
  void plotRawComplete(bool error);

  void updateRunButton(bool enabled = true, std::string const &enableOutputButtons = "unchanged",
                       QString const &message = "Run", QString const &tooltip = "");
  void setInstrumentDefault();

private:
  bool validateInstrumentDetails();

  InstrumentData getInstrumentData();

  void setFileExtensionsByName(bool filter) override;

  std::string m_outputGroupName;
  std::vector<std::string> m_outputWorkspaces;

  std::unique_ptr<IETModel> m_model;
  std::unique_ptr<IETView> m_view;
};
} // namespace CustomInterfaces
} // namespace MantidQt