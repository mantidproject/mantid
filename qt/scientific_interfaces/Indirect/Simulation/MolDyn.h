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
#include "ui_MolDyn.h"

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_INDIRECT_DLL MolDyn : public SimulationTab, public IRunSubscriber {
  Q_OBJECT

public:
  MolDyn(QWidget *parent = nullptr);

  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

  void handleValidation(IUserInputValidator *validator) const override;
  void handleRun() override;
  const std::string getSubscriberName() const override { return "MolDyn"; }

private slots:
  void versionSelected(const QString & /*version*/);
  void saveClicked();
  void algorithmComplete(bool error);

private:
  void setLoadHistory(bool doLoadHistory) override;
  void setSaveEnabled(bool enabled);

  std::string m_outputWsName;
  // The ui form
  Ui::MolDyn m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt
