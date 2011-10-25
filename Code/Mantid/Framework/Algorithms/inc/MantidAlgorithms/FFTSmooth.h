#ifndef MANTID_ALGORITHM_FFTSMOOTH_H_
#define MANTID_ALGORITHM_FFTSMOOTH_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/** Data smoothing using the FFT algorithm and various filters.

    @author Roman Tolchenov
    @date 07/07/2009

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport FFTSmooth : public API::Algorithm
{
public:
  /// Default constructor
  FFTSmooth() : API::Algorithm() {};
  /// Destructor
  virtual ~FFTSmooth() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FFTSmooth";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "General";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();

  // Smoothing by truncation.
  void truncate(int n);
  // Smoothing by zeroing.
  void zero(int n);

  /// The input workspace
  API::MatrixWorkspace_sptr m_inWS;
  /// Temporary workspace for keeping the unfiltered Fourier transform of the imput spectrum
  API::MatrixWorkspace_sptr m_unfilteredWS;
  /// Temporary workspace for keeping the filtered spectrum
  API::MatrixWorkspace_sptr m_filteredWS;
  /// Variable for storing the lowest x value
  double m_x0;

};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_FFTSMOOTH_H_*/
