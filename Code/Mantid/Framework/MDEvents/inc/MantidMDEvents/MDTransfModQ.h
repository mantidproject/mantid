#ifndef  H_CONVERT_TO_MDEVENTS_MODQ_TRANSF
#define  H_CONVERT_TO_MDEVENTS_MODQ_TRANSF
//
#include "MantidMDEvents/MDTransform.h"
//
namespace Mantid
{
namespace MDAlgorithms
{


/** Set of internal class used by ConvertToMDEvents algorithm and responsible for conversion of input workspace 
  * data into proper number of output dimensions for ModQ transformation
  * 
  * Currently contains Elastic and Inelastic transformations
  *
  * This particular file defines  specializations of generic coordinate transformation templated to the ModQ case
   *
   * @date 16-05-2012

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



class DLLExport MDTransfModQInelastic: public IMDTransformation
{ 
    public:
    bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd);
    //
    inline bool calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i)   
    inline bool calc1MatrixCoord(const double& E_tr,std::vector<coord_t> &Coord)const
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
    CoordTransformer():pDet(NULL),pHost(NULL){}
    void setUpTransf(IConvertToMDEventsWS *pConv){
        pHost = pConv;
    }
private:
    // the energy of the incident neutrons
    double Ei;
    // the wavevector of incident neutrons
    double ki;
    //  directions to the detectors 
    double ex,ey,ez;
    // the matrix which transforms the neutron momentums from lablratory to crystall coordinate system. 
    std::vector<double> rotMat;
    // min-max values, some modified to work with squared values:
    std::vector<double> dim_min,dim_max;
    //
    Kernel::V3D const *pDet;
    // Calling Mantid algorithm
    IConvertToMDEventsWS *pHost;
    // class which would convert units
    UnitsConverter<CONV,TYPE> CONV_UNITS_FROM;
 
};
// ModQ,Elastic 
class DLLExport MDTransfModQElastic: public IMDTransformation
{ 
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd);
    inline bool calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i)
    inline bool calc1MatrixCoord(const double& k0,std::vector<coord_t> &Coord)const

    inline bool calcMatrixCoord(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord)const
    inline bool convertAndCalcMatrixCoord(const double & X,std::vector<coord_t> &Coord)const

    // constructor;
    MDTransfModQElastic():pDet(NULL),pHost(NULL){}
    void setUpTransf(IConvertToMDEventsWS *pConv){
        pHost = pConv;
    }
private:
    // the energy of the incident neutrons
    double Ei;
    // the wavevector of incident neutrons
    double ki;
    //  directions to the detectors 
    double ex,ey,ez;
    // the matrix which transforms the neutron momentums from lablratory to crystall coordinate system. 
    std::vector<double> rotMat;
    // min-max values, some modified to work with squared values:
    std::vector<double> dim_min,dim_max;
    //
    Kernel::V3D const * pDet;
    // Calling Mantid algorithm
    IConvertToMDEventsWS *pHost;  
   // class which would convert units
    UnitsConverter<CONV,TYPE> CONV_UNITS_FROM;
 
};

} // End MDAlgorighms namespace
} // End Mantid namespace

#endif
