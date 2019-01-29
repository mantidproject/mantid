// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataManipulationTab.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectDataManipulationTab::IndirectDataManipulationTab(QWidget *parent)
    : IndirectTab(parent), m_inputWorkspace(), m_selectedSpectrum(0),
      m_dblEdFac(nullptr), m_blnEdFac(nullptr) {
  // m_parent = dynamic_cast<IndirectDataManipulation *>(parent);
  m_dblEdFac = new DoubleEditorFactory(this);
  m_blnEdFac = new QtCheckBoxFactory(this);
}

void IndirectDataManipulationTab::setInputWorkspace(
    MatrixWorkspace_sptr workspace) {
  m_inputWorkspace = workspace;
}

MatrixWorkspace_sptr IndirectDataManipulationTab::getInputWorkspace() const {
  return m_inputWorkspace;
}

int IndirectDataManipulationTab::getSelectedSpectrum() const {
  return m_selectedSpectrum;
}

void IndirectDataManipulationTab::plotInput(PreviewPlot *previewPlot) {
  previewPlot->clear();
  auto const inputWS = getInputWorkspace();
  auto const spectrum = getSelectedSpectrum();

  if (inputWS && inputWS->x(spectrum).size() > 1)
    previewPlot->addSpectrum("Sample", inputWS, spectrum);
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
void IndirectDataManipulationTab::updatePlotRange(
    std::string const &rangeName, PreviewPlot *previewPlot,
    std::string const &startRangePropName,
    std::string const &endRangePropName) {

  if (getInputWorkspace()) {
    try {
      QPair<double, double> const curveRange =
          previewPlot->getCurveRange("Sample");
      auto const rangeSelector =
          previewPlot->getRangeSelector(QString::fromStdString(rangeName));
      setPlotPropertyRange(
          rangeSelector,
          m_properties[QString::fromStdString(startRangePropName)],
          m_properties[QString::fromStdString(endRangePropName)], curveRange);
    } catch (std::exception const &ex) {
      showMessageBox(ex.what());
    }
  }
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
