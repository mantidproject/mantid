// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CreateGroupingWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Strings.h"

#include <boost/algorithm/string/detail/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <fstream>
#include <queue>
#include <utility>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace {
Mantid::Kernel::Logger g_log("CreateGroupingWorkspace");

void removeSpacesFromString(std::string &str) { str.erase(std::remove_if(str.begin(), str.end(), isspace), str.end()); }

/** Adds the elements of the second vector onto the end of the first vector.
 *
 * @param vec The vector to be extended.
 * @param extension The vector to emplace onto the end of vec.
 */
void extendVectorBy(std::vector<std::string> &vec, const std::vector<std::string> &extension) {
  vec.reserve(vec.size() + std::distance(extension.cbegin(), extension.cend()));
  vec.insert(vec.end(), extension.cbegin(), extension.cend());
}

/** Splits a string by the provided delimiter characters. Erases any empty
 * sub-strings.
 *
 * @param str The string to be split.
 * @param delimiter The characters to split the string by.
 * @returns :: A vector of sub-strings resulting from the split.
 */
std::vector<std::string> splitStringBy(const std::string &str, const std::string &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  subStrings.erase(std::remove_if(subStrings.begin(), subStrings.end(),
                                  [](const std::string &subString) { return subString.empty(); }),
                   subStrings.cend());
  return subStrings;
}

/** Returns true if a string contains a specific separator.
 *
 * @param str The string to search for the separator.
 * @param separator The separator to search for.
 * @returns :: True if the string contains the specified separator.
 */
bool hasSeparator(const std::string &str, const std::string &separator) {
  return str.find(separator) != std::string::npos;
}

/** Returns a vector of detector IDs (in string format) contained within the
 * lower and upper limits of a range.
 *
 * @param lower The lowest detector ID.
 * @param upper The highest detector ID.
 * @returns :: A vector of detector IDs (in string format).
 */
std::vector<std::string> getDetectorRangeFromLimits(int lower, int upper) {
  std::vector<std::string> detectorIds;
  detectorIds.reserve(static_cast<std::size_t>(upper - lower + 1));
  for (auto i = lower; i <= upper; ++i)
    detectorIds.emplace_back(std::to_string(i));
  return detectorIds;
}

/** Splits a grouping string by the colon separator, and then fully expands the
 * group.
 *
 * @param groupString The grouping string containing a ':' separator.
 * @returns :: The expanded vector of groups.
 */
std::vector<std::string> groupsFromColonRange(const std::string &groupString) {
  const auto splitByColon = splitStringBy(groupString, ":");
  if (splitByColon.size() > 2)
    throw std::runtime_error("Expected a single colon separator.");

  if (splitByColon.size() == 2)
    return getDetectorRangeFromLimits(std::stoi(splitByColon[0]), std::stoi(splitByColon[1]));
  return splitByColon;
}

/** Expands the grouping strings that contain a ':' separator. For example the
 * string '2:5' means the detector IDs 2, 3, 4 and 5 should be in their own
 * individual groups. This means we must expand this string.
 *
 * @param groupsToExpand The grouping strings to check and expand.
 * @returns :: A vector of expanded grouping strings.
 */
std::vector<std::string> expandGroupsWithColonSeparator(const std::vector<std::string> &groupsToExpand) {
  std::vector<std::string> expandedGroupStrings;
  for (const auto &groupString : groupsToExpand)
    extendVectorBy(expandedGroupStrings, groupsFromColonRange(groupString));
  return expandedGroupStrings;
}

/** Maps a single detector ID to a group ID if the detector ID is found in the
 * vector of allowed ID's.
 *
 * @param allowedDetectorIDs The detector IDs which are allowed for the given
 * instrument.
 * @param detectorIDToGroup The mapping of detector IDs to group IDs.
 * @param detectorID The ID of the detector to map to the group ID.
 * @param groupID The ID of the group to map to the detector ID.
 */
