#include "JumpFit.h"
#include "../General/UserInputValidator.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TextAxis.h"

#include <boost/lexical_cast.hpp>
#include <string>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

JumpFit::JumpFit(QWidget *parent) : IndirectFitAnalysisTab(parent) {
  m_uiForm.setupUi(parent);
  m_jfTree = m_propertyTree;
}

void JumpFit::setup() {
  // Create range selector
  auto qRangeSelector = m_uiForm.ppPlotTop->addRangeSelector("JumpFitQ");
  connect(qRangeSelector, SIGNAL(selectionChangedLazy(double, double)), this,
          SLOT(qRangeChanged(double, double)));

  // Add the properties browser to the ui form
  m_jfTree->setFactoryForManager(m_dblManager, m_dblEdFac);
  m_uiForm.treeSpace->addWidget(m_jfTree);

  // Fitting range
  m_properties["QMin"] = m_dblManager->addProperty("QMin");
  m_properties["QMax"] = m_dblManager->addProperty("QMax");

  m_dblManager->setDecimals(m_properties["QMin"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["QMax"], NUM_DECIMALS);

  m_jfTree->addProperty(m_properties["QMin"]);
  m_jfTree->addProperty(m_properties["QMax"]);

  m_properties["ChudleyElliot"] = createFunctionProperty("ChudleyElliot");
  m_properties["HallRoss"] = createFunctionProperty("HallRoss");
  m_properties["FickDiffusion"] = createFunctionProperty("FickDiffusion");
  m_properties["TeixeiraWater"] = createFunctionProperty("TeixeiraWater");

  m_uiForm.cbWidth->setEnabled(false);

  // Connect data selector to handler method
  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this,
          SLOT(handleSampleInputReady(const QString &)));
  // Connect width selector to handler method
  connect(m_uiForm.cbWidth, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(handleWidthChange(const QString &)));

  // Update fit parameters in browser when function is selected
  connect(m_uiForm.cbFunction, SIGNAL(currentIndexChanged(const QString &)),
          this, SLOT(fitFunctionSelected(const QString &)));
  connect(m_uiForm.cbFunction, SIGNAL(currentIndexChanged(const QString &)),
          this, SLOT(updatePlot()));

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateRS(QtProperty *, double)));

  fitFunctionSelected(m_uiForm.cbFunction->currentText());
  enablePlotGuess();

  // Handle plotting and saving
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));
}

/**
 * Validate the form to check the program can be run
 *
 * @return :: Whether the form was valid
 */
bool JumpFit::validate() {
  UserInputValidator uiv;
  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);

  // this workspace doesn't have any valid widths
  if (m_spectraList.size() == 0) {
    uiv.addErrorMessage(
        "Input workspace doesn't appear to contain any width data.");
  }

  QString errors = uiv.generateErrorMessage();
  if (!errors.isEmpty()) {
    emit showMessageBox(errors);
    return false;
  }

  return true;
}

/**
 * Collect the settings on the GUI and build a python
 * script that runs JumpFit
 */
void JumpFit::run() {
  // Do noting with invalid data
  if (!m_uiForm.dsSample->isValid())
    return;

  if (m_batchAlgoRunner->queueLength() > 0)
    return;

  // Fit function to use
  const QString functionName = m_uiForm.cbFunction->currentText();
  setFitFunctions({functionName});
  // Setup fit algorithm
  m_fitAlg = createFitAlgorithm(createPopulatedFunction(
      functionName.toStdString(), m_properties[functionName]));
  m_baseName = m_fitAlg->getPropertyValue("OutputWorkspace");

  m_batchAlgoRunner->addAlgorithm(m_fitAlg);
  // Connect algorithm runner to completion handler function
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(fitAlgDone(bool)));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles the JumpFit algorithm finishing, used to plot fit in miniplot.
 *
 * @param error True if the algorithm failed, false otherwise
 */
void JumpFit::fitAlgDone(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(fitAlgDone(bool)));
  // Ignore errors
  if (error)
    return;
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);

  // Get output workspace name
  const auto paramWsName = m_fitAlg->getPropertyValue("Output") + "_Parameters";
  IndirectFitAnalysisTab::fitAlgorithmComplete(paramWsName + "_Parameters");
}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The current settings
 */
void JumpFit::loadSettings(const QSettings &settings) {
  m_uiForm.dsSample->readSettings(settings.group());
}

/**
 * Plots the loaded file to the miniplot and sets the guides
 * and the range
 *
 * @param filename :: The name of the workspace to plot
 */
