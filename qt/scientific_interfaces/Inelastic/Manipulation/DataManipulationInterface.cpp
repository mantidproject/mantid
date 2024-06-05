// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "DataManipulationInterface.h"

#include "Common/Settings.h"
#include "ElwinPresenter.h"
#include "IqtPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MomentsPresenter.h"
#include "SqwPresenter.h"
#include "SymmetrisePresenter.h"

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

DataManipulationInterface::DataManipulationInterface(QWidget *parent) : InelasticInterface(parent) {}

DataManipulationInterface::~DataManipulationInterface() = default;

std::string DataManipulationInterface::documentationPage() const { return "Inelastic Data Processor"; }

/**
 * Called when the user clicks the Python export button.
 */
void DataManipulationInterface::exportTabPython() {
  auto const &tabName = m_uiForm.twIDRTabs->tabText(m_uiForm.twIDRTabs->currentIndex()).toStdString();
  m_presenters[tabName]->exportPythonScript();
}

/**
 * Sets up Qt UI file and connects signals, slots.
 */
void DataManipulationInterface::initLayout() {
  m_uiForm.setupUi(this);
  m_uiForm.pbSettings->setIcon(Settings::icon());

  // Create the tabs
  addMVPTab<SymmetrisePresenter, SymmetriseView>("Symmetrise");
  addMVPTab<SqwPresenter, SqwView>("S(Q, w)");
  addMVPTab<MomentsPresenter, MomentsView>("Moments");
  addMVPTab<ElwinPresenter, ElwinView>("Elwin");
  addMVPTab<IqtPresenter, IqtView>("Iqt");

  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this, SLOT(settings()));
  // Connect "?" (Help) Button
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  // Connect the Python export button
  connect(m_uiForm.pbPythonExport, SIGNAL(clicked()), this, SLOT(exportTabPython()));
  // Connect the "Manage User Directories" Button
  connect(m_uiForm.pbManageDirectories, SIGNAL(clicked()), this, SLOT(manageUserDirectories()));

  InelasticInterface::initLayout();
}

void DataManipulationInterface::applySettings(std::map<std::string, QVariant> const &settings) {
  for (auto tab = m_presenters.begin(); tab != m_presenters.end(); ++tab) {
    tab->second->filterInputData(settings.at("RestrictInput").toBool());
  }
}

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

} // namespace MantidQt::CustomInterfaces
