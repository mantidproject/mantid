// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationSymmetriseTabView.h"
#include "IndirectDataValidationHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Plotting/SingleSelector.h"

#include <QDoubleValidator>
#include <QFileInfo>

using namespace IndirectDataValidationHelper;
using namespace Mantid::API;

namespace {

QPair<double, double> getXRangeFromWorkspace(const Mantid::API::MatrixWorkspace_const_sptr &workspace) {
  auto const xValues = workspace->x(0);
  return QPair<double, double>(xValues.front(), xValues.back());
}
} // namespace

using MantidQt::MantidWidgets::AxisID;
namespace MantidQt::CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
InelasticDataManipulationSymmetriseTabView::InelasticDataManipulationSymmetriseTabView(QWidget *parent) {
  m_uiForm.setupUi(parent);
  m_dblManager = new QtDoublePropertyManager();
  m_grpManager = new QtGroupPropertyManager();

  m_uiForm.ppRawPlot->setCanvasColour(QColor(240, 240, 240));
  m_uiForm.ppPreviewPlot->setCanvasColour(QColor(240, 240, 240));

  int numDecimals = 6;

  // Property Trees
  m_propTrees["SymmPropTree"] = new QtTreePropertyBrowser();
  m_uiForm.properties->addWidget(m_propTrees["SymmPropTree"]);

  m_propTrees["SymmPVPropTree"] = new QtTreePropertyBrowser();
  m_uiForm.propertiesPreview->addWidget(m_propTrees["SymmPVPropTree"]);

  // Editor Factories
  auto *doubleEditorFactory = new DoubleEditorFactory();
  m_propTrees["SymmPropTree"]->setFactoryForManager(m_dblManager, doubleEditorFactory);

  // Raw Properties
  m_properties["EMin"] = m_dblManager->addProperty("EMin");
  m_dblManager->setDecimals(m_properties["EMin"], numDecimals);
  m_propTrees["SymmPropTree"]->addProperty(m_properties["EMin"]);
  m_properties["EMax"] = m_dblManager->addProperty("EMax");
  m_dblManager->setDecimals(m_properties["EMax"], numDecimals);
  m_propTrees["SymmPropTree"]->addProperty(m_properties["EMax"]);

  QtProperty *rawPlotProps = m_grpManager->addProperty("Raw Plot");
  m_propTrees["SymmPropTree"]->addProperty(rawPlotProps);

  m_properties["PreviewSpec"] = m_dblManager->addProperty("Spectrum No");
  m_dblManager->setDecimals(m_properties["PreviewSpec"], 0);
  rawPlotProps->addSubProperty(m_properties["PreviewSpec"]);

  // Preview Properties
  // Mainly used for display rather than getting user input
  m_properties["NegativeYValue"] = m_dblManager->addProperty("Negative Y");
  m_dblManager->setDecimals(m_properties["NegativeYValue"], numDecimals);
  m_propTrees["SymmPVPropTree"]->addProperty(m_properties["NegativeYValue"]);

  m_properties["PositiveYValue"] = m_dblManager->addProperty("Positive Y");
  m_dblManager->setDecimals(m_properties["PositiveYValue"], numDecimals);
  m_propTrees["SymmPVPropTree"]->addProperty(m_properties["PositiveYValue"]);

  m_properties["DeltaY"] = m_dblManager->addProperty("Delta Y");
  m_dblManager->setDecimals(m_properties["DeltaY"], numDecimals);
  m_propTrees["SymmPVPropTree"]->addProperty(m_properties["DeltaY"]);

  auto const xLimits = m_uiForm.ppRawPlot->getAxisRange(AxisID::XBottom);
  auto const yLimits = m_uiForm.ppRawPlot->getAxisRange(AxisID::YLeft);

  // Indicators for Y value at each EMin position
  auto negativeEMinYPos =
      m_uiForm.ppRawPlot->addSingleSelector("NegativeEMinYPos", MantidWidgets::SingleSelector::YSINGLE, 0.0);
  negativeEMinYPos->setColour(Qt::blue);
  negativeEMinYPos->setBounds(std::get<0>(yLimits), std::get<1>(yLimits));

  auto positiveEMinYPos =
      m_uiForm.ppRawPlot->addSingleSelector("PositiveEMinYPos", MantidWidgets::SingleSelector::YSINGLE, 1.0);
  positiveEMinYPos->setColour(Qt::red);
  positiveEMinYPos->setBounds(std::get<0>(yLimits), std::get<1>(yLimits));

  // Indicator for centre of symmetry (x=0)
  auto centreMarkRaw = m_uiForm.ppRawPlot->addSingleSelector("CentreMark", MantidWidgets::SingleSelector::XSINGLE, 0.0);
  centreMarkRaw->setColour(Qt::cyan);
  centreMarkRaw->setBounds(std::get<0>(xLimits), std::get<1>(xLimits));

  // Indicators for negative and positive X range values on X axis
  // The user can use these to move the X range
  // Note that the max and min of the negative range selector corespond to the
  // opposite X value
  // i.e. RS min is X max

  auto positiveERaw = m_uiForm.ppRawPlot->addRangeSelector("PositiveE");
  positiveERaw->setColour(Qt::darkMagenta);

  // SIGNAL/SLOT CONNECTIONS
  // Validate the E range when it is changed

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SIGNAL(valueChanged(QtProperty *, double)));
  // Plot miniplot when file has finished loading
  connect(m_uiForm.dsInput, SIGNAL(dataReady(QString const &)), this, SIGNAL(dataReady(QString const &)));
  // Preview symmetrise
  connect(m_uiForm.pbPreview, SIGNAL(clicked()), this, SIGNAL(previewClicked()));
  // X range selectors
  connect(positiveERaw, SIGNAL(minValueChanged(double)), this, SLOT(xRangeMinChanged(double)));
  connect(positiveERaw, SIGNAL(maxValueChanged(double)), this, SLOT(xRangeMaxChanged(double)));
  // Handle running, plotting and saving
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SIGNAL(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SIGNAL(saveClicked()));

  connect(this, SIGNAL(updateRunButton(bool, std::string const &, QString const &, QString const &)), this,
          SLOT(updateRunButton(bool, std::string const &, QString const &, QString const &)));
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
InelasticDataManipulationSymmetriseTabView::~InelasticDataManipulationSymmetriseTabView() {}

