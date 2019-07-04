// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "CorrectionsTab.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QSettings>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
/**
 * Constructor.
 *
 * @param parent :: the parent widget
 */
CorrectionsTab::CorrectionsTab(QWidget *parent)
    : IndirectTab(parent), m_dblEdFac(nullptr), m_blnEdFac(nullptr) {
  // Create Editor Factories
  m_dblEdFac = new DoubleEditorFactory(this);
  m_blnEdFac = new QtCheckBoxFactory(this);
}

/**
 * Loads the tab's settings.
 *
 * Calls overridden version of loadSettings() in child class.
 *
 * @param settings :: the QSettings object from which to load
 */
void CorrectionsTab::loadTabSettings(const QSettings &settings) {
  loadSettings(settings);
}

/**
 * Prevents the loading of data with incorrect naming if passed true
 *
 * @param filter :: true if you want to allow filtering
 */
void CorrectionsTab::filterInputData(bool filter) {
  setFileExtensionsByName(filter);
}

/**
 * Allows the user to turn the plotting of error bars off and on
 *
 * @param errorBars :: true if you want output plots to have error bars
 */
void CorrectionsTab::setPlotErrorBars(bool errorBars) {
  IndirectTab::setPlotErrorBars(errorBars);
}

/**
 * Slot that can be called when a user edits an input.
 */
void CorrectionsTab::inputChanged() { validate(); }

/**
 * Check that the binning between two workspaces matches.
 *
 * @param left :: left hand workspace for the equality operator
 * @param right :: right hand workspace for the equality operator
 * @return whether the binning matches
 * @throws std::runtime_error if one of the workspaces is an invalid pointer
 */
bool CorrectionsTab::checkWorkspaceBinningMatches(
    MatrixWorkspace_const_sptr left, MatrixWorkspace_const_sptr right) {
  if (left && right) // check the workspaces actually point to something first
  {
    const auto &leftX = left->x(0);
    const auto &rightX = right->x(0);
    return leftX.size() == rightX.size() &&
           std::equal(leftX.begin(), leftX.end(), rightX.begin());
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
boost::optional<std::string> CorrectionsTab::addConvertUnitsStep(
    MatrixWorkspace_sptr ws, std::string const &unitID,
    std::string const &suffix, std::string eMode, double eFixed) {
  std::string outputName = ws->getName();

  if (suffix != "UNIT")
    outputName += suffix;
  else
    outputName += "_" + unitID;

  IAlgorithm_sptr convertAlg =
      AlgorithmManager::Instance().create("ConvertUnits");
  convertAlg->initialize();

  convertAlg->setProperty("InputWorkspace", ws->getName());
  convertAlg->setProperty("OutputWorkspace", outputName);
  convertAlg->setProperty("Target", unitID);

  if (eMode.empty())
    eMode = getEMode(ws);

  convertAlg->setProperty("EMode", eMode);

  if (eMode == "Indirect" && eFixed == 0.0) {
    try {
      eFixed = getEFixed(ws);
    } catch (std::exception const &) {
      showMessageBox(
          "An Efixed value could not be found, please enter an Efixed value.");
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
void CorrectionsTab::displayInvalidWorkspaceTypeError(
    const std::string &workspaceName, Mantid::Kernel::Logger &log) {
  QString errorMessage =
      "Invalid workspace loaded, ensure a MatrixWorkspace is "
      "entered into the field.\n";

  if (AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
          workspaceName)) {
    errorMessage += "Consider loading the WorkspaceGroup first into mantid, "
                    "and then choose one of its items here.\n";
    log.error() << "Workspace Groups are currently not allowed.\n";
  } else {
    log.error() << "Workspace " << workspaceName
                << " is not a MatrixWorkspace.\n";
  }
  emit showMessageBox(errorMessage);
}

} // namespace CustomInterfaces
} // namespace MantidQt
