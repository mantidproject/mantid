// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFView_presenter.h"
#include "ALFView_model.h"
#include "ALFView_view.h"

#include "MantidAPI/FileFinder.h"

#include <functional>

namespace MantidQt {
namespace CustomInterfaces {

ALFView_presenter::ALFView_presenter(ALFView_view *view, ALFView_model *model)
    : m_view(view), m_model(model), m_currentRun(0), m_currentFile(""),
      m_loadRunObserver(nullptr), m_numberOfTubesInAverage(0),
      m_extractSingleTubeObserver(nullptr), m_averageTubeObserver(nullptr) {
  m_loadRunObserver = new VoidObserver();
  m_extractSingleTubeObserver = new VoidObserver();
  m_averageTubeObserver = new VoidObserver();
  m_model->loadEmptyInstrument();
}

void ALFView_presenter::initLayout() {
  // connect to new run
  m_view->observeLoadRun(m_loadRunObserver);
  std::function<void()> loadBinder =
      std::bind(&ALFView_presenter::loadRunNumber, this);
  m_loadRunObserver->setSlot(loadBinder);
  initInstrument();
}

void ALFView_presenter::loadAndAnalysis(const std::string &pathToRun) {
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
  } catch (...) {
    m_view->setRunQuietly(std::to_string(m_currentRun));
  }
}

void ALFView_presenter::loadRunNumber() {
  auto pathToRun = m_view->getFile();
  if (pathToRun == "" || m_currentFile == pathToRun) {
    return;
  }
  loadAndAnalysis(pathToRun);
}


void ALFView_presenter::initInstrument() {
  // set up instrument
  std::function<bool(std::map<std::string, bool>)> extractConditionBinder =
      std::bind(&ALFView_presenter::extractTubeConditon, this,
                std::placeholders::_1);
  std::function<bool(std::map<std::string, bool>)> averageTubeConditonBinder =
      std::bind(&ALFView_presenter::averageTubeConditon, this,
                std::placeholders::_1);
  m_view->setUpInstrument(m_model->dataFileName(), extractConditionBinder,
                          averageTubeConditonBinder);

  // set up single tube extract
  m_view->observeExtractSingleTube(m_extractSingleTubeObserver);
  std::function<void()> extractSingleTubeBinder =
      std::bind(&ALFView_presenter::extractSingleTube, this);
  m_extractSingleTubeObserver->setSlot(extractSingleTubeBinder);

  // set up average tube
  m_view->observeAverageTube(m_averageTubeObserver);
  std::function<void()> averageTubeBinder =
      std::bind(&ALFView_presenter::averageTube, this);
  m_averageTubeObserver->setSlot(averageTubeBinder);
}

bool ALFView_presenter::extractTubeConditon(
    std::map<std::string, bool> tabBools) {
  try {

    bool ifCurve = (tabBools.find("plotStroed")->second ||
                    tabBools.find("hasCurve")->second);
    return (tabBools.find("isTube")->second && ifCurve);
  } catch (...) {
    return false;
  }
}

bool ALFView_presenter::averageTubeConditon(
    std::map<std::string, bool> tabBools) {
  try {

    bool ifCurve = (tabBools.find("plotStroed")->second ||
                    tabBools.find("hasCurve")->second);
    return (m_numberOfTubesInAverage > 0 && tabBools.find("isTube")->second &&
            ifCurve &&
            m_model->hasTubeBeenExtracted(m_model->getInstrument() +
                                          std::to_string(m_currentRun)));
  } catch (...) {
    return false;
  }
}
void ALFView_presenter::extractSingleTube() {
  m_model->storeSingleTube(m_model->getInstrument() +
                           std::to_string(m_currentRun));
  m_numberOfTubesInAverage = 1;
}

void ALFView_presenter::averageTube() {
  m_model->averageTube(m_numberOfTubesInAverage,
                       m_model->getInstrument() + std::to_string(m_currentRun));
  m_numberOfTubesInAverage++;
}

} // namespace CustomInterfaces
} // namespace MantidQt