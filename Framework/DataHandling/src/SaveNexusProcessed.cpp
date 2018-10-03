// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// SaveNexusProcessed
// @author Ronald Fowler, based on SaveNexus
#include "MantidDataHandling/SaveNexusProcessed.h"
#include "MantidAPI/EnabledWhenWorkspaceIsType.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidNexus/NexusFileIO.h"
#include <Poco/File.h>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>

using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using Geometry::Instrument_const_sptr;

using optional_size_t = NeXus::NexusFileIO::optional_size_t;

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveNexusProcessed)

/** Initialisation method.
 *
 */
void SaveNexusProcessed::init() {
  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Name of the workspace to be saved");
  // Declare required input parameters for algorithm
  const std::vector<std::string> fileExts{".nxs", ".nx5", ".xml"};
  declareProperty(Kernel::make_unique<FileProperty>(
                      "Filename", "", FileProperty::Save, fileExts),
                  "The name of the Nexus file to write, as a full or relative\n"
                  "path");

  // Declare optional parameters (title now optional, was mandatory)
  declareProperty("Title", "", boost::make_shared<NullValidator>(),
                  "A title to describe the saved workspace");
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);

  declareProperty("WorkspaceIndexMin", 0, mustBePositive,
                  "Index number of first spectrum to write, only for single\n"
                  "period data.");
  declareProperty("WorkspaceIndexMax", Mantid::EMPTY_INT(), mustBePositive,
                  "Index of last spectrum to write, only for single period\n"
                  "data.");
  declareProperty(make_unique<ArrayProperty<int>>("WorkspaceIndexList"),
                  "List of spectrum numbers to read, only for single period\n"
                  "data.");

  declareProperty("Append", false,
                  "Determines whether .nxs file needs to be\n"
                  "over written or appended");

  declareProperty(
      "PreserveEvents", true,
      "For EventWorkspaces, preserve the events when saving (default).\n"
      "If false, will save the 2D histogram version of the workspace with the "
      "current binning parameters.");
  setPropertySettings("PreserveEvents",
                      make_unique<EnabledWhenWorkspaceIsType<EventWorkspace>>(
                          "InputWorkspace", true));

  declareProperty(
      "CompressNexus", false,
      "For EventWorkspaces, compress the Nexus data field (default False).\n"
      "This will make smaller files but takes much longer.");
  setPropertySettings("CompressNexus",
                      make_unique<EnabledWhenWorkspaceIsType<EventWorkspace>>(
                          "InputWorkspace", true));
}

/** Get the list of workspace indices to use
 *
 * @param spec :: returns the list of workspace indices
 * @param matrixWorkspace :: pointer to a MatrixWorkspace
 */
void SaveNexusProcessed::getSpectrumList(
    std::vector<int> &spec, MatrixWorkspace_const_sptr matrixWorkspace) {
  std::vector<int> spec_list = getProperty("WorkspaceIndexList");
  int spec_min = getProperty("WorkspaceIndexMin");
  int spec_max = getProperty("WorkspaceIndexMax");
  const bool list = !spec_list.empty();
  const bool interval = (spec_max != Mantid::EMPTY_INT());
  if (spec_max == Mantid::EMPTY_INT())
    spec_max = 0;
  const int numberOfHist =
      static_cast<int>(matrixWorkspace->getNumberHistograms());

  if (interval) {
    if (spec_max < spec_min || spec_max > numberOfHist - 1) {
      g_log.error("Invalid WorkspaceIndex min/max properties");
      throw std::invalid_argument("Inconsistent properties defined");
    }
    spec.reserve(1 + spec_max - spec_min);
    for (int i = spec_min; i <= spec_max; i++)
      spec.push_back(i);
    if (list) {
      for (auto s : spec_list) {
        if (s < 0)
          continue;
        if (s < spec_min || s > spec_max)
          spec.push_back(s);
      }
    }
  } else if (list) {
    spec_max = 0;
    spec_min = numberOfHist - 1;
    for (auto s : spec_list) {
      if (s < 0)
        continue;
      spec.push_back(s);
      if (s > spec_max)
        spec_max = s;
      if (s < spec_min)
        spec_min = s;
    }
  } else {
    spec_min = 0;
    spec_max = numberOfHist - 1;
    spec.reserve(1 + spec_max - spec_min);
    for (int i = spec_min; i <= spec_max; i++)
      spec.push_back(i);
  }
}

