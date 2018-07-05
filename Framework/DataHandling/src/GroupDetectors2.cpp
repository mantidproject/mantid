#include "MantidDataHandling/GroupDetectors2.h"

#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadDetectorsGroupingFile.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidHistogramData/HistogramMath.h"
#include "MantidIndexing/Group.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/SpectrumNumber.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/Strings.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(GroupDetectors2)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using std::size_t;

namespace { // anonymous namespace

/**
 * Translate the PerformIndexOperations processing instructions from a vector
 * into a format usable by GroupDetectors.
 *
 * @param groups : A vector of groups, each group being a vector of its 0-based
 * spectrum indices
 * @param axis : The spectra axis of the workspace
 * @param commands : A stringstream to be filled
 */
void convertGroupsToMapFile(const std::vector<std::vector<int>> &groups,
                            const SpectraAxis &axis,
                            std::stringstream &commands) {
  // The input gives the groups as a vector of a vector of ints. Turn
  // this into a string, just like the contents of a map file.
  commands << groups.size() << "\n";
  for (auto &group : groups) {
    const int groupId = axis.spectraNo(group[0]);
    const int groupSize = static_cast<int>(group.size());

    // Comment the output for readability
    commands << "# Group " << groupId;
    commands << ", contains " << groupSize << " spectra.\n";

    commands << groupId << "\n";
    commands << groupSize << "\n";

    // Group members
    // The input is in 0-indexed workspace ids, but the mapfile syntax expects
    // spectrum ids
    for (size_t j = 0; j < group.size(); ++j) {
      commands << (j > 0 ? " " : "") << axis.spectraNo(group[j]);
    }
    commands << "\n";
  }
}

/**
 * Replace the vertical axis to by a SpectraAxis.
 * @param ws a workspace
 */
void forceSpectraAxis(MatrixWorkspace &ws) {
  if (dynamic_cast<SpectraAxis *>(ws.getAxis(1))) {
    return;
  }
  ws.replaceAxis(1, new SpectraAxis(&ws));
}

} // anonymous namespace

// progress estimates
const double GroupDetectors2::CHECKBINS = 0.10;
const double GroupDetectors2::OPENINGFILE = 0.03;
// if CHECKBINS+OPENINGFILE+2*READFILE > 1 then the algorithm might report
// progress > 100%
const double GroupDetectors2::READFILE = 0.15;

void GroupDetectors2::init() {
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<CommonBinsValidator>()),
                  "The name of the input 2D workspace");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace");

  const std::vector<std::string> exts{".map", ".xml"};
  declareProperty(
      Kernel::make_unique<FileProperty>("MapFile", "",
                                        FileProperty::OptionalLoad, exts),
      "A file that consists of lists of spectra numbers to group. See the "
      "help\n"
      "for the file format");
  declareProperty(
      "IgnoreGroupNumber", true,
      "This option is only relevant if you're using MapFile.\n"
      "If true the spectra will numbered sequentially, starting from one.\n"
      "Otherwise, the group number will be used for the spectrum numbers.");
  declareProperty(make_unique<PropertyWithValue<std::string>>(
                      "GroupingPattern", "", Direction::Input),
                  "Describes how this algorithm should group the detectors. "
                  "See full instruction list.");
  declareProperty(
      make_unique<ArrayProperty<specnum_t>>("SpectraList"),
      "An array containing a list of the spectrum numbers to combine\n"
      "(DetectorList and WorkspaceIndexList are ignored if this is set)");
  declareProperty(make_unique<ArrayProperty<detid_t>>("DetectorList"),
                  "An array of detector IDs to combine (WorkspaceIndexList is "
                  "ignored if this is\n"
                  "set)");
  declareProperty(make_unique<ArrayProperty<size_t>>("WorkspaceIndexList"),
                  "An array of workspace indices to combine");
  declareProperty(
      "KeepUngroupedSpectra", false,
      "If true ungrouped spectra will be copied to the output workspace\n"
      "and placed after the groups");

  std::vector<std::string> groupTypes(2);
  groupTypes[0] = "Sum";
  groupTypes[1] = "Average";
  using Mantid::Kernel::StringListValidator;
  declareProperty(
      "Behaviour", "Sum", boost::make_shared<StringListValidator>(groupTypes),
      "Whether to sum or average the values when grouping detectors.");
  // Are we preserving event workspaces?
  declareProperty("PreserveEvents", true,
                  "Keep the output workspace as an "
                  "EventWorkspace, if the input has "
                  "events.");
  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "CopyGroupingFromWorkspace", "", Direction::Input,
          PropertyMode::Optional),
      "The name of a workspace to copy the grouping from.\n "
      "This can be either a normal workspace or a grouping workspace, but they "
      "must be from the same instrument.\n"
      "Detector ids are used to match up the spectra to be grouped.\n"
      "If this option is selected all file and list options will be ignored.");
}

