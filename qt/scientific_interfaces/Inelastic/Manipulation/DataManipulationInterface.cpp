// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------
// Includes
//----------------------
#include "DataManipulationInterface.h"

#include "Common/Settings.h"
#include "ElwinTab.h"
#include "IqtTab.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MomentsTab.h"
#include "SqwTab.h"
#include "SymmetriseTab.h"

#include <QDir>
#include <QMessageBox>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace MantidQt;

namespace {
Mantid::Kernel::Logger g_log("DataManipulationInterface");
}

namespace MantidQt::CustomInterfaces {
DECLARE_SUBWINDOW(DataManipulationInterface)

DataManipulationInterface::DataManipulationInterface(QWidget *parent) : IndirectInterface(parent) {}

DataManipulationInterface::~DataManipulationInterface() {}

std::string DataManipulationInterface::documentationPage() const { return "Inelastic Data Manipulation"; }

/**
 * Called when the user clicks the Python export button.
 */
void DataManipulationInterface::exportTabPython() {
  QString tabName = m_uiForm.twIDRTabs->tabText(m_uiForm.twIDRTabs->currentIndex());
  m_tabs[tabName].second->exportPythonScript();
}

/**
 * Sets up Qt UI file and connects signals, slots.
 */
void DataManipulationInterface::initLayout() {
  m_uiForm.setupUi(this);
  m_uiForm.pbSettings->setIcon(Settings::icon());

  // Create the tabs
  addMVPTab<SymmetriseTab, SymmetriseTabView>("Symmetrise");
  addMVPTab<SqwTab, SqwTabView>("S(Q, w)");
  addMVPTab<MomentsTab, MomentsTabView>("Moments");
  addMVPTab<ElwinTab, ElwinTabView>("Elwin");
  addMVPTab<IqtTab, IqtTabView>("Iqt");

  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this, SLOT(settings()));
  // Connect "?" (Help) Button
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  // Connect the Python export button
  connect(m_uiForm.pbPythonExport, SIGNAL(clicked()), this, SLOT(exportTabPython()));
  // Connect the "Manage User Directories" Button
  connect(m_uiForm.pbManageDirectories, SIGNAL(clicked()), this, SLOT(manageUserDirectories()));

  auto const facility = Mantid::Kernel::ConfigService::Instance().getFacility();
  filterUiForFacility(QString::fromStdString(facility.name()));

  IndirectInterface::initLayout();
}

void DataManipulationInterface::applySettings(std::map<std::string, QVariant> const &settings) {
  for (auto tab = m_tabs.begin(); tab != m_tabs.end(); ++tab) {
    tab->second->filterInputData(settings.at("RestrictInput").toBool());
  }
}

/**
 * This function is ran after initLayout(), and runPythonCode is unavailable
 * before this function
 * has run (because of the setup of the base class). For this reason, "setup"
 * functions that require
 * Python scripts are located here.
 */
void DataManipulationInterface::initLocalPython() {}

/**
 * Gets a parameter from an instrument component as a string.
 *
 * @param comp Instrument component
 * @param param Parameter name
 * @return Value as QString
 */
QString DataManipulationInterface::getInstrumentParameterFrom(const Mantid::Geometry::IComponent_const_sptr &comp,
                                                              const std::string &param) {
  QString value;

  if (!comp->hasParameter(param)) {
    g_log.debug() << "Component " << comp->getName() << " has no parameter " << param << '\n';
    return "";
  }

  // Determine it's type and call the corresponding get function
  std::string paramType = comp->getParameterType(param);

  if (paramType == "string")
    value = QString::fromStdString(comp->getStringParameter(param)[0]);

  if (paramType == "double")
    value = QString::number(comp->getNumberParameter(param)[0]);

  return value;
}

/**
 * Tasks to be carried out after an empty instument has finished loading
 */
void DataManipulationInterface::instrumentLoadingDone(bool error) {
  if (error) {
    g_log.warning("Instument loading failed! This instrument (or "
                  "analyser/reflection configuration) may not be supported by "
                  "this interface.");
    return;
  }
}

/**
 * Remove the Poco observer on the config service when the interfaces is closed.
 *
 * @param close Close event (unused)
 */
void DataManipulationInterface::closeEvent(QCloseEvent *close) { UNUSED_ARG(close); }

/**
 * Filters the displayed tabs based on the current facility.
 *
 * @param facility Name of facility
 */
void DataManipulationInterface::filterUiForFacility(const QString &facility) {
  g_log.information() << "Facility selected: " << facility.toStdString() << '\n';
  QStringList enabledTabs;
  QStringList disabledInstruments;

  // These tabs work at any facility (always at end of tabs)
  enabledTabs << "Symmetrise"
              << "S(Q, w)"
              << "Moments"
              << "Elwin"
              << "Iqt";

  // First remove all tabs
  while (m_uiForm.twIDRTabs->count() > 0) {
    QString tabName = m_uiForm.twIDRTabs->tabText(0);

    // Remove the tab
    m_uiForm.twIDRTabs->removeTab(0);

    g_log.debug() << "Removing tab " << tabName.toStdString() << '\n';
  }

  // Add the required tabs
  for (auto &enabledTab : enabledTabs) {
    // Add the tab
    m_uiForm.twIDRTabs->addTab(m_tabs[enabledTab].first, enabledTab);

    g_log.debug() << "Adding tab " << enabledTab.toStdString() << '\n';
  }
}

} // namespace MantidQt::CustomInterfaces
