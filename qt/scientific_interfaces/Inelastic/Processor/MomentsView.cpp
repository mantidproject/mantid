// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MomentsView.h"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"
#include "MomentsPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QDoubleValidator>
#include <QFileInfo>

using namespace DataValidationHelper;
using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MomentsView::MomentsView(QWidget *parent) : QWidget(parent), m_presenter() {
  m_uiForm.setupUi(parent);
  m_dblManager = new QtDoublePropertyManager();
  m_dblEdFac = new DoubleEditorFactory(this);

  m_uiForm.ppRawPlot->setCanvasColour(QColor(240, 240, 240));
  m_uiForm.ppMomentsPreview->setCanvasColour(QColor(240, 240, 240));

  MantidWidgets::RangeSelector *xRangeSelector = m_uiForm.ppRawPlot->addRangeSelector("XRange");

  connect(m_uiForm.dsInput, &DataSelector::dataReady, this, &MomentsView::notifyDataReady);
  connect(m_uiForm.ckScale, &QCheckBox::stateChanged, this, &MomentsView::notifyScaleChanged);
  connect(m_uiForm.spScale, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
          &MomentsView::notifyScaleValueChanged);
  connect(m_uiForm.pbSave, &QPushButton::clicked, this, &MomentsView::notifySaveClicked);

  connect(xRangeSelector, &MantidWidgets::RangeSelector::selectionChanged, this, &MomentsView::notifyRangeChanged);
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &MomentsView::notifyValueChanged);

  // Allows empty workspace selector when initially selected
  m_uiForm.dsInput->isOptional(true);

  // Disables searching for run files in the data archive
  m_uiForm.dsInput->isForRunFiles(false);

  // setup Property tree
  setupProperties();
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MomentsView::~MomentsView() { m_propTrees["MomentsPropTree"]->unsetFactoryForManager(m_dblManager); }

/**Subscribes the presenter to the view.
 *@param presenter: A pointer to the presenter to which the view is subscribed.
 */
void MomentsView::subscribePresenter(IMomentsPresenter *presenter) { m_presenter = presenter; }

void MomentsView::notifyDataReady(const QString &dataName) { m_presenter->handleDataReady(dataName.toStdString()); }
/**
 * Updates the property manager when the range selector is moved.
 *
 * @param min :: The new value of the lower guide
 * @param max :: The new value of the upper guide
 */
void MomentsView::notifyRangeChanged(double min, double max) {
  m_dblManager->setValue(m_properties["EMin"], min);
  m_dblManager->setValue(m_properties["EMax"], max);
}

void MomentsView::notifyScaleChanged(int const scale) { m_presenter->handleScaleChanged(scale == Qt::Checked); }

void MomentsView::notifyScaleValueChanged(double const value) { m_presenter->handleScaleValueChanged(value); }

void MomentsView::notifyValueChanged(QtProperty *prop, double value) {
  m_presenter->handleValueChanged(prop->propertyName().toStdString(), value);
  prop->propertyName() == "EMin" ? setRangeSelectorMin(value) : setRangeSelectorMax(value);
}

void MomentsView::notifySaveClicked() { m_presenter->handleSaveClicked(); }

