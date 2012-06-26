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
ConvertToMD::ConvertUnits UnitsConversionHelper::analyzeUnitsConversion(const std::string &UnitsFrom,const std::string &UnitsTo)
{
    // if units are equal, no conversion is necessary;
    if(UnitsFrom.compare(UnitsTo)==0)
    {
        return ConvertToMD::ConvertNo;
    }


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
    pSourceWSUnit=Kernel::UnitFactory::Instance().create(UnitsFrom);
    if(pSourceWSUnit->quickConversion(UnitsTo,factor,power))
    {
        return ConvertToMD::ConvertFast;
    }else{
        // is the input unts are TOF?
        if(UnitsFrom.compare("TOF")==0){
            return ConvertToMD::ConvertFromTOF;
        }else{            // convert using TOF
            pTargetUnit    =Kernel::UnitFactory::Instance().create(UnitsTo);
            return ConvertToMD::ConvertByTOF;
        }
    }
    
}

void UnitsConversionHelper::initialize(const MDWSDescription &TWSD, const std::string &units_to)
{   
  // obtain input workspace units
    API::MatrixWorkspace_const_sptr inWS2D = TWSD.getInWS();
    if(!inWS2D.get()){
        throw(std::logic_error("UnitsConversionHelper::initialize Should not be able to call this function when workpsace is undefined"));
    }
    API::NumericAxis *pAxis = dynamic_cast<API::NumericAxis *>(inWS2D->getAxis(0));
    if(!pAxis){
        std::string ERR = "can not retrieve numeric X axis from the input workspace: "+inWS2D->name();
        throw(std::invalid_argument(ERR));
   }
   pSourceWSUnit =   inWS2D->getAxis(0)->unit();
   if(!pSourceWSUnit){
        throw(std::logic_error(" can not retrieve source workspace units from the source workspace's numeric axis"));
   }
   // Check how input workspace units relate to the units requested
   UnitCnvrsn = analyzeUnitsConversion(pSourceWSUnit->unitID(),units_to);


   // get units class, requested by subalgorithm
   pTargetUnit = Kernel::UnitFactory::Instance().create(units_to);
   if(!pTargetUnit){
         throw(std::logic_error(" can not retrieve target unit from the units factory"));
   }

   // get detectors positions and other data needed for units conversion:
    pTwoTheta =  &(TWSD.getDetectors()->getTwoTheta());      
    pL2       =  &(TWSD.getDetectors()->getL2());

    L1        =  TWSD.getDetectors()->getL1();
   // get efix
    efix      =  TWSD.getEi();
    emode     =  (int)TWSD.getEMode();

}
/** Method updates unit conversion given the index of detector parameters in the array of detectors */
void UnitsConversionHelper::updateConversion(size_t i)
{
    switch(UnitCnvrsn)
    {
    case(ConvertToMD::ConvertNo):        return;
    case(ConvertToMD::ConvertFast):      return;
    case(ConvertToMD::ConvertFromTOF):
        {
            double delta;
            twoTheta = (*pTwoTheta)[i];
            L2       = (*pL2)[i];
            pTargetUnit->initialize(L1,L2,twoTheta,emode,efix,delta);
            return;
        }
    case(ConvertToMD::ConvertByTOF):
        {
            double delta;
            twoTheta = (*pTwoTheta)[i];
            L2       = (*pL2)[i];
            pTargetUnit->initialize(L1,L2,twoTheta,emode,efix,delta);
            pSourceWSUnit->initialize(L1,L2,twoTheta,emode,efix,delta);
            return;
        }
    default:
           throw std::runtime_error("updateConversion: unknown type of conversion requested");
 
    }
}
/** Convert units for the input data */
double UnitsConversionHelper::convertUnits(double val)
{
    switch(UnitCnvrsn)
    {
    case(ConvertToMD::ConvertNo):   
        {
            return val;
        }
    case(ConvertToMD::ConvertFast):
        {
            return factor*std::pow(val,power);
        }
    case(ConvertToMD::ConvertFromTOF):
        {  
            return pTargetUnit->singleFromTOF(val);
        }
    case(ConvertToMD::ConvertByTOF):
        {
             double tof = pSourceWSUnit->singleToTOF(val);
             return  pTargetUnit->singleFromTOF(tof);
        }
    default:
           throw std::runtime_error("updateConversion: unknown type of conversion requested");
 
    }
}
// copy constructor;
UnitsConversionHelper::UnitsConversionHelper(const UnitsConversionHelper &another)
{
      UnitCnvrsn = another.UnitCnvrsn;
      factor     = another.factor;
      power      = another.power;

      emode      = another.emode;
      L1         = another.L1;
      efix       = another.efix;
      twoTheta   = another.twoTheta;
      L2         = another.L2;
      pTwoTheta  = another.pTwoTheta;
      pL2        = another.pL2;

      if(pSourceWSUnit.get())pSourceWSUnit = Kernel::Unit_sptr(another.pSourceWSUnit->clone());      
      if(pTargetUnit.get())  pTargetUnit   = Kernel::Unit_sptr(another.pSourceWSUnit->clone());
}

} // endNamespace MDEvents
} // endNamespace Mantid

