// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QuasiView.h"

#include "QuasiPresenter.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

#include <QColor>
#include <QString>

#include <string>

namespace {
static const std::string TAB_NAME = "Quasi";

struct PlotType {
  inline static const std::string ALL = "All";
  inline static const std::string AMPLITUDE = "Amplitude";
  inline static const std::string FWHM = "FWHM";
  inline static const std::string PROB = "Prob";
  inline static const std::string GAMMA = "Gamma";
};

QColor toQColor(std::string const &colour) {
  if (colour == "red") {
    return Qt::red;
  } else if (colour == "blue") {
    return Qt::blue;
  } else if (colour == "magenta") {
    return Qt::magenta;
  } else if (colour == "cyan") {
    return Qt::cyan;
  }
  return QColor();
}

} // namespace

namespace MantidQt::CustomInterfaces {
using namespace InterfaceUtils;

QuasiView::QuasiView(QWidget *parent)
    : QWidget(parent), m_dblManager(new QtDoublePropertyManager()), m_propTree(new QtTreePropertyBrowser()),
      m_properties(), m_dblEditorFactory(new DoubleEditorFactory()), m_presenter() {
  m_uiForm.setupUi(parent);

  m_propTree->setFactoryForManager(m_dblManager, m_dblEditorFactory);

  setupFitOptions();
  setupPropertyBrowser();
  setupPlotOptions();

  auto eRangeSelector = m_uiForm.ppPlot->addRangeSelector("QuasiERange");
  connect(eRangeSelector, &MantidWidgets::RangeSelector::minValueChanged, this, &QuasiView::minEValueChanged);
  connect(eRangeSelector, &MantidWidgets::RangeSelector::maxValueChanged, this, &QuasiView::maxEValueChanged);

  connect(m_uiForm.chkFixWidth, &QCheckBox::toggled, m_uiForm.mwFixWidthDat, &FileFinderWidget::setEnabled);
  connect(m_uiForm.chkUseResNorm, &QCheckBox::toggled, m_uiForm.dsResNorm, &DataSelector::setEnabled);

  connect(m_uiForm.dsSample, &DataSelector::dataReady, this, &QuasiView::notifySampleInputReady);
  connect(m_uiForm.dsSample, &DataSelector::filesAutoLoaded, this, &QuasiView::notifyFileAutoLoaded);

  connect(m_uiForm.dsResolution, &DataSelector::dataReady, this, &QuasiView::notifyResolutionInputReady);
  connect(m_uiForm.dsResolution, &DataSelector::filesAutoLoaded, this, &QuasiView::notifyFileAutoLoaded);

  connect(m_uiForm.cbProgram, static_cast<void (QComboBox::*)(int const)>(&QComboBox::currentIndexChanged), this,
          &QuasiView::handleProgramChange);
  connect(m_uiForm.spPreviewSpectrum, static_cast<void (QSpinBox::*)(int const)>(&QSpinBox::valueChanged), this,
          &QuasiView::notifyPreviewSpectrumChanged);

  connect(m_uiForm.pbPlotPreview, &QPushButton::clicked, this, &QuasiView::notifyPlotCurrentPreview);
  connect(m_uiForm.pbSave, &QPushButton::clicked, this, &QuasiView::notifySaveClicked);
  connect(m_uiForm.pbPlot, &QPushButton::clicked, this, &QuasiView::notifyPlotClicked);

  m_uiForm.dsSample->isOptional(true);
  m_uiForm.dsResolution->isOptional(true);
  m_uiForm.dsSample->setWorkspaceTypes({"Workspace2D"});
  m_uiForm.dsResolution->setWorkspaceTypes({"Workspace2D"});
}

void QuasiView::subscribe(IQuasiPresenter *presenter) { m_presenter = presenter; }

IRunView *QuasiView::getRunView() const { return m_uiForm.runWidget; }

DataSelector *QuasiView::sampleSelector() const { return m_uiForm.dsSample; }

DataSelector *QuasiView::resolutionSelector() const { return m_uiForm.dsResolution; }

DataSelector *QuasiView::resNormSelector() const { return m_uiForm.dsResNorm; }

FileFinderWidget *QuasiView::fixWidthFileFinder() const { return m_uiForm.mwFixWidthDat; }

void QuasiView::setupFitOptions() {
  auto const useQuickBayes = SettingsHelper::hasDevelopmentFlag("quickbayes");

  m_uiForm.cbBackground->clear();
  if (useQuickBayes) {
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::LINEAR));
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::FLAT));
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::ZERO));

    m_uiForm.chkFixWidth->hide();
    m_uiForm.mwFixWidthDat->hide();

    m_uiForm.chkUseResNorm->hide();
    m_uiForm.dsResNorm->hide();

    m_uiForm.chkSequentialFit->hide();
  } else {
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::SLOPING));
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::FLAT));
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::ZERO));

    m_uiForm.chkFixWidth->show();
    m_uiForm.mwFixWidthDat->show();

    m_uiForm.dsResNorm->show();
    m_uiForm.chkUseResNorm->show();

    m_uiForm.chkSequentialFit->show();
  }
}

