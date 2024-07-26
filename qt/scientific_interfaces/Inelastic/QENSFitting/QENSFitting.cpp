// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QENSFitting.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/Settings.h"

#include "FitTab.h"
#include "TabFactory.h"

namespace MantidQt::CustomInterfaces::Inelastic {
DECLARE_SUBWINDOW(QENSFitting)

QENSFitting::QENSFitting(QWidget *parent)
    : InelasticInterface(parent), m_settingsGroup("CustomInterfaces/IndirectAnalysis/") {
  m_uiForm.setupUi(this);
  m_uiForm.pbSettings->setIcon(Settings::icon());

  auto const tabFactory = std::make_unique<TabFactory>(m_uiForm.twIDATabs);
  m_tabs.emplace(MSD_FIT, tabFactory->makeMSDTab(MSD_FIT));
  m_tabs.emplace(IQT_FIT, tabFactory->makeIqtTab(IQT_FIT));
  m_tabs.emplace(CONV_FIT, tabFactory->makeConvolutionTab(CONV_FIT));
  m_tabs.emplace(FQ_FIT, tabFactory->makeFunctionQTab(FQ_FIT));
}

/**
 * Initialised the layout of the interface.  MUST be called.
 */
void QENSFitting::initLayout() {
  // Set up all tabs
  for (auto &tab : m_tabs) {
    connect(tab.second, SIGNAL(showMessageBox(const std::string &)), this, SLOT(showMessageBox(const std::string &)));
  }

  connect(m_uiForm.pbPythonExport, SIGNAL(clicked()), this, SLOT(exportTabPython()));
  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this, SLOT(settings()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this, SLOT(manageUserDirectories()));

  InelasticInterface::initLayout();
}

std::string QENSFitting::documentationPage() const { return "Inelastic QENS Fitting"; }

/**
 * Handles exporting a Python script for the current tab.
 */
void QENSFitting::exportTabPython() {
  unsigned int currentTab = m_uiForm.twIDATabs->currentIndex();
  m_tabs[currentTab]->exportPythonScript();
}

} // namespace MantidQt::CustomInterfaces::Inelastic