void addDetectorToGroup(const std::vector<detid_t> &allowedDetectorIDs, std::map<detid_t, int> &detectorIDToGroup,
                        int detectorID, int groupID) {
  const auto iter = std::find(allowedDetectorIDs.cbegin(), allowedDetectorIDs.cend(), detectorID);
  if (iter == allowedDetectorIDs.cend())
    throw std::runtime_error("The Detector ID '" + std::to_string(detectorID) +
                             "' is not valid for this instrument component.");

  detectorIDToGroup[detectorID] = groupID;
}

/** Adds the detector IDs from a grouping string containing a dash to the Group
 * ID map. For example the string '2-5' means detector IDs 2, 3, 4 and 5
 * should be mapped to the same group ID.
 *
 * @param allowedDetectorIDs The detector IDs which are allowed for the given
 * instrument.
 * @param detectorIDToGroup The mapping of detector IDs to group IDs.
 * @param groupString The string which contains the '-' separator.
 * @param groupID The ID of the group to map to the detector IDs.
 */
void addDashSeparatedDetectorIDsToSameGroup(const std::vector<detid_t> &allowedDetectorIDs,
                                            std::map<detid_t, int> &detectorIDToGroup, const std::string &groupString,
                                            int groupID) {
  const auto splitByDash = splitStringBy(groupString, "-");

  if (splitByDash.size() < 2)
    throw std::runtime_error("Expected at least one dash separator.");
  else if (splitByDash.size() > 2)
    throw std::runtime_error("Expected a single dash separator.");

  for (auto i = std::stoi(splitByDash[0]); i <= std::stoi(splitByDash[1]); ++i)
    addDetectorToGroup(allowedDetectorIDs, detectorIDToGroup, i, groupID);
}

/** Adds the detector IDs from a grouping string containing a plus to the Group
 * ID map. For example the string '2+3+4+5' means detector IDs 2, 3, 4 and 5
 * should be mapped to the same group ID.
 *
 * @param allowedDetectorIDs The detector IDs which are allowed for the given
 * instrument.
 * @param detectorIDToGroup The mapping of detector IDs to group IDs.
 * @param groupString The string which contains the '+' separator.
 * @param groupID The ID of the group to map to the detector IDs.
 */
void addPlusSeparatedDetectorIDsToSameGroup(const std::vector<detid_t> &allowedDetectorIDs,
                                            std::map<detid_t, int> &detectorIDToGroup, const std::string &groupString,
                                            int groupID) {
  const auto splitByPlus = splitStringBy(groupString, "+");
  if (splitByPlus.size() < 2)
    throw std::runtime_error("Expected at least one plus separator.");

  for (const auto &id : splitByPlus)
    addDetectorToGroup(allowedDetectorIDs, detectorIDToGroup, std::stoi(id), groupID);
}

/** Gets the detector IDs within the component of a given instrument.
 *
 * @param instrument A pointer to instrument.
 * @param componentName Name of component in instrument.
 * @returns :: A vector of Detector IDs.
 */
std::vector<detid_t> getAllowedDetectorIDs(const Instrument_const_sptr &instrument, const std::string &componentName) {
  std::vector<IDetector_const_sptr> detectors;
  detectors.reserve(instrument->getNumberDetectors());
  instrument->getDetectorsInBank(detectors, componentName);

  std::vector<detid_t> detectorIDs;
  detectorIDs.reserve(detectors.size());
  std::transform(detectors.cbegin(), detectors.cend(), std::back_inserter(detectorIDs),
                 [](const IDetector_const_sptr &detector) { return detector->getID(); });
  return detectorIDs;
}

/** Creates a mapping between Detector IDs and Group IDs from several grouping
 * strings already split by the comma ',' separator. At this stage the ':'
 * separated strings have also already been expanded.
 *
 * @param allowedDetectorIDs The detector IDs which are allowed for the given
 * instrument.
 * @param groupingStrings The grouping strings to split and map.
 * @returns :: Map of detector IDs to group IDs.
 */