void QuasiView::setupPropertyBrowser() {
  auto const useQuickBayes = SettingsHelper::hasDevelopmentFlag("quickbayes");

  m_properties.clear();
  m_dblManager->clear();
  m_propTree->clear();

  m_uiForm.treeSpace->addWidget(m_propTree);
  m_properties["EMin"] = m_dblManager->addProperty("EMin");
  m_properties["EMax"] = m_dblManager->addProperty("EMax");

  m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);

  m_propTree->addProperty(m_properties["EMin"]);
  m_propTree->addProperty(m_properties["EMax"]);

  if (!useQuickBayes) {
    m_properties["SampleBinning"] = m_dblManager->addProperty("Sample Binning");
    m_properties["ResBinning"] = m_dblManager->addProperty("Resolution Binning");

    m_dblManager->setDecimals(m_properties["SampleBinning"], INT_DECIMALS);
    m_dblManager->setDecimals(m_properties["ResBinning"], INT_DECIMALS);

    m_propTree->addProperty(m_properties["SampleBinning"]);
    m_propTree->addProperty(m_properties["ResBinning"]);

    m_dblManager->setValue(m_properties["SampleBinning"], 1);
    m_dblManager->setMinimum(m_properties["SampleBinning"], 1);
    m_dblManager->setValue(m_properties["ResBinning"], 1);
    m_dblManager->setMinimum(m_properties["ResBinning"], 1);
  }
  InterfaceUtils::formatTreeWidget(m_propTree, m_properties);
}

void QuasiView::setupPlotOptions() {
  auto const useQuickBayes = SettingsHelper::hasDevelopmentFlag("quickbayes");

  m_uiForm.cbPlot->clear();
  if (useQuickBayes) {
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::ALL));
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::AMPLITUDE));
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::GAMMA));
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::PROB));
  } else {
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::ALL));
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::AMPLITUDE));
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::FWHM));
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::PROB));
  }
}

void QuasiView::notifySampleInputReady(QString const &workspaceName) {
  m_presenter->handleSampleInputReady(workspaceName.toStdString());
}

void QuasiView::notifyResolutionInputReady(QString const &workspaceName) {
  m_presenter->handleResolutionInputReady(workspaceName.toStdString());
}

void QuasiView::notifyFileAutoLoaded() { m_presenter->handleFileAutoLoaded(); }

void QuasiView::notifyPreviewSpectrumChanged(int const value) {
  (void)value;
  m_presenter->handlePreviewSpectrumChanged();
}

void QuasiView::notifyPlotCurrentPreview() { m_presenter->handlePlotCurrentPreview(); }

void QuasiView::notifySaveClicked() { m_presenter->handleSaveClicked(); }

void QuasiView::notifyPlotClicked() { m_presenter->handlePlotClicked(); }

void QuasiView::minEValueChanged(double const min) {
  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &QuasiView::updateProperties);
  m_dblManager->setValue(m_properties["EMin"], min);
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &QuasiView::updateProperties);
}

void QuasiView::maxEValueChanged(double const max) {
  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &QuasiView::updateProperties);
  m_dblManager->setValue(m_properties["EMax"], max);
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &QuasiView::updateProperties);
}

void QuasiView::updateProperties(QtProperty *prop, double const value) {
  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("QuasiERange");

  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &QuasiView::updateProperties);

  if (prop == m_properties["EMin"]) {
    InterfaceUtils::setRangeSelectorMin(m_dblManager, m_properties["EMin"], m_properties["EMax"], eRangeSelector,
                                        value);
  } else if (prop == m_properties["EMax"]) {
    InterfaceUtils::setRangeSelectorMax(m_dblManager, m_properties["EMin"], m_properties["EMax"], eRangeSelector,
                                        value);
  }

  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &QuasiView::updateProperties);
}

void QuasiView::handleProgramChange(int const index) {
  int const numberOptions = m_uiForm.cbPlot->count();
  switch (index) {
  case 0:
    m_uiForm.cbPlot->setItemText(numberOptions - 1, "Prob");
    break;
  case 1:
    m_uiForm.cbPlot->setItemText(numberOptions - 1, "Beta");
    break;
  }
}

void QuasiView::setPreviewSpectrumMax(std::size_t const max) {
  m_uiForm.spPreviewSpectrum->setMaximum(static_cast<int>(max));
}

void QuasiView::setXRange(std::pair<double, double> const &range) {
  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("QuasiERange");

  InterfaceUtils::setRangeSelector(m_dblManager, eRangeSelector, m_properties["EMin"], m_properties["EMax"], range);
  InterfaceUtils::setPlotPropertyRange(m_dblManager, eRangeSelector, m_properties["EMin"], m_properties["EMax"], range);
  eRangeSelector->setMinimum(range.first);
  eRangeSelector->setMaximum(range.second);
}