void JumpFit::handleSampleInputReady(const QString &filename) {
  m_baseName = "";
  // Scale to convert to HWHM
  QString sample = filename + "_HWHM";
  IAlgorithm_sptr scaleAlg = AlgorithmManager::Instance().create("Scale");
  scaleAlg->initialize();
  scaleAlg->setProperty("InputWorkspace", filename.toStdString());
  scaleAlg->setProperty("OutputWorkspace", sample.toStdString());
  scaleAlg->setProperty("Factor", 0.5);
  scaleAlg->execute();
  IndirectFitAnalysisTab::newInputDataLoaded(sample);

  findAllWidths(inputWorkspace());

  auto qRangeSelector = m_uiForm.ppPlotTop->getRangeSelector("JumpFitQ");

  if (m_spectraList.size() > 0) {
    m_uiForm.cbWidth->setEnabled(true);
    std::string currentWidth = m_uiForm.cbWidth->currentText().toStdString();
    setSelectedSpectrum(m_spectraList[currentWidth]);

    QPair<double, double> res;
    QPair<double, double> range = m_uiForm.ppPlotTop->getCurveRange("Sample");

    // Use the values from the instrument parameter file if we can
    if (getResolutionRangeFromWs(sample, res))
      setRangeSelector(qRangeSelector, m_properties["QMin"],
                       m_properties["QMax"], res);
    else
      setRangeSelector(qRangeSelector, m_properties["QMin"],
                       m_properties["QMax"], range);

    setPlotPropertyRange(qRangeSelector, m_properties["QMin"],
                         m_properties["QMax"], range);
  } else {
    m_uiForm.cbWidth->setEnabled(false);
    emit showMessageBox("Workspace doesn't appear to contain any width data");
  }
}

/**
 * Find all of the spectra in the workspace that have width data
 *
 * @param ws :: The workspace to search
 */
void JumpFit::findAllWidths(Mantid::API::MatrixWorkspace_const_sptr ws) {
  m_uiForm.cbWidth->blockSignals(true);
  m_uiForm.cbWidth->clear();
  m_spectraList.clear();
  auto axis = dynamic_cast<Mantid::API::TextAxis *>(ws->getAxis(1));

  if (!axis)
    return;

  for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
    std::string title = axis->label(i);

    // check if the axis labels indicate this spectrum is width data
    size_t qLinesWidthIndex = title.find(".Width");
    size_t convFitWidthIndex = title.find(".FWHM");

    bool qLinesWidth = qLinesWidthIndex != std::string::npos;
    bool convFitWidth = convFitWidthIndex != std::string::npos;

    // if we get a match, add this spectrum to the combobox
    if (convFitWidth || qLinesWidth) {
      std::string cbItemName = "";
      size_t substrIndex = 0;

      if (qLinesWidth) {
        substrIndex = qLinesWidthIndex;
      } else if (convFitWidth) {
        substrIndex = convFitWidthIndex;
      }

      cbItemName = title.substr(0, substrIndex);
      m_spectraList[cbItemName] = static_cast<int>(i);
      m_uiForm.cbWidth->addItem(QString(cbItemName.c_str()));

      // display widths f1.f1, f2.f1 and f2.f2
      if (m_uiForm.cbWidth->count() == 3) {
        return;
      }
    }
  }
  m_uiForm.cbWidth->blockSignals(false);
}

/**
 * Plots the loaded file to the miniplot when the selected spectrum changes
 *
 * @param text :: The name spectrum index to plot
 */
void JumpFit::handleWidthChange(const QString &text) {
  QString sampleName = (m_uiForm.dsSample->getCurrentDataName() + "_HWHM");

  if (!sampleName.isEmpty() && m_spectraList.size() > 0) {
    if (validate()) {
      m_uiForm.ppPlotTop->clear();
      m_uiForm.ppPlotTop->addSpectrum("Sample", sampleName,
                                      m_spectraList[text.toStdString()]);
    }
  }
}

/**
 * Updates the property manager when the range selector is moved on the mini
 *plot.
 *
 * @param min :: The new value of the lower guide
 * @param max :: The new value of the upper guide
 */
void JumpFit::qRangeChanged(double min, double max) {
  m_dblManager->setValue(m_properties["QMin"], min);
  m_dblManager->setValue(m_properties["QMax"], max);
}

/**
 * Handles when properties in the property manager are updated.
 *
 * @param prop :: The property being updated
 * @param val :: The new value for the property
 */
