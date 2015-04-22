//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/GroupDetectors2.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidDataHandling/LoadDetectorsGroupingFile.h"

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <ios>
#include <set>
#include <vector>
#include <numeric>

#include <Poco/StringTokenizer.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/SAX/ContentHandler.h>
#include <Poco/SAX/Attributes.h>
#include <Poco/SAX/SAXParser.h>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(GroupDetectors2)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using std::size_t;

/// (Empty) Constructor
GroupDetectors2::GroupDetectors2() : m_FracCompl(0.0) {}

/// Destructor
GroupDetectors2::~GroupDetectors2() {}

// progress estimates
const double GroupDetectors2::CHECKBINS = 0.10;
const double GroupDetectors2::OPENINGFILE = 0.03;
// if CHECKBINS+OPENINGFILE+2*READFILE > 1 then the algorithm might report
// progress > 100%
const double GroupDetectors2::READFILE = 0.15;

void GroupDetectors2::init() {
  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<CommonBinsValidator>()),
                  "The name of the input 2D workspace");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "The name of the output workspace");

  std::vector<std::string> fileExts(2);
  fileExts[0] = ".map";
  fileExts[1] = ".xml";
  declareProperty(
      new FileProperty("MapFile", "", FileProperty::OptionalLoad, fileExts),
      "A file that consists of lists of spectra numbers to group. See the "
      "help\n"
      "for the file format");
  declareProperty(
      new ArrayProperty<specid_t>("SpectraList"),
      "An array containing a list of the spectrum numbers to combine\n"
      "(DetectorList and WorkspaceIndexList are ignored if this is set)");
  declareProperty(new ArrayProperty<detid_t>("DetectorList"),
                  "An array of detector IDs to combine (WorkspaceIndexList is "
                  "ignored if this is\n"
                  "set)");
  declareProperty(new ArrayProperty<size_t>("WorkspaceIndexList"),
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
  declareProperty("PreserveEvents", true, "Keep the output workspace as an "
                                          "EventWorkspace, if the input has "
                                          "events.");
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("CopyGroupingFromWorkspace", "",
                                             Direction::Input,
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
  if (eventW != NULL && preserveEvents) {
    this->execEvent();
    return;
  }

  const size_t numInHists = inputWS->getNumberHistograms();
  // Bin boundaries need to be the same, so do the full check on whether they
  // actually are
  if (!API::WorkspaceHelpers::commonBoundaries(inputWS)) {
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

  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(
      inputWS, m_GroupSpecInds.size() + numUnGrouped, inputWS->readX(0).size(),
      inputWS->blocksize());

  // prepare to move the requested histograms into groups, first estimate how
  // long for progress reporting. +1 in the demonator gets rid of any divide by
  // zero risk
  double prog4Copy =
      ((1.0 - m_FracCompl) /
       (static_cast<double>(numInHists - unGroupedSet.size()) + 1.)) *
      (keepAll
           ? static_cast<double>(numInHists - unGroupedSet.size()) /
                 static_cast<double>(numInHists)
           : 1.);

  // Build a new map
  const size_t outIndex = formGroups(inputWS, outputWS, prog4Copy);

  // If we're keeping ungrouped spectra
  if (keepAll) {
    // copy them into the output workspace
    moveOthers(unGroupedSet, inputWS, outputWS, outIndex);
  }

  g_log.information() << name() << " algorithm has finished\n";

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
          "EventWorkspace", m_GroupSpecInds.size() + numUnGrouped,
          inputWS->readX(0).size(), inputWS->blocksize()));
  // Copy geometry over.
  WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, true);

  // prepare to move the requested histograms into groups, first estimate how
  // long for progress reporting. +1 in the demonator gets rid of any divide by
  // zero risk
  double prog4Copy =
      ((1.0 - m_FracCompl) /
       (static_cast<double>(numInHists - unGroupedSet.size()) + 1.)) *
      (keepAll
           ? static_cast<double>(numInHists - unGroupedSet.size()) /
                 static_cast<double>(numInHists)
           : 1.);

  // Build a new map
  const size_t outIndex = formGroupsEvent(inputWS, outputWS, prog4Copy);

  // If we're keeping ungrouped spectra
  if (keepAll) {
    // copy them into the output workspace
    moveOthersEvent(unGroupedSet, inputWS, outputWS, outIndex);
  }

  // Set all X bins on the output
  cow_ptr<MantidVec> XValues;
  XValues.access() = inputWS->readX(0);
  outputWS->setAllX(XValues);

  g_log.information() << name() << " algorithm has finished\n";

  setProperty("OutputWorkspace", outputWS);
}

