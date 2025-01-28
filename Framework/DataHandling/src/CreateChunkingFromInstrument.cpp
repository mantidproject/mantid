// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/CreateChunkingFromInstrument.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/OptionalBool.h"

#include "MantidKernel/StringTokenizer.h"

#include "MantidNexusCpp/NeXusException.hpp"
#include "MantidNexusCpp/NeXusFile.hpp"

#include <algorithm>

namespace Mantid::DataHandling {
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Types::Core::DateAndTime;
using namespace std;

using tokenizer = Mantid::Kernel::StringTokenizer;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateChunkingFromInstrument)

namespace { // anonymous namespace to hide things
/// Input file name
const string PARAM_IN_FILE("Filename");
/// Input workspace parameter name
const string PARAM_IN_WKSP("InputWorkspace");
/// Instrument name parameter name
const string PARAM_INST_NAME("InstrumentName");
/// Instrument file parameter name
const string PARAM_INST_FILE("InstrumentFilename");
/// Explicitly name instrument components
const string PARAM_CHUNK_NAMES("ChunkNames");
/// Canned instrument components names
const string PARAM_CHUNK_BY("ChunkBy");
/// Recursion depth parameter name
const string PARAM_MAX_RECURSE("MaxRecursionDepth");
/// Output workspace parameter name
const string PARAM_OUT_WKSP("OutputWorkspace");
/// Maximum number of banks to look for
const string PARAM_MAX_BANK_NUM("MaxBankNumber");
} // namespace

/// Algorithm's name for identification. @see Algorithm::name
const string CreateChunkingFromInstrument::name() const { return "CreateChunkingFromInstrument"; }

/// Algorithm's version for identification. @see Algorithm::version
int CreateChunkingFromInstrument::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const string CreateChunkingFromInstrument::category() const { return "Workflow\\DataHandling"; }

/// Algorithm's summary for identification. @see Algorithm::summary
const string CreateChunkingFromInstrument::summary() const {
  return "Creates chunking at a level of the instrument or instrument "
         "components.";
}

/** Initialize the algorithm's properties.
 */
void CreateChunkingFromInstrument::init() {
  // instrument selection
  string grp1Name("Specify the Instrument");

  std::vector<std::string> extensions{"_event.nxs", ".nxs.h5", ".nxs"};
  this->declareProperty(std::make_unique<FileProperty>(PARAM_IN_FILE, "", FileProperty::OptionalLoad, extensions),
                        "The name of the event nexus file to read, including its full or "
                        "relative path.");

  this->declareProperty(
      std::make_unique<WorkspaceProperty<>>(PARAM_IN_WKSP, "", Direction::Input, PropertyMode::Optional),
      "Optional: An input workspace with the instrument we want to use.");

  this->declareProperty(std::make_unique<PropertyWithValue<string>>(PARAM_INST_NAME, "", Direction::Input),
                        "Optional: Name of the instrument to base the ChunkingWorkpace on which "
                        "to base the GroupingWorkspace.");

  this->declareProperty(std::make_unique<FileProperty>(PARAM_INST_FILE, "", FileProperty::OptionalLoad, ".xml"),
                        "Optional: Path to the instrument definition file on which to base the "
                        "ChunkingWorkpace.");

  this->setPropertyGroup(PARAM_IN_FILE, grp1Name);
  this->setPropertyGroup(PARAM_IN_WKSP, grp1Name);
  this->setPropertyGroup(PARAM_INST_NAME, grp1Name);
  this->setPropertyGroup(PARAM_INST_FILE, grp1Name);

  // chunking
  string grp2Name("Specify Instrument Components");

  declareProperty(PARAM_CHUNK_NAMES, "",
                  "Optional: A string of the instrument component names to use "
                  "as separate groups. "
                  "Use / or , to separate multiple groups. "
                  "If empty, then an empty GroupingWorkspace will be created.");
  vector<string> grouping{"", "All", "Group", "Column", "bank"};
  declareProperty(PARAM_CHUNK_BY, "", std::make_shared<StringListValidator>(grouping),
                  "Only used if GroupNames is empty: All detectors as one group, Groups "
                  "(East,West for SNAP), Columns for SNAP, detector banks");

  this->setPropertyGroup(PARAM_CHUNK_NAMES, grp2Name);
  this->setPropertyGroup(PARAM_CHUNK_BY, grp2Name);

  // everything else
  declareProperty(PARAM_MAX_RECURSE, 5, "Number of levels to search into the instrument (default=5)");
  declareProperty(PARAM_MAX_BANK_NUM, 300, "Maximum bank number to search for in the instrument");

  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>(PARAM_OUT_WKSP, "", Direction::Output),
                  "An output workspace describing the cunking.");
}

