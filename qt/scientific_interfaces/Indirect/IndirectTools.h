// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTTOOLS_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTTOOLS_H_

#include "IndirectSettingsDialog.h"
#include "IndirectToolsTab.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "ui_IndirectTools.h"

#include <Poco/NObserver.h>

namespace MantidQt {
namespace CustomInterfaces {
/**
This class defines the Indirect Foreign interface. It handles the creation of
the interface window and
            handles the interaction between the child tabs on the window.

@author Samuel Jackson, STFC
*/

class DLLExport IndirectTools : public MantidQt::API::UserSubWindow {
  Q_OBJECT

public: // public constants and enums
  /// Enumeration for the index of each tab
  enum TabChoice { TRANSMISSION, LOAD_ILL };

public: // public constructor, destructor and functions
  /// Default Constructor
  IndirectTools(QWidget *parent = nullptr);
  /// Destructor
  ~IndirectTools() override;
  /// Interface name
  static std::string name() { return "Tools"; }
  // This interface's categories.
  static QString categoryInfo() { return "Indirect"; }

  void initLayout() override;

private slots:
  /// Slot for clicking on the run button
  void runClicked();
  /// Opens the Indirect settings GUI
  void settingsClicked();
  /// Slot for clicking on the hlep button
  void helpClicked();
  /// Slot for clicking on the manage directories button
  void manageUserDirectories();
  /// Slot showing a message box to the user
  void showMessageBox(const QString &message);

private:
  /// Load default interface settings for each tab
  void loadSettings();
  /// Called upon a close event.
  void closeEvent(QCloseEvent * /*unused*/) override;
  /// Handle POCO event
  void
  handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf);

  /// The settings dialog
  std::unique_ptr<IDA::IndirectSettingsDialog> m_settingsDialog;
  /// Map of tabs indexed by position on the window
  std::map<unsigned int, IndirectToolsTab *> m_tabs;
  /// Change Observer for ConfigService (monitors user directories)
  Poco::NObserver<IndirectTools, Mantid::Kernel::ConfigValChangeNotification>
      m_changeObserver;
  /// Main interface window
  Ui::IndirectTools m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
