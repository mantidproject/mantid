// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/UnitConversion.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

#include <cmath>
#include <stdexcept>

namespace Mantid::Kernel {
/**
 * Convert a single value between the given units (as strings)
 * @param src :: The starting unit
 * @param dest :: The destination unit
 * @param srcValue :: The value to convert
 *  @param l1 ::       The source-sample distance (in metres)
 *  @param l2 ::       The sample-detector distance (in metres)
 *  @param theta :: The scattering angle (in radians)
 *  @param emode ::    The energy mode enumeration
 *  @param efixed ::   Value of fixed energy: EI (emode=1) or EF (emode=2) (in
 * meV)
 * @return The value converted to the destination unit
 */
double UnitConversion::run(const std::string &src, const std::string &dest, const double srcValue, const double l1,
                           const double l2, const double theta, const DeltaEMode::Type emode, const double efixed) {
  Unit_sptr srcUnit = UnitFactory::Instance().create(src);
  Unit_sptr destUnit = UnitFactory::Instance().create(dest);
  if ((srcUnit->unitID() == "dSpacing") || (destUnit->unitID() == "dSpacing")) {
    throw std::runtime_error("This signature is deprecated for d Spacing unit conversions");
  }
  UnitParametersMap params{{UnitParams::l2, l2}, {UnitParams::twoTheta, theta}, {UnitParams::efixed, efixed}};
  return UnitConversion::run(*srcUnit, *destUnit, srcValue, l1, emode, params);
} // namespace Kernel

double UnitConversion::run(const std::string &src, const std::string &dest, const double srcValue, const double l1,
                           const DeltaEMode::Type emode, const UnitParametersMap &params) {
  Unit_sptr srcUnit = UnitFactory::Instance().create(src);
  Unit_sptr destUnit = UnitFactory::Instance().create(dest);
  return UnitConversion::run(*srcUnit, *destUnit, srcValue, l1, emode, params);
}

/**
 * Convert a single value between the given units (overload for Unit objects)
 * @param srcUnit :: The starting unit
 * @param destUnit :: The destination unit
 * @param srcValue :: The value to convert
 * @param l1 ::       The source-sample distance (in metres)
 * @param emode ::    The energy mode enumeration
 * @param params ::  Map containing optional parameters eg
 *                   The sample-detector distance (in metres)
 *                   The scattering angle (in radians)
 *                   Fixed energy: EI (emode=1) or EF (emode=2)(in meV)
 *                   Delta (not currently used)
 * @return The value converted to the destination unit
 */
double UnitConversion::run(Unit &srcUnit, Unit &destUnit, const double srcValue, const double l1,
                           const DeltaEMode::Type emode, const UnitParametersMap &params) {
  double factor(0.0), power(0.0);
  if (srcUnit.quickConversion(destUnit, factor, power)) {
    return convertQuickly(srcValue, factor, power);
  } else {
    return convertViaTOF(srcUnit, destUnit, srcValue, l1, emode, params);
  }
}

//---------------------------------------------------------------------------------------------
// Private methods
//---------------------------------------------------------------------------------------------

/**
 * Perform a quick conversion by raising the value to a power & multiplying by
 * the factor
 * @param srcValue :: The value to convert
 * @param factor :: A multiplicative constant
 * @param power :: Raise the src value to this power
 * @return The converted unit
 */
double UnitConversion::convertQuickly(const double srcValue, const double factor, const double power) {
  return factor * std::pow(srcValue, power);
}

/**
 * @param srcUnit :: The starting unit
 * @param destUnit :: The destination unit
 * @param srcValue :: The value to convert
 * @param l1 ::       The source-sample distance (in metres)
 * @param emode ::    The energy mode enumeration
 * @param params ::  Map containing optional parameters eg
 *                   The sample-detector distance (in metres)
 *                   The scattering angle (in radians)
 *                   Fixed energy: EI (emode=1) or EF (emode=2)(in meV)
 *                   Delta (not currently used)
 * @return The value converted to the destination unit
 */
double UnitConversion::convertViaTOF(Unit &srcUnit, Unit &destUnit, const double srcValue, const double l1,
                                     const DeltaEMode::Type emode, const UnitParametersMap &params) {
  // Translate the emode to the int formulation
  int emodeAsInt(0);
  switch (emode) {
  case DeltaEMode::Elastic:
    emodeAsInt = 0;
    break;
  case DeltaEMode::Direct:
    emodeAsInt = 1;
    break;
  case DeltaEMode::Indirect:
    emodeAsInt = 2;
    break;
  default:
    throw std::invalid_argument("UnitConversion::convertViaTOF - Unknown emode " + std::to_string(emode));
  };

  const double tof = srcUnit.convertSingleToTOF(srcValue, l1, emodeAsInt, params);
  return destUnit.convertSingleFromTOF(tof, l1, emodeAsInt, params);
}

/**
 *  Convert a to ElasticQ
 *  @param theta :: The scattering angle (in radians)
 *  @param efixed ::   Value of fixed energy: EI (emode=1) or EF (emode=2) (in
 * meV)
 * @return The value converted to ElasticQ
 */
double UnitConversion::convertToElasticQ(const double theta, const double efixed) {

  Mantid::Kernel::Units::Energy energyUnit;
  double wavelengthFactor(0.0), wavelengthPower(0.0);
  energyUnit.quickConversion("Wavelength", wavelengthFactor, wavelengthPower);

  const double stheta = std::sin(theta);
  // Calculate the wavelength to allow it to be used to convert to elasticQ.
  double wavelength = wavelengthFactor * std::pow(efixed, wavelengthPower);
  // The MomentumTransfer value.
  return 4.0 * M_PI * stheta / wavelength;
}

} // namespace Mantid::Kernel
