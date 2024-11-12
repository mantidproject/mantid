// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ResNormView.h"

#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunView.h"

#include "ResNormPresenter.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using namespace MantidQt::CustomInterfaces::InterfaceUtils;

namespace {
Mantid::Kernel::Logger g_log("ResNormView");
} // namespace

namespace MantidQt::CustomInterfaces {

ResNormView::ResNormView(QWidget *parent)
    : QWidget(parent), m_presenter(), m_propTree(), m_dblManager(), m_dblEdFac(), m_properties(), m_selectors() {
  m_uiForm.setupUi(parent);
  setup();
}

ResNormView::~ResNormView() { m_propTree->unsetFactoryForManager(m_dblManager); }

void ResNormView::setup() {
  // Create QtTreePropertyBrowser object
  m_propTree = new QtTreePropertyBrowser();
  m_propTree->setIndentation(0);
  m_uiForm.treeSpace->addWidget(m_propTree);

  // Create Editor Factories
  m_dblEdFac = new DoubleEditorFactory(this);
  m_dblManager = new QtDoublePropertyManager();
  m_propTree->setFactoryForManager(m_dblManager, m_dblEdFac);

  m_selectors = {{"Vanadium", m_uiForm.dsVanadium}, {"Resolution", m_uiForm.dsResolution}};
  m_properties["EMin"] = m_dblManager->addProperty("EMin");
  m_properties["EMax"] = m_dblManager->addProperty("EMax");
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ResNormView::notifyDoublePropertyChanged);

  m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);

  m_propTree->addProperty(m_properties["EMin"]);
  m_propTree->addProperty(m_properties["EMax"]);

  // Create range selector
  auto eRangeSelector = m_uiForm.ppPlot->addRangeSelector("ResNormViewERange");
  connect(eRangeSelector, &MantidWidgets::RangeSelector::minValueChanged, this, &ResNormView::minValueChanged);
  connect(eRangeSelector, &MantidWidgets::RangeSelector::maxValueChanged, this, &ResNormView::maxValueChanged);

  // Connect data selector to handler method
  connect(m_uiForm.dsVanadium, &DataSelector::dataReady, this, &ResNormView::notifyVanadiumInputReady);
  connect(m_uiForm.dsResolution, &DataSelector::dataReady, this, &ResNormView::notifyResolutionInputReady);

  // Connect the preview spectrum selector
  connect(m_uiForm.spPreviewSpectrum, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
          &ResNormView::notifyPreviewSpecChanged);
  // Post Plot and Save
  connect(m_uiForm.pbSave, &QPushButton::clicked, this, &ResNormView::notifySaveClicked);
  connect(m_uiForm.pbPlot, &QPushButton::clicked, this, &ResNormView::notifyPlotClicked);
  connect(m_uiForm.pbPlotCurrent, &QPushButton::clicked, this, &ResNormView::notifyPlotCurrentPreviewClicked);

  // Allows empty workspace selector when initially selected
  m_uiForm.dsVanadium->isOptional(true);
  m_uiForm.dsResolution->isOptional(true);
}

void ResNormView::subscribePresenter(IResNormPresenter *presenter) { m_presenter = presenter; }

IRunView *ResNormView::getRunView() const { return m_uiForm.runWidget; }

void ResNormView::setSuffixes(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("ResNorm");
  m_uiForm.dsVanadium->setFBSuffixes(filter ? getVanadiumFBSuffixes(tabName) : getExtensions(tabName));
  m_uiForm.dsVanadium->setWSSuffixes(filter ? getVanadiumWSSuffixes(tabName) : noSuffixes);
  m_uiForm.dsResolution->setFBSuffixes(filter ? getResolutionFBSuffixes(tabName) : getExtensions(tabName));
  m_uiForm.dsResolution->setWSSuffixes(filter ? getResolutionWSSuffixes(tabName) : noSuffixes);
}

void ResNormView::setLoadHistory(bool doLoadHistory) {
  m_uiForm.dsVanadium->setLoadProperty("LoadHistory", doLoadHistory);
  m_uiForm.dsResolution->setLoadProperty("LoadHistory", doLoadHistory);
}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The current settings
 */
void ResNormView::loadSettings(const QSettings &settings) {
  m_uiForm.dsVanadium->readSettings(settings.group());
  m_uiForm.dsResolution->readSettings(settings.group());
}

void ResNormView::notifyVanadiumInputReady(const QString &filename) {
  m_presenter->handleVanadiumInputReady(filename.toStdString());
}

void ResNormView::notifyPreviewSpecChanged(int value) { m_presenter->handlePreviewSpecChanged(value); }

void ResNormView::watchADS(bool watch) const { m_uiForm.ppPlot->watchADS(watch); }

void ResNormView::setMaximumSpectrum(int maximum) const { m_uiForm.spPreviewSpectrum->setMaximum(maximum); }

