#include "MantidMDEvents/MDEWFindPeaks.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MDEWFindPeaks)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MDEWFindPeaks::MDEWFindPeaks()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MDEWFindPeaks::~MDEWFindPeaks()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MDEWFindPeaks::initDocs()
  {
    this->setWikiSummary("Find peaks in reciprocal space in a MDEventWorkspace.");
    this->setOptionalMessage("Find peaks in reciprocal space in a MDEventWorkspace.");
    this->setWikiDescription(""
        ""
        );
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MDEWFindPeaks::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MDEWFindPeaks::exec()
  {
  }



} // namespace Mantid
} // namespace MDEvents

