//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/MergeRuns.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid {
namespace Algorithms {

// Register with the algorithm factory
DECLARE_ALGORITHM(MergeRuns)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

/// Default constructor
MergeRuns::MergeRuns()
    : MultiPeriodGroupAlgorithm(), m_progress(NULL), m_inEventWS(),
      m_inMatrixWS(), m_tables() {}

/// Destructor
MergeRuns::~MergeRuns() { delete m_progress; }

//------------------------------------------------------------------------------------------------
/// Initialisation method
void MergeRuns::init() {
  // declare arbitrary number of input workspaces as a list of strings at the
  // moment
  declareProperty(
      new ArrayProperty<std::string>(
          "InputWorkspaces",
          boost::make_shared<MandatoryValidator<std::vector<std::string>>>()),
      "The names of the input workspaces as a comma-separated list. You may "
      "also group workspaces using the GUI or [[GroupWorkspaces]], and specify "
      "the name of the group instead.");
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Name of the output workspace");
}

//------------------------------------------------------------------------------------------------
// @return the name of the property used to supply in input workspace(s).
std::string MergeRuns::fetchInputPropertyName() const {
  return "InputWorkspaces";
}

//------------------------------------------------------------------------------------------------
// @returns true since "InputWorkspaces" is a non-workspace array property taken
// to be the input.
bool MergeRuns::useCustomInputPropertyName() const { return true; }

//------------------------------------------------------------------------------------------------
/** Executes the algorithm
 *  @throw Exception::NotFoundError If an input workspace doesn't exist
 *  @throw std::invalid_argument If the input workspaces are not compatible
 */
void MergeRuns::exec() {
  // Check that all input workspaces exist and match in certain important ways
  const std::vector<std::string> inputs_orig = getProperty("InputWorkspaces");

  // This will hold the inputs, with the groups separated off
  std::vector<std::string> inputs;
  for (size_t i = 0; i < inputs_orig.size(); i++) {
    WorkspaceGroup_sptr wsgroup =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            inputs_orig[i]);
    if (wsgroup) { // Workspace group
      std::vector<std::string> group = wsgroup->getNames();
      inputs.insert(inputs.end(), group.begin(), group.end());
    } else {
      // Single workspace
      inputs.push_back(inputs_orig[i]);
    }
  }

  if (inputs.size() == 1) {
    g_log.error("Only one input workspace specified");
    throw std::invalid_argument("Only one input workspace specified");
  }

  // First, try as event workspaces
  if (this->validateInputsForEventWorkspaces(inputs)) {
    // Yes, they are all event workspaces! ---------------------
    this->execEvent();
  } else {
    // At least one is not event workspace ----------------

    // This gets the list of workspaces
    m_inMatrixWS = this->validateInputs(inputs);

    // Iterate over the collection of input workspaces
    auto it = m_inMatrixWS.begin();
    // Take the first input workspace as the first argument to the addition
    MatrixWorkspace_sptr outWS = m_inMatrixWS.front();
    int64_t n = m_inMatrixWS.size() - 1;
    m_progress = new Progress(this, 0.0, 1.0, n);
    // Note that the iterator is incremented before first pass so that 1st
    // workspace isn't added to itself
    for (++it; it != m_inMatrixWS.end(); ++it) {
      MatrixWorkspace_sptr addee;
      // Only do a rebinning if the bins don't already match - otherwise can
      // just add (see the 'else')
      if (!WorkspaceHelpers::matchingBins(outWS, *it, true)) {
        std::vector<double> rebinParams;
        this->calculateRebinParams(outWS, *it, rebinParams);

        // Rebin the two workspaces in turn to the same set of bins
        outWS = this->rebinInput(outWS, rebinParams);
        addee = this->rebinInput(*it, rebinParams);
      } else {
        addee = *it;
      }

      // Add the current workspace to the total
      outWS = outWS + addee;

      m_progress->report();
    }

    // Set the final workspace to the output property
    setProperty("OutputWorkspace", outWS);
  }
}

