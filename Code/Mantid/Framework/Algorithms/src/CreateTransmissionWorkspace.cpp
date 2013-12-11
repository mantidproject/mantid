/*WIKI*
Creates a transmission run workspace given one or more TOF workspaces and the original run Workspace. If two workspaces are provided, then
the workspaces are stitched together using [[Stitch1D]]. InputWorkspaces must be in TOF. A single output workspace is generated with x-units of Wavlength in angstroms.
*WIKI*/

#include "MantidAlgorithms/CreateTransmissionWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CreateTransmissionWorkspace)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CreateTransmissionWorkspace::CreateTransmissionWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CreateTransmissionWorkspace::~CreateTransmissionWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string CreateTransmissionWorkspace::name() const { return "CreateTransmissionWorkspace";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int CreateTransmissionWorkspace::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string CreateTransmissionWorkspace::category() const { return "Reflectometry\\ISIS"; }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void CreateTransmissionWorkspace::initDocs()
  {
    this->setWikiSummary("Creates a transmission run workspace in Wavelength from input TOF workspaces.");
    this->setOptionalMessage(this->getWikiSummary());
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CreateTransmissionWorkspace::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CreateTransmissionWorkspace::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Algorithms
} // namespace Mantid
