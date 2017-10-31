#include "IndirectDataAnalysisTab.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "boost/shared_ptr.hpp"

#include <QSettings>
#include <QString>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
/**
 * Constructor.
 *
 * @param parent :: the parent widget (an IndirectDataAnalysis object).
 */
IndirectDataAnalysisTab::IndirectDataAnalysisTab(QWidget *parent)
    : IndirectTab(parent), m_dblEdFac(nullptr), m_blnEdFac(nullptr),
      m_parent(nullptr), m_inputWorkspace(), m_previewPlotWorkspace(),
      m_selectedSpectrum(0) {
  m_parent = dynamic_cast<IndirectDataAnalysis *>(parent);

  // Create Editor Factories
  m_dblEdFac = new DoubleEditorFactory(this);
  m_blnEdFac = new QtCheckBoxFactory(this);
}

/**
 * Loads the tab's settings.
 *
 * Calls overridden version of loadSettings() in child class.
 *
 * @param settings :: the QSettings object from which to load
 */
void IndirectDataAnalysisTab::loadTabSettings(const QSettings &settings) {
  loadSettings(settings);
}

/**
 * Slot that can be called when a user edits an input.
 */
void IndirectDataAnalysisTab::inputChanged() { validate(); }

/**
 * Retrieves the input workspace to be used in data analysis.
 *
 * @return  The input workspace to be used in data analysis.
 */
MatrixWorkspace_sptr IndirectDataAnalysisTab::inputWorkspace() {
  return m_inputWorkspace.lock();
}

/**
 * Sets the input workspace to be used in data analysis.
 *
 * @param inputWorkspace  The workspace to set.
 */
void IndirectDataAnalysisTab::setInputWorkspace(
    MatrixWorkspace_sptr inputWorkspace) {
  m_inputWorkspace = inputWorkspace;
}

/**
 * Retrieves the workspace containing the data to be displayed in
 * the preview plot.
 *
 * @return  The workspace containing the data to be displayed in
 *          the preview plot.
 */
MatrixWorkspace_sptr IndirectDataAnalysisTab::previewPlotWorkspace() {
  return m_previewPlotWorkspace.lock();
}

/**
 * Sets the workspace containing the data to be displayed in the
 * preview plot.
 *
 * @param previewPlotWorkspace The workspace to set.
 */
void IndirectDataAnalysisTab::setPreviewPlotWorkspace(
    MatrixWorkspace_sptr previewPlotWorkspace) {
  m_previewPlotWorkspace = previewPlotWorkspace;
}

/**
 * Retrieves the selected spectrum.
 *
 * @return  The selected spectrum.
 */
int IndirectDataAnalysisTab::selectedSpectrum() { return m_selectedSpectrum; }

/**
 * Sets the selected spectrum.
 *
 * @param spectrum  The spectrum to set.
 */
void IndirectDataAnalysisTab::setSelectedSpectrum(int spectrum) {
  m_selectedSpectrum = spectrum;
}

/**
 * Retrieves the selected minimum spectrum.
 *
 * @return  The selected minimum spectrum.
 */
int IndirectDataAnalysisTab::minimumSpectrum() { return m_minSpectrum; }

/**
 * Sets the selected spectrum.
 *
 * @param spectrum  The spectrum to set.
 */
void IndirectDataAnalysisTab::setMinimumSpectrum(int spectrum) {
  m_minSpectrum = spectrum;
}

/**
 * Retrieves the selected maximum spectrum.
 *
 * @return  The selected maximum spectrum.
 */
int IndirectDataAnalysisTab::maximumSpectrum() { return m_maxSpectrum; }

/**
 * Sets the selected maximum spectrum.
 *
 * @param spectrum  The spectrum to set.
 */
void IndirectDataAnalysisTab::setMaximumSpectrum(int spectrum) {
  m_maxSpectrum = spectrum;
}

/**
 * Plots the current preview workspace, if none is set, plots
 * the selected spectrum of the current input workspace.
 */
void IndirectDataAnalysisTab::plotCurrentPreview() {
  auto previewWs = previewPlotWorkspace();
  auto inputWs = inputWorkspace();

  // Check a workspace has been selected
  if (previewWs) {

    if (inputWs && previewWs->getName() == inputWs->getName()) {
      IndirectTab::plotSpectrum(QString::fromStdString(previewWs->getName()),
                                m_selectedSpectrum);
    } else {
      IndirectTab::plotSpectrum(QString::fromStdString(previewWs->getName()), 0,
                                2);
    }

  } else if (inputWs &&
             inputWs->getNumberHistograms() <
                 boost::numeric_cast<size_t>(m_selectedSpectrum)) {
    IndirectTab::plotSpectrum(QString::fromStdString(inputWs->getName()),
                              m_selectedSpectrum);
  }
}

