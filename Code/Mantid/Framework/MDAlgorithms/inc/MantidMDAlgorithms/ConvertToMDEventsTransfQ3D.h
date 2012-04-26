#ifndef  H_CONVERT_TO_MDEVENTS_Q3D_TRANSF
#define  H_CONVERT_TO_MDEVENTS_Q3D_TRANSF
//
#include "MantidMDAlgorithms/ConvertToMDEventsTransfInterface.h"
//
namespace Mantid
{
namespace MDAlgorithms
{
//namespace ConvertToMD
//{
/** Set of internal classes used by ConvertToMDEvents algorithm and responsible for conversion of input workspace 
  * data into from 1 to 4 output dimensions as function of input parameters
  *
  *  This file defines  specializations of generic coordinate transformation templated to the 3D momentum conversion case, 
  *  Direct/Indirect tramsformatiom. 
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
#ifndef EXCLUDE_Q_TRANSFORMATION_Q3D
template<ConvertToMD::AnalMode MODE,ConvertToMD::CnvrtUnits CONV,ConvertToMD::XCoordType TYPE,ConvertToMD::SampleType SAMPLE> 
struct CoordTransformer<ConvertToMD::Q3D,MODE,CONV,TYPE,SAMPLE>
{
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)
    {
        // four inital properties came from workspace and all are interconnnected all additional defined by  properties:
        if(!pHost->fillAddProperties(Coord,nd,4))return false;
        // energy 
         Ei  =  pHost->getEi();
         // the wave vector of incident neutrons;
         ki=sqrt(Ei/PhysicalConstants::E_mev_toNeutronWavenumberSq); 
         // 
         rotMat = pHost->getTransfMatrix();
         //
         const Kernel::Unit_sptr pThisUnit= pHost->getAxisUnits();          
         CONV_UNITS_FROM.setUpConversion(*(pHost->getDetectors()),pThisUnit->unitID(),"DeltaE"); 

         pHost->getMinMax(dim_min,dim_max);
     // get pointer to the positions of the detectors
          std::vector<Kernel::V3D> const & DetDir = pHost->pPrepDetectors()->getDetDir();
          pDet = &DetDir[0];
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
         Coord[3]    = (coord_t)E_tr;
         if(Coord[3]<dim_min[3]||Coord[3]>=dim_max[3])return false;

         // get module of the wavevector for scattered neutrons
         double k_tr = k_trans<MODE>(Ei,E_tr);
   
         double  qx  =  -ex*k_tr;                
         double  qy  =  -ey*k_tr;
         double  qz  = ki - ez*k_tr;

         Coord[0]  = (coord_t)(rotMat[0]*qx+rotMat[1]*qy+rotMat[2]*qz);  if(Coord[0]<dim_min[0]||Coord[0]>=dim_max[0])return false;
         Coord[1]  = (coord_t)(rotMat[3]*qx+rotMat[4]*qy+rotMat[5]*qz);  if(Coord[1]<dim_min[1]||Coord[1]>=dim_max[1])return false;
         Coord[2]  = (coord_t)(rotMat[6]*qx+rotMat[7]*qy+rotMat[8]*qz);  if(Coord[2]<dim_min[2]||Coord[2]>=dim_max[2])return false;

         return true;
    }
    // should be actually on ICoordTransformer
    inline bool calcMatrixCoord(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord)const
    {
       UNUSED_ARG(i);
       double X_ev = CONV_UNITS_FROM.getXConverted(X,j);

       return calc1MatrixCoord(X_ev,Coord);
    }

    // calculate the transformations using units conversion
    inline bool convertAndCalcMatrixCoord(const double & X,std::vector<coord_t> &Coord)const
    {
         double X_ev = CONV_UNITS_FROM.getXConverted(X);
         return calc1MatrixCoord(X_ev,Coord);
    }   

   // default constructor
   CoordTransformer():pDet(NULL),pHost(NULL){}
   // initiator
   void setUpTransf(IConvertToMDEventsWS *pConv){
        pHost = pConv;
    }
private:
    // the energy of the incident neutrons
    double Ei;
    // the wavevector of incident neutrons
    double ki;
    // directions to the detectors 
    double ex,ey,ez;
    // the matrix which transforms the neutron momentums from lablratory to orthogonal crystall coordinate system. 
    std::vector<double> rotMat;
    // pointer to the detectors directions
    Kernel::V3D const *pDet;
    // Calling Mantid algorithm
    IConvertToMDEventsWS *pHost;
    // min and max values for this conversion
     std::vector<double> dim_min,dim_max;
   // class which would convert units
    UnitsConverter<CONV,TYPE> CONV_UNITS_FROM;
    
};

// Elastic
template<ConvertToMD::CnvrtUnits CONV,ConvertToMD::XCoordType TYPE,ConvertToMD::SampleType SAMPLE> 
struct CoordTransformer<ConvertToMD::Q3D,ConvertToMD::Elastic,CONV,TYPE,SAMPLE>
{
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)
    {
        // three inital coordinates came from workspace and all are interconnnected. All additional are defined by properties:
        if(!pHost->fillAddProperties(Coord,nd,3))return false;
         // 
        rotMat = pHost->getTransfMatrix();
        const Kernel::Unit_sptr pThisUnit= pHost->getAxisUnits();          
        CONV_UNITS_FROM.setUpConversion(*(pHost->getDetectors()),pThisUnit->unitID(),"Momentum"); 
        // get pointer to the positions of the detectors
         pDet = &((pHost->pPrepDetectors()->getDetDir())[0]);
    
         pHost->getMinMax(dim_min,dim_max);
         return true;
    }
    //
    inline bool calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i)
    {
          UNUSED_ARG(Coord); 
          CONV_UNITS_FROM.updateConversion(i);

          ex = (pDet+i)->X();
          ey = (pDet+i)->Y();
          ezm1 =(1-(pDet+i)->Z());
          return true;
    }

    inline bool calc1MatrixCoord(const double& k0,std::vector<coord_t> &Coord)const
    {
         double  qx  =  -ex*k0;                
         double  qy  =  -ey*k0;
         double  qz  = ezm1*k0;

         Coord[0]  = (coord_t)(rotMat[0]*qx+rotMat[1]*qy+rotMat[2]*qz);  if(Coord[0]<dim_min[0]||Coord[0]>=dim_max[0])return false;
         Coord[1]  = (coord_t)(rotMat[3]*qx+rotMat[4]*qy+rotMat[5]*qz);  if(Coord[1]<dim_min[1]||Coord[1]>=dim_max[1])return false;
         Coord[2]  = (coord_t)(rotMat[6]*qx+rotMat[7]*qy+rotMat[8]*qz);  if(Coord[2]<dim_min[2]||Coord[2]>=dim_max[2])return false;

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

    CoordTransformer():pDet(NULL),pHost(NULL){}
    void setUpTransf(IConvertToMDEventsWS *pConv){
        pHost = pConv;
    }
private:
    // directions to the detectors 
    double ex,ey,ezm1;
    // the matrix which transforms the neutron momentums from lablratory to crystall coordinate system. 
    std::vector<double> rotMat;
    // pointer to the beginning of the array with the detectors directions in Q-space
    Kernel::V3D const *pDet;
    // pointer to the algoritm, which calls all these transformations
    IConvertToMDEventsWS *pHost;  
    // min and max values for this conversion
     std::vector<double> dim_min,dim_max;
     // class which would convert units
    UnitsConverter<CONV,TYPE> CONV_UNITS_FROM;
 
};
#endif
//} // namespace ConvertToMD
} // End MDAlgorighms namespace
} // End Mantid namespace

#endif
