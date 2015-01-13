//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Unit.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"
#include <cmath>
#include <cfloat>
#include <limits>

namespace Mantid {
namespace Kernel {

/**
 * Default constructor
 * Gives the unit an empty UnitLabel
 */
Unit::Unit()
    : initialized(false), l1(0), l2(0), twoTheta(0), emode(0), efixed(0),
      delta(0) {}

/**
 */
Unit::~Unit() {}

/**
 * @param other The unit that initializes this
 */
Unit::Unit(const Unit &other) {
  // call assignment operator for everything else
  *this = other;
}

/**
 * @param rhs A unit object whose state is copied to this
 * @return A reference to this object
 */
Unit &Unit::operator=(const Unit &rhs) {
  if (this != &rhs) {
    initialized = rhs.initialized;
    l1 = rhs.l1;
    l2 = rhs.l2;
    twoTheta = rhs.twoTheta;
    emode = rhs.emode;
    efixed = rhs.efixed;
    delta = rhs.delta;
  }
  return *this;
}

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
bool Unit::quickConversion(const Unit &destination, double &factor,
                           double &power) const {
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
bool Unit::quickConversion(std::string destUnitName, double &factor,
                           double &power) const {
  // From the global map, try to get the map holding the conversions for this
  // unit
  ConversionsMap::const_iterator it = s_conversionFactors.find(unitID());
  // Return false if there are no conversions entered for this unit
  if (it == s_conversionFactors.end())
    return false;

  // See if there's a conversion listed for the requested destination unit
  std::transform(destUnitName.begin(), destUnitName.end(), destUnitName.begin(),
                 toupper);
  UnitConversions::const_iterator iter = it->second.find(destUnitName);
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
void Unit::addConversion(std::string to, const double &factor,
                         const double &power) const {
  std::transform(to.begin(), to.end(), to.begin(), toupper);
  // If this happens in a parallel loop the static map needs protecting
  PARALLEL_CRITICAL(Unit_addConversion) {
    // Add the conversion to the map (does nothing if it's already there)
    s_conversionFactors[unitID()][to] = std::make_pair(factor, power);
  }
}

//---------------------------------------------------------------------------------------
/** Removes all registered 'quick conversions' from the unit class on which this
 * method is called.
 */
void Unit::clearConversions() const { s_conversionFactors.clear(); }

//---------------------------------------------------------------------------------------
/** Initialize the unit to perform conversion using singleToTof() and
 *singleFromTof()
 *
 *  @param _l1 ::       The source-sample distance (in metres)
 *  @param _l2 ::       The sample-detector distance (in metres)
 *  @param _twoTheta :: The scattering angle (in radians)
 *  @param _emode ::    The energy mode (0=elastic, 1=direct geometry,
 *2=indirect geometry)
 *  @param _efixed ::   Value of fixed energy: EI (emode=1) or EF (emode=2) (in
 *meV)
 *  @param _delta ::    Not currently used
 */
void Unit::initialize(const double &_l1, const double &_l2,
                      const double &_twoTheta, const int &_emode,
                      const double &_efixed, const double &_delta) {
  l1 = _l1;
  l2 = _l2;
  twoTheta = _twoTheta;
  emode = _emode;
  efixed = _efixed;
  delta = _delta;
  initialized = true;
  this->init();
}

//---------------------------------------------------------------------------------------
/** Perform the conversion to TOF on a vector of data */
void Unit::toTOF(std::vector<double> &xdata, std::vector<double> &ydata,
                 const double &_l1, const double &_l2, const double &_twoTheta,
                 const int &_emode, const double &_efixed,
                 const double &_delta) {
  UNUSED_ARG(ydata);
  this->initialize(_l1, _l2, _twoTheta, _emode, _efixed, _delta);
  size_t numX = xdata.size();
  for (size_t i = 0; i < numX; i++)
    xdata[i] = this->singleToTOF(xdata[i]);
}

/** Convert a single value to TOF */
double Unit::convertSingleToTOF(const double xvalue, const double &l1,
                                const double &l2, const double &twoTheta,
                                const int &emode, const double &efixed,
                                const double &delta) {
  this->initialize(l1, l2, twoTheta, emode, efixed, delta);
  return this->singleToTOF(xvalue);
}

//---------------------------------------------------------------------------------------
/** Perform the conversion to TOF on a vector of data */
void Unit::fromTOF(std::vector<double> &xdata, std::vector<double> &ydata,
                   const double &_l1, const double &_l2,
                   const double &_twoTheta, const int &_emode,
                   const double &_efixed, const double &_delta) {
  UNUSED_ARG(ydata);
  this->initialize(_l1, _l2, _twoTheta, _emode, _efixed, _delta);
  size_t numX = xdata.size();
  for (size_t i = 0; i < numX; i++)
    xdata[i] = this->singleFromTOF(xdata[i]);
}

/** Convert a single value from TOF */
double Unit::convertSingleFromTOF(const double xvalue, const double &l1,
                                  const double &l2, const double &twoTheta,
                                  const int &emode, const double &efixed,
                                  const double &delta) {
  this->initialize(l1, l2, twoTheta, emode, efixed, delta);
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
  throw Kernel::Exception::NotImplementedError(
      "Cannot convert unit " + this->unitID() + " to time of flight");
}

double Empty::singleFromTOF(const double tof) const {
  UNUSED_ARG(tof);
  throw Kernel::Exception::NotImplementedError(
      "Cannot convert to unit " + this->unitID() + " from time of flight");
}

Unit *Empty::clone() const { return new Empty(*this); }

/**
 * @return NaN as Label can not be obtained from TOF in any reasonable manner
 */
double Empty::conversionTOFMin() const {
  return std::numeric_limits<double>::quiet_NaN();
}

/**
 * @return NaN as Label can not be obtained from TOF in any reasonable manner
 */
double Empty::conversionTOFMax() const {
  return std::numeric_limits<double>::quiet_NaN();
}

/* =============================================================================
 * LABEL
 * =============================================================================
 */

DECLARE_UNIT(Label)

const UnitLabel Label::label() const { return m_label; }

/// Constructor
Label::Label() : Empty(), m_caption("Quantity"), m_label(Symbol::EmptyLabel) {}

Label::Label(const std::string &caption, const std::string &label)
    : Empty(), m_caption(), m_label(Symbol::EmptyLabel) {
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

Wavelength::Wavelength() : Unit() {
  const double AngstromsSquared = 1e20;
  const double factor =
      (AngstromsSquared * PhysicalConstants::h * PhysicalConstants::h) /
      (2.0 * PhysicalConstants::NeutronMass * PhysicalConstants::meV);
  addConversion("Energy", factor, -2.0);
  addConversion("Energy_inWavenumber",
                factor * PhysicalConstants::meVtoWavenumber, -2.0);
  addConversion("Momentum", 2 * M_PI, -1.0);
}

const UnitLabel Wavelength::label() const { return Symbol::Angstrom; }

void Wavelength::init() {
  // ------------ Factors to convert TO TOF ---------------------
  double ltot = 0.0;
  double TOFisinMicroseconds = 1e6;
  double toAngstroms = 1e10;
  sfpTo = 0.0;

  if (emode == 1) {
    ltot = l2;
    sfpTo =
        (sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) *
         TOFisinMicroseconds * l1) /
        sqrt(efixed);
  } else if (emode == 2) {
    ltot = l1;
    sfpTo =
        (sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) *
         TOFisinMicroseconds * l2) /
        sqrt(efixed);
  } else {
    ltot = l1 + l2;
  }
  factorTo = (PhysicalConstants::NeutronMass * (ltot)) / PhysicalConstants::h;
  // Now adjustments for the scale of units used
  factorTo *= TOFisinMicroseconds / toAngstroms;

  // ------------ Factors to convert FROM TOF ---------------------
  ltot = l1 + l2;
  // Protect against divide by zero
  if (ltot == 0.0)
    ltot = DBL_MIN;

  // Now apply the factor to the input data vector
  do_sfpFrom = false;
  if (efixed != DBL_MIN) {
    if (emode == 1) // Direct
    {
      ltot = l2;
      sfpFrom = (sqrt(PhysicalConstants::NeutronMass /
                      (2.0 * PhysicalConstants::meV)) *
                 TOFisinMicroseconds * l1) /
                sqrt(efixed);
      do_sfpFrom = true;
    } else if (emode == 2) // Indirect
    {
      ltot = l1;
      sfpFrom = (sqrt(PhysicalConstants::NeutronMass /
                      (2.0 * PhysicalConstants::meV)) *
                 TOFisinMicroseconds * l2) /
                sqrt(efixed);
      do_sfpFrom = true;
    } else {
      ltot = l1 + l2;
    }
  } else {
    ltot = l1 + l2;
  }

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
Energy::Energy() : Unit() {
  addConversion("Energy_inWavenumber", PhysicalConstants::meVtoWavenumber);
  const double toAngstroms = 1e10;
  const double factor =
      toAngstroms * PhysicalConstants::h /
      sqrt(2.0 * PhysicalConstants::NeutronMass * PhysicalConstants::meV);
  addConversion("Wavelength", factor, -0.5);
  addConversion("Momentum", 2 * M_PI / factor, 0.5);
}

void Energy::init() {
  {
    const double TOFinMicroseconds = 1e6;
    factorTo =
        sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) *
        (l1 + l2) * TOFinMicroseconds;
  }
  {
    const double TOFisinMicroseconds =
        1e-12; // The input tof number gets squared so this is (10E-6)^2
    const double ltot = l1 + l2;
    factorFrom = ((PhysicalConstants::NeutronMass / 2.0) * (ltot * ltot)) /
                 (PhysicalConstants::meV * TOFisinMicroseconds);
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
Energy_inWavenumber::Energy_inWavenumber() : Unit() {
  addConversion("Energy", 1.0 / PhysicalConstants::meVtoWavenumber);
  const double toAngstroms = 1e10;
  const double factor =
      toAngstroms * PhysicalConstants::h /
      sqrt(2.0 * PhysicalConstants::NeutronMass * PhysicalConstants::meV /
           PhysicalConstants::meVtoWavenumber);
  addConversion("Wavelength", factor, -0.5);

  addConversion("Momentum", 2 * M_PI / factor, 0.5);
}

void Energy_inWavenumber::init() {
  {
    const double TOFinMicroseconds = 1e6;
    factorTo = sqrt(PhysicalConstants::NeutronMass *
                    PhysicalConstants::meVtoWavenumber /
                    (2.0 * PhysicalConstants::meV)) *
               (l1 + l2) * TOFinMicroseconds;
  }
  {
    const double TOFisinMicroseconds =
        1e-12; // The input tof number gets squared so this is (10E-6)^2
    const double ltot = l1 + l2;
    factorFrom = ((PhysicalConstants::NeutronMass / 2.0) * (ltot * ltot) *
                  PhysicalConstants::meVtoWavenumber) /
                 (PhysicalConstants::meV * TOFisinMicroseconds);
  }
}

double Energy_inWavenumber::singleToTOF(const double x) const {
  double temp = x;
  if (temp <= DBL_MIN)
    temp =
        DBL_MIN; // Protect against divide by zero and define conversion range
  return factorTo / sqrt(temp);
}
///@return  Minimal time which can be reversibly converted into energy in
/// wavenumner units
double Energy_inWavenumber::conversionTOFMin() const {
  return factorTo / sqrt(std::numeric_limits<double>::max());
}
double Energy_inWavenumber::conversionTOFMax() const {
  return factorTo / sqrt(std::numeric_limits<double>::max());
}

double Energy_inWavenumber::singleFromTOF(const double tof) const {
  double temp = tof;
  if (temp == 0.0)
    temp = DBL_MIN; // Protect against divide by zero
  return factorFrom / (temp * temp);
}

Unit *Energy_inWavenumber::clone() const {
  return new Energy_inWavenumber(*this);
}

// ==================================================================================================
/* D-SPACING
 * ==================================================================================================
 *
 * Conversion uses Bragg's Law: 2d sin(theta) = n * lambda
 */
DECLARE_UNIT(dSpacing)

const UnitLabel dSpacing::label() const { return Symbol::Angstrom; }

dSpacing::dSpacing() : Unit() {
  const double factor = 2.0 * M_PI;
  addConversion("MomentumTransfer", factor, -1.0);
  addConversion("QSquared", (factor * factor), -2.0);
}

void dSpacing::init() {
  // First the crux of the conversion
  factorTo =
      (2.0 * PhysicalConstants::NeutronMass * sin(twoTheta / 2.0) * (l1 + l2)) /
      PhysicalConstants::h;

  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factorTo *= TOFisinMicroseconds / toAngstroms;
  factorFrom = factorTo;
  if (factorFrom == 0.0)
    factorFrom = DBL_MIN; // Protect against divide by zero
}

double dSpacing::singleToTOF(const double x) const { return x * factorTo; }
double dSpacing::singleFromTOF(const double tof) const {
  return tof / factorFrom;
}
double dSpacing::conversionTOFMin() const { return 0; }
double dSpacing::conversionTOFMax() const { return DBL_MAX / factorTo; }

Unit *dSpacing::clone() const { return new dSpacing(*this); }

// ================================================================================
/* MOMENTUM TRANSFER
 * ================================================================================
 *
 * The relationship is Q = 2k sin (theta). where k is 2*pi/wavelength
 */
DECLARE_UNIT(MomentumTransfer)

const UnitLabel MomentumTransfer::label() const {
  return Symbol::InverseAngstrom;
}

MomentumTransfer::MomentumTransfer() : Unit() {
  addConversion("QSquared", 1.0, 2.0);
  const double factor = 2.0 * M_PI;
  addConversion("dSpacing", factor, -1.0);
}

void MomentumTransfer::init() {
  // First the crux of the conversion
  factorTo = (4.0 * M_PI * PhysicalConstants::NeutronMass * (l1 + l2) *
              sin(twoTheta / 2.0)) /
             PhysicalConstants::h;
  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factorTo *= TOFisinMicroseconds / toAngstroms;
  // First the crux of the conversion
  factorFrom = (4.0 * M_PI * PhysicalConstants::NeutronMass * (l1 + l2) *
                sin(twoTheta / 2.0)) /
               PhysicalConstants::h;

  // Now adjustments for the scale of units used
  factorFrom *= TOFisinMicroseconds / toAngstroms;
}

double MomentumTransfer::singleToTOF(const double x) const {
  double temp = x;
  if (temp == 0.0)
    temp = DBL_MIN; // Protect against divide by zero
  return factorTo / temp;
}
//
double MomentumTransfer::singleFromTOF(const double tof) const {
  double temp = tof;
  if (temp == 0.0)
    temp = DBL_MIN; // Protect against divide by zero
  return factorFrom / temp;
}

double MomentumTransfer::conversionTOFMin() const {
  return factorFrom / DBL_MAX;
}
double MomentumTransfer::conversionTOFMax() const { return DBL_MAX; }

Unit *MomentumTransfer::clone() const { return new MomentumTransfer(*this); }

/* ===================================================================================================
 * Q-SQUARED
 * ===================================================================================================
 */
DECLARE_UNIT(QSquared)

const UnitLabel QSquared::label() const { return Symbol::InverseAngstromSq; }

QSquared::QSquared() : Unit() {
  addConversion("MomentumTransfer", 1.0, 0.5);
  const double factor = 2.0 * M_PI;
  addConversion("dSpacing", factor, -0.5);
}

void QSquared::init() {
  // First the crux of the conversion
  factorTo = (4.0 * M_PI * PhysicalConstants::NeutronMass * (l1 + l2) *
              sin(twoTheta / 2.0)) /
             PhysicalConstants::h;
  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factorTo *= TOFisinMicroseconds / toAngstroms;

  // First the crux of the conversion
  factorFrom = (4.0 * M_PI * PhysicalConstants::NeutronMass * (l1 + l2) *
                sin(twoTheta / 2.0)) /
               PhysicalConstants::h;
  // Now adjustments for the scale of units used
  factorFrom *= TOFisinMicroseconds / toAngstroms;
  factorFrom = factorFrom * factorFrom;
}

double QSquared::singleToTOF(const double x) const {
  double temp = x;
  if (temp == 0.0)
    temp = DBL_MIN; // Protect against divide by zero
  return factorTo / sqrt(temp);
}
double QSquared::singleFromTOF(const double tof) const {
  double temp = tof;
  if (temp == 0.0)
    temp = DBL_MIN; // Protect against divide by zero
  return factorFrom / (temp * temp);
}

double QSquared::conversionTOFMin() const {
  if (factorTo > 0)
    return factorTo / sqrt(DBL_MAX);
  else
    return -sqrt(DBL_MAX);
}
double QSquared::conversionTOFMax() const {
  if (factorTo > 0)
    return sqrt(DBL_MAX);
  else
    return factorTo / sqrt(DBL_MAX);
}

Unit *QSquared::clone() const { return new QSquared(*this); }

/* ==============================================================================
 * Energy Transfer
 * ==============================================================================
 */
DECLARE_UNIT(DeltaE)

const UnitLabel DeltaE::label() const { return Symbol::MilliElectronVolts; }

void DeltaE::init() {
  // Efixed must be set to something
  if (efixed == 0.0)
    throw std::invalid_argument(
        "efixed must be set for energy transfer calculation");
  const double TOFinMicroseconds = 1e6;
  factorTo =
      sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) *
      TOFinMicroseconds;
  if (emode == 1) {
    // t_other is t1
    t_other = (factorTo * l1) / sqrt(efixed);
    factorTo *= l2;
  } else if (emode == 2) {
    // t_other is t2
    t_other = (factorTo * l2) / sqrt(efixed);
    factorTo *= l1;
  } else {
    throw std::invalid_argument(
        "emode must be equal to 1 or 2 for energy transfer calculation");
  }

  //------------ from conversion ------------------
  factorFrom =
      sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) *
      TOFinMicroseconds;

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
    if (e2 <=
        0.0) // This shouldn't ever happen (unless the efixed value is wrong)
      return DeltaE::conversionTOFMax();
    else {
      // this_t = t2;
      const double this_t = factorTo / sqrt(e2);
      return this_t + t_other; // (t1+t2);
    }
  } else if (emode == 2) {
    const double e1 = efixed + x / unitScaling;
    if (e1 <=
        0.0) // This shouldn't ever happen (unless the efixed value is wrong)
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
  double time(
      DBL_MAX); // impossible for elastic, this units do not work for elastic
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

DeltaE::DeltaE() : Unit() {
  addConversion("DeltaE_inWavenumber", PhysicalConstants::meVtoWavenumber, 1.);
}

// =====================================================================================================
/* Energy Transfer in units of wavenumber
 * =====================================================================================================
 *
 * This is identical to the above (Energy Transfer in meV) with one division by
 *meVtoWavenumber.
 */
DECLARE_UNIT(DeltaE_inWavenumber)

const UnitLabel DeltaE_inWavenumber::label() const { return Symbol::InverseCM; }

void DeltaE_inWavenumber::init() {
  DeltaE::init();
  // Change the unit scaling factor
  unitScaling = PhysicalConstants::meVtoWavenumber;
}

Unit *DeltaE_inWavenumber::clone() const {
  return new DeltaE_inWavenumber(*this);
}

DeltaE_inWavenumber::DeltaE_inWavenumber() : DeltaE() {
  addConversion("DeltaE", 1 / PhysicalConstants::meVtoWavenumber, 1.);
}

double DeltaE_inWavenumber::conversionTOFMin() const {
  return DeltaE::conversionTOFMin();
}

double DeltaE_inWavenumber::conversionTOFMax() const {
  return DeltaE::conversionTOFMax();
}

// =====================================================================================================
/* Momentum in Angstrom^-1. It is 2*Pi/wavelength
 * =====================================================================================================
 */
DECLARE_UNIT(Momentum)

const UnitLabel Momentum::label() const { return Symbol::InverseAngstrom; }

Momentum::Momentum() : Unit() {

  const double AngstromsSquared = 1e20;
  const double factor =
      (AngstromsSquared * PhysicalConstants::h * PhysicalConstants::h) /
      (2.0 * PhysicalConstants::NeutronMass * PhysicalConstants::meV) /
      (4 * M_PI * M_PI);

  addConversion("Energy", factor, 2.0);
  addConversion("Energy_inWavenumber",
                factor * PhysicalConstants::meVtoWavenumber, 2.0);
  addConversion("Wavelength", 2 * M_PI, -1.0);
  //
}

void Momentum::init() {
  // ------------ Factors to convert TO TOF ---------------------
  double ltot = 0.0;
  double TOFisinMicroseconds = 1e6;
  double toAngstroms = 1e10;
  sfpTo = 0.0;

  if (emode == 1) {
    ltot = l2;
    sfpTo =
        (sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) *
         TOFisinMicroseconds * l1) /
        sqrt(efixed);
  } else if (emode == 2) {
    ltot = l1;
    sfpTo =
        (sqrt(PhysicalConstants::NeutronMass / (2.0 * PhysicalConstants::meV)) *
         TOFisinMicroseconds * l2) /
        sqrt(efixed);
  } else {
    ltot = l1 + l2;
  }
  factorTo = 2 * M_PI * (PhysicalConstants::NeutronMass * (ltot)) /
             PhysicalConstants::h;
  // Now adjustments for the scale of units used
  factorTo *= TOFisinMicroseconds / toAngstroms;

  // ------------ Factors to convert FROM TOF ---------------------
  ltot = l1 + l2;
  // Protect against divide by zero
  if (ltot == 0.0)
    ltot = DBL_MIN;

  // Now apply the factor to the input data vector
  do_sfpFrom = false;
  if (efixed != DBL_MIN) {
    if (emode == 1) // Direct
    {
      ltot = l2;
      sfpFrom = sfpTo;
      do_sfpFrom = true;
    } else if (emode == 2) // Indirect
    {
      ltot = l1;
      sfpFrom = sfpTo;
      do_sfpFrom = true;
    } else {
      ltot = l1 + l2;
    }
  } else {
    ltot = l1 + l2;
  }

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

SpinEchoLength::SpinEchoLength() : Wavelength() { clearConversions(); }

void SpinEchoLength::init() {
  // Efixed must be set to something
  if (efixed == 0.0)
    throw std::invalid_argument(
        "efixed must be set for spin echo length calculation");
  if (emode > 0) {
    throw std::invalid_argument(
        "emode must be equal to 0 for spin echo length calculation");
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

SpinEchoTime::SpinEchoTime() : Wavelength() { clearConversions(); }

void SpinEchoTime::init() {
  // Efixed must be set to something
  if (efixed == 0.0)
    throw std::invalid_argument(
        "efixed must be set for spin echo time calculation");
  if (emode > 0) {
    throw std::invalid_argument(
        "emode must be equal to 0 for spin echo time calculation");
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

Time::Time() : Unit() {}

void Time::init() {}

double Time::singleToTOF(const double x) const {
  UNUSED_ARG(x);
  throw std::runtime_error("Time is not allowed to be convert to TOF. ");
  return 0.0;
}

double Time::singleFromTOF(const double tof) const {
  UNUSED_ARG(tof);
  throw std::runtime_error("Time is not allwed to be converted from TOF. ");
  return 0.0;
}

double Time::conversionTOFMax() const {
  return std::numeric_limits<double>::quiet_NaN();
};
double Time::conversionTOFMin() const {
  return std::numeric_limits<double>::quiet_NaN();
};

Unit *Time::clone() const { return new Time(*this); }

// ================================================================================
/* Degrees
 * ================================================================================
 *
 * Degrees prints degrees as a label
 */

Degrees::Degrees() : Empty(), m_label("degrees") {}

const UnitLabel Degrees::label() const { return m_label; }

} // namespace Units

} // namespace Kernel
} // namespace Mantid
