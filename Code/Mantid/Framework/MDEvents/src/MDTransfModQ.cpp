#include "MantidMDAlgorithms/MDTransfModQ.h"
#include "MantidKernel/RegistrationHelper.h"

namespace Mantid
{
namespace MDAlgorithms
{
// register the class, whith conversion factory
DECLARE_MD_TRANSF(MDTransfModQ);

const std::string MDTransfModQ::usedUnitID()const
{
    if (emode == ConvertToMD::Elastic){
        return "Momentum";
    }else{
        return "DeltaE";
    }
}

bool MDTransfModQ::calcMatrixCoord(const double& x,std::vector<coord_t> &Coord)const
{
    if(emode == ConvertToMD::Elastic){
        return calcMatrixCoordElastic(x,Coord);
    }else{
        return calcMatrixCoordInelastic(x,Coord);
    }
}
// function returns energy conversion modes supported by this class
std::vector<std::string> MDTransfModQ::getEmodes()const
{
    return std::vector<std::string>(this->Emodes.begin(),this->Emodes.end());
}
/// function returns  string presentation of emode
std::string MDTransfModQ::getEmode(ConvertToMD::EModes Mode)const
{
    return Emodes[Mode];
}



bool MDTransfModQ::calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)
{      
        // in Elastic case, 1  coordinate (|Q|) came from workspace
        // in inelastic 2 coordinates (|Q| dE) came from workspace. All other are defined by properties. 
        // nMatrixDim is either 1 in elastic case or 2 in inelastic
        if(!pHost->fillAddProperties(Coord,nd,nMatrixDim))return false;
        return true;
}
//
bool MDTransfModQ::calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i)
{
        UNUSED_ARG(Coord); 
        ex = (pDet+i)->X();
        ey = (pDet+i)->Y();
        ez = (pDet+i)->Z();
        return true;
}
/** function calculates workspace-dependent coordinates in inelastic case. 
  * Namely, it calculates module of Momentum transfer and Energy 
  * transfer and put them into specified positions in the Coord vector
  *
  *@Param     E_tr   input energy transfer
  *@returns   &Coord  vector of MD coordinates with filled in momentum and energy transfer 

  *&returns   true if all momentum and energy are within the limits requested by the algorithm and false otherwise. 
*/    
bool MDTransfModQ::calcMatrixCoordInelastic(const double& E_tr,std::vector<coord_t> &Coord)const
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
/** function calculates workspace-dependent coordinates in elastic case. 
  * Namely it calculates module of Momentum transfer 
  * and put it into requested position of the Coord vector
  *
  *@Param     E_tr   input energy transfer
  *@returns   &Coord  vector of MD coordinates with filled in momentum and energy transfer 

  *&returns   true if all momentum and energy are within the limits requested by the algorithm and false otherwise. 
*/    
bool MDTransfModQ::calcMatrixCoordElastic(const double& k0,std::vector<coord_t> &Coord)const
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

/** function initalizes all variables necessary for converting workspace variables into 
  *
  */
void MDTransfModQ::initialize(const ConvToMDEventsBase &Conv)
{ 
//********** Generic part of initialization, common for elastic and inelastic modes:
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


//************   specific part of the initialization, dependent on emode:
        emode      = (ConvertToMD::EModes)pHost->getEMode();
        if(emode == ConvertToMD::Direct||emode == ConvertToMD::Indir){
            nMatrixDim=2;
            // energy needed in inelastic case
            Ei  =  pHost->getEi();
            // the wave vector of incident neutrons;
            ki=sqrt(Ei/PhysicalConstants::E_mev_toNeutronWavenumberSq); 
        }else{
            if (emode == ConvertToMD::Elastic){
                nMatrixDim=1;
            }else{
                throw(std::invalid_argument("MDTransfModQ::ModQ::Unknown energy conversion mode"));
            }
        }
        
}


    // constructor;
MDTransfModQ::MDTransfModQ():
pDet(NULL),
pHost(NULL),
Emodes(ConvertToMD::No_DE,"")
{
    Emodes[ConvertToMD::Elastic]="Elastic";
    Emodes[ConvertToMD::Direct] ="Direct";
    Emodes[ConvertToMD::Indir]  ="Indirect";

}    

} // End MDAlgorighms namespace
} // End Mantid namespace
