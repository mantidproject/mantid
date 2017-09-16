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
  explicit IndirectDiffractionReduction(QWidget *parent = 0);
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
  void individualGroupingToggled(int state);
  void algorithmComplete(bool error);

private:
  void initLayout() override;
  void initLocalPython() override;

  void loadSettings();
  void saveSettings();

  bool validateRebin();
  bool validateVanCal();
  bool validateCalOnly();

  Mantid::API::MatrixWorkspace_sptr
  loadInstrument(const std::string &instrumentName,
                 const std::string &reflection = "");

  void runGenericReduction(QString instName, QString mode);
  void runOSIRISdiffonlyReduction();

private:
  Ui::IndirectDiffractionReduction
      m_uiForm; /// The form generated using Qt Designer
  QDoubleValidator *m_valDbl;
  QString m_settingsGroup; /// The settings group
  MantidQt::API::BatchAlgorithmRunner *m_batchAlgoRunner;
  std::vector<std::string> m_plotWorkspaces;
};
}
}

#endif // MANTIDQTCUSTOMINTERFACES_INDIRECTDIFFRACTIONREDUCTION_H_
