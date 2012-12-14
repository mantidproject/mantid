/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAlgorithms/RebinRagged.h"
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace Algorithms
{
  using API::WorkspaceProperty;
  using Kernel::Direction;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(RebinRagged)
  


  //----------------------------------------------------------------------------------------------
  /// Constructor
  RebinRagged::RebinRagged()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /// Destructor
  RebinRagged::~RebinRagged()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string RebinRagged::name() const
  {
    return "RebinRagged";
  }
  
  /// Algorithm's version for identification. @see Algorithm::version
  int RebinRagged::version() const
  {
    return 1;
  }
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string RebinRagged::category() const
  {
    return "Transforms\\Rebin";
  }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void RebinRagged::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void RebinRagged::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");

    declareProperty("PreserveEvents", true, "Keep the output workspace as an EventWorkspace, if the input has events (default).\n"
        "If the input and output EventWorkspace names are the same, only the X bins are set, which is very quick.\n"
        "If false, then the workspace gets converted to a Workspace2D histogram.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void RebinRagged::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Algorithms
} // namespace Mantid
