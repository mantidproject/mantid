#include "MantidMDEvents/MakeDiffractionMDEventWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MakeDiffractionMDEventWorkspace)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MakeDiffractionMDEventWorkspace::MakeDiffractionMDEventWorkspace()
  {
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MakeDiffractionMDEventWorkspace::~MakeDiffractionMDEventWorkspace()
  {
    // TODO Auto-generated destructor stub
  }
  

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MakeDiffractionMDEventWorkspace::init()
  {
    setOptionalMessage("Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz)\n"
        "from an input EventWorkspace."
        );
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MakeDiffractionMDEventWorkspace::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Mantid
} // namespace MDEvents

