//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/Material.h"
#include <sstream>
#include <stdexcept>

namespace Mantid
{

  namespace Kernel
  {

    using PhysicalConstants::NeutronAtom;

    /**
     * Construct an "empty" material. Everything returns zero
     */
    Material::Material() : 
      m_name(), m_element(0,0,0.0,0.0,0.0,0.0,0.0,0.0),
      m_numberDensity(0.0), m_temperature(0.0), m_pressure(0.0)
    {
    }

    /**
    * Construct a material object
    * @param name :: The name of the material
    * @param element :: The element it is composed from
    * @param numberDensity :: Density in A^-3
    * @param temperature :: The temperature in Kelvin (Default = 300K)
    * @param pressure :: Pressure in kPa (Default: 101.325 kPa)
    */
    Material::Material(const std::string & name, const PhysicalConstants::NeutronAtom& element,
		       const double numberDensity, const double temperature, 
		       const double pressure) :
      m_name(name), m_element(element), m_numberDensity(numberDensity), 
      m_temperature(temperature), m_pressure(pressure)
    {
    }   
    
    /** 
     * Returns the name 
     * @returns A string containing the name of the material
     */
    const std::string & Material::name() const 
    { 
      return m_name; 
    }

    /** 
     * Get the number density
     * @returns The number density of the material in A^-3
     */
    double Material::numberDensity() const
    { 
      return m_numberDensity; 
    }
    
    /** 
     * Get the temperature
     * @returns The temperature of the material in Kelvin
     */
    double Material::temperature() const 
    { 
      return m_temperature; 
    }
    
    /** 
     * Get the pressure
     * @returns The pressure of the material
     */
    double Material::pressure() const
    { 
      return m_pressure; 
    }

    /**
     * Get the coherent scattering cross section for a given wavelength.
     * CURRENTLY this simply returns the value for the underlying element
     * @param lambda :: The wavelength to evaluate the cross section
     * @returns The value of the coherent scattering cross section at 
     * the given wavelength
     */ 
    double Material::cohScatterXSection(const double lambda) const
    {
      (void)lambda;
      return m_element.coh_scatt_xs;
      
    }
    
    /**
     * Get the incoherent scattering cross section for a given wavelength
     * CURRENTLY this simply returns the value for the underlying element
     * @param lambda :: The wavelength to evaluate the cross section
     * @returns The value of the coherent scattering cross section at 
     * the given wavelength
     */
    double Material::incohScatterXSection(const double lambda) const
    {
      (void)lambda;
      return m_element.inc_scatt_xs;
    }

    /**
     * Get the total scattering cross section for a given wavelength
     * CURRENTLY this simply returns the value for sum of the incoherent
     * and coherent scattering cross sections.
     * @param lambda :: The wavelength to evaluate the cross section
     * @returns The value of the total scattering cross section at 
     * the given wavelength
     */
    double Material::totalScatterXSection(const double lambda) const
    {
      UNUSED_ARG(lambda);
      return m_element.tot_scatt_xs;
    }

    /**
     * Get the absorption cross section for a given wavelength.
     * CURRENTLY This assumes a linear dependence on the wavelength with the reference
     * wavelength = NeutronAtom::ReferenceLambda angstroms.
     * @param lambda :: The wavelength to evaluate the cross section
     * @returns The value of the absoprtioncross section at 
     * the given wavelength
     */
    double Material::absorbXSection(const double lambda) const
    {
      return (m_element.abs_scatt_xs) * (lambda / NeutronAtom::ReferenceLambda);
    }


    /** Save the object to an open NeXus file.
     * @param file :: open NeXus file
     * @param group :: name of the group to create
     */
    void Material::saveNexus(::NeXus::File * file, const std::string & group) const
    {
      file->makeGroup(group, "NXdata", 1);
      file->putAttr("version", 1);
      file->putAttr("name", m_name);
      file->writeData("element_Z", m_element.z_number);
      file->writeData("element_A", m_element.a_number);
      file->writeData("number_density", m_numberDensity);
      file->writeData("temperature", m_temperature);
      file->writeData("pressure", m_pressure);
      file->closeGroup();
    }

