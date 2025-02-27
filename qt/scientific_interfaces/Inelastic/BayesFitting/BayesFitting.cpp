// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "BayesFitting.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/Settings.h"
#include "QuasiModel.h"
#include "QuasiPresenter.h"
#include "QuasiView.h"
#include "ResNormPresenter.h"
#include "StretchPresenter.h"

#include <MantidQtWidgets/Common/QtJobRunner.h>

namespace MantidQt::CustomInterfaces {
DECLARE_SUBWINDOW(BayesFitting)

BayesFitting::BayesFitting(QWidget *parent)
    : InelasticInterface(parent), m_changeObserver(*this, &BayesFitting::handleDirectoryChange) {
  m_uiForm.setupUi(this);
  m_uiForm.pbSettings->setIcon(Settings::icon());

  // Connect Poco Notification Observer
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

  auto resNormRunner = createAlgorithmRunner();

  // insert each tab into the interface on creation
  auto resNormModel = std::make_unique<ResNormModel>();
  auto resNormWidget = m_uiForm.bayesFittingTabs->widget(RES_NORM);
  m_bayesTabs.emplace(RES_NORM, new ResNormPresenter(resNormWidget, std::move(resNormRunner), std::move(resNormModel),
                                                     new ResNormView(resNormWidget)));

  auto quasiRunner = createAlgorithmRunner();
  auto quasiModel = std::make_unique<QuasiModel>();
  auto quasiWidget = m_uiForm.bayesFittingTabs->widget(QUASI);
  m_bayesTabs.emplace(QUASI, new QuasiPresenter(quasiWidget, std::move(quasiRunner), std::move(quasiModel),
                                                new QuasiView(quasiWidget)));

  auto stretchRunner = createAlgorithmRunner();

  auto tabContent = m_uiForm.bayesFittingTabs->widget(STRETCH);
  m_bayesTabs.emplace(STRETCH, new StretchPresenter(tabContent, new StretchView(tabContent),
                                                    std::make_unique<StretchModel>(), std::move(stretchRunner)));
}

std::unique_ptr<MantidQt::API::AlgorithmRunner> BayesFitting::createAlgorithmRunner() const {
  auto jobRunner = std::make_unique<MantidQt::API::QtJobRunner>(true);
  return std::make_unique<MantidQt::API::AlgorithmRunner>(std::move(jobRunner));
}

void BayesFitting::initLayout() {
  // Connect each tab to the actions available in this GUI
  std::map<unsigned int, BayesFittingTab *>::iterator iter;
  for (iter = m_bayesTabs.begin(); iter != m_bayesTabs.end(); ++iter) {
    connect(iter->second, &BayesFittingTab::showMessageBox, this, &BayesFitting::showMessageBox);
  }

  loadSettings();

  connect(m_uiForm.pbSettings, &QPushButton::clicked, this, &BayesFitting::settings);
  connect(m_uiForm.pbHelp, &QPushButton::clicked, this, &BayesFitting::help);
  connect(m_uiForm.pbManageDirs, &QPushButton::clicked, this, &BayesFitting::manageUserDirectories);

  InelasticInterface::initLayout();
}

/**
 * @param :: the detected close event
 */
void BayesFitting::closeEvent(QCloseEvent * /*unused*/) {
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

/**
 * Handles a change in directory.
 *
 * @param pNf :: notification
 */
void BayesFitting::handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf) {
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
void BayesFitting::loadSettings() {
  QSettings settings;
  QString settingsGroup = "CustomInterfaces/IndirectAnalysis/";
  QString saveDir =
      QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  settings.beginGroup(settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", saveDir);

  std::map<unsigned int, BayesFittingTab *>::iterator iter;
  for (iter = m_bayesTabs.begin(); iter != m_bayesTabs.end(); ++iter) {
    iter->second->loadSettings(settings);
  }

  settings.endGroup();
}

void BayesFitting::applySettings(std::map<std::string, QVariant> const &settings) {
  for (auto tab = m_bayesTabs.begin(); tab != m_bayesTabs.end(); ++tab) {
    tab->second->applySettings(settings);
  }
}

std::string BayesFitting::documentationPage() const { return "Inelastic Bayes Fitting"; }

BayesFitting::~BayesFitting() = default;

} // namespace MantidQt::CustomInterfaces
