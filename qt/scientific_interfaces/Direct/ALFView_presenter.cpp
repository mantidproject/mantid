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
#include <tuple>

namespace MantidQt {
namespace CustomInterfaces {

ALFView_presenter::ALFView_presenter(ALFView_view *view, ALFView_model *model,
                                     PlotFitAnalysisPanePresenter *analysisPane)
    : BaseInstrumentPresenter(view, model, analysisPane->getView()),
      m_view(view), m_model(model), m_analysisPane(analysisPane),
      m_extractSingleTubeObserver(nullptr), m_averageTubeObserver(nullptr) {
  addInstrument();
}

void ALFView_presenter::addInstrument() {
  auto setUp = setupALFInstrument();
  initLayout(&setUp);
}

void ALFView_presenter::setUpInstrumentAnalysisSplitter() {
  CompositeFunction_sptr composite = m_model->getDefaultFunction();
  m_analysisPane->addFunction(composite);
  m_view->setupAnalysisPane(m_analysisPane->getView());
}

void ALFView_presenter::loadSideEffects() { m_analysisPane->clearCurrentWS(); }

typedef std::pair<std::string,
                  std::vector<std::function<bool(std::map<std::string, bool>)>>>
    instrumentSetUp;
typedef std::vector<std::tuple<std::string, Observer *>>
    instrumentObserverOptions;

/**
* This creates the custom instrument widget
* @return <instrumentSetUp,
    instrumentObserverOptions> : a pair of the conditions and observers
*/
std::pair<instrumentSetUp, instrumentObserverOptions>
ALFView_presenter::setupALFInstrument() {

  m_extractSingleTubeObserver = new VoidObserver();
  m_averageTubeObserver = new VoidObserver();

  instrumentSetUp setUpContextConditions;

  // set up the slots for the custom context menu
  std::vector<std::tuple<std::string, Observer *>> customInstrumentOptions;
  std::vector<std::function<bool(std::map<std::string, bool>)>> binders;

  // set up custom context menu conditions
  std::function<bool(std::map<std::string, bool>)> extractConditionBinder =
      std::bind(&ALFView_model::extractTubeConditon, m_model,
                std::placeholders::_1);
  std::function<bool(std::map<std::string, bool>)> averageTubeConditonBinder =
      std::bind(&ALFView_model::averageTubeConditon, m_model,
                std::placeholders::_1);

  binders.push_back(extractConditionBinder);
  binders.push_back(averageTubeConditonBinder);

  setUpContextConditions = std::make_pair(m_model->dataFileName(), binders);

  // set up single tube extract
  std::function<void()> extractSingleTubeBinder =
      std::bind(&ALFView_presenter::extractSingleTube, this); // binder for slot
  m_extractSingleTubeObserver->setSlot(
      extractSingleTubeBinder); // add slot to observer
  std::tuple<std::string, Observer *> tmp = std::make_tuple(
      "singleTube", m_extractSingleTubeObserver); // store observer for later
  customInstrumentOptions.push_back(tmp);

  // set up average tube
  std::function<void()> averageTubeBinder =
      std::bind(&ALFView_presenter::averageTube, this);
  m_averageTubeObserver->setSlot(averageTubeBinder);
  tmp = std::make_tuple("averageTube", m_averageTubeObserver);
  customInstrumentOptions.push_back(tmp);

  return std::make_pair(setUpContextConditions, customInstrumentOptions);
}

void ALFView_presenter::extractSingleTube() {
  m_model->extractSingleTube();
  const std::string WSName = m_model->WSName();
  m_analysisPane->addSpectrum(WSName);
}

void ALFView_presenter::averageTube() {
  m_model->averageTube();
  const std::string WSName = m_model->WSName();
  m_analysisPane->addSpectrum(WSName);
}

} // namespace CustomInterfaces
} // namespace MantidQt
