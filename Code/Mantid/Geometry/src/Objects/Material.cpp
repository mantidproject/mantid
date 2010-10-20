#include "MantidGeometry/Objects/Material.h"
#include <cmath>

namespace Mantid
{

  namespace Geometry
  {

    /**
    * Construct a material object
    * @param name The name of the material
    * @param density Density in kg/m^3
    * @param temperature The temperature in Kelvin
    * @param pressure Pressure in kPa
    * @param coherentXsec The coherent scattering cross section 
    * @param incoherentXsec The incoherent scattering cross section 
    * @param absorbXsec The absorption scattering cross section 
    */
    Material::Material(const std::string & name, const double density, 
      const double temperature, const double pressure,
      const double coherentXsec, const double incoherentXsec,
      const double absorbXsec) :
    m_name(name), m_density(density), m_temperature(temperature), m_pressure(pressure),
      m_coherentXsec(coherentXsec), m_incoherentXsec(incoherentXsec), m_absorbXsec(absorbXsec)
    {
    }   

  } 

}  