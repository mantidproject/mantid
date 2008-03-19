//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Unit.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid
{
namespace Kernel
{

/** Get the description attached to a unit instance
 *  @return A string containing the description
 */
const std::string Unit::description() const
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

DECLARE_UNIT(Wavelength)

void Wavelength::toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{
  const double TOFisinMicroseconds = 1e-6;
  const double toAngstroms = 1e10;

  double factor = (toAngstroms * TOFisinMicroseconds * PhysicalConstants::h) / 
                                                  ( PhysicalConstants::NeutronMass * ( l1 + l2 ) );
  std::transform( xdata.begin(), xdata.end(), xdata.begin(), std::bind2nd(std::divides<double>(), factor) );  
}

void Wavelength::fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
    const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
{
  const double TOFisinMicroseconds = 1e-6;
  const double toAngstroms = 1e10;

  double factor = (toAngstroms * TOFisinMicroseconds * PhysicalConstants::h) / 
                                                  ( PhysicalConstants::NeutronMass * ( l1 + l2 ) );
  std::transform( xdata.begin(), xdata.end(), xdata.begin(), std::bind2nd(std::multiplies<double>(), factor) );
}

} // namespace Units

} // namespace Kernel
} // namespace Mantid