void GroupDetectors2::exec() {
  // Get the input workspace
  const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Check if it is an event workspace
  const bool preserveEvents = getProperty("PreserveEvents");
  EventWorkspace_const_sptr eventW =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventW != nullptr && preserveEvents) {
    this->execEvent();
    return;
  }

  const size_t numInHists = inputWS->getNumberHistograms();
  // Bin boundaries need to be the same, so do the full check on whether they
  // actually are
  if (!API::WorkspaceHelpers::commonBoundaries(*inputWS)) {
    g_log.error()
        << "Can only group if the histograms have common bin boundaries\n";
    throw std::invalid_argument(
        "Can only group if the histograms have common bin boundaries");
  }
  progress(m_FracCompl = CHECKBINS);
  interruption_point();

  // some values loaded into this vector can be negative so this needs to be a
  // signed type
  std::vector<int64_t> unGroupedInds;
  // the ungrouped list could be very big but might be none at all
  unGroupedInds.reserve(numInHists);
  for (size_t i = 0; i < numInHists; i++) {
    unGroupedInds.push_back(i);
  }

  getGroups(inputWS, unGroupedInds);

  // converting the list into a set gets rid of repeated values, here the
  // multiple GroupDetectors2::USED become one USED at the start
  const std::set<int64_t> unGroupedSet(unGroupedInds.begin(),
                                       unGroupedInds.end());

  // Check what the user asked to be done with ungrouped spectra
  const bool keepAll = getProperty("KeepUngroupedSpectra");
  // ignore the one USED value in set or ignore all the ungrouped if the user
  // doesn't want them
  const size_t numUnGrouped = keepAll ? unGroupedSet.size() - 1 : 0;

  auto outputWS = boost::dynamic_pointer_cast<Workspace2D>(
      WorkspaceFactory::Instance().create(
          inputWS, m_GroupWsInds.size() + numUnGrouped, inputWS->x(0).size(),
          inputWS->blocksize()));
  // The cast might fail if the input is a WorkspaceSingleValue. That does not
  // seem to make sense for this algorithm, so we throw.
  if (!outputWS)
    throw std::invalid_argument(
        "Input workspace must be an EventWorkspace or Workspace2D");

  // prepare to move the requested histograms into groups, first estimate how
  // long for progress reporting. +1 in the demonator gets rid of any divide by
  // zero risk
  double prog4Copy =
      ((1.0 - m_FracCompl) /
       (static_cast<double>(numInHists - unGroupedSet.size()) + 1.)) *
      (keepAll ? static_cast<double>(numInHists - unGroupedSet.size()) /
                     static_cast<double>(numInHists)
               : 1.);

  // Build a new map
  auto indexInfo = Indexing::IndexInfo(0);
  const size_t outIndex = formGroups(inputWS, outputWS, prog4Copy, keepAll,
                                     unGroupedSet, indexInfo);

  // If we're keeping ungrouped spectra
  if (keepAll) {
    // copy them into the output workspace
    moveOthers(unGroupedSet, *inputWS, *outputWS, outIndex);
  }

  outputWS->setIndexInfo(indexInfo);

  // Make sure output workspace has spectra axis.
  // Numeric axis copied from the input workspace would be initialized with
  // zeros only and contain no information in it.
  forceSpectraAxis(*outputWS);
  setProperty("OutputWorkspace", outputWS);
}

void GroupDetectors2::execEvent() {
  // Get the input workspace
  const MatrixWorkspace_const_sptr matrixInputWS =
      getProperty("InputWorkspace");
  EventWorkspace_const_sptr inputWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(matrixInputWS);

  const size_t numInHists = inputWS->getNumberHistograms();
  progress(m_FracCompl = CHECKBINS);
  interruption_point();

  // some values loaded into this vector can be negative so this needs to be a
  // signed type
  std::vector<int64_t> unGroupedInds;
  // the ungrouped list could be very big but might be none at all
  unGroupedInds.reserve(numInHists);
  for (size_t i = 0; i < numInHists; i++) {
    unGroupedInds.push_back(i);
  }

  // read in the input parameters to make that map, if KeepUngroupedSpectra was
  // set we'll need a list of the ungrouped spectrra too
  getGroups(inputWS, unGroupedInds);

  // converting the list into a set gets rid of repeated values, here the
  // multiple GroupDetectors2::USED become one USED at the start
  const std::set<int64_t> unGroupedSet(unGroupedInds.begin(),
                                       unGroupedInds.end());

  // Check what the user asked to be done with ungrouped spectra
  const bool keepAll = getProperty("KeepUngroupedSpectra");
  // ignore the one USED value in set or ignore all the ungrouped if the user
  // doesn't want them
  const size_t numUnGrouped = keepAll ? unGroupedSet.size() - 1 : 0;

  // Make a brand new EventWorkspace
  EventWorkspace_sptr outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
      WorkspaceFactory::Instance().create(
          "EventWorkspace", m_GroupWsInds.size() + numUnGrouped,
          inputWS->x(0).size(), inputWS->blocksize()));
  // Copy geometry over.
  WorkspaceFactory::Instance().initializeFromParent(*inputWS, *outputWS, true);

  // prepare to move the requested histograms into groups, first estimate how
  // long for progress reporting. +1 in the demonator gets rid of any divide by
  // zero risk
  double prog4Copy =
      ((1.0 - m_FracCompl) /
       (static_cast<double>(numInHists - unGroupedSet.size()) + 1.)) *
      (keepAll ? static_cast<double>(numInHists - unGroupedSet.size()) /
                     static_cast<double>(numInHists)
               : 1.);

  // Build a new map
  const size_t outIndex = formGroupsEvent(inputWS, outputWS, prog4Copy);

  // If we're keeping ungrouped spectra
  if (keepAll) {
    // copy them into the output workspace
    moveOthers(unGroupedSet, *inputWS, *outputWS, outIndex);
  }

  // Set all X bins on the output
  outputWS->setAllX(inputWS->binEdges(0));

  // Make sure output workspace has spectra axis.
  // Numeric axis copied from the input workspace would be initialized with
  // zeros only and contain no information in it.
  forceSpectraAxis(*outputWS);
  setProperty("OutputWorkspace", outputWS);
}

/** Make a map containing spectra indexes to group, the indexes could have come
 *  from a file, or an array, spectra numbers ...
 *  @param workspace :: the user selected input workspace
 *  @param unUsedSpec :: spectra indexes that are not members of any group
 */
