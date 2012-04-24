#ifndef  H_CONVERT_TO_MDEVENTS_TRANSF_NOQ
#define  H_CONVERT_TO_MDEVENTS_TRANSF_NOQ
//
#include "MantidMDAlgorithms/ConvertToMDEventsTransfInterface.h"
//
namespace Mantid
{
namespace MDAlgorithms
{
namespace ConvertToMD
{

/** Set of internal classes used by ConvertToMDEvents algorithm and responsible for conversion of input workspace 
  * data into from 1 to 4 output dimensions as function of input parameters
   *
  * This file defines  specializations of generic coordinate transformation templated to the NoQ case
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


//----------------------------------------------------------------------------------------------------------------------
// SPECIALIZATIONS:
//----------------------------------------------------------------------------------------------------------------------
// ---->    NoQ
// NoQ,ANY_Mode -- no units conversion. This templates just copies the data into MD events and not doing any momentum transformations
//
template<ConvertToMD::AnalMode MODE,ConvertToMD::CnvrtUnits CONV,ConvertToMD::XCoordType Type,ConvertToMD::SampleType Sample> 
struct CoordTransformer<ConvertToMD::NoQ,MODE,CONV,Type,Sample>
{
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)    
    {
       // get optional Y axis which can be used in NoQ-kind of algorithms
       pYAxis = pHost->getPAxis(1);
       if(pYAxis){  // two inital properties came from workspace. All are independant; All other dimensions are obtained from properties
           if(!pHost->fillAddProperties(Coord,nd,2))return false;
       }else{        // only one workspace property availible;
           if(!pHost->fillAddProperties(Coord,nd,1))return false;
       }
        // set up units conversion defined by the host algorithm.  
       CONV_UNITS_FROM.setUpConversion(this->pHost,""); 
       return true;
    }

    inline bool calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i)
    {
        CONV_UNITS_FROM.updateConversion(i);
        if(pYAxis){   
            if(Coord[1]<pHost->dim_min[1]||Coord[1]>=pHost->dim_max[1])return false;
            Coord[1] = (coord_t)(pYAxis->operator()(i));
        }
        return true;
    }

    inline bool calc1MatrixCoord(const double& X,std::vector<coord_t> &Coord)const
    {
       if(X<pHost->dim_min[0]||X>=pHost->dim_max[0])return false;
          
       Coord[0]=(coord_t)X;
       return true;
    }
    // should be actually on ICoordTransformer but there is problem with template-overloaded functions
    inline bool calcMatrixCoord(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord)const
    {
       UNUSED_ARG(i);
       double X_ev = CONV_UNITS_FROM.getXConverted(X,j);

       return calc1MatrixCoord(X_ev,Coord);
    }
    inline bool convertAndCalcMatrixCoord(const double & X,std::vector<coord_t> &Coord)const
    {
         double X_ev = CONV_UNITS_FROM.getXConverted(X);
         return calc1MatrixCoord(X_ev,Coord);
    }   

    // constructor;
    CoordTransformer():pYAxis(NULL),pHost(NULL){} 

    inline void setUpTransf(IConvertToMDEventsMethods *pConv){
        pHost = pConv;
    }
private:
   // the variables used for exchange data between different specific parts of the generic ND algorithm:
    // pointer to Y axis of MD workspace
     API::NumericAxis *pYAxis;
     // pointer to MD workspace convertor
     IConvertToMDEventsMethods *pHost;
// class which would convert units
     UnitsConverter<CONV,Type> CONV_UNITS_FROM;
};
//
} // namespace ConvertToMD
} // End MDAlgorighms namespace
} // End Mantid namespace

#endif