/** Build up addition tables for merging eventlists together.
 * Throws an error if there is any incompatibility.
 */
void MergeRuns::buildAdditionTables() {
  if (m_inEventWS.size() <= 0)
    throw std::invalid_argument("MergeRuns: No workspaces found to merge.");

  // This'll hold the addition tables.
  m_tables.clear();

  // This is the workspace against which everything will be added
  EventWorkspace_sptr lhs = m_inEventWS[0];
  int lhs_nhist = static_cast<int>(lhs->getNumberHistograms());

  detid2index_map lhs_det_to_wi;
  try {
    lhs_det_to_wi = lhs->getDetectorIDToWorkspaceIndexMap(true);
  } catch (std::runtime_error &) {
    // If it fails, then there are some grouped detector IDs, and the map cannot
    // exist
  }

  for (size_t workspaceNum = 1; workspaceNum < m_inEventWS.size();
       workspaceNum++) {
    // Get the workspace
    EventWorkspace_sptr ews = m_inEventWS[workspaceNum];

    // An addition table is a list of pairs:
    //  First int = workspace index in the EW being added
    //  Second int = workspace index to which it will be added in the OUTPUT EW.
    //  -1 if it should add a new entry at the end.
    boost::shared_ptr<AdditionTable> table =
        boost::make_shared<AdditionTable>();

    // Loop through the input workspace indices
    std::size_t nhist = ews->getNumberHistograms();
    table->reserve(nhist);
    for (int inWI = 0; inWI < static_cast<int>(nhist); inWI++) {
      // Get the set of detectors in the output
      std::set<detid_t> &inDets = ews->getEventList(inWI).getDetectorIDs();

      bool done = false;

      // First off, try to match the workspace indices. Most times, this will be
      // ok right away.
      int outWI = inWI;
      if (outWI < lhs_nhist) // don't go out of bounds
      {
        std::set<detid_t> &outDets = lhs->getEventList(outWI).getDetectorIDs();

        // Checks that inDets is a subset of outDets
        if (std::includes(outDets.begin(), outDets.end(), inDets.begin(),
                          inDets.end())) {
          // We found the workspace index right away. No need to keep looking
          table->push_back(std::make_pair(inWI, outWI));
          done = true;
        }
      }

      if (!done && !lhs_det_to_wi.empty() && (inDets.size() == 1)) {
        // Didn't find it. Try to use the LHS map.

        // First, we have to get the (single) detector ID of the RHS
        std::set<detid_t>::iterator inDets_it = inDets.begin();
        detid_t rhs_detector_ID = *inDets_it;

        // Now we use the LHS map to find it. This only works if both the lhs
        // and rhs have 1 detector per pixel
        detid2index_map::const_iterator map_it =
            lhs_det_to_wi.find(rhs_detector_ID);
        if (map_it != lhs_det_to_wi.end()) {
          outWI = static_cast<int>(map_it->second); // This is the workspace
                                                    // index in the LHS that
                                                    // matched rhs_detector_ID
        } else {
          // Did not find it!
          outWI = -1; // Marker to mean its not in the LHS.
        }
        table->push_back(std::make_pair(inWI, outWI));
        done = true; // Great, we did it.
      }

      if (!done) {
        // Didn't find it? Now we need to iterate through the output workspace
        // to
        //  match the detector ID.
        // NOTE: This can be SUPER SLOW!
        for (outWI = 0; outWI < lhs_nhist; outWI++) {
          std::set<detid_t> &outDets2 =
              lhs->getEventList(outWI).getDetectorIDs();
          // Another subset check
          if (std::includes(outDets2.begin(), outDets2.end(), inDets.begin(),
                            inDets.end())) {
            // This one is right. Now we can stop looking.
            table->push_back(std::make_pair(inWI, outWI));
            done = true;
            continue;
          }
        }
      }

      if (!done) {
        // If we reach here, not a single match was found for this set of
        // inDets.

        // TODO: should we check that none of the output ones are subsets of
        // this one?

        // So we need to add it as a new workspace index
        table->push_back(std::make_pair(inWI, -1));
      }
    }

    // Add this table to the list
    m_tables.push_back(table);

  } // each of the workspaces being added

  if (m_tables.size() != m_inEventWS.size() - 1)
    throw std::runtime_error("MergeRuns::buildAdditionTables: Mismatch between "
                             "the number of addition tables and the number of "
                             "workspaces");
}

