#include "MantidLiveData/LoadLiveData.h"
#include "MantidLiveData/Exception.h"
#include "MantidKernel/WriteLock.h"
#include "MantidKernel/ReadLock.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/CPUTimer.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <Poco/Thread.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace LiveData {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadLiveData)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadLiveData::LoadLiveData() : LiveDataAlgorithm() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadLiveData::~LoadLiveData() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadLiveData::name() const { return "LoadLiveData"; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadLiveData::category() const {
  return "DataHandling\\LiveData\\Support";
}

/// Algorithm's version for identification. @see Algorithm::version
int LoadLiveData::version() const { return 1; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadLiveData::init() { this->initProps(); }

//----------------------------------------------------------------------------------------------
/** Run either the chunk or post-processing step
 *
 * @param inputWS :: workspace being processed
 * @param PostProcess :: flag, TRUE if doing the post-processing
 * @return the processed workspace. Will point to inputWS if no processing is to
 *do
 */
Mantid::API::Workspace_sptr
LoadLiveData::runProcessing(Mantid::API::Workspace_sptr inputWS,
                            bool PostProcess) {
  if (!inputWS)
    throw std::runtime_error(
        "LoadLiveData::runProcessing() called for an empty input workspace.");
  // Prevent others writing to the workspace while we run.
  ReadLock _lock(*inputWS);

  // Make algorithm and set the properties
  IAlgorithm_sptr alg = this->makeAlgorithm(PostProcess);
  if (alg) {
    if (PostProcess)
      g_log.notice() << "Performing post-processing";
    else
      g_log.notice() << "Performing chunk processing";
    g_log.notice() << " using " << alg->name() << std::endl;

    // Run the processing algorithm

    // Make a unique anonymous names for the workspace, to put in ADS
    std::string inputName = "__anonymous_livedata_input_" +
                            this->getPropertyValue("OutputWorkspace");
    // Transform the chunk in-place
    std::string outputName = inputName;

    // Except, no need for anonymous names with the post-processing
    if (PostProcess) {
      inputName = this->getPropertyValue("AccumulationWorkspace");
      outputName = this->getPropertyValue("OutputWorkspace");
    }

    // For python scripts to work we need to go through the ADS
    AnalysisDataService::Instance().addOrReplace(inputName, inputWS);
    if (!AnalysisDataService::Instance().doesExist(inputName))
      g_log.error() << "Something really wrong happened when adding "
                    << inputName << " to ADS. "
                    << this->getPropertyValue("OutputWorkspace") << std::endl;

    // What is the name of the input workspace property
    if (alg->existsProperty("InputWorkspace")) {
      g_log.debug()
          << "Using InputWorkspace as the input workspace property name."
          << std::endl;
      alg->setPropertyValue("InputWorkspace", inputName);
    } else {
      // Look for the first Workspace property that is marked INPUT.
      std::vector<Property *> proplist = alg->getProperties();
      g_log.debug() << "Processing algorithm (" << alg->name() << ") has "
                    << proplist.size() << " properties." << std::endl;
      bool inputPropertyWorkspaceFound = false;
      for (size_t i = 0; i < proplist.size(); ++i) {
        Property *prop = proplist[i];
        if ((prop->direction() == 0) &&
            (inputPropertyWorkspaceFound == false)) {
          if (boost::ends_with(prop->type(), "Workspace")) {
            g_log.information() << "Using " << prop->name()
                                << " as the input property." << std::endl;
            alg->setPropertyValue(prop->name(), inputName);
            inputPropertyWorkspaceFound = true;
          }
        }
      }
    }

    // TODO: (Ticket #5774) Decide if we should do the same for output (see
    // below) - Also do we need to do a similar thing for post-processing.

    /*  Leaving the following code in for the moment - will be removed as part
    of trac ticket #5774.

          // Now look at the output workspace property
          if (alg->existsProperty("OutputWorkspace"))
          {
              g_log.debug() << "Using OutputWorkspace as the output workspace
    property name." << std::endl;
              alg->setPropertyValue("OutputWorkspace", outputName);
          }
          else
          {
              // Look for the first Workspace property that is marked OUTPUT.
              std::vector<Property*> proplist = alg->getProperties();
              g_log.debug() << "Processing algorithm (" << alg->name() << ") has
    " << proplist.size() << " properties." << std::endl;
              bool outputPropertyWorkspaceFound = false;
              for (size_t i=0; i<proplist.size(); ++i)
              {
                  Property * prop = proplist[i];
                  if ((prop->direction() == 1) && (outputPropertyWorkspaceFound
    == false))
                  {
                      g_log.information() << "*** " <<
    outputPropertyWorkspaceFound << std::endl;
                      if (prop->type() == "MatrixWorkspace")
                      {
                          g_log.information() << "Using " << prop->name() << "
    as the input property." << std::endl;
                          alg->setPropertyValue(prop->name(), outputName);
                          outputPropertyWorkspaceFound = true;
                      }
                  }

    //              g_log.debug() << "Propery #" << i << std::endl;
    //              g_log.debug() << "\tName: " << prop->name() << std::endl;
    //              g_log.debug() << "\tDirection: " << prop->direction() <<
    std::endl;
    //              g_log.debug() << "\tType: " << prop->type() << std::endl;
    //              g_log.debug() << "\tType_Info: " << prop->type_info() <<
    std::endl;

              }
          }
    */

    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setChild(true);
    alg->execute();
    if (!alg->isExecuted())
      throw std::runtime_error("Error processing the workspace using " +
                               alg->name() + ". See log for details.");

    // Retrieve the output.
    Property *prop = alg->getProperty("OutputWorkspace");
    IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty *>(prop);
    if (!wsProp)
      throw std::runtime_error(
          "The " + alg->name() +
          " Algorithm's OutputWorkspace property is not a WorkspaceProperty!");
    Workspace_sptr temp = wsProp->getWorkspace();

    if (!PostProcess) {
      if (!temp) {
        // a group workspace cannot be returned by wsProp
        temp = AnalysisDataService::Instance().retrieve(inputName);
      }
      // Remove the chunk workspace from the ADS, it is no longer needed there.
      AnalysisDataService::Instance().remove(inputName);
    } else if (!temp) {
      // a group workspace cannot be returned by wsProp
      temp = AnalysisDataService::Instance().retrieve(
          getPropertyValue("OutputWorkspace"));
    }
    return temp;
  } else {
    // Don't do any processing.
    return inputWS;
  }
}

//----------------------------------------------------------------------------------------------
/** Perform the processing on the chunk of workspace data, using the
 * algorithm or scrip given in the algorithm properties
 *
 * @param chunkWS :: chunk workspace to process
 * @return the processed workspace sptr
 */
Mantid::API::Workspace_sptr
LoadLiveData::processChunk(Mantid::API::Workspace_sptr chunkWS) {
  return runProcessing(chunkWS, false);
}

//----------------------------------------------------------------------------------------------
/** Perform the PostProcessing steps on the accumulated workspace.
 * Uses the m_accumWS member in a (hopefully) read-only manner.
 * Sets the m_outputWS member to the processed result.
 */
void LoadLiveData::runPostProcessing() {
  m_outputWS = runProcessing(m_accumWS, true);
}

//----------------------------------------------------------------------------------------------
/** Accumulate the data by adding (summing) to the output workspace.
 * Calls the Plus algorithm
 * Sets m_accumWS.
 *
 * @param chunkWS :: processed live data chunk workspace
 */
void LoadLiveData::addChunk(Mantid::API::Workspace_sptr chunkWS) {
  // Acquire locks on the workspaces we use
  WriteLock _lock1(*m_accumWS);
  ReadLock _lock2(*chunkWS);

  // Choose the appropriate algorithm to add chunks
  std::string algoName = "PlusMD";
  MatrixWorkspace_sptr mws =
      boost::dynamic_pointer_cast<MatrixWorkspace>(chunkWS);
  // ISIS multi-period data come in workspace groups
  WorkspaceGroup_sptr gws =
      boost::dynamic_pointer_cast<WorkspaceGroup>(chunkWS);
  if (mws || gws)
    algoName = "Plus";

  if (gws) {
    WorkspaceGroup_sptr accum_gws =
        boost::dynamic_pointer_cast<WorkspaceGroup>(m_accumWS);
    if (!accum_gws) {
      throw std::runtime_error("Two workspace groups are expected.");
    }
    if (accum_gws->getNumberOfEntries() != gws->getNumberOfEntries()) {
      throw std::runtime_error("Accumulation and chunk workspace groups are "
                               "expected to have the same size.");
    }
    // binary operations cannot handle groups passed by pointers, so add members
    // one by one
    for (size_t i = 0; i < static_cast<size_t>(gws->getNumberOfEntries());
         ++i) {
      addMatrixWSChunk(algoName, accum_gws->getItem(i), gws->getItem(i));
    }
  } else {
    // just add the chunk
    addMatrixWSChunk(algoName, m_accumWS, chunkWS);
  }
}

//----------------------------------------------------------------------------------------------
/**
 * Add a matrix workspace to the accumulation workspace.
 *
 * @param algoName :: Name of algorithm which will be adding the workspaces.
 * @param accumWS :: accumulation matrix workspace
 * @param chunkWS :: processed live data chunk matrix workspace
 */
void LoadLiveData::addMatrixWSChunk(const std::string &algoName,
                                    Workspace_sptr accumWS,
                                    Workspace_sptr chunkWS) {
  // Handle the addition of the internal monitor workspace, if present
  auto accumMW = boost::dynamic_pointer_cast<MatrixWorkspace>(accumWS);
  auto chunkMW = boost::dynamic_pointer_cast<MatrixWorkspace>(chunkWS);
  if (accumMW && chunkMW) {
    auto accumMon = accumMW->monitorWorkspace();
    auto chunkMon = chunkMW->monitorWorkspace();

    if (accumMon && chunkMon)
      accumMon += chunkMon;
  }

  // Now do the main workspace
  IAlgorithm_sptr alg = this->createChildAlgorithm(algoName);
  alg->setProperty("LHSWorkspace", accumWS);
  alg->setProperty("RHSWorkspace", chunkWS);
  alg->setProperty("OutputWorkspace", accumWS);
  alg->execute();
  if (!alg->isExecuted()) {
    throw std::runtime_error("Error when calling " + alg->name() +
                             " to add the chunk of live data. See log.");
  } else {
    // Get the output as the generic Workspace type
    // This step is necessary for when we are operating on MD workspaces
    // (PlusMD)
    Property *prop = alg->getProperty("OutputWorkspace");
    IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty *>(prop);
    if (!wsProp)
      throw std::runtime_error(
          "The " + alg->name() +
          " Algorithm's OutputWorkspace property is not a WorkspaceProperty!");
    Workspace_sptr temp = wsProp->getWorkspace();
    accumWS = temp;
    // And sort the events, if any
    doSortEvents(accumWS);
  }
}

//----------------------------------------------------------------------------------------------
/** Accumulate the data by replacing the output workspace.
 * Sets m_accumWS.
 *
 * @param chunkWS :: processed live data chunk workspace
 */
void LoadLiveData::replaceChunk(Mantid::API::Workspace_sptr chunkWS) {
  // When the algorithm exits the chunk workspace will be renamed
  // and overwrite the old one
  m_accumWS = chunkWS;
  // And sort the events, if any
  doSortEvents(m_accumWS);
}

//----------------------------------------------------------------------------------------------
/** Accumulate the data by appending the spectra into the
 * the output workspace.
 * Checks if the chunk is a group and if it is calls appendMatrixWSChunk for
 *each item.
 * If it's a matrix just calls appendMatrixWSChunk.
 * Sets m_accumWS.
 *
 * @param chunkWS :: processed live data chunk workspace
 */
void LoadLiveData::appendChunk(Mantid::API::Workspace_sptr chunkWS) {
  // ISIS multi-period data come in workspace groups
  WorkspaceGroup_sptr chunk_gws =
      boost::dynamic_pointer_cast<WorkspaceGroup>(chunkWS);

  if (chunk_gws) {
    WorkspaceGroup_sptr accum_gws =
        boost::dynamic_pointer_cast<WorkspaceGroup>(m_accumWS);
    if (!accum_gws) {
      throw std::runtime_error("Two workspace groups are expected.");
    }
    if (accum_gws->getNumberOfEntries() != chunk_gws->getNumberOfEntries()) {
      throw std::runtime_error("Accumulation and chunk workspace groups are "
                               "expected to have the same size.");
    }
    // disassemble the accum group and put it back together again with updated
    // items
    size_t nItems = static_cast<size_t>(chunk_gws->getNumberOfEntries());
    std::vector<Workspace_sptr> items(nItems);
    for (size_t i = 0; i < nItems; ++i) {
      items[i] = accum_gws->getItem(i);
    }
    accum_gws->removeAll();
    // append members one by one
    for (size_t i = 0; i < nItems; ++i) {
      accum_gws->addWorkspace(
          appendMatrixWSChunk(items[i], chunk_gws->getItem(i)));
    }
  } else {
    // just append the chunk
    m_accumWS = appendMatrixWSChunk(m_accumWS, chunkWS);
  }
}

//----------------------------------------------------------------------------------------------
/** Accumulate the data by appending the spectra into the
 * the output workspace.
 * Calls AppendSpectra algorithm.
 *
 * @param accumWS :: accumulation matrix workspace
 * @param chunkWS :: processed live data chunk matrix workspace
 */
Workspace_sptr LoadLiveData::appendMatrixWSChunk(Workspace_sptr accumWS,
                                                 Workspace_sptr chunkWS) {
  IAlgorithm_sptr alg;
  ReadLock _lock1(*accumWS);
  ReadLock _lock2(*chunkWS);

  alg = this->createChildAlgorithm("AppendSpectra");
  alg->setProperty("InputWorkspace1", accumWS);
  alg->setProperty("InputWorkspace2", chunkWS);
  alg->setProperty("ValidateInputs", false);
  alg->setProperty("MergeLogs", true);
  alg->execute();
  if (!alg->isExecuted()) {
    throw std::runtime_error("Error when calling AppendSpectra to append the "
                             "spectra of the chunk of live data. See log.");
  }

  MatrixWorkspace_sptr temp = alg->getProperty("OutputWorkspace");
  accumWS = temp;
  // And sort the events, if any
  doSortEvents(accumWS);
  return accumWS;
}

//----------------------------------------------------------------------------------------------
/** Perform SortEvents on the output workspaces (accumulation or output)
 * but only if they are EventWorkspaces. This will help the GUI
 * cope with redrawing.
 *
 * @param ws :: any Workspace. Does nothing if not EventWorkspace.
 */
void LoadLiveData::doSortEvents(Mantid::API::Workspace_sptr ws) {
  EventWorkspace_sptr eventWS = boost::dynamic_pointer_cast<EventWorkspace>(ws);
  if (!eventWS)
    return;
  CPUTimer tim;
  Algorithm_sptr alg = this->createChildAlgorithm("SortEvents");
  alg->setProperty("InputWorkspace", eventWS);
  alg->setPropertyValue("SortBy", "X Value");
  alg->executeAsChildAlg();
  g_log.debug() << tim << " to perform SortEvents on " << ws->name()
                << std::endl;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadLiveData::exec() {
  // The full, post-processed output workspace
  m_outputWS = this->getProperty("OutputWorkspace");

  // Validate inputs
  if (this->hasPostProcessing()) {
    if (this->getPropertyValue("AccumulationWorkspace").empty())
      throw std::invalid_argument("Must specify the AccumulationWorkspace "
                                  "parameter if using PostProcessing.");

    // The accumulated but not post-processed output workspace
    m_accumWS = this->getProperty("AccumulationWorkspace");
  } else {
    // No post-processing, so the accumulation and output are the same
    m_accumWS = m_outputWS;
  }

  // Get or create the live listener
  ILiveListener_sptr listener = this->getLiveListener();

  // Do we need to reset the data?
  bool dataReset = listener->dataReset();

  // The listener returns a MatrixWorkspace containing the chunk of live data.
  Workspace_sptr chunkWS;
  bool dataNotYetGiven = true;
  while (dataNotYetGiven) {
    try {
      chunkWS = listener->extractData();
      dataNotYetGiven = false;
    } catch (Exception::NotYet &ex) {
      g_log.warning() << "The " << listener->name()
                      << " is not ready to return data: " << ex.what() << "\n";
      g_log.warning()
          << "Trying again in 10 seconds - cancel the algorithm to stop.\n";
      const int tenSeconds = 40;
      for (int i = 0; i < tenSeconds; ++i) {
        Poco::Thread::sleep(10000 / tenSeconds); // 250 ms
        this->interruption_point();
      }
    }
  }

  // TODO: Have the ILiveListener tell me exactly the time stamp
  DateAndTime lastTimeStamp = DateAndTime::getCurrentTime();
  this->setPropertyValue("LastTimeStamp", lastTimeStamp.toISO8601String());

  // Now we process the chunk
  Workspace_sptr processed = this->processChunk(chunkWS);

  bool PreserveEvents = this->getProperty("PreserveEvents");
  EventWorkspace_sptr processedEvent =
      boost::dynamic_pointer_cast<EventWorkspace>(processed);
  if (!PreserveEvents && processedEvent) {
    // Convert the monitor workspace, if there is one and it's necessary
    MatrixWorkspace_sptr monitorWS = processedEvent->monitorWorkspace();
    auto monitorEventWS =
        boost::dynamic_pointer_cast<EventWorkspace>(monitorWS);
    if (monitorEventWS) {
      auto monAlg = this->createChildAlgorithm("ConvertToMatrixWorkspace");
      monAlg->setProperty("InputWorkspace", monitorEventWS);
      monAlg->executeAsChildAlg();
      if (!monAlg->isExecuted())
        g_log.error(
            "Failed to convert monitors from events to histogram form.");
      monitorWS = monAlg->getProperty("OutputWorkspace");
    }

    // Now do the main workspace
    Algorithm_sptr alg = this->createChildAlgorithm("ConvertToMatrixWorkspace");
    alg->setProperty("InputWorkspace", processedEvent);
    std::string outputName = "__anonymous_livedata_convert_" +
                             this->getPropertyValue("OutputWorkspace");
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->execute();
    if (!alg->isExecuted())
      throw std::runtime_error("Error when calling ConvertToMatrixWorkspace "
                               "(since PreserveEvents=False). See log.");
    // Replace the "processed" workspace with the converted one.
    MatrixWorkspace_sptr temp = alg->getProperty("OutputWorkspace");
    if (monitorWS)
      temp->setMonitorWorkspace(monitorWS); // Set back the monitor workspace
    processed = temp;
  }

  // How do we accumulate the data?
  std::string accum = this->getPropertyValue("AccumulationMethod");

  // If the AccumulationWorkspace does not exist, we always replace the
  // AccumulationWorkspace.
  // Also, if the listener said we are resetting the data, then we clear out the
  // old.
  if (!m_accumWS || dataReset)
    accum = "Replace";

  g_log.notice() << "Performing the " << accum << " operation." << std::endl;

  // Perform the accumulation and set the AccumulationWorkspace workspace
  if (accum == "Replace")
    this->replaceChunk(processed);
  else if (accum == "Append")
    this->appendChunk(processed);
  else
    // Default to Add.
    this->addChunk(processed);

  // At this point, m_accumWS is set.

  if (this->hasPostProcessing()) {
    // ----------- Run post-processing -------------
    this->runPostProcessing();
    // Set both output workspaces
    this->setProperty("AccumulationWorkspace", m_accumWS);
    this->setProperty("OutputWorkspace", m_outputWS);
    doSortEvents(m_outputWS);
  } else {
    // ----------- No post-processing -------------
    m_outputWS = m_accumWS;
    // We DO NOT set AccumulationWorkspace.
    this->setProperty("OutputWorkspace", m_outputWS);
  }

  // Output group requires some additional handling
  WorkspaceGroup_sptr out_gws =
      boost::dynamic_pointer_cast<WorkspaceGroup>(m_outputWS);
  if (out_gws) {
    size_t n = static_cast<size_t>(out_gws->getNumberOfEntries());
    for (size_t i = 0; i < n; ++i) {
      auto ws = out_gws->getItem(i);
      std::string itemName = ws->name();
      std::string wsName = getPropertyValue("OutputWorkspace") + "_" +
                           boost::lexical_cast<std::string>(i + 1);
      if (wsName != itemName) {
        if (AnalysisDataService::Instance().doesExist(itemName)) {
          // replace the temporary name with the proper one
          AnalysisDataService::Instance().rename(itemName, wsName);
        }
      } else {
        // touch the workspace in the ADS to issue a notification to update the
        // GUI
        AnalysisDataService::Instance().addOrReplace(itemName, ws);
      }
    }
  }
}

} // namespace LiveData
} // namespace Mantid
