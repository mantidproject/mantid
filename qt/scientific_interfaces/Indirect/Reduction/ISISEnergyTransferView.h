// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "ISISEnergyTransferData.h"
#include "IndirectDataReduction.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "ui_ISISEnergyTransfer.h"

namespace MantidQt {
namespace CustomInterfaces {

class DetectorGroupingOptions;

class MANTIDQT_INDIRECT_DLL IETViewSubscriber {
public:
  virtual void notifyNewMessage(const QString &message) = 0;
  virtual void notifySaveClicked() = 0;
  virtual void notifyRunClicked() = 0;
  virtual void notifyPlotRawClicked() = 0;
  virtual void notifySaveCustomGroupingClicked(std::string const &customGrouping) = 0;
  virtual void notifyRunFinished() = 0;
};

class MANTIDQT_INDIRECT_DLL IETView : public QWidget {
  Q_OBJECT

public:
  IETView(IETViewSubscriber *subscriber, QWidget *parent = nullptr);
  ~IETView();

  IETRunData getRunData() const;
  IETPlotData getPlotData() const;
  IETSaveData getSaveData() const;

  std::string getGroupOutputOption() const;
  OutputPlotOptionsView *getPlotOptionsView() const;
  bool getGroupOutputCheckbox() const;

  std::string getFirstFilename() const;

  bool isRunFilesValid() const;
  void validateCalibrationFileType(UserInputValidator &uiv) const;
  void validateRebinString(UserInputValidator &uiv) const;

  bool showRebinWidthPrompt() const;
  void showSaveCustomGroupingDialog(std::string const &customGroupingOutput, std::string const &defaultGroupingFilename,
                                    std::string const &saveDirectory) const;
  void displayWarning(std::string const &message) const;

  void setCalibVisible(bool visible);
  void setEfixedVisible(bool visible);
  void setBackgroundSectionVisible(bool visible);
  void setPlotTimeSectionVisible(bool visible);
  void setAnalysisSectionVisible(bool visible);
  void setPlottingOptionsVisible(bool visible);
  void setAclimaxSaveVisible(bool visible);
  void setSPEVisible(bool visible);
  void setFoldMultipleFramesVisible(bool visible);
  void setOutputInCm1Visible(bool visible);
  void setGroupOutputCheckBoxVisible(bool visible);
  void setGroupOutputDropdownVisible(bool visible);

  void setDetailedBalance(double detailedBalance);
  void setRunFilesEnabled(bool enable);
  void setSingleRebin(bool enable);
  void setMultipleRebin(bool enable);
  void setSaveEnabled(bool enable);
  void setPlotTimeIsPlotting(bool plotting);
  void setFileExtensionsByName(QStringList calibrationFbSuffixes, QStringList calibrationWSSuffixes);
  void setOutputWorkspaces(std::vector<std::string> const &outputWorkspaces);

  void setInstrumentSpectraRange(int specMin, int specMax);
  void setInstrumentRebinning(QStringList const &rebinParams, QString const &rebinText, bool checked, int tabIndex);
  void setInstrumentEFixed(QString const &instrumentName, double eFixed);
  void setInstrumentGrouping(QString const &instrumentName);
  void setInstrumentSpecDefault(std::map<std::string, bool> &specMap);

public slots:
  void updateRunButton(bool enabled = true, std::string const &enableOutputButtons = "unchanged",
                       QString const &message = "Run", QString const &tooltip = "");

private slots:
  void showMessageBox(const QString &message);
  void saveClicked();
  void runClicked();
  void plotRawClicked();
  void saveCustomGroupingClicked(std::string const &customGrouping);
  void pbRunFinished();

  void handleDataReady();

  void pbRunEditing();
  void pbRunFinding();

private:
  void setRunEnabled(bool enable);
  void setPlotTimeEnabled(bool enable);
  void setButtonsEnabled(bool enable);

  std::vector<std::string> m_outputWorkspaces;
  Ui::ISISEnergyTransfer m_uiForm;
  IETViewSubscriber *m_subscriber;
  DetectorGroupingOptions *m_groupingWidget;
};
} // namespace CustomInterfaces
} // namespace MantidQt