std::map<detid_t, int> mapGroupingStringsToGroupIDs(const std::vector<detid_t> &allowedDetectorIDs,
                                                    const std::vector<std::string> &groupingStrings) {
  std::map<detid_t, int> detectorIDToGroup;
  for (auto j = 0; j < static_cast<int>(groupingStrings.size()); ++j) {
    if (hasSeparator(groupingStrings[j], "+"))
      addPlusSeparatedDetectorIDsToSameGroup(allowedDetectorIDs, detectorIDToGroup, groupingStrings[j], j + 1);
    else if (hasSeparator(groupingStrings[j], "-"))
      addDashSeparatedDetectorIDsToSameGroup(allowedDetectorIDs, detectorIDToGroup, groupingStrings[j], j + 1);
    else
      addDetectorToGroup(allowedDetectorIDs, detectorIDToGroup, std::stoi(groupingStrings[j]), j + 1);
  }
  return detectorIDToGroup;
}

/** Creates a mapping between Detector IDs and Group IDs using a custom grouping
 * string
 *
 * @param instrument A pointer to instrument.
 * @param componentName Name of component in instrument.
 * @param customGroupingString The string used to specify the grouping.
 * @returns :: Map of detector IDs to group IDs.
 */
std::map<detid_t, int> makeGroupingByCustomString(const Instrument_const_sptr &instrument,
                                                  const std::string &componentName, std::string &customGroupingString) {
  removeSpacesFromString(customGroupingString);

  const auto detectorIDs = getAllowedDetectorIDs(instrument, componentName);
  const auto groupStrings = expandGroupsWithColonSeparator(splitStringBy(customGroupingString, ","));

  return mapGroupingStringsToGroupIDs(detectorIDs, groupStrings);
}

} // namespace

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateGroupingWorkspace)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

const std::string CreateGroupingWorkspace::name() const { return "CreateGroupingWorkspace"; }

int CreateGroupingWorkspace::version() const { return 1; }

const std::string CreateGroupingWorkspace::category() const { return "Utility\\Workspaces;Transforms\\Grouping"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateGroupingWorkspace::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, PropertyMode::Optional),
                  "Optional: An input workspace with the instrument we want to use.");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>("InstrumentName", "", Direction::Input),
                  "Optional: Name of the instrument to base the "
                  "GroupingWorkspace on which to base the GroupingWorkspace.");

  declareProperty(std::make_unique<FileProperty>("InstrumentFilename", "", FileProperty::OptionalLoad, ".xml"),
                  "Optional: Path to the instrument definition file on which "
                  "to base the GroupingWorkspace.");

  declareProperty(std::make_unique<FileProperty>("OldCalFilename", "", FileProperty::OptionalLoad, ".cal"),
                  "Optional: Path to the old-style .cal grouping/calibration "
                  "file (multi-column ASCII). You must also specify the "
                  "instrument.");

  declareProperty("GroupNames", "",
                  "Optional: A string of the instrument component names to use "
                  "as separate groups. "
                  "Use / or , to separate multiple groups. "
                  "If empty, then an empty GroupingWorkspace will be created.");

  std::vector<std::string> grouping{"", "All", "Group", "2_4Grouping", "Column", "bank"};
  declareProperty("GroupDetectorsBy", "", std::make_shared<StringListValidator>(grouping),
                  "Only used if GroupNames is empty");
  declareProperty("MaxRecursionDepth", 5, "Number of levels to search into the instrument (default=5)");

  declareProperty("FixedGroupCount", 0, std::make_shared<BoundedValidator<int>>(0, INT_MAX),
                  "Used to distribute the detectors of a given component into "
                  "a fixed number of groups");
  declareProperty("CustomGroupingString", "",
                  "This takes a comma separated list of grouped detector IDs. An example "
                  "of the syntax is 1,2+3,4-6,7:10. The documentation page for this "
                  "algorithm gives a full explanation of this syntax.");
  declareProperty("ComponentName", "",
                  "Specify the instrument component to "
                  "group into a fixed number of groups");

  declareProperty(std::make_unique<WorkspaceProperty<GroupingWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output GroupingWorkspace.");

  std::string inputs("Specify Instrument");
  setPropertyGroup("InputWorkspace", inputs);
  setPropertyGroup("InstrumentName", inputs);
  setPropertyGroup("InstrumentFilename", inputs);

  std::string groupby("Specify Grouping");
  setPropertyGroup("GroupNames", groupby);
  setPropertyGroup("GroupDetectorsBy", groupby);
  setPropertyGroup("MaxRecursionDepth", groupby);
  setPropertyGroup("FixedGroupCount", groupby);
  setPropertyGroup("ComponentName", groupby);
  setPropertyGroup("CustomGroupingString", groupby);

  // output properties
  declareProperty("NumberGroupedSpectraResult", EMPTY_INT(), "The number of spectra in groups", Direction::Output);
  declareProperty("NumberGroupsResult", EMPTY_INT(), "The number of groups", Direction::Output);
}