void InelasticDataManipulationSymmetriseTabView::setDefaults() {
  // Set default X range values
  m_dblManager->setValue(m_properties["EMax"], 0.5);
  m_dblManager->setValue(m_properties["EMin"], 0.1);

  // Set default x axis range
  QPair<double, double> defaultRange(-1.0, 1.0);
  m_uiForm.ppRawPlot->setAxisRange(defaultRange, AxisID::XBottom);
  m_uiForm.ppPreviewPlot->setAxisRange(defaultRange, AxisID::XBottom);

  // Disable run until preview is clicked
  m_uiForm.pbRun->setEnabled(false);
  m_uiForm.pbPreview->setEnabled(false);

  // Allows empty workspace selector when initially selected
  m_uiForm.dsInput->isOptional(true);

  // Disables searching for run files in the data archive
  m_uiForm.dsInput->isForRunFiles(false);
}

IndirectPlotOptionsView *InelasticDataManipulationSymmetriseTabView::getPlotOptions() {
  return m_uiForm.ipoPlotOptions;
}

/**
 * Verifies that the E Range is valid.
 *
 * @param prop QtProperty changed
 * @param value Value it was changed to (unused)
 */
void InelasticDataManipulationSymmetriseTabView::verifyERange(QtProperty *prop, double value) {
  UNUSED_ARG(value);

  double eMin = m_dblManager->value(m_properties["EMin"]);
  double eMax = m_dblManager->value(m_properties["EMax"]);

  if (prop == m_properties["EMin"]) {
    // If the value of EMin is negative try negating it to get a valid range
    if (eMin < 0) {
      eMin = -eMin;
      m_dblManager->setValue(m_properties["EMin"], eMin);
      return;
    }
    // If range is still invalid reset EMin to half EMax
    else if (eMin > eMax) {
      m_dblManager->setValue(m_properties["EMin"], eMax / 2);
      return;
    }
  } else if (prop == m_properties["EMax"]) {
    // If the value of EMax is negative try negating it to get a valid range
    if (eMax < 0) {
      eMax = -eMax;
      m_dblManager->setValue(m_properties["EMax"], eMax);
      return;
    }
    // If range is invalid reset EMax to double EMin
    else if (eMin > eMax) {
      m_dblManager->setValue(m_properties["EMax"], eMin * 2);
      return;
    }
  }

  // If we get this far then the E range is valid
  // Update the range selectors with the new values.
  updateRangeSelectors(prop, value);
}

