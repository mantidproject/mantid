#include "MantidMDEvents/MDTransfModQ.h"
#include "MantidKernel/RegistrationHelper.h"

namespace Mantid {
namespace MDEvents {
// register the class, whith conversion factory under ModQ name
DECLARE_MD_TRANSFID(MDTransfModQ, | Q | );

/**method calculates the units, the transformation expects the input ws to be
in. If the input ws is in different units,
the WS data will be converted into the requested units on the fly.
@param dEmode -- energy conversion mode requested by the user for the
transformation
@param inWS   -- input matrix workspace, the subject of transformation.
*/
const std::string
MDTransfModQ::inputUnitID(Kernel::DeltaEMode::Type dEmode,
                          API::MatrixWorkspace_const_sptr inWS) const {
  UNUSED_ARG(inWS);
  switch (dEmode) {
  case (Kernel::DeltaEMode::Elastic):
    return "Momentum";
  case (Kernel::DeltaEMode::Direct):
    return "DeltaE";
  case (Kernel::DeltaEMode::Indirect):
    return "DeltaE";
  default:
    throw(std::invalid_argument(" MDTransfModQ::inputUnitID: this class "
                                "supports only conversion in Elastic and "
                                "Inelastic energy transfer modes"));
  }
}

/** method returns number of matrix dimensions calculated by this class
*   as function of the energy analysis (conversion) mode
@param mode   -- energy conversion mode requested by the user for the
transformation
@param inWS   -- input matrix workspace, the subject of transformation.
*/
unsigned int
MDTransfModQ::getNMatrixDimensions(Kernel::DeltaEMode::Type mode,
                                   API::MatrixWorkspace_const_sptr inWS) const {
  UNUSED_ARG(inWS);
  switch (mode) {
  case (Kernel::DeltaEMode::Direct):
    return 2;
  case (Kernel::DeltaEMode::Indirect):
    return 2;
  case (Kernel::DeltaEMode::Elastic):
    return 1;
  default:
    throw(
        std::invalid_argument("Unknown or unsupported energy conversion mode"));
  }
}

/**Convert single point of matrix workspace into reciprocal space and
(optionally) modify signal and error
as function of reciprocal space (e.g. Lorents corrections)
@param x      -- the x-coordinate of matrix workspace. Often can be a time of
flight though the unit conversion is available
@param Coord -- converted MD coordinates of the point x calculated for
particular workspace position (detector)

@param signal -- the signal in the point
@param ErrSq -- the signal in the point
No signal or error transformation is performed by this particular method.

@return Coord -- the calculated coordinate of the point in the reciprocal space.

*/
bool MDTransfModQ::calcMatrixCoord(const double &x, std::vector<coord_t> &Coord,
                                   double &signal, double &ErrSq) const {
  UNUSED_ARG(signal);
  UNUSED_ARG(ErrSq);
  if (m_Emode == Kernel::DeltaEMode::Elastic) {
    return calcMatrixCoordElastic(x, Coord);
  } else {
    return calcMatrixCoordInelastic(x, Coord);
  }
}

/** Method fills-in all additional properties requested by user and not defined
*by matrix workspace itself.
*  it fills in [nd - (1 or 2 -- depending on emode)] values into Coord vector;
*
*@param Coord -- input-output vector of MD-coordinates
*@param nd    -- number of current dimensions
*
*@returns     -- Coord vector with nd-(1 or 2, depending on emode) values of MD
*coordinates
*/
bool MDTransfModQ::calcGenericVariables(std::vector<coord_t> &Coord,
                                        size_t nd) {
  // sanity check. If fails, something went fundamentally wrong
  if (m_NMatrixDim + m_AddDimCoordinates.size() != nd) {
    std::string ERR =
        "Number of matrix dimensions: " +
        boost::lexical_cast<std::string>(m_NMatrixDim) +
        " plus number of additional dimensions: " +
        boost::lexical_cast<std::string>(m_AddDimCoordinates.size()) +
        " not equal to number of workspace dimensions: " +
        boost::lexical_cast<std::string>(nd);
    throw(std::invalid_argument(ERR));
  }

  // in Elastic case, 1  coordinate (|Q|) came from workspace
  // in inelastic 2 coordinates (|Q| dE) came from workspace. All other are
  // defined by properties.
  // m_NMatrixDim is either 1 in elastic case or 2 in inelastic
  size_t ic(0);
  for (size_t i = m_NMatrixDim; i < nd; i++) {
    if (m_AddDimCoordinates[ic] < m_DimMin[i] ||
        m_AddDimCoordinates[ic] >= m_DimMax[i])
      return false;
    Coord[i] = m_AddDimCoordinates[ic];
    ic++;
  }
  return true;
}

/**
* Method updates the value of pre-processed detector coordinates in Q-space,
* used by other functions
* @param Coord
* @param i -- index of the detector, which corresponds to the spectra to
* process.
* @return FALSE if spectra should be excluded (if masked)
*/
bool MDTransfModQ::calcYDepCoordinates(std::vector<coord_t> &Coord, size_t i) {
  UNUSED_ARG(Coord);
  m_ex = (m_DetDirecton + i)->X();
  m_ey = (m_DetDirecton + i)->Y();
  m_ez = (m_DetDirecton + i)->Z();
  // if input energy changes on each detector (efixed, indirect mode only), then
  // set up its value
  if (m_pEfixedArray) {
    m_Ei = double(*(m_pEfixedArray + i));
    m_Ki = sqrt(m_Ei / PhysicalConstants::E_mev_toNeutronWavenumberSq);
  }
  // if spectra masked, this spectra should be excluded
  if (m_pDetMasks) {
    if (*(m_pDetMasks + i) > 0)
      return false;
  }
  return true;
}

/** function calculates workspace-dependent coordinates in inelastic case.
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
bool MDTransfModQ::calcMatrixCoordInelastic(const double &E_tr,
                                            std::vector<coord_t> &Coord) const {
  if (E_tr < m_DimMin[1] || E_tr >= m_DimMax[1])
    return false;
  Coord[1] = (coord_t)E_tr;
  double k_tr;
  // get module of the wavevector for scattered neutrons
  if (this->m_Emode == Kernel::DeltaEMode::Direct) {
    k_tr = sqrt((m_Ei - E_tr) / PhysicalConstants::E_mev_toNeutronWavenumberSq);
  } else {
    k_tr = sqrt((m_Ei + E_tr) / PhysicalConstants::E_mev_toNeutronWavenumberSq);
  }

  double qx = -m_ex * k_tr;
  double qy = -m_ey * k_tr;
  double qz = m_Ki - m_ez * k_tr;
  // transformation matrix has to be here for "Crystal AS Powder conversion
  // mode, further specialization possible if "powder" mode defined"
  double Qx = (m_RotMat[0] * qx + m_RotMat[1] * qy + m_RotMat[2] * qz);
  double Qy = (m_RotMat[3] * qx + m_RotMat[4] * qy + m_RotMat[5] * qz);
  double Qz = (m_RotMat[6] * qx + m_RotMat[7] * qy + m_RotMat[8] * qz);

  double Qsq = Qx * Qx + Qy * Qy + Qz * Qz;
  if (Qsq < m_DimMin[0] || Qsq >= m_DimMax[0])
    return false;
  Coord[0] = (coord_t)sqrt(Qsq);

  return true;
}
/** function calculates workspace-dependent coordinates in elastic case.
* Namely, it calculates module of Momentum transfer
* put it into specified (0) position in the Coord vector
*
*@param    k0   module of input momentum
*@param   &Coord  vector of MD coordinates with filled in momentum and energy
transfer

*@return   true if momentum is within the limits requested by the algorithm and
false otherwise.
*
* it uses preprocessed detectors positions, which are calculated by
PreprocessDetectors algorithm and set up by
* calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i) method. */
bool MDTransfModQ::calcMatrixCoordElastic(const double &k0,
                                          std::vector<coord_t> &Coord) const {

  double qx = -m_ex * k0;
  double qy = -m_ey * k0;
  double qz = (1 - m_ez) * k0;
  // transformation matrix has to be here for "Crystal AS Powder mode, further
  // specialization possible if powder mode is defined "
  double Qx = (m_RotMat[0] * qx + m_RotMat[1] * qy + m_RotMat[2] * qz);
  double Qy = (m_RotMat[3] * qx + m_RotMat[4] * qy + m_RotMat[5] * qz);
  double Qz = (m_RotMat[6] * qx + m_RotMat[7] * qy + m_RotMat[8] * qz);

  double Qsq = Qx * Qx + Qy * Qy + Qz * Qz;
  if (Qsq < m_DimMin[0] || Qsq >= m_DimMax[0])
    return false;
  Coord[0] = (coord_t)sqrt(Qsq);
  return true;
}
/** method returns the vector of input coordinates values where the transformed
 *coordinates reach its extremum values in Q or dE
 * direction.
 *
 * @param eMin -- minimal momentum (in elastic mode) or energy transfer (in
 *inelastic) for the transformation
 * @param eMax -- maximal momentum (in elastic mode) or energy transfer (in
 *inelastic) for the transformation
 * @param det_num -- number of the instrument detector for the transformation
 */
std::vector<double> MDTransfModQ::getExtremumPoints(const double eMin,
                                                    const double eMax,
                                                    size_t det_num) const {
  std::vector<double> rez(2);
  switch (m_Emode) {
  case (Kernel::DeltaEMode::Elastic): {
    rez[0] = eMin;
    rez[1] = eMax;
    return rez;
  }
  case (Kernel::DeltaEMode::Direct):
  case (Kernel::DeltaEMode::Indirect): {
    double ei = m_Ei;
    if (m_pEfixedArray)
      ei = double(*(m_pEfixedArray + det_num));

    double ez = (m_DetDirecton + det_num)->Z();
    double eps_extr = ei * (1 - ez * ez);
    if (eps_extr > eMin && eps_extr < eMax) {
      rez.resize(3);
      rez[0] = eMin;
      rez[1] = eps_extr;
      rez[2] = eMax;
    } else {
      rez[0] = eMin;
      rez[1] = eMax;
    }
    return rez;
  }
  default: {
    throw std::invalid_argument(
        "Undefined or unsupported energy conversion mode ");
  }
  }
  return rez;
}

/** function initializes all variables necessary for converting workspace
 * variables into MD variables in ModQ (elastic/inelastic) cases  */
void MDTransfModQ::initialize(const MDWSDescription &ConvParams) {
  //********** Generic part of initialization, common for elastic and inelastic
  // modes:
  //   pHost      = &Conv;
  // get transformation matrix (needed for CrystalAsPoder mode)
  m_RotMat = ConvParams.getTransfMatrix();
  m_pEfixedArray = NULL;
  if (!ConvParams.m_PreprDetTable)
    throw(std::runtime_error("The detectors have not been preprocessed but "
                             "they have to before running initialize"));

  // get pointer to the positions of the detectors
  std::vector<Kernel::V3D> const &DetDir =
      ConvParams.m_PreprDetTable->getColVector<Kernel::V3D>("DetDirections");
  m_DetDirecton = &DetDir[0]; //

  // get min and max values defined by the algorithm.
  ConvParams.getMinMax(m_DimMin, m_DimMax);
  // m_DimMin/max here are momentums and they are verified on momentum squared
  // base
  if (m_DimMin[0] < 0)
    m_DimMin[0] = 0;
  if (m_DimMax[0] < 0)
    m_DimMax[0] = 0;

  // m_DimMin here is a momentum and it is verified on momentum squared base
  m_DimMin[0] *= m_DimMin[0];
  m_DimMax[0] *= m_DimMax[0];
  if (std::fabs(m_DimMin[0] - m_DimMax[0]) < FLT_EPSILON ||
      m_DimMax[0] < m_DimMin[0]) {
    std::string ERR = "ModQ coordinate transformation: Min Q^2 value: " +
                      boost::lexical_cast<std::string>(m_DimMin[0]) +
                      " is more or equal then Max Q^2 value: " +
                      boost::lexical_cast<std::string>(m_DimMax[0]);
    throw(std::invalid_argument(ERR));
  }
  m_AddDimCoordinates = ConvParams.getAddCoord();

  //************   specific part of the initialization, dependent on emode:
  m_Emode = ConvParams.getEMode();
  m_NMatrixDim = getNMatrixDimensions(m_Emode);
  if (m_Emode == Kernel::DeltaEMode::Direct ||
      m_Emode == Kernel::DeltaEMode::Indirect) {
    // energy needed in inelastic case
    volatile double Ei =
        ConvParams.m_PreprDetTable->getLogs()->getPropertyValueAsType<double>(
            "Ei");
    m_Ei = Ei;
    if (Ei !=
        m_Ei) // Ei is NaN, try Efixed, but the value should be overridden later
    {
      try {
        m_Ei = ConvParams.m_PreprDetTable->getLogs()
                   ->getPropertyValueAsType<double>("eFixed");
      } catch (...) {
      }
    }

    // the wave vector of incident neutrons;
    m_Ki = sqrt(m_Ei / PhysicalConstants::E_mev_toNeutronWavenumberSq);

    m_pEfixedArray = NULL;
    if (m_Emode == (int)Kernel::DeltaEMode::Indirect)
      m_pEfixedArray =
          ConvParams.m_PreprDetTable->getColDataArray<float>("eFixed");
  } else if (m_Emode != Kernel::DeltaEMode::Elastic)
    throw(std::invalid_argument(
        "MDTransfModQ::initialize::Unknown energy conversion mode"));

  m_pDetMasks = ConvParams.m_PreprDetTable->getColDataArray<int>("detMask");
}
/**method returns default ID-s for ModQ elastic and inelastic modes. The ID-s
are related to the units,
* this class produces its output in.
*@param dEmode   -- energy conversion mode
*@param inWS -- Input Matrix workspace

*@return       -- vector of default dimension ID-s for correspondent energy
conversion mode.
The position of each dimID in the vector corresponds to the position of each MD
coordinate in the Coord vector
*/
std::vector<std::string>
MDTransfModQ::getDefaultDimID(Kernel::DeltaEMode::Type dEmode,
                              API::MatrixWorkspace_const_sptr inWS) const {
  UNUSED_ARG(inWS);
  std::vector<std::string> default_dim_ID;
  switch (dEmode) {
  case (Kernel::DeltaEMode::Elastic): {
    default_dim_ID.resize(1);
    break;
  }
  case (Kernel::DeltaEMode::Direct):
  case (Kernel::DeltaEMode::Indirect): {
    default_dim_ID.resize(2);
    default_dim_ID[1] = "DeltaE";
    break;
  }
  default:
    throw(std::invalid_argument(
        "MDTransfModQ::getDefaultDimID::Unknown energy conversion mode"));
  }
  default_dim_ID[0] = "|Q|";

  return default_dim_ID;
}

/**function returns units ID-s which this transformation produces its output.
* @param dEmode   -- energy conversion mode
* @param inWS -- Input Matrix workspace
*
* It is Momentum and DelteE in inelastic modes   */
std::vector<std::string>
MDTransfModQ::outputUnitID(Kernel::DeltaEMode::Type dEmode,
                           API::MatrixWorkspace_const_sptr inWS) const {
  UNUSED_ARG(inWS);
  std::vector<std::string> UnitID = this->getDefaultDimID(dEmode, inWS);
  // TODO: is it really momentum transfer, as MomentumTransfer units are seems
  // bound to elastic mode only (at least accorting to Units description on
  // Wiki)?
  if (dEmode == Kernel::DeltaEMode::Elastic) {
    UnitID[0] = "Momentum";
  } else {
    UnitID[0] = "MomentumTransfer";
  }
  return UnitID;
}

/// constructor;
MDTransfModQ::MDTransfModQ()
    : m_ex(0), m_ey(0), m_ez(1), m_DetDirecton(NULL), //,m_NMatrixDim(-1)
      m_NMatrixDim(0),                                // uninitialized
      m_Emode(Kernel::DeltaEMode::Undefined),         // uninitialized
      m_Ki(1.), m_Ei(1.), m_pEfixedArray(NULL), m_pDetMasks(NULL) {}

std::vector<std::string> MDTransfModQ::getEmodes() const {
  return Kernel::DeltaEMode().availableTypes();
}
} // End MDAlgorighms namespace
} // End Mantid namespace