void ResNormView::updateSelectorRange(std::string const &filename) const {
  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("ResNormViewERange");
  QPair<double, double> res;

  auto const range = getXRangeFromWorkspace(filename);

  // Use the values from the instrument parameter file if we can
  // The maximum and minimum value of the plot
  if (getResolutionRangeFromWs(filename, res)) {
    // ResNormView resolution should be +/- 10 * the IPF resolution
    res.first = res.first * 10;
    res.second = res.second * 10;
    setRangeSelector(m_dblManager, eRangeSelector, m_properties["EMin"], m_properties["EMax"], res);
  } else {
    setRangeSelector(m_dblManager, eRangeSelector, m_properties["EMin"], m_properties["EMax"], range);
  }

  setPlotPropertyRange(m_dblManager, eRangeSelector, m_properties["EMin"], m_properties["EMax"], range);

  // Set the current values of the range bars
  eRangeSelector->setMinimum(range.first);
  eRangeSelector->setMaximum(range.second);
}

/**
 * Plots the loaded resolution file on the mini plot.
 *
 * @param filename Name of the workspace to plot
 */
void ResNormView::notifyResolutionInputReady(const QString &filename) {
  (void)filename;
  m_presenter->handleResolutionInputReady();
}

void ResNormView::updatePlot(std::string const &curveName, size_t wsIndex, std::string const &filename,
                             QColor const &color) {
  auto const name = (curveName == "Vanadium" || curveName == "Resolution") ? getCurrentDataName(curveName) : filename;
  try {
    m_uiForm.ppPlot->addSpectrum(QString::fromStdString(curveName), QString::fromStdString(name), wsIndex, color);
  } catch (std::exception const &ex) {
    g_log.warning(ex.what());
  }
}

/**
 * Updates the property manager when the lower guide is moved on the mini plot
 *
 * @param min :: The new value of the lower guide
 */
void ResNormView::minValueChanged(double min) {
  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ResNormView::notifyDoublePropertyChanged);
  m_dblManager->setValue(m_properties["EMin"], min);
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ResNormView::notifyDoublePropertyChanged);
}

/**
 * Updates the property manager when the upper guide is moved on the mini plot
 *
 * @param max :: The new value of the upper guide
 */
void ResNormView::maxValueChanged(double max) {
  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ResNormView::notifyDoublePropertyChanged);
  m_dblManager->setValue(m_properties["EMax"], max);
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ResNormView::notifyDoublePropertyChanged);
}

/**
 * Handles when properties in the property manager are updated.
 *
 * @param prop :: The property being updated
 * @param val :: The new value for the property
 */
void ResNormView::notifyDoublePropertyChanged(QtProperty *prop, double val) {
  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("ResNormViewERange");
  m_presenter->handleDoubleValueChanged(prop->propertyName().toStdString(), val);
  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ResNormView::notifyDoublePropertyChanged);

  if (prop == m_properties["EMin"]) {
    setRangeSelectorMin(m_dblManager, m_properties["EMin"], m_properties["EMax"], eRangeSelector, val);
  } else if (prop == m_properties["EMax"]) {
    setRangeSelectorMax(m_dblManager, m_properties["EMin"], m_properties["EMax"], eRangeSelector, val);
  }
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ResNormView::notifyDoublePropertyChanged);
}

/**
 * Plot the current spectrum in the miniplot
 */

void ResNormView::notifyPlotCurrentPreviewClicked() { m_presenter->handlePlotCurrentPreview(); }

void ResNormView::notifyPlotClicked() { m_presenter->handlePlotClicked(m_uiForm.cbPlot->currentText().toStdString()); }

void ResNormView::notifySaveClicked() { m_presenter->handleSaveClicked(); }

bool ResNormView::plotHasCurve(std::string const &curveName) const {
  return m_uiForm.ppPlot->hasCurve(QString::fromStdString(curveName));
}

std::string ResNormView::getCurrentDataName(std::string const &selectorName) const {
  return m_selectors[selectorName]->getCurrentDataName().toStdString();
}

void ResNormView::clearPlot() const { m_uiForm.ppPlot->clear(); }

DataSelector *ResNormView::getDataSelector(std::string const &selectorName) const { return m_selectors[selectorName]; }

void ResNormView::setPlotResultEnabled(bool enabled) const {
  m_uiForm.pbPlot->setEnabled(enabled);
  m_uiForm.cbPlot->setEnabled(enabled);
}

void ResNormView::setSaveResultEnabled(bool enabled) const { m_uiForm.pbSave->setEnabled(enabled); }

void ResNormView::setButtonsEnabled(bool enabled) const {

  setPlotResultEnabled(enabled);
  setSaveResultEnabled(enabled);
}

void ResNormView::setPlotResultIsPlotting(bool plotting) const {
  m_uiForm.pbPlot->setText(plotting ? "Plotting..." : "Plot");
  setButtonsEnabled(!plotting);
}

} // namespace MantidQt::CustomInterfaces
