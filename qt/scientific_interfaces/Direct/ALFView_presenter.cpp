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
    : m_view(view), m_model(model), m_currentRun(0), m_loadRunObserver(nullptr),
      m_browseObserver(nullptr) {
  m_loadRunObserver = new loadObserver();
  m_browseObserver = new generalObserver();
  m_model->loadEmptyInstrument();
}

void ALFView_presenter::initLayout() {
  // connect to new run
  m_view->observeLoadRun(m_loadRunObserver);
  m_loadRunObserver->setSlot(
      std::bind(&ALFView_presenter::loadRunNumber, this));
  // connect to browse run
  m_view->observeBrowse(m_browseObserver);
  m_browseObserver->setSlot(std::bind(&ALFView_presenter::loadBrowsedFile, this,
                                      std::placeholders::_1));
}

void ALFView_presenter::loadAndAnalysis(const std::string &run) {
  int runNumber = m_model->loadData(run);
  auto bools = m_model->isDataValid();
  if (bools["IsValidInstrument"]) {
    m_model->rename();
    m_currentRun = runNumber;
  } else {
    m_model->remove();
  }
  // if the displayed run number is out of sinc
  if (m_view->getRunNumber() != m_currentRun) {
    m_view->setRunQuietly(QString::number(m_currentRun));
  }
  if (bools["IsValidInstrument"] && !bools["IsItDSpace"]) {
    m_model->transformData();
  }
}

void ALFView_presenter::loadRunNumber() {
  int newRun = m_view->getRunNumber();
  const int currentRunInADS = m_model->currentRun();

  if (currentRunInADS == newRun) {
    return;
  }
  const std::string runNumber = "ALF" + std::to_string(newRun);
  std::string filePath;
  // check its a valid run number
  try {
    filePath = Mantid::API::FileFinder::Instance().findRuns(runNumber)[0];
  } catch (...) {
    m_view->setRunQuietly(QString::number(m_currentRun));
    // if file has been deleted we should replace it
    if (currentRunInADS == -999) {
      loadAndAnalysis("ALF" + std::to_string(m_currentRun));
    }
    return;
  }
  loadAndAnalysis(runNumber);
}

void ALFView_presenter::loadBrowsedFile(const std::string fileName) {
  m_model->loadData(fileName);
  loadAndAnalysis(fileName);
}

} // namespace CustomInterfaces
} // namespace MantidQt