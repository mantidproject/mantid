// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysis.h"

#include "IndirectDataAnalysisConvFitTab.h"
#include "IndirectDataAnalysisFqFitTab.h"
#include "IndirectDataAnalysisIqtFitTab.h"
#include "IndirectDataAnalysisMSDFitTab.h"

namespace MantidQt::CustomInterfaces::IDA {
DECLARE_SUBWINDOW(IndirectDataAnalysis)

IndirectDataAnalysis::IndirectDataAnalysis(QWidget *parent)
    : IndirectInterface(parent), m_settingsGroup("CustomInterfaces/IndirectAnalysis/") {
  m_uiForm.setupUi(this);
  m_uiForm.pbSettings->setIcon(IndirectSettings::icon());

  // Allows us to get a handle on a tab using an enum, for example
  // "m_tabs[ELWIN]".
  // All tabs MUST appear here to be shown in interface.
  // We make the assumption that each map key corresponds to the order in which
  // the tabs appear.
  m_tabs.emplace(MSD_FIT, new IndirectDataAnalysisMSDFitTab(m_uiForm.twIDATabs->widget(MSD_FIT)));
  m_tabs.emplace(IQT_FIT, new IndirectDataAnalysisIqtFitTab(m_uiForm.twIDATabs->widget(IQT_FIT)));
  m_tabs.emplace(CONV_FIT, new IndirectDataAnalysisConvFitTab(m_uiForm.twIDATabs->widget(CONV_FIT)));
  m_tabs.emplace(FQ_FIT, new IndirectDataAnalysisFqFitTab(m_uiForm.twIDATabs->widget(FQ_FIT)));
}

void IndirectDataAnalysis::applySettings(std::map<std::string, QVariant> const &settings) {
  for (auto tab = m_tabs.begin(); tab != m_tabs.end(); ++tab) {
    tab->second->filterInputData(settings.at("RestrictInput").toBool());
  }
}

/**
 * Initialised the layout of the interface.  MUST be called.
 */
void IndirectDataAnalysis::initLayout() {
  // Set up all tabs
  for (auto &tab : m_tabs) {
    tab.second->setupTab();
    connect(tab.second, SIGNAL(showMessageBox(const QString &)), this, SLOT(showMessageBox(const QString &)));
  }

  connect(m_uiForm.pbPythonExport, SIGNAL(clicked()), this, SLOT(exportTabPython()));
  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this, SLOT(settings()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this, SLOT(manageUserDirectories()));

  // Needed to initially apply the settings loaded on the settings GUI
  applySettings(getInterfaceSettings());
}

std::string IndirectDataAnalysis::documentationPage() const { return "Inelastic Data Analysis"; }

/**
 * Handles exporting a Python script for the current tab.
 */
void IndirectDataAnalysis::exportTabPython() {
  unsigned int currentTab = m_uiForm.twIDATabs->currentIndex();
  m_tabs[currentTab]->exportPythonScript();
}

} // namespace MantidQt::CustomInterfaces::IDA
