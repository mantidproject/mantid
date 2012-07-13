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

@returns kind of the initiated conversion, e.g. no conversion (unitsFrom == UnitsTo, fastConversion, convFromTOF or convViaTOF

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
  {
    std::string ERR=" Can not initate conversion from unknown unit: "+UnitsFrom;
    throw(std::invalid_argument(ERR));
  }
  if(Kernel::Strings::isMember(AllKnownUnits,UnitsFrom)<0)
  {
    std::string ERR=" Can not initate conversion to unknown unit: "+UnitsTo;
    throw(std::invalid_argument(ERR));
  }



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

void UnitsConversionHelper::initialize(const MDWSDescription &targetWSDescr, const std::string &units_to)
{   
  // obtain input workspace units
  API::MatrixWorkspace_const_sptr inWS2D = targetWSDescr.getInWS();
  if(!inWS2D)throw(std::logic_error("UnitsConversionHelper::initialize Should not be able to call this function when workpsace is undefined"));

  API::NumericAxis *pAxis = dynamic_cast<API::NumericAxis *>(inWS2D->getAxis(0));
  if(!pAxis)
  {
    std::string ERR = "can not retrieve numeric X axis from the input workspace: "+inWS2D->name();
    throw(std::invalid_argument(ERR));
  }

  m_SourceWSUnit = inWS2D->getAxis(0)->unit();
  if(!m_SourceWSUnit)throw(std::logic_error(" can not retrieve source workspace units from the source workspace's numeric axis"));

  // Check how input workspace units relate to the units requested
  m_UnitCnvrsn = analyzeUnitsConversion(m_SourceWSUnit->unitID(),units_to);


  // get units class, requested by subalgorithm
  m_TargetUnit = Kernel::UnitFactory::Instance().create(units_to);
  if(!m_TargetUnit)throw(std::logic_error(" can not retrieve target unit from the units factory"));


  // get detectors positions and other data needed for units conversion:
  m_pTwoThetas =  &(targetWSDescr.getDetectors()->getTwoTheta());      
  m_pL2s       =  &(targetWSDescr.getDetectors()->getL2());

  m_L1        =  targetWSDescr.getDetectors()->getL1();
  // get efix
  m_Efix      =  targetWSDescr.getEi();
  m_Emode     =  (int)targetWSDescr.getEMode();

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
      m_TargetUnit->initialize(m_L1,m_L2,m_TwoTheta,m_Emode,m_Efix,delta);
      return;
    }
  case(CnvrtToMD::ConvertByTOF):
    {
      double delta;
      m_TwoTheta = (*m_pTwoThetas)[i];
      m_L2       = (*m_pL2s)[i];
      m_TargetUnit->initialize(m_L1,m_L2,m_TwoTheta,m_Emode,m_Efix,delta);
      m_SourceWSUnit->initialize(m_L1,m_L2,m_TwoTheta,m_Emode,m_Efix,delta);
      return;
    }
  default:
    throw std::runtime_error("updateConversion: unknown type of conversion requested");

  }
}
/** Convert units for the input data */
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

  if(another.m_SourceWSUnit)m_SourceWSUnit = Kernel::Unit_sptr(another.m_SourceWSUnit->clone());      
  if(another.m_TargetUnit)  m_TargetUnit   = Kernel::Unit_sptr(another.m_TargetUnit->clone());
}

} // endNamespace MDEvents
} // endNamespace Mantid

