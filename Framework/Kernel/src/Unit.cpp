// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Unit.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"
#include <cfloat>
#include <limits>
#include <math.h>
#include <sstream>

namespace Mantid::Kernel {

namespace {
// static logger object
Logger g_log("Unit");

bool ParamPresent(const UnitParametersMap &params, UnitParams param) { return params.find(param) != params.end(); }

bool ParamPresentAndSet(const UnitParametersMap *params, UnitParams param, double &var) {
  auto it = params->find(param);
  if (it != params->end()) {
    var = it->second;
    return true;
  } else {
    return false;
  }
}
} // namespace

/**
 * Default constructor
 * Gives the unit an empty UnitLabel
 */
Unit::Unit() : initialized(false), l1(0), emode(0) {}

bool Unit::operator==(const Unit &u) const { return unitID() == u.unitID(); }

bool Unit::operator!=(const Unit &u) const { return !(*this == u); }

/** Is conversion by constant multiplication possible?
 *
 *  Look to see if conversion from the unit upon which this method is called
 *requires
 *  only multiplication by a constant and not detector information (i.e.
 *distance & angle),
 *  in which case doing the conversion via time-of-flight is not necessary.
 *  @param destination :: The unit to which conversion is sought
 *  @param factor ::      Returns the constant by which to multiply the input
 *unit (if a conversion is found)
 *  @param power ::       Returns the power to which to raise the unput unit (if
 *a conversion is found)
 *  @return            True if a 'quick conversion' exists, false otherwise
 */
bool Unit::quickConversion(const Unit &destination, double &factor, double &power) const {
  // Just extract the unit's name and forward to other quickConversion method
  return quickConversion(destination.unitID(), factor, power);
}

/** Is conversion by constant multiplication possible?
 *
 *  Look to see if conversion from the unit upon which this method is called
 *requires
 *  only multiplication by a constant and not detector information (i.e.
 *distance & angle),
 *  in which case doing the conversion via time-of-flight is not necessary.
 *  @param destUnitName :: The class name of the unit to which conversion is
 *sought
 *  @param factor ::       Returns the constant by which to multiply the input
 *unit (if a conversion is found)
 *  @param power ::        Returns the power to which to raise the unput unit
 *(if a conversion is found)
 *  @return             True if a 'quick conversion' exists, false otherwise
 */
bool Unit::quickConversion(std::string destUnitName, double &factor, double &power) const {
  // From the global map, try to get the map holding the conversions for this
  // unit
  ConversionsMap::const_iterator it = s_conversionFactors.find(unitID());
  // Return false if there are no conversions entered for this unit
  if (it == s_conversionFactors.end())
    return false;

  // See if there's a conversion listed for the requested destination unit
  std::transform(destUnitName.begin(), destUnitName.end(), destUnitName.begin(), toupper);
  auto iter = it->second.find(destUnitName);
  // If not, return false
  if (iter == it->second.end())
    return false;

  // Conversion found - set the conversion factors
  factor = iter->second.first;
  power = iter->second.second;
  return true;
}

// Initialise the static map holding the 'quick conversions'
Unit::ConversionsMap Unit::s_conversionFactors = Unit::ConversionsMap();

//---------------------------------------------------------------------------------------
/** Add a 'quick conversion' from the unit class on which this method is called.
 *  @param to ::     The destination Unit for this conversion (use name returned
 * by the unit's unitID() method)
 *  @param factor :: The constant by which to multiply the input unit
 *  @param power ::  The power to which to raise the input unit (defaults to 1)
 */
void Unit::addConversion(std::string to, const double &factor, const double &power) const {
  std::transform(to.begin(), to.end(), to.begin(), toupper);
  // Add the conversion to the map (does nothing if it's already there)
  s_conversionFactors[unitID()][to] = std::make_pair(factor, power);
}

//---------------------------------------------------------------------------------------
/** Initialize the unit to perform conversion using singleToTof() and
 *singleFromTof()
 *
 *  @param _l1 ::       The source-sample distance (in metres)
 *  @param _emode ::    The energy mode (0=elastic, 1=direct geometry,
 *2=indirect geometry)
 *  @param params ::  Map containing optional parameters eg
 *                    The sample-detector distance (in metres)
 *                    The scattering angle (in radians)
 *                    Fixed energy: EI (emode=1) or EF (emode=2)(in meV)
 *                    Delta (not currently used)
 */
void Unit::initialize(const double &_l1, const int &_emode, const UnitParametersMap &params) {
  l1 = _l1;
  validateUnitParams(_emode, params);
  emode = _emode;
  m_params = &params;
  initialized = true;
  this->init();
}

void Unit::validateUnitParams(const int, const UnitParametersMap &) {}

//---------------------------------------------------------------------------------------
/** Perform the conversion to TOF on a vector of data
 */

void Unit::toTOF(std::vector<double> &xdata, std::vector<double> &ydata, const double &_l1, const int &_emode,
                 std::initializer_list<std::pair<const UnitParams, double>> params) {
  UnitParametersMap paramsMap(params);
  toTOF(xdata, ydata, _l1, _emode, paramsMap);
}

void Unit::toTOF(std::vector<double> &xdata, std::vector<double> &ydata, const double &_l1, const int &_emode,
                 const UnitParametersMap &params) {
  UNUSED_ARG(ydata);
  this->initialize(_l1, _emode, params);
  size_t numX = xdata.size();
  for (size_t i = 0; i < numX; i++)
    xdata[i] = this->singleToTOF(xdata[i]);
}

/** Convert a single value to TOF
@param xvalue
@param l1
@param emode
@param params (eg efixed or delta)
*/
double Unit::convertSingleToTOF(const double xvalue, const double &l1, const int &emode,
                                const UnitParametersMap &params) {
  this->initialize(l1, emode, params);
  return this->singleToTOF(xvalue);
}

//---------------------------------------------------------------------------------------
/** Perform the conversion to TOF on a vector of data
 */
void Unit::fromTOF(std::vector<double> &xdata, std::vector<double> &ydata, const double &_l1, const int &_emode,
                   std::initializer_list<std::pair<const UnitParams, double>> params) {
  UnitParametersMap paramsMap(params);
  fromTOF(xdata, ydata, _l1, _emode, paramsMap);
}

void Unit::fromTOF(std::vector<double> &xdata, std::vector<double> &ydata, const double &_l1, const int &_emode,
                   const UnitParametersMap &params) {
  UNUSED_ARG(ydata);
  this->initialize(_l1, _emode, params);
  size_t numX = xdata.size();
  for (size_t i = 0; i < numX; i++)
    xdata[i] = this->singleFromTOF(xdata[i]);
}

/** Convert a single value from TOF
@param xvalue
@param l1
@param emode
@param params (eg efixed or delta)
*/
double Unit::convertSingleFromTOF(const double xvalue, const double &l1, const int &emode,
                                  const UnitParametersMap &params) {
  this->initialize(l1, emode, params);
  return this->singleFromTOF(xvalue);
}

std::pair<double, double> Unit::conversionRange() const {
  double u1 = this->singleFromTOF(this->conversionTOFMin());
  double u2 = this->singleFromTOF(this->conversionTOFMax());
  //
  return std::pair<double, double>(std::min(u1, u2), std::max(u1, u2));
}

namespace Units {

/* =============================================================================
 * EMPTY
 * =============================================================================
 */
DECLARE_UNIT(Empty)

const UnitLabel Empty::label() const { return Symbol::EmptyLabel; }

void Empty::init() {}

double Empty::singleToTOF(const double x) const {
  UNUSED_ARG(x);
  throw Kernel::Exception::NotImplementedError("Cannot convert unit " + this->unitID() + " to time of flight");
}

double Empty::singleFromTOF(const double tof) const {
  UNUSED_ARG(tof);
  throw Kernel::Exception::NotImplementedError("Cannot convert to unit " + this->unitID() + " from time of flight");
}

Unit *Empty::clone() const { return new Empty(*this); }

/**
 * @return NaN as Label can not be obtained from TOF in any reasonable manner
 */
double Empty::conversionTOFMin() const { return std::numeric_limits<double>::quiet_NaN(); }

/**
 * @return NaN as Label can not be obtained from TOF in any reasonable manner
 */
double Empty::conversionTOFMax() const { return std::numeric_limits<double>::quiet_NaN(); }

/* =============================================================================
 * LABEL
 * =============================================================================
 */

DECLARE_UNIT(Label)

const UnitLabel Label::label() const { return m_label; }

/// Constructor
Label::Label() : Empty(), m_caption("Quantity"), m_label(Symbol::EmptyLabel) {}

Label::Label(const std::string &caption, const std::string &label) : Empty(), m_caption(), m_label(Symbol::EmptyLabel) {
  setLabel(caption, label);
}

/**
 * Set a caption and a label
 */
void Label::setLabel(const std::string &cpt, const UnitLabel &lbl) {
  m_caption = cpt;
  m_label = lbl;
}

Unit *Label::clone() const { return new Label(*this); }

/* =============================================================================
 * TIME OF FLIGHT
 * =============================================================================
 */
DECLARE_UNIT(TOF)

const UnitLabel TOF::label() const { return Symbol::Microsecond; }

TOF::TOF() : Unit() {}

void TOF::init() {}

double TOF::singleToTOF(const double x) const {
  // Nothing to do
  return x;
}

double TOF::singleFromTOF(const double tof) const {
  // Nothing to do
  return tof;
}

Unit *TOF::clone() const { return new TOF(*this); }
double TOF::conversionTOFMin() const { return -DBL_MAX; }
///@return DBL_MAX as ToF convetanble to TOF for in any time range
double TOF::conversionTOFMax() const { return DBL_MAX; }

// ============================================================================================
/* WAVELENGTH
 * ===================================================================================================
 *
 * This class makes use of the de Broglie relationship: lambda = h/p = h/mv,
 *where v is (l1+l2)/tof.
 */
DECLARE_UNIT(Wavelength)

Wavelength::Wavelength()
    : Unit(), efixed(0.), sfpTo(DBL_MIN), factorTo(DBL_MIN), sfpFrom(DBL_MIN), factorFrom(DBL_MIN), do_sfpFrom(false) {
  const double AngstromsSquared = 1e20;
  const double factor = (AngstromsSquared * PhysicalConstants::h * PhysicalConstants::h) /
                        (2.0 * PhysicalConstants::NeutronMass * PhysicalConstants::meV);
  addConversion("Energy", factor, -2.0);
  addConversion("Energy_inWavenumber", factor * PhysicalConstants::meVtoWavenumber, -2.0);
  addConversion("Momentum", 2 * M_PI, -1.0);
}

const UnitLabel Wavelength::label() const { return Symbol::Angstrom; }

void Wavelength::validateUnitParams(const int emode, const UnitParametersMap &params) {
  if (!ParamPresent(params, UnitParams::l2)) {
    throw std::runtime_error("An l2 value must be supplied in the extra "
                             "parameters when initialising " +
                             this->unitID() + " for conversion via TOF");
  }
  if ((emode != 0) && (!ParamPresent(params, UnitParams::efixed))) {
    throw std::runtime_error("An efixed value must be supplied in the extra "
                             "parameters when initialising " +
                             this->unitID() + " for conversion via TOF");
  }
}

void Wavelength::init() {
  // ------------ Factors to convert TO TOF ---------------------
  double l2 = 0.0;
  double ltot = 0.0;
  double TOFisinMicroseconds = 1e6;
  double toAngstroms = 1e10;
  sfpTo = 0.0;

  ParamPresentAndSet(m_params, UnitParams::efixed, efixed);
  ParamPresentAndSet(m_params, UnitParams::l2, l2);

  if (emode == 1) {
    ltot = l2;
    sfpTo = (sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) * TOFisinMicroseconds * l1) /
            sqrt(efixed);
  } else if (emode == 2) {
    ltot = l1;
    sfpTo = (sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) * TOFisinMicroseconds * l2) /
            sqrt(efixed);
  } else {
    ltot = l1 + l2;
  }
  factorTo = (PhysicalConstants::NeutronMass * (ltot)) / PhysicalConstants::h;
  // Now adjustments for the scale of units used
  factorTo *= TOFisinMicroseconds / toAngstroms;

  // ------------ Factors to convert FROM TOF ---------------------
  // Now apply the factor to the input data vector
  do_sfpFrom = false;
  if (efixed != DBL_MIN) {
    if (emode == 1) // Direct
    {
      sfpFrom = (sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) * TOFisinMicroseconds * l1) /
                sqrt(efixed);
      do_sfpFrom = true;
    } else if (emode == 2) // Indirect
    {
      sfpFrom = (sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) * TOFisinMicroseconds * l2) /
                sqrt(efixed);
      do_sfpFrom = true;
    }
  }

  // Protect against divide by zero
  if (ltot == 0.0)
    ltot = DBL_MIN;

  // First the crux of the conversion
  factorFrom = PhysicalConstants::h / (PhysicalConstants::NeutronMass * (ltot));

  // Now adjustments for the scale of units used
  factorFrom *= toAngstroms / TOFisinMicroseconds;
}

double Wavelength::singleToTOF(const double x) const {
  double tof = x * factorTo;
  // If Direct or Indirect we want to correct TOF values..
  if (emode == 1 || emode == 2)
    tof += sfpTo;
  return tof;
}
double Wavelength::singleFromTOF(const double tof) const {
  double x = tof;
  if (do_sfpFrom)
    x -= sfpFrom;
  x *= factorFrom;
  return x;
}
///@return  Minimal time of flight, which can be reversively converted into
/// wavelength
double Wavelength::conversionTOFMin() const {
  double min_tof(0);
  if (emode == 1 || emode == 2)
    min_tof = sfpTo;
  return min_tof;
}
///@return  Maximal time of flight, which can be reversively converted into
/// wavelength
double Wavelength::conversionTOFMax() const {
  double max_tof;
  if (factorTo > 1) {
    max_tof = (DBL_MAX - sfpTo) / factorTo;
  } else {
    max_tof = DBL_MAX - sfpTo / factorTo;
  }
  return max_tof;
}

Unit *Wavelength::clone() const { return new Wavelength(*this); }

// ============================================================================================
/* ENERGY
 * ===============================================================================================
 *
 * Conversion uses E = 1/2 mv^2, where v is (l1+l2)/tof.
 */
DECLARE_UNIT(Energy)

const UnitLabel Energy::label() const { return Symbol::MilliElectronVolts; }

/// Constructor
Energy::Energy() : Unit(), factorTo(DBL_MIN), factorFrom(DBL_MIN) {
  addConversion("Energy_inWavenumber", PhysicalConstants::meVtoWavenumber);
  const double toAngstroms = 1e10;
  const double factor =
      toAngstroms * PhysicalConstants::h / sqrt(2.0 * PhysicalConstants::NeutronMass * PhysicalConstants::meV);
  addConversion("Wavelength", factor, -0.5);
  addConversion("Momentum", 2 * M_PI / factor, 0.5);
}

void Energy::validateUnitParams(const int, const UnitParametersMap &params) {
  if (!ParamPresent(params, UnitParams::l2)) {
    throw std::runtime_error("An l2 value must be supplied in the extra "
                             "parameters when initialising " +
                             this->unitID() + " for conversion via TOF");
  }
}

void Energy::init() {
  double l2 = 0.0;
  ParamPresentAndSet(m_params, UnitParams::l2, l2);
  {
    const double TOFinMicroseconds = 1e6;
    factorTo = sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) * (l1 + l2) * TOFinMicroseconds;
  }
  {
    const double TOFisinMicroseconds = 1e-12; // The input tof number gets squared so this is (10E-6)^2
    const double ltot = l1 + l2;
    factorFrom =
        ((PhysicalConstants::NeutronMass / 2.0) * (ltot * ltot)) / (PhysicalConstants::meV * TOFisinMicroseconds);
  }
}

double Energy::singleToTOF(const double x) const {
  double temp = x;
  if (temp == 0.0)
    temp = DBL_MIN; // Protect against divide by zero
  return factorTo / sqrt(temp);
}
///@return  Minimal time of flight which can be reversibly converted into energy
double Energy::conversionTOFMin() const { return factorTo / sqrt(DBL_MAX); }
double Energy::conversionTOFMax() const { return sqrt(DBL_MAX); }

double Energy::singleFromTOF(const double tof) const {
  double temp = tof;
  if (temp == 0.0)
    temp = DBL_MIN; // Protect against divide by zero
  return factorFrom / (temp * temp);
}

Unit *Energy::clone() const { return new Energy(*this); }

// ============================================================================================
/* ENERGY IN UNITS OF WAVENUMBER
 * ============================================================================================
 *
 * Conversion uses E = 1/2 mv^2, where v is (l1+l2)/tof.
 */
DECLARE_UNIT(Energy_inWavenumber)

const UnitLabel Energy_inWavenumber::label() const { return Symbol::InverseCM; }

/// Constructor
Energy_inWavenumber::Energy_inWavenumber() : Unit(), factorTo(DBL_MIN), factorFrom(DBL_MIN) {
  addConversion("Energy", 1.0 / PhysicalConstants::meVtoWavenumber);
  const double toAngstroms = 1e10;
  const double factor =
      toAngstroms * PhysicalConstants::h /
      sqrt(2.0 * PhysicalConstants::NeutronMass * PhysicalConstants::meV / PhysicalConstants::meVtoWavenumber);
  addConversion("Wavelength", factor, -0.5);

  addConversion("Momentum", 2 * M_PI / factor, 0.5);
}

void Energy_inWavenumber::validateUnitParams(const int, const UnitParametersMap &params) {
  if (!ParamPresent(params, UnitParams::l2)) {
    throw std::runtime_error("An l2 value must be supplied in the extra "
                             "parameters when initialising " +
                             this->unitID() + " for conversion via TOF");
  }
}

void Energy_inWavenumber::init() {
  double l2 = 0.0;
  ParamPresentAndSet(m_params, UnitParams::l2, l2);
  {
    const double TOFinMicroseconds = 1e6;
    factorTo =
        sqrt(PhysicalConstants::NeutronMass * PhysicalConstants::meVtoWavenumber / (2.0 * PhysicalConstants::meV)) *
        (l1 + l2) * TOFinMicroseconds;
  }
  {
    const double TOFisinMicroseconds = 1e-12; // The input tof number gets squared so this is (10E-6)^2
    const double ltot = l1 + l2;
    factorFrom = ((PhysicalConstants::NeutronMass / 2.0) * (ltot * ltot) * PhysicalConstants::meVtoWavenumber) /
                 (PhysicalConstants::meV * TOFisinMicroseconds);
  }
}

double Energy_inWavenumber::singleToTOF(const double x) const {
  double temp = x;
  if (temp <= DBL_MIN)
    temp = DBL_MIN; // Protect against divide by zero and define conversion range
  return factorTo / sqrt(temp);
}
///@return  Minimal time which can be reversibly converted into energy in
/// wavenumner units
double Energy_inWavenumber::conversionTOFMin() const { return factorTo / sqrt(std::numeric_limits<double>::max()); }
double Energy_inWavenumber::conversionTOFMax() const { return factorTo / sqrt(std::numeric_limits<double>::max()); }

double Energy_inWavenumber::singleFromTOF(const double tof) const {
  double temp = tof;
  if (temp == 0.0)
    temp = DBL_MIN; // Protect against divide by zero
  return factorFrom / (temp * temp);
}

Unit *Energy_inWavenumber::clone() const { return new Energy_inWavenumber(*this); }

// used in calculate of DIFC
const double NEUTRON_MASS_OVER_H = (PhysicalConstants::NeutronMass * 1e6) / (PhysicalConstants::h * 1e10);