void JumpFit::updateRS(QtProperty *prop, double val) {
  UNUSED_ARG(val);

  auto qRangeSelector = m_uiForm.ppPlotTop->getRangeSelector("JumpFitQ");

  if (prop == m_properties["QMin"] || prop == m_properties["QMax"]) {
    auto bounds = qMakePair(m_dblManager->value(m_properties["QMin"]),
                            m_dblManager->value(m_properties["QMax"]));
    setRangeSelector(qRangeSelector, m_properties["QMin"], m_properties["QMax"],
                     bounds);
  }
}

void JumpFit::disablePlotGuess() {
  m_uiForm.ckPlotGuess->setEnabled(false);
  disconnect(m_dblManager, SIGNAL(propertyChanged(QtProperty *)), this,
             SLOT(plotGuess()));
  disconnect(m_uiForm.ckPlotGuess, SIGNAL(stateChanged(int)), this,
             SLOT(plotGuess()));
  disconnect(m_uiForm.cbFunction, SIGNAL(currentIndexChanged(int)), this,
             SLOT(plotGuess()));
}

void JumpFit::enablePlotGuess() {
  m_uiForm.ckPlotGuess->setEnabled(true);
  connect(m_dblManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(plotGuess()));
  connect(m_uiForm.ckPlotGuess, SIGNAL(stateChanged(int)), this,
          SLOT(plotGuess()));
  connect(m_uiForm.cbFunction, SIGNAL(currentIndexChanged(int)), this,
          SLOT(plotGuess()));
  plotGuess();
}

/**
 * Handles a new fit function being selected.
 *
 * @param functionName Name of new fit function
 */
void JumpFit::fitFunctionSelected(const QString &functionName) {
  const auto plotGuess = m_uiForm.ckPlotGuess->isChecked();
  if (plotGuess) {
    m_uiForm.ckPlotGuess->setChecked(false);
  }
  setPropertyFunctions({functionName});
}

/**
 * Updates the plot
 */
void JumpFit::updatePlot() {
  const auto baseGroupName = m_baseName + "_Workspaces";
  IndirectFitAnalysisTab::updatePlot(baseGroupName, m_uiForm.ppPlotTop,
                                     m_uiForm.ppPlotBottom);
  IndirectDataAnalysisTab::updatePlotRange("JumpFitQ", m_uiForm.ppPlotTop);
}

void JumpFit::plotGuess() {
  // Do nothing if there is not a sample
  if (m_uiForm.dsSample->isValid() && m_uiForm.ckPlotGuess->isChecked()) {
    const auto functionName = m_uiForm.cbFunction->currentText();
    IndirectDataAnalysisTab::plotGuess(
        m_uiForm.ppPlotTop,
        createPopulatedFunction(functionName.toStdString(),
                                m_properties[functionName]));
  } else {
    m_uiForm.ppPlotTop->removeSpectrum("Guess");
    m_uiForm.ckPlotGuess->setChecked(false);
  }
}

IAlgorithm_sptr JumpFit::createFitAlgorithm(IFunction_sptr func) {
  std::string widthText = m_uiForm.cbWidth->currentText().toStdString();
  int width = m_spectraList[widthText];
  const auto sample =
      m_uiForm.dsSample->getCurrentDataName().toStdString() + "_HWHM";
  const auto startX = m_dblManager->value(m_properties["QMin"]);
  const auto endX = m_dblManager->value(m_properties["QMax"]);

  auto fitAlg = AlgorithmManager::Instance().create("Fit");
  fitAlg->initialize();
  fitAlg->setProperty("Function", func->asString());
  fitAlg->setProperty("InputWorkspace", sample);
  fitAlg->setProperty("WorkspaceIndex", width);
  fitAlg->setProperty("IgnoreInvalidData", true);
  fitAlg->setProperty("StartX", startX);
  fitAlg->setProperty("EndX", endX);
  fitAlg->setProperty("CreateOutput", true);
  fitAlg->setProperty("Output", "__PlotGuessData");
  return fitAlg;
}

/**
 * Handles mantid plotting
 */
void JumpFit::plotClicked() {
  std::string outWsName = m_fitAlg->getPropertyValue("Output") + "_Workspace";
  IndirectFitAnalysisTab::plotResult(outWsName, "All");
}

/**
 * Handles saving of workspace
 */
void JumpFit::saveClicked() {
  std::string outWsName = m_fitAlg->getPropertyValue("Output") + "_Workspace";
  IndirectFitAnalysisTab::saveResult(outWsName);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
