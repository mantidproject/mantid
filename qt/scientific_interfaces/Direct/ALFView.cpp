// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFView_model.h"
#include "ALFView_presenter.h"

#include "BaseInstrumentModel.h"
#include "BaseInstrumentView.h"

// will need these later
#include "MantidQtWidgets/Common/FunctionBrowser.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"

#include "ALFView.h"

#include <tuple>

namespace MantidQt {
namespace CustomInterfaces {

DECLARE_SUBWINDOW(ALFView)

/// static logger
Mantid::Kernel::Logger g_log("ALFView");

ALFView::ALFView(QWidget *parent)
    : UserSubWindow(parent), m_view(nullptr), m_presenter(nullptr),
      m_extractSingleTubeObserver(nullptr), m_averageTubeObserver(nullptr) {
  m_model = new ALFView_model();
  m_view = new ALFView_view(m_model->getInstrument(), this);
  m_presenter = new ALFView_presenter(m_view, m_model);
}

void ALFView::initLayout() {
  this->setCentralWidget(m_view);
  m_extractSingleTubeObserver = new VoidObserver();
  m_averageTubeObserver = new VoidObserver();
  auto setUp = initInstrument();

  m_presenter->initLayout(&setUp);
}

typedef std::pair<std::string,
                   std::vector<std::function<bool(std::map<std::string, bool>)>>>
    instrumentSetUp;
typedef std::vector<std::tuple<std::string, Observer *>>
    instrumentObserverOptions;

/**
* This creates the custom instrument widget
* @return <instrumentSetUp,
    instrumentObserverOptions> : a pair of the
*/
std::pair<instrumentSetUp, instrumentObserverOptions>
ALFView::initInstrument() {

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
	 
  setUpContextConditions =
      std::make_pair(m_model->dataFileName(),binders);
  

  // set up single tube extract
  std::function<void()> extractSingleTubeBinder =
      std::bind(&ALFView_model::extractSingleTube, m_model); // binder for slot
  m_extractSingleTubeObserver->setSlot(
      extractSingleTubeBinder); // add slot to observer
  std::tuple<std::string, Observer *> tmp = std::make_tuple(
      "singleTube", m_extractSingleTubeObserver); // store observer for later
  customInstrumentOptions.push_back(tmp);

  // set up average tube
  std::function<void()> averageTubeBinder =
      std::bind(&ALFView_model::averageTube, m_model);
  m_averageTubeObserver->setSlot(averageTubeBinder);
  tmp = std::make_tuple("averageTube", m_averageTubeObserver);
  customInstrumentOptions.push_back(tmp);
  
  return std::make_pair(setUpContextConditions, customInstrumentOptions);
  
  }

} // namespace CustomInterfaces
} // namespace MantidQt