/**
 * Calculate DIFC in case of logarithmic binning, used in CalculateDIFC with Signed mode
 * @param l1
 * @param l2
 * @param twotheta scattering angle
 * @param offset
 * @param binWidth the bin width used in logarithmic binning (DX)
 * Will calculate the value of DIFC following
 *   DIFC = (mn/h) * (L1+L2) * 2sin(theta) * (1 + |DX|)^{-offset}
 */
double calculateDIFCCorrection(const double l1, const double l2, const double twotheta, const double offset,
                               const double binWidth) {
  const double sinTheta = std::sin(twotheta / 2.0);
  const double newDIFC = NEUTRON_MASS_OVER_H * (l1 + l2) * 2.0 * sinTheta * pow((1.0 + fabs(binWidth)), -1.0 * offset);
  return newDIFC;
}

// ==================================================================================================
/* D-SPACING
 * ==================================================================================================
 *
 * Conversion uses Bragg's Law: 2d sin(theta) = n * lambda
 */

const double H_OVER_NEUTRON_MASS = (PhysicalConstants::h * 1e10) / (2.0 * PhysicalConstants::NeutronMass * 1e6);

/**
 * Calculate and return conversion factor from tof to d-spacing.
 * @param l1
 * @param l2
 * @param twoTheta scattering angle
 * @param offset
 * @return
 */
double tofToDSpacingFactor(const double l1, const double l2, const double twoTheta, const double offset) {
  if (offset <= -1.) // not physically possible, means result is negative d-spacing
  {
    std::stringstream msg;
    msg << "Encountered offset of " << offset << " which converts data to negative d-spacing\n";
    throw std::logic_error(msg.str());
  }

  auto sinTheta = std::sin(twoTheta / 2);

  const double numerator = (1.0 + offset);
  sinTheta *= (l1 + l2);

  return (numerator * H_OVER_NEUTRON_MASS) / sinTheta;
}

DECLARE_UNIT(dSpacing)

dSpacing::dSpacing() : Unit(), toDSpacingError(""), difa(0), difc(DBL_MIN), tzero(0) {
  const double factor = 2.0 * M_PI;
  addConversion("MomentumTransfer", factor, -1.0);
  addConversion("QSquared", (factor * factor), -2.0);
}

const UnitLabel dSpacing::label() const { return Symbol::Angstrom; }

Unit *dSpacing::clone() const { return new dSpacing(*this); }

void dSpacing::validateUnitParams(const int, const UnitParametersMap &params) {
  double difc_set = 0.;
  if (ParamPresentAndSet(&params, UnitParams::difc, difc_set)) {
    // check validations only applicable to fromTOF
    toDSpacingError = "";
    double difa_set = 0.;
    ParamPresentAndSet(&params, UnitParams::difa, difa_set);
    if ((difa_set == 0) && (difc_set == 0)) {
      toDSpacingError = "Cannot convert to d spacing with DIFA=0 and DIFC=0";
    };
    // singleFromTOF currently assuming difc not negative
    if (difc_set < 0.) {
      toDSpacingError = "A positive difc value must be supplied in the extra parameters when "
                        "initialising " +
                        this->unitID() + " for conversion via TOF";
    }
  } else {
    if (!ParamPresent(params, UnitParams::twoTheta) || (!ParamPresent(params, UnitParams::l2))) {
      throw std::runtime_error("A difc value or L2/two theta must be supplied "
                               "in the extra parameters when initialising " +
                               this->unitID() + " for conversion via TOF");
    }
  }
}

void dSpacing::init() {
  // First the crux of the conversion
  difa = 0.;
  difc = 0.;
  tzero = 0.;
  ParamPresentAndSet(m_params, UnitParams::difa, difa);
  ParamPresentAndSet(m_params, UnitParams::tzero, tzero);

  if (!ParamPresentAndSet(m_params, UnitParams::difc, difc)) {
    // also support inputs as L2, two theta
    double l2;
    if (ParamPresentAndSet(m_params, UnitParams::l2, l2)) {
      double twoTheta;
      if (ParamPresentAndSet(m_params, UnitParams::twoTheta, twoTheta)) {
        if (difa != 0.) {
          g_log.warning("Supplied difa ignored");
          difa = 0.;
        }
        difc = 1. / tofToDSpacingFactor(l1, l2, twoTheta, 0.);
        if (tzero != 0.) {
          g_log.warning("Supplied tzero ignored");
          tzero = 0.;
        }
      }
    }
  }
}

double dSpacing::singleToTOF(const double x) const {
  if (!isInitialized())
    throw std::runtime_error("dSpacingBase::singleToTOF called before object "
                             "has been initialized.");
  if (difa == 0.)
    return difc * x + tzero;
  else
    return difa * x * x + difc * x + tzero;
}

/**
 * DIFA * d^2 + DIFC * d + T0 - TOF = 0
 *
 * Use the citardauq formula to solve quadratic in order to minimise loss of precision. citardauq (quadratic spelled
 * backwards) is an alternate formulation of the quadratic formula. DIFC and sqrt term are often similar and the
 * "classic" quadratic formula involves calculating their difference in the numerator
 *
 *               2*(T0 - TOF)                                            (T0 - TOF)
 * d = -------------------------------------------  =  ---------------------------------------------------
 *     -DIFC -+ SQRT(DIFC^2 - 4*DIFA*(T0 - TOF))       0.5 * DIFC (-1 -+ SQRT(1 - 4*DIFA*(T0 - TOF)/DIFC^2)
 *
 * the variables in this formulation are the same as the quadratic formula
 * a = difa      square term
 * b = DIFC      linear term - assumed to be positive
 * c = T0 - TOF  constant term
 */
double dSpacing::singleFromTOF(const double tof) const {
  // dealing with various edge cases
  if (!isInitialized())
    throw std::runtime_error("dSpacingBase::singleFromTOF called before object "
                             "has been initialized.");
  if (!toDSpacingError.empty())
    throw std::runtime_error(toDSpacingError);

  // this is with the opposite sign from the equation above
  // as it reduces number of individual flops
  const double negativeConstantTerm = tof - tzero;

  // don't need to solve a quadratic when difa==0
  // this allows negative d-spacing to be returned
  // which was the behavior before v6.2 was released
  if (difa == 0.)
    return negativeConstantTerm / difc;

  // non-physical result
  if (tzero > tof) {
    if (difa > 0.) {
      throw std::runtime_error("Cannot convert to d spacing because tzero > time-of-flight and difa is positive. "
                               "Quadratic doesn't have a positive root");
    }
  }

  // citardauq formula hides non-zero root if tof==tzero
  // wich means that the constantTerm == 0
  if (tof == tzero) {
    if (difa < 0.)
      return -difc / difa;
    else
      return 0.;
  }

  // general citarqauq equation
  const double sqrtTerm = 1 + 4 * difa * negativeConstantTerm / (difc * difc);
  if (sqrtTerm < 0.) {
    throw std::runtime_error("Cannot convert to d spacing. Quadratic doesn't have real roots");
  }
  // pick smallest positive root. Since difc is positive it just depends on sign of constantTerm
  // Note - constantTerm is generally negative
  if (negativeConstantTerm < 0)
    // single positive root
    return negativeConstantTerm / (0.5 * difc * (1 - sqrt(sqrtTerm)));
  else
    // two positive roots. pick most negative denominator to get smallest root
    return negativeConstantTerm / (0.5 * difc * (1 + sqrt(sqrtTerm)));
}

