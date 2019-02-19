// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTCORRECTIONS_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTCORRECTIONS_H_

#include "IndirectSettingsPresenter.h"
#include "IndirectTab.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "ui_IndirectCorrections.h"

#include "MantidKernel/ConfigService.h"
#include <Poco/NObserver.h>

class DoubleEditorFactory;
class QtCheckBoxFactory;
class QtStringPropertyManager;

namespace MantidQt {
namespace CustomInterfaces {
// The assumption is made elsewhere that the ordering of these enums matches the
// ordering of the
// tabs as they appear in the interface itself.
enum CorrectionTabChoice {
  CONTAINER_SUBTRACTION,
  CALC_CORR,
  ABSORPTION_CORRECTIONS,
  APPLY_CORR
};

// Forward Declaration
class CorrectionsTab;

/**
 * The IndirectCorrections class is the main class that handles the interface
 * and controls
 * its tabs.
 *
 * Is a friend to the CorrectionsTab class.
 */
class IndirectCorrections : public MantidQt::API::UserSubWindow {
  Q_OBJECT

  /// Allow CorrectionsTab to have access.
  friend class CorrectionsTab;

public:
  /// The name of the interface as registered into the factory
  static std::string name() { return "Corrections"; }
  // This interface's categories.
  static QString categoryInfo() { return "Indirect"; }
  /// Default Constructor
  explicit IndirectCorrections(QWidget *parent = nullptr);

private:
  /// Initialize the layout
  void initLayout() override;
  /// Initialize Python-dependent sections
  void initLocalPython() override;
  /// Load the settings of the interface (and child tabs).
  void loadSettings();

  /// Called upon a close event.
  void closeEvent(QCloseEvent * /*unused*/) override;
  /// handle POCO event
  void
  handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf);

private slots:
  /// Called when the user clicks the Py button
  void exportTabPython();
  /// Opens a directory dialog.
  void openDirectoryDialog();
  /// Opens the Indirect settings GUI
  void settingsClicked();
  /// Opens the Mantid Wiki web page of the current tab.
  void help();
  /// Applies the settings for this interface
  void applySettings();
  /// Slot showing a message box to the user
  void showMessageBox(const QString &message);

private:
  /// The settings dialog
  std::unique_ptr<IDA::IndirectSettingsPresenter> m_settingsPresenter;

  /// UI form containing all Qt elements.
  Ui::IndirectCorrections m_uiForm;

  /// Change Observer for ConfigService (monitors user directories)
  Poco::NObserver<IndirectCorrections,
                  Mantid::Kernel::ConfigValChangeNotification>
      m_changeObserver;

  /// Map of unsigned int (TabChoice enum values) to tabs.
  std::map<unsigned int, CorrectionsTab *> m_tabs;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTCORRECTIONS_H_ */
