// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "CorrectionsTab.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"

#include <QSettings>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets::WorkspaceUtils;

namespace MantidQt::CustomInterfaces {
/**
 * Constructor.
 *
 * @param parent :: the parent widget
 */
CorrectionsTab::CorrectionsTab(QWidget *parent) : InelasticTab(parent), m_dblEdFac(nullptr), m_blnEdFac(nullptr) {
  // Create Editor Factories
  m_dblEdFac = new DoubleEditorFactory(this);
  m_blnEdFac = new QtCheckBoxFactory(this);
}

void CorrectionsTab::setOutputPlotOptionsPresenter(std::unique_ptr<OutputPlotOptionsPresenter> presenter) {
  m_plotOptionsPresenter = std::move(presenter);
}

void CorrectionsTab::setOutputPlotOptionsWorkspaces(std::vector<std::string> const &outputWorkspaces) {
  m_plotOptionsPresenter->setWorkspaces(outputWorkspaces);
}

void CorrectionsTab::clearOutputPlotOptionsWorkspaces() { m_plotOptionsPresenter->clearWorkspaces(); }

/**
 * Loads the tab's settings.
 *
 * Calls overridden version of loadSettings() in child class.
 *
 * @param settings :: the QSettings object from which to load
 */
void CorrectionsTab::loadTabSettings(const QSettings &settings) { loadSettings(settings); }

/**
 * Prevents the loading of data with incorrect naming if passed true
 *
 * @param filter :: true if you want to allow filtering
 */
void CorrectionsTab::filterInputData(bool filter) { setFileExtensionsByName(filter); }

void CorrectionsTab::enableLoadHistoryProperty(bool doLoadHistory) { setLoadHistory(doLoadHistory); }

/**
 * Check that the binning between two workspaces matches.
 *
 * @param left :: left hand workspace for the equality operator
 * @param right :: right hand workspace for the equality operator
 * @return whether the binning matches
 * @throws std::runtime_error if one of the workspaces is an invalid pointer
 */
bool CorrectionsTab::checkWorkspaceBinningMatches(const MatrixWorkspace_const_sptr &left,
                                                  const MatrixWorkspace_const_sptr &right) {
  if (left && right) // check the workspaces actually point to something first
  {
    const auto &leftX = left->x(0);
    const auto &rightX = right->x(0);
    return leftX.size() == rightX.size() && std::equal(leftX.begin(), leftX.end(), rightX.begin());
  } else {
    throw std::runtime_error("CorrectionsTab: One of the operands is an "
                             "invalid MatrixWorkspace pointer");
  }
}

/**
 * Adds a unit converstion step to the batch algorithm queue.
 *
 * Note that if converting diffraction data in wavelength then eMode must be
 *set.
 *
 * @param ws Pointer to the workspace to convert
 * @param unitID ID of unit to convert to
 * @param suffix Suffix to append to output workspace name
 * @param eMode Emode to use (if not set will determine based on current X unit)
 * @return Name of output workspace
 */
boost::optional<std::string> CorrectionsTab::addConvertUnitsStep(const MatrixWorkspace_sptr &ws,
                                                                 std::string const &unitID, std::string const &suffix,
                                                                 std::string eMode, double eFixed) {
  std::string outputName = ws->getName();

  if (suffix != "UNIT")
    outputName += suffix;
  else
    outputName += "_" + unitID;

  IAlgorithm_sptr convertAlg = AlgorithmManager::Instance().create("ConvertUnits");
  convertAlg->initialize();

  convertAlg->setProperty("InputWorkspace", ws->getName());
  convertAlg->setProperty("OutputWorkspace", outputName);
  convertAlg->setProperty("Target", unitID);

  if (eMode.empty())
    eMode = getEMode(ws);

  convertAlg->setProperty("EMode", eMode);

  if (eMode == "Indirect" && eFixed == 0.0) {
    if (auto const eFixedFromWs = getEFixed(ws)) {
      eFixed = *eFixedFromWs;
    } else {
      showMessageBox("Please enter an Efixed value.");
      return boost::none;
    }
  }

  if (eMode == "Indirect")
    convertAlg->setProperty("EFixed", eFixed);

  m_batchAlgoRunner->addAlgorithm(convertAlg);

  return outputName;
}

/*
 * Displays and logs an invalid workspace type error, for the workspace
 * with the specified name.
 *
 * @param workspaceName The name of the workspace.
 * @param log           The logger for sending log messages.
 */
void CorrectionsTab::displayInvalidWorkspaceTypeError(const std::string &workspaceName, Mantid::Kernel::Logger &log) {
  std::string errorMessage = "Invalid workspace loaded, ensure a MatrixWorkspace is "
                             "entered into the field.\n";

  if (AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(workspaceName)) {
    errorMessage += "Consider loading the WorkspaceGroup first into mantid, "
                    "and then choose one of its items here.\n";
    log.error() << "Workspace Groups are currently not allowed.\n";
  } else {
    log.error() << "Workspace " << workspaceName << " is not a MatrixWorkspace.\n";
  }
  emit showMessageBox(errorMessage);
}

} // namespace MantidQt::CustomInterfaces
