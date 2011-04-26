#include "MantidAlgorithms/CreatePeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CreatePeaksWorkspace)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CreatePeaksWorkspace::CreatePeaksWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CreatePeaksWorkspace::~CreatePeaksWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void CreatePeaksWorkspace::initDocs()
  {
    this->setWikiSummary("Create an empty PeaksWorkspace.");
    this->setOptionalMessage("Create an empty PeaksWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CreatePeaksWorkspace::init()
  {
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InstrumentWorkspace","",Direction::Input, true), "An optional input workspace containing the default instrument for peaks in this workspace.");
    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CreatePeaksWorkspace::exec()
  {
    MatrixWorkspace_sptr instWS = getProperty("InstrumentWorkspace");

    // TODO Auto-generated execute stub
    PeaksWorkspace_sptr out(new PeaksWorkspace());
    setProperty("OutputWorkspace", out);

    if (instWS)
    {
      out->setInstrument(instWS->getInstrument());
      // Create a dumb default peak
      out->addPeak( Peak(out->getInstrument(), out->getInstrument()->getDetectorIDs(true)[0], 1.0) );
      out->getPeaks()[0].setH( 1.23 );
      out->getPeaks()[0].setK( 2.34 );
    }
  }



} // namespace Mantid
} // namespace Algorithms

