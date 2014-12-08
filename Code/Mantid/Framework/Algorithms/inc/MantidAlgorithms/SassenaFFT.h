#ifndef MANTID_ALGORITHM_SASSENAFFT_H_
#define MANTID_ALGORITHM_SASSENAFFT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid
{
namespace Algorithms
{

/** Perform Fourier Transform of the Sassena Intermediate Scattering Function

  @author Jose Borreguero
  @date 2012-05-29

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  */

class DLLExport SassenaFFT : public API::Algorithm
{
public:
  /// Default constructor
  SassenaFFT() : API::Algorithm(), m_T2ueV(1000.0/Mantid::PhysicalConstants::meVtoKelvin), m_ps2meV(4.136) {}
  /// Destructor
  virtual ~SassenaFFT() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SassenaFFT"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Performs complex Fast Fourier Transform of intermediate scattering function";}

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Arithmetic\\FFT"; }
protected:
  // Overridden Algorithm methods
  bool processGroups();
private:
  
  // Overridden Algorithm methods
  void init();
  void exec();
  bool checkGroups();
  const double m_T2ueV; // conversion factor from Kelvin to ueV
  const double m_ps2meV; // conversion factor from picosecond to mili-eV

}; // class SassenaFFT

} // namespace Algorithm
} // namespace Mantid

#endif // MANTID_ALGORITHM_SASSENAFFT_H_
