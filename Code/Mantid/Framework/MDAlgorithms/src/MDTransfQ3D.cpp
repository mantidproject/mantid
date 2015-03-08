#include "MantidMDAlgorithms/MDTransfQ3D.h"
#include "MantidKernel/RegistrationHelper.h"

namespace Mantid {
namespace MDAlgorithms {
// register the class, whith conversion factory under Q3D name
DECLARE_MD_TRANSFID(MDTransfQ3D, Q3D);

/** method returns number of matrix dimensions calculated by this class
* as function of energy analysis mode   */
unsigned int
MDTransfQ3D::getNMatrixDimensions(Kernel::DeltaEMode::Type mode,
                                  API::MatrixWorkspace_const_sptr inWS) const {
  UNUSED_ARG(inWS);
  switch (mode) {
  case (Kernel::DeltaEMode::Direct):
    return 4;
  case (Kernel::DeltaEMode::Indirect):
    return 4;
  case (Kernel::DeltaEMode::Elastic):
    return 3;
  default:
    throw(
        std::invalid_argument("Unknow or unsupported energy conversion mode"));
  }
}

/** Calculates 3D transformation of the variable coordinates and (if applicable)
  signal and error depending on 3D coordinates
  * (e.g. Lorents corrections)
  *@param x -- the transformed values
  *@param Coord -- 3 or 4D coordinate of the resulting event
  *@param s -- the signal
  *@param err --the error
  *
  *@return Coord -- converted 3D coordinates corresponding to given detector and
  X-vale
  Optionally:
  @return s    -- Lorentz corrected signal
  @return err  -- Lorentz corrected error
*/
bool MDTransfQ3D::calcMatrixCoord(const double &x, std::vector<coord_t> &Coord,
                                  double &s, double &err) const {
  if (m_Emode == Kernel::DeltaEMode::Elastic) {
    return calcMatrixCoord3DElastic(x, Coord, s, err);
  } else {
    return calcMatrixCoord3DInelastic(x, Coord);
  }
}

/** method calculates workspace-dependent coordinates in inelastic case.
* Namely, it calculates module of Momentum transfer and the Energy
* transfer and put them into initial positions (0 and 1) in the Coord vector
*
*@param     E_tr   input energy transfer
*@param   &Coord  vector of MD coordinates with filled in momentum and energy
transfer

*@return   true if all momentum and energy are within the limits requested by
the algorithm and false otherwise.
*
* it also uses preprocessed detectors positions, which are calculated by
PreprocessDetectors algorithm and set up by
* calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i) method.    */
bool
MDTransfQ3D::calcMatrixCoord3DInelastic(const double &E_tr,
                                        std::vector<coord_t> &Coord) const {
  Coord[3] = (coord_t)E_tr;
  if (Coord[3] < m_DimMin[3] || Coord[3] >= m_DimMax[3])
    return false;

  // get module of the wavevector for scattered neutrons
  double k_tr;
  if (m_Emode == Kernel::DeltaEMode::Direct) {
    k_tr = sqrt((m_Ei - E_tr) / PhysicalConstants::E_mev_toNeutronWavenumberSq);
  } else {
    k_tr = sqrt((m_Ei + E_tr) / PhysicalConstants::E_mev_toNeutronWavenumberSq);
  }

  double qx = -m_ex * k_tr;
  double qy = -m_ey * k_tr;
  double qz = m_Ki - m_ez * k_tr;

  Coord[0] = (coord_t)(m_RotMat[0] * qx + m_RotMat[1] * qy + m_RotMat[2] * qz);
  if (Coord[0] < m_DimMin[0] || Coord[0] >= m_DimMax[0])
    return false;
  Coord[1] = (coord_t)(m_RotMat[3] * qx + m_RotMat[4] * qy + m_RotMat[5] * qz);
  if (Coord[1] < m_DimMin[1] || Coord[1] >= m_DimMax[1])
    return false;
  Coord[2] = (coord_t)(m_RotMat[6] * qx + m_RotMat[7] * qy + m_RotMat[8] * qz);
  if (Coord[2] < m_DimMin[2] || Coord[2] >= m_DimMax[2])
    return false;

  return true;
}
/** function calculates workspace-dependent coordinates in elastic case.
* Namely, it calculates module of Momentum transfer
* put it into specified (0) position in the Coord vector
*
*@param   k0   module of input momentum
*@param   &Coord  vector of MD coordinates with filled in momentum and energy
transfer
*@param   signal signal
*@param   errSq error squared

*@return   true if momentum is within the limits requested by the algorithm and
false otherwise.
*
* it uses preprocessed detectors positions, which are calculated by
PreprocessDetectors algorithm and set up by
* calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i) method. */
bool MDTransfQ3D::calcMatrixCoord3DElastic(const double &k0,
                                           std::vector<coord_t> &Coord,
                                           double &signal,
                                           double &errSq) const {

  double qx = -m_ex * k0;
  double qy = -m_ey * k0;
  double qz = (1 - m_ez) * k0;

  Coord[0] = (coord_t)(m_RotMat[0] * qx + m_RotMat[1] * qy + m_RotMat[2] * qz);
  if (Coord[0] < m_DimMin[0] || Coord[0] >= m_DimMax[0])
    return false;

  Coord[1] = (coord_t)(m_RotMat[3] * qx + m_RotMat[4] * qy + m_RotMat[5] * qz);
  if (Coord[1] < m_DimMin[1] || Coord[1] >= m_DimMax[1])
    return false;

  Coord[2] = (coord_t)(m_RotMat[6] * qx + m_RotMat[7] * qy + m_RotMat[8] * qz);
  if (Coord[2] < m_DimMin[2] || Coord[2] >= m_DimMax[2])
    return false;

  /*Apply Lorentz corrections if necessary */
  if (m_isLorentzCorrected) {
    double kdash = k0 / (2 * M_PI);
    double correct = m_SinThetaSq * kdash * kdash * kdash * kdash;
    signal *= correct;
    errSq *= (correct * correct);
  }

  return true;
}

std::vector<double> MDTransfQ3D::getExtremumPoints(const double xMin,
                                                   const double xMax,
                                                   size_t det_num) const {
  UNUSED_ARG(det_num);

  std::vector<double> rez(2);
  rez[0] = xMin;
  rez[1] = xMax;

  return rez;
}

/** Method updates the value of preprocessed detector coordinates in Q-space,
*used by other functions
* @param Coord -- vector of MD coordinates with filled in momentum and energy
*transfer
* @param i -- index of the detector, which corresponds to the spectra to
*process.
*
*/
bool MDTransfQ3D::calcYDepCoordinates(std::vector<coord_t> &Coord, size_t i) {
  UNUSED_ARG(Coord);
  m_ex = (m_DetDirecton + i)->X();
  m_ey = (m_DetDirecton + i)->Y();
  m_ez = (m_DetDirecton + i)->Z();
  // if Lorentz-corrected, retrieve the sin(Theta)^2 for the detector;
  if (m_isLorentzCorrected)
    m_SinThetaSq = *(m_SinThetaSqArray + i);
  // if input energy changes on each detector (efixed, indirect mode only), then
  // set up its value
  if (m_pEfixedArray) {
    m_Ei = double(*(m_pEfixedArray + i));
    m_Ki = sqrt(m_Ei / PhysicalConstants::E_mev_toNeutronWavenumberSq);
  }
  // if masks are defined and detector masked -- no further calculations
  if (m_pDetMasks) {
    if (*(m_pDetMasks + i) > 0)
      return false;
  }

  return true;
}

/** function initalizes all variables necessary for converting workspace
 * variables into MD variables in ModQ (elastic/inelastic) cases  */
void MDTransfQ3D::initialize(const MDWSDescription &ConvParams) {
  m_pEfixedArray = NULL;
  m_pDetMasks = NULL;
  //********** Generic part of initialization, common for elastic and inelastic
  // modes:
  // get transformation matrix (needed for CrystalAsPoder mode)
  m_RotMat = ConvParams.getTransfMatrix();

  if (!ConvParams.m_PreprDetTable)
    throw(std::runtime_error("The detectors have not been preprocessed but "
                             "they have to before running initialize"));
  // get pointer to the positions of the preprocessed detectors
  std::vector<Kernel::V3D> const &DetDir =
      ConvParams.m_PreprDetTable->getColVector<Kernel::V3D>("DetDirections");
  m_DetDirecton = &DetDir[0]; //

  // get min and max values defined by the algorithm.
  ConvParams.getMinMax(m_DimMin, m_DimMax);
  // get additional coordinates which are
  m_AddDimCoordinates = ConvParams.getAddCoord();

  //************   specific part of the initialization, dependent on emode:
  m_Emode = ConvParams.getEMode();
  m_NMatrixDim = getNMatrixDimensions(m_Emode);
  if (m_Emode == Kernel::DeltaEMode::Direct ||
      m_Emode == Kernel::DeltaEMode::Indirect) {
    // energy needed in inelastic case
    m_Ei =
        ConvParams.m_PreprDetTable->getLogs()->getPropertyValueAsType<double>(
            "Ei");
    // the wave vector of incident neutrons;
    m_Ki = sqrt(m_Ei / PhysicalConstants::E_mev_toNeutronWavenumberSq);

    m_pEfixedArray = NULL;
    if (m_Emode == (int)Kernel::DeltaEMode::Indirect)
      m_pEfixedArray =
          ConvParams.m_PreprDetTable->getColDataArray<float>("eFixed");
  } else {
    if (m_Emode != Kernel::DeltaEMode::Elastic)
      throw(std::runtime_error("MDTransfQ3D::initialize::Unknown or "
                               "unsupported energy conversion mode"));
    // check if we need to calculate Lorentz corrections and if we do, prepare
    // values for their precalculation:
    m_isLorentzCorrected = ConvParams.isLorentsCorrections();
    if (m_isLorentzCorrected) {
      auto &TwoTheta =
          ConvParams.m_PreprDetTable->getColVector<double>("TwoTheta");
      SinThetaSq.resize(TwoTheta.size());
      for (size_t i = 0; i < TwoTheta.size(); i++) {
        double sth = sin(0.5 * TwoTheta[i]);
        SinThetaSq[i] = sth * sth;
      }
      m_SinThetaSqArray = &SinThetaSq[0];
      if (!m_SinThetaSqArray)
        throw(std::runtime_error("MDTransfQ3D::initialize::Uninitilized "
                                 "Sin(Theta)^2 array for calculating Lorentz "
                                 "corrections"));
    }
  }
  // use detectors masks untill signals are masked by 0 instead of NaN
  m_pDetMasks = ConvParams.m_PreprDetTable->getColDataArray<int>("detMask");
}
/**method returns default ID-s for ModQ elastic and inelastic modes. The ID-s
are related to the units,
* this class produces its ouptut in.
*@param dEmode   -- energy conversion mode
*@param inWS -- Input workspace
*@return       -- vector of default dimension ID-s for correspondent energy
conversion mode.
The position of each dimID in the vector corresponds to the position of each MD
coordinate in the Coord vector
*/
std::vector<std::string>
MDTransfQ3D::getDefaultDimID(Kernel::DeltaEMode::Type dEmode,
                             API::MatrixWorkspace_const_sptr inWS) const {
  UNUSED_ARG(inWS);
  std::vector<std::string> default_dim_ID;
  switch (dEmode) {
  case (Kernel::DeltaEMode::Elastic): {
    default_dim_ID.resize(3);
    break;
  }
  case (Kernel::DeltaEMode::Direct):
  case (Kernel::DeltaEMode::Indirect): {
    default_dim_ID.resize(4);
    default_dim_ID[3] = "DeltaE";
    break;
  }
  default:
    throw(std::invalid_argument(
        "MDTransfQ3D::getDefaultDimID::Unknown energy conversion mode"));
  }
  default_dim_ID[0] = "Q1";
  default_dim_ID[1] = "Q2";
  default_dim_ID[2] = "Q3";

  return default_dim_ID;
}

/**function returns units ID-s which this transformation prodiuces its ouptut.
* @param dEmode   -- energy conversion mode
* @param inWS -- input workspace
* @return
* It is Momentum and DelteE in inelastic modes   */
std::vector<std::string>
MDTransfQ3D::outputUnitID(Kernel::DeltaEMode::Type dEmode,
                          API::MatrixWorkspace_const_sptr inWS) const {
  UNUSED_ARG(inWS);
  std::vector<std::string> UnitID = this->getDefaultDimID(dEmode, inWS);

  // TODO: is it really momentum transfer, as MomentumTransfer units are seems
  // bound to elastic mode only (at least accorting to Units description on
  // Wiki)?
  std::string kUnits("MomentumTransfer");
  if (dEmode == Kernel::DeltaEMode::Elastic)
    kUnits = "Momentum";

  UnitID[0] = kUnits;
  UnitID[1] = kUnits;
  UnitID[2] = kUnits;
  return UnitID;
}

/// constructor;
MDTransfQ3D::MDTransfQ3D() : m_isLorentzCorrected(false) {}

} // End MDAlgorighms namespace
} // End Mantid namespace