double dSpacing::conversionTOFMin() const {
  // quadratic only has a min if difa is positive
  if (difa > 0) {
    // min of the quadratic is at d=-difc/(2*difa)
    return std::max(0., tzero - difc * difc / (4 * difa));
  } else {
    // no min so just pick value closest to zero that works
    double TOFmin = singleToTOF(0.);
    if (TOFmin < std::numeric_limits<double>::min()) {
      TOFmin = 0.;
    }
    return TOFmin;
  }
}

double dSpacing::conversionTOFMax() const {
  // quadratic only has a max if difa is negative
  if (difa < 0) {
    return std::min(DBL_MAX, tzero - difc * difc / (4 * difa));
  } else {
    // no max so just pick value closest to DBL_MAX that works
    double TOFmax = singleToTOF(DBL_MAX);
    if (std::isinf(TOFmax)) {
      TOFmax = DBL_MAX;
    }
    return TOFmax;
  }
}

double dSpacing::calcTofMin(const double difc, const double difa, const double tzero, const double tofmin) {
  Kernel::UnitParametersMap params{
      {Kernel::UnitParams::difa, difa}, {Kernel::UnitParams::difc, difc}, {Kernel::UnitParams::tzero, tzero}};
  initialize(-1., 0, params);
  return std::max(conversionTOFMin(), tofmin);
}

double dSpacing::calcTofMax(const double difc, const double difa, const double tzero, const double tofmax) {
  Kernel::UnitParametersMap params{
      {Kernel::UnitParams::difa, difa}, {Kernel::UnitParams::difc, difc}, {Kernel::UnitParams::tzero, tzero}};
  initialize(-1, 0, params);
  return std::min(conversionTOFMax(), tofmax);
}

// ==================================================================================================
/* D-SPACING Perpendicular
 * ==================================================================================================
 *
 * Conversion uses equation: dp^2 = lambda^2 - 2[Angstrom^2]*ln(cos(theta))
 */
DECLARE_UNIT(dSpacingPerpendicular)

const UnitLabel dSpacingPerpendicular::label() const { return Symbol::Angstrom; }

dSpacingPerpendicular::dSpacingPerpendicular() : Unit(), factorTo(DBL_MIN), factorFrom(DBL_MIN) {}

void dSpacingPerpendicular::validateUnitParams(const int, const UnitParametersMap &params) {
  if (!ParamPresent(params, UnitParams::l2)) {
    throw std::runtime_error("A l2 value must be supplied in the extra parameters when "
                             "initialising " +
                             this->unitID() + " for conversion via TOF");
  }
  if (!ParamPresent(params, UnitParams::twoTheta)) {
    throw std::runtime_error("A two theta value must be supplied in the extra parameters when "
                             "initialising " +
                             this->unitID() + " for conversion via TOF");
  }
}

void dSpacingPerpendicular::init() {
  double l2 = 0.0;
  ParamPresentAndSet(m_params, UnitParams::l2, l2);
  ParamPresentAndSet(m_params, UnitParams::twoTheta, twoTheta);
  factorTo = (PhysicalConstants::NeutronMass * (l1 + l2)) / PhysicalConstants::h;

  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factorTo *= TOFisinMicroseconds / toAngstroms;
  factorFrom = factorTo;
  if (factorFrom == 0.0)
    factorFrom = DBL_MIN; // Protect against divide by zero
  double cos_theta = cos(twoTheta / 2.0);
  sfpTo = 0.0;
  if (cos_theta > 0)
    sfpTo = 2.0 * log(cos_theta);
  sfpFrom = sfpTo;
}

double dSpacingPerpendicular::singleToTOF(const double x) const {
  double sqrtarg = x * x + sfpTo;
  // consider very small values to be a rounding error
  if (sqrtarg < 1.0e-17)
    return 0.0;
  return sqrt(sqrtarg) * factorTo;
}
double dSpacingPerpendicular::singleFromTOF(const double tof) const {
  double temp = tof / factorFrom;
  return sqrt(temp * temp - sfpFrom);
}
double dSpacingPerpendicular::conversionTOFMin() const { return sqrt(-1.0 * sfpFrom); }
double dSpacingPerpendicular::conversionTOFMax() const { return sqrt(std::numeric_limits<double>::max()) / factorFrom; }

Unit *dSpacingPerpendicular::clone() const { return new dSpacingPerpendicular(*this); }

// ================================================================================
/* MOMENTUM TRANSFER
 * ================================================================================
 *
 * The relationship is Q = 2k sin (theta). where k is 2*pi/wavelength
 */
DECLARE_UNIT(MomentumTransfer)

const UnitLabel MomentumTransfer::label() const { return Symbol::InverseAngstrom; }

MomentumTransfer::MomentumTransfer() : Unit() {
  addConversion("QSquared", 1.0, 2.0);
  const double factor = 2.0 * M_PI;
  addConversion("dSpacing", factor, -1.0);
}

void MomentumTransfer::validateUnitParams(const int, const UnitParametersMap &params) {
  double difc_set = 0.;
  if (!ParamPresentAndSet(&params, UnitParams::difc, difc_set)) {
    if (!ParamPresent(params, UnitParams::twoTheta) || (!ParamPresent(params, UnitParams::l2)))
      throw std::runtime_error("A difc value or L2/two theta must be supplied "
                               "in the extra parameters when initialising " +
                               this->unitID() + " for conversion via TOF");
  };
}

void MomentumTransfer::init() {
  // First the crux of the conversion
  difc = 0.;

  if (!ParamPresentAndSet(m_params, UnitParams::difc, difc)) {
    // also support inputs as L2, two theta
    double l2;
    if (ParamPresentAndSet(m_params, UnitParams::l2, l2)) {
      double twoTheta;
      if (ParamPresentAndSet(m_params, UnitParams::twoTheta, twoTheta)) {
        difc = 1. / tofToDSpacingFactor(l1, l2, twoTheta, 0.);
      }
    }
  }
}