/**
 * Plots the selected spectrum of the input workspace in this indirect data
 * analysis tab.
 */
void IndirectDataAnalysisTab::plotInput(
    MantidQt::MantidWidgets::PreviewPlot *previewPlot) {
  previewPlot->clear();
  auto inputWS = inputWorkspace();

  if (inputWS)
    previewPlot->addSpectrum("Sample", inputWorkspace(), selectedSpectrum());
}

/**
 * Plots the workspace at the specified index in the specified workspace
 * group. Plots the sample and fit spectrum in the specified top preview
 * plot. Plots the diff spectra in the specified difference preview plot.
 *
 * @param outputWS          The output workspace group.
 * @param index             The index of the workspace (in the group)
 *                          to plot.
 * @param fitPreviewPlot    The fit preview plot.
 * @param diffPreviewPlot   The difference preview plot.
 */
void IndirectDataAnalysisTab::updatePlot(
    WorkspaceGroup_sptr outputWS, size_t index,
    MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
    MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot) {
  // Check whether the specified index is within the bounds of the
  // fitted spectrum.
  if (outputWS && index < outputWS->size()) {
    auto workspace =
        boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(index));
    updatePlot(workspace, fitPreviewPlot, diffPreviewPlot);
  } else {
    diffPreviewPlot->clear();
    plotInput(fitPreviewPlot);
  }
}

void IndirectDataAnalysisTab::updatePlot(
    const std::string &workspaceName,
    MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
    MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot) {

  if (AnalysisDataService::Instance().doesExist(workspaceName)) {
    auto groupWorkspace =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            workspaceName);
    // Check whether the specified workspace is a workspace group.
    if (groupWorkspace) {
      updatePlot(groupWorkspace, fitPreviewPlot, diffPreviewPlot);
    } else {
      auto matWorkspace =
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
              workspaceName);
      updatePlot(matWorkspace, fitPreviewPlot, diffPreviewPlot);
    }
  } else {
    diffPreviewPlot->clear();
    plotInput(fitPreviewPlot);
  }
}

void IndirectDataAnalysisTab::updatePlot(
    WorkspaceGroup_sptr outputWS,
    MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
    MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot) {
  if (outputWS && selectedSpectrum() >= minimumSpectrum())
    updatePlot(outputWS, selectedSpectrum() - minimumSpectrum(), fitPreviewPlot,
               diffPreviewPlot);
  else {
    diffPreviewPlot->clear();
    plotInput(fitPreviewPlot);
  }
}

void IndirectDataAnalysisTab::updatePlot(
    MatrixWorkspace_sptr outputWS,
    MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
    MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot) {
  fitPreviewPlot->clear();
  diffPreviewPlot->clear();

  if (outputWS) {
    setPreviewPlotWorkspace(outputWS);
    fitPreviewPlot->addSpectrum("Sample", outputWS, 0, Qt::black);
    fitPreviewPlot->addSpectrum("Fit", outputWS, 1, Qt::red);
    diffPreviewPlot->addSpectrum("Diff", outputWS, 2, Qt::blue);
  } else {
    diffPreviewPlot->clear();
    plotInput(fitPreviewPlot);
  }
}

void IndirectDataAnalysisTab::updatePlotRange(
    const QString &rangeName, MantidQt::MantidWidgets::PreviewPlot *previewPlot,
    const QString &startRangePropName, const QString &endRangePropName) {
  try {
    const QPair<double, double> curveRange =
        previewPlot->getCurveRange("Sample");
    auto rangeSelector = previewPlot->getRangeSelector(rangeName);
    setPlotPropertyRange(rangeSelector, m_properties[startRangePropName],
                         m_properties[endRangePropName], curveRange);
  } catch (std::exception &exc) {
    showMessageBox(exc.what());
  }
}

void IndirectDataAnalysisTab::plotGuess(
    MantidQt::MantidWidgets::PreviewPlot *previewPlot,
    IFunction_sptr function) {
  previewPlot->removeSpectrum("Guess");

  if (inputWorkspace()) {
    auto guessWs = createGuessWorkspace(function);
    previewPlot->addSpectrum("Guess", guessWs, 0, Qt::green);
  }
}

