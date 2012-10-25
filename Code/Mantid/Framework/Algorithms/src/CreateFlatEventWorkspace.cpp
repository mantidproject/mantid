/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAlgorithms/CreateFlatEventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CreateFlatEventWorkspace)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CreateFlatEventWorkspace::CreateFlatEventWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CreateFlatEventWorkspace::~CreateFlatEventWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string CreateFlatEventWorkspace::name() const { return "CreateFlatEventWorkspace";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int CreateFlatEventWorkspace::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string CreateFlatEventWorkspace::category() const { return "CorrectionFunctions\\BackgroundCorrections";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void CreateFlatEventWorkspace::initDocs()
  {
    this->setWikiSummary("Creates a flat event workspace that can be used for background removal.");
    this->setOptionalMessage("Creates a flat event workspace that can be used for background removal.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CreateFlatEventWorkspace::init()
  {
    declareProperty(new Mantid::API::WorkspaceProperty<>("InputWorkspace","",Mantid::Kernel::Direction::Input), "An input workspace to use as a source for the events.");
    declareProperty(new Mantid::API::WorkspaceProperty<>("OutputWorkspace","",Mantid::Kernel::Direction::Output), "Output event workspace containing a flat background.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CreateFlatEventWorkspace::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Algorithms
} // namespace Mantid