double MomentumTransfer::singleToTOF(const double x) const { return 2. * M_PI * difc / x; }
//
double MomentumTransfer::singleFromTOF(const double tof) const { return 2. * M_PI * difc / tof; }

double MomentumTransfer::conversionTOFMin() const { return 2. * M_PI * difc / DBL_MAX; }
double MomentumTransfer::conversionTOFMax() const { return DBL_MAX; }

Unit *MomentumTransfer::clone() const { return new MomentumTransfer(*this); }

/* ===================================================================================================
 * Q-SQUARED
 * ===================================================================================================
 */
DECLARE_UNIT(QSquared)

const UnitLabel QSquared::label() const { return Symbol::InverseAngstromSq; }

QSquared::QSquared() : MomentumTransfer() {
  addConversion("MomentumTransfer", 1.0, 0.5);
  const double factor = 2.0 * M_PI;
  addConversion("dSpacing", factor, -0.5);
}

double QSquared::singleToTOF(const double x) const { return MomentumTransfer::singleToTOF(sqrt(x)); }
double QSquared::singleFromTOF(const double tof) const { return pow(MomentumTransfer::singleFromTOF(tof), 2); }

double QSquared::conversionTOFMin() const { return 2 * M_PI * difc / sqrt(DBL_MAX); }
double QSquared::conversionTOFMax() const {
  double tofmax = 2 * M_PI * difc / sqrt(DBL_MIN);
  if (std::isinf(tofmax))
    tofmax = DBL_MAX;
  return tofmax;
}

Unit *QSquared::clone() const { return new QSquared(*this); }

/* ==============================================================================
 * Energy Transfer
 * ==============================================================================
 */
DECLARE_UNIT(DeltaE)

const UnitLabel DeltaE::label() const { return Symbol::MilliElectronVolts; }

DeltaE::DeltaE()
    : Unit(), factorTo(DBL_MIN), factorFrom(DBL_MIN), t_other(DBL_MIN), t_otherFrom(DBL_MIN), unitScaling(DBL_MIN) {
  addConversion("DeltaE_inWavenumber", PhysicalConstants::meVtoWavenumber, 1.);
  addConversion("DeltaE_inFrequency", PhysicalConstants::meVtoFrequency, 1.);
}

void DeltaE::validateUnitParams(const int emode, const UnitParametersMap &params) {
  if (emode != 1 && emode != 2) {
    throw std::invalid_argument("emode must be equal to 1 or 2 for energy transfer calculation");
  }
  // Efixed must be set to something
  double efixed_set;
  if (!ParamPresentAndSet(&params, UnitParams::efixed, efixed_set)) {
    if (emode == 1) { // direct, efixed=ei
      throw std::invalid_argument("efixed must be set for energy transfer calculation");
    } else {
      throw std::runtime_error("efixed must be set for energy transfer calculation");
    }
  }
  if (efixed_set <= 0) {
    throw std::runtime_error("efixed must be greater than zero");
  }
  if (!ParamPresent(params, UnitParams::l2)) {
    throw std::runtime_error("A l2 value must be supplied in the extra parameters when "
                             "initialising " +
                             this->unitID() + " for conversion via TOF");
  }
}

void DeltaE::init() {
  double l2 = 0.0;
  ParamPresentAndSet(m_params, UnitParams::l2, l2);
  ParamPresentAndSet(m_params, UnitParams::efixed, efixed);
  const double TOFinMicroseconds = 1e6;
  factorTo = sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) * TOFinMicroseconds;
  if (emode == 1) {
    // t_other is t1
    t_other = (factorTo * l1) / sqrt(efixed);
    factorTo *= l2;
  } else if (emode == 2) {
    // t_other is t2
    t_other = (factorTo * l2) / sqrt(efixed);
    factorTo *= l1;
  }

  //------------ from conversion ------------------
  factorFrom = sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) * TOFinMicroseconds;

  if (emode == 1) {
    // t_otherFrom = t1
    t_otherFrom = (factorFrom * l1) / sqrt(efixed);
    factorFrom = factorFrom * factorFrom * l2 * l2;
  } else if (emode == 2) {
    // t_otherFrom = t2
    t_otherFrom = (factorFrom * l2) / sqrt(efixed);
    factorFrom = factorFrom * factorFrom * l1 * l1;
  }

  // This will be changed for the wavenumber one
  unitScaling = 1;
}

double DeltaE::singleToTOF(const double x) const {
  if (emode == 1) {
    const double e2 = efixed - x / unitScaling;
    if (e2 <= 0.0) // This shouldn't ever happen (unless the efixed value is wrong)
      return DeltaE::conversionTOFMax();
    else {
      // this_t = t2;
      const double this_t = factorTo / sqrt(e2);
      return this_t + t_other; // (t1+t2);
    }
  } else if (emode == 2) {
    const double e1 = efixed + x / unitScaling;
    if (e1 <= 0.0) // This shouldn't ever happen (unless the efixed value is wrong)
      return DeltaE::conversionTOFMax();
    else {
      // this_t = t1;
      const double this_t = factorTo / sqrt(e1);
      return this_t + t_other; // (t1+t2);
    }
  } else {
    return DeltaE::conversionTOFMax();
  }
}

double DeltaE::singleFromTOF(const double tof) const {
  if (emode == 1) {
    // This is t2
    const double this_t = tof - t_otherFrom;
    if (this_t <= 0.0)
      return -DBL_MAX;
    else {
      const double e2 = factorFrom / (this_t * this_t);
      return (efixed - e2) * unitScaling;
    }
  } else if (emode == 2) {
    // This is t1
    const double this_t = tof - t_otherFrom;
    if (this_t <= 0.0)
      return DBL_MAX;
    else {
      const double e1 = factorFrom / (this_t * this_t);
      return (e1 - efixed) * unitScaling;
    }
  } else
    return DBL_MAX;
}

double DeltaE::conversionTOFMin() const {
  double time(DBL_MAX); // impossible for elastic, this units do not work for elastic
  if (emode == 1 || emode == 2)
    time = t_otherFrom * (1 + DBL_EPSILON);
  return time;
}
double DeltaE::conversionTOFMax() const {
  // 0.1 here to provide at least two significant units to conversion range as
  // this conversion range comes from 1-epsilon
  if (efixed > 1)
    return t_otherFrom + sqrt(factorFrom / efixed) / sqrt(DBL_MIN);
  else
    return t_otherFrom + sqrt(factorFrom) / sqrt(DBL_MIN);
}

Unit *DeltaE::clone() const { return new DeltaE(*this); }

// =====================================================================================================
/* Energy Transfer in units of wavenumber
 * =====================================================================================================
 *
 * This is identical to the above (Energy Transfer in meV) with one division
 *by meVtoWavenumber.
 */
