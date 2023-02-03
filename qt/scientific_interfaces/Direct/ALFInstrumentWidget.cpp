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
      MantidWidgets::IWPickToolType::Zoom, MantidWidgets::IWPickToolType::PixelSelect,
      MantidWidgets::IWPickToolType::TubeSelect, MantidWidgets::IWPickToolType::DrawRectangle,
      MantidWidgets::IWPickToolType::EditShape};
  return customizations;
}

void ALFInstrumentWidget::handleActiveWorkspaceDeleted() {
  // We do not want to close the InstrumentWidget. Instead we want to load an empty ALF instrument and reset the
  // instrument view.
  loadEmptyInstrument("ALF", getWorkspaceNameStdString());
  resetInstrumentActor(true, true, 0.0, 0.0, true);
}

std::vector<DetectorTube>
ALFInstrumentWidget::findWholeTubeDetectorIndices(std::vector<std::size_t> const &partTubeDetectorIndices) {
  auto &componentInfo = m_instrumentActor->componentInfo();

  std::vector<DetectorTube> tubes;
  std::vector<std::size_t> allocatedIndices;
  for (auto const &detectorIndex : partTubeDetectorIndices) {
    auto const iter = std::find(allocatedIndices.cbegin(), allocatedIndices.cend(), detectorIndex);
    // Check that the indices for this tube haven't already been added
    if (iter == allocatedIndices.cend() && componentInfo.isDetector(detectorIndex)) {
      // Find all of the detector indices for the whole tube
      auto tubeDetectorIndices = componentInfo.detectorsInSubtree(componentInfo.parent(detectorIndex));
      std::transform(tubeDetectorIndices.cbegin(), tubeDetectorIndices.cend(), std::back_inserter(allocatedIndices),
                     [](auto const &index) { return index; });
      tubes.emplace_back(tubeDetectorIndices);
    }
  }
  return tubes;
}

void ALFInstrumentWidget::drawRectanglesAbove(std::vector<DetectorTube> const &tubes) {
  auto surface = std::dynamic_pointer_cast<MantidWidgets::UnwrappedSurface>(m_instrumentDisplay->getSurface());

  for (auto const &tube : tubes) {
    drawRectangleAbove(surface, tube);
  }
}

void ALFInstrumentWidget::drawRectangleAbove(std::shared_ptr<MantidWidgets::UnwrappedSurface> surface,
                                             DetectorTube const &tube) {
  auto const firstDetectorRect = surface->detectorQRectInPixels(tube.front());
  auto const lastDetectorRect = surface->detectorQRectInPixels(tube.back());

  if (!firstDetectorRect.isNull() && !lastDetectorRect.isNull()) {
    // It is important to block signals when drawing the shape to prevent calling 'notifyShapeChanged'
    surface->blockSignals(true);
    surface->drawShape2D("rectangle", Qt::green, QColor(255, 255, 255, 80), lastDetectorRect.topLeft(),
                         firstDetectorRect.bottomRight(), false);
    surface->blockSignals(false);
  }
}

} // namespace MantidQt::CustomInterfaces
