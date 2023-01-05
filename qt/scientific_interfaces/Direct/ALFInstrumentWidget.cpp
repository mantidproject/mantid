// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFInstrumentWidget.h"

#include "MantidAPI/AlgorithmManager.h"
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

} // namespace MantidQt::CustomInterfaces
