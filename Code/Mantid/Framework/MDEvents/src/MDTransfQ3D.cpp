#include "MantidMDEvents/MDTransfQ3D.h"
#include "MantidKernel/RegistrationHelper.h"

namespace Mantid
{
namespace MDEvents
{
// register the class, whith conversion factory under ModQ name
DECLARE_MD_TRANSFID(MDTransfQ3D,Q3D);

/** method returns number of matrix dimensions calculated by this class
  * as function of energy analysis mode   */
unsigned int MDTransfQ3D::getNMatrixDimensions(ConvertToMD::EModes mode,API::MatrixWorkspace_const_sptr inWS)const
{
   UNUSED_ARG(inWS);
   switch(mode)
   {
   case(ConvertToMD::Direct):  return 4;
   case(ConvertToMD::Indir):   return 4;
   case(ConvertToMD::Elastic): return 3;
   default: throw(std::invalid_argument("Unknow or unsupported energy conversion mode"));
   }
}


bool MDTransfQ3D::calcMatrixCoord(const double& x,std::vector<coord_t> &Coord)const
{
    if(emode == ConvertToMD::Elastic){
        return calcMatrixCoord3DElastic(x,Coord);
    }else{
        return calcMatrixCoord3DInelastic(x,Coord);
    }
}

/** method calculates workspace-dependent coordinates in inelastic case. 
  * Namely, it calculates module of Momentum transfer and the Energy 
  * transfer and put them into initial positions (0 and 1) in the Coord vector
  *
  *@Param     E_tr   input energy transfer
  *@returns   &Coord  vector of MD coordinates with filled in momentum and energy transfer 

  *@returns   true if all momentum and energy are within the limits requested by the algorithm and false otherwise. 
  *
  * it also uses preprocessed detectors positions, which are calculated by PreprocessDetectors algorithm and set up by 
  * calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i) method.    */    
bool MDTransfQ3D::calcMatrixCoord3DInelastic(const double& E_tr,std::vector<coord_t> &Coord)const
{
      Coord[3]    = (coord_t)E_tr;
      if(Coord[3]<dim_min[3]||Coord[3]>=dim_max[3])return false;

      // get module of the wavevector for scattered neutrons
      double k_tr;
      if(this->emode==ConvertToMD::Direct){
          k_tr=sqrt((Ei-E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      }else{
          k_tr=sqrt((Ei+E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      }


      double  qx  =  -ex*k_tr;                
      double  qy  =  -ey*k_tr;
      double  qz  = ki - ez*k_tr;

      Coord[0]  = (coord_t)(rotMat[0]*qx+rotMat[1]*qy+rotMat[2]*qz);  if(Coord[0]<dim_min[0]||Coord[0]>=dim_max[0])return false;
      Coord[1]  = (coord_t)(rotMat[3]*qx+rotMat[4]*qy+rotMat[5]*qz);  if(Coord[1]<dim_min[1]||Coord[1]>=dim_max[1])return false;
      Coord[2]  = (coord_t)(rotMat[6]*qx+rotMat[7]*qy+rotMat[8]*qz);  if(Coord[2]<dim_min[2]||Coord[2]>=dim_max[2])return false;

      return true;


}
/** function calculates workspace-dependent coordinates in elastic case. 
  * Namely, it calculates module of Momentum transfer
  * put it into specified (0) position in the Coord vector
  *
  *@Param     k0   module of input momentum
  *@returns   &Coord  vector of MD coordinates with filled in momentum and energy transfer 

  *@returns   true if momentum is within the limits requested by the algorithm and false otherwise. 
  *
  * it uses preprocessed detectors positions, which are calculated by PreprocessDetectors algorithm and set up by 
  * calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i) method. */    
bool MDTransfQ3D::calcMatrixCoord3DElastic(const double& k0,std::vector<coord_t> &Coord)const
{
   
         double  qx  =  -ex*k0;                
         double  qy  =  -ey*k0;
         double  qz  = (1-ez)*k0;

         Coord[0]  = (coord_t)(rotMat[0]*qx+rotMat[1]*qy+rotMat[2]*qz);  if(Coord[0]<dim_min[0]||Coord[0]>=dim_max[0])return false;
         Coord[1]  = (coord_t)(rotMat[3]*qx+rotMat[4]*qy+rotMat[5]*qz);  if(Coord[1]<dim_min[1]||Coord[1]>=dim_max[1])return false;
         Coord[2]  = (coord_t)(rotMat[6]*qx+rotMat[7]*qy+rotMat[8]*qz);  if(Coord[2]<dim_min[2]||Coord[2]>=dim_max[2])return false;

         return true;

}

/** function initalizes all variables necessary for converting workspace variables into MD variables in ModQ (elastic/inelastic) cases  */
void MDTransfQ3D::initialize(const MDWSDescription &ConvParams)
{ 

//********** Generic part of initialization, common for elastic and inelastic modes:
         // get transformation matrix (needed for CrystalAsPoder mode)
        rotMat = ConvParams.getTransfMatrix();

        // get pointer to the positions of the detectors
        std::vector<Kernel::V3D> const & DetDir = ConvParams.getDetectors()->getDetDir();
        pDet = &DetDir[0];     //

        // get min and max values defined by the algorithm. 
        ConvParams.getMinMax(dim_min,dim_max);
        // get additional coordinates which are 
        this->addDimCoordinates = ConvParams.getAddCoord();

//************   specific part of the initialization, dependent on emode:
        emode      = ConvParams.getEMode();
        nMatrixDim = getNMatrixDimensions(emode);
        if(emode == ConvertToMD::Direct||emode == ConvertToMD::Indir){
            // energy needed in inelastic case
            Ei  =  ConvParams.getEi();
            // the wave vector of incident neutrons;
            ki=sqrt(Ei/PhysicalConstants::E_mev_toNeutronWavenumberSq); 
        }else{
            if (emode != ConvertToMD::Elastic){
                throw(std::invalid_argument("MDTransfModQ::initialize::Unknown or unsupported energy conversion mode"));
            }
        }
        
}
/**method returns default ID-s for ModQ elastic and inelastic modes. The ID-s are related to the units, 
  * this class produces its ouptut in. 
 *@param Emode   -- energy conversion mode

 *@returns       -- vector of default dimension ID-s for correspondent energy conversion mode. 
                    The position of each dimID in the vector corresponds to the position of each MD coordinate in the Coord vector
*/
std::vector<std::string> MDTransfQ3D::getDefaultDimID(ConvertToMD::EModes dEmode, API::MatrixWorkspace_const_sptr inWS)const
{
    UNUSED_ARG(inWS);
    std::vector<std::string> default_dim_ID;
    switch(dEmode)
    {
    case(ConvertToMD::Elastic):
        {
            default_dim_ID.resize(3);
            break;
        }
    case(ConvertToMD::Direct):
    case(ConvertToMD::Indir):
        {
            default_dim_ID.resize(4);
            default_dim_ID[3]= "DeltaE";
            break;
        }
    default:
        throw(std::invalid_argument("MDTransfQ3D::getDefaultDimID::Unknown energy conversion mode"));
    }
    default_dim_ID[0]="Q1";
    default_dim_ID[1]="Q2";
    default_dim_ID[2]="Q3";

    return default_dim_ID;
}


/**function returns units ID-s which this transformation prodiuces its ouptut. 
 * @param Emode   -- energy conversion mode
 *
 * It is Momentum and DelteE in inelastic modes   */
std::vector<std::string> MDTransfQ3D::outputUnitID(ConvertToMD::EModes dEmode, API::MatrixWorkspace_const_sptr inWS)const
{
    UNUSED_ARG(inWS);
    std::vector<std::string> UnitID = MDTransfModQ::getDefaultDimID(dEmode,inWS);
    //TODO: is it really momentum transfer, as MomentumTransfer units are seems bound to elastic mode only (at least accorting to Units description on Wiki)?
    UnitID[0] = "MomentumTransfer";
    UnitID[1] = "MomentumTransfer";
    UnitID[2] = "MomentumTransfer";
    return UnitID;
}


/// constructor;
MDTransfQ3D::MDTransfQ3D():
pDet(NULL),
nMatrixDim(-1)
{
}    

} // End MDAlgorighms namespace
} // End Mantid namespace


