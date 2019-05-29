// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectTools.h"
#include "IndirectLoadILL.h"
#include "IndirectTransmissionCalc.h"

#include "MantidKernel/ConfigService.h"

namespace MantidQt {
namespace CustomInterfaces {
DECLARE_SUBWINDOW(IndirectTools)
}
} // namespace MantidQt

using namespace MantidQt::CustomInterfaces;

IndirectTools::IndirectTools(QWidget *parent)
    : IndirectInterface(parent),
      m_changeObserver(*this, &IndirectTools::handleDirectoryChange) {}

void IndirectTools::initLayout() {
  m_uiForm.setupUi(this);

  // Connect Poco Notification Observer
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

  // Insert each tab into the interface on creation
  m_tabs.emplace(TRANSMISSION,
                 new IndirectTransmissionCalc(
                     m_uiForm.IndirectToolsTabs->widget(TRANSMISSION)));
  m_tabs.emplace(LOAD_ILL, new IndirectLoadILL(
                               m_uiForm.IndirectToolsTabs->widget(LOAD_ILL)));

  // Connect each tab to the actions available in this GUI
  std::map<unsigned int, IndirectToolsTab *>::iterator iter;
  for (iter = m_tabs.begin(); iter != m_tabs.end(); ++iter) {
    connect(iter->second, SIGNAL(executePythonScript(const QString &, bool)),
            this, SIGNAL(runAsPythonScript(const QString &, bool)));
    connect(iter->second, SIGNAL(showMessageBox(const QString &)), this,
            SLOT(showMessageBox(const QString &)));
    iter->second->setupTab();
  }

  loadSettings();

  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this, SLOT(settings()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this,
          SLOT(manageUserDirectories()));
}

/**
 * Handles closing the window.
 *
 * @param :: the detected close event
 */
void IndirectTools::closeEvent(QCloseEvent * /*unused*/) {
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

/**
 * Handles a change in directory.
 *
 * @param pNf :: notification
 */
void IndirectTools::handleDirectoryChange(
    Mantid::Kernel::ConfigValChangeNotification_ptr pNf) {
  std::string key = pNf->key();
  if (key == "defaultsave.directory") {
    loadSettings();
  }
}

/**
 * Load the setting for each tab on the interface.
 *
 * This includes setting the default browsing directory to be the default save
 *directory.
 */
void IndirectTools::loadSettings() {
  QSettings settings;
  QString settingsGroup = "CustomInterfaces/IndirectAnalysis/";
  QString saveDir = QString::fromStdString(
      Mantid::Kernel::ConfigService::Instance().getString(
          "defaultsave.directory"));

  settings.beginGroup(settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", saveDir);

  std::map<unsigned int, IndirectToolsTab *>::iterator iter;
  for (iter = m_tabs.begin(); iter != m_tabs.end(); ++iter) {
    iter->second->loadSettings(settings);
  }

  settings.endGroup();
}

/**
 * Slot to run the underlying algorithm code based on the currently selected
 * tab.
 *
 * This method checks the tabs validate method is passing before calling
 * the run method.
 */
void IndirectTools::runClicked() {
  int tabIndex = m_uiForm.IndirectToolsTabs->currentIndex();
  m_tabs[tabIndex]->runTab();
}

std::string IndirectTools::documentationPage() const {
  return "Indirect Tools";
}

IndirectTools::~IndirectTools() {}
