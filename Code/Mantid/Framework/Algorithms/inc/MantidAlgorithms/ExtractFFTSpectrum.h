#ifndef MANTID_ALGORITHMS_EXTRACT_FFT_SPECTRUM_H_
#define MANTID_ALGORITHMS_EXTRACT_FFT_SPECTRUM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/**
    This algorithm performs a Fast Fourier Transform on each spectra of the input workspace.
    It then takes a specified part of the FFT result (parameter "FFTPart") and places it in
    a new workspace, which will share the Y axis of the old one (ie, spectra-detector map) and
    have the unit label set to "Time / ns" (a non-functional unit).

    @author Michael Whitty, STFC ISIS
    @date 21/09/2010

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ExtractFFTSpectrum : public API::Algorithm
{
public:
  /// (Empty) Constructor
  ExtractFFTSpectrum() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~ExtractFFTSpectrum() {}
  /// Algorithm's name
  virtual const std::string name() const { return "ExtractFFTSpectrum"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "This algorithm performs a Fast Fourier Transform on each spectrum in a workspace, and from the result takes the indicated spectrum and places it into the OutputWorkspace, so that you end up with one result spectrum for each input spectrum in the same workspace.";}

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Arithmetic\\FFT"; }

private:
  
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EXTRACT_FFT_SPECTRUM_H_*/
