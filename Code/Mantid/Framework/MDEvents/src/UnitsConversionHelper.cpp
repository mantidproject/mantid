#include "MantidMDEvents/UnitsConversionHelper.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Strings.h"

namespace Mantid
{
namespace MDEvents
{

/** establish and initialize proper units conversion from input to output units
@param UnitsFrom -- the ID of the units, which have to be converted from
@param UnitsTo   -- the ID of the units to converted to

@return kind of the initiated conversion, e.g. no conversion (unitsFrom == UnitsTo, fastConversion, convFromTOF or convViaTOF. 
             See ConvertUnits for the details of this transformations

if necessary, also sets up the proper units convertor pointers which do the actual conversion. 
*/
CnvrtToMD::ConvertUnits UnitsConversionHelper::analyzeUnitsConversion(const std::string &UnitsFrom,const std::string &UnitsTo)
{
  // if units are equal, no conversion is necessary;
  if(UnitsFrom.compare(UnitsTo)==0) return CnvrtToMD::ConvertNo;


  // get all known units:
  std::vector<std::string> AllKnownUnits= Kernel::UnitFactory::Instance().getKeys();

  // check if unit conversion is possible at all:
  if(Kernel::Strings::isMember(AllKnownUnits,UnitsFrom)<0)
    throw(std::invalid_argument(" Can not initate conversion from unknown unit: "+UnitsFrom));

  if(Kernel::Strings::isMember(AllKnownUnits,UnitsFrom)<0)
    throw(std::invalid_argument(" Can not initate conversion to unknown unit: "+UnitsTo));


  // is a quick conversion availible?
  m_SourceWSUnit=Kernel::UnitFactory::Instance().create(UnitsFrom);
  if(m_SourceWSUnit->quickConversion(UnitsTo,m_Factor,m_Power))
  {
    return CnvrtToMD::ConvertFast;
  }
  else
  {
    // are the input unts TOF?
    if(UnitsFrom.compare("TOF")==0)
    {
      return CnvrtToMD::ConvertFromTOF;
    }
    else
    {   // convert using TOF
      m_TargetUnit    =Kernel::UnitFactory::Instance().create(UnitsTo);
      return CnvrtToMD::ConvertByTOF;
    }
  }

}

void UnitsConversionHelper::initialize(const MDWSDescription &targetWSDescr, const std::string &unitsTo)
{   
  // obtain input workspace units
  API::MatrixWorkspace_const_sptr inWS2D = targetWSDescr.getInWS();
  if(!inWS2D)throw(std::runtime_error("UnitsConversionHelper::initialize Should not be able to call this function when workpsace is undefined"));

  API::NumericAxis *pAxis = dynamic_cast<API::NumericAxis *>(inWS2D->getAxis(0));
  if(!pAxis)
     throw(std::invalid_argument("Cannot retrieve numeric X axis from the input workspace: "+inWS2D->name()));

  std::string unitsFrom = inWS2D->getAxis(0)->unit()->unitID();

  // get detectors positions and other data needed for units conversion:
  if(!(targetWSDescr.m_PreprDetTable))
    throw std::runtime_error("MDWSDescription does not have a detector table");

  int Emode     =  (int)targetWSDescr.getEMode();

  this->initialize(unitsFrom,unitsTo,targetWSDescr.m_PreprDetTable,Emode);


}
void UnitsConversionHelper::initialize(const std::string &unitsFrom,const std::string &unitsTo,const DataObjects::TableWorkspace_const_sptr &DetWS,int Emode)
{
  m_Emode     =  Emode;

  if(!DetWS)throw std::runtime_error("UnitsConversionHelper::initialize called with empty preprocessed detectors table");

  // Check how the source units relate to the units requested and create source units
  m_UnitCnvrsn = analyzeUnitsConversion(unitsFrom,unitsTo);

  // create target units class
  m_TargetUnit = Kernel::UnitFactory::Instance().create(unitsTo);
  if(!m_TargetUnit)throw(std::runtime_error(" Cannot retrieve target unit from the units factory"));

  // get access to all values used by unit conversion. 
  m_pTwoThetas =  &(DetWS->getColVector<double>("TwoTheta"));
  m_pL2s       =  &(DetWS->getColVector<double>("L2"));

  m_L1        =  DetWS->getLogs()->getPropertyValueAsType<double>("L1");

  // get efix
  m_Efix      =  DetWS->getLogs()->getPropertyValueAsType<double>("Ei");
  m_pEfixedArray=NULL;
  if(m_Emode==(int)Kernel::DeltaEMode::Indirect) m_pEfixedArray = DetWS->getColDataArray<float>("eFixed");

}
/** Method updates unit conversion given the index of detector parameters in the array of detectors */
void UnitsConversionHelper::updateConversion(size_t i)
{
  switch(m_UnitCnvrsn)
  {
  case(CnvrtToMD::ConvertNo):        return;
  case(CnvrtToMD::ConvertFast):      return;
  case(CnvrtToMD::ConvertFromTOF):
    {
      double delta;
      m_TwoTheta = (*m_pTwoThetas)[i];
      m_L2       = (*m_pL2s)[i];
      double    Efix = m_Efix;
      if(m_pEfixedArray)Efix=(double)(*(m_pEfixedArray+i));

      m_TargetUnit->initialize(m_L1,m_L2,m_TwoTheta,m_Emode,Efix,delta);
      return;
    }
  case(CnvrtToMD::ConvertByTOF):
    {
      double delta;
      m_TwoTheta = (*m_pTwoThetas)[i];
      m_L2       = (*m_pL2s)[i];
      double  Efix = m_Efix;
      if(m_pEfixedArray)Efix=(double)(*(m_pEfixedArray+i));

      m_TargetUnit->initialize(m_L1,m_L2,m_TwoTheta,m_Emode,Efix,delta);
      m_SourceWSUnit->initialize(m_L1,m_L2,m_TwoTheta,m_Emode,Efix,delta);
      return;
    }
  default:
    throw std::runtime_error("updateConversion: unknown type of conversion requested");

  }
}
/** do actual unit conversion from  input to oputput data 
@param   val  -- the input value which has to be converted
@return          the input value converted into the units requested.
*/
double UnitsConversionHelper::convertUnits(double val)
{
  switch(m_UnitCnvrsn)
  {
  case(CnvrtToMD::ConvertNo):   
    {
      return val;
    }
  case(CnvrtToMD::ConvertFast):
    {
      return m_Factor*std::pow(val,m_Power);
    }
  case(CnvrtToMD::ConvertFromTOF):
    {  
      return m_TargetUnit->singleFromTOF(val);
    }
  case(CnvrtToMD::ConvertByTOF):
    {
      double tof = m_SourceWSUnit->singleToTOF(val);
      return  m_TargetUnit->singleFromTOF(tof);
    }
  default:
    throw std::runtime_error("updateConversion: unknown type of conversion requested");

  }
}
// copy constructor;
UnitsConversionHelper::UnitsConversionHelper(const UnitsConversionHelper &another)
{
  m_UnitCnvrsn = another.m_UnitCnvrsn;
  m_Factor     = another.m_Factor;
  m_Power      = another.m_Power;

  m_Emode      = another.m_Emode;
  m_L1         = another.m_L1;
  m_Efix       = another.m_Efix;
  m_TwoTheta   = another.m_TwoTheta;
  m_L2         = another.m_L2;
  m_pTwoThetas = another.m_pTwoThetas;
  m_pL2s       = another.m_pL2s;
  m_pEfixedArray=another.m_pEfixedArray;

  if(another.m_SourceWSUnit)m_SourceWSUnit = Kernel::Unit_sptr(another.m_SourceWSUnit->clone());      
  if(another.m_TargetUnit)  m_TargetUnit   = Kernel::Unit_sptr(another.m_TargetUnit->clone());
}

} // endNamespace MDEvents
} // endNamespace Mantid

