// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "StretchView.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using namespace MantidQt::CustomInterfaces::InterfaceUtils;

// TODO(): range setters are copied from inelastic tab refactor them
namespace {

static const unsigned int NUM_DECIMALS = 6;
static const unsigned int INT_DECIMALS = 0;

Mantid::Kernel::Logger g_log("Stretch");

struct BackgroundType {
  inline static const std::string SLOPING = "Sloping";
  inline static const std::string FLAT = "Flat";
  inline static const std::string ZERO = "Zero";
  inline static const std::string LINEAR = "Linear";
};

struct PlotType {
  inline static const std::string ALL = "All";
  inline static const std::string SIGMA = "Sigma";
  inline static const std::string BETA = "Beta";
  inline static const std::string FWHM = "FWHM";
};

} // namespace

namespace MantidQt::CustomInterfaces {
StretchView::StretchView(QWidget *parent)
    : m_dblManager(new QtDoublePropertyManager()), m_properties(), m_propTree(new QtTreePropertyBrowser()) {
  m_uiForm.setupUi(parent);

  auto eRangeSelector = m_uiForm.ppPlot->addRangeSelector("StretchERange");
  connect(eRangeSelector, &MantidWidgets::RangeSelector::minValueChanged, this, &StretchView::minValueChanged);
  connect(eRangeSelector, &MantidWidgets::RangeSelector::maxValueChanged, this, &StretchView::maxValueChanged);
  setupFitOptions();
  setupPropertyBrowser();
  setupPlotOptions();

  connect(m_uiForm.dsSample, &DataSelector::dataReady, this, &StretchView::handleSampleInputReady);
  connect(m_uiForm.chkSequentialFit, &QCheckBox::toggled, m_uiForm.cbPlot, &QComboBox::setEnabled);

  connect(m_uiForm.spPreviewSpectrum, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
          &StretchView::previewSpecChanged);
  m_uiForm.spPreviewSpectrum->setMaximum(0);

  connect(m_uiForm.pbSave, &QPushButton::clicked, this, &StretchView::saveWorkspacesClicked);
  connect(m_uiForm.pbPlot, &QPushButton::clicked, this, &StretchView::plotWorkspacesClicked);
  connect(m_uiForm.pbPlotContour, &QPushButton::clicked, this, &StretchView::plotContourClicked);
  connect(m_uiForm.pbPlotPreview, &QPushButton::clicked, this, &StretchView::plotCurrentPreviewClicked);

  m_uiForm.dsSample->isOptional(true);
  m_uiForm.dsResolution->isOptional(true);
}

void StretchView::subscribePresenter(IStretchViewSubscriber *presenter) { m_presenter = presenter; }

void StretchView::loadSettings(const QSettings &settings) {
  m_uiForm.dsSample->readSettings(settings.group());
  m_uiForm.dsResolution->readSettings(settings.group());
}

void StretchView::applySettings(std::map<std::string, QVariant> const &settings) {
  setupFitOptions();
  setupPropertyBrowser();
  setupPlotOptions();
  setFileExtensionsByName(settings.at("RestrictInput").toBool());
  setLoadHistory(settings.at("LoadHistory").toBool());
}

void StretchView::minValueChanged(double min) {
  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &StretchView::propertiesUpdated);
  m_dblManager->setValue(m_properties["EMin"], min);
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &StretchView::propertiesUpdated);
}

void StretchView::maxValueChanged(double max) {
  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &StretchView::propertiesUpdated);
  m_dblManager->setValue(m_properties["EMax"], max);
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &StretchView::propertiesUpdated);
}

void StretchView::propertiesUpdated(QtProperty *prop, double val) {
  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("StretchERange");

  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &StretchView::propertiesUpdated);

  if (prop == m_properties["EMin"]) {
    setRangeSelectorMin(m_dblManager, m_properties["EMin"], m_properties["EMax"], eRangeSelector, val);
  } else if (prop == m_properties["EMax"]) {
    setRangeSelectorMax(m_dblManager, m_properties["EMin"], m_properties["EMax"], eRangeSelector, val);
  }

  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &StretchView::propertiesUpdated);
}

void StretchView::setupFitOptions() {
  auto const useQuickBayes = SettingsHelper::hasDevelopmentFlag("quickbayes");
  m_uiForm.cbBackground->clear();
  if (useQuickBayes) {
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::LINEAR));
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::FLAT));
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::ZERO));
    m_uiForm.chkSequentialFit->hide();
  } else {
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::SLOPING));
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::FLAT));
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::ZERO));
    m_uiForm.chkSequentialFit->show();
  }
}

