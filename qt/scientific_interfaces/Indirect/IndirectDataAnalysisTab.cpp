// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisTab.h"
#include "IndirectSettingsHelper.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "boost/shared_ptr.hpp"

#include <QSettings>
#include <QString>
#include <utility>

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
    : IndirectTab(parent), m_dblEdFac(nullptr), m_blnEdFac(nullptr), m_parent(nullptr), m_inputWorkspace(),
      m_previewPlotWorkspace(), m_selectedSpectrum(0), m_minSpectrum(0), m_maxSpectrum(0) {
  m_parent = dynamic_cast<IndirectDataAnalysis *>(parent);

  // Create Editor Factories
  m_dblEdFac = new DoubleEditorFactory(this);
  m_blnEdFac = new QtCheckBoxFactory(this);
}

void IndirectDataAnalysisTab::setOutputPlotOptionsPresenter(std::unique_ptr<IndirectPlotOptionsPresenter> presenter) {
  m_plotOptionsPresenter = std::move(presenter);
}

void IndirectDataAnalysisTab::setOutputPlotOptionsWorkspaces(std::vector<std::string> const &outputWorkspaces) {
  m_plotOptionsPresenter->setWorkspaces(outputWorkspaces);
}

void IndirectDataAnalysisTab::clearOutputPlotOptionsWorkspaces() { m_plotOptionsPresenter->clearWorkspaces(); }

/**
 * Loads the tab's settings.
 *
 * Calls overridden version of loadSettings() in child class.
 *
 * @param settings :: the QSettings object from which to load
 */
void IndirectDataAnalysisTab::loadTabSettings(const QSettings &settings) {}

/**
 * Prevents the loading of data with incorrect naming if passed true
 *
 * @param filter :: true if you want to allow filtering
 */
void IndirectDataAnalysisTab::filterInputData(bool filter) { setFileExtensionsByName(filter); }

/**
 * Slot that can be called when a user edits an input.
 */
void IndirectDataAnalysisTab::inputChanged() { validate(); }

/**
 * Retrieves the input workspace to be used in data analysis.
 *
 * @return  The input workspace to be used in data analysis.
 */
MatrixWorkspace_sptr IndirectDataAnalysisTab::getInputWorkspace() const { return m_inputWorkspace; }

/**
 * Sets the input workspace to be used in data analysis.
 *
 * @param inputWorkspace  The workspace to set.
 */
void IndirectDataAnalysisTab::setInputWorkspace(MatrixWorkspace_sptr inputWorkspace) {
  m_inputWorkspace = std::move(inputWorkspace);
}

/**
 * Retrieves the workspace containing the data to be displayed in
 * the preview plot.
 *
 * @return  The workspace containing the data to be displayed in
 *          the preview plot.
 */
MatrixWorkspace_sptr IndirectDataAnalysisTab::getPreviewPlotWorkspace() { return m_previewPlotWorkspace.lock(); }

/**
 * Sets the workspace containing the data to be displayed in the
 * preview plot.
 *
 * @param previewPlotWorkspace The workspace to set.
 */
void IndirectDataAnalysisTab::setPreviewPlotWorkspace(const MatrixWorkspace_sptr &previewPlotWorkspace) {
  m_previewPlotWorkspace = previewPlotWorkspace;
}

/**
 * Retrieves the selected spectrum.
 *
 * @return  The selected spectrum.
 */
int IndirectDataAnalysisTab::getSelectedSpectrum() const { return m_selectedSpectrum; }

/**
 * Sets the selected spectrum.
 *
 * @param spectrum  The spectrum to set.
 */
void IndirectDataAnalysisTab::setSelectedSpectrum(int spectrum) { m_selectedSpectrum = spectrum; }

/**
 * Retrieves the selected minimum spectrum.
 *
 * @return  The selected minimum spectrum.
 */
int IndirectDataAnalysisTab::getMinimumSpectrum() const { return m_minSpectrum; }