/**
 * Updates position of XCut range selectors when used changed value of XCut.
 *
 * @param prop QtProperty changed
 * @param value Value it was changed to (unused)
 */
void InelasticDataManipulationSymmetriseTabView::updateRangeSelectors(QtProperty *prop, double value) {
  auto positiveERaw = m_uiForm.ppRawPlot->getRangeSelector("PositiveE");

  value = fabs(value);

  if (prop == m_properties["EMin"]) {
    positiveERaw->setMinimum(value);
  } else if (prop == m_properties["EMax"]) {
    positiveERaw->setMaximum(value);
  }
}

/**
 * Handles the X minimum value being changed from a range selector.
 *
 * @param value New range selector value
 */
void InelasticDataManipulationSymmetriseTabView::xRangeMinChanged(double value) {
  m_dblManager->setValue(m_properties["EMin"], std::abs(value));
  m_uiForm.pbPreview->setEnabled(true);
}

/**
 * Handles the X maximum value being changed from a range selector.
 *
 * @param value New range selector value
 */
void InelasticDataManipulationSymmetriseTabView::xRangeMaxChanged(double value) {
  m_dblManager->setValue(m_properties["EMax"], std::abs(value));
  m_uiForm.pbPreview->setEnabled(true);
}

void InelasticDataManipulationSymmetriseTabView::updateRunButton(bool enabled, std::string const &enableOutputButtons,
                                                                 QString const &message, QString const &tooltip) {
  setRunEnabled(enabled);
  m_uiForm.pbRun->setText(message);
  m_uiForm.pbRun->setToolTip(tooltip);
  if (enableOutputButtons != "unchanged")
    setSaveEnabled(enableOutputButtons == "enable");
}

void InelasticDataManipulationSymmetriseTabView::setRunEnabled(bool enabled) { m_uiForm.pbRun->setEnabled(enabled); }

void InelasticDataManipulationSymmetriseTabView::setSaveEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

void InelasticDataManipulationSymmetriseTabView::setFBSuffixes(QStringList const suffix) {
  m_uiForm.dsInput->setFBSuffixes(suffix);
}

void InelasticDataManipulationSymmetriseTabView::setWSSuffixes(QStringList const suffix) {
  m_uiForm.dsInput->setWSSuffixes(suffix);
}

/**
 * Plots a new workspace in the mini plot when it is loaded form the data
 *selector.
 *
 * @param workspaceName Name of the workspace that has been loaded
 */
