// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSymmetrise.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"

namespace {
Mantid::Kernel::Logger g_log("IndirectSymmetrise");
}

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
//----------------------------------------------------------------------------------------------
/** Constructor
 */
IndirectSymmetrise::IndirectSymmetrise(IndirectDataReduction *idrUI,
                                       QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent) {
  m_uiForm.setupUi(parent);

  int numDecimals = 6;

  // Property Trees
  m_propTrees["SymmPropTree"] = new QtTreePropertyBrowser();
  m_uiForm.properties->addWidget(m_propTrees["SymmPropTree"]);

  m_propTrees["SymmPVPropTree"] = new QtTreePropertyBrowser();
  m_uiForm.propertiesPreview->addWidget(m_propTrees["SymmPVPropTree"]);

  // Editor Factories
  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory();
  m_propTrees["SymmPropTree"]->setFactoryForManager(m_dblManager,
                                                    doubleEditorFactory);

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

  // Indicators for Y value at each EMin position
  auto negativeEMinYPos = m_uiForm.ppRawPlot->addRangeSelector(
      "NegativeEMinYPos", MantidWidgets::RangeSelector::YSINGLE);
  negativeEMinYPos->setInfoOnly(true);
  negativeEMinYPos->setColour(Qt::blue);
  negativeEMinYPos->setMinimum(0.0);

  auto positiveEMinYPos = m_uiForm.ppRawPlot->addRangeSelector(
      "PositiveEMinYPos", MantidWidgets::RangeSelector::YSINGLE);
  positiveEMinYPos->setInfoOnly(true);
  positiveEMinYPos->setColour(Qt::red);
  positiveEMinYPos->setMinimum(0.0);

  // Indicator for centre of symmetry (x=0)
  auto centreMarkRaw = m_uiForm.ppRawPlot->addRangeSelector(
      "CentreMark", MantidWidgets::RangeSelector::XSINGLE);
  centreMarkRaw->setInfoOnly(true);
  centreMarkRaw->setColour(Qt::cyan);
  centreMarkRaw->setMinimum(0.0);

  // Indicators for negative and positive X range values on X axis
  // The user can use these to move the X range
  // Note that the max and min of the negative range selector corespond to the
  // opposite X value
  // i.e. RS min is X max
  auto negativeERaw = m_uiForm.ppRawPlot->addRangeSelector("NegativeE");
  negativeERaw->setColour(Qt::darkGreen);

  auto positiveERaw = m_uiForm.ppRawPlot->addRangeSelector("PositiveE");
  positiveERaw->setColour(Qt::darkGreen);

  // Indicators for negative and positive X range values on X axis
  auto negativeEPV = m_uiForm.ppPreviewPlot->addRangeSelector("NegativeE");
  negativeEPV->setInfoOnly(true);
  negativeEPV->setColour(Qt::darkGreen);

  auto positiveEPV = m_uiForm.ppPreviewPlot->addRangeSelector("PositiveE");
  positiveEPV->setInfoOnly(true);
  positiveEPV->setColour(Qt::darkGreen);

  // Indicator for centre of symmetry (x=0)
  auto centreMarkPV = m_uiForm.ppPreviewPlot->addRangeSelector(
      "CentreMark", MantidWidgets::RangeSelector::XSINGLE);
  centreMarkPV->setInfoOnly(true);
  centreMarkPV->setColour(Qt::cyan);
  centreMarkPV->setMinimum(0.0);

  // SIGNAL/SLOT CONNECTIONS
  // Validate the E range when it is changed
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(verifyERange(QtProperty *, double)));
  // Plot a new spectrum when the user changes the value of the preview spectrum
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(replotNewSpectrum(QtProperty *, double)));
  // Plot miniplot when file has finished loading
  connect(m_uiForm.dsInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(plotRawInput(const QString &)));
  // Preview symmetrise
  connect(m_uiForm.pbPreview, SIGNAL(clicked()), this, SLOT(preview()));
  // X range selectors
  connect(positiveERaw, SIGNAL(minValueChanged(double)), this,
          SLOT(xRangeMinChanged(double)));
  connect(positiveERaw, SIGNAL(maxValueChanged(double)), this,
          SLOT(xRangeMaxChanged(double)));
  connect(negativeERaw, SIGNAL(minValueChanged(double)), this,
          SLOT(xRangeMinChanged(double)));
  connect(negativeERaw, SIGNAL(maxValueChanged(double)), this,
          SLOT(xRangeMaxChanged(double)));
  // Handle plotting and saving
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));

  // Set default X range values
  m_dblManager->setValue(m_properties["EMin"], 0.1);
  m_dblManager->setValue(m_properties["EMax"], 0.5);

  // Set default x axis range
  QPair<double, double> defaultRange(-1.0, 1.0);
  m_uiForm.ppRawPlot->setAxisRange(defaultRange, QwtPlot::xBottom);
  m_uiForm.ppPreviewPlot->setAxisRange(defaultRange, QwtPlot::xBottom);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IndirectSymmetrise::~IndirectSymmetrise() {}

