// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationIqtTabView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include <QFileInfo>

#include <algorithm>

#include "IndirectAddWorkspaceDialog.h"

using namespace Mantid::API;
using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;

namespace {

/**
 * Calculate the number of bins in the sample & resolution workspaces
 * @param wsName The sample workspace name
 * @param resName the resolution woskapce name
 * @param energyMin Minimum energy for chosen bin range
 * @param energyMax Maximum energy for chosen bin range
 * @param binReductionFactor The factor by which to reduce the number of bins
 * @return A 4-tuple where the first entry denotes whether the
 * calculation was successful or not. The final 3 values
 * are EWidth, SampleBins, ResolutionBins if the calculation succeeded,
 * otherwise they are undefined.
 */
std::tuple<bool, float, int, int> calculateBinParameters(std::string const &wsName, std::string const &resName,
                                                         double energyMin, double energyMax,
                                                         double binReductionFactor) {
  ITableWorkspace_sptr propsTable;
  try {
    const auto paramTableName = "__IqtProperties_temp";
    auto toIqt = AlgorithmManager::Instance().createUnmanaged("TransformToIqt");
    toIqt->initialize();
    toIqt->setChild(true); // record this as internal
    toIqt->setProperty("SampleWorkspace", wsName);
    toIqt->setProperty("ResolutionWorkspace", resName);
    toIqt->setProperty("ParameterWorkspace", paramTableName);
    toIqt->setProperty("EnergyMin", energyMin);
    toIqt->setProperty("EnergyMax", energyMax);
    toIqt->setProperty("BinReductionFactor", binReductionFactor);
    toIqt->setProperty("DryRun", true);
    toIqt->execute();
    propsTable = toIqt->getProperty("ParameterWorkspace");
    // the algorithm can create output even if it failed...
    auto deleter = AlgorithmManager::Instance().create("DeleteWorkspace");
    deleter->initialize();
    deleter->setChild(true);
    deleter->setProperty("Workspace", paramTableName);
    deleter->execute();
  } catch (std::exception &) {
    return std::make_tuple(false, 0.0f, 0, 0);
  }
  assert(propsTable);
  return std::make_tuple(true, propsTable->getColumn("EnergyWidth")->cell<float>(0),
                         propsTable->getColumn("SampleOutputBins")->cell<int>(0),
                         propsTable->getColumn("ResolutionBins")->cell<int>(0));
}
} // namespace

