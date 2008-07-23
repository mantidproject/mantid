//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/GroupDetectors.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include <set>
#include <numeric>

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(GroupDetectors)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_sptr;

// Initialise the logger
Kernel::Logger& GroupDetectors::g_log = Kernel::Logger::get("GroupDetectors");

/// (Empty) Constructor
GroupDetectors::GroupDetectors() {}

/// Destructor
GroupDetectors::~GroupDetectors() {}

void GroupDetectors::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("Workspace","", Direction::InOut));
  declareProperty(new ArrayProperty<int>("WorkspaceIndexList"));
  declareProperty(new ArrayProperty<int>("SpectraList"));
  declareProperty(new ArrayProperty<int>("DetectorList"));
  declareProperty("ResultIndex",-1);
}

void GroupDetectors::exec()
{
  // Get the input workspace
  const Workspace2D_sptr WS = getProperty("Workspace");

  Property *wil = getProperty("WorkspaceIndexList");
  Property *sl = getProperty("SpectraList");
  Property *dl = getProperty("DetectorList");

  // Could create a Validator to replace the below
  if ( wil->isDefault() && sl->isDefault() && dl->isDefault() )
  {
    g_log.error("WorkspaceIndexList, SpectraList, and DetectorList properties are empty");
    throw std::invalid_argument("WorkspaceIndexList, SpectraList, and DetectorList properties are empty");
  }
  
  // Bin boundaries need to be the same so, check if they actually are
//  if ( WS->getAxis(0)->unit()->unitID().compare("TOF") )
  if ( !hasSameBoundaries(WS) )
  {
    g_log.error("Can only group if the workspace unit is time-of-flight");
    throw std::runtime_error("Can only group if the workspace unit is time-of-flight");
  }//*/
  // If passes the above check, assume X bin boundaries are the same
  
  // I'm also going to insist for the time being that Y just contains raw counts 
  if ( WS->isDistribution() )
  {
    g_log.error("GroupDetectors at present only works if spectra contain raw counts");
    throw Exception::NotImplementedError("GroupDetectors at present only works if spectra contain raw counts");    
  }
  /// @todo Get this algorithm working on a more generic input workspace so the restrictions above can be lost
  
  std::vector<int> indexList = getProperty("WorkspaceIndexList");
  // Get hold of the axis that holds the spectrum numbers
  Axis *spectraAxis = WS->getAxis(1);

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
    boost::shared_ptr<API::SpectraDetectorMap> spectraMap = WS->getSpectraMap();
    std::vector<int> mySpectraList = spectraMap->getSpectra(detectorList);
    //then from spectra numbers to indices
    fillIndexListFromSpectra(indexList,mySpectraList,WS);
  }

  if ( indexList.size() == 0 )
  {
      g_log.warning("Nothing to group");
      return;
  }
  
  
  const int vectorSize = WS->blocksize();
  const int firstIndex = indexList[0];
  const int firstSpectrum = spectraAxis->spectraNo(firstIndex);
  setProperty("ResultIndex",firstIndex);
  for (unsigned int i = 0; i < indexList.size()-1; ++i)
  {
    const int currentIndex = indexList[i+1];
    // Move the current detector to belong to the first spectrum
    WS->getSpectraMap()->remap(spectraAxis->spectraNo(currentIndex),firstSpectrum);
    // Add up all the Y spectra and store the result in the first one
    std::transform(WS->dataY(firstIndex).begin(), WS->dataY(firstIndex).end(), WS->dataY(currentIndex).begin(),
                   WS->dataY(firstIndex).begin(), std::plus<double>());
    // Now zero the now redundant spectrum and set its spectraNo to indicate this (using -1)
    // N.B. Deleting spectra would cause issues for ManagedWorkspace2D, hence the the approach taken here
    WS->dataY(currentIndex).assign(vectorSize,0.0);
    WS->dataE(currentIndex).assign(vectorSize,0.0);
    spectraAxis->spectraNo(currentIndex) = -1;
  }
  // Deal with the errors (assuming Gaussian)
  /// @todo Deal with Poisson errors
  std::transform(WS->dataY(firstIndex).begin(), WS->dataY(firstIndex).end(), WS->dataE(firstIndex).begin(), dblSqrt);

}

double GroupDetectors::dblSqrt(double in)
{
  return sqrt(in);
}

/// Checks if all histograms have the same boundaries by comparing their sums
bool GroupDetectors::hasSameBoundaries(const Workspace2D_sptr WS)
{

    if (!WS->blocksize()) return true;
    double commonSum = std::accumulate(WS->dataX(0).begin(),WS->dataX(0).end(),0.);
    for (int i = 1; i < WS->getNumberHistograms(); ++i)
        if ( commonSum != std::accumulate(WS->dataX(i).begin(),WS->dataX(i).end(),0.) )
            return false;
    return true;
}

void GroupDetectors::fillIndexListFromSpectra(std::vector<int>& indexList, std::vector<int>& spectraList, 
    const Workspace2D_sptr WS)
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

