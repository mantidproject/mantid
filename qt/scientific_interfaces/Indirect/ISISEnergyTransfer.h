// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ISISENERGYTRANSFER_H_
#define MANTIDQTCUSTOMINTERFACES_ISISENERGYTRANSFER_H_

#include "IndirectDataReductionTab.h"
#include "MantidKernel/System.h"
#include "ui_ISISEnergyTransfer.h"

namespace MantidQt {
namespace CustomInterfaces {
/** ISISEnergyTransfer
  Handles an energy transfer reduction for ISIS instruments.

  @author Dan Nixon
  @date 23/07/2014
*/
class DLLExport ISISEnergyTransfer : public IndirectDataReductionTab {
  Q_OBJECT

public:
  ISISEnergyTransfer(IndirectDataReduction *idrUI, QWidget *parent = nullptr);
  ~ISISEnergyTransfer() override;

  void setup() override;
  void run() override;

public slots:
  bool validate() override;

private slots:
  void algorithmComplete(bool error);

  void setCurrentGroupingOption(QString const &option);
  int getGroupingOptionIndex(QString const &option);
  bool isOptionHidden(QString const &option);
  void removeGroupingOption(QString const &option);
  void includeExtraGroupingOption(bool includeOption, QString const &option);

  void
  setInstrumentDefault(); ///< Sets default parameters for current instrument
  void mappingOptionSelected(
      const QString &groupType); ///< change ui to display appropriate options
  void plotRaw();                ///< plot raw data from instrument
  void
  pbRunEditing(); //< Called when a user starts to type / edit the runs to load.
  void pbRunFinding();  //< Called when the FileFinder starts finding the files.
  void pbRunFinished(); //< Called when the FileFinder has finished finding the
  // files.
  void plotRawComplete(
      bool error); //< Called when the Plot Raw algorithmm chain completes
  /// Handles running, plotting and saving
  void runClicked();
  void plotSpectrumClicked();
  void plotContourClicked();
  void saveClicked();

  void updateRunButton(bool enabled = true,
                       std::string const &enableOutputButtons = "unchanged",
                       QString const message = "Run",
                       QString const tooltip = "");

private:
  void setInstrumentDefault(QMap<QString, QString> const &instDetails);
  void setInstrumentCheckBoxProperty(QCheckBox *checkbox,
                                     QMap<QString, QString> const &instDetails,
                                     QString const &instrumentProperty);

  void setFileExtensionsByName(bool filter) override;

  std::pair<std::string, std::string> createMapFile(
      const std::string
          &groupType); ///< create the mapping file with which to group results

  void plotWorkspace(std::string const &workspaceName,
                     std::string const &plotType);
  void saveWorkspace(std::string const &workspaceName);

  bool numberInCorrectRange(std::size_t const &spectraNumber) const;
  QString checkCustomGroupingNumbersInRange(
      std::vector<std::size_t> const &customGroupingNumbers) const;
  QString validateDetectorGrouping() const;
  std::string getDetectorGroupingString() const;

  void loadDetailedBalance(std::string const &filename);

  void setRunEnabled(bool enable);
  void setPlotSpectrumEnabled(bool enable);
  void setPlotContourEnabled(bool enable);
  void setPlotTimeEnabled(bool enable);
  void setSaveEnabled(bool enable);
  void setButtonsEnabled(bool enable);
  void setPlotSpectrumIsPlotting(bool plotting);
  void setPlotContourIsPlotting(bool plotting);
  void setPlotTimeIsPlotting(bool plotting);

  std::string m_outputGroupName;
  std::vector<std::string> m_outputWorkspaces;

  Ui::ISISEnergyTransfer m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ISISENERGYTRANSFER_H_
