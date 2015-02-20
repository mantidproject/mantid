#include "MantidAlgorithms/CreateGroupingWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include <boost/algorithm/string/detail/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <queue>
#include <fstream>
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"

namespace {
Mantid::Kernel::Logger g_log("CreateGroupingWorkspace");
}

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateGroupingWorkspace)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CreateGroupingWorkspace::CreateGroupingWorkspace() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CreateGroupingWorkspace::~CreateGroupingWorkspace() {}

const std::string CreateGroupingWorkspace::name() const {
  return "CreateGroupingWorkspace";
}

int CreateGroupingWorkspace::version() const { return 1; }

const std::string CreateGroupingWorkspace::category() const {
  return "Utility\\Workspaces;Transforms\\Grouping";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateGroupingWorkspace::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,
                              PropertyMode::Optional),
      "Optional: An input workspace with the instrument we want to use.");

  declareProperty(new PropertyWithValue<std::string>("InstrumentName", "",
                                                     Direction::Input),
                  "Optional: Name of the instrument to base the "
                  "GroupingWorkspace on which to base the GroupingWorkspace.");

  declareProperty(new FileProperty("InstrumentFilename", "",
                                   FileProperty::OptionalLoad, ".xml"),
                  "Optional: Path to the instrument definition file on which "
                  "to base the GroupingWorkspace.");

  declareProperty(new FileProperty("OldCalFilename", "",
                                   FileProperty::OptionalLoad, ".cal"),
                  "Optional: Path to the old-style .cal grouping/calibration "
                  "file (multi-column ASCII). You must also specify the "
                  "instrument.");

  declareProperty("GroupNames", "",
                  "Optional: A string of the instrument component names to use "
                  "as separate groups. "
                  "Use / or , to separate multiple groups. "
                  "If empty, then an empty GroupingWorkspace will be created.");

  std::vector<std::string> grouping;
  grouping.push_back("");
  grouping.push_back("All");
  grouping.push_back("Group");
  grouping.push_back("Column");
  grouping.push_back("bank");
  declareProperty(
      "GroupDetectorsBy", "", boost::make_shared<StringListValidator>(grouping),
      "Only used if GroupNames is empty: All detectors as one group, Groups "
      "(East,West for SNAP), Columns for SNAP, detector banks");
  declareProperty("MaxRecursionDepth", 5,
                  "Number of levels to search into the instrument (default=5)");

  declareProperty("FixedGroupCount", 0, "Used to distribute the detectors of a "
                                        "given component into a fixed number "
                                        "of groups");
  declareProperty("ComponentName", "", "Specify the instrument component to "
                                       "group into a fixed number of groups");

  declareProperty(new WorkspaceProperty<GroupingWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
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

  // output properties
  declareProperty("NumberGroupedSpectraResult", EMPTY_INT(),
                  "The number of spectra in groups", Direction::Output);
  declareProperty("NumberGroupsResult", EMPTY_INT(), "The number of groups",
                  Direction::Output);
}

//------------------------------------------------------------------------------------------------
/** Read old-style .cal file to get the grouping
 *
 * @param groupingFileName :: path to .cal multi-col ascii
 * @param detIDtoGroup :: map of key=detectorID, value=group number.
 * @param prog :: progress reporter
 */
void readGroupingFile(const std::string &groupingFileName,
                      std::map<detid_t, int> &detIDtoGroup, Progress &prog) {
  std::ifstream grFile(groupingFileName.c_str());
  if (!grFile.is_open()) {
    throw Exception::FileError("Error reading .cal file", groupingFileName);
  }
  detIDtoGroup.clear();
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
  return;
}

/** Creates a mapping based on a fixed number of groups for a given instrument
 *component
 *
 * @param compName Name of component in instrument
 * @param numGroups Number of groups to group detectors into
 * @param inst Pointer to instrument
 * @param detIDtoGroup Map of detector IDs to group number
 * @param prog Progress reporter
 */
void makeGroupingByNumGroups(const std::string compName, int numGroups,
                             Instrument_const_sptr inst,
                             std::map<detid_t, int> &detIDtoGroup,
                             Progress &prog) {
  // Get detectors for given instument component
  std::vector<IDetector_const_sptr> detectors;
  inst->getDetectorsInBank(detectors, compName);
  size_t numDetectors = detectors.size();

  // Sanity check for following calculation
  if(numGroups > static_cast<int>(numDetectors))
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
  std::string groupName = groupi;
  // Take out the "group" part of the group name and convert to an int
  groupName.erase(remove_if(groupName.begin(), groupName.end(),
                            not1(std::ptr_fun(::isdigit))),
                  groupName.end());
  Strings::convert(groupName, i);
  int j = 0;
  groupName = groupj;
  // Take out the "group" part of the group name and convert to an int
  groupName.erase(remove_if(groupName.begin(), groupName.end(),
                            not1(std::ptr_fun(::isdigit))),
                  groupName.end());
  Strings::convert(groupName, j);
  return (i < j);
}

//------------------------------------------------------------------------------------------------
/** Use bank names to build grouping
 *
 * @param GroupNames :: comma-sep list of bank names
 * @param inst :: instrument
 * @param detIDtoGroup :: output: map of detID: to group number
 * @param prog :: progress report
 * @param sortnames :: sort names - a boolean
 */
