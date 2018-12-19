// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/MDWSDescription.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"

#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"

#include <boost/lexical_cast.hpp>

namespace Mantid {
namespace MDAlgorithms {

/** set specific (non-default) dimension name
 * @param nDim   -- number of dimension;
 * @param Name   -- the name to assign into dimension names vector;
 */
void MDWSDescription::setDimName(unsigned int nDim, const std::string &Name) {
  if (nDim >= m_NDims) {
    std::string ERR =
        "setDimName::Dimension index: " + std::to_string(nDim) +
        " out of total dimensions range: " + std::to_string(m_NDims);
    throw(std::invalid_argument(ERR));
  }
  m_DimNames[nDim] = Name;
}
/** this is rather misleading function, as MD workspace does not currently have
 *dimension units.
 *It actually sets the units for the dimension names, which will be displayed
 *along axis and have nothing in common with units, defined by unit factory */
void MDWSDescription::setDimUnit(unsigned int nDim, const std::string &Unit) {
  if (nDim >= m_NDims) {
    std::string ERR =
        "setDimUnit::Dimension index: " + std::to_string(nDim) +
        " out of total dimensions range: " + std::to_string(m_NDims);
    throw(std::invalid_argument(ERR));
  }
  m_DimUnits[nDim] = Unit;
}

/** the method builds the MD ws description from existing matrix workspace and
the requested transformation parameters.
*@param  pWS    -- input matrix workspace to be converted into MD workspace
*@param  QMode  -- momentum conversion mode. Any mode supported by Q conversion
factory. Class just carries up the name of Q-mode,
*                  to the place where factory call to the solver is made , so no
code modification is needed when new modes are added
*                  to the factory
*@param  dEMode  -- energy analysis mode (string representation). Should
correspond to energy analysis modes, supported by selected Q-mode
*@param  dimPropertyNames -- the vector of names for additional ws properties,
which will be used as dimensions.

*/
void MDWSDescription::buildFromMatrixWS(
    const API::MatrixWorkspace_sptr &pWS, const std::string &QMode,
    const std::string dEMode,
    const std::vector<std::string> &dimPropertyNames) {
  m_InWS = pWS;
  // fill additional dimensions values, defined by workspace properties;
  this->fillAddProperties(m_InWS, dimPropertyNames, m_AddCoord);

  this->AlgID = QMode;

  // check and get energy conversion mode;

  m_Emode = Kernel::DeltaEMode::fromString(dEMode);

  // get raw pointer to Q-transformation (do not delete this pointer, its held
  // by MDTransfFactory!)
  MDTransfInterface *pQtransf = MDTransfFactory::Instance().create(QMode).get();

  // get number of dimensions this Q transformation generates from the
  // workspace.
  unsigned int nMatrixDim = pQtransf->getNMatrixDimensions(m_Emode, m_InWS);

  // number of MD ws dimensions is the sum of n-matrix dimensions and dimensions
  // coming from additional coordinates
  m_NDims = nMatrixDim + static_cast<unsigned int>(m_AddCoord.size());
  this->resizeDimDescriptions(m_NDims);
  // check if all MD dimensions descriptors are set properly
  if (m_NDims != m_DimNames.size() || m_NDims != m_DimMin.size()) {
    if (m_buildingNewWorkspace) {
      throw(std::invalid_argument(" dimension limits vectors and dimension "
                                  "description vectors inconsistent as have "
                                  "different length"));
    } else {
      throw(std::invalid_argument(
          " dimension limits vectors and dimension description vectors "
          "inconsistent as have different length\n"
          " Are you trying to add to existing workspace with convertToMD, "
          "which generates workspace with different number of dimensions?"));
    }
  }

  //*********** fill in dimension id-s, dimension units and dimension names
  // get default dim ID-s. TODO: it should be possibility to override it later;
  std::vector<std::string> MatrDimID =
      pQtransf->getDefaultDimID(m_Emode, m_InWS);
  std::vector<std::string> MatrUnitID = pQtransf->outputUnitID(m_Emode, m_InWS);
  for (unsigned int i = 0; i < m_NDims; i++) {
    if (i < nMatrixDim) {
      m_DimIDs[i] = MatrDimID[i];
      m_DimNames[i] = MatrDimID[i];
      m_DimUnits[i] = MatrUnitID[i];
    } else {
      m_DimIDs[i] = dimPropertyNames[i - nMatrixDim];
      m_DimNames[i] = dimPropertyNames[i - nMatrixDim];
      m_DimUnits[i] = dimPropertyNames[i - nMatrixDim];
    }
  }
}

void MDWSDescription::setWS(API::MatrixWorkspace_sptr otherMatrixWS) {
  m_InWS = otherMatrixWS;
}
/// Method checks if input workspace has defined goniometer
bool MDWSDescription::hasGoniometer() const {
  if (m_InWS)
    return m_InWS->run().getGoniometer().isDefined();
  else
    return false;
}
/// method returns goniometer matrix if one is defined on the workspace or unit
/// matrix if there are no such matrix
Kernel::Matrix<double> MDWSDescription::getGoniometerMatr() const {
  if (m_InWS)
    return m_InWS->run().getGoniometer().getR();
  else
    return Kernel::Matrix<double>(3, 3, true);
}

/** the function builds MD event WS description from existing workspace.
 * Primary used to obtain existing ws parameters
 *@param pWS -- shared pointer to existing MD workspace
 */
void MDWSDescription::buildFromMDWS(
    const API::IMDEventWorkspace_const_sptr &pWS) {
  m_NDims = static_cast<unsigned int>(pWS->getNumDims());
  // prepare all arrays:
  m_DimNames.resize(m_NDims);
  m_DimIDs.resize(m_NDims);
  m_DimUnits.resize(m_NDims);

  m_NBins.resize(m_NDims);
  m_DimMin.resize(m_NDims);
  m_DimMax.resize(m_NDims);
  for (size_t i = 0; i < m_NDims; i++) {
    const Geometry::IMDDimension *pDim = pWS->getDimension(i).get();
    m_DimNames[i] = pDim->getName();
    m_DimIDs[i] = pDim->getDimensionId();
    m_DimUnits[i] = pDim->getUnits();

    m_NBins[i] = pDim->getNBins();
    m_DimMin[i] = pDim->getMinimum();
    m_DimMax[i] = pDim->getMaximum();
  }
  m_Wtransf = Kernel::DblMatrix(pWS->getWTransf());
}
/** When the workspace has been build from existing MDWrokspace, some target
 *workspace parameters can not be defined,
 * as these parameters are defined by the algorithm and input matrix workspace.
 *  examples are emode or input energy, which is actually source workspace
 *parameters, or some other parameters
 *  defined by the transformation algorithm
 *
 * This method used to define such parameters from MDWS description, build from
 *workspace and the transformation algorithm parameters
 *
 *@param SourceMatrWS -- the MDWS description obtained from input matrix
 *workspace and the algorithm parameters
 */
void MDWSDescription::setUpMissingParameters(
    const MDWSDescription &SourceMatrWS) {
  m_InWS = SourceMatrWS.m_InWS;
  m_Emode = SourceMatrWS.m_Emode;
  m_LorentzCorr = SourceMatrWS.m_LorentzCorr;
  m_AbsMin = SourceMatrWS.m_AbsMin;
  this->AlgID = SourceMatrWS.AlgID;

  m_AddCoord.assign(SourceMatrWS.m_AddCoord.begin(),
                    SourceMatrWS.m_AddCoord.end());
}

/**function compares old workspace description with the new workspace
 *description, defined by the algorithm properties and
 * selects/changes the properties which can be changed through input parameters
 *given that target MD workspace exist
 *
 * This situation occurs if the base description has been obtained from MD
 *workspace, and one is building a description from
 * other matrix workspace to add new data to the existing workspace. The
 *workspaces have to be comparable.
 *
 * @param NewMDWorkspaceD -- MD workspace description, obtained from algorithm
 *parameters
 *
 * @returns NewMDWorkspaceD -- modified md workspace description, which is
 *compatible with existing MD workspace
 *
 */
void MDWSDescription::checkWSCorresponsMDWorkspace(
    MDWSDescription &NewMDWorkspaceD) {
  if (m_NDims != NewMDWorkspaceD.m_NDims) {
    std::string ERR =
        "Dimension numbers are inconsistent: this workspace has " +
        std::to_string(m_NDims) + " dimensions and target one: " +
        std::to_string(NewMDWorkspaceD.m_NDims);
    throw(std::invalid_argument(ERR));
  }

  if (m_Emode == Kernel::DeltaEMode::Undefined)
    throw(std::invalid_argument("Workspace description has not been correctly "
                                "defined, as emode has not been defined"));

  // TODO: !!! Dim Unit currently have decorative name and is not used in real
  // conversion.  It is just a name. This is why this check does not work
  // properly
  /* for(size_t i=0;i<m_NDims;i++)
  {
  if(m_DimUnits[i] != NewMDWorkspaceD.m_DimUnits[i])
  {
  throw std::runtime_error("The target MDEventWorkspace dimension N:
  "+boost::lexical_cast<std::string>(i)+" has units: "+m_DimUnits[i]+
  " different from the requested: "+NewMDWorkspaceD.m_DimUnits[i]+
  "\n Either give a different workspace as the output, or change the
  OutputDimensions parameter.");
  }
  }*/

  // TODO: More thorough checks may be necessary to prevent adding different
  // kind of workspaces e.g 4D |Q|-dE-T-P workspace to Q3d+dE ws
}

/// empty constructor
MDWSDescription::MDWSDescription(unsigned int nDimensions)
    : m_Wtransf(3, 3, true), m_RotMatrix(9, 0), m_buildingNewWorkspace(true),
      m_Emode(Kernel::DeltaEMode::Undefined), m_LorentzCorr(false),
      m_AbsMin(0.), m_coordinateSystem(Mantid::Kernel::None) {

  this->resizeDimDescriptions(nDimensions);
  m_DimMin.assign(m_NDims, std::numeric_limits<double>::quiet_NaN());
  m_DimMax.assign(m_NDims, std::numeric_limits<double>::quiet_NaN());

  // set transformation matrix to identity - aka do nothing
  m_RotMatrix[0] = 1.;
  m_RotMatrix[4] = 1.;
  m_RotMatrix[8] = 1.;
}
void MDWSDescription::resizeDimDescriptions(unsigned int nDimensions,
                                            size_t nBins) {
  m_NDims = nDimensions;

  m_DimNames.assign(m_NDims, "mdn");
  m_DimIDs.assign(m_NDims, "mdn_");
  m_DimUnits.assign(m_NDims, "Momentum");
  m_NBins.assign(m_NDims, nBins);

  for (size_t i = 0; i < m_NDims; i++) {
    m_DimIDs[i] = m_DimIDs[i] + std::to_string(i);
    m_DimNames[i] = m_DimNames[i] + std::to_string(i);
  }
}
/**function sets number of bins each dimension become split
 * @param nBins_toSplit vector, containing number of bins each dimension is
 * split into.
 *                      If the vector contains only one element, each dimension
 * is split according to this element values.
 */
void MDWSDescription::setNumBins(const std::vector<int> &nBins_toSplit) {

  if (!(nBins_toSplit.size() == 1 || nBins_toSplit.size() == this->m_NDims))
    throw std::invalid_argument(
        " Number of dimensions: " + std::to_string(nBins_toSplit.size()) +
        " defining number of bins to split into is not equal to total number "
        "of dimensions: " +
        std::to_string(this->m_NDims));

  this->m_NBins.resize(this->m_NDims);

  bool propagateOneNum = true;
  if (nBins_toSplit.size() == this->m_NDims)
    propagateOneNum = false;

  for (size_t i = 0; i < this->m_NDims; i++) {
    if (propagateOneNum)
      this->m_NBins[i] = nBins_toSplit[0];
    else
      this->m_NBins[i] = nBins_toSplit[i];
  }
}

/**function sets up min-max values to the dimensions, described by the class
 *@param minVal  -- vector of minimal dimension's values
 *@param maxVal  -- vector of maximal dimension's values
 *
 */
void MDWSDescription::setMinMax(const std::vector<double> &minVal,
                                const std::vector<double> &maxVal) {
  m_DimMin.assign(minVal.begin(), minVal.end());
  m_DimMax.assign(maxVal.begin(), maxVal.end());

  this->checkMinMaxNdimConsistent(m_DimMin, m_DimMax);
}

/**get vector of minimal and maximal values from the class */
void MDWSDescription::getMinMax(std::vector<double> &min,
                                std::vector<double> &max) const {
  min.assign(m_DimMin.begin(), m_DimMin.end());
  max.assign(m_DimMax.begin(), m_DimMax.end());
}
//******************************************************************************************************************************************
//*************   HELPER FUNCTIONS
//******************************************************************************************************************************************
/** Method checks if the workspace is expected to be processed in powder mode */
bool MDWSDescription::isPowder() const {
  return (this->AlgID == "|Q|") ||
         (this->AlgID.empty() && !m_InWS->sample().hasOrientedLattice());
}

/** Returns symbolic representation of current Emode */
std::string MDWSDescription::getEModeStr() const {
  return Kernel::DeltaEMode::asString(m_Emode);
}

/** function extracts the coordinates from additional workspace properties and
 *places them to proper position within
 *  the vector of MD coordinates for the particular workspace.
 *
 *  @param inWS2D -- input workspace
 *  @param dimPropertyNames  -- names of properties which should be treated as
 *dimensions
 *  @param AddCoord --
 *
 *  @return AddCoord       -- vector of additional coordinates (derived from WS
 *properties) for current multidimensional event
 */
void MDWSDescription::fillAddProperties(
    Mantid::API::MatrixWorkspace_const_sptr inWS2D,
    const std::vector<std::string> &dimPropertyNames,
    std::vector<coord_t> &AddCoord) {
  size_t nDimPropNames = dimPropertyNames.size();
  if (AddCoord.size() != nDimPropNames)
    AddCoord.resize(nDimPropNames);
  const auto &runObj = inWS2D->run();

  for (size_t i = 0; i < nDimPropNames; i++) {
    try {
      const double value = runObj.getLogAsSingleValue(
          dimPropertyNames[i], Mantid::Kernel::Math::TimeAveragedMean);
      AddCoord[i] = static_cast<coord_t>(value);
    } catch (std::invalid_argument &) {
      std::string ERR =
          " Can not interpret property, used as dimension.\n Property: " +
          dimPropertyNames[i] + " cannot be converted into a double.";
      throw(std::invalid_argument(ERR));
    }
  }
}

/** function verifies the consistency of the min and max dimensions values
 * checking if all necessary
 * values were defined and min values are smaller then max values */
void MDWSDescription::checkMinMaxNdimConsistent(
    const std::vector<double> &minVal, const std::vector<double> &maxVal) {
  if (minVal.size() != maxVal.size()) {
    std::string ERR = " number of specified min dimension values: " +
                      std::to_string(minVal.size()) +
                      " and number of max values: " +
                      std::to_string(maxVal.size()) + " are not consistent\n";
    throw(std::invalid_argument(ERR));
  }

  for (size_t i = 0; i < minVal.size(); i++) {
    if (maxVal[i] <= minVal[i]) {
      std::string ERR = " min value " +
                        boost::lexical_cast<std::string>(minVal[i]) +
                        " not less then max value" +
                        boost::lexical_cast<std::string>(maxVal[i]) +
                        " in direction: " + std::to_string(i) + "\n";
      throw(std::invalid_argument(ERR));
    }
  }
}

/** function retrieves copy of the oriented lattice from the workspace */
boost::shared_ptr<Geometry::OrientedLattice>
MDWSDescription::getOrientedLattice(
    Mantid::API::MatrixWorkspace_const_sptr inWS2D) {
  // try to get the WS oriented lattice
  boost::shared_ptr<Geometry::OrientedLattice> orl;
  if (inWS2D->sample().hasOrientedLattice())
    orl = boost::shared_ptr<Geometry::OrientedLattice>(
        new Geometry::OrientedLattice(inWS2D->sample().getOrientedLattice()));

  return orl;
}

/** Set the special coordinate system if any.
@param system : coordinate system.
*/
void MDWSDescription::setCoordinateSystem(
    const Mantid::Kernel::SpecialCoordinateSystem system) {
  m_coordinateSystem = system;
}

/**
 * create the frame
 * @param d : dimension index to get the frame for.
 * @return MDFrame
 */
Geometry::MDFrame_uptr MDWSDescription::getFrame(size_t d) const {
  auto factory = Geometry::makeMDFrameFactoryChain();
  return factory->create(Geometry::MDFrameArgument(m_frameKey, m_DimUnits[d]));
}

/**
 * Sets the frame.
 * @param frameKey : Frame key desired.
 */
void MDWSDescription::setFrame(const std::string frameKey) {
  m_frameKey = frameKey;
}

/// @return the special coordinate system if any.
Mantid::Kernel::SpecialCoordinateSystem
MDWSDescription::getCoordinateSystem() const {
  return m_coordinateSystem;
}

/**
 * Is the algorithm running in Q3D mode?
 * @return True only if in Q3D mode
 */
bool MDWSDescription::isQ3DMode() const { return this->AlgID == "Q3D"; }

bool MDWSDescription::hasLattice() const {
  return m_InWS->sample().hasOrientedLattice();
}

} // end namespace MDAlgorithms
} // end namespace Mantid