/**
 * Sets the selected spectrum.
 *
 * @param spectrum  The spectrum to set.
 */
void IndirectDataAnalysisTab::setMinimumSpectrum(int spectrum) { m_minSpectrum = spectrum; }

/**
 * Retrieves the selected maximum spectrum.
 *
 * @return  The selected maximum spectrum.
 */
int IndirectDataAnalysisTab::getMaximumSpectrum() const { return m_maxSpectrum; }

/**
 * Sets the selected maximum spectrum.
 *
 * @param spectrum  The spectrum to set.
 */
void IndirectDataAnalysisTab::setMaximumSpectrum(int spectrum) { m_maxSpectrum = spectrum; }

/**
 * Plots the current preview workspace, if none is set, plots
 * the selected spectrum of the current input workspace.
 */
void IndirectDataAnalysisTab::plotCurrentPreview() {
  auto previewWs = getPreviewPlotWorkspace();
  auto inputWs = getInputWorkspace();
  auto index = boost::numeric_cast<size_t>(m_selectedSpectrum);
  auto const errorBars = IndirectSettingsHelper::externalPlotErrorBars();

  // Check a workspace has been selected
  if (previewWs) {

    if (inputWs && previewWs->getName() == inputWs->getName()) {
      m_plotter->plotSpectra(previewWs->getName(), std::to_string(index), errorBars);
    } else {
      m_plotter->plotSpectra(previewWs->getName(), "0-2", errorBars);
    }
  } else if (inputWs && index < inputWs->getNumberHistograms()) {
    m_plotter->plotSpectra(inputWs->getName(), std::to_string(index), errorBars);
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
void IndirectDataAnalysisTab::plotInput(MantidQt::MantidWidgets::PreviewPlot *previewPlot) {
  previewPlot->clear();
  auto inputWS = getInputWorkspace();
  auto spectrum = getSelectedSpectrum();

  if (inputWS && inputWS->x(spectrum).size() > 1)
    previewPlot->addSpectrum("Sample", getInputWorkspace(), spectrum);
}

/**
 * Clears all plots and plots the selected spectrum of the input workspace in
 * this indirect data analysis tab.
 *
 * @param fitPreviewPlot    The fit preview plot.
 * @param diffPreviewPlot   The difference preview plot.
 */
void IndirectDataAnalysisTab::clearAndPlotInput(MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
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
void IndirectDataAnalysisTab::updatePlot(const std::string &outputWSName, size_t index,
                                         MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                                         MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot) {

  if (AnalysisDataService::Instance().doesExist(outputWSName)) {
    auto workspace = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputWSName);

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
void IndirectDataAnalysisTab::updatePlot(const WorkspaceGroup_sptr &outputWS, size_t index,
                                         MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                                         MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot) {
  // Check whether the specified index is within the bounds of the
  // fitted spectrum.
  if (outputWS && index < outputWS->size()) {
    auto workspace = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(index));
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
void IndirectDataAnalysisTab::updatePlot(const std::string &workspaceName,
                                         MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                                         MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot) {

  if (AnalysisDataService::Instance().doesExist(workspaceName)) {
    auto groupWorkspace = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(workspaceName);
    // Check whether the specified workspace is a workspace group.
    if (groupWorkspace) {
      updatePlot(groupWorkspace, fitPreviewPlot, diffPreviewPlot);
    } else {
      auto matWorkspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName);
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
void IndirectDataAnalysisTab::updatePlot(const WorkspaceGroup_sptr &outputWS,
                                         MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
                                         MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot) {
  if (outputWS && getSelectedSpectrum() >= getMinimumSpectrum() && getSelectedSpectrum() <= getMaximumSpectrum())
    updatePlot(outputWS, getSelectedSpectrum() - getMinimumSpectrum(), fitPreviewPlot, diffPreviewPlot);
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
void IndirectDataAnalysisTab::updatePlot(const MatrixWorkspace_sptr &outputWS,
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

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
