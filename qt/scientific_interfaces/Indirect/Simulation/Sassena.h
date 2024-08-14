// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "SimulationTab.h"
#include "ui_Sassena.h"

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_INDIRECT_DLL Sassena : public SimulationTab, public IRunSubscriber {
  Q_OBJECT

public:
  Sassena(QWidget *parent = nullptr);

  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

  void handleValidation(IUserInputValidator *validator) const override;
  void handleRun() override;
  const std::string getSubscriberName() const override { return "Sassena"; }

private slots:
  /// Handle completion of the algorithm batch
  void handleAlgorithmFinish(bool error);
  void saveClicked();

private:
  void setSaveEnabled(bool enabled);

  /// The ui form
  Ui::Sassena m_uiForm;
  /// Name of the output workspace group
  QString m_outWsName;
};
} // namespace CustomInterfaces
} // namespace MantidQt
