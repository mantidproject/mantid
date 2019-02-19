// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_

#include "IndirectSettingsPresenter.h"
#include "IndirectTab.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "ui_IndirectDataAnalysis.h"

#include "MantidKernel/ConfigService.h"
#include <Poco/NObserver.h>

class DoubleEditorFactory;
class QtCheckBoxFactory;
class QtStringPropertyManager;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
// The assumption is made elsewhere that the ordering of these enums matches the
// ordering of the
// tabs as they appear in the interface itself.
enum IDATabChoice { ELWIN, MSD_FIT, IQT, IQT_FIT, CONV_FIT, JUMP_FIT };

// Number of decimal places in property browsers.
static const unsigned int NUM_DECIMALS = 6;

// Forward Declaration
class IndirectDataAnalysisTab;

/**
 * The IndirectDataAnalysis class is the main class that handles the interface
 *and controls
 * its tabs.
 *
 * Is a friend to the IndirectDataAnalysisTab class.
 */
class IndirectDataAnalysis : public MantidQt::API::UserSubWindow {
  Q_OBJECT

  /// Allow IndirectDataAnalysisTab to have access.
  friend class IndirectDataAnalysisTab;

public:
  /// The name of the interface as registered into the factory
  static std::string name() { return "Data Analysis"; }
  // This interface's categories.
  static QString categoryInfo() { return "Indirect"; }
  /// Default Constructor
  explicit IndirectDataAnalysis(QWidget *parent = nullptr);

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
  /// Sets the active workspace in the selected tab
  void tabChanged(int index);
  /// Opens the Indirect settings GUI
  void settingsClicked();
  /// Called when the user clicks the Py button
  void exportTabPython();
  /// Opens a directory dialog.
  void openDirectoryDialog();
  /// Opens the Mantid Wiki web page of the current tab.
  void help();
  /// Applies the settings for this interface
  void applySettings();
  /// Slot showing a message box to the user
  void showMessageBox(const QString &message);

private:
  /// UI form containing all Qt elements.
  Ui::IndirectDataAnalysis m_uiForm;
  /// The settings dialog
  std::unique_ptr<IDA::IndirectSettingsPresenter> m_settingsPresenter;
  /// The settings group
  QString m_settingsGroup;
  /// Integer validator
  QIntValidator *m_valInt;
  /// Double validator
  QDoubleValidator *m_valDbl;

  /// Change Observer for ConfigService (monitors user directories)
  Poco::NObserver<IndirectDataAnalysis,
                  Mantid::Kernel::ConfigValChangeNotification>
      m_changeObserver;

  /// Map of unsigned int (TabChoice enum values) to tabs.
  std::map<unsigned int, IndirectDataAnalysisTab *> m_tabs;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_ */