namespace MantidQt::CustomInterfaces {
using namespace IDA;
InelasticDataManipulationIqtTabView::InelasticDataManipulationIqtTabView(QWidget *parent) : m_iqtTree(nullptr) {
  m_uiForm.setupUi(parent);
  m_dblEdFac = new DoubleEditorFactory(this);
  m_dblManager = new QtDoublePropertyManager();
  setup();
}

InelasticDataManipulationIqtTabView::~InelasticDataManipulationIqtTabView() {
  m_iqtTree->unsetFactoryForManager(m_dblManager);
}

IndirectPlotOptionsView *InelasticDataManipulationIqtTabView::getPlotOptions() { return m_uiForm.ipoPlotOptions; }

void InelasticDataManipulationIqtTabView::setup() {
  m_iqtTree = new QtTreePropertyBrowser();
  m_uiForm.properties->addWidget(m_iqtTree);

  // Number of decimal places in property browsers.
  static const unsigned int NUM_DECIMALS = 6;
  // Create and configure properties
  m_properties["ELow"] = m_dblManager->addProperty("ELow");
  m_dblManager->setDecimals(m_properties["ELow"], NUM_DECIMALS);

  m_properties["EWidth"] = m_dblManager->addProperty("EWidth");
  m_dblManager->setDecimals(m_properties["EWidth"], NUM_DECIMALS);
  m_properties["EWidth"]->setEnabled(false);

  m_properties["EHigh"] = m_dblManager->addProperty("EHigh");
  m_dblManager->setDecimals(m_properties["EHigh"], NUM_DECIMALS);

  m_properties["SampleBinning"] = m_dblManager->addProperty("SampleBinning");
  m_dblManager->setDecimals(m_properties["SampleBinning"], 0);

  m_properties["SampleBins"] = m_dblManager->addProperty("SampleBins");
  m_dblManager->setDecimals(m_properties["SampleBins"], 0);
  m_properties["SampleBins"]->setEnabled(false);

  m_properties["ResolutionBins"] = m_dblManager->addProperty("ResolutionBins");
  m_dblManager->setDecimals(m_properties["ResolutionBins"], 0);
  m_properties["ResolutionBins"]->setEnabled(false);

  m_iqtTree->addProperty(m_properties["ELow"]);
  m_iqtTree->addProperty(m_properties["EWidth"]);
  m_iqtTree->addProperty(m_properties["EHigh"]);
  m_iqtTree->addProperty(m_properties["SampleBinning"]);
  m_iqtTree->addProperty(m_properties["SampleBins"]);
  m_iqtTree->addProperty(m_properties["ResolutionBins"]);

  m_dblManager->setValue(m_properties["SampleBinning"], 10);

  m_iqtTree->setFactoryForManager(m_dblManager, m_dblEdFac);

  // Format the tree widget so its easier to read the contents
  m_iqtTree->setIndentation(0);
  for (auto const &item : m_properties)
    m_iqtTree->setBackgroundColor(m_iqtTree->topLevelItem(item), QColor(246, 246, 246));

  setPreviewSpectrumMaximum(0);

  auto xRangeSelector = m_uiForm.ppPlot->addRangeSelector("IqtRange");
  xRangeSelector->setBounds(-DBL_MAX, DBL_MAX);

  // signals / slots & validators
  connect(m_uiForm.dsInput, SIGNAL(dataReady(const QString &)), this, SIGNAL(sampDataReady(const QString &)));
  connect(m_uiForm.dsResolution, SIGNAL(dataReady(const QString &)), this, SLOT(updateDisplayedBinParameters()));
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SIGNAL(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SIGNAL(saveClicked()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this, SIGNAL(plotCurrentPreview()));
  connect(m_uiForm.cbCalculateErrors, SIGNAL(clicked()), this, SLOT(errorsClicked()));
  connect(m_uiForm.spPreviewSpec, SIGNAL(valueChanged(int)), this, SIGNAL(PreviewSpectrumChanged(int)));
  connect(m_uiForm.ckSymmetricEnergy, SIGNAL(stateChanged(int)), this, SLOT(updateEnergyRange(int)));
  connect(xRangeSelector, SIGNAL(selectionChanged(double, double)), this, SLOT(rangeChanged(double, double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateRangeSelector(QtProperty *, double)));

  m_uiForm.dsInput->isOptional(true);
  m_uiForm.dsResolution->isOptional(true);
}

void InelasticDataManipulationIqtTabView::setPreviewSpectrumMaximum(int value) {
  m_uiForm.spPreviewSpec->setMaximum(value);
}

/**
 * Ensure we have present and valid file/ws inputs.
 *
 * The underlying Fourier transform of Iqt
 * also means we must enforce several rules on the parameters.
 */
bool InelasticDataManipulationIqtTabView::validate() {
  UserInputValidator uiv;

  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsInput);
  uiv.checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);

  auto const eLow = m_dblManager->value(m_properties["ELow"]);
  auto const eHigh = m_dblManager->value(m_properties["EHigh"]);

  if (eLow >= eHigh)
    uiv.addErrorMessage("ELow must be less than EHigh.\n");

  auto const message = uiv.generateErrorMessage();
  showMessageBox(message);

  return message.isEmpty();
}

void InelasticDataManipulationIqtTabView::setSampleFBSuffixes(QStringList const suffix) {
  m_uiForm.dsInput->setFBSuffixes(suffix);
}

void InelasticDataManipulationIqtTabView::setSampleWSSuffixes(QStringList const suffix) {
  m_uiForm.dsInput->setWSSuffixes(suffix);
}

void InelasticDataManipulationIqtTabView::setResolutionFBSuffixes(QStringList const suffix) {
  m_uiForm.dsResolution->setFBSuffixes(suffix);
}

void InelasticDataManipulationIqtTabView::setResolutionWSSuffixes(QStringList const suffix) {
  m_uiForm.dsResolution->setWSSuffixes(suffix);
}

void InelasticDataManipulationIqtTabView::setRunEnabled(bool enabled) { m_uiForm.pbRun->setEnabled(enabled); }

void InelasticDataManipulationIqtTabView::setSaveResultEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

void InelasticDataManipulationIqtTabView::setRunText(bool running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
}

void InelasticDataManipulationIqtTabView::setWatchADS(bool watch) { m_uiForm.ppPlot->watchADS(watch); }

/**
 * Plots the selected spectrum of the input workspace.
 */
void InelasticDataManipulationIqtTabView::plotInput(MatrixWorkspace_sptr inputWS, int spectrum) {
  m_uiForm.ppPlot->clear();

  if (inputWS && inputWS->x(spectrum).size() > 1) {
    m_uiForm.ppPlot->addSpectrum("Sample", inputWS, spectrum);
  }
}

/**
 * Updates the range selectors and properties when range selector is moved.
 *
 * @param min Range selector min value
 * @param max Range selector max value
 */
void InelasticDataManipulationIqtTabView::rangeChanged(double min, double max) {
  double oldMin = m_dblManager->value(m_properties["ELow"]);
  double oldMax = m_dblManager->value(m_properties["EHigh"]);

  auto xRangeSelector = m_uiForm.ppPlot->getRangeSelector("IqtRange");

  disconnect(xRangeSelector, SIGNAL(selectionChanged(double, double)), this, SLOT(rangeChanged(double, double)));
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updateRangeSelector(QtProperty *, double)));

  if (fabs(oldMin - min) > 0.0000001) {
    m_dblManager->setValue(m_properties["ELow"], min);
    xRangeSelector->setMinimum(min);
    if (m_uiForm.ckSymmetricEnergy->isChecked()) {
      m_dblManager->setValue(m_properties["EHigh"], -min);
      xRangeSelector->setMaximum(-min);
    }
  }

  if (fabs(oldMax - max) > 0.0000001) {
    m_dblManager->setValue(m_properties["EHigh"], max);
    xRangeSelector->setMaximum(max);
    if (m_uiForm.ckSymmetricEnergy->isChecked()) {
      m_dblManager->setValue(m_properties["ELow"], -max);
      xRangeSelector->setMinimum(-max);
    }
  }

  connect(xRangeSelector, SIGNAL(selectionChanged(double, double)), this, SLOT(rangeChanged(double, double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateRangeSelector(QtProperty *, double)));
}

