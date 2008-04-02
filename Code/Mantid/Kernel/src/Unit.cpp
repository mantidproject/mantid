//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Unit.h"
#include "MantidKernel/PhysicalConstants.h"
#include <cmath>

namespace Mantid
{
namespace Kernel
{

/** Get the description attached to a unit instance
 *  @return A string containing the description
 */
const std::string& Unit::description() const
{
  return m_description;
}

/** Set the description of a unit instance
 *  @param value The new description
 */ 
void Unit::setDescription(const std::string& value)
{
  m_description = value;
}

namespace Units
{

/* TIME OF FLIGHT
 * ==============
 */
DECLARE_UNIT(TOF)

void TOF::toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{
  // Nothing to do
  return;
}

void TOF::fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
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

void Wavelength::toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{
  // First the crux of the conversion
  double factor = ( PhysicalConstants::NeutronMass * ( l1 + l2 ) ) / PhysicalConstants::h;

  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factor *= TOFisinMicroseconds / toAngstroms;

  // Now apply the factor to the input data vector
  std::transform( xdata.begin(), xdata.end(), xdata.begin(), std::bind2nd(std::multiplies<double>(), factor) );  
}

void Wavelength::fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{
  // First the crux of the conversion
  double factor = PhysicalConstants::h / ( PhysicalConstants::NeutronMass * ( l1 + l2 ) );

  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factor *= toAngstroms / TOFisinMicroseconds;
  
  // Now apply the factor to the input data vector
  std::transform( xdata.begin(), xdata.end(), xdata.begin(), std::bind2nd(std::multiplies<double>(), factor) );
}

/* ENERGY
 * ======
 * 
 * Conversion uses E = 1/2 mv^2, where v is (l1+l2)/tof.
 */
DECLARE_UNIT(Energy)

void Energy::toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{
  const double TOFinMicroseconds = 1e6;
  
  const double factor = sqrt( PhysicalConstants::NeutronMass / (2.0*PhysicalConstants::meV) )
                                     * ( l1 + l2 ) * TOFinMicroseconds;
  
  std::vector<double>::iterator it;
  for (it = xdata.begin(); it != xdata.end(); ++it) 
  {
    *it = factor / sqrt(*it);
  }
}

void Energy::fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{
  const double TOFisinMicroseconds = 1e-12;  // The input tof number gets squared so this is (10E-6)^2

  const double ltot = l1 + l2;
  const double factor = ( (PhysicalConstants::NeutronMass / 2.0) * ( ltot * ltot ) ) 
                                       / (PhysicalConstants::meV * TOFisinMicroseconds);

  std::vector<double>::iterator it;
  for (it = xdata.begin(); it != xdata.end(); ++it) 
  {
    *it = factor / ( (*it)*(*it) );
  }
}

/* D-SPACING
 * =========
 * 
 * Conversion uses Bragg's Law: 2d sin(theta) = n * lambda
 */
DECLARE_UNIT(dSpacing)

void dSpacing::toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
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

void dSpacing::fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{
  // First the crux of the conversion. Note that the input data is DIVIDED by this factor below.
  double factor = ( 2.0 * PhysicalConstants::NeutronMass * sin(twoTheta/2.0) * ( l1 + l2 ) ) 
                            / PhysicalConstants::h;

  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factor *= TOFisinMicroseconds / toAngstroms;
  
  std::transform( xdata.begin(), xdata.end(), xdata.begin(), std::bind2nd(std::divides<double>(), factor) );  
}

/* MOMENTUM TRANSFER
 * =================
 * 
 * The relationship is Q = 2k sin (theta). where k is 2*pi/wavelength
 */
DECLARE_UNIT(MomentumTransfer)

void MomentumTransfer::toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{
  // First the crux of the conversion
  double factor = ( 4.0 * PhysicalConstants::pi * PhysicalConstants::NeutronMass * (l1 + l2) 
                               * sin(twoTheta/2.0) ) / PhysicalConstants::h;
  
  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factor *= TOFisinMicroseconds/ toAngstroms;
  
  std::vector<double>::iterator it;
  for (it = xdata.begin(); it != xdata.end(); ++it) 
  {
    *it = factor / (*it);
  }
}

void MomentumTransfer::fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{
  // First the crux of the conversion
  double factor = ( 4.0 * PhysicalConstants::pi * PhysicalConstants::NeutronMass * (l1 + l2)
      * sin(twoTheta/2.0) ) / PhysicalConstants::h;

  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factor *= TOFisinMicroseconds/ toAngstroms;

  std::vector<double>::iterator it;
  for (it = xdata.begin(); it != xdata.end(); ++it)
  {
    *it = factor / (*it);
  }
}

/* Q-SQUARED
 * =========
 */
DECLARE_UNIT(QSquared)

void QSquared::toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{
  // First the crux of the conversion
  double factor = ( 4.0 * PhysicalConstants::pi * PhysicalConstants::NeutronMass * (l1 + l2)
      * sin(twoTheta/2.0) ) / PhysicalConstants::h;

  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factor *= TOFisinMicroseconds/ toAngstroms;

  std::vector<double>::iterator it;
  for (it = xdata.begin(); it != xdata.end(); ++it)
  {
    *it = factor / sqrt(*it);
  }
}

void QSquared::fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{
  // First the crux of the conversion
  double factor = ( 4.0 * PhysicalConstants::pi * PhysicalConstants::NeutronMass * (l1 + l2) 
                               * sin(twoTheta/2.0) ) / PhysicalConstants::h;
  
  // Now adjustments for the scale of units used
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  factor *= TOFisinMicroseconds/ toAngstroms;
  
  factor = factor * factor;
  
  std::vector<double>::iterator it;
  for (it = xdata.begin(); it != xdata.end(); ++it) 
  {
    *it = factor / ( (*it)*(*it) );
  }
}

/* Energy Transfer
 * ===============
 */
DECLARE_UNIT(DeltaE)

void DeltaE::toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{

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
      const double t2 = factor / sqrt(e2);
      *it = t1 + t2;
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
      const double t1 = factor / sqrt(e1);
      *it = t1 + t2;
    }
  }
  else
  {
    throw std::invalid_argument("emode must be equal to 1 or 2 for energy transfer calculation");
  }
}

void DeltaE::fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{

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
      const double e2 = factor / (t2 * t2);
      *it = efixed - e2;
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
      const double e1 = factor / (t1 * t1);
      *it = e1 - efixed;
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

void DeltaE_inWavenumber::toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{

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
      const double t2 = factor / sqrt(e2);
      *it = t1 + t2;
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
      const double t1 = factor / sqrt(e1);
      *it = t1 + t2;
    }
  }
  else
  {
    throw std::invalid_argument("emode must be equal to 1 or 2 for energy transfer calculation");
  }
}

void DeltaE_inWavenumber::fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{

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
      const double e2 = factor / (t2 * t2);
      *it = (efixed - e2) * PhysicalConstants::meVtoWavenumber;
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
      const double e1 = factor / (t1 * t1);
      *it = (e1 - efixed) * PhysicalConstants::meVtoWavenumber;
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