void InelasticDataManipulationSymmetriseTabView::plotNewData(QString const &workspaceName) {
  // Set the preview spectrum number to the first spectrum in the workspace
  auto sampleWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName.toStdString());
  int minSpectrumRange = sampleWS->getSpectrum(0).getSpectrumNo();
  m_dblManager->setValue(m_properties["PreviewSpec"], static_cast<double>(minSpectrumRange));

  updateMiniPlots();

  // Set the preview range to the maximum absolute X value
  auto const axisRange = getXRangeFromWorkspace(sampleWS);
  double symmRange = std::max(fabs(axisRange.first), fabs(axisRange.second));

  // Set valid range for range selectors
  auto positiveESelector = m_uiForm.ppRawPlot->getRangeSelector("PositiveE");
  positiveESelector->setBounds(axisRange.first, axisRange.second);
  positiveESelector->setRange(0, symmRange);

  // Set some default (and valid) values for E range
  m_dblManager->setValue(m_properties["EMax"], axisRange.second);
  m_dblManager->setValue(m_properties["EMin"], axisRange.second / 10);

  updateMiniPlots();

  auto const xLimits = m_uiForm.ppRawPlot->getAxisRange(AxisID::XBottom);
  auto const yLimits = m_uiForm.ppRawPlot->getAxisRange(AxisID::YLeft);

  // Set indicator positions
  auto negativeEMinYPos = m_uiForm.ppRawPlot->getSingleSelector("NegativeEMinYPos");
  negativeEMinYPos->setBounds(std::get<0>(yLimits), std::get<1>(yLimits));

  auto positiveEMinYPos = m_uiForm.ppRawPlot->getSingleSelector("PositiveEMinYPos");
  positiveEMinYPos->setBounds(std::get<0>(yLimits), std::get<1>(yLimits));

  auto centreMarkRaw = m_uiForm.ppRawPlot->getSingleSelector("CentreMark");
  centreMarkRaw->setBounds(std::get<0>(xLimits), std::get<1>(xLimits));
}

/**
 * Updates the mini plots.
 */
void InelasticDataManipulationSymmetriseTabView::updateMiniPlots() {
  if (!m_uiForm.dsInput->isValid())
    return;

  QString workspaceName = m_uiForm.dsInput->getCurrentDataName();
  int spectrumNumber = static_cast<int>(m_dblManager->value(m_properties["PreviewSpec"]));

  Mantid::API::MatrixWorkspace_sptr input =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName.toStdString());

  // Plot the spectrum chosen by the user
  size_t spectrumIndex = input->getIndexFromSpectrumNumber(spectrumNumber);
  m_uiForm.ppRawPlot->clear();
  m_uiForm.ppRawPlot->addSpectrum("Raw", input, spectrumIndex);

  // Match X axis range on preview plot
  auto const axisRange = getXRangeFromWorkspace(input);
  m_uiForm.ppPreviewPlot->setAxisRange(axisRange, AxisID::XBottom);
  m_uiForm.ppPreviewPlot->replot();
}

/**
 * Redraws mini plots when user changes previw range or spectrum.
 *
 * @param prop QtProperty that was changed
 * @param value Value it was changed to
 */
void InelasticDataManipulationSymmetriseTabView::replotNewSpectrum(double value) {
  // Validate the preview spectra
  // Get the range of possible spectra numbers
  QString workspaceName = m_uiForm.dsInput->getCurrentDataName();
  MatrixWorkspace_sptr sampleWS =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName.toStdString());
  int minSpectrumRange = sampleWS->getSpectrum(0).getSpectrumNo();
  int maxSpectrumRange = sampleWS->getSpectrum(sampleWS->getNumberHistograms() - 1).getSpectrumNo();

  // If entered value is lower then set spectra number to lowest valid value
  if (value < minSpectrumRange) {
    m_dblManager->setValue(m_properties["PreviewSpec"], minSpectrumRange);
    return;
  }

  // If entered value is higer then set spectra number to highest valid value
  if (value > maxSpectrumRange) {
    m_dblManager->setValue(m_properties["PreviewSpec"], maxSpectrumRange);
    return;
  }
  // If we get this far then properties are valid so update mini plots
  updateMiniPlots();
}

