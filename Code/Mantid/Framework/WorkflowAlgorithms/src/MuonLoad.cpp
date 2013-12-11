/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidWorkflowAlgorithms/MuonLoad.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MuonLoad)

  //----------------------------------------------------------------------------------------------
  /** 
   * Constructor
   */
  MuonLoad::MuonLoad()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** 
   * Destructor
   */
  MuonLoad::~MuonLoad()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string MuonLoad::name() const { return "MuonLoad";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int MuonLoad::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string MuonLoad::category() const { return "Workflow\\Muon";}

  //----------------------------------------------------------------------------------------------

  /// Sets documentation strings for this algorithm
  void MuonLoad::initDocs()
  {
    this->setWikiSummary("Loads Muon workspace ready for analysis.");
    this->setOptionalMessage("Loads Muon workspace ready for analysis.");
  }

  //----------------------------------------------------------------------------------------------
  /*
   * Initialize the algorithm's properties.
   */
  void MuonLoad::init()
  {
    declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
        "The name of the Nexus file to load" );

    declareProperty("FirstPeriod", EMPTY_INT(), "Group index of the first period workspace to use");
    declareProperty("SecondPeriod", EMPTY_INT(), "Group index of the first period workspace to use");

    std::vector<std::string> allowedOperations;
    allowedOperations.push_back("+"); allowedOperations.push_back("-");
    declareProperty("PeriodOperation","+", boost::make_shared<StringListValidator>(allowedOperations),
      "If two periods specified, what operation to apply to workspaces to get a final one.");

    declareProperty("ApplyDeadTimeCorrection", false, 
        "Whether dead time correction should be applied to loaded workspace");
    declareProperty(new FileProperty("CustomDeadTimeFile", "", FileProperty::OptionalLoad, ".nxs"),
        "Nexus file with custom dead time table. See LoadMuonNexus for format expected.");

    declareProperty(new WorkspaceProperty<TableWorkspace>("DetectorGroupingTable","",Direction::Input), 
        "Table with detector grouping information. See LoadMuonNexus for format expected.");

    declareProperty("TimeZero", EMPTY_DBL(), "Value used for Time Zero correction.");
    declareProperty(new ArrayProperty<double>("RebinParams"),
        "Params used for rebinning. If empty - rebinning is not done.");
    declareProperty("Xmin", EMPTY_DBL(), "Minimal X value to include");
    declareProperty("Xmax", EMPTY_DBL(), "Maximal X value to include");

    std::vector<std::string> allowedTypes;
    allowedTypes.push_back("PairAsymmetry");
    allowedTypes.push_back("GroupAsymmetry");
    allowedTypes.push_back("GroupCounts");
    declareProperty("OutputType", "PairAsymmetry", boost::make_shared<StringListValidator>(allowedTypes),
      "What kind of workspace required for analysis.");

    declareProperty("PairFirstIndex", EMPTY_INT(), "Workspace index of the first pair group");
    declareProperty("PairSecondIndex", EMPTY_INT(), "Workspace index of the second pair group");
    declareProperty("PairAlpha", 1.0, "Alpha value of the pair");

    declareProperty("GroupIndex", EMPTY_INT(), "Workspace index of the group");

    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output), 
        "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /**
   * Execute the algorithm.
   */
  void MuonLoad::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace WorkflowAlgorithms
} // namespace Mantid