std::map<std::string, std::string> CreateGroupingWorkspace::validateInputs() {
  std::map<std::string, std::string> result;

  // only allow specifying the instrument in one way
  int numInstrument = 0;
  if (!isDefault("InputWorkspace"))
    numInstrument += 1;
  if (!isDefault("InstrumentName"))
    numInstrument += 1;
  if (!isDefault("InstrumentFilename"))
    numInstrument += 1;
  if (numInstrument == 0) {
    std::string msg("Must supply an instrument");
    result["InputWorkspace"] = msg;
    result["InstrumentName"] = msg;
    result["InstrumentFilename"] = msg;
  } else if (numInstrument > 1) {
    std::string msg("Must supply an instrument only one way");

    if (!isDefault("InputWorkspace"))
      result["InputWorkspace"] = msg;
    if (!isDefault("InstrumentName"))
      result["InstrumentName"] = msg;
    if (!isDefault("InstrumentFilename"))
      result["InstrumentFilename"] = msg;
  }

  // only allow specifying the grouping one way
  int numGroupings = 0;
  if (!isDefault("GroupNames"))
    numGroupings += 1;
  if (!isDefault("GroupDetectorsBy"))
    numGroupings += 1;
  if (!isDefault("ComponentName"))
    numGroupings += 1;
  if (numGroupings != 1) {
    std::string msg("Must supply grouping only one way");
    if (!isDefault("GroupNames"))
      result["GroupNames"] = msg;
    if (!isDefault("GroupDetectorsBy"))
      result["GroupDetectorsBy"] = msg;
    if (!isDefault("ComponentName"))
      result["ComponentName"] = msg;
  }

  std::string customGroupingString = getPropertyValue("CustomGroupingString");
  std::string componentName = getPropertyValue("ComponentName");

  if (!componentName.empty() && !customGroupingString.empty()) {
    try {
      (void)makeGroupingByCustomString(getInstrument(), componentName, customGroupingString);
    } catch (const std::runtime_error &ex) {
      result["CustomGroupingString"] = ex.what();
    }
  }

  return result;
}

//------------------------------------------------------------------------------------------------
/** Read old-style .cal file to get the grouping
 *
 * @param groupingFileName :: path to .cal multi-col ascii
 * @param prog :: progress reporter
 * @returns :: map of key=detectorID, value=group number.
 */
std::map<detid_t, int> readGroupingFile(const std::string &groupingFileName, Progress &prog) {
  std::ifstream grFile(groupingFileName.c_str());
  if (!grFile.is_open()) {
    throw Exception::FileError("Error reading .cal file", groupingFileName);
  }
  std::map<detid_t, int> detIDtoGroup;
  std::string str;
  while (getline(grFile, str)) {
    // Comment
    if (str.empty() || str[0] == '#')
      continue;
    std::istringstream istr(str);
    int n, udet, sel, group;
    double offset;
    istr >> n >> udet >> offset >> sel >> group;
    if ((sel) && (group > 0)) {
      detIDtoGroup[udet] = group; // Register this detector id
    }
    prog.report();
  }
  grFile.close();
  return detIDtoGroup;
}