/// @copydoc Mantid::API::Algorithm::validateInputs
map<string, string> CreateChunkingFromInstrument::validateInputs() {
  map<string, string> result;

  // get the input paramters
  string filename = getPropertyValue(PARAM_IN_FILE);
  string inWSname = getPropertyValue(PARAM_IN_WKSP);
  string instName = getPropertyValue(PARAM_INST_NAME);
  string instFilename = getPropertyValue(PARAM_INST_FILE);

  // count how many ways the input instrument was specified
  int numInst = 0;
  if (!filename.empty())
    numInst++;
  if (!inWSname.empty())
    numInst++;
  if (!instName.empty())
    numInst++;
  if (!instFilename.empty())
    numInst++;

  // set the error bits
  string msg;
  if (numInst == 0) {
    msg = "Must specify instrument one way";
  } else if (numInst > 1) {
    msg = "Can only specify instrument one way";
  }
  if (!msg.empty()) {
    result[PARAM_IN_FILE] = msg;
    result[PARAM_IN_WKSP] = msg;
    result[PARAM_INST_NAME] = msg;
    result[PARAM_INST_FILE] = msg;
  }

  // get the chunking technology to use
  string chunkNames = getPropertyValue(PARAM_CHUNK_NAMES);
  string chunkGroups = getPropertyValue(PARAM_CHUNK_BY);
  msg = "";
  if (chunkNames.empty() && chunkGroups.empty()) {
    msg = "Must specify either " + PARAM_CHUNK_NAMES + " or " + PARAM_CHUNK_BY;
  } else if ((!chunkNames.empty()) && (!chunkGroups.empty())) {
    msg = "Must specify either " + PARAM_CHUNK_NAMES + " or " + PARAM_CHUNK_BY + " not both";
  }
  if (!msg.empty()) {
    result[PARAM_CHUNK_NAMES] = msg;
    result[PARAM_CHUNK_BY] = msg;
  }

  return result;
}

/**
 * Returns true if str starts with prefix.
 *
 * @param str The string to check.
 * @param prefix The prefix to look for.
 * @return true if str starts with prefix.
 */
bool startsWith(const string &str, const string &prefix) {
  // can't start with if it is shorter than the prefix
  if (str.length() < prefix.length())
    return false;

  return (str.substr(0, prefix.length()) == prefix);
}

/**
 * Find the name of the parent of the component that starts with the
 * supplied prefix.
 *
 * @param comp The component to find the parent of.
 * @param prefix Prefix of parent names to look for.
 * @return The correct parent name. This is an empty string if the name
 * isn't found.
 */
string parentName(const IComponent_const_sptr &comp, const string &prefix) {
  // handle the special case of the component has the name
  if (startsWith(comp->getName(), prefix))
    return comp->getName();

  // find the parent with the correct name
  IComponent_const_sptr parent = comp->getParent();
  if (parent) {
    if (startsWith(parent->getName(), prefix))
      return parent->getName();
    else
      return parentName(parent, prefix);
  } else {
    return "";
  }
}

/**
 * Find the name of the parent of the component that is in the list of
 * parents that are being searched for.
 *
 * @param comp The component to find the parent of.
 * @param names List of parent names to look for.
 * @return The correct parent name. This is an empty string if the name
 * isn't found.
 */
string parentName(const IComponent_const_sptr &comp, const vector<string> &names) {
  // handle the special case of the component has the name
  const auto compName = comp->getName();
  auto it = std::find(names.cbegin(), names.cend(), compName);
  if (it != names.cend())
    return *it;

  // find the parent with the correct name
  IComponent_const_sptr parent = comp->getParent();
  if (parent) {
    const auto parName = parent->getName();
    // see if this is the parent
    it = std::find(names.cbegin(), names.cend(), parName);
    if (it != names.cend())
      return *it;
    // or recurse
    return parentName(parent, names);
  } else {
    return "";
  }
}

/**
 * Split a list of instrument components into a vector of strings.
 *
 * @param names Comma separated list of instrument components
 * @return The vector of instrument component names.
 */
vector<string> getGroupNames(const string &names) {
  // check that there is something
  if (names.empty())
    return std::vector<string>();
  // do the actual splitting
  tokenizer tokens(names, ",", Mantid::Kernel::StringTokenizer::TOK_TRIM);
  return tokens.asVector();
}

/**
 * Determine the instrument from the various input parameters.
 *
 * @return The correct instrument.
 */
