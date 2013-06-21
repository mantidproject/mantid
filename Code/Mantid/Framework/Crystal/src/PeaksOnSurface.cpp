/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidCrystal/PeaksOnSurface.h"

using namespace Mantid::Kernel;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(PeaksOnSurface)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PeaksOnSurface::PeaksOnSurface()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PeaksOnSurface::~PeaksOnSurface()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string PeaksOnSurface::name() const { return "PeaksOnSurface";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int PeaksOnSurface::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string PeaksOnSurface::category() const { return "Crystal";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void PeaksOnSurface::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void PeaksOnSurface::init()
  {
    
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void PeaksOnSurface::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Crystal
} // namespace Mantid