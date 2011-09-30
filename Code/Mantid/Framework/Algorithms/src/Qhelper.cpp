//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Qhelper.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Histogram1D.h"

namespace Mantid
{
namespace Algorithms
{


using namespace Kernel;
using namespace API;
using namespace Geometry;


/** Checks if workspaces input to Q1D or Qxy are reasonable
  @param dataWS data workspace
  @param binWS (WavelengthAdj) workpace that will be checked to see if it has one spectrum and the same number of bins as dataWS
  @param detectWS (PixelAdj) passing NULL for this wont raise an error, if set it will be checked this workspace has as many histograms as dataWS each with one bin
  @throw invalid_argument if the workspaces are not mututially compatible
*/
void Qhelper::examineInput(API::MatrixWorkspace_const_sptr dataWS, 
     API::MatrixWorkspace_const_sptr binAdj, API::MatrixWorkspace_const_sptr detectAdj)
{
  if ( dataWS->getNumberHistograms() < 1 )
  if ( dataWS->getNumberHistograms() < 1 )
  {
    throw std::invalid_argument("Empty data workspace passed, can not continue");
  }

  //it is not an error for these workspaces not to exist
  if (binAdj)
  {
    if ( binAdj->getNumberHistograms() != 1 )
    {
      throw std::invalid_argument("The WavelengthAdj workspace must have one spectrum");
    }
    if ( binAdj->readY(0).size() != dataWS->readY(0).size() )
    {
      throw std::invalid_argument("The WavelengthAdj workspace's bins must match those of the detector bank workspace");
    }
    MantidVec::const_iterator reqX = dataWS->readX(0).begin();
    MantidVec::const_iterator testX = binAdj->readX(0).begin();
    for ( ; reqX != dataWS->readX(0).end(); ++reqX, ++testX)
    {
      if ( *reqX != *testX )
      {
        throw std::invalid_argument("The WavelengthAdj workspace must have matching bins with the detector bank workspace");
      }
    }
    if ( binAdj->isDistribution() != dataWS->isDistribution() )
    {
      throw std::invalid_argument("The distrbution/raw counts status of the wavelengthAdj and DetBankWorkspace must be the same, use ConvertToDistribution");
    }
  }
  else if( ! dataWS->isDistribution() )
  {
    throw std::invalid_argument("The data workspace must be a distrbution if there is no Wavelength dependent adjustment");
  }
  
  if (detectAdj)
  {
    if ( detectAdj->blocksize() != 1 )
    {
      throw std::invalid_argument("The PixelAdj workspace must point to a workspace with single bin spectra, as only the first bin is used");
    }
    if ( detectAdj->getNumberHistograms() != dataWS->getNumberHistograms() )
    {
      throw std::invalid_argument("The PixelAdj workspace must have one spectrum for each spectrum in the detector bank workspace");
    }
  }
}



} // namespace Algorithms
} // namespace Mantid

