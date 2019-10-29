// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "BaseInstrumentPresenter.h"
#include "BaseInstrumentView.h"
#include "MantidAPI/FileFinder.h"

#include <functional>
#include <tuple>

namespace MantidQt {
namespace CustomInterfaces {

BaseInstrumentPresenter::BaseInstrumentPresenter(BaseInstrumentView *view,
                                                 BaseInstrumentModel *model,
                                                 QWidget *analysisPaneView)
    : m_view(view), m_model(model), m_currentRun(0), m_currentFile(""),
      m_loadRunObserver(nullptr), m_analysisPaneView(analysisPaneView) {
  m_loadRunObserver = new VoidObserver();
  m_model->loadEmptyInstrument();
}

void BaseInstrumentPresenter::addInstrument() {
  auto setUp = setupInstrument();
  initLayout(&setUp);
}

void BaseInstrumentPresenter::initLayout(
    std::pair<instrumentSetUp, instrumentObserverOptions> *setUp) {
  // connect to new run
  m_view->observeLoadRun(m_loadRunObserver);
  std::function<void()> loadBinder =
      std::bind(&BaseInstrumentPresenter::loadRunNumber, this);
  m_loadRunObserver->setSlot(loadBinder);
  initInstrument(setUp);
  setUpInstrumentAnalysisSplitter();
  m_view->setupHelp();
}

void BaseInstrumentPresenter::setUpInstrumentAnalysisSplitter() {
  m_view->setupInstrumentAnalysisSplitters(m_analysisPaneView);
}

void BaseInstrumentPresenter::loadAndAnalysis(const std::string &pathToRun) {
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
    loadSideEffects();
  } catch (...) {
    m_view->setRunQuietly(std::to_string(m_currentRun));
    m_model->setCurrentRun(m_currentRun);
  }
}

void BaseInstrumentPresenter::loadRunNumber() {
  auto pathToRun = m_view->getFile();
  if (pathToRun == "" || m_currentFile == pathToRun) {
    return;
  }
  loadAndAnalysis(pathToRun);
}

// All of the below are specific to ALF

void BaseInstrumentPresenter::initInstrument(
    std::pair<instrumentSetUp, instrumentObserverOptions> *setUp) {
  if (!setUp) {
    return;
  }
  // set up instrument
  auto instrumentSetUp = setUp->first;

  m_view->setUpInstrument(instrumentSetUp.first, instrumentSetUp.second);

  auto customContextMenu = setUp->second;
  for (auto options : customContextMenu) {
    m_view->addObserver(options);
  }
}

typedef std::pair<std::string,
                  std::vector<std::function<bool(std::map<std::string, bool>)>>>
    instrumentSetUp;
typedef std::vector<std::tuple<std::string, Observer *>>
    instrumentObserverOptions;
std::pair<instrumentSetUp, instrumentObserverOptions>

BaseInstrumentPresenter::setupInstrument() {
  instrumentSetUp setUpContextConditions;

  // set up the slots for the custom context menu
  std::vector<std::tuple<std::string, Observer *>> customInstrumentOptions;
  std::vector<std::function<bool(std::map<std::string, bool>)>> binders;

  setUpContextConditions = std::make_pair(m_model->dataFileName(), binders);

  return std::make_pair(setUpContextConditions, customInstrumentOptions);
}

} // namespace CustomInterfaces
} // namespace MantidQt
