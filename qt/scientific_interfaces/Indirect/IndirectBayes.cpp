// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectBayes.h"
#include "Quasi.h"
#include "ResNorm.h"
#include "Stretch.h"

using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {
DECLARE_SUBWINDOW(IndirectBayes)

IndirectBayes::IndirectBayes(QWidget *parent)
    : IndirectInterface(parent),
      m_changeObserver(*this, &IndirectBayes::handleDirectoryChange) {
  m_uiForm.setupUi(this);

  // Connect Poco Notification Observer
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

  // insert each tab into the interface on creation
  m_bayesTabs.emplace(
      RES_NORM, new ResNorm(m_uiForm.indirectBayesTabs->widget(RES_NORM)));
  m_bayesTabs.emplace(QUASI,
                      new Quasi(m_uiForm.indirectBayesTabs->widget(QUASI)));
  m_bayesTabs.emplace(STRETCH,
                      new Stretch(m_uiForm.indirectBayesTabs->widget(STRETCH)));
}

void IndirectBayes::initLayout() {
  // Connect each tab to the actions available in this GUI
  std::map<unsigned int, IndirectBayesTab *>::iterator iter;
  for (iter = m_bayesTabs.begin(); iter != m_bayesTabs.end(); ++iter) {
    connect(iter->second, SIGNAL(runAsPythonScript(const QString &, bool)),
            this, SIGNAL(runAsPythonScript(const QString &, bool)));
    connect(iter->second, SIGNAL(showMessageBox(const QString &)), this,
            SLOT(showMessageBox(const QString &)));
  }

  loadSettings();

  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this, SLOT(settings()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this,
          SLOT(manageUserDirectories()));

  // Needed to initially apply the settings loaded on the settings GUI
  applySettings(getInterfaceSettings());
}

/**
 * @param :: the detected close event
 */
void IndirectBayes::closeEvent(QCloseEvent * /*unused*/) {
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

/**
 * Handles a change in directory.
 *
 * @param pNf :: notification
 */
void IndirectBayes::handleDirectoryChange(
    Mantid::Kernel::ConfigValChangeNotification_ptr pNf) {
  std::string key = pNf->key();

  if (key == "defaultsave.directory")
    loadSettings();
}

/**
 * Load the setting for each tab on the interface.
 *
 * This includes setting the default browsing directory to be the default save
 *directory.
 */
void IndirectBayes::loadSettings() {
  QSettings settings;
  QString settingsGroup = "CustomInterfaces/IndirectAnalysis/";
  QString saveDir = QString::fromStdString(
      Mantid::Kernel::ConfigService::Instance().getString(
          "defaultsave.directory"));

  settings.beginGroup(settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", saveDir);

  std::map<unsigned int, IndirectBayesTab *>::iterator iter;
  for (iter = m_bayesTabs.begin(); iter != m_bayesTabs.end(); ++iter) {
    iter->second->loadSettings(settings);
  }

  settings.endGroup();
}

void IndirectBayes::applySettings(
    std::map<std::string, QVariant> const &settings) {
  for (auto tab = m_bayesTabs.begin(); tab != m_bayesTabs.end(); ++tab) {
    tab->second->filterInputData(settings.at("RestrictInput").toBool());
  }
}

std::string IndirectBayes::documentationPage() const {
  return "Indirect Bayes";
}

IndirectBayes::~IndirectBayes() {}

} // namespace CustomInterfaces
} // namespace MantidQt