void GroupDetectors2::getGroups(API::MatrixWorkspace_const_sptr workspace,
                                std::vector<int64_t> &unUsedSpec) {
  // this is the map that we are going to fill
  m_GroupWsInds.clear();

  // There are several properties that may contain the user data go through them
  // in order of precedence
  // copy grouping from a workspace
  const MatrixWorkspace_const_sptr groupingWS_sptr =
      getProperty("CopyGroupingFromWorkspace");
  if (groupingWS_sptr) {
    DataObjects::GroupingWorkspace_const_sptr groupWS =
        boost::dynamic_pointer_cast<const DataObjects::GroupingWorkspace>(
            groupingWS_sptr);
    if (groupWS) {
      g_log.debug() << "Extracting grouping from GroupingWorkspace ("
                    << groupWS->getName() << ")\n";
      processGroupingWorkspace(groupWS, workspace, unUsedSpec);
    } else {
      g_log.debug() << "Extracting grouping from MatrixWorkspace ("
                    << groupingWS_sptr->getName() << ")\n";
      processMatrixWorkspace(groupingWS_sptr, workspace, unUsedSpec);
    }
    return;
  }

  // grouping described in a file
  const std::string filename = getProperty("MapFile");
  if (!filename
           .empty()) { // The file property has been set so try to load the file
    try {
      // check if XML file and if yes assume it is a XML grouping file
      std::string filenameCopy(filename);
      std::transform(filenameCopy.begin(), filenameCopy.end(),
                     filenameCopy.begin(), tolower);
      if ((filenameCopy.find(".xml")) != std::string::npos) {
        processXMLFile(filename, workspace, unUsedSpec);
      } else {
        // the format of this input file format is described in
        // "GroupDetectors2.h"
        processFile(filename, workspace, unUsedSpec);
      }
    } catch (std::exception &) {
      g_log.error() << name() << ": Error reading input file " << filename
                    << '\n';
      throw;
    }
    return;
  }

  const std::string instructions = getProperty("GroupingPattern");
  if (!instructions.empty()) {
    const SpectraAxis axis(workspace.get());
    const auto specs2index = axis.getSpectraIndexMap();

    // Translate the instructions into a vector of groups
    auto groups = Kernel::Strings::parseGroups<int>(instructions);
    // Fill commandsSS with the contents of a map file
    std::stringstream commandsSS;
    convertGroupsToMapFile(groups, axis, commandsSS);
    // readFile expects the first line to have already been removed, so we do
    // that, even though we don't use it.
    std::string firstLine;
    std::getline(commandsSS, firstLine);
    // We don't use lineNum either, but it's expected.
    size_t lineNum = 0;
    readFile(specs2index, commandsSS, lineNum, unUsedSpec,
             /* don't ignore group numbers */ false);
    return;
  }

  // manually specified grouping
  const std::vector<specnum_t> spectraList = getProperty("SpectraList");
  const std::vector<detid_t> detectorList = getProperty("DetectorList");
  const std::vector<size_t> indexList = getProperty("WorkspaceIndexList");

  // only look at these other parameters if the file wasn't set
  if (!spectraList.empty()) {
    m_GroupWsInds[0] = workspace->getIndicesFromSpectra(spectraList);
    g_log.debug() << "Converted " << spectraList.size()
                  << " spectra numbers into spectra indices to be combined\n";
  } else { // go through the rest of the properties in order of decreasing
           // presidence, abort when we get the data we need ignore the rest
    if (!detectorList.empty()) {
      // we are going to group on the basis of detector IDs, convert from
      // detectors to workspace indices
      m_GroupWsInds[0] = workspace->getIndicesFromDetectorIDs(detectorList);
      g_log.debug() << "Found " << m_GroupWsInds[0].size()
                    << " spectra indices from the list of "
                    << detectorList.size() << " detectors\n";
    } else if (!indexList.empty()) {
      m_GroupWsInds[0] = indexList;
      g_log.debug() << "Read in " << m_GroupWsInds[0].size()
                    << " spectra indices to be combined\n";
    }
    // check we don't have an index that is too high for the workspace
    size_t maxIn = static_cast<size_t>(workspace->getNumberHistograms() - 1);
    auto indices0 = m_GroupWsInds[0];
    auto it = indices0.begin();
    for (; it != indices0.end(); ++it) {
      if (*it > maxIn) {
        g_log.error() << "Spectra index " << *it
                      << " doesn't exist in the input workspace, the highest "
                         "possible index is "
                      << maxIn << '\n';
        throw std::out_of_range("One of the spectra requested to group does "
                                "not exist in the input workspace");
      }
    }
  }

  if (m_GroupWsInds[0].empty()) {
    g_log.information() << name()
                        << ": File, WorkspaceIndexList, SpectraList, "
                           "and DetectorList properties are all "
                           "empty\n";
    throw std::invalid_argument(
        "All list properties are empty, nothing to group");
  }

  // up date unUsedSpec, this is used to find duplicates and when the user has
  // set KeepUngroupedSpectra
  auto indices0 = m_GroupWsInds[0];
  auto index = indices0.begin();
  for (; index != indices0.end();
       ++index) { // the vector<int> m_GroupWsInds[0] must not index contain
                  // numbers that don't exist in the workspaace
    if (unUsedSpec[*index] != USED) {
      unUsedSpec[*index] = USED;
    } else
      g_log.warning() << "Duplicate index, " << *index << ", found\n";
  }
}
/** Read the spectra numbers in from the input file (the file format is in the
 *  source file "GroupDetectors2.h" and make an array of spectra indexes to
 * group
 *  @param fname :: the full path name of the file to open
 *  @param workspace :: a pointer to the input workspace, used to get spectra
 * indexes from numbers
 *  @param unUsedSpec :: the list of spectra indexes that have been included in
 * a group (so far)
 *  @throw FileError if there's any problem with the file or its format
 */
