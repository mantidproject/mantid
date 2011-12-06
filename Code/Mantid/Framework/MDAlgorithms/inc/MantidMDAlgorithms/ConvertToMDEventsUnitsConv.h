#ifndef H_CONVERT_TO_MDEVENTS_UNITS
#define H_CONVERT_TO_MDEVENTS_UNITS
#include "MantidMDAlgorithms/ConvertToMDEvents.h"

namespace Mantid
{
namespace MDAlgorithms
{
// predefine the host class:
//template<Q_state Q,AnalMode MODE,CnvrtUnits CONV>
//struct COORD_TRANSFORMER;

// DO UNITS CONVERSION -- generally does almost nothing
// procedure which converts/ non-converts units
template<CnvrtUnits CONV,Q_state Q,AnalMode MODE> 
struct UNITS_CONVERSION
{ 
    /// set up all variables necessary for units conversion at the beginning of the loop
    inline void     setUpConversion(COORD_TRANSFORMER<Q,MODE,CONV> *){};
    /// update all variables in the loop over spectra
    inline void     updateConversion(uint64_t){};
    /// convert current X variable
    inline coord_t  getXConverted(const MantidVec& X,size_t j)const{return coord_t(0.5*(X[j]+X[j+1]));}

};

// Fast conversion:
template<Q_state Q,AnalMode MODE>
struct UNITS_CONVERSION<ConvertFast,Q,MODE>
{

    void setUpConversion(COORD_TRANSFORMER<Q,MODE,ConvertFast> *pCoordTransf)
    {       
       const Kernel::Unit_sptr pThisUnit= pCoordTransf->getAxisUnits();
       std::string native_units         = pCoordTransf->getNativeUnitsID();

       if(!pThisUnit->quickConversion(native_units,factor,power)){
           throw(std::logic_error(" should be able to convert units and catch case of non-conversions much earlier"));
       }
      
    };
    // does nothing
    inline void    updateConversion(const uint64_t ){}
    // 
    inline coord_t  getXConverted(const MantidVec& X,size_t j)const
    {
        coord_t X0=coord_t(0.5*(X[j]+X[j+1]));
        return (factor*std::pow(X0,power));
    }
private:
    // variables for units conversion:
    double factor, power;

};

// Convert from TOF:
template<Q_state Q,AnalMode MODE>
struct UNITS_CONVERSION<ConvFromTOF,Q,MODE>
{

    void setUpConversion(COORD_TRANSFORMER<Q,MODE,ConvFromTOF> *pCoordTransf)
    {       
       // check if axis units are TOF
       const Kernel::Unit_sptr pThisUnit= pCoordTransf->getAxisUnits();
       if(std::string("TOF").compare(pThisUnit->unitID())!=0){
           throw(std::logic_error(" it whould be only TOF here"));
       }
       // get units class, requested by subalgorithm
       std::string native_units       = pCoordTransf->getNativeUnitsID();
       Kernel::Unit_sptr pWSUnit      = Kernel::UnitFactory::Instance().create(native_units);
       if(!pWSUnit){
           throw(std::logic_error(" can not retrieve workspace unit from the units factory"));
       }
       // get detectors positions and other data needed for units conversion:
       const preprocessed_detectors det = pCoordTransf->getPrepDetectors();
       pTwoTheta = &(det.TwoTheta[0]);
       pL2       = &(det.L2[0]);
       L1        =  det.L1;
       efix      =  pCoordTransf->getEi();
    };
    inline void updateConversion(uint64_t i)
    {
        double delta;
        pWSUnit->initialize(L1,pL2[i],pTwoTheta[i],MODE,efix,delta);
    }
    inline coord_t  getXConverted(const MantidVec& X,size_t j)const{
        double X0=(0.5*(X[j]+X[j+1]));        
        return (coord_t)pWSUnit->singleFromTOF(X0);
    }
private:
    // variables for units conversion:

    // Make local copies of the units. This allows running the loop in parallel
    //      Unit * localFromUnit = fromUnit->clone();
    //  Unit * localOutputUnit = outputUnit->clone();
      Kernel::Unit_sptr pWSUnit;
 
      double L1,efix;
      double const *pTwoTheta;
      double const *pL2;

};

// Convert By TOF:
template<Q_state Q,AnalMode MODE>
struct UNITS_CONVERSION<ConvByTOF,Q,MODE>
{

    void setUpConversion(COORD_TRANSFORMER<Q,MODE,ConvByTOF> *pCoordTransf)
    {       

       pSourceWSUnit= pCoordTransf->getAxisUnits();
       if(!pSourceWSUnit){
           throw(std::logic_error(" can not retrieve source workspace units from the input workspacee"));
       }

       // get units class, requested by subalgorithm
       std::string native_units       = pCoordTransf->getNativeUnitsID();
       Kernel::Unit_sptr pWSUnit      = Kernel::UnitFactory::Instance().create(native_units);
       if(!pWSUnit){
           throw(std::logic_error(" can not retrieve target workspace unit from the units factory"));
       }

       // get detectors positions and other data needed for units conversion:
       const preprocessed_detectors det = pCoordTransf->getPrepDetectors();
       pTwoTheta = &(det.TwoTheta[0]);
       pL2       = &(det.L2[0]);
       L1        =  det.L1;
       // get efix
       efix      =  pCoordTransf->getEi();
    };

    inline void updateConversion(uint64_t i)
    {
        double delta;
        pWSUnit->initialize(L1,pL2[i],pTwoTheta[i],MODE,efix,delta);
        pSourceWSUnit->initialize(L1,pL2[i],pTwoTheta[i],MODE,efix,delta);
    }
    //
    inline coord_t  getXConverted(const MantidVec& X,size_t j)const{
        double X0=(0.5*(X[j]+X[j+1]));  
        double tof  = pSourceWSUnit->singleToTOF(X0);
        return (coord_t)pWSUnit->singleFromTOF(tof);
    }
private:
    // variables for units conversion:
    // pointer to input workpsace units 
      Kernel::Unit_sptr pWSUnit;
      // pointer to target workspace units
      Kernel::Unit_sptr pSourceWSUnit;
    // Make local copies of the units. This allows running the loop in parallel
    //      Unit * localFromUnit = fromUnit->clone();
    //  Unit * localOutputUnit = outputUnit->clone();
  
      double L1,efix;
      double const *pTwoTheta;
      double const *pL2;

};



} // endNamespace MDAlgorithms
} // endNamespace Mantid

#endif