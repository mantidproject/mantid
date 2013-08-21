/*WIKI*

Clears the OrientedLattice of each ExperimentInfo attached to the intput [[Workspace]]. Works with both single ExperimentInfos and MultipleExperimentInfo instances.

*WIKI*/

#include "MantidCrystal/ClearUB.h"
#include "MantidAPI/MultipleExperimentInfos.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ClearUB)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ClearUB::ClearUB()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ClearUB::~ClearUB()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string ClearUB::name() const { return "ClearUB";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int ClearUB::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string ClearUB::category() const { return "Crystal";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void ClearUB::initDocs()
  {
    this->setWikiSummary("Clears the UB by removing the oriented lattice from the sample.");
    this->setOptionalMessage(this->getWikiSummary());
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ClearUB::init()
  {
    declareProperty(new WorkspaceProperty<Workspace>("Workspace","",Direction::InOut), "Workspace to clear the UB from.");
  }

  /**
   * Clear the Oriented Lattice from a single experiment info.
   * @param experimentInfo
   */
  void ClearUB::clearSingleExperimentInfo(ExperimentInfo * const experimentInfo) const
  {
    Sample& sampleObject = experimentInfo->mutableSample();
    if(!sampleObject.hasOrientedLattice())
    {
      this->g_log.information("Workspace has no oriented lattice to clear.");
    }
    else
    {
      sampleObject.clearOrientedLattice();
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ClearUB::exec()
  {
    Workspace_sptr ws = getProperty("Workspace");
    ExperimentInfo_sptr experimentInfo = boost::dynamic_pointer_cast<ExperimentInfo>(ws);
    if(experimentInfo)
    {
      clearSingleExperimentInfo(experimentInfo.get());
    }
    else
    {
      MultipleExperimentInfos_sptr experimentInfos = boost::dynamic_pointer_cast<MultipleExperimentInfos>(ws);
      if(!experimentInfos)
      {
        throw std::runtime_error("Input workspace is neither of type ExperimentInfo or MultipleExperimentInfo, cannot process.");
      }
      const uint16_t nInfos = experimentInfos->getNumExperimentInfo();
      for(uint16_t i = 0; i < nInfos; ++i)
      {
        ExperimentInfo_sptr info = experimentInfos->getExperimentInfo(i);
        clearSingleExperimentInfo(info.get());
      }
    }
  }



} // namespace Crystal
} // namespace Mantid
