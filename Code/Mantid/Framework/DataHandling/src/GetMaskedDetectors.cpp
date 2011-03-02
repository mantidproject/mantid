//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/GetMaskedDetectors.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/ArrayProperty.h"
#include <map>

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(GetMaskedDetectors)

/// Sets documentation strings for this algorithm
void GetMaskedDetectors::initDocs()
{
  this->setWikiSummary("This algorithm returns a std::vector<int> containing the detector ID's of detectors that have been masked with [[MaskDetectors]] or it's like. ");
  this->setOptionalMessage("This algorithm returns a std::vector<int> containing the detector ID's of detectors that have been masked with MaskDetectors or it's like.");
}


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
    "A comma separated list or array containing a list of masked detector ID's" );
}

void GetMaskedDetectors::exec()
{
  // Get the input workspace
  const MatrixWorkspace_sptr WS = getProperty("InputWorkspace");

  // List masked of detector IDs
  std::vector<int> detectorList;

  std::map<int, Geometry::IDetector_sptr> det_map = WS->getInstrument()->getDetectors();

  for (std::map<int, Geometry::IDetector_sptr>::const_iterator iter = det_map.begin();
      iter != det_map.end(); ++iter )
  {
    if ( iter->second->isMasked() )
    {
      detectorList.push_back(iter->first);
     }
  }

  setProperty("DetectorList", detectorList);
}



} // namespace DataHandling
} // namespace Mantid
