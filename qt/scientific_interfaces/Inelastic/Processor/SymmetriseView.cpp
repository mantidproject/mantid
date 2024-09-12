// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "SymmetriseView.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "SymmetrisePresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Plotting/SingleSelector.h"

using namespace DataValidationHelper;
using namespace Mantid::API;

using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using namespace MantidQt::CustomInterfaces::InterfaceUtils;

constexpr auto NUMERICAL_PRECISION = 2;

using MantidQt::MantidWidgets::AxisID;
namespace MantidQt::CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SymmetriseView::SymmetriseView(QWidget *parent) : QWidget(parent), m_presenter() {
  m_uiForm.setupUi(parent);

  m_dblManager = new QtDoublePropertyManager();
  m_grpManager = new QtGroupPropertyManager();
  m_enumManager = new QtEnumPropertyManager(); // "Suggestion"

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
  auto *enumEditorFactory = new QtEnumEditorFactory();
  m_propTrees["SymmPropTree"]->setFactoryForManager(m_enumManager, enumEditorFactory);

  // Raw Properties
  m_properties["Elow"] = m_dblManager->addProperty("Elow");
  m_dblManager->setDecimals(m_properties["Elow"], numDecimals);
  m_propTrees["SymmPropTree"]->addProperty(m_properties["Elow"]);
  m_properties["Ehigh"] = m_dblManager->addProperty("Ehigh");
  m_dblManager->setDecimals(m_properties["Ehigh"], numDecimals);
  m_propTrees["SymmPropTree"]->addProperty(m_properties["Ehigh"]);

  QtProperty *rawPlotProps = m_grpManager->addProperty("Raw Plot");
  m_propTrees["SymmPropTree"]->addProperty(rawPlotProps);

  m_properties["PreviewSpec"] = m_dblManager->addProperty("Spectrum No");
  m_dblManager->setDecimals(m_properties["PreviewSpec"], 0);
  rawPlotProps->addSubProperty(m_properties["PreviewSpec"]);

  m_properties["ReflectType"] = m_enumManager->addProperty("ReflectType");
  QStringList types;
  types << "Positive to Negative"
        << "Negative to Positive";
  m_enumManager->setEnumNames(m_properties["ReflectType"], types);
  m_enumManager->setValue(m_properties["ReflectType"], 0);
  m_propTrees["SymmPropTree"]->addProperty(m_properties["ReflectType"]);

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

  // Indicator for centre of symmetry (x=0)
  auto centreMarkRaw = m_uiForm.ppRawPlot->addSingleSelector("CentreMark", MantidWidgets::SingleSelector::XSINGLE, 0.0,
                                                             MantidWidgets::PlotLineStyle::Solid);
  centreMarkRaw->setColour(Qt::red);
  centreMarkRaw->disconnectMouseSignals();

  // Horizontal Marker Lines
  auto horzMarkFirst = m_uiForm.ppRawPlot->addSingleSelector("horzMarkFirst", MantidWidgets::SingleSelector::YSINGLE,
                                                             0.1, MantidWidgets::PlotLineStyle::Dotted);
  horzMarkFirst->setColour(Qt::blue);
  auto horzMarkSecond = m_uiForm.ppRawPlot->addSingleSelector("horzMarkSecond", MantidWidgets::SingleSelector::YSINGLE,
                                                              0.5, MantidWidgets::PlotLineStyle::Dotted);
  horzMarkSecond->setColour(Qt::darkBlue);

  // Indicators for negative and positive X range values on X axis
  // The user can use these to move the X range
  // Note that the max and min of the negative range selector correspond to the
  // opposite X value
  // i.e. RS min is X max
  auto const xLimits = m_uiForm.ppRawPlot->getAxisRange(AxisID::XBottom);
  auto rangeESelector = m_uiForm.ppRawPlot->addRangeSelector("rangeE");
  rangeESelector->setColour(Qt::darkGreen);
  rangeESelector->setBounds(0.0, std::get<1>(xLimits));

