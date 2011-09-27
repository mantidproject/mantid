#include "MantidMDEvents/SliceMD.h"
#include "MantidKernel/System.h"
#include "MantidAPI/IMDEventWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SliceMD)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SliceMD::SliceMD()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SliceMD::~SliceMD()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SliceMD::initDocs()
  {
    this->setWikiSummary("Make a MDEventWorkspace containing the events in a slice of an input MDEventWorkspace.");
    this->setOptionalMessage("Make a MDEventWorkspace containing the events in a slice of an input MDEventWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SliceMD::init()
  {
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::Input), "An input MDEventWorkspace.");
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output), "An output MDEventWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SliceMD::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Mantid
} // namespace MDEvents

