#ifndef MANTID_ALGORITHMS_GetDetOffsetsMultiPeaks_H_
#define MANTID_ALGORITHMS_GetDetOffsetsMultiPeaks_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
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

 File change history is stored at: <https://github.com/mantidproject/mantid>
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
  /// Call Gaussian as a Child Algorithm to fit the peak in a spectrum
  void fitSpectra(const int64_t s, API::MatrixWorkspace_sptr inputW, const std::vector<double> &peakPositions, const std::vector<double> &fitWindows, size_t &nparams,
                  double &minD, double &maxD,
                  std::vector<double>&peakPosToFit, std::vector<double> &peakPosFitted, std::vector<double> &chisq);

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();
  std::string m_backType;
  std::string m_peakType;
  double m_maxChiSq;

  DataObjects::TableWorkspace_sptr m_infoTableWS;
  DataObjects::TableWorkspace_sptr m_peakOffsetTableWS;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_GetDetOffsetsMultiPeaks_H_*/
