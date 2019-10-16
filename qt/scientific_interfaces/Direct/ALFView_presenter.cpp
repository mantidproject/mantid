// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFView_presenter.h"
#include "ALFView_view.h"

#include "MantidAPI/FileFinder.h"

#include <functional>
#include <tuple>

namespace MantidQt {
namespace CustomInterfaces {

ALFView_presenter::ALFView_presenter(BaseInstrumentView *view,
                                     BaseInstrumentModel *model)
    : m_view(view), m_model(model), m_currentRun(0), m_currentFile(""),
      m_loadRunObserver(nullptr) {
  m_loadRunObserver = new VoidObserver();
  m_model->loadEmptyInstrument();
}

void ALFView_presenter::initLayout(
    std::pair<instrumentSetUp, instrumentObserverOptions> *setUp) {
  // connect to new run
  m_view->observeLoadRun(m_loadRunObserver);
  std::function<void()> loadBinder =
      std::bind(&ALFView_presenter::loadRunNumber, this);
  m_loadRunObserver->setSlot(loadBinder);
  initInstrument(setUp);
  m_view->setupInstrumentPlotFitSplitters();
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
    m_model->setCurrentRun(m_currentRun);
  } catch (...) {
    m_view->setRunQuietly(std::to_string(m_currentRun));
    m_model->setCurrentRun(m_currentRun);
  }
}

void ALFView_presenter::loadRunNumber() {
  auto pathToRun = m_view->getFile();
  if (pathToRun == "" || m_currentFile == pathToRun) {
    return;
  }
  loadAndAnalysis(pathToRun);
}

// All of the below are specific to ALF

void ALFView_presenter::initInstrument(
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

} // namespace CustomInterfaces
} // namespace MantidQt