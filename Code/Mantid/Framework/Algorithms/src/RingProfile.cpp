/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAlgorithms/RingProfile.h"

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(RingProfile)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  RingProfile::RingProfile()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  RingProfile::~RingProfile()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string RingProfile::name() const { return "RingProfile";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int RingProfile::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string RingProfile::category() const { return TODO: FILL IN A CATEGORY;}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void RingProfile::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void RingProfile::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void RingProfile::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Algorithms
} // namespace Mantid