void QuasiView::watchADS(bool const watch) { m_uiForm.ppPlot->watchADS(watch); }

void QuasiView::clearPlot() { m_uiForm.ppPlot->clear(); }

bool QuasiView::hasSpectrum(std::string const &label) const {
  return m_uiForm.ppPlot->hasCurve(QString::fromStdString(label));
}

void QuasiView::addSpectrum(std::string const &label, Mantid::API::MatrixWorkspace_sptr const &workspace,
                            std::size_t const spectrumIndex, std::string const &colour) {
  m_uiForm.ppPlot->addSpectrum(QString::fromStdString(label), workspace, spectrumIndex, toQColor(colour));
}

std::size_t QuasiView::previewSpectrum() const { return static_cast<std::size_t>(m_uiForm.spPreviewSpectrum->value()); }

std::string QuasiView::sampleName() const { return m_uiForm.dsSample->getCurrentDataName().toStdString(); }

std::string QuasiView::resolutionName() const { return m_uiForm.dsResolution->getCurrentDataName().toStdString(); }

std::string QuasiView::resNormName() const { return m_uiForm.dsResNorm->getCurrentDataName().toStdString(); }

std::string QuasiView::fixWidthName() const { return m_uiForm.mwFixWidthDat->getFirstFilename().toStdString(); }

std::string QuasiView::programName() const { return m_uiForm.cbProgram->currentText().toStdString(); }

std::string QuasiView::backgroundName() const { return m_uiForm.cbBackground->currentText().toStdString(); }

std::string QuasiView::plotName() const { return m_uiForm.cbPlot->currentText().toLower().toStdString(); }

double QuasiView::eMin() const { return m_dblManager->value(m_properties["EMin"]); }

double QuasiView::eMax() const { return m_dblManager->value(m_properties["EMax"]); }

int QuasiView::sampleBinning() const { return m_properties["SampleBinning"]->valueText().toInt(); }

int QuasiView::resolutionBinning() const { return m_properties["ResBinning"]->valueText().toInt(); }

bool QuasiView::useResolution() const { return m_uiForm.chkUseResNorm->isChecked(); }

bool QuasiView::fixWidth() const { return m_uiForm.chkFixWidth->isChecked(); }

bool QuasiView::elasticPeak() const { return m_uiForm.chkElasticPeak->isChecked(); }

bool QuasiView::sequentialFit() const { return m_uiForm.chkSequentialFit->isChecked(); }

void QuasiView::setPlotResultEnabled(bool const enable) {
  m_uiForm.pbPlot->setEnabled(enable);
  m_uiForm.cbPlot->setEnabled(enable);
}

void QuasiView::setSaveResultEnabled(bool const enable) { m_uiForm.pbSave->setEnabled(enable); }

void QuasiView::enableUseResolution(bool const enable) {
  m_uiForm.chkUseResNorm->setEnabled(enable);
  if (!enable)
    m_uiForm.chkUseResNorm->setChecked(false);
}

void QuasiView::enableView(bool const enable) {
  m_uiForm.dsSample->setEnabled(enable);
  m_uiForm.dsResolution->setEnabled(enable);
}

bool QuasiView::displaySaveDirectoryMessage() const {
  char const *textMessage = "BayesQuasi requires a default save directory and "
                            "one is not currently set."
                            " If run, the algorithm will default to saving files "
                            "to the current working directory."
                            " Would you still like to run the algorithm?";
  return QMessageBox::question(nullptr, "Save Directory", textMessage, QMessageBox::Yes, QMessageBox::No,
                               QMessageBox::NoButton) == QMessageBox::No;
}

void QuasiView::setFileExtensionsByName(bool const filter) {
  QStringList const noSuffixes{""};
  m_uiForm.dsSample->setFBSuffixes(filter ? getSampleFBSuffixes(TAB_NAME) : getExtensions(TAB_NAME));
  m_uiForm.dsSample->setWSSuffixes(filter ? getSampleWSSuffixes(TAB_NAME) : noSuffixes);
  m_uiForm.dsResolution->setFBSuffixes(filter ? getResolutionFBSuffixes(TAB_NAME) : getExtensions(TAB_NAME));
  m_uiForm.dsResolution->setWSSuffixes(filter ? getResolutionWSSuffixes(TAB_NAME) : noSuffixes);
}

void QuasiView::setLoadHistory(bool const loadHistory) {
  m_uiForm.dsSample->setLoadProperty("LoadHistory", loadHistory);
  m_uiForm.dsResolution->setLoadProperty("LoadHistory", loadHistory);
  m_uiForm.dsResNorm->setLoadProperty("LoadHistory", loadHistory);
}

void QuasiView::loadSettings(const QSettings &settings) {
  m_uiForm.dsSample->readSettings(settings.group());
  m_uiForm.dsResolution->readSettings(settings.group());
  m_uiForm.dsResNorm->readSettings(settings.group());
  m_uiForm.mwFixWidthDat->readSettings(settings.group());
}

} // namespace MantidQt::CustomInterfaces
