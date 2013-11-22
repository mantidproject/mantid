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
    std::string filename = getPropertyValue("Filename"); 

    IAlgorithm_sptr loadAlg = createChildAlgorithm("LoadMuonNexus");
    loadAlg->setPropertyValue("Filename", filename);
    loadAlg->executeAsChildAlg();

    Workspace_sptr loadedWS = loadAlg->getProperty("OutputWorkspace");

    std::string dtcType = getPropertyValue("DtcType");

    if ( dtcType == "None" )
    {
      setProperty("OutputWorkspace", loadedWS);
    }
    else if ( dtcType == "FromData" )
    {
      int numPeriods = 1; // For now

      Workspace_sptr deadTimes = loadDeadTimesFromNexus(filename, numPeriods);
      Workspace_sptr correctedWS = applyDtc(loadedWS, deadTimes);
      setProperty("OutputWorkspace", correctedWS);
    }
    else
    {
      // TODO
    }
  }

  /**
   * Attempts to load dead time table from given Muon Nexus file.
   * @param filename :: Path of the Muon Nexus file to load dead times from
   * @param numPeriods :: Number of data collection periods
   * @return TableWorkspace when one period, otherwise a group of TableWorkspace-s with dead times
   */
  Workspace_sptr MuonLoadCorrected::loadDeadTimesFromNexus(const std::string& filename, int numPeriods)
  {
    NeXus::NXRoot root(filename);

    NeXus::NXFloat deadTimes = root.openNXFloat("run/instrument/detector/deadtimes");
    deadTimes.load();

    std::vector<double> dtVector;
    dtVector.reserve( deadTimes.size() );

    for (int i = 0; i < deadTimes.size(); i++)
      dtVector.push_back(deadTimes[i]);

    return createDeadTimeTable( dtVector.begin(), dtVector.end() );
  }

  /**
   * Creates Dead Time Table from the given list of dead times.
   *
   * @param begin :: Iterator to the first element of the data to use
   * @param   end :: Iterator to the last element of the data to use
   * @return TableWorkspace created using the data
   */
  TableWorkspace_sptr MuonLoadCorrected::createDeadTimeTable( std::vector<double>::const_iterator begin,
    std::vector<double>::const_iterator end)
  {
    TableWorkspace_sptr deadTimeTable = boost::dynamic_pointer_cast<TableWorkspace>( 
      WorkspaceFactory::Instance().createTable("TableWorkspace") );

    deadTimeTable->addColumn("int","spectrum");
    deadTimeTable->addColumn("double","dead-time");

    int s = 1; // Current spectrum

    for(auto it = begin; it != end; it++)
    {
      TableRow row = deadTimeTable->appendRow();
      row << s++ << *it;
    }

    return deadTimeTable;
  }

  /**
   * Applies dead time correction to a workspace.
   * @param  ws :: Workspace to apply correction to
   * @param  dt :: Dead Times to use
   * @return Corrected workspace
   */
  Workspace_sptr MuonLoadCorrected::applyDtc(Workspace_sptr ws, Workspace_sptr dt)
  {
    if ( auto wsMatrix = boost::dynamic_pointer_cast<MatrixWorkspace>(ws) )
    {
      if ( auto dtTable = boost::dynamic_pointer_cast<TableWorkspace>(dt) )
      {
        return runApplyDtc(wsMatrix, dtTable); 
      }
      else
        throw std::invalid_argument("Can't apply group of dead time tables to a single workspace");
    }

    return Workspace_sptr();
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
