/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidDataHandling/LoadPDCharacterizations.h"

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadPDCharacterizations)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadPDCharacterizations::LoadPDCharacterizations()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadPDCharacterizations::~LoadPDCharacterizations()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string LoadPDCharacterizations::name() const { return "LoadPDCharacterizations";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int LoadPDCharacterizations::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string LoadPDCharacterizations::category() const { return TODO: FILL IN A CATEGORY;}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadPDCharacterizations::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadPDCharacterizations::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadPDCharacterizations::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace DataHandling
} // namespace Mantid