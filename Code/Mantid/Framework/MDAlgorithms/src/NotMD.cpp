/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidMDAlgorithms/NotMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(NotMD)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  NotMD::NotMD()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  NotMD::~NotMD()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string NotMD::name() const { return "NotMD";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int NotMD::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string NotMD::category() const { return "General";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void NotMD::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void NotMD::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void NotMD::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Mantid
} // namespace MDAlgorithms