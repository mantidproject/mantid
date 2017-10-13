#include "IndirectDataAnalysisTab.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
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

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
