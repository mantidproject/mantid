// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFInstrumentPresenter.h"

#include "ALFInstrumentModel.h"
#include "ALFInstrumentView.h"

#include "MantidAPI/FileFinder.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"

namespace MantidQt::CustomInterfaces {

ALFInstrumentPresenter::ALFInstrumentPresenter(IALFInstrumentView *view, std::unique_ptr<IALFInstrumentModel> model)
    : m_view(view), m_model(std::move(model)) {
  m_view->subscribePresenter(this);
  m_view->setUpInstrument(m_model->loadedWsName());
}

QWidget *ALFInstrumentPresenter::getLoadWidget() { return m_view->generateLoadWidget(); }

MantidWidgets::InstrumentWidget *ALFInstrumentPresenter::getInstrumentView() { return m_view->getInstrumentView(); }

void ALFInstrumentPresenter::subscribeAnalysisPresenter(
    MantidQt::MantidWidgets::IPlotFitAnalysisPanePresenter *presenter) {
  m_analysisPresenter = presenter;
}

void ALFInstrumentPresenter::loadRunNumber() {
  auto const filepath = m_view->getFile();
  if (!filepath) {
    return;
  }
  if (auto const message = loadAndTransform(*filepath)) {
    m_view->warningBox(*message);
  }
  m_view->setRunQuietly(std::to_string(m_model->runNumber()));
}

std::optional<std::string> ALFInstrumentPresenter::loadAndTransform(const std::string &pathToRun) {
  try {
    return m_model->loadAndTransform(pathToRun);
  } catch (std::exception const &ex) {
    return ex.what();
  }
}

void ALFInstrumentPresenter::extractSingleTube() {
  m_model->extractSingleTube();

  m_analysisPresenter->addSpectrum(m_model->extractedWsName());
  m_analysisPresenter->updateEstimateClicked();
}

void ALFInstrumentPresenter::averageTube() {
  m_model->averageTube();
  m_analysisPresenter->addSpectrum(m_model->extractedWsName());
}

bool ALFInstrumentPresenter::showAverageTubeOption() const { return m_model->showAverageTubeOption(); }

} // namespace MantidQt::CustomInterfaces
