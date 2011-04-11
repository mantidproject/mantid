//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Unit.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include <cmath>
#include <cfloat>

namespace Mantid
{
namespace Kernel
{

/** Is conversion by constant multiplication possible?
 *
 *  Look to see if conversion from the unit upon which this method is called requires
 *  only multiplication by a constant and not detector information (i.e. distance & angle),
 *  in which case doing the conversion via time-of-flight is not necessary.
 *  @param destination :: The unit to which conversion is sought
 *  @param factor ::      Returns the constant by which to multiply the input unit (if a conversion is found)
 *  @param power ::       Returns the power to which to raise the unput unit (if a conversion is found)
 *  @return            True if a 'quick conversion' exists, false otherwise
 */
bool Unit::quickConversion(const Unit& destination, double& factor, double& power) const
{
  // Just extract the unit's name and forward to other quickConversion method
  return quickConversion(destination.unitID(),factor,power);
}

/** Is conversion by constant multiplication possible?
 *
 *  Look to see if conversion from the unit upon which this method is called requires
 *  only multiplication by a constant and not detector information (i.e. distance & angle),
 *  in which case doing the conversion via time-of-flight is not necessary.
 *  @param destUnitName :: The class name of the unit to which conversion is sought
 *  @param factor ::       Returns the constant by which to multiply the input unit (if a conversion is found)
 *  @param power ::        Returns the power to which to raise the unput unit (if a conversion is found)
 *  @return             True if a 'quick conversion' exists, false otherwise
 */
bool Unit::quickConversion(std::string destUnitName, double& factor, double& power) const
{
  // From the global map, try to get the map holding the conversions for this unit
  ConversionsMap::const_iterator it = s_conversionFactors.find(unitID());
  // Return false if there are no conversions entered for this unit
  if ( it == s_conversionFactors.end() ) return false;

  // See if there's a conversion listed for the requested destination unit
  std::transform(destUnitName.begin(),destUnitName.end(),destUnitName.begin(),toupper);
  UnitConversions::const_iterator iter = it->second.find(destUnitName);
  // If not, return false
  if ( iter == it->second.end() ) return false;

  // Conversion found - set the conversion factors
  factor = iter->second.first;
  power = iter->second.second;
  return true;
}

// Initialise the static map holding the 'quick conversions'
Unit::ConversionsMap Unit::s_conversionFactors = Unit::ConversionsMap();

/** Add a 'quick conversion' from the unit class on which this method is called.
 *  @param to ::     The destination Unit for this conversion (use name returned by the unit's unitID() method)
 *  @param factor :: The constant by which to multiply the input unit
 *  @param power ::  The power to which to raise the input unit (defaults to 1)
 */
void Unit::addConversion(std::string to, const double& factor, const double& power) const
{
  std::transform(to.begin(), to.end(), to.begin(), toupper);
  // Add the conversion to the map (does nothing if it's already there)
  s_conversionFactors[unitID()][to] = std::make_pair(factor,power);
}


namespace Units
{

/* EMPTY
 * ==============
 */
DECLARE_UNIT(Empty)

void Empty::toTOF(std::vector<double>&, std::vector<double>&, const double&, const double&,
    const double&, const int&, const double&, const double&) const
{
  throw Kernel::Exception::NotImplementedError("Cannot convert unit "+this->unitID()+" to time of flight");
}

void Empty::fromTOF(std::vector<double>&, std::vector<double>&, const double&, const double& ,
    const double&, const int&, const double&, const double&) const
{
  throw Kernel::Exception::NotImplementedError("Cannot convert unit "+this->unitID()+" to time of flight");
}

/* LABEL
 * ==============
 */

DECLARE_UNIT(Label)

/// Constructor
Label::Label()
:Empty(),m_caption("Quantity"),m_label("")
{
}

/**
  * Set a caption and a label
  */
void Label::setLabel(const std::string& cpt, const std::string& lbl)
{
  m_caption = cpt;
  m_label = lbl;
}

/* TIME OF FLIGHT
 * ==============
 */
DECLARE_UNIT(TOF)

void TOF::toTOF(std::vector<double>&, std::vector<double>& , const double& , const double&,
    const double&, const int&, const double&, const double& ) const
{
  // Nothing to do
  return;
}

void TOF::fromTOF(std::vector<double>&, std::vector<double>&, const double&, const double& ,
    const double&, const int&, const double&, const double&) const
{
  // Nothing to do
  return;
}

/* WAVELENGTH
 * ==========
 *
 * This class makes use of the de Broglie relationship: lambda = h/p = h/mv, where v is (l1+l2)/tof.
 */
DECLARE_UNIT(Wavelength)

Wavelength::Wavelength() : Unit()
{
  const double AngstromsSquared = 1e20;
  const double factor = ( AngstromsSquared * PhysicalConstants::h * PhysicalConstants::h )
                                 / ( 2.0 * PhysicalConstants::NeutronMass * PhysicalConstants::meV );
  addConversion("Energy",factor,-2.0);
}


void Wavelength::toTOF(std::vector<double>& xdata, std::vector<double>&, const double& l1, const double& l2,
    const double&, const int& emode, const double& efixed, const double&) const
{
  // First the crux of the conversion
  double ltot = 0.0;
  double sfp = 0.0;
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;

  if ( emode == 1 )
  {
    ltot = l2;
    sfp = ( sqrt( PhysicalConstants::NeutronMass / (2.0*PhysicalConstants::meV) ) * TOFisinMicroseconds * l1 ) / sqrt(efixed);
  }
  else if ( emode == 2 )
  {
    ltot = l1;
    sfp = ( sqrt( PhysicalConstants::NeutronMass / (2.0*PhysicalConstants::meV) ) * TOFisinMicroseconds * l2 ) / sqrt(efixed);
  }
  else
  {
    ltot = l1 + l2;
  }

  double factor = ( PhysicalConstants::NeutronMass * ( ltot ) ) / PhysicalConstants::h;
  // Now adjustments for the scale of units used
  factor *= TOFisinMicroseconds / toAngstroms;

  // Now apply the factor to the input data vector
  std::transform( xdata.begin(), xdata.end(), xdata.begin(), std::bind2nd(std::multiplies<double>(), factor) );

  if ( emode == 1 || emode == 2 )
  { // If Direct or Indirect we want to correct TOF values...
    std::transform(xdata.begin(), xdata.end(), xdata.begin(), std::bind2nd(std::plus<double>(), sfp));
  }
}

void Wavelength::fromTOF(std::vector<double>& xdata, std::vector<double>&, const double& l1, const double& l2,
    const double&, const int& emode, const double& efixed, const double&) const
{
  double ltot = l1 + l2;
  // Protect against divide by zero
  if ( ltot == 0.0 ) ltot = DBL_MIN;

  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;

  // Now apply the factor to the input data vector
  if ( efixed != DBL_MIN )
  {
    if ( emode == 1 ) // Direct
    {
      ltot = l2;
      double sfp = ( sqrt( PhysicalConstants::NeutronMass / (2.0*PhysicalConstants::meV) ) * TOFisinMicroseconds * l1 ) / sqrt(efixed);
      std::transform(xdata.begin(), xdata.end(), xdata.begin(), std::bind2nd(std::minus<double>(), sfp));
    }
    else if ( emode == 2 ) // Indirect
    {
      ltot = l1;
      double sfp = ( sqrt( PhysicalConstants::NeutronMass / (2.0*PhysicalConstants::meV) ) * TOFisinMicroseconds * l2 ) / sqrt(efixed);
      std::transform(xdata.begin(), xdata.end(), xdata.begin(), std::bind2nd(std::minus<double>(), sfp));
    }
    else
    {
      ltot = l1 + l2;
    }
  }
  else
  {
    ltot = l1 + l2;
  }

  // First the crux of the conversion
  double factor = PhysicalConstants::h / ( PhysicalConstants::NeutronMass * ( ltot ) );

  // Now adjustments for the scale of units used

  factor *= toAngstroms / TOFisinMicroseconds;

  std::transform( xdata.begin(), xdata.end(), xdata.begin(), std::bind2nd(std::multiplies<double>(), factor) );
}

/* ENERGY
 * ======
 *
 * Conversion uses E = 1/2 mv^2, where v is (l1+l2)/tof.
 */
DECLARE_UNIT(Energy)

/// Constructor
Energy::Energy() : Unit()
{
  const double toAngstroms = 1e10;
  const double factor = toAngstroms * PhysicalConstants::h
                                / sqrt( 2.0 * PhysicalConstants::NeutronMass * PhysicalConstants::meV);
  addConversion("Wavelength",factor,-0.5);
}

void Energy::toTOF(std::vector<double>& xdata, std::vector<double>&, const double& l1, const double& l2,
    const double&, const int&, const double&, const double&) const
{
  const double TOFinMicroseconds = 1e6;

  const double factor = sqrt( PhysicalConstants::NeutronMass / (2.0*PhysicalConstants::meV) )
                                     * ( l1 + l2 ) * TOFinMicroseconds;

  std::vector<double>::iterator it;
  for (it = xdata.begin(); it != xdata.end(); ++it)
  {
    if (*it == 0.0) *it = DBL_MIN; // Protect against divide by zero
    *it = factor / sqrt(*it);
  }
}

void Energy::fromTOF(std::vector<double>& xdata, std::vector<double>&, const double& l1, const double& l2,
    const double&, const int&, const double&, const double&) const
{
  const double TOFisinMicroseconds = 1e-12;  // The input tof number gets squared so this is (10E-6)^2

  const double ltot = l1 + l2;
  const double factor = ( (PhysicalConstants::NeutronMass / 2.0) * ( ltot * ltot ) )
                                       / (PhysicalConstants::meV * TOFisinMicroseconds);

  std::vector<double>::iterator it;
  for (it = xdata.begin(); it != xdata.end(); ++it)
  {
    if (*it == 0.0) *it = DBL_MIN; // Protect against divide by zero
    *it = factor / ( (*it)*(*it) );
  }
}

/* D-SPACING
 * =========
 *
 * Conversion uses Bragg's Law: 2d sin(theta) = n * lambda
 */
DECLARE_UNIT(dSpacing)

dSpacing::dSpacing() : Unit()
{
  const double factor = 2.0 * M_PI;
  addConversion("MomentumTransfer",factor,-1.0);
  addConversion("QSquared",(factor*factor),-2.0);
}

void dSpacing::toTOF(std::vector<double>& xdata, std::vector<double>&, const double& l1, const double& l2,
    const double& twoTheta, const int&, const double& , const double& ) const
{
  // First the crux of the conversion
  double factor = ( 2.0 * PhysicalConstants::NeutronMass * sin(twoTheta/2.0) * ( l1 + l2 ) )
                            / PhysicalConstants::h;

  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factor *= TOFisinMicroseconds / toAngstroms;

  std::transform( xdata.begin(), xdata.end(), xdata.begin(), std::bind2nd(std::multiplies<double>(), factor) );
}

void dSpacing::fromTOF(std::vector<double>& xdata, std::vector<double>& , const double& l1, const double& l2,
    const double& twoTheta, const int& , const double&, const double&) const
{
  // First the crux of the conversion. Note that the input data is DIVIDED by this factor below.
  double factor = ( 2.0 * PhysicalConstants::NeutronMass * sin(twoTheta/2.0) * ( l1 + l2 ) )
                            / PhysicalConstants::h;

  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factor *= TOFisinMicroseconds / toAngstroms;
  if (factor == 0.0) factor = DBL_MIN; // Protect against divide by zero

  std::transform( xdata.begin(), xdata.end(), xdata.begin(), std::bind2nd(std::divides<double>(), factor) );
}

/* MOMENTUM TRANSFER
 * =================
 *
 * The relationship is Q = 2k sin (theta). where k is 2*pi/wavelength
 */
DECLARE_UNIT(MomentumTransfer)

MomentumTransfer::MomentumTransfer() : Unit()
{
  addConversion("QSquared",1.0,2.0);
  const double factor = 2.0 * M_PI;
  addConversion("dSpacing",factor,-1.0);
}

void MomentumTransfer::toTOF(std::vector<double>& xdata, std::vector<double>&, const double& l1, const double& l2,
    const double& twoTheta, const int&, const double&, const double&) const
{
  // First the crux of the conversion
  double factor = ( 4.0 * M_PI * PhysicalConstants::NeutronMass * (l1 + l2)
                               * sin(twoTheta/2.0) ) / PhysicalConstants::h;

  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factor *= TOFisinMicroseconds/ toAngstroms;

  std::vector<double>::iterator it;
  for (it = xdata.begin(); it != xdata.end(); ++it)
  {
    if (*it == 0.0) *it = DBL_MIN; // Protect against divide by zero
    *it = factor / (*it);
  }
}

void MomentumTransfer::fromTOF(std::vector<double>& xdata, std::vector<double>& , const double& l1, const double& l2,
    const double& twoTheta, const int&, const double&, const double&) const
{
  // First the crux of the conversion
  double factor = ( 4.0 * M_PI * PhysicalConstants::NeutronMass * (l1 + l2)
      * sin(twoTheta/2.0) ) / PhysicalConstants::h;

  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factor *= TOFisinMicroseconds/ toAngstroms;

  std::vector<double>::iterator it;
  for (it = xdata.begin(); it != xdata.end(); ++it)
  {
    if (*it == 0.0) *it = DBL_MIN; // Protect against divide by zero
    *it = factor / (*it);
  }
}

/* Q-SQUARED
 * =========
 */
DECLARE_UNIT(QSquared)

QSquared::QSquared() : Unit()
{
  addConversion("MomentumTransfer",1.0,0.5);
  const double factor = 2.0 * M_PI;
  addConversion("dSpacing",factor,-0.5);
}

void QSquared::toTOF(std::vector<double>& xdata, std::vector<double>&, const double& l1, const double& l2,
    const double& twoTheta, const int&, const double&, const double&) const
{
  // First the crux of the conversion
  double factor = ( 4.0 * M_PI * PhysicalConstants::NeutronMass * (l1 + l2)
      * sin(twoTheta/2.0) ) / PhysicalConstants::h;

  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factor *= TOFisinMicroseconds/ toAngstroms;

  std::vector<double>::iterator it;
  for (it = xdata.begin(); it != xdata.end(); ++it)
  {
    if (*it == 0.0) *it = DBL_MIN; // Protect against divide by zero
    *it = factor / sqrt(*it);
  }
}

void QSquared::fromTOF(std::vector<double>& xdata, std::vector<double>&, const double& l1, const double& l2,
    const double& twoTheta, const int&, const double&, const double&) const
{
  // First the crux of the conversion
  double factor = ( 4.0 * M_PI * PhysicalConstants::NeutronMass * (l1 + l2)
                               * sin(twoTheta/2.0) ) / PhysicalConstants::h;

  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factor *= TOFisinMicroseconds/ toAngstroms;

  factor = factor * factor;

  std::vector<double>::iterator it;
  for (it = xdata.begin(); it != xdata.end(); ++it)
  {
    if (*it == 0.0) *it = DBL_MIN; // Protect against divide by zero
    *it = factor / ( (*it)*(*it) );
  }
}

/* Energy Transfer
 * ===============
 */
DECLARE_UNIT(DeltaE)

void DeltaE::toTOF(std::vector<double>& xdata, std::vector<double>&, const double& l1, const double& l2,
    const double&, const int& emode, const double& efixed, const double&) const
{
  // Efixed must be set to something
  if (efixed == 0.0) throw std::invalid_argument("efixed must be set for energy transfer calculation");

  const double TOFinMicroseconds = 1e6;
  double factor = sqrt( PhysicalConstants::NeutronMass / (2.0*PhysicalConstants::meV) ) * TOFinMicroseconds;

  if (emode == 1)
  {
    const double t1 = ( factor * l1 ) / sqrt( efixed );
    factor *= l2;

    std::vector<double>::iterator it;
    for (it = xdata.begin(); it != xdata.end(); ++it)
    {
      const double e2 = efixed - *it;
      if (e2<=0.0)  // This shouldn't ever happen (unless the efixed value is wrong)
      {
        *it = DBL_MAX;
      }
      else
      {
        const double t2 = factor / sqrt(e2);
        *it = t1 + t2;
      }
    }
  }
  else if (emode == 2)
  {
    const double t2 = ( factor * l2 ) / sqrt( efixed );
    factor *= l1;

    std::vector<double>::iterator it;
    for (it = xdata.begin(); it != xdata.end(); ++it)
    {
      const double e1 = efixed + *it;
      if (e1<=0.0)  // This shouldn't ever happen (unless the efixed value is wrong)
      {
        *it = -DBL_MAX;
      }
      else
      {
        const double t1 = factor / sqrt(e1);
        *it = t1 + t2;
      }
    }
  }
  else
  {
    throw std::invalid_argument("emode must be equal to 1 or 2 for energy transfer calculation");
  }
}

void DeltaE::fromTOF(std::vector<double>& xdata, std::vector<double>&, const double& l1, const double& l2,
    const double&, const int& emode, const double& efixed, const double&) const
{
  // Efixed must be set to something
  if (efixed == 0.0) throw std::invalid_argument("efixed must be set for energy transfer calculation");

  const double TOFinMicroseconds = 1e6;
  double factor = sqrt( PhysicalConstants::NeutronMass / (2.0*PhysicalConstants::meV) ) * TOFinMicroseconds;

  if (emode == 1)
  {
    const double t1 = ( factor * l1 ) / sqrt( efixed );
    factor = factor * factor * l2 * l2;

    std::vector<double>::iterator it;
    for (it = xdata.begin(); it != xdata.end(); ++it)
    {
      const double t2 = *it - t1;
      if (t2<=0.0)
      {
        *it = -DBL_MAX;
      }
      else
      {
        const double e2 = factor / (t2 * t2);
        *it = efixed - e2;
      }
    }
  }
  else if (emode == 2)
  {
    const double t2 = (factor * l2) / sqrt( efixed );
    factor = factor * factor * l1 * l1;

    std::vector<double>::iterator it;
    for (it = xdata.begin(); it != xdata.end(); ++it)
    {
      const double t1 = *it - t2;
      if (t1<=0.0)
      {
        *it = DBL_MAX;
      }
      else
      {
        const double e1 = factor / (t1 * t1);
        *it = e1 - efixed;
      }
    }
  }
  else
  {
    throw std::invalid_argument("emode must be equal to 1 or 2 for energy transfer calculation");
  }
}

/* Energy Transfer in units of wavenumber
 * ======================================
 *
 * This is identical to the above (Energy Transfer in meV) with one division by meVtoWavenumber.
 */
DECLARE_UNIT(DeltaE_inWavenumber)

void DeltaE_inWavenumber::toTOF(std::vector<double>& xdata, std::vector<double>&, const double& l1, const double& l2,
    const double&, const int& emode, const double& efixed, const double&) const
{
  // Efixed must be set to something
  if (efixed == 0.0) throw std::invalid_argument("efixed must be set for energy transfer calculation");

  const double TOFinMicroseconds = 1e6;
  double factor = sqrt( PhysicalConstants::NeutronMass / (2.0*PhysicalConstants::meV) ) * TOFinMicroseconds;

  if (emode == 1)
  {
    const double t1 = ( factor * l1 ) / sqrt( efixed );
    factor *= l2;

    std::vector<double>::iterator it;
    for (it = xdata.begin(); it != xdata.end(); ++it)
    {
      const double e2 = (efixed - *it) / PhysicalConstants::meVtoWavenumber;
      if (e2<=0.0)  // This shouldn't ever happen (unless the efixed value is wrong)
      {
        *it = DBL_MAX;
      }
      else
      {
        const double t2 = factor / sqrt(e2);
        *it = t1 + t2;
      }
    }
  }
  else if (emode == 2)
  {
    const double t2 = ( factor * l2 ) / sqrt( efixed );
    factor *= l1;

    std::vector<double>::iterator it;
    for (it = xdata.begin(); it != xdata.end(); ++it)
    {
      const double e1 = (efixed + *it) / PhysicalConstants::meVtoWavenumber;
      if (e1<=0.0)  // This shouldn't ever happen (unless the efixed value is wrong)
      {
        *it = -DBL_MAX;
      }
      else
      {
        const double t1 = factor / sqrt(e1);
        *it = t1 + t2;
      }
    }
  }
  else
  {
    throw std::invalid_argument("emode must be equal to 1 or 2 for energy transfer calculation");
  }
}

void DeltaE_inWavenumber::fromTOF(std::vector<double>& xdata, std::vector<double>& , const double& l1, const double& l2,
    const double&, const int& emode, const double& efixed, const double&) const
{
  // Efixed must be set to something
  if (efixed == 0.0) throw std::invalid_argument("efixed must be set for energy transfer calculation");

  const double TOFinMicroseconds = 1e6;
  double factor = sqrt( PhysicalConstants::NeutronMass / (2.0*PhysicalConstants::meV) ) * TOFinMicroseconds;

  if (emode == 1)
  {
    const double t1 = ( factor * l1 ) / sqrt( efixed );
    factor = factor * factor * l2 * l2;

    std::vector<double>::iterator it;
    for (it = xdata.begin(); it != xdata.end(); ++it)
    {
      const double t2 = *it - t1;
      if (t2<=0.0)
      {
        *it = -DBL_MAX;
      }
      else
      {
        const double e2 = factor / (t2 * t2);
        *it = (efixed - e2) * PhysicalConstants::meVtoWavenumber;
      }
    }
  }
  else if (emode == 2)
  {
    const double t2 = (factor * l2) / sqrt( efixed );
    factor = factor * factor * l1 * l1;

    std::vector<double>::iterator it;
    for (it = xdata.begin(); it != xdata.end(); ++it)
    {
      const double t1 = *it - t2;
      if (t1<=0.0)
      {
        *it = DBL_MAX;
      }
      else
      {
        const double e1 = factor / (t1 * t1);
        *it = (e1 - efixed) * PhysicalConstants::meVtoWavenumber;
      }
    }
  }
  else
  {
    throw std::invalid_argument("emode must be equal to 1 or 2 for energy transfer calculation");
  }
}

} // namespace Units

} // namespace Kernel
} // namespace Mantid