/** Make a map containing spectra indexes to group, the indexes could have come
* from
*  file, or an array, spectra numbers ...
*  @param workspace :: the user selected input workspace
*  @param unUsedSpec :: spectra indexes that are not members of any group
*/
void GroupDetectors2::getGroups(API::MatrixWorkspace_const_sptr workspace,
                                std::vector<int64_t> &unUsedSpec) {
  // this is the map that we are going to fill
  m_GroupSpecInds.clear();

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
                    << groupWS->name() << ")" << std::endl;
      processGroupingWorkspace(groupWS, workspace, unUsedSpec);
    } else {
      g_log.debug() << "Extracting grouping from MatrixWorkspace ("
                    << groupingWS_sptr->name() << ")" << std::endl;
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
                    << std::endl;
      throw;
    }
    return;
  }

  // manually specified grouping
  const std::vector<specid_t> spectraList = getProperty("SpectraList");
  const std::vector<detid_t> detectorList = getProperty("DetectorList");
  const std::vector<size_t> indexList = getProperty("WorkspaceIndexList");

  // only look at these other parameters if the file wasn't set
  if (!spectraList.empty()) {
    workspace->getIndicesFromSpectra(spectraList, m_GroupSpecInds[0]);
    g_log.debug() << "Converted " << spectraList.size()
                  << " spectra numbers into spectra indices to be combined\n";
  } else { // go through the rest of the properties in order of decreasing
           // presidence, abort when we get the data we need ignore the rest
    if (!detectorList.empty()) {
      // we are going to group on the basis of detector IDs, convert from
      // detectors to workspace indices
      workspace->getIndicesFromDetectorIDs(detectorList, m_GroupSpecInds[0]);
      g_log.debug() << "Found " << m_GroupSpecInds[0].size()
                    << " spectra indices from the list of "
                    << detectorList.size() << " detectors\n";
    } else if (!indexList.empty()) {
      m_GroupSpecInds[0] = indexList;
      g_log.debug() << "Read in " << m_GroupSpecInds[0].size()
                    << " spectra indices to be combined\n";
    }
    // check we don't have an index that is too high for the workspace
    size_t maxIn = static_cast<size_t>(workspace->getNumberHistograms() - 1);
    std::vector<size_t>::const_iterator it = m_GroupSpecInds[0].begin();
    for (; it != m_GroupSpecInds[0].end(); ++it) {
      if (*it > maxIn) {
        g_log.error() << "Spectra index " << *it
                      << " doesn't exist in the input workspace, the highest "
                         "possible index is " << maxIn << std::endl;
        throw std::out_of_range("One of the spectra requested to group does "
                                "not exist in the input workspace");
      }
    }
  }

  if (m_GroupSpecInds[0].empty()) {
    g_log.information() << name() << ": File, WorkspaceIndexList, SpectraList, "
                                     "and DetectorList properties are all "
                                     "empty\n";
    throw std::invalid_argument(
        "All list properties are empty, nothing to group");
  }

  // up date unUsedSpec, this is used to find duplicates and when the user has
  // set KeepUngroupedSpectra
  std::vector<size_t>::const_iterator index = m_GroupSpecInds[0].begin();
  for (; index != m_GroupSpecInds[0].end();
       ++index) { // the vector<int> m_GroupSpecInds[0] must not index contain
                  // numbers that don't exist in the workspaace
    if (unUsedSpec[*index] != USED) {
      unUsedSpec[*index] = USED;
    } else
      g_log.warning() << "Duplicate index, " << *index << ", found\n";
  }
}
/** Read the spectra numbers in from the input file (the file format is in the
*  source file "GroupDetectors2.h" and make an array of spectra indexes to group
*  @param fname :: the full path name of the file to open
*  @param workspace :: a pointer to the input workspace, used to get spectra
* indexes from numbers
*  @param unUsedSpec :: the list of spectra indexes that have been included in a
* group (so far)
*  @throw FileError if there's any problem with the file or its format
*/
void GroupDetectors2::processFile(std::string fname,
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
  g_log.debug() << " success opening input file " << fname << std::endl;
  progress(m_FracCompl += OPENINGFILE);
  // check for a (user) cancel message
  interruption_point();

  // allow spectra number to spectra index look ups
  spec2index_map specs2index;
  const SpectraAxis *axis =
      dynamic_cast<const SpectraAxis *>(workspace->getAxis(1));
  if (axis) {
    axis->getSpectraIndexMap(specs2index);
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

    readFile(specs2index, File, lineNum, unUsedSpec);

    if (m_GroupSpecInds.size() != static_cast<size_t>(totalNumberOfGroups)) {
      g_log.warning() << "The input file header states there are "
                      << totalNumberOfGroups << " but the file contains "
                      << m_GroupSpecInds.size() << " groups\n";
    }
  }
  // add some more info to the error messages, including the line number, to
  // help users correct their files. These problems should cause the algorithm
  // to stop
  catch (std::invalid_argument &e) {
    g_log.debug() << "Exception thrown: " << e.what() << std::endl;
    File.close();
    std::string error(e.what() + std::string(" near line number ") +
                      boost::lexical_cast<std::string>(lineNum));
    if (File.fail()) {
      error = "Input output error while reading file ";
    }
    throw Exception::FileError(error, fname);
  } catch (boost::bad_lexical_cast &e) {
    g_log.debug() << "Exception thrown: " << e.what() << std::endl;
    File.close();
    std::string error(std::string("Problem reading integer value \"") +
                      e.what() + std::string("\" near line number ") +
                      boost::lexical_cast<std::string>(lineNum));
    if (File.fail()) {
      error = "Input output error while reading file ";
    }
    throw Exception::FileError(error, fname);
  }
  File.close();
  g_log.debug() << "Closed file " << fname << " after reading in "
                << m_GroupSpecInds.size() << " groups\n";
  m_FracCompl += fileReadProg(m_GroupSpecInds.size(), specs2index.size());
  return;
}

