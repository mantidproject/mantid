// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidLiveData/LoadLiveData.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ReadLock.h"
#include "MantidKernel/WriteLock.h"
#include "MantidLiveData/Exception.h"

#include <utility>

#include <Poco/Thread.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::Types::Core::DateAndTime;

namespace Mantid::LiveData {

namespace {

/**
 * Copy the Instrument from source workspace to target workspace if possible
 *
 * Handles cases where source does not exist, or are not forms of
 *ExperimentInfo.
 * Expects source and target workspaces to be the same type and size (if
 *workspace group)
 *
 * @param source : Source workspace containing instrument
 * @param target : Target workspace to write instrument to
 */
void copyInstrument(const API::Workspace *source, API::Workspace *target) {

  if (!source || !target)
    return;

  // Special handling for Worspace Groups.
  if (source->isGroup() && target->isGroup()) {
    auto *sourceGroup = dynamic_cast<const API::WorkspaceGroup *>(source);
    auto *targetGroup = dynamic_cast<API::WorkspaceGroup *>(target);
    auto minSize = std::min(sourceGroup->size(), targetGroup->size());
    for (size_t index = 0; index < minSize; ++index) {
      copyInstrument(sourceGroup->getItem(index).get(), targetGroup->getItem(index).get());
    }
  } else if (source->isGroup()) {
    auto *sourceGroup = dynamic_cast<const API::WorkspaceGroup *>(source);
    copyInstrument(sourceGroup->getItem(0).get(), target);
  } else if (target->isGroup()) {
    auto *targetGroup = dynamic_cast<API::WorkspaceGroup *>(target);
    copyInstrument(source, targetGroup->getItem(0).get());
  } else {
    if (auto *sourceExpInfo = dynamic_cast<const API::ExperimentInfo *>(source)) {
      dynamic_cast<API::ExperimentInfo &>(*target).setInstrument(sourceExpInfo->getInstrument());
    }
  }
}
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadLiveData)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadLiveData::name() const { return "LoadLiveData"; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadLiveData::category() const { return "DataHandling\\LiveData\\Support"; }

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
Mantid::API::Workspace_sptr LoadLiveData::runProcessing(Mantid::API::Workspace_sptr inputWS, bool PostProcess) {
  if (!inputWS)
    throw std::runtime_error("LoadLiveData::runProcessing() called for an empty input workspace.");
  // Prevent others writing to the workspace while we run.
  ReadLock _lock(*inputWS);

  // Make algorithm and set the properties
  auto alg = this->makeAlgorithm(PostProcess);
  if (alg) {
    if (PostProcess)
      g_log.notice() << "Performing post-processing";
    else
      g_log.notice() << "Performing chunk processing";
    g_log.notice() << " using " << alg->name() << '\n';

    // Run the processing algorithm

    // Make a unique anonymous names for the workspace, to put in ADS
    std::string inputName = "__anonymous_livedata_input_" + this->getPropertyValue("OutputWorkspace");
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
      g_log.error() << "Something really wrong happened when adding " << inputName << " to ADS. "
                    << this->getPropertyValue("OutputWorkspace") << '\n';

    // What is the name of the input workspace property and is it InOut
    std::string algoInputWSName("InputWorkspace"), algoOutputWSName("OutputWorkspace");
    bool inOutProperty{false};
    if (alg->existsProperty(algoInputWSName)) {
      g_log.debug() << "Using InputWorkspace as the input workspace property name.\n";
      alg->setPropertyValue(algoInputWSName, inputName);
    } else {
      // Look for the first Workspace property that is marked INPUT or INOUT.
      const auto &proplist = alg->getProperties();
      g_log.debug() << "Processing algorithm (" << alg->name() << ") has " << proplist.size() << " properties.\n";
      for (auto prop : proplist) {
        if ((prop->direction() == Direction::Input || prop->direction() == Direction::InOut)) {
          if (prop->type().ends_with("Workspace")) {
            g_log.information() << "Using " << prop->name() << " as the input property.\n";
            algoInputWSName = prop->name();
            alg->setPropertyValue(algoInputWSName, inputName);
            if (prop->direction() == Direction::InOut) {
              inOutProperty = true;
              algoOutputWSName = algoInputWSName;
            }
            break;
          }
        }
      }
    }

    if (!inOutProperty)
      alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setChild(true);
    alg->execute();
    if (!alg->isExecuted())
      throw std::runtime_error("Error processing the workspace using " + alg->name() + ". See log for details.");

    // Retrieve the output.
    Property *prop = alg->getProperty(algoOutputWSName);
    auto *wsProp = dynamic_cast<IWorkspaceProperty *>(prop);
    if (!wsProp)
      throw std::runtime_error("The " + alg->name() +
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
      temp = AnalysisDataService::Instance().retrieve(getPropertyValue("OutputWorkspace"));
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
Mantid::API::Workspace_sptr LoadLiveData::processChunk(Mantid::API::Workspace_sptr chunkWS) {
  try {
    return runProcessing(std::move(chunkWS), false);
  } catch (...) {
    g_log.error("While processing chunk:");
    throw;
  }
}

//----------------------------------------------------------------------------------------------
/** Perform the PostProcessing steps on the accumulated workspace.
 * Uses the m_accumWS member in a (hopefully) read-only manner.
 * Sets the m_outputWS member to the processed result.
 */
void LoadLiveData::runPostProcessing() {
  try {
    m_outputWS = runProcessing(m_accumWS, true);
  } catch (...) {
    g_log.error("While post processing:");
    throw;
  }
}

//----------------------------------------------------------------------------------------------
/** Accumulate the data by adding (summing) to the output workspace.
 * Calls the Plus algorithm
 * Sets m_accumWS.
 *
 * @param chunkWS :: processed live data chunk workspace
 */
void LoadLiveData::addChunk(const Mantid::API::Workspace_sptr &chunkWS) {
  // Acquire locks on the workspaces we use
  WriteLock _lock1(*m_accumWS);
  ReadLock _lock2(*chunkWS);

  // ISIS multi-period data come in workspace groups
  if (WorkspaceGroup_sptr gws = std::dynamic_pointer_cast<WorkspaceGroup>(chunkWS)) {
    WorkspaceGroup_sptr accum_gws = std::dynamic_pointer_cast<WorkspaceGroup>(m_accumWS);
    if (!accum_gws) {
      throw std::runtime_error("Two workspace groups are expected.");
    }
    if (accum_gws->getNumberOfEntries() != gws->getNumberOfEntries()) {
      throw std::runtime_error("Accumulation and chunk workspace groups are "
                               "expected to have the same size.");
    }
    // binary operations cannot handle groups passed by pointers, so add members
    // one by one
    for (size_t i = 0; i < static_cast<size_t>(gws->getNumberOfEntries()); ++i) {
      addMatrixWSChunk(accum_gws->getItem(i), gws->getItem(i));
    }
  } else if (std::dynamic_pointer_cast<MatrixWorkspace>(chunkWS)) {
    // If workspace is a Matrix workspace just add the chunk
    addMatrixWSChunk(m_accumWS, chunkWS);
  } else {
    // Assume MD Workspace
    addMDWSChunk(m_accumWS, chunkWS);
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
void LoadLiveData::addMatrixWSChunk(const Workspace_sptr &accumWS, const Workspace_sptr &chunkWS) {
  // Handle the addition of the internal monitor workspace, if present
  auto accumMW = std::dynamic_pointer_cast<MatrixWorkspace>(accumWS);
  auto chunkMW = std::dynamic_pointer_cast<MatrixWorkspace>(chunkWS);
  if (accumMW && chunkMW) {
    auto accumMon = accumMW->monitorWorkspace();
    auto chunkMon = chunkMW->monitorWorkspace();

    if (accumMon && chunkMon) {
      accumMon += chunkMon;
    }
  }

  // Now do the main workspace
  auto alg = this->createChildAlgorithm("Plus");
  alg->setProperty("LHSWorkspace", accumWS);
  alg->setProperty("RHSWorkspace", chunkWS);
  alg->setProperty("OutputWorkspace", accumWS);
  alg->execute();
}

//----------------------------------------------------------------------------------------------
/**
 * Add an MD Workspace to the accumulation workspace.
 *
 * @param accumWS :: accumulation MD workspace
 * @param chunkWS :: processed live data chunk MD workspace
 */
void LoadLiveData::addMDWSChunk(Workspace_sptr &accumWS, const Workspace_sptr &chunkWS) {
  // Need to add chunk to ADS for MergeMD
  std::string chunkName = "__anonymous_livedata_addmdws_" + this->getPropertyValue("OutputWorkspace");
  AnalysisDataService::Instance().addOrReplace(chunkName, chunkWS);

  std::string ws_names_to_merge = accumWS->getName();
  ws_names_to_merge.append(", ");
  ws_names_to_merge.append(chunkName);

  auto alg = this->createChildAlgorithm("MergeMD");
  alg->setPropertyValue("InputWorkspaces", ws_names_to_merge);
  alg->execute();

  // Chunk no longer needed in ADS
  AnalysisDataService::Instance().remove(chunkName);

  // Get the output as the generic Workspace type
  // This step is necessary for when we are operating on MD workspaces
  Property *prop = alg->getProperty("OutputWorkspace");
  auto *wsProp = dynamic_cast<IWorkspaceProperty *>(prop);
  if (!wsProp)
    throw std::runtime_error("The " + alg->name() +
                             " Algorithm's OutputWorkspace property is not a WorkspaceProperty!");
  Workspace_sptr temp = wsProp->getWorkspace();
  accumWS = temp;
}

//----------------------------------------------------------------------------------------------
/** Accumulate the data by replacing the output workspace.
 * Sets m_accumWS.
 *
 * @param chunkWS :: processed live data chunk workspace
 */
void LoadLiveData::replaceChunk(Mantid::API::Workspace_sptr chunkWS) {
  // We keep a temporary to the orignal workspace containing the instrument
  auto instrumentWS = m_accumWS;
  // When the algorithm exits the chunk workspace will be renamed
  // and overwrite the old one
  m_accumWS = std::move(chunkWS);
  // Put the original instrument back. Otherwise geometry changes will not be
  // persistent
  copyInstrument(instrumentWS.get(), m_accumWS.get());
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
void LoadLiveData::appendChunk(const Mantid::API::Workspace_sptr &chunkWS) {
  // ISIS multi-period data come in workspace groups
  WorkspaceGroup_sptr chunk_gws = std::dynamic_pointer_cast<WorkspaceGroup>(chunkWS);

  if (chunk_gws) {
    WorkspaceGroup_sptr accum_gws = std::dynamic_pointer_cast<WorkspaceGroup>(m_accumWS);
    if (!accum_gws) {
      throw std::runtime_error("Two workspace groups are expected.");
    }
    if (accum_gws->getNumberOfEntries() != chunk_gws->getNumberOfEntries()) {
      throw std::runtime_error("Accumulation and chunk workspace groups are "
                               "expected to have the same size.");
    }
    // disassemble the accum group and put it back together again with updated
    // items
    auto nItems = static_cast<size_t>(chunk_gws->getNumberOfEntries());
    std::vector<Workspace_sptr> items(nItems);
    for (size_t i = 0; i < nItems; ++i) {
      items[i] = accum_gws->getItem(i);
    }
    accum_gws->removeAll();
    // append members one by one
    for (size_t i = 0; i < nItems; ++i) {
      accum_gws->addWorkspace(appendMatrixWSChunk(items[i], chunk_gws->getItem(i)));
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
Workspace_sptr LoadLiveData::appendMatrixWSChunk(Workspace_sptr accumWS, const Workspace_sptr &chunkWS) {
  ReadLock _lock1(*accumWS);
  ReadLock _lock2(*chunkWS);

  auto alg = this->createChildAlgorithm("AppendSpectra");
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
  return accumWS;
}

namespace {
bool isUsingDefaultBinBoundaries(const EventWorkspace *workspace) {
  // returning false for workspaces where we don't have enough events
  // for a meaningful rebinning, and tells the caller not to try
  // to rebin the data. See EventList::getEventXMinMax() for what the
  // workspace binning will look like with this choice.
  if (workspace->getNumberEvents() <= 2)
    return false;

  // only check first spectrum
  const auto &x = workspace->binEdges(0);
  if (x.size() > 2)
    return false;
  // make sure that they are sorted
  return (x.front() < x.back());
}
} // namespace

//----------------------------------------------------------------------------------------------
/** Resets all HistogramX in given EventWorkspace(s) to a single bin.
 *
 * Ensures bin boundaries encompass all events currently in the workspace.
 * This will overwrite any rebinning that was previously done.
 *
 * Input should be an EventWorkspace or WorkspaceGroup containing
 * EventWorkspaces. Any other workspace types are ignored.
 *
 * @param workspace :: Workspace(Group) that will have its bins reset
 */
void LoadLiveData::updateDefaultBinBoundaries(API::Workspace *workspace) {
  if (auto *ws_event = dynamic_cast<EventWorkspace *>(workspace)) {
    if (isUsingDefaultBinBoundaries(ws_event))
      ws_event->resetAllXToSingleBin();
  } else if (auto *ws_group = dynamic_cast<WorkspaceGroup *>(workspace)) {
    auto num_entries = static_cast<size_t>(ws_group->getNumberOfEntries());
    for (size_t i = 0; i < num_entries; ++i) {
      auto ws = ws_group->getItem(i);
      if (auto *ws_event = dynamic_cast<EventWorkspace *>(ws.get()))
        if (isUsingDefaultBinBoundaries(ws_event))
          ws_event->resetAllXToSingleBin();
    }
  }
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
      g_log.warning() << "The " << listener->name() << " is not ready to return data: " << ex.what() << "\n";
      g_log.warning() << "Trying again in 10 seconds - cancel the algorithm to stop.\n";
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

  // For EventWorkspaces, we adjust the X values such that all events fit
  // within the bin boundaries
  const bool preserveEvents = this->getProperty("PreserveEvents");
  if (preserveEvents)
    this->updateDefaultBinBoundaries(chunkWS.get());

  // Now we process the chunk
  Workspace_sptr processed = this->processChunk(chunkWS);

  EventWorkspace_sptr processedEvent = std::dynamic_pointer_cast<EventWorkspace>(processed);
  if (!preserveEvents && processedEvent) {
    // Convert the monitor workspace, if there is one and it's necessary
    MatrixWorkspace_sptr monitorWS = processedEvent->monitorWorkspace();
    auto monitorEventWS = std::dynamic_pointer_cast<EventWorkspace>(monitorWS);
    if (monitorEventWS) {
      auto monAlg = this->createChildAlgorithm("ConvertToMatrixWorkspace");
      monAlg->setProperty("InputWorkspace", monitorEventWS);
      monAlg->executeAsChildAlg();
      if (!monAlg->isExecuted())
        g_log.error("Failed to convert monitors from events to histogram form.");
      monitorWS = monAlg->getProperty("OutputWorkspace");
    }

    // Now do the main workspace
    Algorithm_sptr alg = this->createChildAlgorithm("ConvertToMatrixWorkspace");
    alg->setProperty("InputWorkspace", processedEvent);
    std::string outputName = "__anonymous_livedata_convert_" + this->getPropertyValue("OutputWorkspace");
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

  g_log.notice() << "Performing the " << accum << " operation.\n";

  // Perform the accumulation and set the AccumulationWorkspace workspace
  if (accum == "Replace") {
    this->replaceChunk(processed);
  } else if (accum == "Append") {
    this->appendChunk(processed);
  } else {
    // Default to Add.
    this->addChunk(processed);

    // When adding events, the default bin boundaries may need to be updated.
    // The function itself checks to see if it is appropriate
    if (preserveEvents) {
      this->updateDefaultBinBoundaries(m_accumWS.get());
    }
  }

  // At this point, m_accumWS is set.

  if (this->hasPostProcessing()) {
    // ----------- Run post-processing -------------
    this->runPostProcessing();
    // Set both output workspaces
    this->setProperty("AccumulationWorkspace", m_accumWS);
    this->setProperty("OutputWorkspace", m_outputWS);
  } else {
    // ----------- No post-processing -------------
    m_outputWS = m_accumWS;
    // We DO NOT set AccumulationWorkspace.
    this->setProperty("OutputWorkspace", m_outputWS);
  }

  // Output group requires some additional handling
  WorkspaceGroup_sptr out_gws = std::dynamic_pointer_cast<WorkspaceGroup>(m_outputWS);
  if (out_gws) {
    auto n = static_cast<size_t>(out_gws->getNumberOfEntries());
    for (size_t i = 0; i < n; ++i) {
      auto ws = out_gws->getItem(i);
      const std::string &itemName = ws->getName();
      std::string wsName = getPropertyValue("OutputWorkspace") + "_" + std::to_string(i + 1);
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

} // namespace Mantid::LiveData
