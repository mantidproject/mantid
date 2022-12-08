// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFInstrumentWidget.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"

using namespace Mantid::API;

namespace {

void loadEmptyInstrument(std::string const &instrumentName, std::string const &outputName) {
  auto alg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
  alg->initialize();
  alg->setProperty("InstrumentName", instrumentName);
  alg->setProperty("OutputWorkspace", outputName);
  alg->execute();
}

} // namespace

namespace MantidQt::CustomInterfaces {

ALFInstrumentWidget::ALFInstrumentWidget(QString workspaceName)
    : MantidWidgets::InstrumentWidget(std::move(workspaceName), nullptr, true, true, 0.0, 0.0, true,
                                      MantidWidgets::InstrumentWidget::Dependencies(), false, getTabCustomizations()) {
  removeTab("Instrument");
  removeTab("Draw");
  hideHelp();

  m_pickTab->expandPlotPanel();
}

MantidWidgets::InstrumentWidget::TabCustomizations ALFInstrumentWidget::getTabCustomizations() const {
  MantidWidgets::InstrumentWidget::TabCustomizations customizations;
  customizations.pickTools = std::vector<MantidWidgets::IWPickToolType>{
      MantidWidgets::IWPickToolType::Zoom,       MantidWidgets::IWPickToolType::PixelSelect,
      MantidWidgets::IWPickToolType::TubeSelect, MantidWidgets::IWPickToolType::PeakSelect,
      MantidWidgets::IWPickToolType::EditShape,  MantidWidgets::IWPickToolType::DrawRectangle};
  return customizations;
}

void ALFInstrumentWidget::handleActiveWorkspaceDeleted() {
  // We do not want to close the InstrumentWidget. Instead we want to load an empty ALF instrument and reset the
  // instrument view.
  loadEmptyInstrument("ALF", getWorkspaceNameStdString());
  resetInstrumentActor(true, true, 0.0, 0.0, true);
}

std::vector<std::size_t>
ALFInstrumentWidget::findWholeTubeDetectorIndices(std::vector<std::size_t> const &partTubeDetectorIndices) {
  auto &componentInfo = m_instrumentActor->componentInfo();

  std::vector<std::size_t> wholeTubeIndices;
  for (auto const &detectorIndex : partTubeDetectorIndices) {
    auto const iter = std::find(wholeTubeIndices.cbegin(), wholeTubeIndices.cend(), detectorIndex);
    // Check that the indices for this tube haven't already been added
    if (iter == wholeTubeIndices.cend()) {
      // Find all of the detector indices for the whole tube
      auto const detectors = componentInfo.detectorsInSubtree(componentInfo.parent(detectorIndex));
      std::transform(detectors.cbegin(), detectors.cend(), std::back_inserter(wholeTubeIndices),
                     [](auto const &index) { return index; });
    }
  }
  return wholeTubeIndices;
}

} // namespace MantidQt::CustomInterfaces
