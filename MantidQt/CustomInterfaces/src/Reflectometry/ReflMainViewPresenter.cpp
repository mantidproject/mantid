#include "MantidQtCustomInterfaces/Reflectometry/ReflMainViewPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NotebookWriter.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/CatalogInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UserCatalogInfo.h"
#include "MantidKernel/Utils.h"
#include "MantidQtCustomInterfaces/ParseKeyValueString.h"
#include "MantidQtCustomInterfaces/Reflectometry/QtReflOptionsDialog.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflCatalogSearcher.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflGenerateNotebook.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflLegacyTransferStrategy.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMainView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMeasureTransferStrategy.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflNexusMeasurementItemSource.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSearchModel.h"
#include "MantidQtMantidWidgets/AlgorithmHintStrategy.h"

#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <sstream>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace CustomInterfaces {
ReflMainViewPresenter::ReflMainViewPresenter(
    ReflMainView *mainView, boost::shared_ptr<IReflSearcher> searcher)
    : WorkspaceObserver(), m_view(mainView), m_searcher(searcher) {

  // TODO. Select strategy.
  /*
  std::unique_ptr<CatalogConfigService> catConfigService(
  makeCatalogConfigServiceAdapter(ConfigService::Instance()));
  UserCatalogInfo catalogInfo(
  ConfigService::Instance().getFacility().catalogInfo(), *catConfigService);
  */

  // If we don't have a searcher yet, use ReflCatalogSearcher
  if (!m_searcher)
    m_searcher.reset(new ReflCatalogSearcher());

  // Set the possible tranfer methods
  std::set<std::string> methods;
  methods.insert(LegacyTransferMethod);
  methods.insert(MeasureTransferMethod);
  m_view->setTransferMethods(methods);
}

ReflMainViewPresenter::~ReflMainViewPresenter() {}

/**
Calculates the minimum and maximum values for Q
@param ws : The workspace to fetch the instrument values from
@param theta : The value of two theta to use in calculations
*/
std::vector<double> ReflMainViewPresenter::calcQRange(Workspace_sptr ws,
                                                      double theta) {
  auto mws = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
  auto wsg = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);

  // If we've got a workspace group, use the first workspace in it
  if (!mws && wsg)
    mws = boost::dynamic_pointer_cast<MatrixWorkspace>(wsg->getItem(0));

  if (!mws)
    throw std::runtime_error("Could not convert " + ws->name() +
                             " to a MatrixWorkspace.");

  double lmin, lmax;
  try {
    const Instrument_const_sptr instrument = mws->getInstrument();
    lmin = instrument->getNumberParameter("LambdaMin")[0];
    lmax = instrument->getNumberParameter("LambdaMax")[0];
  } catch (std::exception &) {
    throw std::runtime_error("LambdaMin/LambdaMax instrument parameters are "
                             "required to calculate qmin/qmax");
  }

  double qmin = 4 * M_PI / lmax * sin(theta * M_PI / 180.0);
  double qmax = 4 * M_PI / lmin * sin(theta * M_PI / 180.0);

  if (m_options["RoundQMin"].toBool())
    qmin = Utils::roundToDP(qmin, m_options["RoundQMinPrecision"].toInt());

  if (m_options["RoundQMax"].toBool())
    qmax = Utils::roundToDP(qmax, m_options["RoundQMaxPrecision"].toInt());

  std::vector<double> ret;
  ret.push_back(qmin);
  ret.push_back(qmax);
  return ret;
}

