/*WIKI*
Load Muon data with Dead Time Correction applied. Part of the Muon workflow.
*WIKI*/

#include "MantidWorkflowAlgorithms/MuonLoadCorrected.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MuonLoadCorrected)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MuonLoadCorrected::MuonLoadCorrected()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MuonLoadCorrected::~MuonLoadCorrected()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string MuonLoadCorrected::name() const { return "MuonLoadCorrected";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int MuonLoadCorrected::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string MuonLoadCorrected::category() const { return "Workflow\\Muon";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MuonLoadCorrected::initDocs()
  {
    this->setWikiSummary("Loads Muon data with Dead Time Correction applied.");
    this->setOptionalMessage("Loads Muon data with Dead Time Correction applied.");
  }

  //----------------------------------------------------------------------------------------------
  /** 
   * Initialize the algorithm's properties.
   */
  void MuonLoadCorrected::init()
  {
    declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
      "The name of the Nexus file to load" );      
    
    std::vector<std::string> dtcTypes;
    dtcTypes.push_back("None");
    dtcTypes.push_back("FromData");
    dtcTypes.push_back("FromSpecifiedFile");

    declareProperty("DtcType","None", boost::make_shared<StringListValidator>(dtcTypes),
      "Type of dead time correction to apply");

    declareProperty(new FileProperty("DtcFile", "", FileProperty::OptionalLoad, ".nxs"),
      "File with dead time values. Used only when DtcType is FromSpecifiedFile.");      

    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output),
      "The name of the workspace to be created as the output of the algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** 
   * Execute the algorithm.
   */
  void MuonLoadCorrected::exec()
  {
    const std::string filename = getPropertyValue("Filename"); 

    IAlgorithm_sptr loadAlg = createChildAlgorithm("LoadMuonNexus");
    loadAlg->setPropertyValue("Filename", filename);
    loadAlg->executeAsChildAlg();

    Workspace_sptr loadedWS = loadAlg->getProperty("OutputWorkspace");

    std::string dtcType = getPropertyValue("DtcType");

    if ( dtcType == "None" )
    {
      setProperty("OutputWorkspace", loadedWS);
    }
    else
    {
      Workspace_sptr deadTimes; 
     
      if ( dtcType == "FromData" )
      {
        deadTimes = loadAlg->getProperty("DeadTimeTable");
      }
      else if ( dtcType == "FromSpecifiedFile" ) 
      {
        const std::string dtcFile = getPropertyValue("DtcFile"); 

        deadTimes = loadDeadTimesFromNexus(dtcFile);
      }

      Workspace_sptr correctedWS = applyDtc(loadedWS, deadTimes);
      setProperty("OutputWorkspace", correctedWS);
    }
  }

  /**
   * Attempts to load dead time table from custom file.
   * @param filename :: Path to the file. 
   * @param numPeriods :: Number of data collection periods
   * @return TableWorkspace when one period, otherwise a group of TableWorkspace-s with dead times
   */
  Workspace_sptr MuonLoadCorrected::loadDeadTimesFromNexus(const std::string& filename)
  {
    IAlgorithm_sptr loadNexusProc = createChildAlgorithm("LoadNexusProcessed");
    loadNexusProc->setPropertyValue("Filename", filename);
    loadNexusProc->execute();

    return loadNexusProc->getProperty("OutputWorkspace");
  }

  /**
   * Applies dead time correction to a workspace.
   * @param  ws :: Workspace to apply correction to
   * @param  dt :: Dead Times to use
   * @return Corrected workspace
   */
  Workspace_sptr MuonLoadCorrected::applyDtc(Workspace_sptr ws, Workspace_sptr dt)
  {
    using namespace boost; // We will use it a lot

    if ( auto wsMatrix = dynamic_pointer_cast<MatrixWorkspace>(ws) )
    {
      if ( auto dtTable = dynamic_pointer_cast<TableWorkspace>(dt) )
      {
        return runApplyDtc(wsMatrix, dtTable); 
      }
      else if ( auto dtGroup = dynamic_pointer_cast<WorkspaceGroup>(ws) )
      {
        throw std::invalid_argument("Can't apply group of dead time tables to a single workspace");
      }
      else
      {
        throw std::invalid_argument("Incorrect DeadTimeTable type");
      }
    }
    else if ( auto wsGroup = dynamic_pointer_cast<WorkspaceGroup>(ws) )
    {
      if ( auto dtTable = dynamic_pointer_cast<TableWorkspace>(dt) )
      {
        return applyDtcTableToGroup(wsGroup, dtTable);
      }
      else if ( auto dtGroup = dynamic_pointer_cast<WorkspaceGroup>(dt) )
      {
        return applyDtcGroupToGroup(wsGroup, dtGroup);
      }
      else
      {
        throw std::invalid_argument("Incorrect DeadTimeTable type");
      }
    }
    else
    {
      throw std::invalid_argument("Unsupported workspace type");
    }
  }
  
  /**
   * Applies dead time table to a group of workspaces.
   * @param wsGroup :: Group to apply correction to
   * @param dtTable :: Dead Time Table
   * @return Group of corrected workspaces
   */
  WorkspaceGroup_sptr MuonLoadCorrected::applyDtcTableToGroup(WorkspaceGroup_sptr wsGroup,
    TableWorkspace_sptr dtTable)
  {
    WorkspaceGroup_sptr outputGroup = boost::make_shared<WorkspaceGroup>();

    for(size_t i = 0; i < wsGroup->size(); i++)
    {
      auto wsMember = boost::dynamic_pointer_cast<MatrixWorkspace>(wsGroup->getItem(i));

      if(!wsMember)
        throw std::invalid_argument("Group contains unsupported type of workspace");

      MatrixWorkspace_sptr outputWs = runApplyDtc(wsMember, dtTable);

      outputGroup->addWorkspace(outputWs);
    }

    return outputGroup;
  }

  /**
   * Applies group of dead time tables to a group of workspaces.
   * @param wsGroup :: Group to apply correction to
   * @param dtGroup :: Group of Dead Time Tables
   * @return Group of corrected workspaces
   */
  WorkspaceGroup_sptr MuonLoadCorrected::applyDtcGroupToGroup(WorkspaceGroup_sptr wsGroup, 
    WorkspaceGroup_sptr dtGroup)
  {
    if(dtGroup->size() != wsGroup->size())
      throw std::invalid_argument("Dead Time Table group size is not equal to ws group size");

    WorkspaceGroup_sptr outputGroup = boost::make_shared<WorkspaceGroup>();

    for(size_t i = 0; i < wsGroup->size(); i++)
    {
      auto wsMember = boost::dynamic_pointer_cast<MatrixWorkspace>(wsGroup->getItem(i));

      if(!wsMember)
        throw std::invalid_argument("Group contains unsupported type of workspace");

      auto dtMember = boost::dynamic_pointer_cast<TableWorkspace>(dtGroup->getItem(i));

      if(!dtMember)
        throw std::invalid_argument("Dead Time Table group contains workspace which is not a table");

      MatrixWorkspace_sptr outputWs = runApplyDtc(wsMember, dtMember);

      outputGroup->addWorkspace(outputWs);
    }

    return outputGroup;
  }


  /**
   * Runs ApplyDeadTimeCorr algorithm.
   * @param  ws :: Workspace to apply correction to
   * @param  dt :: Dead Times to use
   * @return Corrected workspace 
   */
  MatrixWorkspace_sptr MuonLoadCorrected::runApplyDtc(MatrixWorkspace_sptr ws, TableWorkspace_sptr dt)
  {
    IAlgorithm_sptr applyDtc = createChildAlgorithm("ApplyDeadTimeCorr");

    applyDtc->setProperty<MatrixWorkspace_sptr>("InputWorkspace", ws);
    applyDtc->setProperty<ITableWorkspace_sptr>("DeadTimeTable", dt);
    applyDtc->execute();

    MatrixWorkspace_sptr output = applyDtc->getProperty("OutputWorkspace");

    return output;
  }

} // namespace WorkflowAlgorithms
} // namespace Mantid