MatrixWorkspace_sptr
IndirectDataAnalysisTab::createGuessWorkspace(IFunction_sptr func) {
  const auto inputWS = inputWorkspace();
  const auto startX = m_dblManager->value(m_properties["StartX"]);
  const auto endX = m_dblManager->value(m_properties["EndX"]);
  const size_t binIndexLow = inputWS->binIndexOf(startX);
  const size_t binIndexHigh = inputWS->binIndexOf(endX);
  const size_t nData = binIndexHigh - binIndexLow;

  const auto &xPoints = inputWS->points(0);

  std::vector<double> dataX(nData);
  std::copy(&xPoints[binIndexLow], &xPoints[binIndexLow + nData],
            dataX.begin());
  std::vector<double> dataY = computeOutput(func, dataX);

  IAlgorithm_sptr createWsAlg =
      createWorkspaceAlgorithm("__GuessAnon", 1, dataX, dataY);
  createWsAlg->execute();
  return createWsAlg->getProperty("OutputWorkspace");
}

std::vector<double>
IndirectDataAnalysisTab::computeOutput(IFunction_sptr func,
                                       const std::vector<double> &dataX) {
  FunctionDomain1DVector domain(dataX);
  FunctionValues outputData(domain);
  func->function(domain, outputData);

  std::vector<double> dataY(dataX.size());
  for (size_t i = 0; i < dataY.size(); i++) {
    dataY[i] = outputData.getCalculated(i);
  }
  return dataY;
}

IAlgorithm_sptr IndirectDataAnalysisTab::createWorkspaceAlgorithm(
    const std::string &workspaceName, int numSpec,
    const std::vector<double> &dataX, const std::vector<double> &dataY) {
  IAlgorithm_sptr createWsAlg =
      AlgorithmManager::Instance().create("CreateWorkspace");
  createWsAlg->initialize();
  createWsAlg->setChild(true);
  createWsAlg->setLogging(false);
  createWsAlg->setProperty("OutputWorkspace", workspaceName);
  createWsAlg->setProperty("NSpec", numSpec);
  createWsAlg->setProperty("DataX", dataX);
  createWsAlg->setProperty("DataY", dataY);
  return createWsAlg;
}

/**
 * Create and populates a function with given values
 * @param funcName  The name of the function to create and populate populate
 * @param group     The QtProperty representing the fit type
 * @param comp      A composite function of the previously called functions to
 *                  be used in tie
 * @param tie       Bool to state if parameters are to be tied together
 * @param pref      The index of the functions eg. (f0.f1)
 */
IFunction_sptr IndirectDataAnalysisTab::createPopulatedFunction(
    const std::string &funcName, IFunction_sptr comp, QtProperty *group,
    bool tie, const std::string &pref) {
  IFunction_sptr func = FunctionFactory::Instance().createFunction(funcName);
  populateFunction(func, comp, group, tie, pref);
  return func;
}

/**
 * Create and populates a function with given values
 * @param func  The function to populate
 * @param group The QtProperty representing the fit type
 * @param tie   Bool to state if parameters are to be tied together
 * @param pref  The index of the functions eg. (f0.f1)
 */
IFunction_sptr
IndirectDataAnalysisTab::createPopulatedFunction(const std::string &funcName,
                                                 QtProperty *group, bool tie,
                                                 const std::string &pref) {
  IFunction_sptr func = FunctionFactory::Instance().createFunction(funcName);
  populateFunction(func, group, tie, pref);
  return func;
}

/**
 * Populates the properties of a function with given values
 * @param func  The function to populate
 * @param group The QtProperty representing the fit type
 * @param tie   Bool to state if parameters are to be tied together
 * @param pref  The index of the functions eg. (f0.f1)
 */
void IndirectDataAnalysisTab::populateFunction(IFunction_sptr func,
                                               QtProperty *group, bool tie,
                                               const std::string &pref) {
  populateFunction(func, func, group, tie, pref);
}

/**
 * Populates the properties of a function with given values
 * @param func  The function currently being added to the composite
 * @param comp  A composite function of the previously called functions
 * @param group The QtProperty representing the fit type
 * @param pref  The index of the functions eg. (f0.f1)
 * @param tie   Bool to state if parameters are to be tied together
 */
void IndirectDataAnalysisTab::populateFunction(IFunction_sptr func,
                                               IFunction_sptr comp,
                                               QtProperty *group, bool tie,
                                               const std::string &pref) {
  // Get sub-properties of group and apply them as parameters on the function
  // object
  QList<QtProperty *> props = group->subProperties();

  for (const auto &prop : props) {
    if (tie || !prop->subProperties().isEmpty()) {
      std::string name = pref + prop->propertyName().toStdString();
      std::string value = prop->valueText().toStdString();
      comp->tie(name, value);
    } else {
      std::string propName = prop->propertyName().toStdString();
      double propValue = prop->valueText().toDouble();
      if (propValue != 0.0) {
        if (func->hasAttribute(propName))
          func->setAttributeValue(propName, propValue);
        else
          func->setParameter(propName, propValue);
      }
    }
  }
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
