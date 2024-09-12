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
#include "ui_DensityOfStates.h"

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_INDIRECT_DLL DensityOfStates : public SimulationTab, public IRunSubscriber {
  Q_OBJECT

public:
  DensityOfStates(QWidget *parent = nullptr);

  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

  void handleValidation(IUserInputValidator *validator) const override;
  void handleRun() override;
  const std::string getSubscriberName() const override { return "DensityOfStates"; }

private slots:
  void dosAlgoComplete(bool error);
  void handleFileChange();
  void ionLoadComplete(bool error);
  void saveClicked();

private:
  void setSaveEnabled(bool enabled);

  enum class InputFormat : int;
  InputFormat filenameToFormat(std::string const &filename) const;
  std::string formatToFilePropName(InputFormat const &format) const;
  bool isPdosFile(InputFormat const &dosFileFormat) const;

  /// The ui form
  Ui::DensityOfStates m_uiForm;
  /// Name of output workspace
  QString m_outputWsName;
};
} // namespace CustomInterfaces
} // namespace MantidQt
