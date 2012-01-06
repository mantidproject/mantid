#ifndef MANTID_ALGORITHMS_GetDetOffsetsMultiPeaks_H_
#define MANTID_ALGORITHMS_GetDetOffsetsMultiPeaks_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_statistics.h>

namespace Mantid
{
namespace Algorithms
{
/**
 Find the offsets for each detector

 @author Vickie Lynch, SNS
 @date 12/12/2011

 Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport GetDetOffsetsMultiPeaks: public API::Algorithm
{
public:
  /// Default constructorMatrix
  GetDetOffsetsMultiPeaks();
  /// Destructor
  virtual ~GetDetOffsetsMultiPeaks();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "GetDetOffsetsMultiPeaks"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction"; }
  /// Call Gaussian as a sub-algorithm to fit the peak in a spectrum
  double fitSpectra(const int64_t s, double offset, std::string inname, std::string peakPositions);

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();
  /// Read in all the input parameters
  void retrieveProperties();
  
  
  API::MatrixWorkspace_sptr inputW;  ///< A pointer to the input workspace
  DataObjects::OffsetsWorkspace_sptr outputW; ///< A pointer to the output workspace
  double maxOffset;   ///< The maximum absolute value of offsets
  double dreference;  ///< The expected peak position in d-spacing (?)
  int64_t nspec;          ///< The number of spectra in the input workspace
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_GetDetOffsetsMultiPeaks_H_*/
