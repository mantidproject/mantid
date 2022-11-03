// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFCustomInstrumentPresenter.h"
#include "ALFCustomInstrumentView.h"
#include "MantidAPI/FileFinder.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentModel.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"

#include <functional>
#include <tuple>

namespace MantidQt::CustomInterfaces {

ALFCustomInstrumentPresenter::ALFCustomInstrumentPresenter(IALFCustomInstrumentView *view,
                                                           MantidWidgets::IBaseCustomInstrumentModel *model)
    : BaseCustomInstrumentPresenter(view, model), m_view(view), m_model(model) {
  addInstrument();
}

void ALFCustomInstrumentPresenter::addInstrument() {
  auto setUp = setupALFInstrument();
  initLayout(&setUp);
}

using instrumentSetUp = std::pair<std::string, std::vector<std::function<bool(std::map<std::string, bool>)>>>;
using instrumentObserverOptions = std::vector<std::tuple<std::string, Observer *>>;

/**
* This creates the custom instrument widget
* @return <instrumentSetUp,
    instrumentObserverOptions> : a pair of the conditions and observers
*/
std::pair<instrumentSetUp, instrumentObserverOptions> ALFCustomInstrumentPresenter::setupALFInstrument() {
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

} // namespace MantidQt::CustomInterfaces