void MomentsView::setupProperties() {

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

void MomentsView::setFBSuffixes(QStringList const &suffix) { m_uiForm.dsInput->setFBSuffixes(suffix); }

void MomentsView::setWSSuffixes(QStringList const &suffix) { m_uiForm.dsInput->setWSSuffixes(suffix); }

void MomentsView::setLoadHistory(bool doLoadHistory) {
  m_uiForm.dsInput->setLoadProperty("LoadHistory", doLoadHistory);
}

IOutputPlotOptionsView *MomentsView::getPlotOptions() const { return m_uiForm.ipoPlotOptions; }

MantidWidgets::DataSelector *MomentsView::getDataSelector() const { return m_uiForm.dsInput; }

IRunView *MomentsView::getRunView() const { return m_uiForm.runWidget; }

std::string MomentsView::getDataName() const { return m_uiForm.dsInput->getCurrentDataName().toStdString(); }

/**
 * Clears previous plot data (in both preview and raw plot) and sets the new
 * range bars
 * @param filename :: Name of the loaded sqw workspace
 */
void MomentsView::plotNewData(std::string const &filename) {
  // Clears previous plotted data
  m_uiForm.ppRawPlot->clear();
  m_uiForm.ppMomentsPreview->clear();

  // Update plot and change data in interface
  m_uiForm.ppRawPlot->addSpectrum("Raw", QString(filename.data()), 0);
}

/**
 * Sets the edge bounds of plot to prevent the user inputting invalid values
 * Also sets limits for range selector movement
 *
 * @param bounds :: The upper and lower bounds to be set in the property browser
 */
void MomentsView::setPlotPropertyRange(const QPair<double, double> &bounds) {
  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &MomentsView::notifyValueChanged);
  m_dblManager->setMinimum(m_properties["EMin"], bounds.first);
  m_dblManager->setMaximum(m_properties["EMin"], bounds.second);
  m_dblManager->setMinimum(m_properties["EMax"], bounds.first);
  m_dblManager->setMaximum(m_properties["EMax"], bounds.second);
  auto xRangeSelector = getRangeSelector();
  xRangeSelector->setBounds(bounds.first, bounds.second);
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &MomentsView::notifyValueChanged);
}

/**
 * Set the position of the range selectors on the mini plot
 *
 * @param bounds :: The upper and lower bounds to be set
 */
void MomentsView::setRangeSelector(const QPair<double, double> &bounds) {
  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &MomentsView::notifyValueChanged);

  double deltaX = abs(bounds.second - bounds.first);
  double lowX = bounds.first + (0.1) * deltaX;
  double highX = bounds.second - (0.1) * deltaX;

  m_dblManager->setValue(m_properties["EMin"], lowX);
  m_dblManager->setValue(m_properties["EMax"], highX);

  // connecting back so that the model is updated.
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &MomentsView::notifyValueChanged);

  auto xRangeSelector = getRangeSelector();
  xRangeSelector->setRange(bounds.first, bounds.second);
  xRangeSelector->setMinimum(lowX);
  xRangeSelector->setMaximum(highX);
}

/**
 * Set the minimum of a range selector if it is less than the maximum value.
 * To be used when changing the min or max via the Property table
 *
 * @param newValue :: The new value for the minimum
 */
void MomentsView::setRangeSelectorMin(double newValue) {
  if (newValue <= m_properties["EMax"]->valueText().toDouble())
    getRangeSelector()->setMinimum(newValue);
  else
    m_dblManager->setValue(m_properties["EMin"], getRangeSelector()->getMinimum());
}

/**
 * Set the maximum of a range selector if it is greater than the minimum value
 * To be used when changing the min or max via the Property table
 *
 * @param newValue :: The new value for the maximum
 */
void MomentsView::setRangeSelectorMax(double newValue) {
  if (newValue >= m_properties["EMin"]->valueText().toDouble())
    getRangeSelector()->setMaximum(newValue);
  else
    m_dblManager->setValue(m_properties["EMax"], getRangeSelector()->getMaximum());
}

void MomentsView::replot() { m_uiForm.ppRawPlot->replot(); }

MantidWidgets::RangeSelector *MomentsView::getRangeSelector() { return m_uiForm.ppRawPlot->getRangeSelector("XRange"); }

void MomentsView::plotOutput(MatrixWorkspace_sptr outputWorkspace) {
  // Plot each spectrum
  m_uiForm.ppMomentsPreview->clear();
  m_uiForm.ppMomentsPreview->addSpectrum("M0", outputWorkspace, 0, Qt::green);
  m_uiForm.ppMomentsPreview->addSpectrum("M1", outputWorkspace, 1, Qt::black);
  m_uiForm.ppMomentsPreview->addSpectrum("M2", outputWorkspace, 2, Qt::red);
  m_uiForm.ppMomentsPreview->resizeX();

  // Enable plot and save buttons
  setSaveResultEnabled(true);
}

void MomentsView::setSaveResultEnabled(bool enable) { m_uiForm.pbSave->setEnabled(enable); }

void MomentsView::showMessageBox(std::string const &message) const {
  QMessageBox::information(parentWidget(), this->windowTitle(), QString::fromStdString(message));
}

} // namespace MantidQt::CustomInterfaces
