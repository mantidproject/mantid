#include "MantidMDEvents/MDTransfNoQ.h"
//
namespace Mantid {
namespace MDEvents {

// register the class, whith conversion factory under NoQ name
DECLARE_MD_TRANSFID(MDTransfNoQ, CopyToMD)

/** Method fills-in all additional properties requested by user and not defined
*by matrix workspace itselt.
*  it fills in [nd - (1 or 2 -- depending on input ws)] values into the Coord
*vector;
*
*@param Coord -- input-output vector of MD-coordinates
*@param nd    -- number of current dimensions
*
*@returns     -- Coord vector with nd-(1 or 2, depending on input ws) values of
*MD coordinates
*/
bool MDTransfNoQ::calcGenericVariables(std::vector<coord_t> &Coord, size_t nd) {
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
  // if one axis is numeric, 1  coordinate  came from workspace
  // if two axis are numeric, two values come from the ws. All other are defined
  // by properties.
  unsigned int ic(0);
  for (unsigned int i = m_NMatrixDim; i < nd; i++) {
    if (m_AddDimCoordinates[ic] < m_DimMin[i] ||
        m_AddDimCoordinates[ic] >= m_DimMax[i])
      return false;
    Coord[i] = m_AddDimCoordinates[ic];
    ic++;
  }
  return true;
}
void MDTransfNoQ::initialize(const MDWSDescription &ConvParams) {

  // get pointer to the positions of the detectors
  std::vector<Kernel::V3D> const &DetDir =
      ConvParams.m_PreprDetTable->getColVector<Kernel::V3D>("DetDirections");
  m_Det = &DetDir[0]; //

  // get min and max values defined by the algorithm.
  ConvParams.getMinMax(m_DimMin, m_DimMax);

  m_NMatrixDim =
      getNMatrixDimensions(Kernel::DeltaEMode::Undefined, ConvParams.getInWS());
  m_AddDimCoordinates = ConvParams.getAddCoord();

  API::NumericAxis *pXAx;
  this->getAxes(ConvParams.getInWS(), pXAx, m_YAxis);
}
/** Method updates the value of preprocessed detector coordinates in Q-space,
*used by other functions
*@param Coord : input-output vector of MD Coordinates
*@param i -- index of the detector, which corresponds to the spectra to process.
*
*/
bool MDTransfNoQ::calcYDepCoordinates(std::vector<coord_t> &Coord, size_t i) {
  if (m_YAxis) {
    if (Coord[1] < m_DimMin[1] || Coord[1] >= m_DimMax[1])
      return false;
    Coord[1] = (coord_t)(m_YAxis->operator()(i));
  }
  return true;
}
bool MDTransfNoQ::calcMatrixCoord(const double &X, std::vector<coord_t> &Coord,
                                  double & /*s*/, double & /*errSq*/) const {
  if (X < m_DimMin[0] || X >= m_DimMax[0])
    return false;

  Coord[0] = (coord_t)X;
  return true;
}

std::vector<double> MDTransfNoQ::getExtremumPoints(const double xMin,
                                                   const double xMax,
                                                   size_t det_num) const {
  UNUSED_ARG(det_num);

  std::vector<double> rez(2);
  rez[0] = xMin;
  rez[1] = xMax;

  return rez;
}

/** return the number of dimensions, calculated by the transformation from the
workspace.
Depending on ws axis units, the numebr here is either 1 or 2* and is independent
on emode*/
unsigned int
MDTransfNoQ::getNMatrixDimensions(Kernel::DeltaEMode::Type mode,
                                  API::MatrixWorkspace_const_sptr inWS) const {
  UNUSED_ARG(mode);

  API::NumericAxis *pXAx, *pYAx;
  this->getAxes(inWS, pXAx, pYAx);

  unsigned int nMatrDim = 1;
  if (pYAx)
    nMatrDim = 2;
  return nMatrDim;
}
// internal helper function which extract one or two axis from input matrix
// workspace;
void MDTransfNoQ::getAxes(API::MatrixWorkspace_const_sptr inWS,
                          API::NumericAxis *&pXAxis,
                          API::NumericAxis *&pYAxis) {
  // get the X axis of input workspace, it has to be there; if not axis throws
  // invalid index
  pXAxis = dynamic_cast<API::NumericAxis *>(inWS->getAxis(0));
  if (!pXAxis) {
    std::string ERR =
        "Can not retrieve X axis from the source workspace: " + inWS->getName();
    throw(std::invalid_argument(ERR));
  }
  // get optional Y axis which can be used in NoQ-kind of algorithms
  pYAxis = dynamic_cast<API::NumericAxis *>(inWS->getAxis(1));
}

/**function returns units ID-s which this transformation prodiuces its ouptut.
here it is usually input ws units, which are independent on emode
* @param  mode -- current energy analysis mode (not used in NoQ mode)
* @param  inWS -- input matrix workspace shared pointer
*
*/
std::vector<std::string>
MDTransfNoQ::outputUnitID(Kernel::DeltaEMode::Type mode,
                          API::MatrixWorkspace_const_sptr inWS) const {
  UNUSED_ARG(mode);

  std::vector<std::string> rez;
  API::NumericAxis *pXAxis, *pYAx;
  this->getAxes(inWS, pXAxis, pYAx);

  if (pYAx) {
    rez.resize(2);
    rez[1] = pYAx->unit()->unitID();
  } else {
    rez.resize(1);
  }
  rez[0] = pXAxis->unit()->unitID();

  return rez;
}
/**the default dimID-s in noQ mode equal to input WS dim-id-s */
std::vector<std::string>
MDTransfNoQ::getDefaultDimID(Kernel::DeltaEMode::Type mode,
                             API::MatrixWorkspace_const_sptr inWS) const {
  return this->outputUnitID(mode, inWS);
}
/**  returns the units, the input ws is actually in as they coinside with input
 * units for this class */
const std::string
MDTransfNoQ::inputUnitID(Kernel::DeltaEMode::Type mode,
                         API::MatrixWorkspace_const_sptr inWS) const {
  UNUSED_ARG(mode);
  API::NumericAxis *pXAxis;
  // get the X axis of input workspace, it has to be there; if not axis throws
  // invalid index
  pXAxis = dynamic_cast<API::NumericAxis *>(inWS->getAxis(0));
  if (!pXAxis) {
    std::string ERR =
        "Can not retrieve X axis from the source workspace: " + inWS->getName();
    throw(std::invalid_argument(ERR));
  }
  return pXAxis->unit()->unitID();
}

MDTransfNoQ::MDTransfNoQ() : m_NMatrixDim(0), m_YAxis(NULL), m_Det(NULL){}

} // End MDAlgorighms namespace
} // End Mantid namespace
