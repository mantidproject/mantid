/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidWorkflowAlgorithms/MuonCalculateAsymmetry.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MuonCalculateAsymmetry)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MuonCalculateAsymmetry::MuonCalculateAsymmetry()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MuonCalculateAsymmetry::~MuonCalculateAsymmetry()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string MuonCalculateAsymmetry::name() const { return "MuonCalculateAsymmetry";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int MuonCalculateAsymmetry::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string MuonCalculateAsymmetry::category() const { return TODO: FILL IN A CATEGORY;}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MuonCalculateAsymmetry::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MuonCalculateAsymmetry::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MuonCalculateAsymmetry::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace WorkflowAlgorithms
} // namespace Mantid