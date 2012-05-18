#include "MantidMDEvents/MDTransfModQ.h"
#include "MantidKernel/RegistrationHelper.h"

namespace Mantid
{
namespace MDEvents
{
// register the function, which adds 
DECLARE_MD_TRANSF(MDTransfModQElastic);
DECLARE_MD_TRANSF(MDTransfModQInelastic);

//**********************************************************************************************************************
//***************************************************   ELASTIC   ******************************************************
//**********************************************************************************************************************

bool MDTransfModQElastic::calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)
{      
        // in Elastic case, 1  coordinate (|Q|) came from workspace
        // in inelastic 2 coordinates (|Q| dE) came from workspace. All other are defined by properties. 
        // nMatrixDim is either 1 in elastic case or 2 in inelastic
        if(!pHost->fillAddProperties(Coord,nd,nMatrixDim))return false;
        return true;
}
//
bool MDTransfModQElastic::calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i)
{
        UNUSED_ARG(Coord); 
        ex = (pDet+i)->X();
        ey = (pDet+i)->Y();
        ez = (pDet+i)->Z();
        return true;
}
//
bool MDTransfModQElastic::calcMatrixCoord(const double& k0,std::vector<coord_t> &Coord)const
{
   
        double  qx  =  -ex*k0;                
        double  qy  =  -ey*k0;       
        double  qz  = (1 - ez)*k0;
        // transformation matrix has to be here for "Crystal AS Powder mode, further specialization possible if powder mode is defined "
        double Qx  = (rotMat[0]*qx+rotMat[1]*qy+rotMat[2]*qz);
        double Qy  = (rotMat[3]*qx+rotMat[4]*qy+rotMat[5]*qz); 
        double Qz  = (rotMat[6]*qx+rotMat[7]*qy+rotMat[8]*qz);

        double Qsq = Qx*Qx+Qy*Qy+Qz*Qz;
        if(Qsq < dim_min[0]||Qsq>=dim_max[0])return false;
        Coord[0]   = (coord_t)sqrt(Qsq);
        return true;

}

void MDTransfModQElastic::initialize(const ConvToMDEventsBase &Conv)
{      
        pHost      = &Conv;
        // get transformation matrix (needed for CrystalAsPoder mode)
        rotMat = pHost->getTransfMatrix();

        // get pointer to the positions of the detectors
        std::vector<Kernel::V3D> const & DetDir = pHost->pPrepDetectors()->getDetDir();
        pDet = &DetDir[0];     //
        //
        pHost->getMinMax(dim_min,dim_max);
         // dim_min/max here are momentums and they are verified on momentum squared base         
        if(dim_min[0]<0)dim_min[0]=0;
        if(dim_max[0]<0)dim_max[0]=0;

         // dim_min here is a momentum and it is verified on momentum squared base
         dim_min[0]*=dim_min[0];
         dim_max[0]*=dim_max[0];
         if(std::fabs(dim_min[0]-dim_max[0])<FLT_EPSILON||dim_max[0]<dim_min[0])
         {
            std::string ERR = "ModQ coordinate transformation: Min Q^2 value: "+boost::lexical_cast<std::string>(dim_min[0])+
                              " is more or equal then Max Q^2 value: "+boost::lexical_cast<std::string>(dim_max[0]);
            throw(std::invalid_argument(ERR));
         }

}

//**********************************************************************************************************************
//***************************************************   INELASTIC  *****************************************************
//**********************************************************************************************************************
    
bool MDTransfModQInelastic::calcMatrixCoord(const double& E_tr,std::vector<coord_t> &Coord)const
{
        if(E_tr<dim_min[1]||E_tr>=dim_max[1])return false;
        Coord[1]    =(coord_t)E_tr;
        double k_tr;
        // get module of the wavevector for scattered neutrons
        if(this->emode==ConvertToMD::Direct){
            k_tr=sqrt((Ei-E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
        }else{
            k_tr=sqrt((Ei+E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
        }
   
        double  qx  =  -ex*k_tr;                
        double  qy  =  -ey*k_tr;       
        double  qz  = ki - ez*k_tr;
        // transformation matrix has to be here for "Crystal AS Powder conversion mode, further specialization possible if "powder" mode defined"
        double Qx  = (rotMat[0]*qx+rotMat[1]*qy+rotMat[2]*qz);
        double Qy  = (rotMat[3]*qx+rotMat[4]*qy+rotMat[5]*qz); 
        double Qz  = (rotMat[6]*qx+rotMat[7]*qy+rotMat[8]*qz);

        double Qsq = Qx*Qx+Qy*Qy+Qz*Qz;
        if(Qsq < dim_min[0]||Qsq>=dim_max[0])return false;
        Coord[0]   = (coord_t)sqrt(Qsq);

        return true;

}

void MDTransfModQInelastic::initialize(const ConvToMDEventsBase &Conv)
{ 
        MDTransfModQElastic::initialize(Conv);

        emode      = (ConvertToMD::EModes)pHost->getEMode();
       // energy needed in inelastic case
        Ei  =  pHost->getEi();
       // the wave vector of incident neutrons;
        ki=sqrt(Ei/PhysicalConstants::E_mev_toNeutronWavenumberSq); 
}

} // End MDAlgorighms namespace
} // End Mantid namespace