//------------------------------------------------------------------------------------------------
/** Executes the algorithm for EventWorkspaces
 */
void MergeRuns::execEvent() {
  g_log.information() << "Creating an output EventWorkspace\n";

  // Make the addition tables, or throw an error if there was a problem.
  this->buildAdditionTables();

  // Create a new output event workspace, by copying the first WS in the list
  EventWorkspace_sptr inputWS = m_inEventWS[0];

  // Make a brand new EventWorkspace
  EventWorkspace_sptr outWS = boost::dynamic_pointer_cast<EventWorkspace>(
      API::WorkspaceFactory::Instance().create(
          "EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
  // Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outWS, false);
  // You need to copy over the data as well.
  outWS->copyDataFrom((*inputWS));

  int64_t n = m_inEventWS.size() - 1;
  m_progress = new Progress(this, 0.0, 1.0, n);

  // Note that we start at 1, since we already have the 0th workspace
  for (size_t workspaceNum = 1; workspaceNum < m_inEventWS.size();
       workspaceNum++) {
    // You are adding this one here
    EventWorkspace_sptr addee = m_inEventWS[workspaceNum];

    boost::shared_ptr<AdditionTable> table = m_tables[workspaceNum - 1];

    // Add all the event lists together as the table says to do
    for (auto it = table->begin(); it != table->end(); ++it) {
      int64_t inWI = it->first;
      int64_t outWI = it->second;
      if (outWI >= 0) {
        outWS->getEventList(outWI) += addee->getEventList(inWI);
      } else {
        // Add an entry to list
        outWS->getOrAddEventList(outWS->getNumberHistograms()) +=
            addee->getEventList(inWI);
      }
    }

    // Now we add up the runs
    outWS->mutableRun() += addee->mutableRun();

    m_progress->report();
  }

  // Set the final workspace to the output property
  setProperty("OutputWorkspace",
              boost::dynamic_pointer_cast<MatrixWorkspace>(outWS));
}

//------------------------------------------------------------------------------------------------
/// @cond
// Local function used within validateInputs() below in a call to
// std::list::sort(compare)
// to order the input workspaces by the start of their frame (i.e. the first X
// value).
static bool compare(MatrixWorkspace_sptr first, MatrixWorkspace_sptr second) {
  return (first->readX(0).front() < second->readX(0).front());
}
/// @endcond

/**
Test a workspace for compatibility with others on the basis of the arguments
provided.
@param ws : Workspace to test
@param xUnitID : Unit id for the x axis
@param YUnit : Y Unit
@param dist : flag indicating that the workspace should be a distribution
@param instrument : name of the instrument
@throws an invalid argument if a full match is not acheived.
*/
void MergeRuns::testCompatibility(MatrixWorkspace_const_sptr ws,
                                  const std::string &xUnitID,
                                  const std::string &YUnit, const bool dist,
                                  const std::string instrument) const {
  std::string errors;
  if (ws->getAxis(0)->unit()->unitID() != xUnitID)
    errors += "different X units; ";
  if (ws->YUnit() != YUnit)
    errors += "different Y units; ";
  if (ws->isDistribution() != dist)
    errors += "not all distribution or all histogram type; ";
  if (ws->getInstrument()->getName() != instrument)
    errors += "different instrument names; ";
  if (errors.length() > 0) {
    g_log.error("Input workspaces are not compatible: " + errors);
    throw std::invalid_argument("Input workspaces are not compatible: " +
                                errors);
  }
}

//------------------------------------------------------------------------------------------------
/** Validate the input event workspaces
 *
 *  @param  inputWorkspaces The names of the input workspaces
 *  @throw invalid_argument if there is an incompatibility.
 *  @return true if all workspaces are event workspaces and valid. False if any
 *are not found,
 */
bool MergeRuns::validateInputsForEventWorkspaces(
    const std::vector<std::string> &inputWorkspaces) {
  std::string xUnitID;
  std::string YUnit;
  bool dist(false);

  m_inEventWS.clear();

  // Going to check that name of instrument matches - think that's the best
  // possible at the moment
  //   because if instrument is created from raw file it'll be a different
  //   object
  std::string instrument;

  for (size_t i = 0; i < inputWorkspaces.size(); ++i) {
    // Fetch the next input workspace as an - throw an error if it's not there
    EventWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            inputWorkspaces[i]);

    if (!ws) { // Either it is not found, or it is not an EventWorkspace
      return false;
    }
    m_inEventWS.push_back(ws);

    // Check a few things are the same for all input workspaces
    if (i == 0) {
      xUnitID = ws->getAxis(0)->unit()->unitID();
      YUnit = ws->YUnit();
      dist = ws->isDistribution();
      instrument = ws->getInstrument()->getName();
    } else {
      testCompatibility(ws, xUnitID, YUnit, dist, instrument);
    }
  } // for each input WS name

  // We got here: all are event workspaces
  return true;
}

//------------------------------------------------------------------------------------------------
/** Checks that the input workspace all exist, that they are the same size, have
 * the same units
 *  and the same instrument name. Will throw if they don't.
 *  @param  inputWorkspaces The names of the input workspaces
 *  @return A list of pointers to the input workspace, ordered by increasing
 * frame starting point
 *  @throw  Exception::NotFoundError If an input workspace doesn't exist
 *  @throw  std::invalid_argument    If the input workspaces are not compatible
 */
std::list<API::MatrixWorkspace_sptr>
MergeRuns::validateInputs(const std::vector<std::string> &inputWorkspaces) {
  std::list<MatrixWorkspace_sptr> inWS;

  std::string xUnitID;
  std::string YUnit;
  bool dist(false);
  // Going to check that name of instrument matches - think that's the best
  // possible at the moment
  //   because if instrument is created from raw file it'll be a different
  //   object
  std::string instrument;

  for (size_t i = 0; i < inputWorkspaces.size(); ++i) {
    MatrixWorkspace_sptr ws;
    // Fetch the next input workspace - throw an error if it's not there
    try {
      ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          inputWorkspaces[i]);
      if (!ws) {
        g_log.error() << "Input workspace " << inputWorkspaces[i]
                      << " not found." << std::endl;
        throw Kernel::Exception::NotFoundError("Data Object",
                                               inputWorkspaces[i]);
      }
      inWS.push_back(ws);
    } catch (Exception::NotFoundError &) {
      g_log.error() << "Input workspace " << inputWorkspaces[i] << " not found."
                    << std::endl;
      throw;
    }
    // Check that it has common binning
    if (!WorkspaceHelpers::commonBoundaries(inWS.back())) {
      g_log.error("Input workspaces must have common binning for all spectra");
      throw std::invalid_argument(
          "Input workspaces must have common binning for all spectra");
    }
    // Check a few things are the same for all input workspaces
    if (i == 0) {
      xUnitID = ws->getAxis(0)->unit()->unitID();
      YUnit = ws->YUnit();
      dist = ws->isDistribution();
      instrument = ws->getInstrument()->getName();
    } else {
      testCompatibility(ws, xUnitID, YUnit, dist, instrument);
    }
  }

  // Order the workspaces by ascending frame (X) starting point
  inWS.sort(compare);

  return inWS;
}

