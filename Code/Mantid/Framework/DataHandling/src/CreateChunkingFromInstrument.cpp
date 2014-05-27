/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/CreateChunkingFromInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid
{
namespace DataHandling
{
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;
  using namespace Mantid::Kernel;
  using namespace std;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CreateChunkingFromInstrument)
  

  namespace { // anonymous namespace to hide things
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
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CreateChunkingFromInstrument::CreateChunkingFromInstrument()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CreateChunkingFromInstrument::~CreateChunkingFromInstrument()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const string CreateChunkingFromInstrument::name() const { return "CreateChunkingFromInstrument";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int CreateChunkingFromInstrument::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const string CreateChunkingFromInstrument::category() const { return "Workflow\\DataHandling";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void CreateChunkingFromInstrument::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CreateChunkingFromInstrument::init()
  {
    // instrument selection
    string grp1Name("Specify the Instrument");

    this->declareProperty(new WorkspaceProperty<>(PARAM_IN_WKSP,"",Direction::Input, PropertyMode::Optional),
                          "Optional: An input workspace with the instrument we want to use.");

    this->declareProperty(new PropertyWithValue<string>(PARAM_INST_NAME,"",Direction::Input),
                          "Optional: Name of the instrument to base the ChunkingWorkpace on which to base the GroupingWorkspace.");

    this->declareProperty(new FileProperty(PARAM_INST_FILE, "", FileProperty::OptionalLoad, ".xml"),
                          "Optional: Path to the instrument definition file on which to base the ChunkingWorkpace.");

    this->setPropertyGroup(PARAM_IN_WKSP, grp1Name);
    this->setPropertyGroup(PARAM_INST_NAME, grp1Name);
    this->setPropertyGroup(PARAM_INST_FILE, grp1Name);

    // chunking
    string grp2Name("Specify Instrument Components");

    declareProperty(PARAM_CHUNK_NAMES,"",
      "Optional: A string of the instrument component names to use as separate groups. "
      "Use / or , to separate multiple groups. "
      "If empty, then an empty GroupingWorkspace will be created.");
    vector<string> grouping;
    grouping.push_back("");
    grouping.push_back("All");
    grouping.push_back("Group");
    grouping.push_back("Column");
    grouping.push_back("bank");
    declareProperty(PARAM_CHUNK_BY, "", boost::make_shared<StringListValidator>(grouping),
        "Only used if GroupNames is empty: All detectors as one group, Groups (East,West for SNAP), Columns for SNAP, detector banks");

    this->setPropertyGroup(PARAM_CHUNK_NAMES, grp2Name);
    this->setPropertyGroup(PARAM_CHUNK_BY, grp2Name);

    // everything else
    declareProperty(PARAM_MAX_RECURSE, 5,
                    "Number of levels to search into the instrument (default=5)");
    declareProperty(PARAM_MAX_BANK_NUM, 300,
                    "Maximum bank number to search for in the instrument");

    declareProperty(new WorkspaceProperty<API::ITableWorkspace>(PARAM_OUT_WKSP,"",Direction::Output),
                    "An output workspace describing the cunking.");
  }

  /// @copydoc Mantid::API::Algorithm::validateInputs
  map<string, string> CreateChunkingFromInstrument::validateInputs()
  {
    map<string, string> result;

    // get the input paramters
    MatrixWorkspace_sptr inWS = getProperty(PARAM_IN_WKSP);
    string instName = getPropertyValue(PARAM_INST_NAME);
    string instFilename = getPropertyValue(PARAM_INST_FILE);

    // input workspace wins
    int numInst = 0;

    if (inWS) numInst++;
    if (!instName.empty()) numInst++;
    if (!instFilename.empty()) numInst++;

    // set the error bits
    if (numInst == 0)
    {
      result[PARAM_IN_WKSP]   = "Must specify instrument one way";
      result[PARAM_INST_NAME] = "Must specify instrument one way";
      result[PARAM_INST_FILE] = "Must specify instrument one way";
    }
    else if (numInst > 1)
    {
      result[PARAM_IN_WKSP]   = "Can only specify instrument one way";
      result[PARAM_INST_NAME] = "Can only specify instrument one way";
      result[PARAM_INST_FILE] = "Can only specify instrument one way";
    }

    return result;
  }

  bool startsWith(const string & str, const string & prefix)
  {
    // can't start with if it is shorter than the prefix
    if (str.length() < prefix.length())
      return false;

    return (str.substr(0, prefix.length()).compare(prefix) == 0);
  }

  string parentName(IComponent_const_sptr comp, const string & prefix)
  {
    // handle the special case of the component has the name
    if (startsWith(comp->getName(), prefix))
      return comp->getName();

    // find the parent with the correct name
    IComponent_const_sptr parent = comp->getParent();
    if (parent)
    {
      if (startsWith(parent->getName(), prefix))
        return parent->getName();
      else
        return parentName(parent, prefix);
    }
    else
    {
      return "";
    }
  }

  string parentName(IComponent_const_sptr comp, const vector<string> & names)
  {
    // handle the special case of the component has the name
    for (auto name = names.begin(); name != names.end(); ++name)
      if (name->compare(comp->getName()) == 0)
        return (*name);

    // find the parent with the correct name
    IComponent_const_sptr parent = comp->getParent();
    if (parent)
    {
      // see if this is the parent
      for (auto name = names.begin(); name != names.end(); ++name)
        if (name->compare(parent->getName()) == 0)
          return (*name);

      // or recurse
      return parentName(parent, names);
    }
    else
    {
      return "";
    }
  }

  vector<string> getGroupNames(const string & names)
  {
    vector<string> groups;

    // check that there is something
    if (names.empty())
      return groups;

    // TODO should do the actual splitting

    return groups;
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CreateChunkingFromInstrument::exec()
  {
    // get the input parameters
    MatrixWorkspace_sptr inWS = getProperty(PARAM_IN_WKSP);
    string instName = getPropertyValue(PARAM_INST_NAME);
    string instFilename = getPropertyValue(PARAM_INST_FILE);

    // get the instrument
    Instrument_const_sptr inst;
    if (inWS)
    {
      inst = inWS->getInstrument();
    }
    else
    {
      Algorithm_sptr childAlg = createChildAlgorithm("LoadInstrument",0.0,0.2);
      MatrixWorkspace_sptr tempWS(new Workspace2D());
      childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
      childAlg->setPropertyValue("Filename", instFilename);
      childAlg->setPropertyValue("InstrumentName", instName);
      childAlg->executeAsChildAlg();
      inst = tempWS->getInstrument();
    }

    // setup the output workspace
    ITableWorkspace_sptr strategy = WorkspaceFactory::Instance().createTable("TableWorkspace");
    strategy->addColumn("str", "BankName");
    this->setProperty("OutputWorkspace", strategy);

    // get the correct level of grouping
    string groupLevel = this->getPropertyValue(PARAM_CHUNK_BY);
    vector<string> groupNames = getGroupNames(this->getPropertyValue(PARAM_CHUNK_NAMES));
    if (groupLevel.compare("All") == 0)
    {
      groupNames.clear();
      groupNames.push_back(inst->getName());
    }
    else if (inst->getName().compare("SNAP") == 0 && groupLevel.compare("Group") == 0)
    {
      groupNames.clear();
      groupNames.push_back("East");
      groupNames.push_back("West");
    }

    // set up a progress bar with the "correct" number of steps
    int maxBankNum = this->getProperty(PARAM_MAX_BANK_NUM);
    Progress progress(this, .2, 1., maxBankNum);

    // search the instrument for the bank names
    int maxRecurseDepth = this->getProperty(PARAM_MAX_RECURSE);
    map<string, vector<string> > grouping;
    // cppcheck-suppress syntaxError
    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (int num = 0; num < maxBankNum; ++num)
    {
      PARALLEL_START_INTERUPT_REGION
      ostringstream mess;
      mess<< "bank"<<num;
      IComponent_const_sptr comp = inst->getComponentByName(mess.str(), maxRecurseDepth);
      PARALLEL_CRITICAL(grouping)
      if(comp)
      {
        // get the name of the correct parent
        string parent;
        if (groupNames.empty())
        {
          parent = parentName(comp, groupLevel);
        }
        else
        {
          parent = parentName(comp, groupNames);
        }

        // add it to the correct chunk
        if (!parent.empty())
        {
          if (grouping.count(parent) == 0)
            grouping[parent] = vector<string>();

          grouping[parent].push_back(comp->getName());
        }
      }
      progress.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    // fill in the table workspace
    for (auto group = grouping.begin(); group != grouping.end(); ++group)
    {
      stringstream banks;
      for (auto bank = group->second.begin(); bank != group->second.end(); ++bank)
        banks << (*bank) << ",";

      // remove the trailing comma
      string banksStr = banks.str();
      banksStr = banksStr.substr(0, banksStr.size()-1);

      // add it to the table
      TableRow row = strategy->appendRow();
      row << banksStr;
    }

  }



} // namespace DataHandling
} // namespace Mantid
