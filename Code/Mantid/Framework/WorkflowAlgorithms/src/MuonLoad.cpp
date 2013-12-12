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

    declareProperty("FirstPeriod", 0, "Group index of the first period workspace to use");
    declareProperty("SecondPeriod", EMPTY_INT(), "Group index of the first period workspace to use");

    std::vector<std::string> allowedOperations;
    allowedOperations.push_back("+"); allowedOperations.push_back("-");
    declareProperty("PeriodOperation","+", boost::make_shared<StringListValidator>(allowedOperations),
      "If two periods specified, what operation to apply to workspaces to get a final one.");

    declareProperty("ApplyDeadTimeCorrection", false, 
        "Whether dead time correction should be applied to loaded workspace");
    declareProperty(new FileProperty("CustomDeadTimeFile", "", FileProperty::OptionalLoad, ".nxs"),
        "Nexus file with custom dead time table. See LoadMuonNexus for format expected.");

    declareProperty(new WorkspaceProperty<TableWorkspace>("DetectorGroupingTable","",Direction::Input,
        PropertyMode::Optional), "Table with detector grouping information."
        " See LoadMuonNexus for format expected.");

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
    declareProperty("Alpha", 1.0, "Alpha value of the pair");

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
    const std::string filename = getProperty("Filename");

    // Load the file
    IAlgorithm_sptr load = createChildAlgorithm("LoadMuonNexus");
    load->setProperty("Filename", filename);
    load->execute();

    Workspace_sptr loadedWS = load->getProperty("OutputWorkspace");

    MatrixWorkspace_sptr firstPeriodWS, secondPeriodWS;

    // Deal with single-period workspace
    if ( auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(loadedWS) )
    {
      if ( static_cast<int>( getProperty("FirstPeriod") ) != 0 )
        throw std::invalid_argument("Single period data but first period is not 0.");

      if ( static_cast<int>( getProperty("SecondPeriod") ) != EMPTY_INT() )
        throw std::invalid_argument("Single period data but second period specified");

      firstPeriodWS = ws;
    }
    // Deal with multi-period workspace
    else if ( auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(loadedWS) )
    {
      firstPeriodWS = getFirstPeriodWS(group);
      secondPeriodWS = getSecondPeriodWS(group);
    }
    // Unexpected workspace type
    else
    {
      throw std::runtime_error("Loaded workspace is of invalid type");
    }
    
    IAlgorithm_sptr calcAssym = createChildAlgorithm("MuonCalculateAsymmetry");

    // Set period workspaces
    calcAssym->setProperty("FirstPeriodWorkspace", firstPeriodWS);
    calcAssym->setProperty("SecondPeriodWorkspace", secondPeriodWS);

    // Copy similar properties over
    calcAssym->setProperty("PeriodOperation",
        static_cast<std::string>( getProperty("PeriodOperation") ) );
    calcAssym->setProperty("OutputType",
        static_cast<std::string>( getProperty("OutputType") ) );
    calcAssym->setProperty("PairFirstIndex",
        static_cast<int>( getProperty("PairFirstIndex") ) );
    calcAssym->setProperty("PairSecondIndex",
        static_cast<int>( getProperty("PairSecondIndex") ) );
    calcAssym->setProperty("Alpha",
        static_cast<double>( getProperty("Alpha") ) );
    calcAssym->setProperty("GroupIndex",
        static_cast<int>( getProperty("GroupIndex") ) );

    calcAssym->execute();

    MatrixWorkspace_sptr outWS = calcAssym->getProperty("OutputWorkspace");
    setProperty("OutputWorkspace", outWS);
  }

  /**
   * Returns a workspace for the first period as specified using FirstPeriod property.
   * @param group :: Loaded group of workspaces to use
   * @return Workspace for the period
   */
  MatrixWorkspace_sptr MuonLoad::getFirstPeriodWS(WorkspaceGroup_sptr group)
  {
    int firstPeriod = getProperty("FirstPeriod");  

    MatrixWorkspace_sptr resultWS;
    
    if ( firstPeriod < 0 || firstPeriod >= static_cast<int>( group->size() ) )
      throw std::invalid_argument("Workspace doesn't contain specified first period");

    resultWS = boost::dynamic_pointer_cast<MatrixWorkspace>( group->getItem(firstPeriod) );

    if ( ! resultWS )
      throw std::invalid_argument("First period workspace is not a MatrixWorkspace");

    return resultWS;
  }

  /**
   * Returns a workspace for the second period as specified using SecondPeriod property.
   * @param group :: Loaded group of workspaces to use
   * @return Workspace for the period
   */
  MatrixWorkspace_sptr MuonLoad::getSecondPeriodWS(WorkspaceGroup_sptr group)
  {
    int secondPeriod = getProperty("SecondPeriod");  

    MatrixWorkspace_sptr resultWS;

    if ( secondPeriod != EMPTY_INT() )
    {
      if ( secondPeriod < 0 || secondPeriod >= static_cast<int>( group->size() ) )
        throw std::invalid_argument("Workspace doesn't contain specified second period");

      resultWS = boost::dynamic_pointer_cast<MatrixWorkspace>( group->getItem(secondPeriod) );

      if ( ! resultWS )
        throw std::invalid_argument("Second period workspace is not a MatrixWorkspace");
    }

    return resultWS;
  }

} // namespace WorkflowAlgorithms
} // namespace Mantid
