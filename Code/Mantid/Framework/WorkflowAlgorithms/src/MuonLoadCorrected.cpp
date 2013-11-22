/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidWorkflowAlgorithms/MuonLoadCorrected.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{

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
  /** Initialize the algorithm's properties.
   */
  void MuonLoadCorrected::init()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MuonLoadCorrected::exec()
  {
  }

} // namespace WorkflowAlgorithms
} // namespace Mantid