/** Creates a mapping based on a fixed number of groups for a given instrument
 *component
 *
 * @param compName Name of component in instrument
 * @param numGroups Number of groups to group detectors into
 * @param inst Pointer to instrument
 * @param prog Progress reporter
 * @returns :: Map of detector IDs to group number
 */
std::map<detid_t, int> makeGroupingByNumGroups(const std::string &compName, int numGroups,
                                               const Instrument_const_sptr &inst, Progress &prog) {
  std::map<detid_t, int> detIDtoGroup;

  // Get detectors for given instument component
  std::vector<IDetector_const_sptr> detectors;
  inst->getDetectorsInBank(detectors, compName);
  size_t numDetectors = detectors.size();

  // Sanity check for following calculation
  if (numGroups > static_cast<int>(numDetectors))
    throw std::runtime_error("Number of groups must be less than or "
                             "equal to number of detectors");

  // Calculate number of detectors per group
  int detectorsPerGroup = static_cast<int>(numDetectors) / numGroups;

  // Map detectors to group
  for (unsigned int detIndex = 0; detIndex < numDetectors; detIndex++) {
    int detectorID = detectors[detIndex]->getID();
    int groupNum = (detIndex / detectorsPerGroup) + 1;

    // Ignore any detectors the do not fit nicely into the group divisions
    if (groupNum <= numGroups)
      detIDtoGroup[detectorID] = groupNum;

    prog.report();
  }
  return detIDtoGroup;
}

//------------------------------------------------------------------------------------------------
/** Use group numbers for sorting
 *
 * @param groupi :: first group string
 * @param groupj :: second group string
 * @return true if first group number is less than second group number
 */

bool groupnumber(std::string groupi, std::string groupj) {
  int i = 0;
  std::string groupName = std::move(groupi);
  // Take out the "group" part of the group name and convert to an int
  groupName.erase(remove_if(groupName.begin(), groupName.end(), std::not_fn(::isdigit)), groupName.end());
  Strings::convert(groupName, i);
  int j = 0;
  groupName = std::move(groupj);
  // Take out the "group" part of the group name and convert to an int
  groupName.erase(remove_if(groupName.begin(), groupName.end(), std::not_fn(::isdigit)), groupName.end());
  Strings::convert(groupName, j);
  return (i < j);
}

//------------------------------------------------------------------------------------------------
/** Use bank names to build grouping
 *
 * @param GroupNames :: comma-sep list of bank names
 * @param inst :: instrument
 * @param prog :: progress report
 * @param sortnames :: sort names - a boolean
 * @returns:: map of detID to group number
 */