/**
Stitches the workspaces created by the given rows together.
@param rows : the list of rows
*/
void ReflMainViewPresenter::stitchRows(std::set<int> rows) {
  // If we can get away with doing nothing, do.
  if (rows.size() < 2)
    return;

  // Properties for Stitch1DMany
  std::vector<std::string> workspaceNames;
  std::vector<std::string> runs;

  std::vector<double> params;
  std::vector<double> startOverlaps;
  std::vector<double> endOverlaps;

  // Go through each row and prepare the properties
  for (auto rowIt = rows.begin(); rowIt != rows.end(); ++rowIt) {
    const std::string runStr =
        m_model->data(m_model->index(*rowIt, ReflTableSchema::COL_RUNS))
            .toString()
            .toStdString();
    const double qmin =
        m_model->data(m_model->index(*rowIt, ReflTableSchema::COL_QMIN))
            .toDouble();
    const double qmax =
        m_model->data(m_model->index(*rowIt, ReflTableSchema::COL_QMAX))
            .toDouble();

    Workspace_sptr runWS = prepareRunWorkspace(runStr);
    if (runWS) {
      const std::string runNo = getRunNumber(runWS);
      if (AnalysisDataService::Instance().doesExist("IvsQ_" + runNo)) {
        runs.push_back(runNo);
        workspaceNames.emplace_back("IvsQ_" + runNo);
      }
    }

    startOverlaps.push_back(qmin);
    endOverlaps.push_back(qmax);
  }

  double dqq =
      m_model->data(m_model->index(*(rows.begin()), ReflTableSchema::COL_DQQ))
          .toDouble();

  // params are qmin, -dqq, qmax for the final output
  params.push_back(
      *std::min_element(startOverlaps.begin(), startOverlaps.end()));
  params.push_back(-dqq);
  params.push_back(*std::max_element(endOverlaps.begin(), endOverlaps.end()));

  // startOverlaps and endOverlaps need to be slightly offset from each other
  // See usage examples of Stitch1DMany to see why we discard first qmin and
  // last qmax
  startOverlaps.erase(startOverlaps.begin());
  endOverlaps.pop_back();

  std::string outputWSName = "IvsQ_" + boost::algorithm::join(runs, "_");

  // If the previous stitch result is in the ADS already, we'll need to remove
  // it.
  // If it's a group, we'll get an error for trying to group into a used group
  // name
  if (AnalysisDataService::Instance().doesExist(outputWSName))
    AnalysisDataService::Instance().remove(outputWSName);

  IAlgorithm_sptr algStitch =
      AlgorithmManager::Instance().create("Stitch1DMany");
  algStitch->initialize();
  algStitch->setProperty("InputWorkspaces", workspaceNames);
  algStitch->setProperty("OutputWorkspace", outputWSName);
  algStitch->setProperty("Params", params);
  algStitch->setProperty("StartOverlaps", startOverlaps);
  algStitch->setProperty("EndOverlaps", endOverlaps);

  algStitch->execute();

  if (!algStitch->isExecuted())
    throw std::runtime_error("Failed to run Stitch1DMany on IvsQ workspaces.");
}

/**
Create a transmission workspace
@param transString : the numbers of the transmission runs to use
*/
Workspace_sptr
ReflMainViewPresenter::makeTransWS(const std::string &transString) {
  const size_t maxTransWS = 2;

  std::vector<std::string> transVec;
  std::vector<Workspace_sptr> transWSVec;

  // Take the first two run numbers
  boost::split(transVec, transString, boost::is_any_of(","));
  if (transVec.size() > maxTransWS)
    transVec.resize(maxTransWS);

  if (transVec.size() == 0)
    throw std::runtime_error("Failed to parse the transmission run list.");

  for (auto it = transVec.begin(); it != transVec.end(); ++it)
    transWSVec.push_back(loadRun(*it, m_tableView->getProcessInstrument()));

  // If the transmission workspace is already in the ADS, re-use it
  std::string lastName = "TRANS_" + boost::algorithm::join(transVec, "_");
  if (AnalysisDataService::Instance().doesExist(lastName))
    return AnalysisDataService::Instance().retrieveWS<Workspace>(lastName);

  // We have the runs, so we can create a TransWS
  IAlgorithm_sptr algCreateTrans =
      AlgorithmManager::Instance().create("CreateTransmissionWorkspaceAuto");
  algCreateTrans->initialize();
  algCreateTrans->setProperty("FirstTransmissionRun", transWSVec[0]->name());
  if (transWSVec.size() > 1)
    algCreateTrans->setProperty("SecondTransmissionRun", transWSVec[1]->name());

  std::string wsName = "TRANS_" + getRunNumber(transWSVec[0]);
  if (transWSVec.size() > 1)
    wsName += "_" + getRunNumber(transWSVec[1]);

  algCreateTrans->setProperty("OutputWorkspace", wsName);

  if (!algCreateTrans->isInitialized())
    throw std::runtime_error(
        "Could not initialize CreateTransmissionWorkspaceAuto");

  algCreateTrans->execute();

  if (!algCreateTrans->isExecuted())
    throw std::runtime_error(
        "CreateTransmissionWorkspaceAuto failed to execute");

  return AnalysisDataService::Instance().retrieveWS<Workspace>(wsName);
}

