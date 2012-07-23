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
    unsigned int MDTransfQ3D::getNMatrixDimensions(CnvrtToMD::EModes mode,API::MatrixWorkspace_const_sptr inWS)const
    {
      UNUSED_ARG(inWS);
      switch(mode)
      {
      case(CnvrtToMD::Direct):  return 4;
      case(CnvrtToMD::Indir):   return 4;
      case(CnvrtToMD::Elastic): return 3;
      default: throw(std::invalid_argument("Unknow or unsupported energy conversion mode"));
      }
    }


    bool MDTransfQ3D::calcMatrixCoord(const double& x,std::vector<coord_t> &Coord)const
    {
      if(m_Emode == CnvrtToMD::Elastic)
      {
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
      if(Coord[3]<m_DimMin[3]||Coord[3]>=m_DimMax[3])return false;

      // get module of the wavevector for scattered neutrons
      double k_tr;
      if(m_Emode==CnvrtToMD::Direct)
      {
        k_tr=sqrt((m_Ei-E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      }else{
        k_tr=sqrt((m_Ei+E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      }


      double  qx  =  -m_ex*k_tr;                
      double  qy  =  -m_ey*k_tr;
      double  qz  = m_Ki - m_ez*k_tr;

      Coord[0]  = (coord_t)(m_RotMat[0]*qx+m_RotMat[1]*qy+m_RotMat[2]*qz);  if(Coord[0]<m_DimMin[0]||Coord[0]>=m_DimMax[0])return false;
      Coord[1]  = (coord_t)(m_RotMat[3]*qx+m_RotMat[4]*qy+m_RotMat[5]*qz);  if(Coord[1]<m_DimMin[1]||Coord[1]>=m_DimMax[1])return false;
      Coord[2]  = (coord_t)(m_RotMat[6]*qx+m_RotMat[7]*qy+m_RotMat[8]*qz);  if(Coord[2]<m_DimMin[2]||Coord[2]>=m_DimMax[2])return false;

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

      double  qx  =  -m_ex*k0;                
      double  qy  =  -m_ey*k0;
      double  qz  = (1-m_ez)*k0;

      Coord[0]  = (coord_t)(m_RotMat[0]*qx+m_RotMat[1]*qy+m_RotMat[2]*qz);  if(Coord[0]<m_DimMin[0]||Coord[0]>=m_DimMax[0])return false;
      Coord[1]  = (coord_t)(m_RotMat[3]*qx+m_RotMat[4]*qy+m_RotMat[5]*qz);  if(Coord[1]<m_DimMin[1]||Coord[1]>=m_DimMax[1])return false;
      Coord[2]  = (coord_t)(m_RotMat[6]*qx+m_RotMat[7]*qy+m_RotMat[8]*qz);  if(Coord[2]<m_DimMin[2]||Coord[2]>=m_DimMax[2])return false;

      return true;

    }

    /** function initalizes all variables necessary for converting workspace variables into MD variables in ModQ (elastic/inelastic) cases  */
    void MDTransfQ3D::initialize(const MDWSDescription &ConvParams)
    { 

      //********** Generic part of initialization, common for elastic and inelastic modes:
      // get transformation matrix (needed for CrystalAsPoder mode)
      m_RotMat = ConvParams.getTransfMatrix();

      // get pointer to the positions of the detectors
      std::vector<Kernel::V3D> const & DetDir = ConvParams.getDetectors()->getDetDir();
      m_Det = &DetDir[0];     //

      // get min and max values defined by the algorithm. 
      ConvParams.getMinMax(m_DimMin,m_DimMax);
      // get additional coordinates which are 
      m_AddDimCoordinates = ConvParams.getAddCoord();

      //************   specific part of the initialization, dependent on emode:
      m_Emode      = ConvParams.getEMode();
      m_NMatrixDim = getNMatrixDimensions(m_Emode);
      if(m_Emode == CnvrtToMD::Direct||m_Emode == CnvrtToMD::Indir)
      {
        // energy needed in inelastic case
        m_Ei  =  ConvParams.getEi();
        // the wave vector of incident neutrons;
        m_Ki=sqrt(m_Ei/PhysicalConstants::E_mev_toNeutronWavenumberSq); 
      }else{
        if (m_Emode != CnvrtToMD::Elastic) throw(std::invalid_argument("MDTransfModQ::initialize::Unknown or unsupported energy conversion mode"));
      }

    }
    /**method returns default ID-s for ModQ elastic and inelastic modes. The ID-s are related to the units, 
    * this class produces its ouptut in. 
    *@param Emode   -- energy conversion mode

    *@returns       -- vector of default dimension ID-s for correspondent energy conversion mode. 
    The position of each dimID in the vector corresponds to the position of each MD coordinate in the Coord vector
    */
    std::vector<std::string> MDTransfQ3D::getDefaultDimID(CnvrtToMD::EModes dEmode, API::MatrixWorkspace_const_sptr inWS)const
    {
      UNUSED_ARG(inWS);
      std::vector<std::string> default_dim_ID;
      switch(dEmode)
      {
      case(CnvrtToMD::Elastic):
        {
          default_dim_ID.resize(3);
          break;
        }
      case(CnvrtToMD::Direct):
      case(CnvrtToMD::Indir):
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
    std::vector<std::string> MDTransfQ3D::outputUnitID(CnvrtToMD::EModes dEmode, API::MatrixWorkspace_const_sptr inWS)const
    {
      UNUSED_ARG(inWS);
      std::vector<std::string> UnitID = this->getDefaultDimID(dEmode,inWS);

      //TODO: is it really momentum transfer, as MomentumTransfer units are seems bound to elastic mode only (at least accorting to Units description on Wiki)?
      std::string kUnits("MomentumTransfer");
      if(dEmode==CnvrtToMD::Elastic)kUnits= "Momentum";
     

      UnitID[0] = kUnits;
      UnitID[1] = kUnits;
      UnitID[2] = kUnits;
      return UnitID;
    }


    /// constructor;
    MDTransfQ3D::MDTransfQ3D()
    {}    

  } // End MDAlgorighms namespace
} // End Mantid namespace


