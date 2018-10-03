// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTDIFFRACTIONREDUCTION_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTDIFFRACTIONREDUCTION_H_

//----------------------
// Includes
//----------------------
#include "ui_IndirectDiffractionReduction.h"

#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

namespace MantidQt {
namespace CustomInterfaces {
class IndirectDiffractionReduction : public MantidQt::API::UserSubWindow {
  Q_OBJECT

public:
  /// The name of the interface as registered into the factory
  static std::string name() { return "Diffraction"; }
  // This interface's categories.
  static QString categoryInfo() { return "Indirect"; }

public:
  /// Default Constructor
  explicit IndirectDiffractionReduction(QWidget *parent = nullptr);
  ~IndirectDiffractionReduction() override;

public slots:
  void instrumentSelected(const QString &instrumentName,
                          const QString &analyserName,
                          const QString &reflectionName);
  void run();
  void openDirectoryDialog();
  void help();
  void plotResults();
  void saveReductions();
  void runFilesChanged();
  void runFilesFinding();
  void runFilesFound();
  void manualGroupingToggled(int state);
  void algorithmComplete(bool error);
  void deleteGroupingWorkspace();

private:
  void initLayout() override;
  void initLocalPython() override;

  void loadSettings();
  void saveSettings();

  Mantid::API::IAlgorithm_sptr saveGSSAlgorithm(const std::string &filename);
  Mantid::API::IAlgorithm_sptr
  saveASCIIAlgorithm(const std::string &filename,
                     const std::string &inputWsName);
  Mantid::API::IAlgorithm_sptr
  saveNexusProcessedAlgorithm(const std::string &filename,
                              const std::string &inputWsName);
  Mantid::API::IAlgorithm_sptr
  saveAlgorithm(const std::string &saveAlgName, const std::string &filename,
                const std::string &inputWsName = "", const int &version = -1);
  Mantid::API::IAlgorithm_sptr
  convertUnitsAlgorithm(const std::string &inputWsName,
                        const std::string &outputWsName,
                        const std::string &target);

  bool validateRebin();
  bool validateVanCal();
  bool validateCalOnly();

  Mantid::API::MatrixWorkspace_sptr
  loadInstrument(const std::string &instrumentName,
                 const std::string &reflection = "");

  void runGenericReduction(QString instName, QString mode);
  void connectRunButtonValidation(const MantidQt::API::MWRunFiles *file_field);
  void runOSIRISdiffonlyReduction();
  void createGroupingWorkspace(const std::string &outputWsName);

private:
  Ui::IndirectDiffractionReduction
      m_uiForm; /// The form generated using Qt Designer
  QDoubleValidator *m_valDbl;
  QString m_settingsGroup; /// The settings group
  MantidQt::API::BatchAlgorithmRunner *m_batchAlgoRunner;
  std::vector<std::string> m_plotWorkspaces;
  std::string m_groupingWsName;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_INDIRECTDIFFRACTIONREDUCTION_H_
