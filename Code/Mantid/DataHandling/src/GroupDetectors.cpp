//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/GroupDetectors.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include <set>

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
}

void GroupDetectors::exec()
{
  // Get the input workspace
  const Workspace2D_sptr WS = getProperty("Workspace");

  Property *wil = getProperty("WorkspaceIndexList");
  Property *sl = getProperty("SpectraList");
  // Could create a Validator to replace the below
  if ( wil->isDefault() && sl->isDefault() )
  {
    g_log.error("WorkspaceIndexList & SpectraList properties are both empty");
    throw std::invalid_argument("WorkspaceIndexList & SpectraList properties are both empty");
  }
  
  // Bin boundaries need to be the same so, for now, only allow this action if the workspace unit is TOF
  // Later, could include conversion into TOF and back again after combination (or some other solution)
  if ( WS->XUnit()->unitID().compare("TOF") )
  {
    g_log.error("Can only group if the workspace unit is time-of-flight");
    throw std::runtime_error("Can only group if the workspace unit is time-of-flight");
  }
  // If passes the above check, assume X bin boundaries are the same
  
  // I'm also going to insist for the time being that Y just contains raw counts 
  if ( WS->isDistribution() )
  {
    g_log.error("GroupDetectors at present only works if spectra contain raw counts");
    throw Exception::NotImplementedError("GroupDetectors at present only works if spectra contain raw counts");    
  }
  /// @todo Get this algorithm working on a more generic input workspace so the restrictions above can be lost
  
  std::vector<int> indexList = getProperty("WorkspaceIndexList");

  // If the spectraList property has been set, need to loop over the workspace looking for the
  // appropriate spectra number and adding the indices they are linked to the list to be processed
  if ( ! sl->isDefault() )
  {
    const std::vector<int> spectraList = getProperty("SpectraList");
    // Convert the vector of properties into a set for easy searching
    std::set<int> spectraSet(spectraList.begin(),spectraList.end());
    // Next line means that anything in WorkspaceIndexList is ignored if SpectraList isn't empty
    indexList.clear();
    
    for (int i = 0; i < WS->getHistogramNumber(); ++i)
    {
      int currentSpec = WS->spectraNo(i);
      if ( spectraSet.find(currentSpec) != spectraSet.end() )
      {
        indexList.push_back(i);
      }
    }  
  }
  // End dealing with spectraList
  
  const int vectorSize = WS->blocksize();
  const int firstIndex = indexList[0];
  const int firstSpectrum = WS->spectraNo(firstIndex);
  for (unsigned int i = 0; i < indexList.size()-1; ++i)
  {
    const int currentIndex = indexList[i+1];
    // Move the current detector to belong to the first spectrum
    WS->getSpectraMap()->remap(WS->spectraNo(currentIndex),firstSpectrum);
    // Add up all the Y spectra and store the result in the first one
    std::transform(WS->dataY(firstIndex).begin(), WS->dataY(firstIndex).end(), WS->dataY(currentIndex).begin(),
                   WS->dataY(firstIndex).begin(), std::plus<double>());
    // Now zero the now redundant spectrum and set its spectraNo to indicate this (using -1)
    // N.B. Deleting spectra would cause issues for ManagedWorkspace2D, hence the the approach taken here
    WS->dataY(currentIndex).assign(vectorSize,0.0);
    WS->dataE(currentIndex).assign(vectorSize,0.0);
    WS->dataE2(currentIndex).assign(vectorSize,0.0);
    WS->spectraNo(currentIndex) = -1;
  }
  // Deal with the errors (assuming Gaussian)
  /// @todo Deal with Poisson errors (E2)
  std::transform(WS->dataY(firstIndex).begin(), WS->dataY(firstIndex).end(), WS->dataE(firstIndex).begin(), dblSqrt);

}

double GroupDetectors::dblSqrt(double in)
{
  return sqrt(in);
}

} // namespace DataHandling
} // namespace Mantid
