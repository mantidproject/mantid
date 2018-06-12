#ifndef MANTID_ALGORITHMS_INTERPOLATINGREBIN_H_
#define MANTID_ALGORITHMS_INTERPOLATINGREBIN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Rebin.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace HistogramData {
class HistogramE;
class Histogram;
class BinEdges;
class Points;
}
namespace Algorithms {
/**Uses cubic splines to interpolate the mean rate of change of the integral
  over the inputed data bins to that for the user supplied bins.
  Note that this algorithm was inplemented to provide a little more resolution
  on high count rate data. Whether it is more accurate than the standard rebin
  for all, or your, applications requires thought.
  The input data must be a distribution (proportional to the rate of change e.g.
  raw_counts/bin_widths) but note that these mean rate of counts data
  are integrals not (instanteously) sampled data. The error values on each point
  are a weighted mean of the error values from the surrounding input data. This
  makes sense if the interpolation error is low compared to the statistical
  errors on each input data point. The weighting is inversely proportional to
  the distance from the original data point to the new interpolated one.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the workspace to take as input. Must
  contain histogram data. </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
  result. </LI>
    <LI> RebinParameters - The new bin boundaries in the form
  X1,deltaX1,X2,deltaX2,X3,... </LI>
    </UL>

    The algorithms used in the VectorHelper::rebin() and
  VectorHelper::createAxisFromRebinParams()
    are based on the algorithms used in OPENGENIE /src/transform_utils.cxx
  (Freddie Akeroyd, ISIS).
    When calculating the bin boundaries, if the last bin ends up with a width
  being less than 25%
    of the penultimate one, then the two are combined.

    @author Steve Williams, STFC
    @date 05/05/2010

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class DLLExport InterpolatingRebin : public Rebin {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "InterpolatingRebin"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Creates a workspace with different x-value bin boundaries where "
           "the new y-values are estimated using cubic splines.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Rebin"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Rebin"; }
  /// Alias for the algorithm. Must override so it doesn't get parent class's
  const std::string alias() const override { return ""; }

protected:
  const std::string workspaceMethodName() const override { return ""; }
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  void outputYandEValues(API::MatrixWorkspace_const_sptr inputW,
                         const HistogramData::BinEdges &XValues_new,
                         API::MatrixWorkspace_sptr outputW);
  HistogramData::Histogram
  cubicInterpolation(const HistogramData::Histogram &oldHistogram,
                     const HistogramData::BinEdges &xNew) const;

  HistogramData::Histogram
  noInterpolation(const HistogramData::Histogram &oldHistogram,
                  const HistogramData::BinEdges &xNew) const;

  double estimateError(const HistogramData::Points &xsOld,
                       const HistogramData::HistogramE &esOld,
                       const double xNew) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_INTERPOLATINGREBIN_H_*/