  // SIGNAL/SLOT CONNECTIONS
  // Validate the E range when it is changed
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(notifyDoubleValueChanged(QtProperty *, double)));
  connect(m_enumManager, SIGNAL(valueChanged(QtProperty *, int)), this,
          SLOT(notifyReflectTypeChanged(QtProperty *, int)));
  // Plot miniplot when file has finished loading
  connect(m_uiForm.dsInput, SIGNAL(dataReady(QString const &)), this, SLOT(notifyDataReady(QString const &)));
  // Preview symmetrise
  connect(m_uiForm.pbPreview, SIGNAL(clicked()), this, SLOT(notifyPreviewClicked()));
  // X range selectors
  connect(rangeESelector, SIGNAL(minValueChanged(double)), this, SLOT(notifyXrangeLowChanged(double)));
  connect(rangeESelector, SIGNAL(maxValueChanged(double)), this, SLOT(notifyXrangeHighChanged(double)));
  // Handle running, plotting and saving
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(notifySaveClicked()));
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SymmetriseView::~SymmetriseView() {}

void SymmetriseView::subscribePresenter(ISymmetrisePresenter *presenter) { m_presenter = presenter; }

void SymmetriseView::setDefaults() {
  // Set default X range values
  m_dblManager->setValue(m_properties["Ehigh"], 0.5);
  m_dblManager->setValue(m_properties["Elow"], 0.1);
  auto rangeESelector = m_uiForm.ppRawPlot->getRangeSelector("rangeE");
  rangeESelector->setRange(0.1, 0.5);

  // Set default reflection type
  m_enumManager->setValue(m_properties["ReflectType"], 0);

  // Set default x axis range
  QPair<double, double> defaultRange(-1.0, 1.0);
  m_uiForm.ppRawPlot->setAxisRange(defaultRange, AxisID::XBottom);
  m_uiForm.ppPreviewPlot->setAxisRange(defaultRange, AxisID::XBottom);

  // Disable run until preview is clicked
  m_uiForm.pbPreview->setEnabled(false);

  // Allows empty workspace selector when initially selected
  m_uiForm.dsInput->isOptional(true);

  // Disables searching for run files in the data archive
  m_uiForm.dsInput->isForRunFiles(false);
}

MantidWidgets::DataSelector *SymmetriseView::getDataSelector() const { return m_uiForm.dsInput; }

IRunView *SymmetriseView::getRunView() const { return m_uiForm.runWidget; }

IOutputPlotOptionsView *SymmetriseView::getPlotOptions() const { return m_uiForm.ipoPlotOptions; }

void SymmetriseView::notifyDoubleValueChanged(QtProperty *prop, double value) {
  m_presenter->handleDoubleValueChanged(prop->propertyName().toStdString(), value);
}

void SymmetriseView::notifyDataReady(QString const &dataName) { m_presenter->handleDataReady(dataName.toStdString()); }

void SymmetriseView::notifyPreviewClicked() { m_presenter->handlePreviewClicked(); }

void SymmetriseView::notifySaveClicked() { m_presenter->handleSaveClicked(); }
/**
 * Handles the X minimum value being changed from a range selector.
 *
 * @param value New range selector value
 */
void SymmetriseView::notifyXrangeLowChanged(double value) {
  m_dblManager->setValue(m_properties["Elow"], value);
  m_dblManager->setMinimum(m_properties["Ehigh"], value);
}

/**
 * Handles the X maximum value being changed from a range selector.
 *
 * @param value New range selector value
 */
void SymmetriseView::notifyXrangeHighChanged(double value) {
  m_dblManager->setValue(m_properties["Ehigh"], value);
  m_dblManager->setMaximum(m_properties["Elow"], value);
}

void SymmetriseView::notifyReflectTypeChanged(QtProperty *prop, int value) {
  if (prop->propertyName() == "ReflectType")
    m_presenter->handleReflectTypeChanged(value);
}

