/**TODO: FOR DEPRICATION */ 
#include "MantidMDEvents/MDWSDescriptionDepricated.h"
#include "MantidAPI/NumericAxis.h"

namespace Mantid
{
namespace MDEvents
{

/** method sets up the pointer to the class which contains detectors parameters
* @param -- DetLoc the class which contaits the preprocessed detectors parameters
*
* Throws if the parameters were not defined
*/
void MDWSDescriptionDepricated::setDetectors(const ConvToMDPreprocDet &DetLoc)
{
  m_DetLoc = &DetLoc;
  if(m_DetLoc->nDetectors()==0) throw(std::invalid_argument( " Preprocessed detectors positions are either empty or undefined. Nothing to do"));

}
/** the method builds the MD ws description from existing matrix workspace and the requested transformation parameters.
*@param pWS -- input matrix workspace to be converted into MD workpsace
*@param QMode -- momentum conversion mode. Any mode supported by Q conversion factory. Class just carries up the name of Q-mode,
* to the place where factory call to the solver is made , so no code modification is needed when new modes are added
* to the factory
*@param dEMode -- energy analysis mode (string representation). Should correspond to energy analysis modes, supported by selected Q-mode
*@param dimPropertyNames -- the vector of names for additional ws properties, which will be used as dimensions.

*/
void MDWSDescriptionDepricated::buildFromMatrixWS(const API::MatrixWorkspace_const_sptr &pWS,const std::string &QMode,const std::string dEMode,
  const std::vector<std::string> &dimProperyNames)
{
  MDWSDescription::buildFromMatrixWS(pWS,QMode,dEMode,dimProperyNames);
  // to be on a safe side, the preprocessed detectrod position should be reset at this point as we can initiate old WSDescr class with absolutely new ws;
  m_DetLoc = NULL;

}


/** the function builds MD event WS description from existing workspace.
* Primary used to obtain existing ws parameters
*@param pWS -- shared pointer to existing MD workspace
*/
void MDWSDescriptionDepricated::buildFromMDWS(const API::IMDEventWorkspace_const_sptr &pWS)
{
  MDWSDescription::buildFromMDWS(pWS);
  // to be on a safe side, the preprocessed detectrod position should be reset at this point as we can initiate old WSDescr class with absolutely new ws;
  m_DetLoc = NULL;

}

/// empty constructor
MDWSDescriptionDepricated::MDWSDescriptionDepricated(unsigned int nDimensions):
MDWSDescription(nDimensions),
  m_DetLoc(NULL)
{
}
 /// function checks if source workspace still has information about detectors. Some ws (like rebinned one) do not have this information any more.
bool MDWSDescriptionDepricated::isDetInfoLost(Mantid::API::MatrixWorkspace_const_sptr inWS2D)
{
  API::NumericAxis *pYAxis = dynamic_cast<API::NumericAxis *>(inWS2D->getAxis(1));
     // if this is numeric axis, then the detector's information has been lost:
  if(pYAxis) return true;
  return false;
}

/** Helper function to obtain the energy of incident neutrons from the input workspaec
*
*@param pHost the pointer to the algorithm to work with
*
*@returns the incident energy of the neutrons or quet NaN if can not retrieve one 
*/
double MDWSDescriptionDepricated::getEi(API::MatrixWorkspace_const_sptr inWS2D)
{
  if(!inWS2D)throw(std::invalid_argument(" getEi: invoked on empty input workspace "));

  Kernel::PropertyWithValue<double>  *pProp(NULL);
  try
  {
    pProp  =dynamic_cast<Kernel::PropertyWithValue<double>  *>(inWS2D->run().getProperty("Ei"));
  }
  catch(...)
  {}
  // this can be preferable name for indirect conversion
  try
  {
    pProp  =dynamic_cast<Kernel::PropertyWithValue<double>  *>(inWS2D->run().getProperty("eFixed"));
  }
  catch(...)
  {}


  if(!pProp) return std::numeric_limits<double>::quiet_NaN();

  return (*pProp); 
}
} //end namespace MDEvents
} //end namespace Mantid