void GroupDetectors2::processFile(const std::string &fname,
                                  API::MatrixWorkspace_const_sptr workspace,
                                  std::vector<int64_t> &unUsedSpec) {
  // tring to open the file the user told us exists, skip down 20 lines to find
  // out what happens if we can read from it
  g_log.debug() << "Opening input file ... " << fname;
  std::ifstream File(fname.c_str(), std::ios::in);

  std::string firstLine;
  std::getline(File, firstLine);
  // for error reporting keep a count of where we are reading in the file
  size_t lineNum = 1;

  if (File.fail()) {
    g_log.debug() << " file state failbit set after read attempt\n";
    throw Exception::FileError("Couldn't read file", fname);
  }
  g_log.debug() << " success opening input file " << fname << '\n';
  progress(m_FracCompl += OPENINGFILE);
  // check for a (user) cancel message
  interruption_point();

  // allow spectra number to spectra index look ups
  spec2index_map specs2index;
  const SpectraAxis *axis =
      dynamic_cast<const SpectraAxis *>(workspace->getAxis(1));
  if (axis) {
    specs2index = axis->getSpectraIndexMap();
  }

  try {
    // we don't use the total number of groups report at the top of the file but
    // we'll tell them later if there is a problem with it for their diagnostic
    // purposes
    int totalNumberOfGroups = readInt(firstLine);

    // Reading file now ...
    while (totalNumberOfGroups == EMPTY_LINE) {
      if (!File)
        throw Exception::FileError(
            "The input file doesn't appear to contain any data", fname);
      std::getline(File, firstLine), lineNum++;
      totalNumberOfGroups = readInt(firstLine);
    }

    bool ignoreGroupNo = getProperty("IgnoreGroupNumber");
    readFile(specs2index, File, lineNum, unUsedSpec, ignoreGroupNo);

    if (m_GroupWsInds.size() != static_cast<size_t>(totalNumberOfGroups)) {
      g_log.warning() << "The input file header states there are "
                      << totalNumberOfGroups << " but the file contains "
                      << m_GroupWsInds.size() << " groups\n";
    }
  }
  // add some more info to the error messages, including the line number, to
  // help users correct their files. These problems should cause the algorithm
  // to stop
  catch (std::invalid_argument &e) {
    g_log.debug() << "Exception thrown: " << e.what() << '\n';
    File.close();
    std::string error(e.what() + std::string(" near line number ") +
                      std::to_string(lineNum));
    if (File.fail()) {
      error = "Input output error while reading file ";
    }
    throw Exception::FileError(error, fname);
  } catch (boost::bad_lexical_cast &e) {
    g_log.debug() << "Exception thrown: " << e.what() << '\n';
    File.close();
    std::string error(std::string("Problem reading integer value \"") +
                      e.what() + std::string("\" near line number ") +
                      std::to_string(lineNum));
    if (File.fail()) {
      error = "Input output error while reading file ";
    }
    throw Exception::FileError(error, fname);
  }
  File.close();
  g_log.debug() << "Closed file " << fname << " after reading in "
                << m_GroupWsInds.size() << " groups\n";
  m_FracCompl += fileReadProg(m_GroupWsInds.size(), specs2index.size());
}

/** Get groupings from XML file
 *  @param fname :: the full path name of the file to open
 *  @param workspace :: a pointer to the input workspace, used to get spectra
 * indexes from numbers
 *  @param unUsedSpec :: the list of spectra indexes that have been included in
 * a group (so far)
 *  @throw FileError if there's any problem with the file or its format
 */
void GroupDetectors2::processXMLFile(const std::string &fname,
                                     API::MatrixWorkspace_const_sptr workspace,
                                     std::vector<int64_t> &unUsedSpec) {
  // 1. Get maps for spectrum No and detector ID
  spec2index_map specs2index;
  const SpectraAxis *axis =
      dynamic_cast<const SpectraAxis *>(workspace->getAxis(1));
  if (axis) {
    specs2index = axis->getSpectraIndexMap();
  }

  const detid2index_map detIdToWiMap =
      workspace->getDetectorIDToWorkspaceIndexMap();

  // 2. Load XML file
  DataHandling::LoadGroupXMLFile loader;
  loader.setDefaultStartingGroupID(0);
  loader.loadXMLFile(fname);
  std::map<int, std::vector<detid_t>> mGroupDetectorsMap =
      loader.getGroupDetectorsMap();
  std::map<int, std::vector<int>> mGroupSpectraMap =
      loader.getGroupSpectraMap();

  // 3. Build m_GroupWsInds
  for (const auto &det : mGroupDetectorsMap) {
    m_GroupWsInds.emplace(det.first, std::vector<size_t>());
  }

  // 4. Detector IDs
  for (const auto &det : mGroupDetectorsMap) {
    int groupid = det.first;
    const std::vector<detid_t> &detids = det.second;

    auto sit = m_GroupWsInds.find(groupid);
    if (sit == m_GroupWsInds.end())
      continue;

    std::vector<size_t> &wsindexes = sit->second;

    for (auto detid : detids) {
      auto ind = detIdToWiMap.find(detid);
      if (ind != detIdToWiMap.end()) {
        size_t wsid = ind->second;
        wsindexes.push_back(wsid);
        if (unUsedSpec[wsid] != (USED)) {
          unUsedSpec[wsid] = (USED);
        }
      } else {
        g_log.error() << "Detector with ID " << detid
                      << " is not found in instrument \n";
      }
    } // for index
  }   // for group

  // 5. Spectrum Nos
  for (const auto &pit : mGroupSpectraMap) {
    int groupid = pit.first;
    const std::vector<int> &spectra = pit.second;

    auto sit = m_GroupWsInds.find(groupid);
    if (sit == m_GroupWsInds.end())
      continue;

    std::vector<size_t> &wsindexes = sit->second;

    for (auto specNum : spectra) {
      auto ind = specs2index.find(specNum);
      if (ind != specs2index.end()) {
        size_t wsid = ind->second;
        wsindexes.push_back(wsid);
        if (unUsedSpec[wsid] != (USED)) {
          unUsedSpec[wsid] = (USED);
        }
      } else {
        g_log.error() << "Spectrum with ID " << specNum
                      << " is not found in instrument \n";
      }
    } // for index
  }   // for group
}

/** Get groupings from groupingworkspace
 *  @param groupWS :: the grouping workspace to use
 *  @param workspace :: a pointer to the input workspace, used to get spectra
 * indexes from numbers
 *  @param unUsedSpec :: the list of spectra indexes that have been not included
 * in a group (so far)
 */
