#include "MantidQtCustomInterfaces/Reflectometry/ReflSettingsTabPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsTabView.h"
#include "MantidQtMantidWidgets/AlgorithmHintStrategy.h"

#include <boost/algorithm/string.hpp>

namespace MantidQt {
namespace CustomInterfaces {

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

/** Constructor
* @param view :: The view we are handling
*/
ReflSettingsTabPresenter::ReflSettingsTabPresenter(IReflSettingsTabView *view)
    : m_view(view), m_mainPresenter() {

  // Create the 'HintingLineEdits'
  createPlusHints();
  createTransmissionHints();
  createReductionHints();
  createStitchHints();
}

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

/** Returns global options for 'Plus' algorithm
* @return :: Global options for 'Plus' algorithm
*/
std::string ReflSettingsTabPresenter::getPlusOptions() const {

  return m_view->getPlusOptions();
}

/** Returns global options for 'CreateTransmissionWorkspaceAuto'
* @return :: Global options for 'CreateTransmissionWorkspaceAuto'
*/
std::string ReflSettingsTabPresenter::getTransmissionOptions() const {

  std::vector<std::string> options;

  // Global options
  auto globalOptions = m_view->getTransmissionOptions();
  if (!globalOptions.empty())
    options.push_back(globalOptions);

  // Add analysis mode
  auto analysisMode = m_view->getAnalysisMode();
  if (!analysisMode.empty())
    options.push_back("AnalysisMode=" + analysisMode);

  return boost::algorithm::join(options, ",");
}

/** Returns global options for 'ReflectometryReductionOneAuto'
* @return :: Global options for 'ReflectometryReductionOneAuto'
*/
std::string ReflSettingsTabPresenter::getReductionOptions() const {

  std::vector<std::string> options;

  // Global options
  auto globalOptions = m_view->getReductionOptions();
  if (!globalOptions.empty())
    options.push_back(globalOptions);

  // Add analysis mode
  auto analysisMode = m_view->getAnalysisMode();
  if (!analysisMode.empty())
	  options.push_back("AnalysisMode=" + analysisMode);

  // Add CRho
  auto crho = m_view->getCRho();
  if (!crho.empty())
    options.push_back("CRho=" + crho);

  // Add CAlpha
  auto calpha = m_view->getCAlpha();
  if (!calpha.empty())
	  options.push_back("CAlpha=" + calpha);

  // Add CAp
  auto cap = m_view->getCAp();
  if (!cap.empty())
    options.push_back("CAp=" + cap);

  // Add CPp
  auto cpp = m_view->getCPp();
  if (!cpp.empty())
	  options.push_back("CPp=" + cpp);

  // Add direct beam
  auto dbnr = m_view->getDirectBeam();
  if (!dbnr.empty())
    options.push_back("RegionOfDirectBeam=" + dbnr);

  // Add polarisation corrections
  auto polCorr = m_view->getPolarisationCorrections();
  if (!polCorr.empty())
    options.push_back("PolarizationAnalysis=" + polCorr);

  return boost::algorithm::join(options, ",");
}

/** Returns global options for 'Stitch1DMany'
* @return :: Global options for 'Stitch1DMany'
*/
std::string ReflSettingsTabPresenter::getStitchOptions() const {

  return m_view->getStitchOptions();
}

/** Creates hints for 'Plus'
*/
void ReflSettingsTabPresenter::createPlusHints() {

  // The algorithm
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Plus");
  // The blacklist
  std::set<std::string> blacklist = {"LHSWorkspace", "RHSWorkspace",
                                     "OutputWorkspace"};
  AlgorithmHintStrategy strategy(alg, blacklist);

  m_view->createPlusHints(strategy.createHints());
}

/** Creates hints for 'CreateTransmissionWorkspaceAuto'
*/
void ReflSettingsTabPresenter::createTransmissionHints() {

  // The algorithm
  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().create("CreateTransmissionWorkspaceAuto");
  // The blacklist
  std::set<std::string> blacklist = {
      "FirstTransmissionRun", "SecondTransmissionRun", "OutputWorkspace"};
  AlgorithmHintStrategy strategy(alg, blacklist);

  m_view->createTransmissionHints(strategy.createHints());
}

/** Creates hints for 'ReflectometryReductionOneAuto'
*/
void ReflSettingsTabPresenter::createReductionHints() {

  // The algorithm
  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
  // The blacklist
  std::set<std::string> blacklist = {
      "ThetaIn", "ThetaOut", "InputWorkspace", "OutputWorkspace",
      "OutputWorkspaceWavelength", "FirstTransmissionRun",
      "SecondTransmissionRun", "MomentumTransferMinimum",
      "MomentumTransferMaximum", "MomentumTransferStep", "ScaleFactor"};
  AlgorithmHintStrategy strategy(alg, blacklist);

  m_view->createReductionHints(strategy.createHints());
}

/** Creates hints for 'Stitch1DMany'
*/
void ReflSettingsTabPresenter::createStitchHints() {

  // The algorithm
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Stitch1DMany");
  // The blacklist
  std::set<std::string> blacklist = {"InputWorkspaces", "OutputWorkspace",
                                     "OutputWorkspace"};
  AlgorithmHintStrategy strategy(alg, blacklist);

  m_view->createStitchHints(strategy.createHints());
}
}
}