void StretchView::setupPropertyBrowser() {
  auto const useQuickBayes = SettingsHelper::hasDevelopmentFlag("quickbayes");

  m_properties.clear();
  m_dblManager->clear();
  m_propTree->clear();

  m_uiForm.treeSpace->addWidget(m_propTree);

  m_properties["EMin"] = m_dblManager->addProperty("EMin");
  m_properties["EMax"] = m_dblManager->addProperty("EMax");
  m_properties["Beta"] = m_dblManager->addProperty("Beta");

  m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["Beta"], INT_DECIMALS);

  m_propTree->addProperty(m_properties["EMin"]);
  m_propTree->addProperty(m_properties["EMax"]);
  m_propTree->addProperty(m_properties["Beta"]);

  m_dblManager->setValue(m_properties["Beta"], 50);
  m_dblManager->setMinimum(m_properties["Beta"], 1);
  m_dblManager->setMaximum(m_properties["Beta"], 200);

  if (!useQuickBayes) {
    m_properties["SampleBinning"] = m_dblManager->addProperty("Sample Binning");
    m_properties["Sigma"] = m_dblManager->addProperty("Sigma");

    m_dblManager->setDecimals(m_properties["SampleBinning"], INT_DECIMALS);
    m_dblManager->setDecimals(m_properties["Sigma"], INT_DECIMALS);

    m_propTree->addProperty(m_properties["SampleBinning"]);
    m_propTree->addProperty(m_properties["Sigma"]);

    m_dblManager->setValue(m_properties["Sigma"], 50);
    m_dblManager->setMinimum(m_properties["Sigma"], 1);
    m_dblManager->setMaximum(m_properties["Sigma"], 200);
    m_dblManager->setValue(m_properties["SampleBinning"], 1);
    m_dblManager->setMinimum(m_properties["SampleBinning"], 1);
  }

  formatTreeWidget(m_propTree, m_properties);
}

// TODO(): common function in bayes fitting tab
void StretchView::formatTreeWidget(QtTreePropertyBrowser *treeWidget,
                                   QMap<QString, QtProperty *> const &properties) const {
  treeWidget->setIndentation(0);
  for (auto const &item : properties)
    treeWidget->setBackgroundColor(treeWidget->topLevelItem(item), QColor(246, 246, 246));
}

void StretchView::setupPlotOptions() {
  auto const useQuickBayes = SettingsHelper::hasDevelopmentFlag("quickbayes");
  m_uiForm.cbPlot->clear();
  if (useQuickBayes) {
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::ALL));
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::FWHM));
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::BETA));
  } else {
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::ALL));
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::SIGMA));
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::BETA));
  }
}

void StretchView::handleSampleInputReady(const QString &filename) {
  try {
    m_uiForm.ppPlot->clear();
    m_uiForm.ppPlot->addSpectrum("Sample", filename, 0);
  } catch (std::exception const &ex) {
    g_log.warning(ex.what());
    return;
  }

  auto const range = getXRangeFromWorkspace(filename.toStdString());
  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("StretchERange");
  setRangeSelector(m_dblManager, eRangeSelector, m_properties["EMin"], m_properties["EMax"], range, std::nullopt);
  setPlotPropertyRange(m_dblManager, eRangeSelector, m_properties["EMin"], m_properties["EMax"], range);

  eRangeSelector->setMinimum(range.first);
  eRangeSelector->setMaximum(range.second);

  MatrixWorkspace_const_sptr sampleWs = getADSWorkspace(filename.toStdString());
  const int spectra = static_cast<int>(sampleWs->getNumberHistograms());
  m_uiForm.spPreviewSpectrum->setMaximum(spectra - 1);
}

void StretchView::previewSpecChanged(int value) {
  if (!m_uiForm.dsSample->isValid())
    return;

  m_uiForm.ppPlot->clear();

  auto const sampleName = m_uiForm.dsSample->getCurrentDataName();
  try {
    m_uiForm.ppPlot->addSpectrum("Sample", sampleName, value);
    m_presenter->notifyPreviewSpecChanged(value);
  } catch (std::exception const &ex) {
    g_log.warning(ex.what());
  }
}

void StretchView::saveWorkspacesClicked() { m_presenter->notifySaveClicked(); }

void StretchView::plotWorkspacesClicked() { m_presenter->notifyPlotClicked(); }

void StretchView::plotContourClicked() { m_presenter->notifyPlotContourClicked(); }

void StretchView::plotCurrentPreviewClicked() { m_presenter->notifyPlotCurrentPreviewClicked(); }