DECLARE_UNIT(DeltaE_inWavenumber)

const UnitLabel DeltaE_inWavenumber::label() const { return Symbol::InverseCM; }

void DeltaE_inWavenumber::init() {
  DeltaE::init();
  // Change the unit scaling factor
  unitScaling = PhysicalConstants::meVtoWavenumber;
}

Unit *DeltaE_inWavenumber::clone() const { return new DeltaE_inWavenumber(*this); }

DeltaE_inWavenumber::DeltaE_inWavenumber() : DeltaE() {
  addConversion("DeltaE", 1 / PhysicalConstants::meVtoWavenumber, 1.);
}

// =====================================================================================================
/* Energy Transfer in units of frequency
 * =====================================================================================================
 *
 * This is identical to Energy Transfer in meV, with one division by Plank's
 *constant, or multiplication
 * by factor PhysicalConstants::meVtoFrequency
 */
DECLARE_UNIT(DeltaE_inFrequency)

const UnitLabel DeltaE_inFrequency::label() const { return Symbol::GHz; }

void DeltaE_inFrequency::init() {
  DeltaE::init();
  // Change the unit scaling factor
  unitScaling = PhysicalConstants::meVtoFrequency;
}

Unit *DeltaE_inFrequency::clone() const { return new DeltaE_inFrequency(*this); }

DeltaE_inFrequency::DeltaE_inFrequency() : DeltaE() {
  addConversion("DeltaE", 1.0 / PhysicalConstants::meVtoFrequency, 1.);
}

// =====================================================================================================
/* Momentum in Angstrom^-1. It is 2*Pi/wavelength
 * =====================================================================================================
 */
DECLARE_UNIT(Momentum)

const UnitLabel Momentum::label() const { return Symbol::InverseAngstrom; }

Momentum::Momentum()
    : Unit(), efixed(0.), sfpTo(DBL_MIN), factorTo(DBL_MIN), sfpFrom(DBL_MIN), factorFrom(DBL_MIN), do_sfpFrom(false) {

  const double AngstromsSquared = 1e20;
  const double factor = (AngstromsSquared * PhysicalConstants::h * PhysicalConstants::h) /
                        (2.0 * PhysicalConstants::NeutronMass * PhysicalConstants::meV) / (4 * M_PI * M_PI);

  addConversion("Energy", factor, 2.0);
  addConversion("Energy_inWavenumber", factor * PhysicalConstants::meVtoWavenumber, 2.0);
  addConversion("Wavelength", 2 * M_PI, -1.0);
  //
}

void Momentum::validateUnitParams(const int emode, const UnitParametersMap &params) {
  if (!ParamPresent(params, UnitParams::l2)) {
    throw std::runtime_error("An l2 value must be supplied in the extra parameters when "
                             "initialising momentum for conversion via TOF");
  }
  if ((emode != 0) && (!ParamPresent(params, UnitParams::efixed))) {
    throw std::runtime_error("An efixed value must be supplied in the extra parameters when "
                             "initialising momentum for conversion via TOF");
  }
}

void Momentum::init() {
  // ------------ Factors to convert TO TOF ---------------------
  double l2 = 0.0;
  double ltot = 0.0;
  double TOFisinMicroseconds = 1e6;
  double toAngstroms = 1e10;
  sfpTo = 0.0;

  ParamPresentAndSet(m_params, UnitParams::l2, l2);
  ParamPresentAndSet(m_params, UnitParams::efixed, efixed);

  if (emode == 1) {
    ltot = l2;
    sfpTo = (sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) * TOFisinMicroseconds * l1) /
            sqrt(efixed);
  } else if (emode == 2) {
    ltot = l1;
    sfpTo = (sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) * TOFisinMicroseconds * l2) /
            sqrt(efixed);
  } else {
    ltot = l1 + l2;
  }
  factorTo = 2 * M_PI * (PhysicalConstants::NeutronMass * (ltot)) / PhysicalConstants::h;
  // Now adjustments for the scale of units used
  factorTo *= TOFisinMicroseconds / toAngstroms;

  // ------------ Factors to convert FROM TOF ---------------------

  // Now apply the factor to the input data vector
  do_sfpFrom = false;
  if (efixed != DBL_MIN) {
    if (emode == 1) // Direct
    {
      sfpFrom = sfpTo;
      do_sfpFrom = true;
    } else if (emode == 2) // Indirect
    {
      sfpFrom = sfpTo;
      do_sfpFrom = true;
    }
  }

  // Protect against divide by zero
  if (ltot == 0.0)
    ltot = DBL_MIN;

  // First the crux of the conversion
  factorFrom = PhysicalConstants::h / (PhysicalConstants::NeutronMass * (ltot));

  // Now adjustments for the scale of units used
  factorFrom *= toAngstroms / TOFisinMicroseconds;
  factorFrom = 2 * M_PI / factorFrom;
}

double Momentum::singleToTOF(const double ki) const {
  double tof = factorTo / ki;
  // If Direct or Indirect we want to correct TOF values..
  if (emode == 1 || emode == 2)
    tof += sfpTo;
  return tof;
}
double Momentum::conversionTOFMin() const {
  double range = DBL_MIN * factorFrom;
  if (emode == 1 || emode == 2)
    range = sfpFrom * (1 + DBL_EPSILON * factorFrom);
  return range;
}
double Momentum::conversionTOFMax() const {
  double range = DBL_MAX / factorTo;
  if (emode == 1 || emode == 2) {
    range = 1 + DBL_MAX / factorTo + sfpFrom;
  }
  return range;
}

double Momentum::singleFromTOF(const double tof) const {
  double x = tof;
  if (do_sfpFrom)
    x -= sfpFrom;
  if (x == 0)
    x = DBL_MIN;

  return factorFrom / x;
}

Unit *Momentum::clone() const { return new Momentum(*this); }

// ============================================================================================
/* SpinEchoLength
 * ===================================================================================================
 *
 * Delta = (constant)*(wavelength)^2
 */
DECLARE_UNIT(SpinEchoLength)

const UnitLabel SpinEchoLength::label() const { return Symbol::Nanometre; }

SpinEchoLength::SpinEchoLength() : Wavelength() {}

void SpinEchoLength::init() {
  ParamPresentAndSet(m_params, UnitParams::efixed, efixed);
  // Efixed must be set to something
  if (efixed == 0.0)
    throw std::invalid_argument("efixed must be set for spin echo length calculation");
  if (emode > 0) {
    throw std::invalid_argument("emode must be equal to 0 for spin echo length calculation");
  }
  Wavelength::init();
}

double SpinEchoLength::singleToTOF(const double x) const {
  double wavelength = sqrt(x / efixed);
  double tof = Wavelength::singleToTOF(wavelength);
  return tof;
}

