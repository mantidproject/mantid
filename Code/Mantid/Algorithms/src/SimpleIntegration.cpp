//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "../inc/SimpleIntegration.h"
#include "../../DataObjects/inc/Workspace2D.h"
#include "../../DataObjects/inc/Workspace1D.h"

namespace Mantid
{

SimpleIntegration::SimpleIntegration()
{
}

SimpleIntegration::~SimpleIntegration()
{
}

StatusCode SimpleIntegration::init()
{
  return StatusCode::SUCCESS;
}

StatusCode SimpleIntegration::exec()
{
  const Workspace2D *localworkspace = dynamic_cast<Workspace2D*>(m_inputWorkspace);
  int numberOfSpectra = localworkspace->getHistogramNumber();
  
  // Loop over spectra
  std::vector<double> detectorNumber;
  std::vector<double> sums;
  for (int i = 0; i < numberOfSpectra; ++i) {
    
    std::vector<double> spectrumValues = localworkspace->getY(i);
    double spectrumSum = 0.0;
    
    // Loop over and sum the contents of an individual spectrum
    // Note this sum includes the overflow bin at the end of a spectrum
    std::vector<double>::const_iterator iter;
    for (iter = spectrumValues.begin(); iter != spectrumValues.end(); ++iter)
    {
      spectrumSum += *iter;
    }
    
    // Add the results to the vectors for the new workspace
    // Count detector number from 1
    detectorNumber.push_back(i+1);
    sums.push_back(spectrumSum);
  }
  
  // Create the 1D workspace for the output
  WorkspaceFactory *factory = WorkspaceFactory::Instance();
  m_outputWorkspace = factory->createWorkspace("Workspace1D");
  Workspace1D *localWorkspace = dynamic_cast<Workspace1D*>(m_outputWorkspace);

  // Populate the 1D workspace
  localWorkspace->setX(detectorNumber);
  localWorkspace->setData(sums);
  
  return StatusCode::SUCCESS;
}

StatusCode SimpleIntegration::final()
{
  return StatusCode::SUCCESS;
}

}
