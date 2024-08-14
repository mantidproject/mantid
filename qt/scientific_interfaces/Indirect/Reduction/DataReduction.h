// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "ui_DataReduction.h"

#include "DataReductionTab.h"
#include "MantidQtWidgets/Spectroscopy/InelasticInterface.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/QtJobRunner.h"

#include <string>
#include <unordered_map>

#include <QRegExp>
#include <QScrollArea>

namespace MantidQt {
namespace CustomInterfaces {

class DataReductionTab;

class IDataReduction {
public:
  virtual ~IDataReduction() = default;

  virtual Mantid::API::MatrixWorkspace_sptr instrumentWorkspace() = 0;

  virtual MantidWidgets::IInstrumentConfig *getInstrumentConfiguration() const = 0;
  virtual QMap<QString, QString> getInstrumentDetails() = 0;

  virtual void showAnalyserAndReflectionOptions(bool visible) = 0;
};

/**
This class defines the DataReduction interface. It handles the overall
instrument settings
and sets up the appropriate interface depending on the deltaE mode of the
instrument. The deltaE
mode is defined in the instrument definition file using the "deltaE-mode".

@author Martyn Gigg, Tessella Support Services plc
@author Michael Whitty
*/

class DataReduction : public InelasticInterface, public IDataReduction {
  Q_OBJECT

public:
  /// Default Constructor
  explicit DataReduction(QWidget *parent = nullptr);
  /// Destructor
  ~DataReduction() override;
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

  Mantid::API::MatrixWorkspace_sptr instrumentWorkspace() override;

  void loadInstrumentIfNotExist(const std::string &instrumentName, const std::string &analyser = "",
                                const std::string &reflection = "");

  MantidWidgets::IInstrumentConfig *getInstrumentConfiguration() const override;
  QMap<QString, QString> getInstrumentDetails() override;

  void showAnalyserAndReflectionOptions(bool visible) override;

signals:
  /// Emitted when the instrument setup is changed
  void newInstrumentConfiguration();

private slots:
  /// Shows/hides tabs based on facility
  void filterUiForFacility(const QString &facility);

  /// Exports the current tab algorithms as a Python script
  void exportTabPython();

  /// Called when the load instrument algorithms complete
  void instrumentLoadingDone(bool error);

  /// Called when the instrument setup has been changed
  void instrumentSetupChanged(const QString &instrumentName, const QString &analyser, const QString &reflection);

private:
  std::string documentationPage() const override;

  void applySettings(std::map<std::string, QVariant> const &settings) override;

  void loadInstrumentDetails();

  QString getInstrumentParameterFrom(const Mantid::Geometry::IComponent_const_sptr &comp, const std::string &param);

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
  template <typename T> void addTab(const std::string &name) {
    QWidget *tabWidget = new QWidget(m_uiForm.twIDRTabs);
    QVBoxLayout *tabLayout = new QVBoxLayout(tabWidget);
    tabWidget->setLayout(tabLayout);

    QScrollArea *tabScrollArea = new QScrollArea(tabWidget);
    tabLayout->addWidget(tabScrollArea);
    tabScrollArea->setWidgetResizable(true);

    QWidget *tabContent = new QWidget(tabScrollArea);
    tabContent->setObjectName("tab" + QString::fromStdString(name).remove(QRegExp("[ ,()]")));
    tabScrollArea->setWidget(tabContent);
    tabScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    DataReductionTab *tabIDRContent = new T(this, tabContent);
    tabContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(tabIDRContent, SIGNAL(showMessageBox(const std::string &)), this,
            SLOT(showMessageBox(const std::string &)));

    // Add to the cache
    m_tabs[name] = qMakePair(tabWidget, tabIDRContent);

    // Add all tabs to UI initially
    m_uiForm.twIDRTabs->addTab(tabWidget, QString::fromStdString(name));
  }

  /**
   * Adds an MVP tab to the cache of tabs that can be shown.
   *
   * This method is used to ensure that the tabs are always loaded and their
   * layouts setup for the sake of screenshoting them for documentation.
   *
   * @param name Name to be displayed on tab
   */

  template <typename TabPresenter, typename TabView, typename TabModel> void addMVPTab(const std::string &name) {
    QWidget *tabWidget = new QWidget(m_uiForm.twIDRTabs);
    QVBoxLayout *tabLayout = new QVBoxLayout(tabWidget);
    tabWidget->setLayout(tabLayout);

    QScrollArea *tabScrollArea = new QScrollArea(tabWidget);
    tabLayout->addWidget(tabScrollArea);
    tabScrollArea->setWidgetResizable(true);

    QWidget *tabContent = new QWidget(tabScrollArea);
    tabContent->setObjectName("tab" + QString::fromStdString(name).remove(QRegExp("[ ,()]")));
    tabScrollArea->setWidget(tabContent);
    tabScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto jobRunner = std::make_unique<MantidQt::API::QtJobRunner>();
    auto algorithmRunner = std::make_unique<MantidQt::API::AlgorithmRunner>(std::move(jobRunner));
    auto presenter = std::make_unique<TabPresenter>(this, new TabView(tabContent), std::make_unique<TabModel>(),
                                                    std::move(algorithmRunner));

    tabContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(presenter.get(), SIGNAL(showMessageBox(const std::string &)), this,
            SLOT(showMessageBox(const std::string &)));

    // Add to the cache
    m_tabs[name] = qMakePair(tabWidget, presenter.get());
    m_presenters[name] = std::move(presenter);

    // Add all tabs to UI initially
    m_uiForm.twIDRTabs->addTab(tabWidget, QString::fromStdString(name));
  }

  friend class DataReductionTab;
  /// The .ui form generated by Qt Designer
  Ui::DataReduction m_uiForm;
  /// The settings group
  QString m_settingsGroup;

  // All indirect tabs - this should be removed when the interface is in MVP
  QMap<std::string, QPair<QWidget *, DataReductionTab *>> m_tabs;
  /// A map to hold the presenter corresponding to each tab
  std::unordered_map<std::string, std::unique_ptr<DataReductionTab>> m_presenters;

  /// Poco observer for changes in user directory settings
  Poco::NObserver<DataReduction, Mantid::Kernel::ConfigValChangeNotification> m_changeObserver;
  QString m_dataDir; ///< default data search directory
  QString m_saveDir; ///< default data save directory

  /// Pointer to the current empty instrument workspace
  Mantid::API::MatrixWorkspace_sptr m_instWorkspace;
  /// The currently loaded instrument parameter file
  std::string m_ipfFilename;
  /// The instrument definition file directory
  std::string m_idfDirectory;
  /// Stores the details of the instrument
  QMap<QString, QString> m_instDetails;
};

} // namespace CustomInterfaces
} // namespace MantidQt