void SaveNexusProcessed::doExec(
    Workspace_sptr inputWorkspace,
    boost::shared_ptr<Mantid::NeXus::NexusFileIO> &nexusFile,
    const bool keepFile, optional_size_t entryNumber) {
  // TODO: Remove?
  NXMEnableErrorReporting();

  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");
  // m_entryname = getPropertyValue("EntryName");
  m_title = getPropertyValue("Title");
  // Do we prserve events?
  bool PreserveEvents = getProperty("PreserveEvents");

  MatrixWorkspace_const_sptr matrixWorkspace =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(inputWorkspace);
  ITableWorkspace_const_sptr tableWorkspace =
      boost::dynamic_pointer_cast<const ITableWorkspace>(inputWorkspace);
  PeaksWorkspace_const_sptr peaksWorkspace =
      boost::dynamic_pointer_cast<const PeaksWorkspace>(inputWorkspace);
  OffsetsWorkspace_const_sptr offsetsWorkspace =
      boost::dynamic_pointer_cast<const OffsetsWorkspace>(inputWorkspace);
  if (peaksWorkspace)
    g_log.debug("We have a peaks workspace");
  // check if inputWorkspace is something we know how to save
  if (!matrixWorkspace && !tableWorkspace) {
    // get the workspace name for the error message
    std::string name = getProperty("InputWorkspace");

    // md workspaces should be saved using SaveMD
    if (bool(boost::dynamic_pointer_cast<const IMDEventWorkspace>(
            inputWorkspace)) ||
        bool(boost::dynamic_pointer_cast<const IMDHistoWorkspace>(
            inputWorkspace)))
      g_log.warning() << name << " can be saved using SaveMD\n";

    // standard error message
    std::stringstream msg;
    msg << "Workspace \"" << name
        << "\" not saved because it is not of a type we can presently save.";

    throw std::runtime_error(msg.str());
  }
  m_eventWorkspace =
      boost::dynamic_pointer_cast<const EventWorkspace>(matrixWorkspace);
  const std::string workspaceID = inputWorkspace->id();
  if ((workspaceID.find("Workspace2D") == std::string::npos) &&
      (workspaceID.find("RebinnedOutput") == std::string::npos) &&
      !m_eventWorkspace && !tableWorkspace && !offsetsWorkspace)
    throw Exception::NotImplementedError(
        "SaveNexusProcessed passed invalid workspaces. Must be Workspace2D, "
        "EventWorkspace, ITableWorkspace, or OffsetsWorkspace.");

  // Create progress object for initial part - depends on whether events are
  // processed
  if (PreserveEvents && m_eventWorkspace) {
    m_timeProgInit = 0.07; // Events processed 0.05 to 1.0
  } else {
    m_timeProgInit = 1.0; // All work is done in the initial part
  }
  Progress prog_init(this, 0.0, m_timeProgInit, 7);

  // If no title's been given, use the workspace title field
  if (m_title.empty())
    m_title = inputWorkspace->getTitle();

  // get the workspace name to write to file
  std::string wsName = inputWorkspace->getName();

  // If we don't want to append then remove the file if it already exists
  bool append_to_file = getProperty("Append");
  if (!append_to_file && !keepFile) {
    Poco::File file(m_filename);
    if (file.exists())
      file.remove();
  }

  nexusFile->resetProgress(&prog_init);
  nexusFile->openNexusWrite(m_filename, entryNumber);

  // Equivalent C++ API handle
  ::NeXus::File cppFile(nexusFile->fileID);

  prog_init.reportIncrement(1, "Opening file");
  if (nexusFile->writeNexusProcessedHeader(m_title, wsName) != 0)
    throw Exception::FileError("Failed to write to file", m_filename);

  prog_init.reportIncrement(1, "Writing header");

  // write instrument data, if present and writer enabled
  if (matrixWorkspace) {
    // Save the instrument names, ParameterMap, sample, run
    matrixWorkspace->saveExperimentInfoNexus(&cppFile);
    prog_init.reportIncrement(1, "Writing sample and instrument");

    // check if all X() are in fact the same array
    const bool uniformSpectra =
        API::WorkspaceHelpers::commonBoundaries(*matrixWorkspace);

    // Retrieve the workspace indices (from params)
    std::vector<int> spec;
    this->getSpectrumList(spec, matrixWorkspace);

    prog_init.reportIncrement(1, "Writing data");
    // Write out the data (2D or event)
    if (m_eventWorkspace && PreserveEvents) {
      this->execEvent(nexusFile.get(), uniformSpectra, spec);
    } else if (offsetsWorkspace) {
      g_log.warning() << "Writing SpecialWorkspace2D ID=" << workspaceID
                      << "\n";
      nexusFile->writeNexusProcessedData2D(matrixWorkspace, uniformSpectra,
                                           spec, "offsets_workspace", true);
    } else {
      nexusFile->writeNexusProcessedData2D(matrixWorkspace, uniformSpectra,
                                           spec, "workspace", true);
    }

    cppFile.openGroup("instrument", "NXinstrument");
    saveSpectraMapNexus(*matrixWorkspace, &cppFile, spec, ::NeXus::LZW);
    cppFile.closeGroup();

  } // finish matrix workspace specifics

  if (peaksWorkspace) {
    // Save the instrument names, ParameterMap, sample, run
    peaksWorkspace->saveExperimentInfoNexus(&cppFile);
    prog_init.reportIncrement(1, "Writing sample and instrument");
  }

  // peaks workspace specifics
  if (peaksWorkspace) {
    peaksWorkspace->saveNexus(&cppFile);

  }                        // finish peaks workspace specifics
  else if (tableWorkspace) // Table workspace specifics
  {
    nexusFile->writeNexusTableWorkspace(tableWorkspace, "table_workspace");
  } // finish table workspace specifics

  // Switch to the Cpp API for the algorithm history
  if (trackingHistory()) {
    m_history->fillAlgorithmHistory(
        this, Mantid::Types::Core::DateAndTime::getCurrentTime(), 0,
        Algorithm::g_execCount);
    if (!isChild()) {
      inputWorkspace->history().addHistory(m_history);
    }
    // this is a child algorithm, but we still want to keep the history.
    else if (isRecordingHistoryForChild() && m_parentHistory) {
      m_parentHistory->addChildHistory(m_history);
    }
  }

  inputWorkspace->history().saveNexus(&cppFile);
  nexusFile->closeGroup();
}

