//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/MarkDeadDetectors.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include <set>

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(MarkDeadDetectors)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_sptr;

// Initialise the logger
Kernel::Logger& MarkDeadDetectors::g_log = Kernel::Logger::get("MarkDeadDetectors");

/// (Empty) Constructor
MarkDeadDetectors::MarkDeadDetectors() {}

/// Destructor
MarkDeadDetectors::~MarkDeadDetectors() {}

void MarkDeadDetectors::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("Workspace","", Direction::InOut));
  declareProperty(new ArrayProperty<int>("SpectraList"));
  declareProperty(new ArrayProperty<int>("DetectorList"));
  declareProperty(new ArrayProperty<int>("WorkspaceIndexList"));
}

void MarkDeadDetectors::exec()
{
  // Get the input workspace
  const Workspace2D_sptr WS = getProperty("Workspace");
  // Get the size of the vectors
  const int vectorSize = WS->blocksize();
  // Get hold of the axis that holds the spectrum numbers
  Axis *spectraAxis = WS->getAxis(1);

  Property *wil = getProperty("WorkspaceIndexList");
  Property *sl = getProperty("SpectraList");
  Property *dl = getProperty("DetectorList");

  // Could create a Validator to replace the below
  if ( wil->isDefault() && sl->isDefault() && dl->isDefault() )
  {
    g_log.error("WorkspaceIndexList, SpectraList, and DetectorList properties are empty");
    throw std::invalid_argument("WorkspaceIndexList, SpectraList, and DetectorList properties are empty");
  }

  std::vector<int> indexList = getProperty("WorkspaceIndexList");

  // If the spectraList property has been set, need to loop over the workspace looking for the
  // appropriate spectra number and adding the indices they are linked to the list to be processed
  if ( ! sl->isDefault() )
  {
    std::vector<int> spectraList = getProperty("SpectraList");
    fillIndexListFromSpectra(indexList,spectraList,WS);
  }// End dealing with spectraList
  else if ( ! dl->isDefault() )
  {// Dealing with DetectorList
    const std::vector<int> detectorList = getProperty("DetectorList");
    //convert from detectors to spectra numbers
    SpectraMap_const_sptr spectraMap = WS->getSpectraMap();
    std::vector<int> mySpectraList = spectraMap->getSpectra(detectorList);
    //then from spectra numbers to indices
    fillIndexListFromSpectra(indexList,mySpectraList,WS);
  }

  if ( indexList.size() == 0 )
  {
      g_log.warning("Nothing to do");
      return;
  }

  std::vector<int>::const_iterator it;
  for (it = indexList.begin(); it != indexList.end(); ++it)
  {
    // Mark associated detector as dead
    WS->getSpectraMap()->getDetector(spectraAxis->spectraNo(*it))->markDead();

    // Zero the workspace spectra (data and errors, not X values)
    WS->dataY(*it).assign(vectorSize,0.0);
    WS->dataE(*it).assign(vectorSize,0.0);
  }

}

/// Convert a list of spectra numbers into the corresponding workspace indices
void MarkDeadDetectors::fillIndexListFromSpectra(std::vector<int>& indexList, std::vector<int>& spectraList,
                                              const DataObjects::Workspace2D_sptr WS)
{
  // Convert the vector of properties into a set for easy searching
  std::set<int> spectraSet(spectraList.begin(),spectraList.end());
  // Next line means that anything in Clear the index list first
  indexList.clear();
  //get the spectra axis
  Axis *spectraAxis = WS->getAxis(1);

  for (int i = 0; i < WS->getNumberHistograms(); ++i)
  {
    int currentSpec = spectraAxis->spectraNo(i);
    if ( spectraSet.find(currentSpec) != spectraSet.end() )
    {
      indexList.push_back(i);
    }
  }
}

} // namespace DataHandling
} // namespace Mantid