/**
Used by the view to tell the presenter something has changed
*/
void ReflMainViewPresenter::notify(IReflPresenter::Flag flag) {
  switch (flag) {
  case IReflPresenter::SearchFlag:
    search();
    break;
  case IReflPresenter::ICATSearchCompleteFlag: {
    auto algRunner = m_view->getAlgorithmRunner();
    IAlgorithm_sptr searchAlg = algRunner->getAlgorithm();
    populateSearch(searchAlg);
  } break;
  case IReflPresenter::TransferFlag:
    transfer();
    break;
  }
  // Not having a 'default' case is deliberate. gcc issues a warning if there's
  // a flag we aren't handling.
}

/** Searches for runs that can be used */
void ReflMainViewPresenter::search() {
  const std::string searchString = m_view->getSearchString();
  // Don't bother searching if they're not searching for anything
  if (searchString.empty())
    return;

  // This is breaking the abstraction provided by IReflSearcher, but provides a
  // nice usability win
  // If we're not logged into a catalog, prompt the user to do so
  if (CatalogManager::Instance().getActiveSessions().empty()) {
    try {
      m_view->showAlgorithmDialog("CatalogLogin");
    } catch (std::runtime_error &e) {
      m_view->giveUserCritical("Error Logging in:\n" + std::string(e.what()),
                               "login failed");
    }
  }
  std::string sessionId;
  // check to see if we have any active sessions for ICAT
  if (!CatalogManager::Instance().getActiveSessions().empty()) {
    // we have an active session, so grab the ID
    sessionId =
        CatalogManager::Instance().getActiveSessions().front()->getSessionId();
  } else {
    // there are no active sessions, we return here to avoid an exception
    m_view->giveUserInfo(
        "Error Logging in: Please press 'Search' to try again.",
        "Login Failed");
    return;
  }
  auto algSearch = AlgorithmManager::Instance().create("CatalogGetDataFiles");
  algSearch->initialize();
  algSearch->setChild(true);
  algSearch->setLogging(false);
  algSearch->setProperty("Session", sessionId);
  algSearch->setProperty("InvestigationId", searchString);
  algSearch->setProperty("OutputWorkspace", "_ReflSearchResults");
  auto algRunner = m_view->getAlgorithmRunner();
  algRunner->startAlgorithm(algSearch);
}

void ReflMainViewPresenter::populateSearch(IAlgorithm_sptr searchAlg) {
  if (searchAlg->isExecuted()) {
    ITableWorkspace_sptr results = searchAlg->getProperty("OutputWorkspace");
    m_searchModel = ReflSearchModel_sptr(new ReflSearchModel(
        *getTransferStrategy(), results, m_view->getSearchInstrument()));
    m_view->showSearch(m_searchModel);
  }
}