void GroupDetectors2::processGroupingWorkspace(
    GroupingWorkspace_const_sptr groupWS,
    API::MatrixWorkspace_const_sptr workspace,
    std::vector<int64_t> &unUsedSpec) {
  detid2index_map detIdToWiMap = workspace->getDetectorIDToWorkspaceIndexMap();

  using Group2SetMapType = std::map<size_t, std::set<size_t>>;
  Group2SetMapType group2WSIndexSetmap;

  const auto &spectrumInfo = groupWS->spectrumInfo();
  const auto &detectorIDs = groupWS->detectorInfo().detectorIDs();
  for (size_t i = 0; i < spectrumInfo.size(); ++i) {
    // read spectra from groupingws
    size_t groupid = static_cast<int>(groupWS->y(i)[0]);
    // group 0 is are unused spectra - don't process them
    if (groupid > 0) {
      if (group2WSIndexSetmap.find(groupid) == group2WSIndexSetmap.end()) {
        // not found - create an empty set
        group2WSIndexSetmap.emplace(groupid, std::set<size_t>());
      }
      // get a reference to the set
      std::set<size_t> &targetWSIndexSet = group2WSIndexSetmap[groupid];
      for (const auto &spectrumDefinition :
           spectrumInfo.spectrumDefinition(i)) {
        // translate detectors to target det ws indexes
        size_t targetWSIndex =
            detIdToWiMap[detectorIDs[spectrumDefinition.first]];
        targetWSIndexSet.insert(targetWSIndex);
        // mark as used
        if (unUsedSpec[targetWSIndex] != (USED)) {
          unUsedSpec[targetWSIndex] = (USED);
        }
      }
    }
  }

  // Build m_GroupWsInds (group -> list of ws indices)
  for (auto &dit : group2WSIndexSetmap) {
    size_t groupid = dit.first;
    std::set<size_t> &targetWSIndexSet = dit.second;
    m_GroupWsInds.emplace(
        static_cast<specnum_t>(groupid),
        std::vector<size_t>(targetWSIndexSet.begin(), targetWSIndexSet.end()));
  }
}

/** Get groupings from a matrix workspace
 *  @param groupWS :: the matrix workspace to use
 *  @param workspace :: a pointer to the input workspace, used to get spectra
 * indexes from numbers
 *  @param unUsedSpec :: the list of spectra indexes that have been not included
 * in
 * a group (so far)
 */
void GroupDetectors2::processMatrixWorkspace(
    MatrixWorkspace_const_sptr groupWS, MatrixWorkspace_const_sptr workspace,
    std::vector<int64_t> &unUsedSpec) {
  detid2index_map detIdToWiMap = workspace->getDetectorIDToWorkspaceIndexMap();

  using Group2SetMapType = std::map<size_t, std::set<size_t>>;
  Group2SetMapType group2WSIndexSetmap;

  const auto &spectrumInfo = groupWS->spectrumInfo();
  const auto &detectorIDs = groupWS->detectorInfo().detectorIDs();
  for (size_t i = 0; i < spectrumInfo.size(); ++i) {
    // read spectra from groupingws
    size_t groupid = i;

    if (group2WSIndexSetmap.find(groupid) == group2WSIndexSetmap.end()) {
      // not found - create an empty set
      group2WSIndexSetmap.emplace(groupid, std::set<size_t>());
    }
    // get a reference to the set
    std::set<size_t> &targetWSIndexSet = group2WSIndexSetmap[groupid];

    // If the detector was not found or was not in a group, then ignore it.
    if (spectrumInfo.spectrumDefinition(i).size() > 1) {
      for (const auto &spectrumDefinition :
           spectrumInfo.spectrumDefinition(i)) {
        // translate detectors to target det ws indexes
        size_t targetWSIndex =
            detIdToWiMap[detectorIDs[spectrumDefinition.first]];
        targetWSIndexSet.insert(targetWSIndex);
        // mark as used
        if (unUsedSpec[targetWSIndex] != (USED)) {
          unUsedSpec[targetWSIndex] = (USED);
        }
      }
    }
  }

  // Build m_GroupWsInds (group -> list of ws indices)
  for (auto &dit : group2WSIndexSetmap) {
    size_t groupid = dit.first;
    std::set<size_t> &targetWSIndexSet = dit.second;
    if (!targetWSIndexSet.empty()) {
      std::vector<size_t> tempv;
      tempv.assign(targetWSIndexSet.begin(), targetWSIndexSet.end());
      m_GroupWsInds.insert(
          std::make_pair(static_cast<specnum_t>(groupid), tempv));
    }
  }
}
/** The function expects that the string passed to it contains an integer
 * number,
 *  it reads the number and returns it
 *  @param line :: a line read from the file, we'll interpret this
 *  @return the integer read from the line, error code if not readable
 *  @throw invalid_argument when the line contains more just an integer
 *  @throw boost::bad_lexical_cast when the string can't be interpreted as an
 * integer
 */
int GroupDetectors2::readInt(const std::string &line) {
  // remove comments and white space (TOK_TRIM)
  Mantid::Kernel::StringTokenizer dataComment(
      line, "#", Mantid::Kernel::StringTokenizer::TOK_TRIM);
  if (dataComment.begin() != dataComment.end()) {
    Mantid::Kernel::StringTokenizer data(
        *(dataComment.begin()), " ", Mantid::Kernel::StringTokenizer::TOK_TRIM);
    if (data.count() == 1) {
      if (!data[0].empty()) {
        try {
          return boost::lexical_cast<int>(data[0]);
        } catch (boost::bad_lexical_cast &e) {
          g_log.debug() << "Exception thrown: " << e.what() << '\n';
          throw std::invalid_argument("Error reading file, integer expected");
        }
      }
    } else {
      if (data.count() == 0) {
        return EMPTY_LINE;
      }
      // we expected an integer but there were more things on the line, before
      // any #
      g_log.debug() << "Error: found " << data.count()
                    << " strings the first string is " << data[0] << '\n';
      throw std::invalid_argument(
          "Problem reading file, a singe integer expected");
    }
  }
  // we haven't found any data, return the nodata condition
  return EMPTY_LINE;
}
/** Reads from the file getting in order: an unused integer, on the next line
 * the number of
 *  spectra in the group and next one or more lines the spectra numbers, (format
 * in GroupDetectors.h)
 * @param specs2index :: a map that links spectra numbers to indexes
 * @param File :: the input stream that is linked to the file
 * @param lineNum :: the last line read in the file, is updated by this function
 * @param unUsedSpec :: list of spectra that haven't yet been included in a
 * group
 * @param ignoreGroupNumber :: ignore group numbers when numbering spectra
 * @throw invalid_argument if there is any problem with the file
 */