void IndirectSymmetrise::setup() {}

bool IndirectSymmetrise::validate() {
  // Check for a valid input file
  if (!m_uiForm.dsInput->isValid())
    return false;

  // EMin and EMax must be positive
  if (m_dblManager->value(m_properties["EMin"]) <= 0.0)
    return false;
  if (m_dblManager->value(m_properties["EMax"]) <= 0.0)
    return false;

  return true;
}

void IndirectSymmetrise::run() {
  QString workspaceName = m_uiForm.dsInput->getCurrentDataName();
  QString outputWorkspaceName = workspaceName.left(workspaceName.length() - 4) +
                                "_sym" + workspaceName.right(4);

  double e_min = m_dblManager->value(m_properties["EMin"]);
  double e_max = m_dblManager->value(m_properties["EMax"]);

  IAlgorithm_sptr symmetriseAlg =
      AlgorithmManager::Instance().create("Symmetrise", -1);
  symmetriseAlg->initialize();
  symmetriseAlg->setProperty("InputWorkspace", workspaceName.toStdString());
  symmetriseAlg->setProperty("XMin", e_min);
  symmetriseAlg->setProperty("XMax", e_max);
  symmetriseAlg->setProperty("OutputWorkspace",
                             outputWorkspaceName.toStdString());
  symmetriseAlg->setProperty("OutputPropertiesTable", "__SymmetriseProps_temp");

  m_batchAlgoRunner->addAlgorithm(symmetriseAlg);

  // Set the workspace name for Python script export
  m_pythonExportWsName = outputWorkspaceName.toStdString();

  // Handle algorithm completion signal
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));

  // Execute algorithm on seperate thread
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handle plotting result workspace.
 *
 * @param error If the algorithm failed
 */
void IndirectSymmetrise::algorithmComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(algorithmComplete(bool)));

  if (error)
    return;

  // Enable save and plot
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);
}

/**
 * Plots a new workspace in the mini plot when it is loaded form the data
 *selector.
 *
 * @param workspaceName Name of the workspace that has been laoded
 */
void IndirectSymmetrise::plotRawInput(const QString &workspaceName) {
  // Set the preview spectrum number to the first spectrum in the workspace
  MatrixWorkspace_sptr sampleWS =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          workspaceName.toStdString());
  int minSpectrumRange = sampleWS->getSpectrum(0).getSpectrumNo();
  m_dblManager->setValue(m_properties["PreviewSpec"],
                         static_cast<double>(minSpectrumRange));

  updateMiniPlots();

  // Set the preview range to the maximum absolute X value
  QPair<double, double> axisRange = m_uiForm.ppRawPlot->getCurveRange("Raw");
  double symmRange = std::max(fabs(axisRange.first), fabs(axisRange.second));

  // Set valid range for range selectors
  m_uiForm.ppRawPlot->getRangeSelector("NegativeE")->setRange(-symmRange, 0);
  m_uiForm.ppRawPlot->getRangeSelector("PositiveE")->setRange(0, symmRange);

  // Set some default (and valid) values for E range
  m_dblManager->setValue(m_properties["EMax"], axisRange.second);
  m_dblManager->setValue(m_properties["EMin"], axisRange.second / 10);

  updateMiniPlots();
}

/**
 * Updates the mini plots.
 */