/** Transfers the selected runs in the search results to the processing table */
void ReflMainViewPresenter::transfer() {
  // Build the input for the transfer strategy
  SearchResultMap runs;
  auto selectedRows = m_view->getSelectedSearchRows();

  for (auto rowIt = selectedRows.begin(); rowIt != selectedRows.end();
       ++rowIt) {
    const int row = *rowIt;
    const std::string run = m_searchModel->data(m_searchModel->index(row, 0))
                                .toString()
                                .toStdString();
    SearchResult searchResult;

    searchResult.description = m_searchModel->data(m_searchModel->index(row, 1))
                                   .toString()
                                   .toStdString();

    searchResult.location = m_searchModel->data(m_searchModel->index(row, 2))
                                .toString()
                                .toStdString();
    runs[run] = searchResult;
  }

  ReflProgress progress(0, static_cast<double>(selectedRows.size()),
                        static_cast<int64_t>(selectedRows.size()),
                        this->m_progressView);

  TransferResults results = getTransferStrategy()->transferRuns(runs, progress);

  auto invalidRuns =
      results.getErrorRuns(); // grab our invalid runs from the transfer

  // iterate through invalidRuns to set the 'invalid transfers' in the search
  // model
  if (!invalidRuns.empty()) { // check if we have any invalid runs
    for (auto invalidRowIt = invalidRuns.begin();
         invalidRowIt != invalidRuns.end(); ++invalidRowIt) {
      auto &error = *invalidRowIt; // grab row from vector
      // iterate over row containing run number and reason why it's invalid
      for (auto errorRowIt = error.begin(); errorRowIt != error.end();
           ++errorRowIt) {
        const std::string runNumber = errorRowIt->first; // grab run number

        // iterate over rows that are selected in the search table
        for (auto rowIt = selectedRows.begin(); rowIt != selectedRows.end();
             ++rowIt) {
          const int row = *rowIt;
          // get the run number from that selected row
          const auto searchRun =
              m_searchModel->data(m_searchModel->index(row, 0))
                  .toString()
                  .toStdString();
          if (searchRun == runNumber) { // if search run number is the same as
                                        // our invalid run number

            // add this error to the member of m_searchModel that holds errors.
            m_searchModel->m_errors.push_back(error);
          }
        }
      }
    }
  }

  auto newRows = results.getTransferRuns();

  std::map<std::string, int> groups;
  // Loop over the rows (vector elements)
  for (auto rowsIt = newRows.begin(); rowsIt != newRows.end(); ++rowsIt) {
    auto &row = *rowsIt;

    if (groups.count(row[ReflTableSchema::GROUP]) == 0)
      groups[row[ReflTableSchema::GROUP]] = getUnusedGroup();

    // Overwrite the first blank row we find, otherwise, append a new row to the
    // end.
    int rowIndex = getBlankRow();
    if (rowIndex == -1) {
      rowIndex = m_model->rowCount();
      insertRow(rowIndex);
    }

    /* Set the scale to 1.0 for new rows. If there's a columnHeading specified
    otherwise,
    it will be overwritten below.
    */
    m_model->setData(m_model->index(rowIndex, ReflTableSchema::COL_SCALE), 1.0);
    auto colIndexLookup = ReflTableSchema::makeColumnNameMap();

    // Loop over the map (each row with column heading keys to cell values)
    for (auto rowIt = row.begin(); rowIt != row.end(); ++rowIt) {
      const std::string columnHeading = rowIt->first;
      const std::string cellEntry = rowIt->second;
      m_model->setData(m_model->index(rowIndex, colIndexLookup[columnHeading]),
                       QString::fromStdString(cellEntry));
    }

    // Special case grouping. Group cell entry is string it seems!
    m_model->setData(m_model->index(rowIndex, ReflTableSchema::COL_GROUP),
                     groups[row[ReflTableSchema::GROUP]]);
  }
}

/**
* Select and make a transfer strategy on demand based. Pick up the
*user-provided
* transfer strategy to do this.
*
* @return new TransferStrategy
*/
std::unique_ptr<ReflTransferStrategy>
ReflMainViewPresenter::getTransferStrategy() {
  const std::string currentMethod = m_view->getTransferMethod();
  std::unique_ptr<ReflTransferStrategy> rtnStrategy;
  if (currentMethod == MeasureTransferMethod) {

    // We need catalog info overrides from the user-based config service
    std::unique_ptr<CatalogConfigService> catConfigService(
        makeCatalogConfigServiceAdapter(ConfigService::Instance()));

    // We make a user-based Catalog Info object for the transfer
    std::unique_ptr<ICatalogInfo> catInfo = make_unique<UserCatalogInfo>(
        ConfigService::Instance().getFacility().catalogInfo(),
        *catConfigService);

    // We are going to load from disk to pick up the meta data, so provide the
    // right repository to do this.
    std::unique_ptr<ReflMeasurementItemSource> source =
        make_unique<ReflNexusMeasurementItemSource>();

    // Finally make and return the Measure based transfer strategy.
    rtnStrategy = Mantid::Kernel::make_unique<ReflMeasureTransferStrategy>(
        std::move(catInfo), std::move(source));
    return rtnStrategy;
  } else if (currentMethod == LegacyTransferMethod) {
    rtnStrategy = make_unique<ReflLegacyTransferStrategy>();
    return rtnStrategy;
  } else {
    throw std::runtime_error("Unknown tranfer method selected: " +
                             currentMethod);
  }
}

const std::string ReflMainViewPresenter::MeasureTransferMethod = "Measurement";
const std::string ReflMainViewPresenter::LegacyTransferMethod = "Description";
}
}