void StretchView::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Stretch");
  m_uiForm.dsSample->setFBSuffixes(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
  m_uiForm.dsSample->setWSSuffixes(filter ? getSampleWSSuffixes(tabName) : noSuffixes);
  m_uiForm.dsResolution->setFBSuffixes(filter ? getResolutionFBSuffixes(tabName) : getExtensions(tabName));
  m_uiForm.dsResolution->setWSSuffixes(filter ? getResolutionWSSuffixes(tabName) : noSuffixes);
}

void StretchView::setLoadHistory(bool doLoadHistory) {
  m_uiForm.dsSample->setLoadProperty("LoadHistory", doLoadHistory);
  m_uiForm.dsResolution->setLoadProperty("LoadHistory", doLoadHistory);
}

void StretchView::validateUserInput(IUserInputValidator *validator) const {
  validator->checkDataSelectorIsValid("Sample", m_uiForm.dsSample);
  validator->checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);
}

StretchRunData StretchView::getRunData() const {
  auto const useQuickBayes = SettingsHelper::hasDevelopmentFlag("quickbayes");

  auto const sampleName = m_uiForm.dsSample->getCurrentDataName().toStdString();
  auto const resName = m_uiForm.dsResolution->getCurrentDataName().toStdString();

  auto const background = m_uiForm.cbBackground->currentText().toStdString();

  auto const eMin = m_properties["EMin"]->valueText().toDouble();
  auto const eMax = m_properties["EMax"]->valueText().toDouble();
  auto const beta = m_properties["Beta"]->valueText().toInt();

  auto const elasticPeak = m_uiForm.chkElasticPeak->isChecked();

  auto const sigma = !useQuickBayes ? m_properties["Sigma"]->valueText().toInt() : 0;
  auto const nBins = !useQuickBayes ? m_properties["SampleBinning"]->valueText().toInt() : 0;
  auto const sequence = !useQuickBayes ? m_uiForm.chkSequentialFit->isChecked() : false;

  return StretchRunData(sampleName, resName, eMin, eMax, beta, elasticPeak, background, sigma, nBins, sequence);
}

CurrentPreviewData StretchView::getCurrentPreviewData() const {
  auto const sampleName = m_uiForm.dsSample->getCurrentDataName().toStdString();
  auto const hasSample = m_uiForm.ppPlot->hasCurve("Sample");

  return CurrentPreviewData(sampleName, hasSample);
}

std::string StretchView::getPlotType() const { return m_uiForm.cbPlot->currentText().toStdString(); }

std::string StretchView::getPlotContour() const { return m_uiForm.cbPlotContour->currentText().toStdString(); }

IRunView *StretchView::getRunWidget() const { return m_uiForm.runWidget; }

void StretchView::resetPlotContourOptions(const std::vector<std::string> &contourNames) {
  m_uiForm.cbPlotContour->clear();
  for (auto const &name : contourNames)
    m_uiForm.cbPlotContour->addItem(QString::fromStdString(name));
}

void StretchView::setPlotADSEnabled(bool enabled) { m_uiForm.ppPlot->watchADS(enabled); }

void StretchView::setPlotResultEnabled(bool enabled) {
  m_uiForm.pbPlot->setEnabled(enabled);
  m_uiForm.cbPlot->setEnabled(enabled);
}

void StretchView::setPlotContourEnabled(bool enabled) {
  m_uiForm.pbPlotContour->setEnabled(enabled);
  m_uiForm.cbPlotContour->setEnabled(enabled);
}

void StretchView::setSaveResultEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

int StretchView::displaySaveDirectoryMessage() {
  char const *textMessage = "BayesStretch requires a default save directory and "
                            "one is not currently set."
                            " If run, the algorithm will default to saving files "
                            "to the current working directory."
                            " Would you still like to run the algorithm?";
  auto const response = QMessageBox::question(nullptr, tr("Save Directory"), tr(textMessage), QMessageBox::Yes,
                                              QMessageBox::No, QMessageBox::NoButton);
  return response == QMessageBox::No;
}

void StretchView::setButtonsEnabled(bool enabled) {
  setPlotResultEnabled(enabled);
  setPlotContourEnabled(enabled);
  setSaveResultEnabled(enabled);
}

void StretchView::setPlotResultIsPlotting(bool plotting) {
  m_uiForm.pbPlot->setText(plotting ? "Plotting..." : "Plot");
}

void StretchView::setPlotContourIsPlotting(bool plotting) {
  m_uiForm.pbPlotContour->setText(plotting ? "Plotting..." : "Plot Contour");
}

} // namespace MantidQt::CustomInterfaces
