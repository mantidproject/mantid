/*  Takes a 2D workspace as input and sums each Histogram1D contained within
    it, storing the result as a Workspace1D.
    
    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>
       
    Optional Properties (assume that you count from zero):
    <UL>
    <LI> StartX - X bin number to integrate from (default 0)</LI>
    <LI> EndX - X bin number to integrate to (inclusive, default max)</LI>
    <LI> StartY - Y bin number to integrate from (default 0)</LI>
    <LI> EndY - Y bin number to integrate to (inclusive, default max)</LI>
    </UL>
    
    @author Russell Taylor, Tessella Support Services plc
    @date 05/10/2007
    
    Copyright &copy; 2007 ???RAL???

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "../inc/SimpleIntegration.h"
#include "../../DataObjects/inc/Workspace2D.h"
#include "../../DataObjects/inc/Workspace1D.h"

#include <sstream>
#include <numeric>
#include <math.h>

namespace Mantid
{

StatusCode SimpleIntegration::init()
{
  // Try and retrieve the optional properties
  // Since a property can only be a string at present, need to convert to int
  // No type checking - this code MUST be temporary
  std::string strInput;
  StatusCode status = getProperty("StartX", strInput);
  if (status.isFailure() ) {
    m_MinX = 0;
  } else {
    std::istringstream iss (strInput,std::istringstream::in);
    iss >> m_MinX;
  }
  status = getProperty("EndX", strInput);
  if (status.isFailure() ) {
    m_MaxX = 0;
  } else {
    std::istringstream iss (strInput,std::istringstream::in);
    iss >> m_MaxX;
  }
  status = getProperty("StartY", strInput);
  if (status.isFailure() ) {
    m_MinY = 0;
  } else {
    std::istringstream iss (strInput,std::istringstream::in);
    iss >> m_MinY;
  }
  status = getProperty("EndY", strInput);
  if (status.isFailure() ) {
    m_MaxY = 0;
  } else {
    std::istringstream iss (strInput,std::istringstream::in);
    iss >> m_MaxY;
  }
  
  return StatusCode::SUCCESS;
}

StatusCode SimpleIntegration::exec()
{
  MsgStream log(0,"");
  
  const Workspace2D *localworkspace = dynamic_cast<Workspace2D*>(m_inputWorkspace);
  const int numberOfYBins = localworkspace->getHistogramNumber();
  // Check 'StartX' is in range 0-numberOfSpectra
  if ( 0 > m_MinY > numberOfYBins)
  {
    log << "StartY out of range! Set to 0." << endreq;
    m_MinY = 0;
  }
  
  if ( !m_MaxY ) m_MaxY = numberOfYBins;
  if ( m_MaxY > numberOfYBins || m_MaxY < m_MinY ) 
  {
    log << "EndY out of range! Set to max detector number" << endreq;
    m_MaxY = numberOfYBins;
  }
  
  // Create vectors to hold result
  std::vector<double> detectorNumber;
  std::vector<double> sums;
  std::vector<double> errors;
  // Loop over spectra
  for (int i = m_MinY; i < m_MaxY; ++i) {
    
    // Retrieve the spectrum into a vector
    const std::vector<double>& YValues = localworkspace->getY(i);
    const std::vector<double>& YErrors = localworkspace->getE(i);

    // First time through the loop, do some checking on StartY & EndY
    if ( i == m_MinY )
    {
      const int numberOfXBins = YValues.size();
      if ( 0 > m_MinX > numberOfXBins)
      {
        log << "StartX out of range! Set to 0" << endreq;
        m_MinX = 0;
      }
      if ( !m_MaxX ) m_MaxX = numberOfXBins;
      if ( m_MaxX > numberOfXBins || m_MaxX < m_MinX)
      {
        log << "EndX out of range! Set to max number" << endreq;
        m_MaxX = numberOfXBins;
      }
    }
    
    // Sum up the required elements of the vector
    double YSum = std::accumulate(YValues.begin()+m_MinX,YValues.begin()+m_MaxX,0.0);
    // Error propagation - sqrt(sum of squared elements)
    double YError = sqrt(std::inner_product(YErrors.begin()+m_MinX,YErrors.begin()+m_MaxX,
                                        YErrors.begin()+m_MinX,0.0));
    
    // Add the results to the vectors for the new workspace
    // Warning: Counting detector number from 0
    detectorNumber.push_back(i);
    sums.push_back(YSum);    
    errors.push_back(YError);
  }
  
  // Create the 1D workspace for the output
  WorkspaceFactory *factory = WorkspaceFactory::Instance();
  m_outputWorkspace = factory->createWorkspace("Workspace1D");
  Workspace1D *localWorkspace = dynamic_cast<Workspace1D*>(m_outputWorkspace);

  // Populate the 1D workspace
  localWorkspace->setX(detectorNumber);
  localWorkspace->setData(sums, errors);
  
  return StatusCode::SUCCESS;
}

StatusCode SimpleIntegration::final()
{
  return StatusCode::SUCCESS;
}

}