void GroupDetectors2::readFile(const spec2index_map &specs2index,
                               std::istream &File, size_t &lineNum,
                               std::vector<int64_t> &unUsedSpec,
                               const bool ignoreGroupNumber) {
  // go through the rest of the file reading in lists of spectra number to group
  int oldSpectrumNo = 1;
  while (File) {
    int groupNo = EMPTY_LINE;
    std::string thisLine;
    do {
      std::getline(File, thisLine), lineNum++;
      groupNo = readInt(thisLine);
      // we haven't started reading a new group and so if the file ends here it
      // is OK
      if (!File)
        return;
    } while (groupNo == EMPTY_LINE && File);

    // If we're ignoring the group number, use the old spectrum number way of
    // just counting, otherwise use the group number.
    const int spectrumNo = ignoreGroupNumber ? oldSpectrumNo++ : groupNo;

    // the number of spectra that will be combined in the group
    int numberOfSpectra = EMPTY_LINE;
    do {
      if (!File)
        throw std::invalid_argument("Premature end of file, expecting an "
                                    "integer with the number of spectra in the "
                                    "group");
      std::getline(File, thisLine), lineNum++;
      numberOfSpectra = readInt(thisLine);
    } while (numberOfSpectra == EMPTY_LINE);

    if (numberOfSpectra <= 0) {
      throw std::invalid_argument("The number of spectra is zero or negative");
    }

    // the value of this map is the list of spectra numbers that will be
    // combined into a group
    m_GroupWsInds[spectrumNo].reserve(numberOfSpectra);
    do {
      if (!File)
        throw std::invalid_argument("Premature end of file, found number of "
                                    "spectra specification but no spectra "
                                    "list");
      std::getline(File, thisLine), lineNum++;
      // the spectra numbers that will be included in the group
      readSpectraIndexes(thisLine, specs2index, m_GroupWsInds[spectrumNo],
                         unUsedSpec);
    } while (static_cast<int>(m_GroupWsInds[spectrumNo].size()) <
             numberOfSpectra);
    if (static_cast<int>(m_GroupWsInds[spectrumNo].size()) !=
        numberOfSpectra) { // it makes no sense to continue reading the file,
      // we'll stop here
      throw std::invalid_argument(std::string("Bad number of spectra "
                                              "specification or spectra list "
                                              "near line number ") +
                                  std::to_string(lineNum));
    }
    // make regular progress reports and check for a cancellation notification
    if ((m_GroupWsInds.size() % INTERVAL) == 1) {
      fileReadProg(m_GroupWsInds.size(), specs2index.size());
    }
  }
}
/** The function expects that the string passed to it contains a series of
 *  integers, ranges specified with a '-' are possible
 *  @param line :: a line read from the file, we'll interpret this
 *  @param specs2index :: a map with spectra numbers as indexes and index
 * numbers as values
 *  @param output :: the list of integers, with any ranges expanded
 *  @param unUsedSpec :: the list of spectra indexes that have been included in
 * a group (so far)
 *  @param seperator :: the symbol for the index range separator
 *  @throw invalid_argument when a number couldn't be found or the number is not
 * in the spectra map
 */
void GroupDetectors2::readSpectraIndexes(const std::string &line,
                                         const spec2index_map &specs2index,
                                         std::vector<size_t> &output,
                                         std::vector<int64_t> &unUsedSpec,
                                         const std::string &seperator) {
  // remove comments and white space
  Mantid::Kernel::StringTokenizer dataComment(line, seperator, IGNORE_SPACES);
  for (const auto &itr : dataComment) {
    std::vector<size_t> specNums;
    specNums.reserve(output.capacity());

    RangeHelper::getList(itr, specNums);

    std::vector<size_t>::const_iterator specN = specNums.begin();
    for (; specN != specNums.end(); ++specN) {
      specnum_t spectrumNum = static_cast<specnum_t>(*specN);
      spec2index_map::const_iterator ind = specs2index.find(spectrumNum);
      if (ind == specs2index.end()) {
        g_log.debug() << name() << ": spectrum number " << spectrumNum
                      << " referred to in the input file was not found in the "
                         "input workspace\n";
        throw std::invalid_argument("Spectrum number " +
                                    std::to_string(spectrumNum) + " not found");
      }
      if (unUsedSpec[ind->second] != USED) { // this array is used when the user
                                             // sets KeepUngroupedSpectra, as
                                             // well as to find duplicates
        unUsedSpec[ind->second] = USED;
        output.push_back(ind->second);
      } else { // the spectra was already included in a group
        output.push_back(ind->second);
      }
    }
  }
}

/** Called while reading input file to report progress (doesn't update
 * m_FracCompl ) and
 *  check for algorithm cancel messages, doesn't look at file size to estimate
 * progress
 *  @param numGroupsRead :: number of groups read from the file so far (not the
 * number of spectra)
 *  @param numInHists :: the total number of histograms in the input workspace
 *  @return estimate of the amount of algorithm progress obtained by reading
 * from the file
 */
double GroupDetectors2::fileReadProg(
    DataHandling::GroupDetectors2::storage_map::size_type numGroupsRead,
    DataHandling::GroupDetectors2::storage_map::size_type numInHists) {
  // I'm going to guess that there are half as many groups as spectra
  double progEstim =
      2. * static_cast<double>(numGroupsRead) / static_cast<double>(numInHists);
  // but it might be more, in which case this complex function always increases
  // but slower and slower
  progEstim = READFILE * progEstim / (1 + progEstim);
  // now do the reporting
  progress(m_FracCompl + progEstim);
  // check for a (user) cancel message
  interruption_point();
  return progEstim;
}

/**
 *  Move the user selected spectra in the input workspace into groups in the
 * output workspace
 *  @param inputWS :: user selected input workspace for the algorithm
 *  @param outputWS :: user selected output workspace for the algorithm
 *  @param prog4Copy :: the amount of algorithm progress to attribute to moving
 * a single spectra
 *  @param keepAll :: whether or not to keep ungrouped spectra
 *  @param unGroupedSet :: the set of workspace indexes that are left ungrouped
 *  @param indexInfo :: an IndexInfo object that will contain the desired
 * indexing after grouping
 *  @return number of new grouped spectra
 */