void IndirectSymmetrise::updateMiniPlots() {
  if (!m_uiForm.dsInput->isValid())
    return;

  QString workspaceName = m_uiForm.dsInput->getCurrentDataName();
  int spectrumNumber =
      static_cast<int>(m_dblManager->value(m_properties["PreviewSpec"]));

  Mantid::API::MatrixWorkspace_sptr input =
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(
              workspaceName.toStdString()));

  // Plot the spectrum chosen by the user
  size_t spectrumIndex = input->getIndexFromSpectrumNumber(spectrumNumber);
  m_uiForm.ppRawPlot->clear();
  m_uiForm.ppRawPlot->addSpectrum("Raw", input, spectrumIndex);

  // Match X axis range on preview plot
  m_uiForm.ppPreviewPlot->setAxisRange(m_uiForm.ppRawPlot->getCurveRange("Raw"),
                                       QwtPlot::xBottom);
  m_uiForm.ppPreviewPlot->replot();
}

/**
 * Redraws mini plots when user changes previw range or spectrum.
 *
 * @param prop QtProperty that was changed
 * @param value Value it was changed to
 */
void IndirectSymmetrise::replotNewSpectrum(QtProperty *prop, double value) {
  // Validate the preview spectra
  if (prop == m_properties["PreviewSpec"]) {
    // Get the range of possible spectra numbers
    QString workspaceName = m_uiForm.dsInput->getCurrentDataName();
    MatrixWorkspace_sptr sampleWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            workspaceName.toStdString());
    int minSpectrumRange = sampleWS->getSpectrum(0).getSpectrumNo();
    int maxSpectrumRange =
        sampleWS->getSpectrum(sampleWS->getNumberHistograms() - 1)
            .getSpectrumNo();

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
  }

  // If we get this far then properties are valid so update mini plots
  if (prop == m_properties["PreviewSpec"])
    updateMiniPlots();
}

/**
 * Verifies that the E Range is valid.
 *
 * @param prop QtProperty changed
 * @param value Value it was changed to (unused)
 */
void IndirectSymmetrise::verifyERange(QtProperty *prop, double value) {
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
    if (eMin > eMax) {
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
    if (eMin > eMax) {
      m_dblManager->setValue(m_properties["EMax"], eMin * 2);
      return;
    }
  }

  // If we get this far then the E range is valid
  // Update the range selectors with the new values.
  updateRangeSelectors(prop, value);
}

/**
 * Handles a request to preview the symmetrise.
 *
 * Runs Symmetrise on the current spectrum and plots in preview mini plot.
 *
 * @see IndirectSymmetrise::previewAlgDone()
 */
void IndirectSymmetrise::preview() {
  // Handle algorithm completion signal
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(previewAlgDone(bool)));

  // Do nothing if no data has been laoded
  QString workspaceName = m_uiForm.dsInput->getCurrentDataName();
  if (workspaceName.isEmpty())
    return;

  double e_min = m_dblManager->value(m_properties["EMin"]);
  double e_max = m_dblManager->value(m_properties["EMax"]);
  long spectrumNumber =
      static_cast<long>(m_dblManager->value(m_properties["PreviewSpec"]));
  std::vector<long> spectraRange(2, spectrumNumber);

  // Run the algorithm on the preview spectrum only
  IAlgorithm_sptr symmetriseAlg =
      AlgorithmManager::Instance().create("Symmetrise", -1);
  symmetriseAlg->initialize();
  symmetriseAlg->setProperty("InputWorkspace", workspaceName.toStdString());
  symmetriseAlg->setProperty("XMin", e_min);
  symmetriseAlg->setProperty("XMax", e_max);
  symmetriseAlg->setProperty("SpectraRange", spectraRange);
  symmetriseAlg->setProperty("OutputWorkspace", "__Symmetrise_temp");
  symmetriseAlg->setProperty("OutputPropertiesTable", "__SymmetriseProps_temp");

  runAlgorithm(symmetriseAlg);
}

/**
 * Handles completion of the preview algorithm.
 *
 * @param error If the algorithm failed
 */
