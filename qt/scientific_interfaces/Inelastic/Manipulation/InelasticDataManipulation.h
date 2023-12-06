// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "ui_InelasticDataManipulation.h"

#include "IndirectInterface.h"
#include "InelasticDataManipulationTab.h"

#include "MantidGeometry/IComponent.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"

#include <QRegExp>
#include <QScrollArea>

namespace MantidQt {
namespace CustomInterfaces {

class InelasticDataManipulationTab;

/**
This class defines the InelasticDataManipulation interface. It handles the overall
instrument settings
and sets up the appropriate interface depending on the deltaE mode of the
instrument. The deltaE
mode is defined in the instrument definition file using the "deltaE-mode".
*/

class InelasticDataManipulation : public IndirectInterface {
  Q_OBJECT

public:
  /// Default Constructor
  explicit InelasticDataManipulation(QWidget *parent = nullptr);
  /// Destructor
  ~InelasticDataManipulation() override;
  /// Interface name
  static std::string name() { return "Data Manipulation"; }
  // This interface's categories.
  static QString categoryInfo() { return "Inelastic"; }

  /// Initialize the layout
  void initLayout() override;
  /// Run Python-based initialisation commands
  void initLocalPython() override;

  std::vector<std::pair<std::string, std::vector<std::string>>> getInstrumentModes();

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

private:
  std::string documentationPage() const override;

  void applySettings(std::map<std::string, QVariant> const &settings) override;

  QString getInstrumentParameterFrom(const Mantid::Geometry::IComponent_const_sptr &comp, const std::string &param);

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
  template <typename TabPresenter> void addTab(const QString &name) {
    QWidget *tabWidget = new QWidget(m_uiForm.twIDRTabs);
    QVBoxLayout *tabLayout = new QVBoxLayout(tabWidget);
    tabWidget->setLayout(tabLayout);

    QScrollArea *tabScrollArea = new QScrollArea(tabWidget);
    tabLayout->addWidget(tabScrollArea);
    tabScrollArea->setWidgetResizable(true);

    QWidget *tabContent = new QWidget(tabScrollArea);
    tabContent->setObjectName("tab" + QString(name).remove(QRegExp("[ ,()]")));
    tabScrollArea->setWidget(tabContent);
    tabScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    InelasticDataManipulationTab *tabIDRContent = new TabPresenter(tabContent);

    tabIDRContent->setupTab();
    tabContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(tabIDRContent, SIGNAL(showMessageBox(const QString &)), this, SLOT(showMessageBox(const QString &)));

    // Add to the cache
    m_tabs[name] = qMakePair(tabWidget, tabIDRContent);

    // Add all tabs to UI initially
    m_uiForm.twIDRTabs->addTab(tabWidget, name);
  }

  template <typename TabPresenter, typename TabView> void addMVPTab(const QString &name) {
    QWidget *tabWidget = new QWidget(m_uiForm.twIDRTabs);
    QVBoxLayout *tabLayout = new QVBoxLayout(tabWidget);
    tabWidget->setLayout(tabLayout);

    QScrollArea *tabScrollArea = new QScrollArea(tabWidget);
    tabLayout->addWidget(tabScrollArea);
    tabScrollArea->setWidgetResizable(true);

    QWidget *tabContent = new QWidget(tabScrollArea);
    tabContent->setObjectName("tab" + QString(name).remove(QRegExp("[ ,()]")));
    tabScrollArea->setWidget(tabContent);
    tabScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    InelasticDataManipulationTab *tabIDRContent = new TabPresenter(tabContent, new TabView(tabContent));

    tabIDRContent->setupTab();
    tabContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(tabIDRContent, SIGNAL(showMessageBox(const QString &)), this, SLOT(showMessageBox(const QString &)));

    // Add to the cache
    m_tabs[name] = qMakePair(tabWidget, tabIDRContent);

    // Add all tabs to UI initially
    m_uiForm.twIDRTabs->addTab(tabWidget, name);
  }

  friend class InelasticDataManipulationTab;
  /// The .ui form generated by Qt Designer
  Ui::InelasticDataManipulation m_uiForm;

  // All indirect tabs
  QMap<QString, QPair<QWidget *, InelasticDataManipulationTab *>> m_tabs;

  QString m_dataDir; ///< default data search directory
  QString m_saveDir; ///< default data save directory
};

} // namespace CustomInterfaces
} // namespace MantidQt
