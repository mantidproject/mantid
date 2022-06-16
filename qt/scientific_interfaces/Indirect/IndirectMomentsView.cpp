// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectMomentsView.h"
#include "IndirectDataValidationHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QDoubleValidator>
#include <QFileInfo>

using namespace IndirectDataValidationHelper;
using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
IndirectMomentsView::IndirectMomentsView(QWidget *parent) {
  m_uiForm.setupUi(parent);
  m_dblManager = new QtDoublePropertyManager();
  m_dblEdFac = new DoubleEditorFactory(this);

  m_uiForm.ppRawPlot->setCanvasColour(QColor(240, 240, 240));
  m_uiForm.ppMomentsPreview->setCanvasColour(QColor(240, 240, 240));

  MantidWidgets::RangeSelector *xRangeSelector = m_uiForm.ppRawPlot->addRangeSelector("XRange");
  connect(xRangeSelector, SIGNAL(selectionChanged(double, double)), this, SLOT(rangeChanged(double, double)));

  connect(m_uiForm.dsInput, SIGNAL(dataReady(QString const &)), this, SIGNAL(dataReady(const QString &)));
  connect(m_uiForm.ckScale, SIGNAL(stateChanged(int)), this, SIGNAL(scaleChanged(int)));
  connect(m_uiForm.spScale, SIGNAL(valueChanged(double)), this, SIGNAL(scaleValueChanged(double)));
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SIGNAL(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SIGNAL(saveClicked()));

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SIGNAL(valueChanged(QtProperty *, double)));

  // Allows empty workspace selector when initially selected
  m_uiForm.dsInput->isOptional(true);

  // Disables searching for run files in the data archive
  m_uiForm.dsInput->isForRunFiles(false);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IndirectMomentsView::~IndirectMomentsView() { m_propTrees["MomentsPropTree"]->unsetFactoryForManager(m_dblManager); }

/**
 * Updates the property manager when the range selector is moved.
 *
 * @param min :: The new value of the lower guide
 * @param max :: The new value of the upper guide
 */
void IndirectMomentsView::rangeChanged(double min, double max) {
  m_dblManager->setValue(m_properties["EMin"], min);
  m_dblManager->setValue(m_properties["EMax"], max);
}

void IndirectMomentsView::setupProperties() {

  const unsigned int NUM_DECIMALS = 6;
  // PROPERTY TREE
  m_propTrees["MomentsPropTree"] = new QtTreePropertyBrowser();
  m_propTrees["MomentsPropTree"]->setFactoryForManager(m_dblManager, m_dblEdFac);
  m_uiForm.properties->addWidget(m_propTrees["MomentsPropTree"]);
  m_properties["EMin"] = m_dblManager->addProperty("EMin");
  m_propTrees["MomentsPropTree"]->addProperty(m_properties["EMin"]);
  m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);

  m_properties["EMax"] = m_dblManager->addProperty("EMax");
  m_propTrees["MomentsPropTree"]->addProperty(m_properties["EMax"]);
  m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);
}

IndirectPlotOptionsView *IndirectMomentsView::getPlotOptions() { return m_uiForm.ipoPlotOptions; }

std::string IndirectMomentsView::getDataName() { return m_uiForm.dsInput->getCurrentDataName().toStdString(); }

bool IndirectMomentsView::validate() {
  UserInputValidator uiv;
  validateDataIsOfType(uiv, m_uiForm.dsInput, "Sample", DataType::Sqw);

  auto const errorMessage = uiv.generateErrorMessage();
  if (!errorMessage.isEmpty())
    showMessageBox(errorMessage);
  return errorMessage.isEmpty();
}

/**
 * Clears previous plot data (in both preview and raw plot) and sets the new
 * range bars
 */
void IndirectMomentsView::plotNewData(QString const &filename) {
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updateProperties(QtProperty *, double)));
  // Clears previous plotted data
  m_uiForm.ppRawPlot->clear();
  m_uiForm.ppMomentsPreview->clear();

  // Update plot and change data in interface
  m_uiForm.ppRawPlot->addSpectrum("Raw", filename, 0);

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateProperties(QtProperty *, double)));
}

/**
 * Sets the edge bounds of plot to prevent the user inputting invalid values
 * Also sets limits for range selector movement
 *
 * @param min :: The lower bound property in the property browser
 * @param max :: The upper bound property in the property browser
 * @param bounds :: The upper and lower bounds to be set
 */