double SpinEchoLength::conversionTOFMin() const {
  double wl = Wavelength::conversionTOFMin();
  return efixed * wl * wl;
}
double SpinEchoLength::conversionTOFMax() const {
  double sel = sqrt(DBL_MAX);
  if (efixed > 1) {
    sel /= efixed;
  }

  return sel;
}

double SpinEchoLength::singleFromTOF(const double tof) const {
  double wavelength = Wavelength::singleFromTOF(tof);
  double x = efixed * wavelength * wavelength;
  return x;
}

Unit *SpinEchoLength::clone() const { return new SpinEchoLength(*this); }

// ============================================================================================
/* SpinEchoTime
 * ===================================================================================================
 *
 * Tau = (constant)*(wavelength)^3
 */
DECLARE_UNIT(SpinEchoTime)

const UnitLabel SpinEchoTime::label() const { return Symbol::Nanosecond; }

SpinEchoTime::SpinEchoTime() : Wavelength(), efixed(0.) {}

void SpinEchoTime::init() {
  ParamPresentAndSet(m_params, UnitParams::efixed, efixed);
  // Efixed must be set to something
  if (efixed == 0.0)
    throw std::invalid_argument("efixed must be set for spin echo time calculation");
  if (emode > 0) {
    throw std::invalid_argument("emode must be equal to 0 for spin echo time calculation");
  }
  Wavelength::init();
}

double SpinEchoTime::singleToTOF(const double x) const {
  double wavelength = pow(x / efixed, 1.0 / 3.0);
  double tof = Wavelength::singleToTOF(wavelength);
  return tof;
}
double SpinEchoTime::conversionTOFMin() const { return 0; }
double SpinEchoTime::conversionTOFMax() const {
  double tm = std::pow(DBL_MAX, 1. / 3.);
  if (efixed > 1)
    tm /= efixed;
  return tm;
}

double SpinEchoTime::singleFromTOF(const double tof) const {
  double wavelength = Wavelength::singleFromTOF(tof);
  double x = efixed * wavelength * wavelength * wavelength;
  return x;
}

Unit *SpinEchoTime::clone() const { return new SpinEchoTime(*this); }

// ================================================================================
/* Time
 * ================================================================================
 *
 * Time is an independant unit to others related to energy and neutron
 */
DECLARE_UNIT(Time)

const UnitLabel Time::label() const { return Symbol::Second; }

Time::Time() : Unit(), factorTo(DBL_MIN), factorFrom(DBL_MIN) {}

void Time::init() {}

double Time::singleToTOF(const double x) const {
  UNUSED_ARG(x);
  throw std::runtime_error("Time is not allowed to be convert to TOF. ");
}

double Time::singleFromTOF(const double tof) const {
  UNUSED_ARG(tof);
  throw std::runtime_error("Time is not allowed to be converted from TOF. ");
}

double Time::conversionTOFMax() const { return std::numeric_limits<double>::quiet_NaN(); }
double Time::conversionTOFMin() const { return std::numeric_limits<double>::quiet_NaN(); }

Unit *Time::clone() const { return new Time(*this); }

// ================================================================================
/* Degrees
 * ================================================================================
 *
 * Degrees prints degrees as a label
 */

DECLARE_UNIT(Degrees)

Degrees::Degrees() : Empty(), m_label("degrees") {}

const UnitLabel Degrees::label() const { return m_label; }

double Degrees::singleToTOF(const double x) const {
  UNUSED_ARG(x);
  throw std::runtime_error("Degrees is not allowed to be converted to TOF. ");
}

double Degrees::singleFromTOF(const double tof) const {
  UNUSED_ARG(tof);
  throw std::runtime_error("Degrees is not allowed to be converted from TOF. ");
}

Unit *Degrees::clone() const { return new Degrees(*this); }

// Phi
DECLARE_UNIT(Phi)

// ================================================================================
/* Temperature in kelvin
 * ================================================================================
 *
 * TemperatureKelvin prints Temperature in units of Kelvin as a label
 */

DECLARE_UNIT(Temperature)

Temperature::Temperature() : Empty(), m_label("K") {}

const UnitLabel Temperature::label() const { return m_label; }

double Temperature::singleToTOF(const double x) const {
  UNUSED_ARG(x);
  throw std::runtime_error("Temperature is not allowed to be converted to TOF. ");
}

double Temperature::singleFromTOF(const double tof) const {
  UNUSED_ARG(tof);
  throw std::runtime_error("Temperature is not allowed to be converted from TOF. ");
}

Unit *Temperature::clone() const { return new Temperature(*this); }

// =====================================================================================================
/* Atomic Distance in units of Angstroms
 * =====================================================================================================
 *
 * The distance from the center of an atom in Angstroms
 */
DECLARE_UNIT(AtomicDistance)

AtomicDistance::AtomicDistance() : Empty(), m_label("Atomic Distance") {}

const UnitLabel AtomicDistance::label() const { return Symbol::Angstrom; }

Unit *AtomicDistance::clone() const { return new AtomicDistance(*this); }

double AtomicDistance::singleToTOF(const double x) const {
  UNUSED_ARG(x);
  throw std::runtime_error("Atomic Distance is not allowed to be converted to TOF. ");
}

double AtomicDistance::singleFromTOF(const double tof) const {
  UNUSED_ARG(tof);
  throw std::runtime_error("Atomic Distance is not allowed to be converted from TOF. ");
}

// ================================================================================

double timeConversionValue(const std::string &input_unit, const std::string &output_unit) {
  std::map<std::string, double> timesList;
  double seconds = 1.0e9;
  double milliseconds = 1.0e-3 * seconds;
  double microseconds = 1.0e-3 * milliseconds;
  double nanoseconds = 1.0e-3 * microseconds;

  timesList["seconds"] = seconds;
  timesList["second"] = seconds;
  timesList["s"] = seconds;
  timesList["milliseconds"] = milliseconds;
  timesList["millisecond"] = milliseconds;
  timesList["ms"] = milliseconds;
  timesList["microseconds"] = microseconds;
  timesList["microsecond"] = microseconds;
  timesList["us"] = microseconds;
  timesList["nanoseconds"] = nanoseconds;
  timesList["nanosecond"] = nanoseconds;
  timesList["ns"] = nanoseconds;

  double input_float = timesList[input_unit];
  double output_float = timesList[output_unit];
  if (input_float == 0)
    throw std::runtime_error("timeConversionValue: input unit " + input_unit + " not known.");
  if (output_float == 0)
    throw std::runtime_error("timeConversionValue: output unit " + input_unit + " not known.");
  return input_float / output_float;
}

} // namespace Units

} // namespace Mantid::Kernel