    /** Load the object from an open NeXus file.
     * @param file :: open NeXus file
     * @param group :: name of the group to open
     */
    void Material::loadNexus(::NeXus::File * file, const std::string & group)
    {
      file->openGroup(group, "NXdata");
      file->getAttr("name", m_name);

      // Find the element
      uint16_t element_Z, element_A;
      file->readData("element_Z", element_Z);
      file->readData("element_A", element_A);
      try {
      m_element = Mantid::PhysicalConstants::getNeutronAtom(element_Z, element_A);
      }
      catch (std::runtime_error &)
      { /* ignore and use the default */ }

      file->readData("number_density", m_numberDensity);
      file->readData("temperature", m_temperature);
      file->readData("pressure", m_pressure);
      file->closeGroup();
    }
    Material::ChemicalFormula Material::parseChemicalFormula(const std::string chemicalSymbol)
    {
      Material::ChemicalFormula CF;
      const char *s;
      s = chemicalSymbol.c_str();
      size_t i = 0;
      size_t ia = 0;
      size_t numberParen = 0;
      bool isotope = false;
      while (i < chemicalSymbol.length())
      {
        if (s[i] >= 'A' && s[i]<='Z')
        {
          std::string buf(s+i, s+i+1);
          CF.atoms.push_back(buf);
          CF.numberAtoms.push_back(0);
          CF.aNumbers.push_back(0);
          ia ++;
        }
        else if (s[i] >= 'a' && s[i]<='z')
        {
          std::string buf(s+i, s+i+1);
          CF.atoms[ia-1].append(buf);
        }
        else if (s[i] >= '0' && s[i]<='9')
        {
          if (isotope)
          {
            size_t ilast = i;
            // Number of digits in aNumber
            if (CF.aNumbers[ia-1] != 0) ilast -= (int) std::log10 ((double) CF.aNumbers[ia-1]) + 1;
            std::string buf(s+ilast, s+i+1);
            CF.aNumbers[ia-1] = static_cast<uint16_t>(std::atoi(buf.c_str()));
          }
          else
          {
            size_t ilast = i;
            // Number of digits in aNumber
            if (CF.numberAtoms[ia-1] != 0) ilast -= (int) std::log10 ((double) CF.numberAtoms[ia-1]) + 1;
            std::string buf(s+ilast, s+i+1);
            CF.numberAtoms[ia-1] = static_cast<uint16_t>(std::atoi(buf.c_str()));
          }

        }
        else if (s[i] == '(' || s[i] ==')')
        {
          isotope = !isotope;
          if (s[i] == '(')
          {
            // next atom

            numberParen = ia + 1;
          }
          else
          {
            if (ia > numberParen)for (size_t i0 = numberParen - 1; i0 < ia; i0++)
            {
              // if more than one atom in parenthesis, it is compound
              CF.numberAtoms[i0] = CF.aNumbers[i0];
              CF.aNumbers[i0] = 0;
            }
          }
        }
        else if (s[i] == ' ' || s[i] == '-')
        {
          // skip it as spacing character
        }
        else
        {
          std::stringstream msg;
          msg << "Encountered invalid character at position " << i << " in formula \""
              << chemicalSymbol << "\"";
          throw std::runtime_error(msg.str());
        }
        i++;
      }

      // fix up D -> H2 and number of atoms
      for (size_t i=0; i<ia; i++)
  		{
        if (CF.numberAtoms[i] == 0)
          CF.numberAtoms[i] = 1;

        if (CF.atoms[i] == "D")
        {
          CF.atoms[i] = "H";
          CF.aNumbers[i] = 2;
        }
  		}
  		return CF;
    }
  } 

}  