std::map<detid_t, int> makeGroupingByNames(std::string GroupNames, const Instrument_const_sptr &inst, Progress &prog,
                                           bool sortnames) {
  // This will contain the grouping
  std::map<detid_t, int> detIDtoGroup;

  // Split the names of the group and insert in a vector
  std::vector<std::string> vgroups;
  boost::split(vgroups, GroupNames, boost::algorithm::detail::is_any_ofF<char>(",/*"));
  while (vgroups.back().empty()) {
    vgroups.pop_back();
  }
  if (sortnames) {
    std::sort(vgroups.begin(), vgroups.end(), groupnumber);
  }

  // Trim and assign incremental number to each group
  std::map<std::string, int> group_map;
  int index = 0;
  for (auto &vgroup : vgroups) {
    boost::trim(vgroup);
    group_map[vgroup] = ++index;
  }

  // Find Detectors that belong to groups
  if (!group_map.empty()) {
    // Find Detectors that belong to groups
    using sptr_ICompAss = std::shared_ptr<const Geometry::ICompAssembly>;
    using sptr_IComp = std::shared_ptr<const Geometry::IComponent>;
    using sptr_IDet = std::shared_ptr<const Geometry::IDetector>;
    std::queue<std::pair<sptr_ICompAss, int>> assemblies;
    sptr_ICompAss current = std::dynamic_pointer_cast<const Geometry::ICompAssembly>(inst);
    sptr_IDet currentDet;
    sptr_IComp currentIComp;
    sptr_ICompAss currentchild;

    int top_group, child_group;

    if (current.get()) {
      top_group = group_map[current->getName()]; // Return 0 if not in map
      assemblies.emplace(current, top_group);
    }

    prog.setNumSteps(int(assemblies.size()));

    while (!assemblies.empty()) // Travel the tree from the instrument point
    {
      current = assemblies.front().first;
      top_group = assemblies.front().second;
      assemblies.pop();
      int nchilds = current->nelements();
      if (nchilds != 0) {
        for (int i = 0; i < nchilds; ++i) {
          currentIComp = (*(current.get()))[i]; // Get child
          currentDet = std::dynamic_pointer_cast<const Geometry::IDetector>(currentIComp);
          if (currentDet.get()) // Is detector
          {
            if (top_group > 0) {
              detIDtoGroup[currentDet->getID()] = top_group;
            }
          } else // Is an assembly, push in the queue
          {
            currentchild = std::dynamic_pointer_cast<const Geometry::ICompAssembly>(currentIComp);
            if (currentchild.get()) {
              child_group = group_map[currentchild->getName()];
              if (child_group == 0)
                child_group = top_group;
              assemblies.emplace(currentchild, child_group);
            }
          }
        }
      }
      prog.report();
    }
  }
  return detIDtoGroup;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateGroupingWorkspace::exec() {
  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  std::string InstrumentName = getPropertyValue("InstrumentName");
  std::string InstrumentFilename = getPropertyValue("InstrumentFilename");
  std::string OldCalFilename = getPropertyValue("OldCalFilename");
  std::string GroupNames = getPropertyValue("GroupNames");
  std::string grouping = getPropertyValue("GroupDetectorsBy");
  int numGroups = getProperty("FixedGroupCount");
  std::string customGroupingString = getPropertyValue("CustomGroupingString");
  std::string componentName = getPropertyValue("ComponentName");

  // Some validation
  int numParams = 0;
  if (inWS)
    numParams++;
  if (!InstrumentName.empty())
    numParams++;
  if (!InstrumentFilename.empty())
    numParams++;

  if (numParams > 1)
    throw std::invalid_argument("You must specify exactly ONE way to get an "
                                "instrument (workspace, instrument name, or "
                                "IDF file). You specified more than one.");
  if (numParams == 0)
    throw std::invalid_argument("You must specify exactly ONE way to get an "
                                "instrument (workspace, instrument name, or "
                                "IDF file). You specified none.");

  if (!OldCalFilename.empty() && !GroupNames.empty())
    throw std::invalid_argument("You must specify either to use the "
                                "OldCalFilename parameter OR GroupNames but "
                                "not both!");

  bool sortnames = false;

  const auto inst = getInstrument();

  // Validation for 2_4Grouping input used only for SNAP
  if (inst->getName() != "SNAP" && grouping == "2_4Grouping") {
    const std::string message("2_4Grouping only works for SNAP.");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

  if (GroupNames.empty() && OldCalFilename.empty()) {
    if (grouping == "All") {
      GroupNames = inst->getName();
    } else if (inst->getName() == "SNAP" && grouping == "Group") {
      GroupNames = "East,West";
    } else if (inst->getName() == "POWGEN" && grouping == "Group") {
      GroupNames = "South,North";
    } else if (inst->getName() == "SNAP" && grouping == "2_4Grouping") {
      GroupNames = "Column1,Column2,Column3,Column4,Column5,Column6,";
    } else {
      sortnames = true;
      GroupNames = "";
      int maxRecurseDepth = this->getProperty("MaxRecursionDepth");

          PRAGMA_OMP(parallel for schedule(dynamic, 1) )
          for (int num = 0; num < 300; ++num) {
            PARALLEL_START_INTERRUPT_REGION
            std::ostringstream mess;
            mess << grouping << num;
            IComponent_const_sptr comp = inst->getComponentByName(mess.str(), maxRecurseDepth);
            PARALLEL_CRITICAL(GroupNames)
            if (comp)
              GroupNames += mess.str() + ",";
            PARALLEL_END_INTERRUPT_REGION
          }
          PARALLEL_CHECK_INTERRUPT_REGION
    }
  }

  // --------------------------- Create the output --------------------------
  auto outWS = std::make_shared<GroupingWorkspace>(inst);
  this->setProperty("OutputWorkspace", outWS);

  // This will get the grouping
  std::map<detid_t, int> detIDtoGroup;

  Progress prog(this, 0.2, 1.0, outWS->getNumberHistograms());
  // Make the grouping one of three ways:
  if (!GroupNames.empty()) {
    detIDtoGroup = makeGroupingByNames(GroupNames, inst, prog, sortnames);
    if (grouping == "2_4Grouping") {
      std::map<detid_t, int>::const_iterator it_end = detIDtoGroup.end();
      std::map<detid_t, int>::const_iterator it;
      for (it = detIDtoGroup.begin(); it != it_end; ++it) {
        if (it->second < 5)
          detIDtoGroup[it->first] = 1;
        else
          detIDtoGroup[it->first] = 2;
      }
    }

  } else if (!OldCalFilename.empty())
    detIDtoGroup = readGroupingFile(OldCalFilename, prog);
  else if ((numGroups > 0) && !componentName.empty())
    detIDtoGroup = makeGroupingByNumGroups(componentName, numGroups, inst, prog);
  else if (!customGroupingString.empty() && !componentName.empty()) {
    try {
      detIDtoGroup = makeGroupingByCustomString(inst, componentName, customGroupingString);
    } catch (const std::runtime_error &ex) {
      g_log.error(ex.what());
      return;
    }
  }

  g_log.information() << detIDtoGroup.size() << " entries in the detectorID-to-group map.\n";
  setProperty("NumberGroupedSpectraResult", static_cast<int>(detIDtoGroup.size()));

  if (detIDtoGroup.empty()) {
    g_log.warning() << "Creating empty group workspace\n";
    setProperty("NumberGroupsResult", static_cast<int>(0));
  } else {
    size_t numNotFound = 0;

    // Make the groups, if any
    std::map<detid_t, int>::const_iterator it_end = detIDtoGroup.end();
    std::map<detid_t, int>::const_iterator it;
    std::unordered_set<int> groupCount;
    for (it = detIDtoGroup.begin(); it != it_end; ++it) {
      int detID = it->first;
      int group = it->second;
      groupCount.insert(group);
      try {
        outWS->setValue(detID, double(group));
      } catch (std::invalid_argument &) {
        numNotFound++;
      }
    }
    setProperty("NumberGroupsResult", static_cast<int>(groupCount.size()));

    if (numNotFound > 0)
      g_log.warning() << numNotFound << " detector IDs (out of " << detIDtoGroup.size()
                      << ") were not found in the instrument\n.";
  }
}

Instrument_const_sptr CreateGroupingWorkspace::getInstrument() {
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  std::string instrumentName = getPropertyValue("InstrumentName");
  std::string instrumentFilename = getPropertyValue("InstrumentFilename");

  Instrument_const_sptr instrument;
  if (inputWorkspace) {
    instrument = inputWorkspace->getInstrument();
  } else {
    Algorithm_sptr childAlg = createChildAlgorithm("LoadInstrument", 0.0, 0.2);
    MatrixWorkspace_sptr tempWS = std::make_shared<Workspace2D>();
    childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
    childAlg->setPropertyValue("Filename", instrumentFilename);
    childAlg->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
    childAlg->setPropertyValue("InstrumentName", instrumentName);
    childAlg->executeAsChildAlg();
    instrument = tempWS->getInstrument();
  }
  return instrument;
}

} // namespace Mantid::Algorithms