//------------------------------------------------------------------------------------------------
/** Calculates the parameters to hand to the Rebin algorithm. Specifies the new
 * binning, bin-by-bin,
 *  to cover the full range covered by the two input workspaces. In regions of
 * overlap, the bins from
 *  the workspace having the wider bins are taken. Note that because the list of
 * input workspaces
 *  is sorted, ws1 will always start before (or at the same point as) ws2.
 *  @param ws1 ::    The first input workspace. Will start before ws2.
 *  @param ws2 ::    The second input workspace.
 *  @param params :: A reference to the vector of rebinning parameters
 */
void MergeRuns::calculateRebinParams(const API::MatrixWorkspace_const_sptr &ws1,
                                     const API::MatrixWorkspace_const_sptr &ws2,
                                     std::vector<double> &params) const {
  const MantidVec &X1 = ws1->readX(0);
  const MantidVec &X2 = ws2->readX(0);
  const double end1 = X1.back();
  const double start2 = X2.front();
  const double end2 = X2.back();

  if (end1 <= start2) {
    // First case is if there's no overlap between the workspaces
    this->noOverlapParams(X1, X2, params);
  } else {
    // Add the bins from the first workspace up to the start of the overlap
    params.push_back(X1[0]);
    int64_t i;
    for (i = 1; X1[i] <= start2; ++i) {
      params.push_back(X1[i] - X1[i - 1]);
      params.push_back(X1[i]);
    }
    // If the range of workspace2 is completely within that of workspace1, then
    // call the
    // 'inclusion' routine. Otherwise call the standard 'intersection' one.
    if (end1 < end2) {
      this->intersectionParams(X1, i, X2, params);
    } else {
      this->inclusionParams(X1, i, X2, params);
    }
  }
}