//-----------------------------------------------------------------------------------------------
/** Executes the algorithm for a single workspace.
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void SaveNexusProcessed::exec() {
  Workspace_sptr inputWorkspace = getProperty("InputWorkspace");

  // Then immediately open the file
  auto nexusFile = boost::make_shared<Mantid::NeXus::NexusFileIO>();

  // Perform the execution.
  doExec(inputWorkspace, nexusFile);
}

//-------------------------------------------------------------------------------------
/** Append out each field of a vector of events to separate array.
 *
 * @param events :: vector of TofEvent or WeightedEvent, etc.
 * @param offset :: where the first event goes in the array
 * @param tofs, weights, errorSquareds, pulsetimes :: arrays to write to.
 *        Must be initialized and big enough,
 *        or NULL if they are not meant to be written to.
 */
template <class T>
void SaveNexusProcessed::appendEventListData(const std::vector<T> &events,
                                             size_t offset, double *tofs,
                                             float *weights,
                                             float *errorSquareds,
                                             int64_t *pulsetimes) {
  // Do nothing if there are no events.
  if (events.empty())
    return;

  const auto it = events.cbegin();
  const auto it_end = events.cend();

  // Fill the C-arrays with the fields from all the events, as requested.
  if (tofs) {
    std::transform(it, it_end, std::next(tofs, offset),
                   [](const T &event) { return event.tof(); });
  }
  if (weights) {
    std::transform(it, it_end, std::next(weights, offset), [](const T &event) {
      return static_cast<float>(event.weight());
    });
  }
  if (errorSquareds) {
    std::transform(it, it_end, std::next(errorSquareds, offset),
                   [](const T &event) {
                     return static_cast<float>(event.errorSquared());
                   });
  }
  if (pulsetimes) {
    std::transform(
        it, it_end, std::next(pulsetimes, offset),
        [](const T &event) { return event.pulseTime().totalNanoseconds(); });
  }
}

//-----------------------------------------------------------------------------------------------
/** Execute the saving of event data.
 * This will make one long event list for all events contained.
 * */
