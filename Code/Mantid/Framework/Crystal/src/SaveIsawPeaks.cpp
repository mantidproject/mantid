#include "MantidCrystal/SaveIsawPeaks.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveIsawPeaks)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveIsawPeaks::SaveIsawPeaks()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveIsawPeaks::~SaveIsawPeaks()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SaveIsawPeaks::initDocs()
  {
    this->setWikiSummary("Save a PeaksWorkspace to a ISAW-style ASCII .peaks file.");
    this->setOptionalMessage("Save a PeaksWorkspace to a ISAW-style ASCII .peaks file.");
    this->setWikiDescription("Save a PeaksWorkspace to a ISAW-style ASCII .peaks file.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveIsawPeaks::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveIsawPeaks::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Mantid
} // namespace Crystal

