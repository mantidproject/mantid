/*WIKI*
Determine if a workspace has a UB matrix on any of it's samples. Returns True if one is found. Returns false if none can be found, or if the
workspace type is incompatible.
*WIKI*/

#include "MantidCrystal/HasUB.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(HasUB)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  HasUB::HasUB()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  HasUB::~HasUB()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string HasUB::name() const { return "HasUB";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int HasUB::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string HasUB::category() const { return "crystal";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void HasUB::initDocs()
  {
    this->setWikiSummary("Determines whether the workspace has one or more UB Matrix.");
    this->setOptionalMessage("Determines whether the workspace has one or more UB Matrix");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void HasUB::init()
  {
    declareProperty(new WorkspaceProperty<Workspace>("Workspace", "", Direction::Input),
              "Workspace to clear the UB from.");
    declareProperty(new PropertyWithValue<bool>("HasUB", "", Direction::Output),
              "Indicates action performed, or predicted to perform if DryRun.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void HasUB::exec()
  {
    Workspace_sptr ws =  getProperty("Workspace");
    bool hasUB = ClearUB::doExecute(ws.get(), true /*DryRun*/);
    setProperty("HasUB", hasUB);
  }


} // namespace Crystal
} // namespace Mantid
