// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectBayes.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/ManageUserDirectories.h"
#include "Quasi.h"
#include "ResNorm.h"
#include "Stretch.h"

// Add this class to the list of specialised dialogs in this namespace
namespace MantidQt {
namespace CustomInterfaces {
DECLARE_SUBWINDOW(IndirectBayes)
}
} // namespace MantidQt

using namespace MantidQt::CustomInterfaces;

IndirectBayes::IndirectBayes(QWidget *parent)
    : UserSubWindow(parent),
      m_settingsPresenter(
          Mantid::Kernel::make_unique<IDA::IndirectSettingsPresenter>(
              this, "Indirect Bayes",
              "restrict-input-by-name,plot-error-bars")),
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

  // Connect each tab to the actions available in this GUI
  std::map<unsigned int, IndirectBayesTab *>::iterator iter;
  for (iter = m_bayesTabs.begin(); iter != m_bayesTabs.end(); ++iter) {
    connect(iter->second, SIGNAL(runAsPythonScript(const QString &, bool)),
            this, SIGNAL(runAsPythonScript(const QString &, bool)));
    connect(iter->second, SIGNAL(showMessageBox(const QString &)), this,
            SLOT(showMessageBox(const QString &)));
  }

  loadSettings();

  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this,
          SLOT(settingsClicked()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this,
          SLOT(manageUserDirectories()));

  connect(m_settingsPresenter.get(), SIGNAL(applySettings()), this,
          SLOT(applySettings()));
}

void IndirectBayes::initLayout() {}

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

/**
 * Opens the settings dialog
 */
void IndirectBayes::settingsClicked() {
  m_settingsPresenter->loadSettings();
  m_settingsPresenter->showDialog();
}

/**
 * Slot to open a new browser window and navigate to the help page
 * on the wiki for the currently selected tab.
 */
void IndirectBayes::helpClicked() {
  MantidQt::API::HelpWindow::showCustomInterface(nullptr,
                                                 QString("Indirect Bayes"));
}

/**
 * Slot to show the manage user dicrectories dialog when the user clicks
 * the button on the interface.
 */
void IndirectBayes::manageUserDirectories() {
  MantidQt::API::ManageUserDirectories *ad =
      new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}

/**
 * Updates the settings decided on the Settings Dialog
 */
void IndirectBayes::applySettings() {
  auto const restrict =
      m_settingsPresenter->getSetting("restrict-input-by-name").toBool();
  auto const errorBars =
      m_settingsPresenter->getSetting("plot-error-bars").toBool();

  for (auto tab = m_bayesTabs.begin(); tab != m_bayesTabs.end(); ++tab) {
    tab->second->filterInputData(restrict);
    tab->second->setPlotErrorBars(errorBars);
  }
}

/**
 * Slot to wrap the protected showInformationBox method defined
 * in UserSubWindow and provide access to composed tabs.
 *
 * @param message :: The message to display in the message box
 */
void IndirectBayes::showMessageBox(const QString &message) {
  showInformationBox(message);
}

IndirectBayes::~IndirectBayes() {}