/** Get groupings from XML file
*  @param fname :: the full path name of the file to open
*  @param workspace :: a pointer to the input workspace, used to get spectra
* indexes from numbers
*  @param unUsedSpec :: the list of spectra indexes that have been included in a
* group (so far)
*  @throw FileError if there's any problem with the file or its format
*/
void GroupDetectors2::processXMLFile(std::string fname,
                                     API::MatrixWorkspace_const_sptr workspace,
                                     std::vector<int64_t> &unUsedSpec) {
  // 1. Get maps for spectrum ID and detector ID
  spec2index_map specs2index;
  const SpectraAxis *axis =
      dynamic_cast<const SpectraAxis *>(workspace->getAxis(1));
  if (axis) {
    axis->getSpectraIndexMap(specs2index);
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

  // 3. Build m_GroupSpecInds
  std::map<int, std::vector<detid_t>>::iterator dit;
  for (dit = mGroupDetectorsMap.begin(); dit != mGroupDetectorsMap.end();
       ++dit) {
    int groupid = dit->first;
    std::vector<size_t> tempv;
    m_GroupSpecInds.insert(std::make_pair(groupid, tempv));
  }

  // 4. Detector IDs
  for (dit = mGroupDetectorsMap.begin(); dit != mGroupDetectorsMap.end();
       ++dit) {
    int groupid = dit->first;
    std::vector<detid_t> detids = dit->second;

    storage_map::iterator sit;
    sit = m_GroupSpecInds.find(groupid);
    if (sit == m_GroupSpecInds.end())
      continue;

    std::vector<size_t> &wsindexes = sit->second;

    for (size_t i = 0; i < detids.size(); i++) {
      detid_t detid = detids[i];
      detid2index_map::const_iterator ind = detIdToWiMap.find(detid);
      if (ind != detIdToWiMap.end()) {
        size_t wsid = ind->second;
        wsindexes.push_back(wsid);
        if (unUsedSpec[wsid] != (USED)) {
          unUsedSpec[wsid] = (USED);
        }
      } else {
        g_log.error() << "Detector with ID " << detid
                      << " is not found in instrument " << std::endl;
      }
    } // for index
  }   // for group

  // 5. Spectrum IDs
  std::map<int, std::vector<int>>::iterator pit;
  for (pit = mGroupSpectraMap.begin(); pit != mGroupSpectraMap.end(); ++pit) {
    int groupid = pit->first;
    std::vector<int> spectra = pit->second;

    storage_map::iterator sit;
    sit = m_GroupSpecInds.find(groupid);
    if (sit == m_GroupSpecInds.end())
      continue;

    std::vector<size_t> &wsindexes = sit->second;

    for (size_t i = 0; i < spectra.size(); i++) {
      int specid = spectra[i];
      spec2index_map::iterator ind = specs2index.find(specid);
      if (ind != specs2index.end()) {
        size_t wsid = ind->second;
        wsindexes.push_back(wsid);
        if (unUsedSpec[wsid] != (USED)) {
          unUsedSpec[wsid] = (USED);
        }
      } else {
        g_log.error() << "Spectrum with ID " << specid
                      << " is not found in instrument " << std::endl;
      }
    } // for index
  }   // for group

  return;
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

  typedef std::map<size_t, std::set<size_t>> Group2SetMapType;
  Group2SetMapType group2WSIndexSetmap;

  const size_t nspec = groupWS->getNumberHistograms();
  for (size_t i = 0; i < nspec; ++i) {
    // read spectra from groupingws
    size_t groupid = static_cast<int>(groupWS->readY(i)[0]);
    // group 0 is are unused spectra - don't process them
    if (groupid > 0) {
      if (group2WSIndexSetmap.find(groupid) == group2WSIndexSetmap.end()) {
        // not found - create an empty set
        group2WSIndexSetmap.insert(std::make_pair(groupid, std::set<size_t>()));
      }
      // get a reference to the set
      std::set<size_t> &targetWSIndexSet = group2WSIndexSetmap[groupid];

      // translate to detectors
      std::vector<detid_t> det_ids;
      try {
        const Geometry::IDetector_const_sptr det = groupWS->getDetector(i);
        Geometry::DetectorGroup_const_sptr detGroup =
            boost::dynamic_pointer_cast<const Geometry::DetectorGroup>(det);
        if (detGroup) {
          det_ids = detGroup->getDetectorIDs();

        } else {
          det_ids.push_back(det->getID());
        }

        for (auto dit = det_ids.begin(); dit != det_ids.end(); ++dit) {
          // translate detectors to target det ws indexes
          detid_t det_id = *dit;
          size_t targetWSIndex = detIdToWiMap[det_id];
          targetWSIndexSet.insert(targetWSIndex);
          // mark as used
          if (unUsedSpec[targetWSIndex] != (USED)) {
            unUsedSpec[targetWSIndex] = (USED);
          }
        }
      } catch (Mantid::Kernel::Exception::NotFoundError) {
        // the detector was not found - don't add it
      }
    }
  }

  // Build m_GroupSpecInds (group -> list of ws indices)
  for (auto dit = group2WSIndexSetmap.begin(); dit != group2WSIndexSetmap.end();
       ++dit) {
    size_t groupid = dit->first;
    std::set<size_t> &targetWSIndexSet = dit->second;
    std::vector<size_t> tempv;
    tempv.assign(targetWSIndexSet.begin(), targetWSIndexSet.end());
    m_GroupSpecInds.insert(
        std::make_pair(static_cast<specid_t>(groupid), tempv));
  }

  return;
}

/** Get groupings from a matrix workspace
*  @param groupWS :: the matrix workspace to use
*  @param workspace :: a pointer to the input workspace, used to get spectra
* indexes from numbers
*  @param unUsedSpec :: the list of spectra indexes that have been not included
* in
* a group (so far)
*/
void
GroupDetectors2::processMatrixWorkspace(MatrixWorkspace_const_sptr groupWS,
                                        MatrixWorkspace_const_sptr workspace,
                                        std::vector<int64_t> &unUsedSpec) {
  detid2index_map detIdToWiMap = workspace->getDetectorIDToWorkspaceIndexMap();

  typedef std::map<size_t, std::set<size_t>> Group2SetMapType;
  Group2SetMapType group2WSIndexSetmap;

  const size_t nspec = groupWS->getNumberHistograms();
  for (size_t i = 0; i < nspec; ++i) {
    // read spectra from groupingws
    size_t groupid = i;

    if (group2WSIndexSetmap.find(groupid) == group2WSIndexSetmap.end()) {
      // not found - create an empty set
      group2WSIndexSetmap.insert(std::make_pair(groupid, std::set<size_t>()));
    }
    // get a reference to the set
    std::set<size_t> &targetWSIndexSet = group2WSIndexSetmap[groupid];

    // translate to detectors
    std::vector<detid_t> det_ids;
    try {
      const Geometry::IDetector_const_sptr det = groupWS->getDetector(i);

      Geometry::DetectorGroup_const_sptr detGroup =
          boost::dynamic_pointer_cast<const Geometry::DetectorGroup>(det);

      if (detGroup) {
        det_ids = detGroup->getDetectorIDs();

        for (auto dit = det_ids.begin(); dit != det_ids.end(); ++dit) {
          // translate detectors to target det ws indexes
          detid_t det_id = *dit;
          size_t targetWSIndex = detIdToWiMap[det_id];
          targetWSIndexSet.insert(targetWSIndex);
          // mark as used
          if (unUsedSpec[targetWSIndex] != (USED)) {
            unUsedSpec[targetWSIndex] = (USED);
          }
        }
      }
    } catch (Mantid::Kernel::Exception::NotFoundError) {
      // the detector was not found - don't add it
    }
  }

  // Build m_GroupSpecInds (group -> list of ws indices)
  for (auto dit = group2WSIndexSetmap.begin(); dit != group2WSIndexSetmap.end();
       ++dit) {
    size_t groupid = dit->first;
    std::set<size_t> &targetWSIndexSet = dit->second;
    if (!targetWSIndexSet.empty()) {
      std::vector<size_t> tempv;
      tempv.assign(targetWSIndexSet.begin(), targetWSIndexSet.end());
      m_GroupSpecInds.insert(
          std::make_pair(static_cast<specid_t>(groupid), tempv));
    }
  }

  return;
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
int GroupDetectors2::readInt(std::string line) {
  // remove comments and white space (TOK_TRIM)
  Poco::StringTokenizer dataComment(line, "#", Poco::StringTokenizer::TOK_TRIM);
  if (dataComment.begin() != dataComment.end()) {
    Poco::StringTokenizer data(*(dataComment.begin()), " ",
                               Poco::StringTokenizer::TOK_TRIM);
    if (data.count() == 1) {
      if (!data[0].empty()) {
        try {
          return boost::lexical_cast<int>(data[0]);
        } catch (boost::bad_lexical_cast &e) {
          g_log.debug() << "Exception thrown: " << e.what() << std::endl;
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
                    << " strings the first string is " << data[0] << std::endl;
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
* @param unUsedSpec :: list of spectra that haven't yet been included in a group
* @throw invalid_argument if there is any problem with the file
*/
void GroupDetectors2::readFile(spec2index_map &specs2index, std::istream &File,
                               size_t &lineNum,
                               std::vector<int64_t> &unUsedSpec) {
  // go through the rest of the file reading in lists of spectra number to group
  while (File) {
    int spectrumNo = EMPTY_LINE;
    std::string thisLine;
    do {
      std::getline(File, thisLine), lineNum++;
      spectrumNo = readInt(thisLine);
      if (spectrumNo != EMPTY_LINE)
        spectrumNo = specs2index[spectrumNo];
      // we haven't started reading a new group and so if the file ends here it
      // is OK
      if (!File)
        return;
    } while (spectrumNo == EMPTY_LINE && File);

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

    // the value of this map is the list of spectra numbers that will be
    // combined into a group
    m_GroupSpecInds[spectrumNo].reserve(numberOfSpectra);
    do {
      if (!File)
        throw std::invalid_argument("Premature end of file, found number of "
                                    "spectra specification but no spectra "
                                    "list");
      std::getline(File, thisLine), lineNum++;
      // the spectra numbers that will be included in the group
      readSpectraIndexes(thisLine, specs2index, m_GroupSpecInds[spectrumNo],
                         unUsedSpec);
    } while (static_cast<int>(m_GroupSpecInds[spectrumNo].size()) <
             numberOfSpectra);
    if (static_cast<int>(m_GroupSpecInds[spectrumNo].size()) !=
        numberOfSpectra) { // it makes no sense to continue reading the file,
                           // we'll stop here
      throw std::invalid_argument(
          std::string("Bad number of spectra specification or spectra list "
                      "near line number ") +
          boost::lexical_cast<std::string>(lineNum));
    }
    // make regular progress reports and check for a cancellation notification
    if ((m_GroupSpecInds.size() % INTERVAL) == 1) {
      fileReadProg(m_GroupSpecInds.size(), specs2index.size());
    }
  }
}
/** The function expects that the string passed to it contains a series of
* integers,
*  ranges specified with a '-' are possible
*  @param line :: a line read from the file, we'll interpret this
*  @param specs2index :: a map with spectra numbers as indexes and index numbers
* as values
*  @param output :: the list of integers, with any ranges expanded
*  @param unUsedSpec :: the list of spectra indexes that have been included in a
* group (so far)
*  @param seperator :: the symbol for the index range separator
*  @throw invalid_argument when a number couldn't be found or the number is not
* in the spectra map
*/
void GroupDetectors2::readSpectraIndexes(std::string line,
                                         spec2index_map &specs2index,
                                         std::vector<size_t> &output,
                                         std::vector<int64_t> &unUsedSpec,
                                         std::string seperator) {
  // remove comments and white space
  Poco::StringTokenizer dataComment(line, seperator, IGNORE_SPACES);
  Poco::StringTokenizer::Iterator iend = dataComment.end();
  for (Poco::StringTokenizer::Iterator itr = dataComment.begin(); itr != iend;
       ++itr) {
    std::vector<size_t> specNums;
    specNums.reserve(output.capacity());

    RangeHelper::getList(*itr, specNums);

    std::vector<size_t>::const_iterator specN = specNums.begin();
    for (; specN != specNums.end(); ++specN) {
      specid_t spectrumNum = static_cast<specid_t>(*specN);
      spec2index_map::const_iterator ind = specs2index.find(spectrumNum);
      if (ind == specs2index.end()) {
        g_log.debug() << name() << ": spectrum number " << spectrumNum
                      << " refered to in the input file was not found in the "
                         "input workspace\n";
        throw std::invalid_argument(
            "Spectrum number " + boost::lexical_cast<std::string>(spectrumNum) +
            " not found");
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
*  @return estimate of the amount of algorithm progress obtained by reading from
* the file
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
*  @param prog4Copy :: the amount of algorithm progress to attribute to moving a
* single spectra
*  @return number of new grouped spectra
*/
size_t GroupDetectors2::formGroups(API::MatrixWorkspace_const_sptr inputWS,
                                   API::MatrixWorkspace_sptr outputWS,
                                   const double prog4Copy) {
  // get "Behaviour" string
  const std::string behaviour = getProperty("Behaviour");
  int bhv = 0;
  if (behaviour == "Average")
    bhv = 1;

  API::MatrixWorkspace_sptr beh = API::WorkspaceFactory::Instance().create(
      "Workspace2D", static_cast<int>(m_GroupSpecInds.size()), 1, 1);

  g_log.debug() << name() << ": Preparing to group spectra into "
                << m_GroupSpecInds.size() << " groups\n";

  // where we are copying spectra to, we start copying to the start of the
  // output workspace
  size_t outIndex = 0;
  // Only used for averaging behaviour. We may have a 1:1 map where a Divide
  // would be waste as it would be just dividing by 1
  bool requireDivide(false);
  for (storage_map::const_iterator it = m_GroupSpecInds.begin();
       it != m_GroupSpecInds.end(); ++it) {
    // This is the grouped spectrum
    ISpectrum *outSpec = outputWS->getSpectrum(outIndex);

    // The spectrum number of the group is the key
    outSpec->setSpectrumNo(it->first);
    // Start fresh with no detector IDs
    outSpec->clearDetectorIDs();

    // Copy over X data from first spectrum, the bin boundaries for all spectra
    // are assumed to be the same here
    outSpec->dataX() = inputWS->readX(0);

    // the Y values and errors from spectra being grouped are combined in the
    // output spectrum
    MantidVec &firstY = outSpec->dataY();
    // Keep track of number of detectors required for masking
    size_t nonMaskedSpectra(0);
    beh->dataX(outIndex)[0] = 0.0;
    beh->dataE(outIndex)[0] = 0.0;
    for (std::vector<size_t>::const_iterator wsIter = it->second.begin();
         wsIter != it->second.end(); ++wsIter) {
      const size_t originalWI = *wsIter;

      // detectors to add to firstSpecNum
      const ISpectrum *fromSpectrum = inputWS->getSpectrum(originalWI);

      // Add up all the Y spectra and store the result in the first one
      MantidVec::iterator fEit = outSpec->dataE().begin();
      MantidVec::const_iterator Yit = fromSpectrum->dataY().begin();
      MantidVec::const_iterator Eit = fromSpectrum->dataE().begin();
      for (auto fYit = firstY.begin(); fYit != firstY.end();
           ++fYit, ++fEit, ++Yit, ++Eit) {
        *fYit += *Yit;
        // Assume 'normal' (i.e. Gaussian) combination of errors
        *fEit = std::sqrt((*fEit) * (*fEit) + (*Eit) * (*Eit));
      }

      // detectors to add to the output spectrum
      outSpec->addDetectorIDs(fromSpectrum->getDetectorIDs());
      try {
        Geometry::IDetector_const_sptr det = inputWS->getDetector(originalWI);
        if (!det->isMasked())
          ++nonMaskedSpectra;
      } catch (Exception::NotFoundError &) {
        // If a detector cannot be found, it cannot be masked
        ++nonMaskedSpectra;
      }
    }
    if (nonMaskedSpectra == 0)
      ++nonMaskedSpectra; // Avoid possible divide by zero
    if (!requireDivide)
      requireDivide = (nonMaskedSpectra > 1);
    beh->dataY(outIndex)[0] = static_cast<double>(nonMaskedSpectra);

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

/**
*  Move the user selected spectra in the input workspace into groups in the
* output workspace
*  @param inputWS :: user selected input workspace for the algorithm
*  @param outputWS :: user selected output workspace for the algorithm
*  @param prog4Copy :: the amount of algorithm progress to attribute to moving a
* single spectra
*  @return number of new grouped spectra
*/
size_t
GroupDetectors2::formGroupsEvent(DataObjects::EventWorkspace_const_sptr inputWS,
                                 DataObjects::EventWorkspace_sptr outputWS,
                                 const double prog4Copy) {
  // get "Behaviour" string
  const std::string behaviour = getProperty("Behaviour");
  int bhv = 0;
  if (behaviour == "Average")
    bhv = 1;

  API::MatrixWorkspace_sptr beh = API::WorkspaceFactory::Instance().create(
      "Workspace2D", static_cast<int>(m_GroupSpecInds.size()), 1, 1);

  g_log.debug() << name() << ": Preparing to group spectra into "
                << m_GroupSpecInds.size() << " groups\n";

  // where we are copying spectra to, we start copying to the start of the
  // output workspace
  size_t outIndex = 0;
  // Only used for averaging behaviour. We may have a 1:1 map where a Divide
  // would be waste as it would be just dividing by 1
  bool requireDivide(false);
  for (storage_map::const_iterator it = m_GroupSpecInds.begin();
       it != m_GroupSpecInds.end(); ++it) {
    // This is the grouped spectrum
    EventList &outEL = outputWS->getEventList(outIndex);

    // The spectrum number of the group is the key
    outEL.setSpectrumNo(it->first);
    // Start fresh with no detector IDs
    outEL.clearDetectorIDs();

    // the Y values and errors from spectra being grouped are combined in the
    // output spectrum
    // Keep track of number of detectors required for masking
    size_t nonMaskedSpectra(0);
    beh->dataX(outIndex)[0] = 0.0;
    beh->dataE(outIndex)[0] = 0.0;
    for (std::vector<size_t>::const_iterator wsIter = it->second.begin();
         wsIter != it->second.end(); ++wsIter) {
      const size_t originalWI = *wsIter;

      const EventList &fromEL = inputWS->getEventList(originalWI);
      // Add the event lists with the operator
      outEL += fromEL;

      // detectors to add to the output spectrum
      outEL.addDetectorIDs(fromEL.getDetectorIDs());
      try {
        Geometry::IDetector_const_sptr det = inputWS->getDetector(originalWI);
        if (!det->isMasked())
          ++nonMaskedSpectra;
      } catch (Exception::NotFoundError &) {
        // If a detector cannot be found, it cannot be masked
        ++nonMaskedSpectra;
      }
    }
    if (nonMaskedSpectra == 0)
      ++nonMaskedSpectra; // Avoid possible divide by zero
    if (!requireDivide)
      requireDivide = (nonMaskedSpectra > 1);
    beh->dataY(outIndex)[0] = static_cast<double>(nonMaskedSpectra);

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

/**
*  Only to be used if the KeepUnGrouped property is true, moves the spectra that
* were not selected
*  to be in a group to the end of the output spectrum
*  @param unGroupedSet :: list of WORKSPACE indexes that were included in a
* group
*  @param inputWS :: user selected input workspace for the algorithm
*  @param outputWS :: user selected output workspace for the algorithm
*  @param outIndex :: the next spectra index available after the grouped spectra
*/
void GroupDetectors2::moveOthers(const std::set<int64_t> &unGroupedSet,
                                 API::MatrixWorkspace_const_sptr inputWS,
                                 API::MatrixWorkspace_sptr outputWS,
                                 size_t outIndex) {
  g_log.debug() << "Starting to copy the ungrouped spectra" << std::endl;
  double prog4Copy = (1. - 1. * static_cast<double>(m_FracCompl)) /
                     static_cast<double>(unGroupedSet.size());

  std::set<int64_t>::const_iterator copyFrIt = unGroupedSet.begin();
  // go thorugh all the spectra in the input workspace
  for (; copyFrIt != unGroupedSet.end(); ++copyFrIt) {
    if (*copyFrIt == USED)
      continue; // Marked as not to be used
    size_t sourceIndex = static_cast<size_t>(*copyFrIt);

    // The input spectrum we'll copy
    const ISpectrum *inputSpec = inputWS->getSpectrum(sourceIndex);

    // Destination of the copying
    ISpectrum *outputSpec = outputWS->getSpectrum(outIndex);

    // Copy the data
    outputSpec->dataX() = inputSpec->dataX();
    outputSpec->dataY() = inputSpec->dataY();
    outputSpec->dataE() = inputSpec->dataE();

    // Spectrum numbers etc.
    outputSpec->setSpectrumNo(inputSpec->getSpectrumNo());
    outputSpec->clearDetectorIDs();
    outputSpec->addDetectorIDs(inputSpec->getDetectorIDs());

    // go to the next free index in the output workspace
    outIndex++;
    // make regular progress reports and check for cancelling the algorithm
    if (outIndex % INTERVAL == 0) {
      m_FracCompl += INTERVAL * prog4Copy;
      if (m_FracCompl > 1.0) {
        m_FracCompl = 1.0;
      }
      progress(m_FracCompl);
      interruption_point();
    }
  }

  g_log.debug() << name() << " copied " << unGroupedSet.size() - 1
                << " ungrouped spectra\n";
}

/**
*  Only to be used if the KeepUnGrouped property is true, moves the spectra that
* were not selected
*  to be in a group to the end of the output spectrum
*  @param unGroupedSet :: list of WORKSPACE indexes that were included in a
* group
*  @param inputWS :: user selected input workspace for the algorithm
*  @param outputWS :: user selected output workspace for the algorithm
*  @param outIndex :: the next spectra index available after the grouped spectra
*/
void
GroupDetectors2::moveOthersEvent(const std::set<int64_t> &unGroupedSet,
                                 DataObjects::EventWorkspace_const_sptr inputWS,
                                 DataObjects::EventWorkspace_sptr outputWS,
                                 size_t outIndex) {
  g_log.debug() << "Starting to copy the ungrouped spectra" << std::endl;
  double prog4Copy = (1. - 1. * static_cast<double>(m_FracCompl)) /
                     static_cast<double>(unGroupedSet.size());

  std::set<int64_t>::const_iterator copyFrIt = unGroupedSet.begin();
  // go thorugh all the spectra in the input workspace
  for (; copyFrIt != unGroupedSet.end(); ++copyFrIt) {
    if (*copyFrIt == USED)
      continue; // Marked as not to be used
    size_t sourceIndex = static_cast<size_t>(*copyFrIt);

    // The input spectrum we'll copy
    const EventList &inputSpec = inputWS->getEventList(sourceIndex);

    // Destination of the copying
    EventList &outputSpec = outputWS->getEventList(outIndex);

    // Copy the data
    outputSpec += inputSpec;

    // Spectrum numbers etc.
    outputSpec.setSpectrumNo(inputSpec.getSpectrumNo());
    outputSpec.clearDetectorIDs();
    outputSpec.addDetectorIDs(inputSpec.getDetectorIDs());

    // go to the next free index in the output workspace
    outIndex++;
    // make regular progress reports and check for cancelling the algorithm
    if (outIndex % INTERVAL == 0) {
      m_FracCompl += INTERVAL * prog4Copy;
      if (m_FracCompl > 1.0) {
        m_FracCompl = 1.0;
      }
      progress(m_FracCompl);
      interruption_point();
    }
  }

  g_log.debug() << name() << " copied " << unGroupedSet.size() - 1
                << " ungrouped spectra\n";
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
  Poco::StringTokenizer ranges(line, "-");

  try {
    size_t loop = 0;
    do {
      Poco::StringTokenizer beforeHyphen(ranges[loop], " ", IGNORE_SPACES);
      Poco::StringTokenizer::Iterator readPostion = beforeHyphen.begin();
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

      Poco::StringTokenizer afterHyphen(ranges[loop + 1], " ", IGNORE_SPACES);
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

} // namespace DataHandling
} // namespace Mantid