void IndirectSymmetrise::previewAlgDone(bool error) {
  if (error)
    return;

  QString workspaceName = m_uiForm.dsInput->getCurrentDataName();
  int spectrumNumber =
      static_cast<int>(m_dblManager->value(m_properties["PreviewSpec"]));

  MatrixWorkspace_sptr sampleWS =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          workspaceName.toStdString());
  ITableWorkspace_sptr propsTable =
      AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
          "__SymmetriseProps_temp");
  MatrixWorkspace_sptr symmWS =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          "__Symmetrise_temp");

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

  // Set indicator positions
  m_uiForm.ppRawPlot->getRangeSelector("NegativeEMinYPos")
      ->setMinimum(negativeY);
  m_uiForm.ppRawPlot->getRangeSelector("PositiveEMinYPos")
      ->setMinimum(positiveY);

  // Plot preview plot
  size_t spectrumIndex = symmWS->getIndexFromSpectrumNumber(spectrumNumber);
  m_uiForm.ppPreviewPlot->clear();
  m_uiForm.ppPreviewPlot->addSpectrum("Symmetrised", "__Symmetrise_temp",
                                      spectrumIndex);

  // Don't want this to trigger when the algorithm is run for all spectra
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(previewAlgDone(bool)));
}

/**
 * Updates position of XCut range selectors when used changed value of XCut.
 *
 * @param prop QtProperty changed
 * @param value Value it was changed to (unused)
 */
void IndirectSymmetrise::updateRangeSelectors(QtProperty *prop, double value) {
  auto negativeERaw = m_uiForm.ppRawPlot->getRangeSelector("NegativeE");
  auto positiveERaw = m_uiForm.ppRawPlot->getRangeSelector("PositiveE");
  auto negativeEPV = m_uiForm.ppPreviewPlot->getRangeSelector("NegativeE");
  auto positiveEPV = m_uiForm.ppPreviewPlot->getRangeSelector("PositiveE");

  value = fabs(value);

  if (prop == m_properties["EMin"]) {
    negativeERaw->setMaximum(-value);
    positiveERaw->setMinimum(value);

    negativeEPV->setMinimum(-value);
    positiveEPV->setMinimum(value);
  }

  if (prop == m_properties["EMax"]) {
    negativeERaw->setMinimum(-value);
    positiveERaw->setMaximum(value);

    negativeEPV->setMaximum(-value);
    positiveEPV->setMaximum(value);
  }
}

/**
 * Handles the X minimum value being changed from a range selector.
 *
 * @param value New range selector value
 */
void IndirectSymmetrise::xRangeMinChanged(double value) {
  auto negativeERaw = m_uiForm.ppRawPlot->getRangeSelector("NegativeE");
  auto positiveERaw = m_uiForm.ppRawPlot->getRangeSelector("PositiveE");

  MantidWidgets::RangeSelector *from =
      qobject_cast<MantidWidgets::RangeSelector *>(sender());

  if (from == positiveERaw) {
    m_dblManager->setValue(m_properties["EMin"], std::abs(value));
  } else if (from == negativeERaw) {
    m_dblManager->setValue(m_properties["EMax"], std::abs(value));
  }
}

/**
 * Handles the X maximum value being changed from a range selector.
 *
 * @param value New range selector value
 */
void IndirectSymmetrise::xRangeMaxChanged(double value) {
  auto negativeERaw = m_uiForm.ppRawPlot->getRangeSelector("NegativeE");
  auto positiveERaw = m_uiForm.ppRawPlot->getRangeSelector("PositiveE");

  MantidWidgets::RangeSelector *from =
      qobject_cast<MantidWidgets::RangeSelector *>(sender());

  if (from == positiveERaw) {
    m_dblManager->setValue(m_properties["EMax"], std::abs(value));
  } else if (from == negativeERaw) {
    m_dblManager->setValue(m_properties["EMin"], std::abs(value));
  }
}
/**
 * Handles mantid plotting
 */
void IndirectSymmetrise::plotClicked() {

  QStringList workspaces;
  workspaces.append(m_uiForm.dsInput->getCurrentDataName());
  workspaces.append(QString::fromStdString(m_pythonExportWsName));
  plotSpectrum(workspaces);
}

/**
 * Handles saving of workspace
 */
void IndirectSymmetrise::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    plotSpectrum(QString::fromStdString(m_pythonExportWsName));
}
} // namespace CustomInterfaces
} // namespace MantidQt
