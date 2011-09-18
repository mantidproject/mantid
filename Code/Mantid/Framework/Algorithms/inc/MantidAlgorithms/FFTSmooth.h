#ifndef MANTID_ALGORITHM_FFTSMOOTH_H_
#define MANTID_ALGORITHM_FFTSMOOTH_H_
/*WIKI* 

FFTSmooth uses the FFT algorithm to create a Fourier transform of a spectrum, applies a filter to it and transforms it back. The filters remove higher frequencies from the spectrum which reduces the noise.

The second version of the FFTSmooth algorithm has two filters:

===Zeroing===
* Filter: "Zeroing"
* Params: "n" - an integer greater than 1 meaning that the Fourier coefficients with frequencies outside the 1/n of the original range will be set to zero.

===Butterworth===
* Filter: "Butterworth"
* Params: A string containing two positive integer parameters separated by a comma, such as 20,2.  

"n"- the first integer, specifies the cutoff frequency for the filter, in the same way as for the "Zeroing" filter.  That is, the cutoff is at m/n where m is the original range. "n" is required to be strictly more than 1.

"order"- the second integer, specifies the order of the filter.  For low order values, such as 1 or 2, the Butterworth filter will smooth the data without the strong "ringing" artifacts produced by the abrupt cutoff of the "Zeroing" filter.  As the order parameter is increased, the action of the "Butterworth" filter will approach the action of the "Zeroing" filter.

For both filter types, the resulting spectrum has the same size as the original one.


*WIKI*/

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
