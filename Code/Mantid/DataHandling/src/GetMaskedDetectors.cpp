//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/GetMaskedDetectors.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/ArrayProperty.h"
#include <set>

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(GetMaskedDetectors)

using namespace Kernel;
using namespace API;

/// (Empty) Constructor
GetMaskedDetectors::GetMaskedDetectors() {}

/// Destructor
GetMaskedDetectors::~GetMaskedDetectors() {}

void GetMaskedDetectors::init()
{
  declareProperty(
    new WorkspaceProperty<>("InputWorkspace","", Direction::Input),
    "The name of the workspace that will be used as input for the algorithm" );
  declareProperty(new ArrayProperty<int>("DetectorList", new NullValidator<std::vector<int> >, Direction::Output),
    "A coma separated list or array containing a list of masked detector ID's" );
}

void GetMaskedDetectors::exec()
{
  // Get the input workspace
  const MatrixWorkspace_sptr WS = getProperty("InputWorkspace");

  // List masked of detector IDs
  std::vector<int> detectorList;

  // Loop through every spectrum and get the list of masked detector IDs
  for ( int i=0; i<WS->getNumberHistograms(); i++ )
  {
    Geometry::IDetector_const_sptr det = WS->getDetector(i);
    if ( det->isMasked() )
    {
      detectorList.push_back(i);
    }
  }

  setProperty("DetectorList", detectorList);
}



} // namespace DataHandling
} // namespace Mantid
