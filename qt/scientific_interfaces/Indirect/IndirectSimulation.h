// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTSIMULATION_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTSIMULATION_H_

//----------------------
// Includes
//----------------------
#include "IndirectSimulationTab.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "ui_IndirectSimulation.h"

#include <Poco/NObserver.h>

namespace MantidQt {
namespace CustomInterfaces {
/**
  This class defines the Indirect Simulation interface. It handles the creation
  of the interface window and
  handles the interaction between the child tabs on the window.

  @author Samuel Jackson, STFC
  */

class DLLExport IndirectSimulation : public MantidQt::API::UserSubWindow {
  Q_OBJECT

public: // public constants and enums
  /// Enumeration for the index of each tab
  enum TabChoice { MOLDYN, SASSENA, DOS };

public: // public constructor, destructor and functions
  /// Default Constructor
  IndirectSimulation(QWidget *parent = nullptr);
  /// Destructor
  ~IndirectSimulation() override;
  /// Interface name
  static std::string name() { return "Simulation"; }
  /// This interface's categories.
  static QString categoryInfo() { return "Indirect"; }
  /// Setup tab UI
  void initLayout() override;

private slots:
  /// Slot for clicking on the help button
  void helpClicked();
  /// Slot for clicking on the manage directories button
  void manageUserDirectories();
  /// Slot showing a message box to the user
  void showMessageBox(const QString &message);

private:
  /// Load default interface settings for each tab
  void loadSettings();
  /// Called upon a close event.
  void closeEvent(QCloseEvent *) override;
  /// handle POCO event
  void
  handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf);

  /// Map of tabs indexed by position on the window
  std::map<unsigned int, IndirectSimulationTab *> m_simulationTabs;
  /// Change Observer for ConfigService (monitors user directories)
  Poco::NObserver<IndirectSimulation,
                  Mantid::Kernel::ConfigValChangeNotification>
      m_changeObserver;
  /// Main interface window
  Ui::IndirectSimulation m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
