/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAlgorithms/RadiusSum.h"

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(RadiusSum)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  RadiusSum::RadiusSum()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  RadiusSum::~RadiusSum()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string RadiusSum::name() const { return "RadiusSum";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int RadiusSum::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string RadiusSum::category() const { return TODO: FILL IN A CATEGORY;}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void RadiusSum::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void RadiusSum::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void RadiusSum::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Algorithms
} // namespace Mantid