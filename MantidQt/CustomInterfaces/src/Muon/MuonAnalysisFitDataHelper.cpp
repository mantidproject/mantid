#include "MantidQtCustomInterfaces/Muon/MuonAnalysisFitDataHelper.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h"
#include "MantidAPI/AnalysisDataService.h"

using MantidQt::MantidWidgets::IMuonFitDataSelector;
using MantidQt::MantidWidgets::IWorkspaceFitControl;
using Mantid::API::AnalysisDataService;

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Constructor
 * @param fitBrowser :: [input] Pointer to fit browser to update
 * @param dataSelector :: [input] Pointer to data selector to get input from
 */
MuonAnalysisFitDataHelper::MuonAnalysisFitDataHelper(
    IWorkspaceFitControl *fitBrowser, IMuonFitDataSelector *dataSelector)
    : m_fitBrowser(fitBrowser), m_dataSelector(dataSelector) {}

/**
 * Called when data selector reports "data properties changed"
 * Updates WS index, startX, endX
 */
void MuonAnalysisFitDataHelper::handleDataPropertiesChanged() {
  // update workspace index
  const unsigned int wsIndex = m_dataSelector->getWorkspaceIndex();
  m_fitBrowser->setWorkspaceIndex(static_cast<int>(wsIndex));

  // update start and end times
  const double start = m_dataSelector->getStartTime();
  const double end = m_dataSelector->getEndTime();
  m_fitBrowser->setStartX(start);
  m_fitBrowser->setEndX(end);
}

/**
 * Called when data selector reports "selected groups changed"
 */
void MuonAnalysisFitDataHelper::handleSelectedGroupsChanged() {
  createWorkspacesToFit();
}

/**
 * Called when data selector reports "selected periods changed"
 */
void MuonAnalysisFitDataHelper::handleSelectedPeriodsChanged() {
  createWorkspacesToFit();
}

/**
 * Called when user drags lines to set fit range
 * Update the text boxes silently (no event)
 * @param start :: [input] start of fit range
 * @param end :: [input] end of fit range
 */
void MuonAnalysisFitDataHelper::handleXRangeChangedGraphically(double start,
                                                               double end) {
  m_dataSelector->setStartTimeQuietly(start);
  m_dataSelector->setEndTimeQuietly(end);
}

/**
 * Called by selectMultiPeak: fit browser has been reassigned to a new
 * workspace.
 * Sets run number, instrument and workspace index in the data selector.
 * If multiple runs selected, disable sequential fit option.
 * @param wsName :: [input] Name of new workspace
 */
void MuonAnalysisFitDataHelper::setAssignedFirstRun(const QString &wsName) {
  if (wsName == m_PPAssignedFirstRun)
    return;

  m_PPAssignedFirstRun = wsName;
  // Parse workspace name here for run number and instrument name
  const auto wsParams =
      MuonAnalysisHelper::parseWorkspaceName(wsName.toStdString());
  const QString instRun = QString::fromStdString(wsParams.label);
  const int firstZero = instRun.indexOf("0");
  const QString numberString = instRun.right(instRun.size() - firstZero);
  m_dataSelector->setWorkspaceDetails(
      numberString, QString::fromStdString(wsParams.instrument));
  m_dataSelector->setWorkspaceIndex(0u); // always has only one spectrum
  // Check for multiple runs
  if (wsParams.runs.size() > 1) {
    m_fitBrowser->allowSequentialFits(false);
  } else {
    m_fitBrowser->allowSequentialFits(
        true); // will still be forbidden if no function
  }

  // Set selected groups/pairs and periods here too
  m_dataSelector->setChosenGroup(QString::fromStdString(wsParams.itemName));
  m_dataSelector->setChosenPeriod(QString::fromStdString(wsParams.periods));
}

/**
 * Called when runs are changed in data selector
 * Creates workspace and sets it in the fit browser
 * The signal from that will set peak picker and UI properties
 */
void MuonAnalysisFitDataHelper::handleDataWorkspaceChanged() {
  createWorkspacesToFit();
}

/**
 * Gets names of all workspaces needed from the view and updates the
 * model (fit browser) with these. Creates all workspaces that don't
 * yet exist in the ADS and adds them. Sets the workspace name, which
 * sends a signal to update the peak picker.
 */
void MuonAnalysisFitDataHelper::createWorkspacesToFit() {
  // From view, get names of all workspaces needed
  std::vector<std::string> workspaces;
  const auto filenames = m_dataSelector->getFilenames();
  const auto groups = m_dataSelector->getChosenGroups();
  const auto periods = m_dataSelector->getPeriodSelections();

  Muon::DatasetParams params;
  QString instRuns = m_dataSelector->getInstrumentName();
  instRuns.append(m_dataSelector->getRuns());
  std::vector<int> selectedRuns;
  MuonAnalysisHelper::parseRunLabel(instRuns.toStdString(), params.instrument,
                                    selectedRuns);

  // We need to know if the runs are sequential (loop over selectedRuns) or
  // co-added (use whole run string at once)

  params.version = 1; // ???
  //params.plotType =  // ???

  // generate workspace names
  //for (const int run : selectedRuns) {
    for (const auto &group : groups) {
      //params.itemType = group? pair? how to tell?
      params.itemName = group.toStdString();
      for (const auto &period : periods) {
        params.periods = period.toStdString();
        workspaces.push_back(MuonAnalysisHelper::generateWorkspaceName(params));
      }
    }
  //}


  // Update model with these
  //m_fitBrowser->... (m_workspacesToFit)

  // For each name, if not in the ADS, create and add it
  for (const auto &workspace : workspaces) {
    if (!AnalysisDataService::Instance().doesExist(workspace)) {
      //AnalysisDataService::Instance().add(workspace, ...)
    }
  }
   
  // NB This is necessary to set peak picker, UI properties via signal!
  if (!workspaces.empty()) {
    m_fitBrowser->setWorkspaceName(QString::fromStdString(workspaces.front()));
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
