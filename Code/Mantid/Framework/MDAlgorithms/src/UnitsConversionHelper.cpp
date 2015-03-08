#include "MantidMDAlgorithms/UnitsConversionHelper.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Strings.h"
#include <boost/math/special_functions/fpclassify.hpp>

namespace Mantid {
namespace MDAlgorithms {

/** establish and initialize proper units conversion from input to output units
@param UnitsFrom -- the ID of the units, which have to be converted from
@param UnitsTo   -- the ID of the units to converted to
@param forceViaTOF   -- force to perform unit conversion via TOF even if quick
conversion exist (by default, false)

@return kind of the initiated conversion, e.g. no conversion (unitsFrom ==
UnitsTo, fastConversion, convFromTOF or convViaTOF.
See ConvertUnits for the details of this transformations

if necessary, also sets up the proper units converter pointers which do the
actual conversion.
*/
CnvrtToMD::ConvertUnits
UnitsConversionHelper::analyzeUnitsConversion(const std::string &UnitsFrom,
                                              const std::string &UnitsTo,
                                              bool forceViaTOF) {
  // if units are equal, no conversion is necessary;
  if (UnitsFrom.compare(UnitsTo) == 0)
    return CnvrtToMD::ConvertNo;

  // get all known units:
  std::vector<std::string> AllKnownUnits =
      Kernel::UnitFactory::Instance().getKeys();

  // check if unit conversion is possible at all:
  if (Kernel::Strings::isMember(AllKnownUnits, UnitsFrom) < 0)
    throw(std::invalid_argument(
        " Can not initate conversion from unknown unit: " + UnitsFrom));

  if (Kernel::Strings::isMember(AllKnownUnits, UnitsFrom) < 0)
    throw(std::invalid_argument(
        " Can not initiate conversion to unknown unit: " + UnitsTo));

  // is a quick conversion available?
  m_SourceWSUnit = Kernel::UnitFactory::Instance().create(UnitsFrom);
  if (m_SourceWSUnit->quickConversion(UnitsTo, m_Factor, m_Power) &&
      !forceViaTOF) {
    return CnvrtToMD::ConvertFast;
  } else {
    // are the input units TOF?
    if (UnitsFrom.compare("TOF") == 0) {
      return CnvrtToMD::ConvertFromTOF;
    } else { // convert using TOF
      m_TargetUnit = Kernel::UnitFactory::Instance().create(UnitsTo);
      return CnvrtToMD::ConvertByTOF;
    }
  }
}
/** Test and check if units conversion really occurs. Return true if unit
 * conversion happens or false if noConversion mode is selected*/
bool UnitsConversionHelper::isUnitConverted() const {
  if (m_UnitCnvrsn == CnvrtToMD::ConvertNo)
    return false;
  return true;
}
/** Initialize unit conversion helper
 * This method is interface to internal initialize method, which actually takes
 all parameters UnitConversion helper needs from
 * targetWSDescr class

 * @param targetWSDescr -- the class which contains all information about target
 workspace
                         including energy transfer mode, number of dimensions,
 input workspace etc.
 * @param unitsTo       -- the ID of the units conversion helper would help to
 convert to
 * @param forceViaTOF   -- force to perform unit conversion via TOF even if
 quick conversion exist (by default, false)
 *
*/
void UnitsConversionHelper::initialize(const MDWSDescription &targetWSDescr,
                                       const std::string &unitsTo,
                                       bool forceViaTOF) {
  // obtain input workspace units
  API::MatrixWorkspace_const_sptr inWS2D = targetWSDescr.getInWS();
  if (!inWS2D)
    throw(std::runtime_error("UnitsConversionHelper::initialize Should not be "
                             "able to call this function when workpsace is "
                             "undefined"));

  API::NumericAxis *pAxis =
      dynamic_cast<API::NumericAxis *>(inWS2D->getAxis(0));
  if (!pAxis)
    throw(std::invalid_argument(
        "Cannot retrieve numeric X axis from the input workspace: " +
        inWS2D->name()));

  std::string unitsFrom = inWS2D->getAxis(0)->unit()->unitID();

  // get detectors positions and other data needed for units conversion:
  if (!(targetWSDescr.m_PreprDetTable))
    throw std::runtime_error("MDWSDescription does not have a detector table");

  int Emode = (int)targetWSDescr.getEMode();

  this->initialize(unitsFrom, unitsTo, targetWSDescr.m_PreprDetTable, Emode,
                   forceViaTOF);
}
// the helper function which used in the code below to simplify check if the
// variable is in range
inline bool inRange(const std::pair<double, double> &range, const double &val) {
  if (val >= range.first && val <= range.second)
    return true;
  else
    return false;
}
// the helper function which used in the code below to simplify check if the
// variable is in range
inline bool inRange(const double &xMin, const double &xMax, const double &val) {
  if (val >= xMin && val <= xMax)
    return true;
  else
    return false;
}

/** Method verify if the Units transformation is well defined in the range
provided and if not
   returns the range where the transformation is well defined.

It is assumed that the points beyond of this range will be filtered in some
other way
@param x1 -- the initial point of the units conversion range to verify
@param x2 -- the final point of the units conversion range to verify
*/
std::pair<double, double>
UnitsConversionHelper::getConversionRange(double x1, double x2) const {
  std::pair<double, double> range;
  range.first = std::min(x1, x2);
  range.second = std::max(x1, x2);

  switch (m_UnitCnvrsn) {
  case (CnvrtToMD::ConvertNo): {
    return range;
  }
  case (CnvrtToMD::ConvertFast): {
    auto trRange = m_TargetUnit->conversionRange();
    double u1 = this->convertUnits(x1);
    double u2 = this->convertUnits(x2);

    if (!inRange(trRange, u1) ||
        !inRange(trRange, u2)) // hopefully it is a rare event
    {
      double uMin = std::min(u1, u2);
      double uMax = std::max(u1, u2);
      if (inRange(uMin, uMax, trRange.first)) {
        double t1 = m_TargetUnit->singleToTOF(trRange.first);
        range.first = m_TargetUnit->singleFromTOF(t1);
      }
      if (inRange(uMin, uMax, trRange.second)) {
        double t2 = m_TargetUnit->singleToTOF(trRange.second);
        range.second = m_SourceWSUnit->singleFromTOF(t2);
      }
    }
    return range;
  }
  case (CnvrtToMD::ConvertFromTOF): {
    double tMin = m_TargetUnit->conversionTOFMin();
    double tMax = m_TargetUnit->conversionTOFMax();

    if (inRange(tMin, tMax, x1) && inRange(tMin, tMax, x2)) {
      return range;
    } else {
      if (inRange(range, tMin))
        range.first = tMin;
      if (inRange(range, tMax))
        range.second = tMax;
    }
    return range;
  }
  case (CnvrtToMD::ConvertByTOF): {
    auto source_range = m_SourceWSUnit->conversionRange();
    if (!inRange(source_range, range.first) ||
        !inRange(source_range, range.second)) {
      if (inRange(range, source_range.first))
        range.first = source_range.first;
      if (inRange(range, source_range.second))
        range.second = source_range.second;
    }
    double tof1 = m_SourceWSUnit->singleToTOF(range.first);
    double tof2 = m_SourceWSUnit->singleToTOF(range.second);
    if (boost::math::isnan(tof1) || boost::math::isnan(tof2)) {
      if (range.first < source_range.first)
        range.first = source_range.first;
      if (range.second > source_range.second)
        range.second = source_range.second;
      tof1 = m_SourceWSUnit->singleToTOF(range.first);
      tof2 = m_SourceWSUnit->singleToTOF(range.second);
    }

    double tMin = m_TargetUnit->conversionTOFMin();
    double tMax = m_TargetUnit->conversionTOFMax();
    if (inRange(tMin, tMax, tof1) && inRange(tMin, tMax, tof2)) {
      return range;
    } else {
      double u1 = range.first;
      double u2 = range.second;
      if (inRange(tof1, tof2, tMin))
        u1 = m_SourceWSUnit->singleFromTOF(tMin);
      if (inRange(tof1, tof2, tMax))
        u2 = m_SourceWSUnit->singleFromTOF(tMax);
      range.first = std::min(u1, u2);
      range.second = std::max(u1, u2);
    }
    return range;
  }
  default:
    throw std::runtime_error(
        "updateConversion: unknown type of conversion requested");
  }
}

/** Initialize unit conversion helper

 * @param unitsFrom     -- the ID of the unit, which should be converted from
 * @param unitsTo       -- the ID of the units conversion helper helps to
 convert to
 * @param DetWS         -- table workspace with preprocessed detectors
 information.
                           See MDAlgorithms::PreprocessDetectorsToMD for the
 info what this workspace contains
 * @param Emode         -- energy transfer mode (integer value used by
 Kernel::ConvertUnits to indetify energy transfer mode
 * @param forceViaTOF   -- force to perform unit conversion via TOF even if
 quick conversion exist (by default, false)
 *
*/

void UnitsConversionHelper::initialize(
    const std::string &unitsFrom, const std::string &unitsTo,
    const DataObjects::TableWorkspace_const_sptr &DetWS, int Emode,
    bool forceViaTOF) {
  m_Emode = Emode;

  if (!DetWS)
    throw std::runtime_error("UnitsConversionHelper::initialize called with "
                             "empty preprocessed detectors table");

  // Check how the source units relate to the units requested and create source
  // units
  m_UnitCnvrsn = analyzeUnitsConversion(unitsFrom, unitsTo, forceViaTOF);

  // create target units class
  m_TargetUnit = Kernel::UnitFactory::Instance().create(unitsTo);
  if (!m_TargetUnit)
    throw(std::runtime_error(
        " Cannot retrieve target unit from the units factory"));

  // get access to all values used by unit conversion.
  m_pTwoThetas = &(DetWS->getColVector<double>("TwoTheta"));
  m_pL2s = &(DetWS->getColVector<double>("L2"));

  m_L1 = DetWS->getLogs()->getPropertyValueAsType<double>("L1");

  // get efix
  m_Efix = DetWS->getLogs()->getPropertyValueAsType<double>("Ei");
  m_pEfixedArray = NULL;
  if (m_Emode == (int)Kernel::DeltaEMode::Indirect)
    m_pEfixedArray = DetWS->getColDataArray<float>("eFixed");

  // set up conversion to working state -- in some tests it can be used straight
  // from the beginning.
  m_TwoTheta = (*m_pTwoThetas)[0];
  m_L2 = (*m_pL2s)[0];
  double Efix = m_Efix;
  if (m_pEfixedArray)
    Efix = (double)(*(m_pEfixedArray + 0));

  m_TargetUnit->initialize(m_L1, m_L2, m_TwoTheta, m_Emode, Efix, 0.);
  if (m_SourceWSUnit) {
    m_SourceWSUnit->initialize(m_L1, m_L2, m_TwoTheta, m_Emode, Efix, 0.);
  }
}
/** Method updates unit conversion given the index of detector parameters in the
 * array of detectors */
void UnitsConversionHelper::updateConversion(size_t i) {
  switch (m_UnitCnvrsn) {
  case (CnvrtToMD::ConvertNo):
    return;
  case (CnvrtToMD::ConvertFast):
    return;
  case (CnvrtToMD::ConvertFromTOF): {
    double delta(std::numeric_limits<double>::quiet_NaN());
    m_TwoTheta = (*m_pTwoThetas)[i];
    m_L2 = (*m_pL2s)[i];
    double Efix = m_Efix;
    if (m_pEfixedArray)
      Efix = (double)(*(m_pEfixedArray + i));

    m_TargetUnit->initialize(m_L1, m_L2, m_TwoTheta, m_Emode, Efix, delta);
    return;
  }
  case (CnvrtToMD::ConvertByTOF): {
    double delta(std::numeric_limits<double>::quiet_NaN());
    m_TwoTheta = (*m_pTwoThetas)[i];
    m_L2 = (*m_pL2s)[i];
    double Efix = m_Efix;
    if (m_pEfixedArray)
      Efix = (double)(*(m_pEfixedArray + i));

    m_TargetUnit->initialize(m_L1, m_L2, m_TwoTheta, m_Emode, Efix, delta);
    m_SourceWSUnit->initialize(m_L1, m_L2, m_TwoTheta, m_Emode, Efix, delta);
    return;
  }
  default:
    throw std::runtime_error(
        "updateConversion: unknown type of conversion requested");
  }
}
/** do actual unit conversion from  input to oputput data
@param   val  -- the input value which has to be converted
@return          the input value converted into the units requested.
*/
double UnitsConversionHelper::convertUnits(double val) const {
  switch (m_UnitCnvrsn) {
  case (CnvrtToMD::ConvertNo): {
    return val;
  }
  case (CnvrtToMD::ConvertFast): {
    return m_Factor * std::pow(val, m_Power);
  }
  case (CnvrtToMD::ConvertFromTOF): {
    return m_TargetUnit->singleFromTOF(val);
  }
  case (CnvrtToMD::ConvertByTOF): {
    double tof = m_SourceWSUnit->singleToTOF(val);
    return m_TargetUnit->singleFromTOF(tof);
  }
  default:
    throw std::runtime_error(
        "updateConversion: unknown type of conversion requested");
  }
}
// copy constructor;
UnitsConversionHelper::UnitsConversionHelper(
    const UnitsConversionHelper &another) {
  m_UnitCnvrsn = another.m_UnitCnvrsn;
  m_Factor = another.m_Factor;
  m_Power = another.m_Power;

  m_Emode = another.m_Emode;
  m_L1 = another.m_L1;
  m_Efix = another.m_Efix;
  m_TwoTheta = another.m_TwoTheta;
  m_L2 = another.m_L2;
  m_pTwoThetas = another.m_pTwoThetas;
  m_pL2s = another.m_pL2s;
  m_pEfixedArray = another.m_pEfixedArray;

  if (another.m_SourceWSUnit)
    m_SourceWSUnit = Kernel::Unit_sptr(another.m_SourceWSUnit->clone());
  if (another.m_TargetUnit)
    m_TargetUnit = Kernel::Unit_sptr(another.m_TargetUnit->clone());
}

UnitsConversionHelper::UnitsConversionHelper()
    : m_UnitCnvrsn(CnvrtToMD::ConvertNo), m_Factor(1), m_Power(1),
      m_Emode(-1), // undefined
      m_L1(1), m_Efix(1), m_TwoTheta(0), m_L2(1), m_pTwoThetas(NULL),
      m_pL2s(NULL), m_pEfixedArray(NULL){};

} // endNamespace DataObjects
} // endNamespace Mantid
