// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTDATAREDUCTION_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTDATAREDUCTION_H_

//----------------------
// Includes
//----------------------
#include "ui_IndirectDataReduction.h"

#include "IndirectDataReductionTab.h"
#include "IndirectSettings.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

#include "MantidGeometry/IComponent.h"

#include <QRegExp>
#include <QScrollArea>

namespace MantidQt {
namespace CustomInterfaces {
//-------------------------------------------
// Forward declarations
//-------------------------------------------
class IndirectDataReductionTab;

/**
This class defines the IndirectDataReduction interface. It handles the overall
instrument settings
and sets up the appropriate interface depending on the deltaE mode of the
instrument. The deltaE
mode is defined in the instrument definition file using the "deltaE-mode".

@author Martyn Gigg, Tessella Support Services plc
@author Michael Whitty
*/

class IndirectDataReduction : public MantidQt::API::UserSubWindow {
  Q_OBJECT

public:
  /// Default Constructor
  explicit IndirectDataReduction(QWidget *parent = nullptr);
  /// Destructor
  ~IndirectDataReduction() override;
  /// Interface name
  static std::string name() { return "Data Reduction"; }
  // This interface's categories.
  static QString categoryInfo() { return "Indirect"; }

  /// Initialize the layout
  void initLayout() override;
  /// Run Python-based initialisation commands
  void initLocalPython() override;

  /// Handled configuration changes
  void handleConfigChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf);

  static Mantid::API::MatrixWorkspace_sptr
  loadInstrumentIfNotExist(const std::string &instrumentName,
                           const std::string &analyser = "",
                           const std::string &reflection = "");

  std::vector<std::pair<std::string, std::vector<std::string>>>
  getInstrumentModes();
  QMap<QString, QString> getInstrumentDetails();

signals:
  /// Emitted when the instrument setup is changed
  void newInstrumentConfiguration();

private slots:
  /// Shows/hides tabs based on facility
  void filterUiForFacility(QString facility);
  /// Opens the Indirect settings GUI
  void settingsClicked();
  /// Opens the help page for the current tab
  void helpClicked();
  /// Exports the current tab algorithms as a Python script
  void exportTabPython();
  /// Opens the manage directory dialog
  void openDirectoryDialog();

  /// Applies the settings once they have been decided
  void applySettings();

  /// Shows a information dialog box
  void showMessageBox(const QString &message);

  /// Called when the load instrument algorithms complete
  void instrumentLoadingDone(bool error);

  /// Called when the instrument setup has been changed
  void instrumentSetupChanged(const QString &instrumentName,
                              const QString &analyser,
                              const QString &reflection);

private:
  QString
  getInstrumentParameterFrom(Mantid::Geometry::IComponent_const_sptr comp,
                             std::string param);

  void readSettings();
  void saveSettings();

  /// Set and show an instrument-specific widget
  void closeEvent(QCloseEvent *close) override;

  /**
   * Adds a tab to the cache of tabs that can be shown.
   *
   * THis method is used to ensure that the tabs are always loaded and their
   * layouts setup for the sake of screenshoting them for documentation.
   *
   * @param name Name to be displayed on tab
   */
  template <typename T> void addTab(const QString &name) {
    QWidget *tabWidget = new QWidget(m_uiForm.twIDRTabs);
    QVBoxLayout *tabLayout = new QVBoxLayout(tabWidget);
    tabWidget->setLayout(tabLayout);

    QScrollArea *tabScrollArea = new QScrollArea(tabWidget);
    tabLayout->addWidget(tabScrollArea);
    tabScrollArea->setWidgetResizable(true);

    QWidget *tabContent = new QWidget(tabScrollArea);
    tabContent->setObjectName("tab" + QString(name).remove(QRegExp("[ ,()]")));
    tabScrollArea->setWidget(tabContent);
    tabScrollArea->setSizePolicy(QSizePolicy::Expanding,
                                 QSizePolicy::Expanding);

    IndirectDataReductionTab *tabIDRContent = new T(this, tabContent);
    tabIDRContent->setupTab();
    tabContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(tabIDRContent, SIGNAL(runAsPythonScript(const QString &, bool)),
            this, SIGNAL(runAsPythonScript(const QString &, bool)));
    connect(tabIDRContent, SIGNAL(showMessageBox(const QString &)), this,
            SLOT(showMessageBox(const QString &)));

    // Add to the cache
    m_tabs[name] = qMakePair(tabWidget, tabIDRContent);

    // Add all tabs to UI initially
    m_uiForm.twIDRTabs->addTab(tabWidget, name);
  }

  friend class IndirectDataReductionTab;
  /// The .ui form generated by Qt Designer
  Ui::IndirectDataReduction m_uiForm;
  /// The settings dialog
  std::unique_ptr<IDA::IndirectSettings> m_settings;
  /// The settings group
  QString m_settingsGroup;
  /// Runner for insturment load algorithm
  MantidQt::API::AlgorithmRunner *m_algRunner;

  // All indirect tabs
  QMap<QString, QPair<QWidget *, IndirectDataReductionTab *>> m_tabs;

  /// Poco observer for changes in user directory settings
  Poco::NObserver<IndirectDataReduction,
                  Mantid::Kernel::ConfigValChangeNotification>
      m_changeObserver;
  QString m_dataDir; ///< default data search directory
  QString m_saveDir; ///< default data save directory

  // Pointer to the current empty instrument workspace
  Mantid::API::MatrixWorkspace_sptr m_instWorkspace;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_INDIRECTDATAREDUCTION_H_
