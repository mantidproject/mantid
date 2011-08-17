#include "MantidMDEvents/MergeMDEW.h"
#include "MantidKernel/System.h"
#include "MantidAPI/MultipleFileProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MergeMDEW)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MergeMDEW::MergeMDEW()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MergeMDEW::~MergeMDEW()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MergeMDEW::initDocs()
  {
    this->setWikiSummary("Merge multiple MDEventWorkspaces from files that obey a common box format.");
    this->setOptionalMessage("Merge multiple MDEventWorkspaces from files that obey a common box format.");
    this->setWikiDescription(""
        "This algorithm is meant to merge a large number of large MDEventWorkspaces together into one "
        "file-backed MDEventWorkspace, without exceeding available memory."
        "\n\n"
        "First, you will need to generate a MDEventWorkspaces NXS file for each run with a fixed box structure:"
        "\n\n"
        "* This would be a MaxDepth=1 structure but with finer boxes, maybe 50x50x50.\n"
        "* This can be done immediately after acquiring each run so that less processing has to be done at once.\n"
        "\n\n"
        "Then, enter the path to all of the files created previously. The algorithm avoids excessive memory use by only keeping "
        "the events from ONE box from ALL the files in memory at once to further process and refine it.\n"
        "This is why it requires a common box structure.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MergeMDEW::init()
  {
    declareProperty(new MultipleFileProperty("Filenames"),
        "Select several MDEventWorkspace NXS files to merge together. Files must have common box structure.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MergeMDEW::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Mantid
} // namespace MDEvents

