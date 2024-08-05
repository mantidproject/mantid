// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "DataProcessorInterface.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/Settings.h"

#include "ElwinModel.h"
#include "ElwinPresenter.h"
#include "IqtModel.h"
#include "IqtPresenter.h"
#include "MomentsModel.h"
#include "MomentsPresenter.h"
#include "SqwModel.h"
#include "SqwPresenter.h"
#include "SymmetriseModel.h"
#include "SymmetrisePresenter.h"

#include <QDir>
#include <QMessageBox>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace MantidQt;

namespace {
Mantid::Kernel::Logger g_log("DataProcessorInterface");
}

namespace MantidQt::CustomInterfaces {
DECLARE_SUBWINDOW(DataProcessorInterface)

DataProcessorInterface::DataProcessorInterface(QWidget *parent) : InelasticInterface(parent) {}

DataProcessorInterface::~DataProcessorInterface() = default;

std::string DataProcessorInterface::documentationPage() const { return "Inelastic Data Processor"; }

/**
 * Called when the user clicks the Python export button.
 */
void DataProcessorInterface::exportTabPython() {
  auto const &tabName = m_uiForm.twIDRTabs->tabText(m_uiForm.twIDRTabs->currentIndex()).toStdString();
  m_presenters[tabName]->exportPythonDialog();
}

/**
 * Sets up Qt UI file and connects signals, slots.
 */
void DataProcessorInterface::initLayout() {
  m_uiForm.setupUi(this);
  m_uiForm.pbSettings->setIcon(Settings::icon());

  // Create the tabs
  addMVPTab<SymmetrisePresenter, SymmetriseView, SymmetriseModel>("Symmetrise");
  addMVPTab<SqwPresenter, SqwView, SqwModel>("S(Q, w)");
  addMVPTab<MomentsPresenter, MomentsView, MomentsModel>("Moments");
  addMVPTab<ElwinPresenter, ElwinView, ElwinModel>("Elwin");
  addMVPTab<IqtPresenter, IqtView, IqtModel>("Iqt");

  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this, SLOT(settings()));
  // Connect "?" (Help) Button
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  // Connect the Python export button
  connect(m_uiForm.pbPythonExport, SIGNAL(clicked()), this, SLOT(exportTabPython()));
  // Connect the "Manage User Directories" Button
  connect(m_uiForm.pbManageDirectories, SIGNAL(clicked()), this, SLOT(manageUserDirectories()));

  InelasticInterface::initLayout();
}

void DataProcessorInterface::applySettings(std::map<std::string, QVariant> const &settings) {
  for (auto tab = m_presenters.begin(); tab != m_presenters.end(); ++tab) {
    tab->second->filterInputData(settings.at("RestrictInput").toBool());
    tab->second->enableLoadHistoryProperty(settings.at("LoadHistory").toBool());
  }
}

/**
 * Gets a parameter from an instrument component as a string.
 *
 * @param comp Instrument component
 * @param param Parameter name
 * @return Value as QString
 */
QString DataProcessorInterface::getInstrumentParameterFrom(const Mantid::Geometry::IComponent_const_sptr &comp,
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
void DataProcessorInterface::instrumentLoadingDone(bool error) {
  if (error) {
    g_log.warning("Instument loading failed! This instrument (or "
                  "analyser/reflection configuration) may not be supported by "
                  "this interface.");
    return;
  }
}

} // namespace MantidQt::CustomInterfaces
