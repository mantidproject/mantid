#ifndef H_CONVERT_TO_MDEVENTS_UNITS
#define H_CONVERT_TO_MDEVENTS_UNITS
/**TODO: FOR DEPRICATION */ 
#include "MantidMDAlgorithms/ConvertToMDEventsParams.h"
#include "MantidMDEvents/MDWSDescription.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
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
// for Histogram we take centerpiece average
template<ConvertToMD::XCoordType TYPE>
inline double XValue(const MantidVec& X,size_t j){return static_cast<double>(0.5*(X[j]+X[j+1]));}
// for axis type -- just value
template <>
inline double XValue<ConvertToMD::Centered>(const MantidVec& X,size_t j){return static_cast<double>(X[j]);}
// DO UNITS CONVERSION --


//  general procedure does nothing (non-converts units)
template<ConvertToMD::CnvrtUnits CONV,ConvertToMD::XCoordType TYPE> 
struct UnitsConverter
{ 
    /** Set up all variables necessary for units conversion at the beginning of the conversion loop
     * @param pHost     -- pointer to the Mantid algorithm, which calls this function to obtain the variables, 
     *                     relevant to the units conversion
     *@param targ_units -- the units we want to convert to 
    */
    inline void  setUpConversion(const MDEvents::MDWSDescription &targetWSDescr, const std::string &units_from,const std::string &units_to )
    {UNUSED_ARG(targetWSDescr);UNUSED_ARG(units_from);UNUSED_ARG(units_to);}
    /// Update all spectra dependednt  variables, relevant to conversion in the loop over spectra (detectors)
    inline void     updateConversion(size_t i){UNUSED_ARG(i);}
    /// Convert current X variable into the units requested;
    inline double  getXConverted(const MantidVec& X,size_t j)const
    {
        return XValue<TYPE>(X,j);
    }
    /// Convert current X variable into the units requested;
    inline double  getXConverted(const double& X)const
    {
        return X;
    }


};

// Fast conversion:
template<ConvertToMD::XCoordType TYPE>
struct UnitsConverter<ConvertToMD::ConvFast,TYPE>
{

    void setUpConversion(const MDEvents::MDWSDescription &targetWSDescr, const std::string &units_from,const std::string &units_to)
    {       
       UNUSED_ARG(targetWSDescr);
       //const Kernel::Unit_sptr pThisUnit= pHost->getAxisUnits();
      const Kernel::Unit_sptr pThisUnit= Kernel::UnitFactory::Instance().create(units_from);
    
       if(!pThisUnit->quickConversion(units_to,factor,power)){
           throw(std::logic_error(" should be able to convert units quickly and catch the case of non-conversions before invoking this template"));
       }
      
    };
    // does nothing
    inline void    updateConversion(const size_t ){}
    // convert X coordinate using power series
    inline double  getXConverted(const MantidVec& X,size_t j)const
    {
        double x = XValue<TYPE>(X,j);
        return getXConverted(x);
    }
   // convert X coordinate using power series
    inline double  getXConverted(const double& X)const
    {
        return (factor*std::pow(X,power));
    }
private:
    // variables for units conversion:
    double factor, power;

};

// Convert from TOF:
template<ConvertToMD::XCoordType TYPE>
struct UnitsConverter<ConvertToMD::ConvFromTOF,TYPE>
{

    void setUpConversion(const MDEvents::MDWSDescription &targetWSDescr, const std::string &units_from,const std::string &units_to)
    {       
       // check if axis units are TOF
       //const Kernel::Unit_sptr pThisUnit= pHost->getAxisUnits();       
       if(std::string("TOF").compare(units_from)!=0){
           throw(std::logic_error(" it whould be only TOF here"));
       }
      // create units for this subalgorith to convert to 
       pWSUnit      = Kernel::UnitFactory::Instance().create(units_to);
       if(!pWSUnit){
           throw(std::logic_error(" can not retrieve workspace unit from the units factory"));
       }
   
 
      // get detectors positions and other data needed for units conversion:
      pTwoTheta =  &(targetWSDescr.getDetectors()->getTwoTheta());      
      pL2       =  &(targetWSDescr.getDetectors()->getL2());

      L1        =  targetWSDescr.getDetectors()->getL1();
      // get efix
      efix      =  targetWSDescr.getEi();
      emode  =  (int)targetWSDescr.getEMode();

    };
    inline void updateConversion(size_t i)
    {
        double delta;
        pWSUnit->initialize(L1,(*pL2)[i],(*pTwoTheta)[i],emode,efix,delta);
    }
    inline double  getXConverted(const MantidVec& X,size_t j)const
    {   
        double x = XValue<TYPE>(X,j);
        return getXConverted(x);
    }

   inline double  getXConverted(const double& X)const
    {
        return pWSUnit->singleFromTOF(X);
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
      std::vector<double>const *pTwoTheta;
      std::vector<double>const *pL2;


};

// Convert By TOF:
template<ConvertToMD::XCoordType TYPE>
struct UnitsConverter<ConvertToMD::ConvByTOF,TYPE>
{

    void setUpConversion(const MDEvents::MDWSDescription &targetWSDescr, const std::string &units_from,const std::string &units_to)
    {       

       pSourceWSUnit=     Kernel::UnitFactory::Instance().create(units_from);
       //pSourceWSUnit= pHost->getAxisUnits();
       if(!pSourceWSUnit){
           throw(std::logic_error(" can not retrieve source workspace units from the input workspacee"));
       }

       // get units class, requested by subalgorithm
       pWSUnit                   = Kernel::UnitFactory::Instance().create(units_to);
       if(!pWSUnit){
           throw(std::logic_error(" can not retrieve target workspace unit from the units factory"));
       }

       // get detectors positions and other data needed for units conversion:
       pTwoTheta =  &(targetWSDescr.getDetectors()->getTwoTheta());      
       pL2       =  &(targetWSDescr.getDetectors()->getL2());

       L1        =  targetWSDescr.getDetectors()->getL1();
       // get efix
       efix      =  targetWSDescr.getEi();
       emode  =  (int)targetWSDescr.getEMode();



    };

    inline void updateConversion(size_t i)
    {
        double delta;
        pWSUnit->initialize(L1,(*pL2)[i],(*pTwoTheta)[i],emode,efix,delta);
        pSourceWSUnit->initialize(L1,(*pL2)[i],(*pTwoTheta)[i],emode,efix,delta);
    }
    //
    inline double  getXConverted(const MantidVec& X,size_t j)const
    {
        double x = XValue<TYPE>(X,j);
        return getXConverted(x);
    }
    // 
    inline double  getXConverted(const double& X)const
    {
        double tof  = pSourceWSUnit->singleToTOF(X);
        return (double)pWSUnit->singleFromTOF(tof);
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
      std::vector<double>const *pTwoTheta;
      std::vector<double>const *pL2;

};

} // endNamespace MDAlgorithms
} // endNamespace Mantid

#endif