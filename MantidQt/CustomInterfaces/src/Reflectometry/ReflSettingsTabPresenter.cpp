#include "MantidQtCustomInterfaces/Reflectometry/ReflSettingsTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsTabView.h"

#include <boost/algorithm/string.hpp>

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param view :: The view we are handling
*/
ReflSettingsTabPresenter::ReflSettingsTabPresenter(IReflSettingsTabView *view)
    : m_view(view), m_mainPresenter() {}

/** Destructor
*/
ReflSettingsTabPresenter::~ReflSettingsTabPresenter() {}

/** Accept a main presenter
* @param mainPresenter :: [input] The main presenter
*/
void ReflSettingsTabPresenter::acceptMainPresenter(
    IReflMainWindowPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

/** Get global pre-processing options
* @return :: Pre-processing options as a map where keys are column names that
* need pre-processing and values pre-processing options
*/
std::map<std::string, std::string>
ReflSettingsTabPresenter::getPreprocessingOptions() const {

  // For the moment, return emtpy map
  return std::map<std::string, std::string>();
}

/** Get global processing options
* @return :: Processing options as a string
*/
std::string ReflSettingsTabPresenter::getProcessingOptions() const {

  std::vector<std::string> options;

  // Analysis mode
  options.push_back("AnalysisMode=" + m_view->getAnalysisMode());

  return boost::algorithm::join(options, ",");
}

/** Get global post-processing options
* @return :: Post-processing options as a string
*/
std::string ReflSettingsTabPresenter::getPostprocessingOptions() const {

  std::vector<std::string> options;

  // Resolution (dQ/Q)
  options.push_back("Params=\"" + m_view->getResolution());

  return boost::algorithm::join(options, ",");
}
}
}