//------------------------------------------------------------------------------------------------
/** Calculates the rebin paramters in the case where the two input workspaces do
 * not overlap at all.
 *  @param X1 ::     The bin boundaries from the first workspace
 *  @param X2 ::     The bin boundaries from the second workspace
 *  @param params :: A reference to the vector of rebinning parameters
 */
void MergeRuns::noOverlapParams(const MantidVec &X1, const MantidVec &X2,
                                std::vector<double> &params) const {
  // Add all the bins from the first workspace
  for (size_t i = 1; i < X1.size(); ++i) {
    params.push_back(X1[i - 1]);
    params.push_back(X1[i] - X1[i - 1]);
  }
  // Put a single bin in the 'gap' (but check first the 'gap' isn't zero)
  if (X1.back() < X2.front()) {
    params.push_back(X1.back());
    params.push_back(X2.front() - X1.back());
  }
  // Now add all the bins from the second workspace
  for (size_t j = 1; j < X2.size(); ++j) {
    params.push_back(X2[j - 1]);
    params.push_back(X2[j] - X2[j - 1]);
  }
  params.push_back(X2.back());
}

//------------------------------------------------------------------------------------------------
/** Calculates the rebin parameters in the case where the bins of the two
 * workspaces intersect.
 *  'Intersect' is used in the sense of two intersecting sets.
 *  @param X1 ::     The bin boundaries from the first workspace
 *  @param i ::      Indicates the index in X1 immediately before the overlap
 * region starts
 *  @param X2 ::     The bin boundaries from the second workspace
 *  @param params :: A reference to the vector of rebinning parameters
 */
