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

#include <functional>
#include <qobject.h>
#include <tuple>

#include <iostream>

namespace MantidQt {
namespace MantidWidgets {

BaseCustomInstrumentPresenter::BaseCustomInstrumentPresenter(
    IBaseCustomInstrumentView *view, IBaseCustomInstrumentModel *model,
    IPlotFitAnalysisPanePresenter *analysisPanePresenter)
    : m_view(view), m_model(model), m_currentRun(0), m_currentFile(""),
      m_loadRunObserver(nullptr),
      m_analysisPanePresenter(analysisPanePresenter) {
  m_loadRunObserver = new VoidObserver();
  m_model->loadEmptyInstrument();
}

void BaseCustomInstrumentPresenter::addInstrument() {
  auto setUp = setupInstrument();
  initLayout(setUp);
}

void BaseCustomInstrumentPresenter::initLayout(
    std::pair<instrumentSetUp, instrumentObserverOptions> *setUp) {
  // connect to new run
  m_view->observeLoadRun(m_loadRunObserver);
  std::function<void()> loadBinder =
      std::bind(&BaseCustomInstrumentPresenter::loadRunNumber, this);
  m_loadRunObserver->setSlot(loadBinder);
  initInstrument(setUp);
  setUpInstrumentAnalysisSplitter();
  m_view->setupHelp();
}

void BaseCustomInstrumentPresenter::setUpInstrumentAnalysisSplitter() {
  auto paneView = m_analysisPanePresenter->getView();
  m_view->setupInstrumentAnalysisSplitters(paneView->getQWidget());
}

void BaseCustomInstrumentPresenter::loadAndAnalysis(
    const std::string &pathToRun) {
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

void BaseCustomInstrumentPresenter::loadRunNumber() {
  auto pathToRun = m_view->getFile();
  if (pathToRun == "" || m_currentFile == pathToRun) {
    return;
  }
  loadAndAnalysis(pathToRun);
}

void BaseCustomInstrumentPresenter::initInstrument(
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

} // namespace MantidWidgets
} // namespace MantidQt
