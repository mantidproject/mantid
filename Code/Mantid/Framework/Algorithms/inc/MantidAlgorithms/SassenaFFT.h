#ifndef MANTID_ALGORITHM_SASSENAFFT_H_
#define MANTID_ALGORITHM_SASSENAFFT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace Algorithms
{

/** Perform Fourier Transform of the Sassena Intermediate Scattering Function

  @author Jose Borreguero
  @date 2012-05-29

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  SassenaFFT() : API::Algorithm(), m_T2meV(1.0/11.604) {}
  /// Destructor
  virtual ~SassenaFFT() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SassenaFFT"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Arithmetic\\FFT"; }
protected:
  // Overridden Algorithm methods
  bool processGroups();
private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();
  bool checkGroups();
  const double m_T2meV;  //conversion factor from Kelvin to meV

}; // class SassenaFFT

} // namespace Algorithm
} // namespace Mantid

#endif // MANTID_ALGORITHM_SASSENAFFT_H_