void InelasticDataManipulationIqtTabView::setRangeSelectorDefault(const MatrixWorkspace_sptr workspace,
                                                                  const QPair<double, double> &range) {
  auto xRangeSelector = m_uiForm.ppPlot->getRangeSelector("IqtRange");
  try {
    double rounded_min(range.first);
    double rounded_max(range.second);
    const std::string instrName(workspace->getInstrument()->getName());
    if (instrName == "BASIS") {
      xRangeSelector->setRange(range.first, range.second);
      m_dblManager->setValue(m_properties["ELow"], rounded_min);
      m_dblManager->setValue(m_properties["EHigh"], rounded_max);
      m_dblManager->setValue(m_properties["EWidth"], 0.0004);
      m_dblManager->setValue(m_properties["SampleBinning"], 1);
    } else {
      rounded_min = floor(rounded_min * 10 + 0.5) / 10.0;
      rounded_max = floor(rounded_max * 10 + 0.5) / 10.0;

      // corrections for if nearest value is outside of range
      if (rounded_max > range.second) {
        rounded_max -= 0.1;
      }

      if (rounded_min < range.first) {
        rounded_min += 0.1;
      }

      // check incase we have a really small range
      if (fabs(rounded_min) > 0 && fabs(rounded_max) > 0) {
        xRangeSelector->setRange(rounded_min, rounded_max);
        m_dblManager->setValue(m_properties["ELow"], rounded_min);
        m_dblManager->setValue(m_properties["EHigh"], rounded_max);
      } else {
        xRangeSelector->setRange(range.first, range.second);
        m_dblManager->setValue(m_properties["ELow"], range.first);
        m_dblManager->setValue(m_properties["EHigh"], range.second);
      }
      // set default value for width
      m_dblManager->setValue(m_properties["EWidth"], 0.005);
    }
  } catch (std::invalid_argument &exc) {
    showMessageBox(exc.what());
  }
}

/**
 * Updates the range selectors when the ELow or EHigh property is changed in the
 * table.
 *
 * @param prop The property which has been changed
 * @param val The new position for the range selector
 */
void InelasticDataManipulationIqtTabView::updateRangeSelector(QtProperty *prop, double val) {
  auto xRangeSelector = m_uiForm.ppPlot->getRangeSelector("IqtRange");

  disconnect(xRangeSelector, SIGNAL(selectionChanged(double, double)), this, SLOT(rangeChanged(double, double)));
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updateRangeSelector(QtProperty *, double)));

  if (prop == m_properties["ELow"]) {
    setRangeSelectorMin(m_properties["ELow"], m_properties["EHigh"], xRangeSelector, val);
    if (m_uiForm.ckSymmetricEnergy->isChecked()) {
      m_dblManager->setValue(m_properties["EHigh"], -val);
      setRangeSelectorMax(m_properties["ELow"], m_properties["EHigh"], xRangeSelector, -val);
    }

  } else if (prop == m_properties["EHigh"]) {
    setRangeSelectorMax(m_properties["ELow"], m_properties["EHigh"], xRangeSelector, val);
    if (m_uiForm.ckSymmetricEnergy->isChecked()) {
      m_dblManager->setValue(m_properties["ELow"], -val);
      setRangeSelectorMin(m_properties["ELow"], m_properties["EHigh"], xRangeSelector, -val);
    }
  }

  connect(xRangeSelector, SIGNAL(selectionChanged(double, double)), this, SLOT(rangeChanged(double, double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateRangeSelector(QtProperty *, double)));

  updateDisplayedBinParameters();
}

