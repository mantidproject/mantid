// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DataReduction.h"
#include "DllConfig.h"
#include "ISISEnergyTransferData.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "ui_ISISEnergyTransfer.h"

namespace MantidQt {
namespace CustomInterfaces {

class DetectorGroupingOptions;
class IIETPresenter;
class IRunView;

class MANTIDQT_INDIRECT_DLL IIETView {
public:
  virtual ~IIETView() = default;

  virtual void subscribePresenter(IIETPresenter *presenter) = 0;

  virtual IETRunData getRunData() const = 0;
  virtual IETPlotData getPlotData() const = 0;
  virtual IETSaveData getSaveData() const = 0;

  virtual std::string getGroupOutputOption() const = 0;
  virtual IRunView *getRunView() const = 0;
  virtual IOutputPlotOptionsView *getPlotOptionsView() const = 0;
  virtual bool getGroupOutputCheckbox() const = 0;
  virtual IOutputNameView *getOutputName() const = 0;

  virtual std::string getFirstFilename() const = 0;

  virtual bool isRunFilesValid() const = 0;
  virtual void validateCalibrationFileType(IUserInputValidator *uiv) const = 0;
  virtual void validateRebinString(IUserInputValidator *uiv) const = 0;
  virtual std::optional<std::string> validateGroupingProperties(std::size_t const &spectraMin,
                                                                std::size_t const &spectraMax) const = 0;

  virtual bool showRebinWidthPrompt() const = 0;
  virtual void showSaveCustomGroupingDialog(std::string const &customGroupingOutput,
                                            std::string const &defaultGroupingFilename,
                                            std::string const &saveDirectory) const = 0;
  virtual void displayWarning(std::string const &message) const = 0;

  virtual void setCalibVisible(bool visible) = 0;
  virtual void setEfixedVisible(bool visible) = 0;
  virtual void setBackgroundSectionVisible(bool visible) = 0;
  virtual void setPlotTimeSectionVisible(bool visible) = 0;
  virtual void setAnalysisSectionVisible(bool visible) = 0;
  virtual void setPlottingOptionsVisible(bool visible) = 0;
  virtual void setAclimaxSaveVisible(bool visible) = 0;
  virtual void setSPEVisible(bool visible) = 0;
  virtual void setFoldMultipleFramesVisible(bool visible) = 0;
  virtual void setOutputInCm1Visible(bool visible) = 0;
  virtual void setGroupOutputCheckBoxVisible(bool visible) = 0;
  virtual void setGroupOutputDropdownVisible(bool visible) = 0;

  virtual void setDetailedBalance(double detailedBalance) = 0;
  virtual void setRunFilesEnabled(bool enable) = 0;
  virtual void setSingleRebin(bool enable) = 0;
  virtual void setMultipleRebin(bool enable) = 0;
  virtual void setSaveEnabled(bool enable) = 0;
  virtual void setPlotTimeIsPlotting(bool plotting) = 0;
  virtual void setFileExtensionsByName(QStringList calibrationFbSuffixes, QStringList calibrationWSSuffixes) = 0;
  virtual void setLoadHistory(bool doLoadHistory) = 0;
  virtual void setEnableOutputOptions(bool const enable) = 0;

  virtual void setInstrumentSpectraRange(int specMin, int specMax) = 0;
  virtual void setInstrumentRebinning(std::vector<double> const &rebinParams, std::string const &rebinText,
                                      bool checked, int tabIndex) = 0;
  virtual void setInstrumentEFixed(std::string const &instrumentName, double eFixed) = 0;
  virtual void setInstrumentGrouping(std::string const &instrumentName) = 0;
  virtual void setInstrumentSpecDefault(std::map<std::string, bool> &specMap) = 0;

  virtual void showMessageBox(std::string const &message) = 0;
};

class MANTIDQT_INDIRECT_DLL IETView : public QWidget, public IIETView {
  Q_OBJECT

public:
  IETView(QWidget *parent = nullptr);

  void subscribePresenter(IIETPresenter *presenter) override;

  IETRunData getRunData() const override;
  IETPlotData getPlotData() const override;
  IETSaveData getSaveData() const override;

  std::string getGroupOutputOption() const override;
  IRunView *getRunView() const override;
  IOutputPlotOptionsView *getPlotOptionsView() const override;
  bool getGroupOutputCheckbox() const override;
  IOutputNameView *getOutputName() const override;

  std::string getFirstFilename() const override;

  bool isRunFilesValid() const override;
  void validateCalibrationFileType(IUserInputValidator *uiv) const override;
  void validateRebinString(IUserInputValidator *uiv) const override;
  std::optional<std::string> validateGroupingProperties(std::size_t const &spectraMin,
                                                        std::size_t const &spectraMax) const override;

  bool showRebinWidthPrompt() const override;
  void showSaveCustomGroupingDialog(std::string const &customGroupingOutput, std::string const &defaultGroupingFilename,
                                    std::string const &saveDirectory) const override;
  void displayWarning(std::string const &message) const override;

  void setCalibVisible(bool visible) override;
  void setEfixedVisible(bool visible) override;
  void setBackgroundSectionVisible(bool visible) override;
  void setPlotTimeSectionVisible(bool visible) override;
  void setAnalysisSectionVisible(bool visible) override;
  void setPlottingOptionsVisible(bool visible) override;
  void setAclimaxSaveVisible(bool visible) override;
  void setSPEVisible(bool visible) override;
  void setFoldMultipleFramesVisible(bool visible) override;
  void setOutputInCm1Visible(bool visible) override;
  void setGroupOutputCheckBoxVisible(bool visible) override;
  void setGroupOutputDropdownVisible(bool visible) override;

  void setDetailedBalance(double detailedBalance) override;
  void setRunFilesEnabled(bool enable) override;
  void setSingleRebin(bool enable) override;
  void setMultipleRebin(bool enable) override;
  void setSaveEnabled(bool enable) override;
  void setPlotTimeIsPlotting(bool plotting) override;
  void setFileExtensionsByName(QStringList calibrationFbSuffixes, QStringList calibrationWSSuffixes) override;
  void setLoadHistory(bool doLoadHistory) override;
  void setEnableOutputOptions(bool const enable) override;

  void setInstrumentSpectraRange(int specMin, int specMax) override;
  void setInstrumentRebinning(std::vector<double> const &rebinParams, std::string const &rebinText, bool checked,
                              int tabIndex) override;
  void setInstrumentEFixed(std::string const &instrumentName, double eFixed) override;
  void setInstrumentGrouping(std::string const &instrumentName) override;
  void setInstrumentSpecDefault(std::map<std::string, bool> &specMap) override;

  void showMessageBox(std::string const &message) override;

private slots:
  void saveClicked();
  void plotRawClicked();
  void saveCustomGroupingClicked(std::string const &customGrouping);
  void pbRunFinished();

  void handleDataReady();

  void pbRunFinding();

private:
  void setPlotTimeEnabled(bool enable);

  Ui::ISISEnergyTransfer m_uiForm;
  IIETPresenter *m_presenter;
  DetectorGroupingOptions *m_groupingWidget;
};
} // namespace CustomInterfaces
} // namespace MantidQt