void makeGroupingByNames(std::string GroupNames, Instrument_const_sptr inst,
                         std::map<detid_t, int> &detIDtoGroup, Progress &prog,
                         bool sortnames) {
  // Split the names of the group and insert in a vector
  std::vector<std::string> vgroups;
  boost::split(vgroups, GroupNames,
               boost::algorithm::detail::is_any_ofF<char>(",/*"));
  while (vgroups.back().empty()) {
    vgroups.pop_back();
  }
  if (sortnames) {
    std::sort(vgroups.begin(), vgroups.end(), groupnumber);
  }

  // Trim and assign incremental number to each group
  std::map<std::string, int> group_map;
  int index = 0;
  for (std::vector<std::string>::iterator it = vgroups.begin();
       it != vgroups.end(); ++it) {
    boost::trim(*it);
    group_map[(*it)] = ++index;
  }

  // Find Detectors that belong to groups
  if (!group_map.empty()) {
    // Find Detectors that belong to groups
    typedef boost::shared_ptr<const Geometry::ICompAssembly> sptr_ICompAss;
    typedef boost::shared_ptr<const Geometry::IComponent> sptr_IComp;
    typedef boost::shared_ptr<const Geometry::IDetector> sptr_IDet;
    std::queue<std::pair<sptr_ICompAss, int>> assemblies;
    sptr_ICompAss current =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(inst);
    sptr_IDet currentDet;
    sptr_IComp currentIComp;
    sptr_ICompAss currentchild;

    int top_group, child_group;

    if (current.get()) {
      top_group = group_map[current->getName()]; // Return 0 if not in map
      assemblies.push(std::make_pair(current, top_group));
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
          currentDet = boost::dynamic_pointer_cast<const Geometry::IDetector>(
              currentIComp);
          if (currentDet.get()) // Is detector
          {
            if (top_group > 0) {
              detIDtoGroup[currentDet->getID()] = top_group;
            }
          } else // Is an assembly, push in the queue
          {
            currentchild =
                boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(
                    currentIComp);
            if (currentchild.get()) {
              child_group = group_map[currentchild->getName()];
              if (child_group == 0)
                child_group = top_group;
              assemblies.push(std::make_pair(currentchild, child_group));
            }
          }
        }
      }
      prog.report();
    }

    return;
  }
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

  // ---------- Get the instrument one of 3 ways ---------------------------
  Instrument_const_sptr inst;
  if (inWS) {
    inst = inWS->getInstrument();
  } else {
    Algorithm_sptr childAlg = createChildAlgorithm("LoadInstrument", 0.0, 0.2);
    MatrixWorkspace_sptr tempWS(new Workspace2D());
    childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
    childAlg->setPropertyValue("Filename", InstrumentFilename);
    childAlg->setPropertyValue("InstrumentName", InstrumentName);
    childAlg->executeAsChildAlg();
    inst = tempWS->getInstrument();
  }

  if (GroupNames.empty() && OldCalFilename.empty()) {
    if (grouping.compare("All") == 0) {
      GroupNames = inst->getName();
    } else if (inst->getName().compare("SNAP") == 0 &&
               grouping.compare("Group") == 0) {
      GroupNames = "East,West";
    } else {
      sortnames = true;
      GroupNames = "";
      int maxRecurseDepth = this->getProperty("MaxRecursionDepth");

      // cppcheck-suppress syntaxError
          PRAGMA_OMP(parallel for schedule(dynamic, 1) )
          for (int num = 0; num < 300; ++num) {
            PARALLEL_START_INTERUPT_REGION
            std::ostringstream mess;
            mess << grouping << num;
            IComponent_const_sptr comp =
                inst->getComponentByName(mess.str(), maxRecurseDepth);
            PARALLEL_CRITICAL(GroupNames)
            if (comp)
              GroupNames += mess.str() + ",";
            PARALLEL_END_INTERUPT_REGION
          }
          PARALLEL_CHECK_INTERUPT_REGION
    }
  }

  // --------------------------- Create the output --------------------------
  GroupingWorkspace_sptr outWS(new GroupingWorkspace(inst));
  this->setProperty("OutputWorkspace", outWS);

  // This will get the grouping
  std::map<detid_t, int> detIDtoGroup;

  Progress prog(this, 0.2, 1.0, outWS->getNumberHistograms());
  // Make the grouping one of two ways:
  if (!GroupNames.empty())
    makeGroupingByNames(GroupNames, inst, detIDtoGroup, prog, sortnames);
  else if (!OldCalFilename.empty())
    readGroupingFile(OldCalFilename, detIDtoGroup, prog);
  else if ((numGroups > 0) && !componentName.empty())
    makeGroupingByNumGroups(componentName, numGroups, inst, detIDtoGroup, prog);

  g_log.information() << detIDtoGroup.size()
                      << " entries in the detectorID-to-group map.\n";
  setProperty("NumberGroupedSpectraResult",
              static_cast<int>(detIDtoGroup.size()));

  if (detIDtoGroup.empty()) {
    g_log.warning() << "Creating empty group workspace\n";
    setProperty("NumberGroupsResult", static_cast<int>(0));
  } else {
    size_t numNotFound = 0;

    // Make the groups, if any
    std::map<detid_t, int>::const_iterator it_end = detIDtoGroup.end();
    std::map<detid_t, int>::const_iterator it;
    std::set<int> groupCount;
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
      g_log.warning() << numNotFound << " detector IDs (out of "
                      << detIDtoGroup.size()
                      << ") were not found in the instrument\n.";
  }
}

} // namespace Mantid
} // namespace Algorithms
