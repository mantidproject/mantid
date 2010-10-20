#ifndef MANTID_GEOMETRY_MATERIAL_H_
#define MANTID_GEOMETRY_MATERIAL_H_

#include "MantidKernel/System.h"
#include <string>

namespace Mantid
{

  namespace Geometry
  {
    /**
    Simple class defining a material. It holds basic information:
    <UL>
    <LI>Temperature (Kelvin)</LI>
    <LI>Pressure (KPa) </LI>
    <LI>Density (kg/m^3)</LI>
    <LI>Coherent scattering cross section</LI>
    <LI>Incoherent scattering cross section</LI>
    <LI>Absorption cross section</LI>
    </UL>

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>

    */
    class DLLExport Material
    {
    public:
      /// Constructor
      explicit Material(const std::string & name, const double density, 
        const double temperature, const double pressure,
        const double coherentXsec, const double incoherentXsec,
        const double absorbXsec);

      /** 
      * Returns the name 
      * @returns A string containing the name of the material
      */
      inline const std::string& name() const { return m_name; }
      /** 
      * Get the density
      * @returns The density of the material
      */
      inline double density() const { return m_density; }
      /** 
      * Get the temperature
      * @returns The temperature of the material
      */
      inline double temperature() const { return m_temperature; }
      /** 
      * Get the pressure
      * @returns The pressure of the material
      */
      inline double pressure() const { return m_pressure; }
      /**
      * Get the total cross section
      * @returns The sum of the coherent and incoherent scattering cross section
      */
      inline double totalCrossSection() const { return m_coherentXsec + m_incoherentXsec; }
      /**
      * Get the coherent cross section
      * @returns The coherent scattering cross section
      */
      inline double coherentCrossSection() const { return m_coherentXsec; }
      /**
      * Get the incoherent cross section
      * @returns The incoherent scattering cross section
      */
      inline double incoherentCrossSection() const { return m_incoherentXsec; }
      /**
      * Get the absorption cross section
      * @returns The absorption scattering cross section
      */
      inline double absorptionCrossSection() const { return m_absorbXsec; }

    private:
      /// Default constructor
      Material();

    private:
      /// Material name
      std::string m_name;
      /// Density in kg/m^3
      double m_density;
      /// Temperature
      double m_temperature;
      /// Pressure
      double m_pressure;
      /// Coherent scattering cross section
      double m_coherentXsec;
      /// Incoherent scattering cross section
      double m_incoherentXsec;
      /// Absorption cross section
      double m_absorbXsec;     
    };


  }
}

#endif //MANTID_GEOMETRY_MATERIAL_H_