void SymmetriseView::resetEDefaults(bool isPositive) {
  MatrixWorkspace_sptr sampleWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(getDataName());
  auto const axisRange = getXRangeFromWorkspace(sampleWS);
  resetEDefaults(isPositive, axisRange);
}

/**
 * Updates boundaries and initial values for Selector and Data properties when changing between negative/positive side
 * of spectrum.
 *
 * @param isPositive flag for spectrum positive side
 * @param range Active spectra range
 *
 */
void SymmetriseView::resetEDefaults(bool isPositive, QPair<double, double> range) {
  auto rangeESelector = m_uiForm.ppRawPlot->getRangeSelector("rangeE");

  // Set Selector range boundaries
  auto rangeReflect = isPositive ? QPair(0.0, range.second) : QPair(range.first, 0.0);
  rangeESelector->setBounds(rangeReflect.first, rangeReflect.second);
  m_dblManager->setRange(m_properties["Ehigh"], rangeReflect.first, rangeReflect.second);
  m_dblManager->setRange(m_properties["Elow"], rangeReflect.first, rangeReflect.second);

  // Set Initial selector range values
  auto rangeInitial =
      isPositive ? QPair(0.1 * range.second, 0.9 * range.second) : QPair(0.9 * range.first, 0.1 * range.first);
  rangeESelector->setRange(rangeInitial.first, rangeInitial.second);
  m_dblManager->setValue(m_properties["Ehigh"], rangeInitial.second);
  m_dblManager->setValue(m_properties["Elow"], rangeInitial.first);
}

/**
 * Verifies that the E Range is valid. Logs message guiding user on what's wrong with selection.
 *
 * @param QString with current workspace name.
 *
 * @return true if selected E range is valid for calling the symmetrise algorithm, false otherwise.
 */
bool SymmetriseView::verifyERange(std::string const &workspaceName) {
  MatrixWorkspace_sptr sampleWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName);
  auto axisRange = getXRangeFromWorkspace(sampleWS);
  auto Erange = QPair(getElow(), getEhigh());

  bool const reflectType = m_enumManager->value(m_properties["ReflectType"]);
  if ((reflectType == 0) && (Erange.first > abs(axisRange.first))) {
    showMessageBox("Invalid Data Range: Elow is larger than the lower limit of spectrum.\nReduce Elow to " +
                   makeQStringNumber(abs(axisRange.first), NUMERICAL_PRECISION).toStdString());
    return false;
  } else if ((reflectType == 1) && (abs(Erange.second) > axisRange.second)) {
    showMessageBox("Invalid Data Range: Ehigh is larger than the upper limit of spectrum.\nIncrease Ehigh to " +
                   makeQStringNumber(axisRange.second, NUMERICAL_PRECISION).toStdString());
    return false;
  }
  return true;
}

/**
 * Updates position of XCut range selectors when user changed value of XCut.
 *
 * @param prop QtProperty changed
 * @param value Value it was changed to (unused)
 */
void SymmetriseView::updateRangeSelectors(std::string const &propName, double value) {
  auto rangeESelector = m_uiForm.ppRawPlot->getRangeSelector("rangeE");

  if (propName == "Elow") {
    rangeESelector->setMinimum(value);
  } else if (propName == "Ehigh") {
    rangeESelector->setMaximum(value);
  }
}

void SymmetriseView::setFBSuffixes(QStringList const &suffix) { m_uiForm.dsInput->setFBSuffixes(suffix); }

void SymmetriseView::setWSSuffixes(QStringList const &suffix) { m_uiForm.dsInput->setWSSuffixes(suffix); }

void SymmetriseView::setLoadHistory(bool doLoadHistory) {
  m_uiForm.dsInput->setLoadProperty("LoadHistory", doLoadHistory);
}