/**
 * Calculates binning parameters.
 */
void InelasticDataManipulationIqtTabView::updateDisplayedBinParameters() {
  auto const sampleName = m_uiForm.dsInput->getCurrentDataName().toStdString();
  auto const resolutionName = m_uiForm.dsResolution->getCurrentDataName().toStdString();

  auto &ads = AnalysisDataService::Instance();
  if (!ads.doesExist(sampleName) || !ads.doesExist(resolutionName))
    return;

  double energyMin = m_dblManager->value(m_properties["ELow"]);
  double energyMax = m_dblManager->value(m_properties["EHigh"]);
  double numBins = m_dblManager->value(m_properties["SampleBinning"]);

  if (numBins == 0)
    return;
  if (energyMin == 0 && energyMax == 0)
    return;

  bool success(false);
  float energyWidth(0.0f);
  int resolutionBins(0), sampleBins(0);
  std::tie(success, energyWidth, sampleBins, resolutionBins) =
      calculateBinParameters(sampleName, resolutionName, energyMin, energyMax, numBins);
  if (success) {
    disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
               SLOT(updateRangeSelector(QtProperty *, double)));

    // Update data in property editor
    m_dblManager->setValue(m_properties["EWidth"], energyWidth);
    m_dblManager->setValue(m_properties["ResolutionBins"], resolutionBins);
    m_dblManager->setValue(m_properties["SampleBins"], sampleBins);

    connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
            SLOT(updateRangeSelector(QtProperty *, double)));

    // Warn for low number of resolution bins
    if (resolutionBins < 5)
      showMessageBox("Results may be inaccurate as ResolutionBins is "
                     "less than 5.\nLower the SampleBinning.");
  }
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
void InelasticDataManipulationIqtTabView::setRangeSelectorMin(QtProperty *minProperty, QtProperty *maxProperty,
                                                              RangeSelector *rangeSelector, double newValue) {
  if (newValue <= maxProperty->valueText().toDouble())
    rangeSelector->setMinimum(newValue);
  else
    m_dblManager->setValue(minProperty, rangeSelector->getMinimum());
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
void InelasticDataManipulationIqtTabView::setRangeSelectorMax(QtProperty *minProperty, QtProperty *maxProperty,
                                                              RangeSelector *rangeSelector, double newValue) {
  if (newValue >= minProperty->valueText().toDouble())
    rangeSelector->setMaximum(newValue);
  else
    m_dblManager->setValue(maxProperty, rangeSelector->getMaximum());
}

void InelasticDataManipulationIqtTabView::updateEnergyRange(int state) {
  if (state != 0) {
    auto const value = m_dblManager->value(m_properties["ELow"]);
    m_dblManager->setValue(m_properties["EHigh"], -value);
  }
}

void InelasticDataManipulationIqtTabView::errorsClicked() {
  m_uiForm.spIterations->setEnabled(m_uiForm.cbCalculateErrors->isChecked());
}

// getters for properties
std::string InelasticDataManipulationIqtTabView::getSampleName() {
  return m_uiForm.dsInput->getCurrentDataName().toStdString();
}

std::string InelasticDataManipulationIqtTabView::getResolutionName() {
  return m_uiForm.dsResolution->getCurrentDataName().toStdString();
}

std::string InelasticDataManipulationIqtTabView::getIterations() {
  return m_uiForm.spIterations->cleanText().toStdString();
}

double InelasticDataManipulationIqtTabView::getELow() { return m_dblManager->value(m_properties["ELow"]); }

double InelasticDataManipulationIqtTabView::getEHigh() { return m_dblManager->value(m_properties["EHigh"]); }

double InelasticDataManipulationIqtTabView::getSampleBinning() {
  return m_dblManager->value(m_properties["SampleBinning"]);
}

bool InelasticDataManipulationIqtTabView::getCalculateErrors() { return m_uiForm.cbCalculateErrors->isChecked(); }

} // namespace MantidQt::CustomInterfaces