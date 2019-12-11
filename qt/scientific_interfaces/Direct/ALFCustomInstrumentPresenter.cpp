// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFCustomInstrumentPresenter.h"
#include "ALFCustomInstrumentModel.h"
#include "ALFCustomInstrumentView.h"
#include "MantidAPI/FileFinder.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"

#include <functional>
#include <tuple>

namespace MantidQt {
namespace CustomInterfaces {

ALFCustomInstrumentPresenter::ALFCustomInstrumentPresenter(
    ALFCustomInstrumentView *view, ALFCustomInstrumentModel *model,
    MantidWidgets::PlotFitAnalysisPanePresenter *analysisPane)
    : BaseCustomInstrumentPresenter(view, model, analysisPane->getView()),
      m_view(view), m_model(model), m_analysisPane(analysisPane),
      m_extractSingleTubeObserver(nullptr), m_averageTubeObserver(nullptr) {
  addInstrument();
}

void ALFCustomInstrumentPresenter::addInstrument() {
  auto setUp = setupALFInstrument();
  initLayout(&setUp);
}

void ALFCustomInstrumentPresenter::setUpInstrumentAnalysisSplitter() {
  CompositeFunction_sptr composite = m_model->getDefaultFunction();
  m_analysisPane->addFunction(composite);
  m_view->setupAnalysisPane(m_analysisPane->getView());
}

void ALFCustomInstrumentPresenter::loadSideEffects() {
  m_analysisPane->clearCurrentWS();
}

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
ALFCustomInstrumentPresenter::setupALFInstrument() {

  m_extractSingleTubeObserver = new VoidObserver();
  m_averageTubeObserver = new VoidObserver();

  instrumentSetUp setUpContextConditions;

  // set up the slots for the custom context menu
  std::vector<std::tuple<std::string, Observer *>> customInstrumentOptions;
  std::vector<std::function<bool(std::map<std::string, bool>)>> binders;

  // set up custom context menu conditions
  std::function<bool(std::map<std::string, bool>)> extractConditionBinder =
      std::bind(&ALFCustomInstrumentModel::extractTubeConditon, m_model,
                std::placeholders::_1);
  std::function<bool(std::map<std::string, bool>)> averageTubeConditonBinder =
      std::bind(&ALFCustomInstrumentModel::averageTubeConditon, m_model,
                std::placeholders::_1);

  binders.emplace_back(extractConditionBinder);
  binders.emplace_back(averageTubeConditonBinder);

  setUpContextConditions = std::make_pair(m_model->dataFileName(), binders);

  // set up single tube extract
  std::function<void()> extractSingleTubeBinder =
      std::bind(&ALFCustomInstrumentPresenter::extractSingleTube,
                this); // binder for slot
  m_extractSingleTubeObserver->setSlot(
      extractSingleTubeBinder); // add slot to observer
  std::tuple<std::string, Observer *> tmp = std::make_tuple(
      "singleTube", m_extractSingleTubeObserver); // store observer for later
  customInstrumentOptions.emplace_back(tmp);

  // set up average tube
  std::function<void()> averageTubeBinder =
      std::bind(&ALFCustomInstrumentPresenter::averageTube, this);
  m_averageTubeObserver->setSlot(averageTubeBinder);
  tmp = std::make_tuple("averageTube", m_averageTubeObserver);
  customInstrumentOptions.emplace_back(tmp);

  return std::make_pair(setUpContextConditions, customInstrumentOptions);
}

void ALFCustomInstrumentPresenter::extractSingleTube() {
  m_model->extractSingleTube();
  const std::string WSName = m_model->WSName();
  m_analysisPane->addSpectrum(WSName);
}

void ALFCustomInstrumentPresenter::averageTube() {
  m_model->averageTube();
  const std::string WSName = m_model->WSName();
  m_analysisPane->addSpectrum(WSName);
}

} // namespace CustomInterfaces
} // namespace MantidQt
