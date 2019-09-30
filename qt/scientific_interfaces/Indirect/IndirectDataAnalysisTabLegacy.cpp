// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
      m_selectedSpectrum(0), m_minSpectrum(0), m_maxSpectrum(0) {
  m_parent = dynamic_cast<IndirectDataAnalysis *>(parent);

  // Create Editor Factories
  m_dblEdFac = new DoubleEditorFactory(this);
  m_blnEdFac = new QtCheckBoxFactory(this);
}

void IndirectDataAnalysisTab::setOutputPlotOptionsPresenter(
    std::unique_ptr<IndirectPlotOptionsPresenter> presenter) {
  m_plotOptionsPresenter = std::move(presenter);
}

void IndirectDataAnalysisTab::setOutputPlotOptionsWorkspaces(
    std::vector<std::string> const &outputWorkspaces) {
  m_plotOptionsPresenter->setWorkspaces(outputWorkspaces);
}

void IndirectDataAnalysisTab::clearOutputPlotOptionsWorkspaces() {
  m_plotOptionsPresenter->clearWorkspaces();
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
 * Prevents the loading of data with incorrect naming if passed true
 *
 * @param filter :: true if you want to allow filtering
 */
void IndirectDataAnalysisTab::filterInputData(bool filter) {
  setFileExtensionsByName(filter);
}

/**
 * Sets the active browser workspace when the tab is changed
 */
void IndirectDataAnalysisTab::setActiveWorkspace() { setBrowserWorkspace(); }

/**
 * Slot that can be called when a user edits an input.
 */
void IndirectDataAnalysisTab::inputChanged() { validate(); }

/**
 * Retrieves the input workspace to be used in data analysis.
 *
 * @return  The input workspace to be used in data analysis.
 */
MatrixWorkspace_sptr IndirectDataAnalysisTab::inputWorkspace() const {
  return m_inputWorkspace;
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
int IndirectDataAnalysisTab::selectedSpectrum() const {
  return m_selectedSpectrum;
}

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
int IndirectDataAnalysisTab::minimumSpectrum() const { return m_minSpectrum; }

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
int IndirectDataAnalysisTab::maximumSpectrum() const { return m_maxSpectrum; }

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
  auto index = boost::numeric_cast<size_t>(m_selectedSpectrum);

  // Check a workspace has been selected
  if (previewWs) {

    if (inputWs && previewWs->getName() == inputWs->getName()) {
      m_plotter->plotSpectra(previewWs->getName(), std::to_string(index));
    } else {
      m_plotter->plotSpectra(previewWs->getName(), "0-2");
    }
  } else if (inputWs && index < inputWs->getNumberHistograms()) {
    m_plotter->plotSpectra(inputWs->getName(), std::to_string(index));
  } else
    showMessageBox("Workspace not found - data may not be loaded.");
}

/**
 * Plots the selected spectrum of the input workspace in this indirect data
 * analysis tab.
 *
 * @param previewPlot The preview plot widget in which to plot the input
 *                    input workspace.
 */
void IndirectDataAnalysisTab::plotInput(
    MantidQt::MantidWidgets::PreviewPlot *previewPlot) {
  previewPlot->clear();
  auto inputWS = inputWorkspace();
  auto spectrum = selectedSpectrum();

  if (inputWS && inputWS->x(spectrum).size() > 1)
    previewPlot->addSpectrum("Sample", inputWorkspace(), spectrum);
}

/**
 * Clears all plots and plots the selected spectrum of the input workspace in
 * this indirect data analysis tab.
 *
 * @param fitPreviewPlot    The fit preview plot.
 * @param diffPreviewPlot   The difference preview plot.
 */
void IndirectDataAnalysisTab::clearAndPlotInput(
    MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
    MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot) {
  m_previewPlotWorkspace.reset();
  plotInput(fitPreviewPlot);
  diffPreviewPlot->clear();
}

/**
 * Plots the workspace at the specified index in the specified workspace
 * group. Plots the sample and fit spectrum in the specified top preview
 * plot. Plots the diff spectra in the specified difference preview plot.
 *
 * @param outputWSName      The name of the output workspace group.
 * @param index             The index of the workspace (in the group)
 *                          to plot.
 * @param fitPreviewPlot    The fit preview plot.
 * @param diffPreviewPlot   The difference preview plot.
 */
void IndirectDataAnalysisTab::updatePlot(
    const std::string &outputWSName, size_t index,
    MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
    MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot) {

  if (AnalysisDataService::Instance().doesExist(outputWSName)) {
    auto workspace = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
        outputWSName);

    if (workspace) {
      updatePlot(workspace, index, fitPreviewPlot, diffPreviewPlot);
      return;
    }
  }
  clearAndPlotInput(fitPreviewPlot, diffPreviewPlot);
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
  } else
    clearAndPlotInput(fitPreviewPlot, diffPreviewPlot);
}

/**
 * Plots the data in the workspace with the specified name. Plots the
 * sample and fit  spectrum in the specified top preview plot. Plots
 * the diff spectra in the specified difference preview plot.
 *
 * @param workspaceName     The name of the workspace to plot.
 * @param fitPreviewPlot    The fit preview plot.
 * @param diffPreviewPlot   The difference preview plot.
 */
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
  } else
    clearAndPlotInput(fitPreviewPlot, diffPreviewPlot);
}

/**
 * Plots the workspace at the index specified by the selected
 * spectrum, in the specified workspace group. Plots the sample
 * and fit  spectrum in the specified top preview plot. Plots
 * the diff spectra in the specified difference preview plot.
 *
 * @param outputWS          The group workspace containing the
 *                          workspaced to plot.
 * @param fitPreviewPlot    The fit preview plot.
 * @param diffPreviewPlot   The difference preview plot.
 */
void IndirectDataAnalysisTab::updatePlot(
    WorkspaceGroup_sptr outputWS,
    MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
    MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot) {
  if (outputWS && selectedSpectrum() >= minimumSpectrum() &&
      selectedSpectrum() <= maximumSpectrum())
    updatePlot(outputWS, selectedSpectrum() - minimumSpectrum(), fitPreviewPlot,
               diffPreviewPlot);
  else
    clearAndPlotInput(fitPreviewPlot, diffPreviewPlot);
}

/**
 * Plots the data in the specified workspace. Plots the sample
 * and fit  spectrum in the specified top preview plot. Plots
 * the diff spectra in the specified difference preview plot.
 *
 * @param outputWS          The workspace to plot.
 * @param fitPreviewPlot    The fit preview plot.
 * @param diffPreviewPlot   The difference preview plot.
 */
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
    diffPreviewPlot->addSpectrum("Difference", outputWS, 2, Qt::blue);
  } else
    clearAndPlotInput(fitPreviewPlot, diffPreviewPlot);
}

/*
 * Updates the plot range with the specified name, to match the range of
 * the sample curve.
 *
 * @param rangeName           The name of the range to update.
 * @param previewPlot         The preview plot widget, in which the range
 *                            is specified.
 * @param startRangePropName  The name of the property specifying the start
 *                            value for the range.
 * @parma endRangePropName    The name of the property specifying the end
 *                            value for the range.
 */
void IndirectDataAnalysisTab::updatePlotRange(
    const QString &rangeName, MantidQt::MantidWidgets::PreviewPlot *previewPlot,
    const QString &startRangePropName, const QString &endRangePropName) {

  if (inputWorkspace()) {
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
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
