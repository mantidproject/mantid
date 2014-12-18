#ifndef MANTID_ALGORITHMS_GETDETECTOROFFSETS_H_
#define MANTID_ALGORITHMS_GETDETECTOROFFSETS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/OffsetsWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/**
 Find the offsets for each detector

 @author Laurent Chapon, ISIS Facility, Rutherford Appleton Laboratory
 @date 08/03/2009

 Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport GetDetectorOffsets: public API::Algorithm
{
public:
  /// Default constructorMatrix
  GetDetectorOffsets();
  /// Destructor
  virtual ~GetDetectorOffsets();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "GetDetectorOffsets"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Creates an OffsetsWorkspace containing offsets for each detector. You can then save these to a .cal file using SaveCalFile.";}

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction"; }

private:
  
  // Overridden Algorithm methods
  void init();
  void exec();
  /// Call Gaussian as a Child Algorithm to fit the peak in a spectrum
  double fitSpectra(const int64_t s, bool isAbsolbute);
  /// Create a function string from the given parameters and the algorithm inputs
  API::IFunction_sptr createFunction(const double peakHeight, const double peakLoc);
  /// Read in all the input parameters
  void retrieveProperties();
  
  
  API::MatrixWorkspace_sptr inputW;  ///< A pointer to the input workspace
  DataObjects::OffsetsWorkspace_sptr outputW; ///< A pointer to the output workspace
  double Xmin;        ///< The start of the X range for fitting
  double Xmax;        ///< The end of the X range for fitting
  double maxOffset;   ///< The maximum absolute value of offsets
  double dreference;  ///< The expected peak position in d-spacing (?)
  double dideal;      ///< The known peak centre value from the NIST standard information
  double step;        ///< The step size
  int64_t nspec;      ///< The number of spectra in the input workspace
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_GETDETECTOROFFSETS_H_*/
