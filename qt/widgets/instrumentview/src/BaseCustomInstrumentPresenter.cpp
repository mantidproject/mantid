// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentPresenter.h"
#include "MantidAPI/FileFinder.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentModel.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentView.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"

namespace MantidQt::MantidWidgets {

BaseCustomInstrumentPresenter::BaseCustomInstrumentPresenter(IBaseCustomInstrumentView *view,
                                                             IBaseCustomInstrumentModel *model)
    : m_view(view), m_model(model), m_currentRun(0), m_currentFile("") {
  m_view->subscribePresenter(this);
  m_model->loadEmptyInstrument();
  addInstrument();
}

void BaseCustomInstrumentPresenter::subscribeAnalysisPresenter(
    MantidQt::MantidWidgets::PlotFitAnalysisPanePresenter *presenter) {
  m_analysisPresenter = presenter;
}

void BaseCustomInstrumentPresenter::addInstrument() {
  auto setUp = setupInstrument();
  initLayout(setUp);
}

QWidget *BaseCustomInstrumentPresenter::getLoadWidget() { return m_view->generateLoadWidget(); }

MantidWidgets::InstrumentWidget *BaseCustomInstrumentPresenter::getInstrumentView() {
  return m_view->getInstrumentView();
}

void BaseCustomInstrumentPresenter::initLayout(std::pair<instrumentSetUp, instrumentObserverOptions> &setUp) {
  initInstrument(setUp);
  m_view->setupHelp();
}

void BaseCustomInstrumentPresenter::loadAndAnalysis(const std::string &pathToRun) {
  try {
    auto loadedResult = m_model->loadData(pathToRun);

    if (loadedResult.second == "success") {
      m_currentRun = loadedResult.first;
      m_currentFile = pathToRun;
    } else {
      // reset to the previous data
      m_view->warningBox(loadedResult.second);
    }
    // make displayed run number be in sinc
    m_view->setRunQuietly(std::to_string(m_currentRun));
    m_model->setCurrentRun(m_currentRun);
  } catch (...) {
    m_view->setRunQuietly(std::to_string(m_currentRun));
    m_model->setCurrentRun(m_currentRun);
  }
}

void BaseCustomInstrumentPresenter::loadRunNumber() {
  auto pathToRun = m_view->getFile();
  if (pathToRun == "" || m_currentFile == pathToRun) {
    return;
  }
  loadAndAnalysis(pathToRun);
}

void BaseCustomInstrumentPresenter::extractSingleTube() {
  m_model->extractSingleTube();
  const std::string WSName = m_model->WSName();
  m_analysisPresenter->addSpectrum(WSName);
  m_analysisPresenter->updateEstimateClicked();
}

void BaseCustomInstrumentPresenter::averageTube() {
  m_model->averageTube();
  const std::string WSName = m_model->WSName();
  m_analysisPresenter->addSpectrum(WSName);
}

void BaseCustomInstrumentPresenter::initInstrument(std::pair<instrumentSetUp, instrumentObserverOptions> &setUp) {
  // set up instrument
  auto instrumentSetUp = setUp.first;

  m_view->setUpInstrument(instrumentSetUp.first, instrumentSetUp.second);
}

using instrumentSetUp = std::pair<std::string, std::vector<std::function<bool(std::map<std::string, bool>)>>>;
using instrumentObserverOptions = std::vector<std::tuple<std::string, Observer *>>;

/**
* This creates the custom instrument widget
* @return <instrumentSetUp,
    instrumentObserverOptions> : a pair of the conditions and observers
*/
std::pair<instrumentSetUp, instrumentObserverOptions> BaseCustomInstrumentPresenter::setupInstrument() {
  instrumentSetUp setUpContextConditions;

  // set up the slots for the custom context menu
  std::vector<std::tuple<std::string, Observer *>> customInstrumentOptions;
  std::vector<std::function<bool(std::map<std::string, bool>)>> binders;

  // set up custom context menu conditions
  std::function<bool(std::map<std::string, bool>)> extractConditionBinder =
      std::bind(&MantidWidgets::IBaseCustomInstrumentModel::extractTubeCondition, m_model, std::placeholders::_1);
  std::function<bool(std::map<std::string, bool>)> averageTubeConditionBinder =
      std::bind(&MantidWidgets::IBaseCustomInstrumentModel::averageTubeCondition, m_model, std::placeholders::_1);

  binders.emplace_back(extractConditionBinder);
  binders.emplace_back(averageTubeConditionBinder);

  setUpContextConditions = std::make_pair(m_model->dataFileName(), binders);

  return std::make_pair(setUpContextConditions, customInstrumentOptions);
}

} // namespace MantidQt::MantidWidgets
