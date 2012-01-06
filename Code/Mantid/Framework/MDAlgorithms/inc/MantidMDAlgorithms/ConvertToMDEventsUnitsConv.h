#ifndef H_CONVERT_TO_MDEVENTS_UNITS
#define H_CONVERT_TO_MDEVENTS_UNITS
#include "MantidMDAlgorithms/ConvertToMDEvents.h"
/** Set of internal classes used by ConvertToMDEvents algorithm and responsible for Units conversion
   *
   * @date 11-10-2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
namespace Mantid
{
namespace MDAlgorithms
{

// How to treat X-coordinates:
// for histohram we take centerpiece average
template<XCoordType TYPE>
double XValue(const MantidVec& X,size_t j){return static_cast<double>(0.5*(X[j]+X[j+1]));}
// for axis type -- just value
template <>
double XValue<Axis>(const MantidVec& X,size_t j){return static_cast<double>(X[j]);}
// DO UNITS CONVERSION --


//  general procedure does nothing (non-converts units)
template<CnvrtUnits CONV,XCoordType Type> 
struct UNITS_CONVERSION
{ 
    /** Set up all variables necessary for units conversion at the beginning of the conversion loop
     * @param pHost   -- pointer to the Mantid algorithm, which calls this function to obtain the variables, 
     *                   relevant to the units conversion
    */
    inline void     setUpConversion(ConvertToMDEvents const * const pHost ){UNUSED_ARG(pHost);}
    /// Update all spectra dependednt  variables, relevant to conversion in the loop over spectra (detectors)
    inline void     updateConversion(uint64_t i){UNUSED_ARG(i);}
    /// Convert current X variable into the units requested;
    inline coord_t  getXConverted(const MantidVec& X,size_t j)const
    {
        return XValue<Type>(X,j);
    }

};

// Fast conversion:
template<XCoordType Type>
struct UNITS_CONVERSION<ConvFast,Type>
{

    void setUpConversion(ConvertToMDEvents const *const pHost)
    {       
       const Kernel::Unit_sptr pThisUnit= ConvertToMDEvents::getAxisUnits(pHost);
       std::string native_units         = ConvertToMDEvents::getNativeUnitsID(pHost);

       if(!pThisUnit->quickConversion(native_units,factor,power)){
           throw(std::logic_error(" should be able to convert units and catch case of non-conversions much earlier"));
       }
      
    };
    // does nothing
    inline void    updateConversion(const uint64_t ){}
    // convert X coordinate using power series
    inline coord_t  getXConverted(const MantidVec& X,size_t j)const
    {
        return (factor*std::pow(XValue<Type>(X,j),power));
    }
private:
    // variables for units conversion:
    double factor, power;

};

// Convert from TOF:
template<XCoordType Type>
struct UNITS_CONVERSION<ConvFromTOF,Type>
{

    void setUpConversion(ConvertToMDEvents const *const pHost)
    {       
       // check if axis units are TOF
       const Kernel::Unit_sptr pThisUnit= ConvertToMDEvents::getAxisUnits(pHost);
       if(std::string("TOF").compare(pThisUnit->unitID())!=0){
           throw(std::logic_error(" it whould be only TOF here"));
       }
       // get units class, requested by subalgorithm
       std::string native_units       = ConvertToMDEvents::getNativeUnitsID(pHost);
       pWSUnit      = Kernel::UnitFactory::Instance().create(native_units);
       if(!pWSUnit){
           throw(std::logic_error(" can not retrieve workspace unit from the units factory"));
       }
       // get detectors positions and other data needed for units conversion:
       pTwoTheta =  ConvertToMDEvents::getPrepDetectors(pHost).pTwoTheta();
       pL2       =  ConvertToMDEvents::getPrepDetectors(pHost).pL2();
       L1        =  ConvertToMDEvents::getPrepDetectors(pHost).L1;

       efix      =  ConvertToMDEvents::getEi(pHost);
       emode     =  ConvertToMDEvents::getEMode(pHost);
    };
    inline void updateConversion(uint64_t i)
    {
        double delta;
        pWSUnit->initialize(L1,pL2[i],pTwoTheta[i],emode,efix,delta);
    }
    inline coord_t  getXConverted(const MantidVec& X,size_t j)const{
   
        return (coord_t)pWSUnit->singleFromTOF(XValue<Type>(X,j));
    }
private:
    // variables for units conversion:

    // Make local copies of the units. This allows running the loop in parallel
    //      Unit * localFromUnit = fromUnit->clone();
    //  Unit * localOutputUnit = outputUnit->clone();
      Kernel::Unit_sptr pWSUnit;
      // Energy analysis mode
      int emode;
 
      double L1,efix;
      double const *pTwoTheta;
      double const *pL2;

};

// Convert By TOF:
template<XCoordType Type>
struct UNITS_CONVERSION<ConvByTOF,Type>
{

    void setUpConversion(ConvertToMDEvents const *const pHost)
    {       

       pSourceWSUnit= ConvertToMDEvents::getAxisUnits(pHost);
       if(!pSourceWSUnit){
           throw(std::logic_error(" can not retrieve source workspace units from the input workspacee"));
       }

       // get units class, requested by subalgorithm
       std::string native_units  = ConvertToMDEvents::getNativeUnitsID(pHost);
       pWSUnit                   = Kernel::UnitFactory::Instance().create(native_units);
       if(!pWSUnit){
           throw(std::logic_error(" can not retrieve target workspace unit from the units factory"));
       }

       // get detectors positions and other data needed for units conversion:
       pTwoTheta =  ConvertToMDEvents::getPrepDetectors(pHost).pTwoTheta();
       pL2       =  ConvertToMDEvents::getPrepDetectors(pHost).pL2();
       L1        =  ConvertToMDEvents::getPrepDetectors(pHost).L1;
       // get efix
       efix      =  ConvertToMDEvents::getEi(pHost);
       emode     =  ConvertToMDEvents::getEMode(pHost);
    };

    inline void updateConversion(uint64_t i)
    {
        double delta;
        pWSUnit->initialize(L1,pL2[i],pTwoTheta[i],emode,efix,delta);
        pSourceWSUnit->initialize(L1,pL2[i],pTwoTheta[i],emode,efix,delta);
    }
    //
    inline coord_t  getXConverted(const MantidVec& X,size_t j)const{
        double tof  = pSourceWSUnit->singleToTOF(XValue<Type>(X,j));
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
      int emode;
      double L1,efix;
      double const *pTwoTheta;
      double const *pL2;

};



} // endNamespace MDAlgorithms
} // endNamespace Mantid

#endif