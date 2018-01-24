#include "ReflSettingsTabPresenter.h"
#include "IReflMainWindowPresenter.h"
#include "ReflSettingsPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmHintStrategy.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <boost/algorithm/string.hpp>

using namespace MantidQt::MantidWidgets::DataProcessor;

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
*
* @param presenters :: The presenters of each group as a vector
*/
ReflSettingsTabPresenter::ReflSettingsTabPresenter(
    std::vector<IReflSettingsPresenter *> presenters)
    : m_settingsPresenters(presenters) {
  passSelfToChildren(presenters);
}

void ReflSettingsTabPresenter::passSelfToChildren(
    std::vector<IReflSettingsPresenter *> const &children) {
  for (auto *presenter : children)
    presenter->acceptTabPresenter(this);
}

void ReflSettingsTabPresenter::acceptMainPresenter(
    IReflMainWindowPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

void ReflSettingsTabPresenter::settingsChanged(int group) {
  m_mainPresenter->settingsChanged(group);
}

/// Destructor
ReflSettingsTabPresenter::~ReflSettingsTabPresenter() {}

/** Sets the current instrument name and changes accessibility status of
* the polarisation corrections option in the view accordingly
*
* @param instName :: [input] The name of the instrument to set to
*/
void ReflSettingsTabPresenter::setInstrumentName(const std::string &instName) {
  for (auto presenter : m_settingsPresenters)
    presenter->setInstrumentName(instName);
}

/** Returns values passed for 'Transmission run(s)'
*
* @param group :: The group from which to get the values
* @return :: Values passed for 'Transmission run(s)'
*/
std::string ReflSettingsTabPresenter::getTransmissionRuns(int group) const {

  return m_settingsPresenters.at(group)->getTransmissionRuns();
}

/** Returns global options for 'CreateTransmissionWorkspaceAuto'
*
* @param group :: The group from which to get the options
* @return :: Global options for 'CreateTransmissionWorkspaceAuto'
*/
OptionsQMap ReflSettingsTabPresenter::getTransmissionOptions(int group) const {

  return m_settingsPresenters.at(group)->getTransmissionOptions();
}

/** Returns global options for 'ReflectometryReductionOneAuto'
*
* @param group :: The group from which to get the options
* @return :: Global options for 'ReflectometryReductionOneAuto'
*/
OptionsQMap ReflSettingsTabPresenter::getReductionOptions(int group) const {

  return m_settingsPresenters.at(group)->getReductionOptions();
}

/** Returns global options for 'Stitch1DMany'
*
* @param group :: The group from which to get the options
* @return :: Global options for 'Stitch1DMany'
*/
std::string ReflSettingsTabPresenter::getStitchOptions(int group) const {

  return m_settingsPresenters.at(group)->getStitchOptions();
}
}
}