Instrument_const_sptr CreateChunkingFromInstrument::getInstrument() {
  // try the input workspace
  MatrixWorkspace_sptr inWS = getProperty(PARAM_IN_WKSP);
  if (inWS) {
    return inWS->getInstrument();
  }

  // temporary workspace to hang everything else off of
  MatrixWorkspace_sptr tempWS(new Workspace2D());
  // name of the instrument
  string instName = getPropertyValue(PARAM_INST_NAME);

  // see if there is an input file
  string filename = getPropertyValue(PARAM_IN_FILE);
  if (!filename.empty()) {
    string top_entry_name("entry"); // TODO make more flexible

    // get the instrument name from the filename
    size_t n = filename.rfind('/');
    if (n != std::string::npos) {
      std::string temp = filename.substr(n + 1, filename.size() - n - 1);
      n = temp.find('_');
      if (n != std::string::npos && n > 0) {
        instName = temp.substr(0, n);
      }
    }

    // read information from the nexus file itself
    try {
      NeXus::File nxsfile(filename);

      // get the run start time
      string start_time;
      nxsfile.openGroup(top_entry_name, "NXentry");
      nxsfile.readData("start_time", start_time);
      tempWS->mutableRun().addProperty("run_start", DateAndTime(start_time).toISO8601String(), true);

      // get the instrument name
      nxsfile.openGroup("instrument", "NXinstrument");
      nxsfile.readData("name", instName);
      nxsfile.closeGroup();

      // Test if IDF exists in file, move on quickly if not
      nxsfile.openPath("instrument/instrument_xml");
      nxsfile.close();
      auto loadInst = createChildAlgorithm("LoadIDFFromNexus", 0.0, 0.2);
      // Now execute the Child Algorithm. Catch and log any error, but don't
      // stop.
      try {
        loadInst->setPropertyValue("Filename", filename);
        loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
        loadInst->setPropertyValue("InstrumentParentPath", top_entry_name);
        loadInst->execute();
      } catch (std::invalid_argument &) {
        g_log.error("Invalid argument to LoadIDFFromNexus Child Algorithm ");
      } catch (std::runtime_error &) {
        g_log.debug("No instrument definition found in " + filename + " at " + top_entry_name + "/instrument");
      }

      if (loadInst->isExecuted())
        return tempWS->getInstrument();
      else
        g_log.information("No IDF loaded from Nexus file.");

    } catch (::NeXus::Exception &) {
      g_log.information("No instrument definition found in " + filename + " at " + top_entry_name + "/instrument");
    }
  }

  // run LoadInstrument if other methods have not run
  string instFilename = getPropertyValue(PARAM_INST_FILE);

  Algorithm_sptr childAlg = createChildAlgorithm("LoadInstrument", 0.0, 0.2);
  childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
  childAlg->setPropertyValue("Filename", instFilename);
  childAlg->setPropertyValue("InstrumentName", instName);
  childAlg->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
  childAlg->executeAsChildAlg();
  return tempWS->getInstrument();
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateChunkingFromInstrument::exec() {
  // get the instrument
  Instrument_const_sptr inst = this->getInstrument();

  // setup the output workspace
  ITableWorkspace_sptr strategy = WorkspaceFactory::Instance().createTable("TableWorkspace");
  strategy->addColumn("str", "BankName");
  this->setProperty("OutputWorkspace", strategy);

  // get the correct level of grouping
  string groupLevel = this->getPropertyValue(PARAM_CHUNK_BY);
  vector<string> groupNames = getGroupNames(this->getPropertyValue(PARAM_CHUNK_NAMES));
  if (groupLevel == "All") {
    return; // nothing to do
  } else if (inst->getName() == "SNAP" && groupLevel == "Group") {
    groupNames.clear();
    groupNames.emplace_back("East");
    groupNames.emplace_back("West");
  }

  // set up a progress bar with the "correct" number of steps
  int maxBankNum = this->getProperty(PARAM_MAX_BANK_NUM);
  Progress progress(this, .2, 1., maxBankNum);

  // search the instrument for the bank names
  int maxRecurseDepth = this->getProperty(PARAM_MAX_RECURSE);
  map<string, vector<string>> grouping;

    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (int num = 0; num < maxBankNum; ++num) {
      PARALLEL_START_INTERRUPT_REGION
      ostringstream mess;
      mess << "bank" << num;
      IComponent_const_sptr comp = inst->getComponentByName(mess.str(), maxRecurseDepth);
      PARALLEL_CRITICAL(grouping)
      if (comp) {
        // get the name of the correct parent
        string parent;
        if (groupNames.empty()) {
          parent = parentName(comp, groupLevel);
        } else {
          parent = parentName(comp, groupNames);
        }

        // add it to the correct chunk
        if (!parent.empty()) {
          grouping.try_emplace(parent, vector<string>());
          grouping[parent].emplace_back(comp->getName());
        }
      }
      progress.report();
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION

    // check to see that something happened
    if (grouping.empty())
      throw std::runtime_error("Failed to find any banks in the instrument");

    // fill in the table workspace
    for (const auto &group : grouping) {
      stringstream banks;
      for (const auto &bank : group.second) {
        banks << bank << ",";
      }
      // remove the trailing comma
      string banksStr = banks.str();
      // cppcheck-suppress uselessCallsSubstr
      banksStr = banksStr.substr(0, banksStr.size() - 1);

      // add it to the table
      TableRow row = strategy->appendRow();
      row << banksStr;
    }
}

} // namespace Mantid::DataHandling
