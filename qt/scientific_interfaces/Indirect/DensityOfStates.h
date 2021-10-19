// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectSimulationTab.h"
#include "ui_DensityOfStates.h"

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport DensityOfStates : public IndirectSimulationTab {
  Q_OBJECT

public:
  DensityOfStates(QWidget *parent = nullptr);

  QString help() { return "DensityOfStates"; };

  void setup() override;
  bool validate() override;
  void run() override;

  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

private slots:
  void dosAlgoComplete(bool error);
  void handleFileChange();
  void ionLoadComplete(bool error);
  void runClicked();
  void saveClicked();

private:
  void setRunIsRunning(bool running);
  void setButtonsEnabled(bool enabled);
  void setRunEnabled(bool enabled);
  void setSaveEnabled(bool enabled);

  enum class InputFormat : int;
  InputFormat filenameToFormat(QString filename);
  std::string formatToFilePropName(InputFormat format);
  bool isPdosFile(InputFormat dosFileFormat);

  /// The ui form
  Ui::DensityOfStates m_uiForm;
  /// Name of output workspace
  QString m_outputWsName;
};
} // namespace CustomInterfaces
} // namespace MantidQt