void SaveNexusProcessed::execEvent(Mantid::NeXus::NexusFileIO *nexusFile,
                                   const bool uniformSpectra,
                                   const std::vector<int> &spec) {
  m_progress = make_unique<Progress>(this, m_timeProgInit, 1.0,
                                     m_eventWorkspace->getNumberEvents() * 2);

  // Start by writing out the axes and crap
  nexusFile->writeNexusProcessedData2D(m_eventWorkspace, uniformSpectra, spec,
                                       "event_workspace", false);

  // Make a super long list of tofs, weights, etc.
  std::vector<int64_t> indices;
  indices.reserve(m_eventWorkspace->getNumberHistograms() + 1);
  // First we need to index the events in each spectrum
  size_t index = 0;
  for (int wi = 0;
       wi < static_cast<int>(m_eventWorkspace->getNumberHistograms()); wi++) {
    indices.push_back(index);
    // Track the total # of events
    index += m_eventWorkspace->getSpectrum(wi).getNumberEvents();
  }
  indices.push_back(index);

  // Initialize all the arrays
  int64_t num = index;
  double *tofs = nullptr;
  float *weights = nullptr;
  float *errorSquareds = nullptr;
  int64_t *pulsetimes = nullptr;

  // overall event type.
  EventType type = m_eventWorkspace->getEventType();
  bool writeTOF = true;
  bool writePulsetime = false;
  bool writeWeight = false;
  bool writeError = false;

  switch (type) {
  case TOF:
    writePulsetime = true;
    break;
  case WEIGHTED:
    writePulsetime = true;
    writeWeight = true;
    writeError = true;
    break;
  case WEIGHTED_NOTIME:
    writeWeight = true;
    writeError = true;
    break;
  }

  // --- Initialize the combined event arrays ----
  if (writeTOF)
    tofs = new double[num];
  if (writeWeight)
    weights = new float[num];
  if (writeError)
    errorSquareds = new float[num];
  if (writePulsetime)
    pulsetimes = new int64_t[num];

  // --- Fill in the combined event arrays ----
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int wi = 0;
       wi < static_cast<int>(m_eventWorkspace->getNumberHistograms()); wi++) {
    PARALLEL_START_INTERUPT_REGION
    const DataObjects::EventList &el = m_eventWorkspace->getSpectrum(wi);

    // This is where it will land in the output array.
    // It is okay to write in parallel since none should step on each other.
    size_t offset = indices[wi];

    switch (el.getEventType()) {
    case TOF:
      appendEventListData(el.getEvents(), offset, tofs, weights, errorSquareds,
                          pulsetimes);
      break;
    case WEIGHTED:
      appendEventListData(el.getWeightedEvents(), offset, tofs, weights,
                          errorSquareds, pulsetimes);
      break;
    case WEIGHTED_NOTIME:
      appendEventListData(el.getWeightedEventsNoTime(), offset, tofs, weights,
                          errorSquareds, pulsetimes);
      break;
    }
    m_progress->reportIncrement(el.getNumberEvents(), "Copying EventList");

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  /*Default = DONT compress - much faster*/
  bool CompressNexus = getProperty("CompressNexus");

  // Write out to the NXS file.
  nexusFile->writeNexusProcessedDataEventCombined(m_eventWorkspace, indices,
                                                  tofs, weights, errorSquareds,
                                                  pulsetimes, CompressNexus);

  // Free mem.
  delete[] tofs;
  delete[] weights;
  delete[] errorSquareds;
  delete[] pulsetimes;
}

//-----------------------------------------------------------------------------------------------
/** virtual method to set the non workspace properties for this algorithm
 *  @param alg :: pointer to the algorithm
 *  @param propertyName :: name of the property
 *  @param propertyValue :: value  of the property
 *  @param perioidNum :: period number
 */
void SaveNexusProcessed::setOtherProperties(IAlgorithm *alg,
                                            const std::string &propertyName,
                                            const std::string &propertyValue,
                                            int perioidNum) {
  if (propertyName == "Append") {
    if (perioidNum != 1) {
      alg->setPropertyValue(propertyName, "1");
    } else
      alg->setPropertyValue(propertyName, propertyValue);
  } else
    Algorithm::setOtherProperties(alg, propertyName, propertyValue, perioidNum);
}

/**
 Overriden process groups.
 */
bool SaveNexusProcessed::processGroups() {
  // Then immediately open the file
  auto nexusFile = boost::make_shared<Mantid::NeXus::NexusFileIO>();

  /* Unless we have explicity been asked to append to the file. We should assume
  that we can remove any existing
  files of the same name prior to processing. */
  bool append_to_file = this->getProperty("Append");
  if (!append_to_file) {
    const std::string filename = getPropertyValue("Filename");
    Poco::File file(filename);
    if (file.exists()) {
      file.remove();
    }
  }

  // Only the input workspace property can take group workspaces. Therefore
  // index = 0.
  std::vector<Workspace_sptr> &thisGroup = m_groups[0];
  if (!thisGroup.empty()) {
    for (size_t entry = 0; entry < m_groupSize; entry++) {
      Workspace_sptr ws = thisGroup[entry];
      this->doExec(ws, nexusFile, true /*keepFile*/, entry);
      std::stringstream buffer;
      buffer << "Saving group index " << entry;
      m_log.information(buffer.str());
    }
  }

  nexusFile->closeNexusFile();

  return true;
}

