// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTBAYES_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTBAYES_H_

#include "IndirectBayesTab.h"
#include "IndirectSettingsPresenter.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "ui_IndirectBayes.h"

#include "MantidKernel/ConfigService.h"
#include <Poco/NObserver.h>

namespace MantidQt {
namespace CustomInterfaces {
/**
This class defines the Indirect Bayes interface. It handles the creation of the
interface window and handles the interaction between the child tabs on the
window.

@author Samuel Jackson, STFC
*/

class DLLExport IndirectBayes : public MantidQt::API::UserSubWindow {
  Q_OBJECT

public: // public constants and enums
  /// Enumeration for the index of each tab
  enum TabChoice { RES_NORM, QUASI, STRETCH };

public: // public constructor, destructor and functions
  /// Default Constructor
  IndirectBayes(QWidget *parent = nullptr);
  /// Destructor
  ~IndirectBayes() override;
  /// Interface name
  static std::string name() { return "Bayes"; }
  // This interface's categories.
  static QString categoryInfo() { return "Indirect"; }
  void initLayout() override;

private slots:
  /// Opens the Indirect settings GUI
  void settingsClicked();
  /// Slot for clicking on the hlep button
  void helpClicked();
  /// Slot for clicking on the manage directories button
  void manageUserDirectories();
  /// Applies the settings for this interface
  void applySettings();
  /// Slot showing a message box to the user
  void showMessageBox(const QString &message);

private:
  /// Called upon a close event.
  void closeEvent(QCloseEvent * /*unused*/) override;
  /// handle POCO event
  void
  handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf);
  /// Load default interface settings for each tab
  void loadSettings();

  /// The settings dialog
  std::unique_ptr<IDA::IndirectSettingsPresenter> m_settingsPresenter;
  /// Map of tabs indexed by position on the window
  std::map<unsigned int, IndirectBayesTab *> m_bayesTabs;
  /// Change Observer for ConfigService (monitors user directories)
  Poco::NObserver<IndirectBayes, Mantid::Kernel::ConfigValChangeNotification>
      m_changeObserver;
  /// Main interface window
  Ui::IndirectBayes m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
