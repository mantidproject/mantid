#include "IndirectDataAnalysisTab.h"

#include "MantidAPI/AnalysisDataService.h"
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
 * Plots the workspace at the specified index in the workspace group
 * with the specified name. Plots the sample and fit spectrum in the
 * specified top preview plot. Plots the diff spectra in the specified
 * bottom preview plot.
 *
 * @param outputWSName      The name of the output workspace group.
 * @param index             The index of the workspace (in the group)
 *                          to plot.
 * @param topPreviewPlot    The top preview plot.
 * @param bottomPreviewPlot The bottom preview plot.
 */
void IndirectDataAnalysisTab::updatePlot(
    const std::string &outputWSName, size_t index,
    MantidQt::MantidWidgets::PreviewPlot *topPreviewPlot,
    MantidQt::MantidWidgets::PreviewPlot *bottomPreviewPlot) {

  if (AnalysisDataService::Instance().doesExist(outputWSName)) {
    auto workspace = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
        outputWSName);
    updatePlot(workspace, index, topPreviewPlot, bottomPreviewPlot);
  } else {
    bottomPreviewPlot->clear();
    plotInput(topPreviewPlot);
  }
}

/**
 * Plots the workspace at the specified index in the specified workspace
 * group. Plots the sample and fit spectrum in the specified top preview
 * plot. Plots the diff spectra in the specified bottom preview plot.
 *
 * @param outputWS          The output workspace group.
 * @param index             The index of the workspace (in the group)
 *                          to plot.
 * @param topPreviewPlot    The top preview plot.
 * @param bottomPreviewPlot The bottom preview plot.
 */
void IndirectDataAnalysisTab::updatePlot(
    WorkspaceGroup_sptr outputWS, size_t index,
    MantidQt::MantidWidgets::PreviewPlot *topPreviewPlot,
    MantidQt::MantidWidgets::PreviewPlot *bottomPreviewPlot) {
  // Check whether the specified index is within the bounds of the
  // fitted spectrum.
  if (outputWS && index >= 0 && index < outputWS->size()) {
    auto workspace =
        boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(index));
    updatePlot(workspace, topPreviewPlot, bottomPreviewPlot);
  } else {
    bottomPreviewPlot->clear();
    plotInput(topPreviewPlot);
  }
}

void IndirectDataAnalysisTab::updatePlot(
    const std::string &workspaceName,
    MantidQt::MantidWidgets::PreviewPlot *topPreviewPlot,
    MantidQt::MantidWidgets::PreviewPlot *bottomPreviewPlot) {
  auto workspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      workspaceName);
  updatePlot(workspace, topPreviewPlot, bottomPreviewPlot);
}

void IndirectDataAnalysisTab::updatePlot(
    MatrixWorkspace_sptr outputWS,
    MantidQt::MantidWidgets::PreviewPlot *topPreviewPlot,
    MantidQt::MantidWidgets::PreviewPlot *bottomPreviewPlot) {
  topPreviewPlot->clear();
  bottomPreviewPlot->clear();

  if (outputWS) {
    setPreviewPlotWorkspace(outputWS);
    topPreviewPlot->addSpectrum("Sample", outputWS, 0, Qt::black);
    topPreviewPlot->addSpectrum("Fit", outputWS, 1, Qt::red);
    bottomPreviewPlot->addSpectrum("Diff", outputWS, 2, Qt::blue);
  } else {
    plotInput(topPreviewPlot);
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
  } catch (std::invalid_argument &exc) {
    showMessageBox(exc.what());
  }
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
