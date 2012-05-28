#include "MantidMDAlgorithms/UnitConversionHelper.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid
{
namespace MDAlgorithms
{
/*** function checks if the candidate belongs to the group and returns its number in the group or -1 if the candidate is not a group member */
int is_member(const std::vector<std::string> &group,const std::string &candidate)
{
    int num(-1);
    for(size_t i=0;i<group.size();i++){
        if(candidate.compare(group[i])==0){
            num = int(i);
            return num;
        }
    }
    return num;
}

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
    if(is_member(AllKnownUnits,UnitsFrom)<0)
    {
       std::string ERR=" Can not initate conversion from unknown unit: "+UnitsFrom;
       throw(std::invalid_argument(ERR));
    }
    if(is_member(AllKnownUnits,UnitsTo)<0)
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

void UnitsConversionHelper::initialize(const ConvToMDPreprocDet &det, API::MatrixWorkspace_const_sptr inWS2D,const std::string &units_to)
{   
  // obtain input workspace units
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
    pTwoTheta =  &(det.getTwoTheta());      
    pL2       =  &(det.getL2());

    L1        =  det.getL1();
   // get efix
    efix      =  det.getEfix();
    emode     =  det.getEmode();

}

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
/** Convert units for the vector of input data */
void UnitsConversionHelper::convertUnits(const std::vector<double> &convertFrom, std::vector<double> &convertTo)
{
    std::vector<double> unused;
    convertTo.assign(convertFrom.begin(),convertFrom.end());
    switch(UnitCnvrsn)
    {
    case(ConvertToMD::ConvertNo):   
        {
            return ;
        }
    case(ConvertToMD::ConvertFast):
        {
            size_t nPoints = convertFrom.size();
            for(size_t i=0;i<nPoints;i++){
                convertTo[i]=factor*std::pow(convertFrom[i],power);
            }
            return;
        }
    case(ConvertToMD::ConvertFromTOF):
        {  // unused delta
            double delta;
            pTargetUnit->fromTOF(convertTo, unused, L1,L2,twoTheta,emode,efix,delta);
            return;
        }
    case(ConvertToMD::ConvertByTOF):
        {
             double delta; // unused delta
             pSourceWSUnit->toTOF(convertTo, unused, L1,L2,twoTheta,emode,efix,delta);
             pTargetUnit->fromTOF(convertTo, unused, L1,L2,twoTheta,emode,efix,delta);
             return;
        }
    default:
           throw std::runtime_error("updateConversion: unknown type of conversion requested");
 
    }


}

} // endNamespace MDEvents
} // endNamespace Mantid