/** Save the spectra detector map to an open NeXus file.
 * @param ws :: Workspace containing spectrum data
 * @param file :: open NeXus file
 * @param spec :: list of the Workspace Indices to save.
 * @param compression :: NXcompression int to indicate how to compress
 */
void SaveNexusProcessed::saveSpectraMapNexus(
    const MatrixWorkspace &ws, ::NeXus::File *file,
    const std::vector<int> &spec,
    const ::NeXus::NXcompression compression) const {
  // Count the total number of detectors
  std::size_t nDetectors = 0;
  for (auto index : spec) {
    nDetectors +=
        ws.getSpectrum(static_cast<size_t>(index)).getDetectorIDs().size();
  }

  if (nDetectors < 1) {
    // No data in spectraMap to write
    g_log.warning("No spectramap data to write");
    return;
  }

  // Start the detector group
  file->makeGroup("detector", "NXdetector", true);
  file->putAttr("version", 1);

  int numberSpec = int(spec.size());
  // allocate space for the Nexus Muon format of spctra-detector mapping
  std::vector<int32_t> detector_index(
      numberSpec + 1, 0); // allow for writing one more than required
  std::vector<int32_t> detector_count(numberSpec, 0);
  std::vector<int32_t> detector_list(nDetectors, 0);
  std::vector<int32_t> spectra(numberSpec, 0);
  std::vector<double> detPos(nDetectors * 3);
  detector_index[0] = 0;
  int id = 0;

  int ndet = 0;
  // get data from map into Nexus Muon format
  for (int i = 0; i < numberSpec; i++) {
    // Workspace index
    int si = spec[i];
    // Spectrum there
    const auto &spectrum = ws.getSpectrum(si);
    spectra[i] = int32_t(spectrum.getSpectrumNo());

    // The detectors in this spectrum
    const auto &detectorgroup = spectrum.getDetectorIDs();
    const int ndet1 = static_cast<int>(detectorgroup.size());

    detector_index[i + 1] = int32_t(
        detector_index[i] +
        ndet1); // points to start of detector list for the next spectrum
    detector_count[i] = int32_t(ndet1);
    ndet += ndet1;

    std::set<detid_t>::const_iterator it;
    for (it = detectorgroup.begin(); it != detectorgroup.end(); ++it) {
      detector_list[id++] = int32_t(*it);
    }
  }
  // Cut the extra entry at the end of detector_index
  detector_index.resize(numberSpec);

  // write data as Nexus sections detector{index,count,list}
  std::vector<int> dims(1, numberSpec);
  file->writeCompData("detector_index", detector_index, dims, compression,
                      dims);
  file->writeCompData("detector_count", detector_count, dims, compression,
                      dims);
  dims[0] = ndet;
  file->writeCompData("detector_list", detector_list, dims, compression, dims);
  dims[0] = numberSpec;
  file->writeCompData("spectra", spectra, dims, compression, dims);

  // Get all the positions
  try {
    Geometry::Instrument_const_sptr inst = ws.getInstrument();
    Geometry::IComponent_const_sptr sample = inst->getSample();
    if (sample) {
      Kernel::V3D sample_pos = sample->getPos();
      for (int i = 0; i < ndet; i++) {
        double R, Theta, Phi;
        try {
          Geometry::IDetector_const_sptr det =
              inst->getDetector(detector_list[i]);
          Kernel::V3D pos = det->getPos() - sample_pos;
          pos.getSpherical(R, Theta, Phi);
          R = det->getDistance(*sample);
          Theta = ws.detectorTwoTheta(*det) * Geometry::rad2deg;
        } catch (...) {
          R = 0.;
          Theta = 0.;
          Phi = 0.;
        }
        // Need to get R & Theta through these methods to be correct for grouped
        // detectors
        detPos[3 * i] = R;
        detPos[3 * i + 1] = Theta;
        detPos[3 * i + 2] = Phi;
      }
    } else
      for (int i = 0; i < 3 * ndet; i++)
        detPos[i] = 0.;

    dims[0] = ndet;
    dims.push_back(3);
    dims[1] = 3;
    file->writeCompData("detector_positions", detPos, dims, compression, dims);
  } catch (...) {
    g_log.error("Unknown error caught when saving detector positions.");
  }

  file->closeGroup();
}

} // namespace DataHandling
} // namespace Mantid