size_t GroupDetectors2::formGroups(API::MatrixWorkspace_const_sptr inputWS,
                                   API::MatrixWorkspace_sptr outputWS,
                                   const double prog4Copy, const bool keepAll,
                                   const std::set<int64_t> &unGroupedSet,
                                   Indexing::IndexInfo &indexInfo) {
  // get "Behaviour" string
  const std::string behaviour = getProperty("Behaviour");
  int bhv = 0;
  if (behaviour == "Average")
    bhv = 1;

  API::MatrixWorkspace_sptr beh = API::WorkspaceFactory::Instance().create(
      "Workspace2D", static_cast<int>(m_GroupWsInds.size()), 1, 1);

  g_log.debug() << name() << ": Preparing to group spectra into "
                << m_GroupWsInds.size() << " groups\n";

  // where we are copying spectra to, we start copying to the start of the
  // output workspace
  size_t outIndex = 0;
  // Only used for averaging behaviour. We may have a 1:1 map where a Divide
  // would be waste as it would be just dividing by 1
  bool requireDivide(false);
  const auto &spectrumInfo = inputWS->spectrumInfo();

  auto spectrumGroups = std::vector<std::vector<size_t>>();
  auto spectrumNumbers = std::vector<Indexing::SpectrumNumber>();

  for (storage_map::const_iterator it = m_GroupWsInds.begin();
       it != m_GroupWsInds.end(); ++it) {
    // This is the grouped spectrum
    auto &outSpec = outputWS->getSpectrum(outIndex);

    // The spectrum number of the group is the key
    spectrumNumbers.push_back(it->first);
    // Start fresh with no detector IDs
    outSpec.clearDetectorIDs();

    // Copy over X data from first spectrum, the bin boundaries for all spectra
    // are assumed to be the same here
    outSpec.setSharedX(inputWS->sharedX(0));
    auto outputHistogram = outSpec.histogram();

    // Keep track of number of detectors required for masking
    size_t nonMaskedSpectra(0);
    auto spectrumGroup = std::vector<size_t>();

    for (auto originalWI : it->second) {
      // detectors to add to firstSpecNum
      const auto &inputSpectrum = inputWS->getSpectrum(originalWI);

      outputHistogram += inputSpectrum.histogram();
      outSpec.addDetectorIDs(inputSpectrum.getDetectorIDs());

      if (!isMaskedDetector(spectrumInfo, originalWI))
        ++nonMaskedSpectra;

      spectrumGroup.push_back(originalWI);
    }

    spectrumGroups.push_back(spectrumGroup);

    outSpec.setHistogram(outputHistogram);

    if (nonMaskedSpectra == 0)
      ++nonMaskedSpectra; // Avoid possible divide by zero
    if (!requireDivide)
      requireDivide = (nonMaskedSpectra > 1);
    beh->mutableY(outIndex)[0] = static_cast<double>(nonMaskedSpectra);

    // make regular progress reports and check for cancelling the algorithm
    if (outIndex % INTERVAL == 0) {
      m_FracCompl += INTERVAL * prog4Copy;
      if (m_FracCompl > 1.0)
        m_FracCompl = 1.0;
      progress(m_FracCompl);
      interruption_point();
    }

    outIndex++;
  }

  // Add the ungrouped spectra to IndexInfo, if they are being kept
  if (keepAll) {
    for (const auto originalWI : unGroupedSet) {
      // Negative WIs are intended to be invalid
      if (originalWI < 0)
        continue;

      spectrumGroups.push_back(std::vector<size_t>(1, originalWI));

      auto spectrumNumber = inputWS->getSpectrum(originalWI).getSpectrumNo();
      spectrumNumbers.push_back(spectrumNumber);
    }
  }

  indexInfo = Indexing::group(inputWS->indexInfo(), std::move(spectrumNumbers),
                              spectrumGroups);

  if (bhv == 1 && requireDivide) {
    g_log.debug() << "Running Divide algorithm to perform averaging.\n";
    Mantid::API::IAlgorithm_sptr divide = createChildAlgorithm("Divide");
    divide->initialize();
    divide->setProperty<API::MatrixWorkspace_sptr>("LHSWorkspace", outputWS);
    divide->setProperty<API::MatrixWorkspace_sptr>("RHSWorkspace", beh);
    divide->setProperty<API::MatrixWorkspace_sptr>("OutputWorkspace", outputWS);
    divide->execute();
  }

  g_log.debug() << name() << " created " << outIndex
                << " new grouped spectra\n";
  return outIndex;
}

/**
 *  Move the user selected spectra in the input workspace into groups in the
 * output workspace
 *  @param inputWS :: user selected input workspace for the algorithm
 *  @param outputWS :: user selected output workspace for the algorithm
 *  @param prog4Copy :: the amount of algorithm progress to attribute to moving
 * a single spectra
 *  @return number of new grouped spectra
 */
