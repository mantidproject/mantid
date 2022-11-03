// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFInstrumentPresenter.h"

#include "ALFInstrumentModel.h"
#include "ALFInstrumentView.h"
#include "MantidAPI/FileFinder.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"

namespace MantidQt::CustomInterfaces {

ALFInstrumentPresenter::ALFInstrumentPresenter(IALFInstrumentView *view, IALFInstrumentModel *model)
    : m_view(view), m_model(model) {
  m_view->subscribePresenter(this);
  addInstrument();
}

void ALFInstrumentPresenter::subscribeAnalysisPresenter(
    MantidQt::MantidWidgets::PlotFitAnalysisPanePresenter *presenter) {
  m_analysisPresenter = presenter;
}

void ALFInstrumentPresenter::addInstrument() {
  m_view->setUpInstrument(m_model->getWSName());
  m_view->setupHelp();
}

QWidget *ALFInstrumentPresenter::getLoadWidget() { return m_view->generateLoadWidget(); }

MantidWidgets::InstrumentWidget *ALFInstrumentPresenter::getInstrumentView() { return m_view->getInstrumentView(); }

void ALFInstrumentPresenter::loadAndAnalysis(const std::string &pathToRun) {
  try {
    if (auto const message = m_model->loadData(pathToRun)) {
      m_view->warningBox(*message);
    }
    auto runNumber = m_model->runNumber();
    // make displayed run number be in sinc
    m_view->setRunQuietly(std::to_string(runNumber));
    m_model->setCurrentRun(runNumber);
  } catch (...) {
    auto runNumber = m_model->runNumber();
    m_view->setRunQuietly(std::to_string(runNumber));
    m_model->setCurrentRun(runNumber);
  }
}

void ALFInstrumentPresenter::loadRunNumber() {
  auto pathToRun = m_view->getFile();
  if (pathToRun.empty()) {
    return;
  }
  loadAndAnalysis(pathToRun);
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

bool ALFInstrumentPresenter::hasTubeBeenExtracted() const { return m_model->hasTubeBeenExtracted(); }

int ALFInstrumentPresenter::numberOfTubesInAverage() const { return m_model->numberOfTubesInAverage(); }

} // namespace MantidQt::CustomInterfaces