/**
 * Plots a new workspace in the mini plot when it is loaded form the data
 *selector.
 *
 * @param workspaceName Name of the workspace that has been loaded
 */
void SymmetriseView::plotNewData(std::string const &workspaceName) {
  // Set the preview spectrum number to the first spectrum in the workspace
  auto sampleWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName);
  int minSpectrumRange = sampleWS->getSpectrum(0).getSpectrumNo();
  m_dblManager->setValue(m_properties["PreviewSpec"], static_cast<double>(minSpectrumRange));

  // Set the preview range to the maximum absolute X value
  auto const axisRange = getXRangeFromWorkspace(sampleWS);

  // Set some default (and valid) values for E range
  resetEDefaults(m_enumManager->value(m_properties["ReflectType"]) == 0, axisRange);
  updateMiniPlots();

  m_uiForm.pbPreview->setEnabled(true);
}

/**
 * Updates the mini plots.
 */
void SymmetriseView::updateMiniPlots() {
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

  // Update bounds for horizontal markers
  auto verticalRange = m_uiForm.ppRawPlot->getAxisRange(AxisID::YLeft);
  updateHorizontalMarkers(convertTupleToQPair(verticalRange));
}

/**
 * Updates limits for horizontal markers when user loads new spectra
 *
 * @param Y range for current workspace
 *
 */
void SymmetriseView::updateHorizontalMarkers(QPair<double, double> yrange) {
  auto horzMarkFirst = m_uiForm.ppRawPlot->getSingleSelector("horzMarkFirst");
  auto horzMarkSecond = m_uiForm.ppRawPlot->getSingleSelector("horzMarkSecond");

  horzMarkFirst->setBounds(yrange.first, yrange.second);
  horzMarkSecond->setBounds(yrange.first, yrange.second);

  double windowRange = yrange.second - yrange.first;
  double markerSeparation = 0.1;
  horzMarkFirst->setPosition(yrange.first + windowRange * markerSeparation);
  horzMarkSecond->setPosition(yrange.second - windowRange * markerSeparation);
}

/**
 * Redraws mini plots when user changes preview range or spectrum.
 *
 * @param prop QtProperty that was changed
 * @param value Value it was changed to
 */
void SymmetriseView::replotNewSpectrum(double value) {
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

  // If entered value is higher then set spectra number to highest valid value
  if (value > maxSpectrumRange) {
    m_dblManager->setValue(m_properties["PreviewSpec"], maxSpectrumRange);
    return;
  }
  // If we get this far then properties are valid so update mini plots
  updateMiniPlots();
}

void SymmetriseView::setRawPlotWatchADS(bool watchADS) { m_uiForm.ppRawPlot->watchADS(watchADS); }

double SymmetriseView::getElow() const { return m_dblManager->value(m_properties["Elow"]); }

double SymmetriseView::getEhigh() const { return m_dblManager->value(m_properties["Ehigh"]); }

double SymmetriseView::getPreviewSpec() { return m_dblManager->value(m_properties["PreviewSpec"]); }

std::string SymmetriseView::getDataName() const { return m_uiForm.dsInput->getCurrentDataName().toStdString(); }

void SymmetriseView::previewAlgDone() {
  QString workspaceName = QString::fromStdString(getDataName());
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

  // Plot preview plot
  size_t spectrumIndex = symmWS->getIndexFromSpectrumNumber(spectrumNumber);
  m_uiForm.ppPreviewPlot->clear();
  m_uiForm.ppPreviewPlot->addSpectrum("Symmetrised", "__Symmetrise_temp", spectrumIndex);

  m_uiForm.ppRawPlot->watchADS(true);
}

void SymmetriseView::enableSave(bool save) { m_uiForm.pbSave->setEnabled(save); }

void SymmetriseView::showMessageBox(std::string const &message) const {
  QMessageBox::information(parentWidget(), this->windowTitle(), QString::fromStdString(message));
}

} // namespace MantidQt::CustomInterfaces