size_t
GroupDetectors2::formGroupsEvent(DataObjects::EventWorkspace_const_sptr inputWS,
                                 DataObjects::EventWorkspace_sptr outputWS,
                                 const double prog4Copy) {
  if (inputWS->detectorInfo().isScanning())
    throw std::runtime_error("GroupDetectors does not currently support "
                             "EventWorkspaces with detector scans.");

  // get "Behaviour" string
  const std::string behaviour = getProperty("Behaviour");
  int bhv = 0;
  if (behaviour == "Average")
    bhv = 1;

  API::MatrixWorkspace_sptr beh = API::WorkspaceFactory::Instance().create(
      "Workspace2D", static_cast<int>(m_GroupWsInds.size()), 1, 1);

  g_log.debug() << name() << ": Preparing to group spectra into "
                << m_GroupWsInds.size() << " groups\n";

  // where we are copying spectra to, we start copying to the start of the
  // output workspace
  size_t outIndex = 0;
  // Only used for averaging behaviour. We may have a 1:1 map where a Divide
  // would be waste as it would be just dividing by 1
  bool requireDivide(false);
  const auto &spectrumInfo = inputWS->spectrumInfo();
  for (storage_map::const_iterator it = m_GroupWsInds.begin();
       it != m_GroupWsInds.end(); ++it) {
    // This is the grouped spectrum
    EventList &outEL = outputWS->getSpectrum(outIndex);

    // The spectrum number of the group is the key
    outEL.setSpectrumNo(it->first);
    // Start fresh with no detector IDs
    outEL.clearDetectorIDs();

    // the Y values and errors from spectra being grouped are combined in the
    // output spectrum
    // Keep track of number of detectors required for masking
    size_t nonMaskedSpectra(0);
    beh->mutableX(outIndex)[0] = 0.0;
    beh->mutableE(outIndex)[0] = 0.0;
    for (auto originalWI : it->second) {
      const EventList &fromEL = inputWS->getSpectrum(originalWI);
      // Add the event lists with the operator
      outEL += fromEL;

      // detectors to add to the output spectrum
      outEL.addDetectorIDs(fromEL.getDetectorIDs());
      if (!isMaskedDetector(spectrumInfo, originalWI)) {
        ++nonMaskedSpectra;
      }
    }
    if (nonMaskedSpectra == 0)
      ++nonMaskedSpectra; // Avoid possible divide by zero
    if (!requireDivide)
      requireDivide = (nonMaskedSpectra > 1);
    beh->mutableY(outIndex)[0] = static_cast<double>(nonMaskedSpectra);

    // make regular progress reports and check for cancelling the algorithm
    if (outIndex % INTERVAL == 0) {
      m_FracCompl += INTERVAL * prog4Copy;
      if (m_FracCompl > 1.0)
        m_FracCompl = 1.0;
      progress(m_FracCompl);
      interruption_point();
    }
    outIndex++;
  }

  if (bhv == 1 && requireDivide) {
    g_log.debug() << "Running Divide algorithm to perform averaging.\n";
    Mantid::API::IAlgorithm_sptr divide = createChildAlgorithm("Divide");
    divide->initialize();
    divide->setProperty<API::MatrixWorkspace_sptr>("LHSWorkspace", outputWS);
    divide->setProperty<API::MatrixWorkspace_sptr>("RHSWorkspace", beh);
    divide->setProperty<API::MatrixWorkspace_sptr>("OutputWorkspace", outputWS);
    divide->execute();
  }

  g_log.debug() << name() << " created " << outIndex
                << " new grouped spectra\n";
  return outIndex;
}

bool GroupDetectors2::isMaskedDetector(const API::SpectrumInfo &spectrum,
                                       const size_t index) const {
  if (spectrum.hasDetectors(index)) {
    return spectrum.isMasked(index);
  } else {
    // Can't be masked if it doesn't exist
    return false;
  }
}

// RangeHelper
/** Expands any ranges in the input string of non-negative integers, eg. "1 3-5
 * 4" -> "1 3 4 5 4"
 *  @param line :: a line of input that is interpreted and expanded
 *  @param outList :: all integers specified both as ranges and individually in
 * order
 *  @throw invalid_argument if a character is found that is not an integer or
 * hypehn and when a hyphen occurs at the start or the end of the line
 */
void GroupDetectors2::RangeHelper::getList(const std::string &line,
                                           std::vector<size_t> &outList) {
  if (line.empty()) { // it is not an error to have an empty line but it would
                      // cause problems with an error check a the end of this
                      // function
    return;
  }
  Mantid::Kernel::StringTokenizer ranges(line, "-");

  try {
    size_t loop = 0;
    do {
      Mantid::Kernel::StringTokenizer beforeHyphen(ranges[loop], " ",
                                                   IGNORE_SPACES);
      auto readPostion = beforeHyphen.begin();
      if (readPostion == beforeHyphen.end()) {
        throw std::invalid_argument("'-' found at the start of a list, can't "
                                    "interpret range specification");
      }
      for (; readPostion != beforeHyphen.end(); ++readPostion) {
        outList.push_back(boost::lexical_cast<size_t>(*readPostion));
      }
      // this will be the start of a range if it was followed by a - i.e.
      // another token was captured
      const size_t rangeStart = outList.back();
      if (loop + 1 == ranges.count()) { // there is no more input
        break;
      }

      Mantid::Kernel::StringTokenizer afterHyphen(ranges[loop + 1], " ",
                                                  IGNORE_SPACES);
      readPostion = afterHyphen.begin();
      if (readPostion == afterHyphen.end()) {
        throw std::invalid_argument("A '-' follows straight after another '-', "
                                    "can't interpret range specification");
      }

      // the tokenizer will always return at least on string
      const size_t rangeEnd = boost::lexical_cast<size_t>(*readPostion);

      // this is unanticipated and marked as an error, it would be easy to
      // change this to count down however
      if (rangeStart > rangeEnd) {
        throw std::invalid_argument("A range where the first integer is larger "
                                    "than the second is not allowed");
      }

      // expand the range
      for (size_t j = rangeStart + 1; j < rangeEnd; j++) {
        outList.push_back(j);
      }

      loop++;
    } while (loop < ranges.count());
  } catch (boost::bad_lexical_cast &e) {
    throw std::invalid_argument(
        std::string("Expected list of integers, exception thrown: ") +
        e.what());
  }
  if (*(line.end() - 1) == '-') {
    throw std::invalid_argument(
        "'-' found at the end of a list, can't interpret range specification");
  }
}

/**
 * Used to validate the inputs for GroupDetectors2
 *
 * @returns : A map of the invalid property names to what the problem is.
 */
std::map<std::string, std::string> GroupDetectors2::validateInputs() {
  std::map<std::string, std::string> errors;

  const std::string pattern = getPropertyValue("GroupingPattern");

  boost::regex re(
      R"(^\s*[0-9]+\s*$|^(\s*,*[0-9]+(\s*(,|:|\+|\-)\s*)*[0-9]*)*$)");

  try {
    if (!pattern.empty() && !boost::regex_match(pattern, re)) {
      errors["GroupingPattern"] =
          "GroupingPattern is not well formed: " + pattern;
    }
  } catch (boost::exception &) {
    // If the pattern is too large, split on comma and evaluate each piece.
    auto groups = Kernel::StringTokenizer(pattern, ",", IGNORE_SPACES);
    for (const auto &groupStr : groups) {
      if (!pattern.empty() && !boost::regex_match(groupStr, re)) {
        errors["GroupingPattern"] =
            "GroupingPattern is not well formed: " + pattern;
        break;
      }
    }
  }

  return errors;
}
} // namespace DataHandling
} // namespace Mantid