void IndirectMomentsView::setPlotPropertyRange(const QPair<double, double> &bounds) {
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updateProperties(QtProperty *, double)));
  m_dblManager->setMinimum(m_properties["EMin"], bounds.first);
  m_dblManager->setMaximum(m_properties["EMin"], bounds.second);
  m_dblManager->setMinimum(m_properties["EMax"], bounds.first);
  m_dblManager->setMaximum(m_properties["EMax"], bounds.second);
  auto xRangeSelector = m_uiForm.ppRawPlot->getRangeSelector("XRange");
  xRangeSelector->setBounds(bounds.first, bounds.second);
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateProperties(QtProperty *, double)));
}

/**
 * Set the position of the range selectors on the mini plot
 *
 * @param lower :: The lower bound property in the property browser
 * @param upper :: The upper bound property in the property browser
 * @param bounds :: The upper and lower bounds to be set
 * @param range :: The range to set the range selector to.
 */
void IndirectMomentsView::setRangeSelector(const QPair<double, double> &bounds,
                                           const boost::optional<QPair<double, double>> &range) {
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updateProperties(QtProperty *, double)));
  m_dblManager->setValue(m_properties["EMin"], bounds.first);
  m_dblManager->setValue(m_properties["EMax"], bounds.second);
  auto xRangeSelector = m_uiForm.ppRawPlot->getRangeSelector("XRange");
  if (range) {
    xRangeSelector->setMinimum(range.get().first);
    xRangeSelector->setMaximum(range.get().second);
    // clamp the bounds of the selector
    xRangeSelector->setRange(range.get().first, range.get().second);
  } else {
    xRangeSelector->setMinimum(bounds.first);
    xRangeSelector->setMaximum(bounds.second);
  }
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateProperties(QtProperty *, double)));
}

/**
 * Set the minimum of a range selector if it is less than the maximum value.
 * To be used when changing the min or max via the Property table
 *
 * @param minProperty :: The property managing the minimum of the range
 * @param maxProperty :: The property managing the maximum of the range
 * @param rangeSelector :: The range selector
 * @param newValue :: The new value for the minimum
 */
void IndirectMomentsView::setRangeSelectorMin(double newValue) {
  if (newValue <= m_properties["EMax"]->valueText().toDouble())
    getRangeSelector()->setMinimum(newValue);
  else
    m_dblManager->setValue(m_properties["EMin"], getRangeSelector()->getMinimum());
}

/**
 * Set the maximum of a range selector if it is greater than the minimum value
 * To be used when changing the min or max via the Property table
 *
 * @param minProperty :: The property managing the minimum of the range
 * @param maxProperty :: The property managing the maximum of the range
 * @param rangeSelector :: The range selector
 * @param newValue :: The new value for the maximum
 */
void IndirectMomentsView::setRangeSelectorMax(double newValue) {
  if (newValue >= m_properties["EMin"]->valueText().toDouble())
    getRangeSelector()->setMaximum(newValue);
  else
    m_dblManager->setValue(m_properties["EMax"], getRangeSelector()->getMaximum());
}

void IndirectMomentsView::replot() { m_uiForm.ppRawPlot->replot(); }

MantidWidgets::RangeSelector *IndirectMomentsView::getRangeSelector() {
  return m_uiForm.ppRawPlot->getRangeSelector("XRange");
}

void IndirectMomentsView::plotOutput(QString outputWorkspace) {
  // Plot each spectrum
  m_uiForm.ppMomentsPreview->clear();
  m_uiForm.ppMomentsPreview->addSpectrum("M0", outputWorkspace, 0, Qt::green);
  m_uiForm.ppMomentsPreview->addSpectrum("M1", outputWorkspace, 1, Qt::black);
  m_uiForm.ppMomentsPreview->addSpectrum("M2", outputWorkspace, 2, Qt::red);
  m_uiForm.ppMomentsPreview->resizeX();

  // Enable plot and save buttons
  m_uiForm.pbSave->setEnabled(true);
}

void IndirectMomentsView::setFBSuffixes(QStringList const suffix) { m_uiForm.dsInput->setFBSuffixes(suffix); }

void IndirectMomentsView::setWSSuffixes(QStringList const suffix) { m_uiForm.dsInput->setWSSuffixes(suffix); }

void IndirectMomentsView::updateRunButton(bool enabled, std::string const &enableOutputButtons, QString const &message,
                                          QString const &tooltip) {
  m_uiForm.pbRun->setEnabled(enabled);
  m_uiForm.pbRun->setText(message);
  m_uiForm.pbRun->setToolTip(tooltip);
  if (enableOutputButtons != "unchanged")
    m_uiForm.pbSave->setEnabled(enabled);
}

} // namespace MantidQt::CustomInterfaces