#ifndef MATERIAL_H
#define MATERIAL_H

#include "MantidKernel/System.h"
#include <string>

namespace Mantid
{
  namespace Kernel
  {
    class Logger;
  }

namespace Geometry
{
  /*!
    \class Material
    \brief Nuetronic information on the material
    \author S. Ansell
    \version 1.0
    \date July 2007
    
    This can be extended so that more sophisticated material
    components can be used.
    \todo This class nees to have a base class.

    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
 private:

  static Kernel::Logger& PLog;           ///< The official logger
  
  std::string Name;      ///< Name 
  double density;        ///< number density [atom/A^3]
  double scoh;           ///< scattering cross section 
  double sinc;           ///< incoherrrent cross section 
  double sabs;           ///< Absorption cross section
  
 public:
  
  Material();
  Material(const std::string& N,const double D,
	   const double S,const double I,const double A);
  Material(const double D,const double S,const double I,const double A);
  Material(const Material&);
  virtual Material* clone() const;
  Material& operator=(const Material&);
  virtual ~Material();
  
  /// Set the name
  void setName(const std::string& N) { Name=N; }
  void setDensity(const double D);
  void setScat(const double S,const double I,const double A);

  /// Get the number density
  double getAtomDensity() const { return density; }

  /// Set the name
  const std::string& getName() const { return Name; }
  /// Get the total cross section
  double getScat() const { return scoh+sinc; }
  /// Get the scattering cross section
  double getCoh() const { return scoh; }
  /// Get the incoherent cross section
  double getInc() const { return sinc; }
  double getScatFrac(const double Wave) const;
  double getAtten(const double Wave) const; 
  double getAttenAbs(const double Wave) const; 

  double calcAtten(const double Wave,const double Length) const;
  
};


} // NAMESPACE Geometry

}  // NAMESPACE Mantid

#endif
