#ifndef  H_CONVERT_TO_MDEVENTS_MODQ_TRANSF
#define  H_CONVERT_TO_MDEVENTS_MODQ_TRANSF
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
  * This particular file defines  specializations of generic coordinate transformation templated to the ModQ case
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


// ModQ,Inelastic 
template<AnalMode MODE,CnvrtUnits CONV,XCoordType Type,SampleType Sample> 
struct CoordTransformer<ModQ,MODE,CONV,Type,Sample>
{ 
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)
    {
        /// 2 coordinates (|Q|, DeltaE) came from workspace, are interconnnected all additional defined by  properties, 
        /// here we copy these values into Coord vector
        if(!pHost->fillAddProperties(Coord,nd,2))return false;

        // energy 
         Ei  =  pHost->getEi();
         // the wave vector of incident neutrons;
         ki=sqrt(Ei/PhysicalConstants::E_mev_toNeutronWavenumberSq); 
         // get transformation matrix (needed for CrystalAsPoder mode)
         rotMat = pHost->getTransfMatrix();
         // if workspace is not in DeltaE, initiate units conversion, if not -- empty conversion should be instanciated
         CONV_UNITS_FROM.setUpConversion(this->pHost,"DeltaE"); 
         // get pointer to the positions of the detectors
          std::vector<Kernel::V3D> const & DetDir = pHost->pPrepDetectors()->getDetDir();
          pDet = &DetDir[0];

          dim_min.assign(pHost->dim_min.begin(),pHost->dim_min.end());
          dim_max.assign(pHost->dim_max.begin(),pHost->dim_max.end());
          // dim_min here is a momentum and it is verified on momentum squared base
          dim_min[0]*=dim_min[0];
          dim_max[0]*=dim_max[0];
        //
         return true;
    }
    //
    inline bool calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i)
    {
        UNUSED_ARG(Coord); 
        CONV_UNITS_FROM.updateConversion(i);
        ex = (pDet+i)->X();
        ey = (pDet+i)->Y();
        ez = (pDet+i)->Z();
        return true;
    }
    
    inline bool calc1MatrixCoord(const double& E_tr,std::vector<coord_t> &Coord)const
    {
        if(E_tr<dim_min[1]||E_tr>=dim_max[1])return false;
        Coord[1]    =(coord_t)E_tr;

        // get module of the wavevector for scattered neutrons
        double k_tr = k_trans<MODE>(Ei,E_tr);
   
        double  qx  =  -ex*k_tr;                
        double  qy  =  -ey*k_tr;       
        double  qz  = ki - ez*k_tr;
        // transformation matrix has to be here for "Crystal AS Powder conversion mode, further specialization possible if "powder" switch provided"
        double Qx  = (rotMat[0]*qx+rotMat[1]*qy+rotMat[2]*qz);
        double Qy  = (rotMat[3]*qx+rotMat[4]*qy+rotMat[5]*qz); 
        double Qz  = (rotMat[6]*qx+rotMat[7]*qy+rotMat[8]*qz);

        double Qsq = Qx*Qx+Qy*Qy+Qz*Qz;
        if(Qsq < dim_min[0]||Qsq>=dim_max[0])return false;
        Coord[0]   = (coord_t)sqrt(Qsq);

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
    CoordTransformer():pDet(NULL),pHost(NULL){}
    void setUpTransf(IConvertToMDEventsMethods *pConv){
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
    IConvertToMDEventsMethods *pHost;
    // class which would convert units
    UnitsConverter<CONV,Type> CONV_UNITS_FROM;
 
};
// ModQ,Elastic 
template<CnvrtUnits CONV,XCoordType Type,SampleType Sample> 
struct CoordTransformer<ModQ,Elastic,CONV,Type,Sample>
{ 
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)
    {
      
        // 1 coordinate (|Q|) came from workspace, all additional defined by  properties:
        if(!pHost->fillAddProperties(Coord,nd,1))return false;
          // get transformation matrix (needed for CrystalAsPoder mode)
          rotMat = pHost->getTransfMatrix();
          // 
          CONV_UNITS_FROM.setUpConversion(this->pHost,"Momentum"); 

         // get pointer to the positions of the detectors
          std::vector<Kernel::V3D> const & DetDir = pHost->pPrepDetectors()->getDetDir();
          pDet = &DetDir[0];     //

          dim_min.assign(pHost->dim_min.begin(),pHost->dim_min.end());
          dim_max.assign(pHost->dim_max.begin(),pHost->dim_max.end());
          // dim_min here is a momentum and it is verified on momentum squared base
          dim_min[0]*=dim_min[0];
          dim_max[0]*=dim_max[0];
        return true;
    }
    //
    inline bool calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i)
    {
        UNUSED_ARG(Coord); 
        CONV_UNITS_FROM.updateConversion(i);
        ex = (pDet+i)->X();
        ey = (pDet+i)->Y();
        ez = (pDet+i)->Z();
        return true;
    }
    //
    inline bool calc1MatrixCoord(const double& k0,std::vector<coord_t> &Coord)const
    {
   
        double  qx  =  -ex*k0;                
        double  qy  =  -ey*k0;       
        double  qz  = (1 - ez)*k0;
        // transformation matrix has to be here for "Crystal AS Powder mode, further specialization possible if "
        double Qx  = (rotMat[0]*qx+rotMat[1]*qy+rotMat[2]*qz);
        double Qy  = (rotMat[3]*qx+rotMat[4]*qy+rotMat[5]*qz); 
        double Qz  = (rotMat[6]*qx+rotMat[7]*qy+rotMat[8]*qz);

        double Qsq = Qx*Qx+Qy*Qy+Qz*Qz;
        if(Qsq < dim_min[0]||Qsq>=dim_max[0])return false;
        Coord[0]   = (coord_t)sqrt(Qsq);
        return true;

    }

    // should be actually on ICoordTransformer
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
    void setUpTransf(IConvertToMDEventsMethods *pConv){
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
    IConvertToMDEventsMethods *pHost;  
   // class which would convert units
     UnitsConverter<CONV,Type> CONV_UNITS_FROM;
 
};

}
} // End MDAlgorighms namespace
} // End Mantid namespace

#endif