void MergeRuns::intersectionParams(const MantidVec &X1, int64_t &i,
                                   const MantidVec &X2,
                                   std::vector<double> &params) const {
  // First calculate the number of bins in each workspace that are in the
  // overlap region
  int64_t overlapbins1, overlapbins2;
  overlapbins1 = X1.size() - i;
  for (overlapbins2 = 0; X2[overlapbins2] < X1.back(); ++overlapbins2) {
  }

  // We want to use whichever one has the larger bins (on average)
  if (overlapbins1 < overlapbins2) {
    // In this case we want the rest of the bins from the first workspace.....
    for (; i < static_cast<int64_t>(X1.size()); ++i) {
      params.push_back(X1[i] - X1[i - 1]);
      params.push_back(X1[i]);
    }
    // Now remove the last bin & boundary
    params.pop_back();
    params.pop_back();
    // ....and then the non-overlap ones from the second workspace
    for (size_t j = overlapbins2; j < X2.size(); ++j) {
      params.push_back(X2[j] - params.back());
      params.push_back(X2[j]);
    }
  } else {
    // In this case we just have to add all the bins from the second workspace
    for (size_t j = 1; j < X2.size(); ++j) {
      params.push_back(X2[j] - params.back());
      params.push_back(X2[j]);
    }
  }
}

//------------------------------------------------------------------------------------------------
/** Calculates the rebin parameters in the case where the range of the second
 * workspace is
 *  entirely within that of the first workspace.
 *  'Inclusion' is used in the sense of a set being included in anothre.
 *  @param X1 ::     The bin boundaries from the first workspace
 *  @param i ::      Indicates the index in X1 immediately before the overlap
 * region starts
 *  @param X2 ::     The bin boundaries from the second workspace
 *  @param params :: A reference to the vector of rebinning parameters
 */
void MergeRuns::inclusionParams(const MantidVec &X1, int64_t &i,
                                const MantidVec &X2,
                                std::vector<double> &params) const {
  // First calculate the number of bins in each workspace that are in the
  // overlap region
  int64_t overlapbins1, overlapbins2;
  for (overlapbins1 = 1; X1[i + overlapbins1] < X2.back(); ++overlapbins1) {
  }
  //++overlapbins1;
  overlapbins2 = X2.size() - 1;

  // In the overlap region, we want to use whichever one has the larger bins (on
  // average)
  if (overlapbins1 + 1 <= overlapbins2) {
    // In the case where the first workspace has larger bins it's easy
    // - just add the rest of X1's bins
    for (; i < static_cast<int64_t>(X1.size()); ++i) {
      params.push_back(X1[i] - X1[i - 1]);
      params.push_back(X1[i]);
    }
  } else {
    // In this case we want all of X2's bins first (without the first and last
    // boundaries)
    for (size_t j = 1; j < X2.size() - 1; ++j) {
      params.push_back(X2[j] - params.back());
      params.push_back(X2[j]);
    }
    // And now those from X1 that lie above the overlap region
    i += overlapbins1;
    for (; i < static_cast<int64_t>(X1.size()); ++i) {
      params.push_back(X1[i] - params.back());
      params.push_back(X1[i]);
    }
  }
}

//------------------------------------------------------------------------------------------------
/** Calls the Rebin algorithm as a ChildAlgorithm.
 *  @param  workspace The workspace to use as input to the Rebin algorithms
 *  @param  params    The rebin parameters
 *  @return A shared pointer to the output (rebinned) workspace
 *  @throw  std::runtime_error If the Rebin algorithm fails
 */
API::MatrixWorkspace_sptr
MergeRuns::rebinInput(const API::MatrixWorkspace_sptr &workspace,
                      const std::vector<double> &params) {
  // Create a Rebin child algorithm
  IAlgorithm_sptr rebin = createChildAlgorithm("Rebin");
  rebin->setProperty("InputWorkspace", workspace);
  rebin->setProperty("Params", params);
  rebin->executeAsChildAlg();
  return rebin->getProperty("OutputWorkspace");
}

/** Overriden fillHistory method to correctly store history from merged
 * workspaces.
 */
void MergeRuns::fillHistory() {
  // check if we were merging event or matrix workspaces
  if (!m_inEventWS.empty()) {
    copyHistoryFromInputWorkspaces<std::vector<EventWorkspace_sptr>>(
        m_inEventWS);
  } else {
    copyHistoryFromInputWorkspaces<std::list<MatrixWorkspace_sptr>>(
        m_inMatrixWS);
  }
}

} // namespace Algorithm
} // namespace Mantid