bool InelasticDataManipulationSymmetriseTabView::validate() {
  UserInputValidator uiv;
  validateDataIsOfType(uiv, m_uiForm.dsInput, "Sample", DataType::Red);

  // EMin and EMax must be positive
  if (m_dblManager->value(m_properties["EMin"]) <= 0.0)
    uiv.addErrorMessage("EMin must be positive.");
  if (m_dblManager->value(m_properties["EMax"]) <= 0.0)
    uiv.addErrorMessage("EMax must be positive.");

  auto const errorMessage = uiv.generateErrorMessage();
  if (!errorMessage.isEmpty())
    showMessageBox(errorMessage);
  return errorMessage.isEmpty();
}

void InelasticDataManipulationSymmetriseTabView::setRawPlotWatchADS(bool watchADS) {
  m_uiForm.ppRawPlot->watchADS(watchADS);
}

double InelasticDataManipulationSymmetriseTabView::getEMin() { return m_dblManager->value(m_properties["EMin"]); }

double InelasticDataManipulationSymmetriseTabView::getEMax() { return m_dblManager->value(m_properties["EMax"]); }

double InelasticDataManipulationSymmetriseTabView::getPreviewSpec() {
  return m_dblManager->value(m_properties["PreviewSpec"]);
}

QString InelasticDataManipulationSymmetriseTabView::getInputName() { return m_uiForm.dsInput->getCurrentDataName(); }

void InelasticDataManipulationSymmetriseTabView::previewAlgDone() {
  QString workspaceName = getInputName();
  int spectrumNumber = static_cast<int>(m_dblManager->value(m_properties["PreviewSpec"]));

  MatrixWorkspace_sptr sampleWS =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName.toStdString());
  ITableWorkspace_sptr propsTable =
      AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("__SymmetriseProps_temp");
  MatrixWorkspace_sptr symmWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("__Symmetrise_temp");

  // Get the index of XCut on each side of zero
  int negativeIndex = propsTable->getColumn("NegativeXMinIndex")->cell<int>(0);
  int positiveIndex = propsTable->getColumn("PositiveXMinIndex")->cell<int>(0);

  // Get the Y values for each XCut and the difference between them
  double negativeY = sampleWS->y(0)[negativeIndex];
  double positiveY = sampleWS->y(0)[positiveIndex];
  double deltaY = fabs(negativeY - positiveY);

  // Show values in property tree
  m_dblManager->setValue(m_properties["NegativeYValue"], negativeY);
  m_dblManager->setValue(m_properties["PositiveYValue"], positiveY);
  m_dblManager->setValue(m_properties["DeltaY"], deltaY);

  auto const yLimits = m_uiForm.ppRawPlot->getAxisRange(AxisID::YLeft);
  // Set indicator positions
  auto const negativeEMinYPos = m_uiForm.ppRawPlot->getSingleSelector("NegativeEMinYPos");
  negativeEMinYPos->setBounds(std::get<0>(yLimits), std::get<1>(yLimits));
  negativeEMinYPos->setPosition(negativeY);

  auto const positiveEMinYPos = m_uiForm.ppRawPlot->getSingleSelector("PositiveEMinYPos");
  positiveEMinYPos->setBounds(std::get<0>(yLimits), std::get<1>(yLimits));
  positiveEMinYPos->setPosition(positiveY);

  // Plot preview plot
  size_t spectrumIndex = symmWS->getIndexFromSpectrumNumber(spectrumNumber);
  m_uiForm.ppPreviewPlot->clear();
  m_uiForm.ppPreviewPlot->addSpectrum("Symmetrised", "__Symmetrise_temp", spectrumIndex);

  m_uiForm.ppRawPlot->watchADS(true);
}

void InelasticDataManipulationSymmetriseTabView::enableSave(bool save) { m_uiForm.pbSave->setEnabled(save); }

void InelasticDataManipulationSymmetriseTabView::enableRun(bool run) { m_uiForm.pbRun->setEnabled(run); }

} // namespace MantidQt::CustomInterfaces