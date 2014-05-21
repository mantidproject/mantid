/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidDataHandling/CreateChunkingFromInstrument.h"

namespace Mantid
{
namespace DataHandling
{

  using Mantid::Kernel::Direction;
  using Mantid::API::WorkspaceProperty;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CreateChunkingFromInstrument)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CreateChunkingFromInstrument::CreateChunkingFromInstrument()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CreateChunkingFromInstrument::~CreateChunkingFromInstrument()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string CreateChunkingFromInstrument::name() const { return "CreateChunkingFromInstrument";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int CreateChunkingFromInstrument::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string CreateChunkingFromInstrument::category() const { return "Workflow\\DataHandling";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void CreateChunkingFromInstrument::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CreateChunkingFromInstrument::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CreateChunkingFromInstrument::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace DataHandling
} // namespace Mantid
