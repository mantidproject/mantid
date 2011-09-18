#ifndef MANTID_ALGORITHMS_INTERPOLATINGREBIN_H_
#define MANTID_ALGORITHMS_INTERPOLATINGREBIN_H_
/*WIKI* 

This algorithms is useful for increasing the time resolution of spectra whose bins have large numbers of counts which vary smoothly e.g. monitor spectra.

The "params" property defines the new bin boundaries using the same syntax as in [[Rebin]]. That is, the first number is the first bin boundary and the second number is the width of the bins. Bins are created until the third number would be exceeded, the third number is the x-value of the last bin. There can be further pairs of numbers, the first in the pair will be the bin width and the last number the last bin boundary. 

The bin immediately before the specified boundaries <math>x_2</math>, <math>x_3</math>, ... <math>x_i</math> is likely to have a different width from its neighbors because there can be no gaps between bins. Rebin ensures that any of these space filling bins cannot be less than 25% or more than 125% of the width that was specified.

To calculate the y-values the input spectra are approximated with a time series where the value at the center of each bin mean is the mean count rate over the bin. This series is interpolated by calculating cubic splines that fit this series and evaluating the splines at the centers of the requested bin. The splines have natural boundary conditions and zero second derivative at the end points, they are calculated using the [http://www.gnu.org/software/gsl/manual/html_node/Interpolation-Types.html gsl].

The errors on the count rates are estimated as a weighted mean of the errors values for the nearest input bin centers. These weights are inversely proportional to the distance of the output bin center to the respective input bin data points.

=== Example Rebin param strings ===
The same syntax as for [[Rebin]]
;0,100,20000
:From 0 rebin in constant size bins of 100 up to 20,000
;0,100,10000,200,20000
:From 0 rebin in steps of 100 to 10,000 then steps of 200 to 20,000

== Usage ==
'''Python'''
 InterpolatingRebin("InWS2","OutWS","x1,dx1,x2")


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Rebin.h"
#include "MantidDataObjects/Histogram1D.h"

namespace Mantid
{
namespace Algorithms
{
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
    <LI> InputWorkspace  - The name of the workspace to take as input. Must contain histogram data. </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result. </LI>
    <LI> RebinParameters - The new bin boundaries in the form X1,deltaX1,X2,deltaX2,X3,... </LI>
    </UL>

    The algorithms used in the VectorHelper::rebin() and VectorHelper::createAxisFromRebinParams() 
    are based on the algorithms used in OPENGENIE /src/transform_utils.cxx (Freddie Akeroyd, ISIS).
    When calculating the bin boundaries, if the last bin ends up with a width being less than 25%
    of the penultimate one, then the two are combined. 

    @author Steve Williams, STFC
    @date 05/05/2010

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport InterpolatingRebin : public Algorithms::Rebin
{
public:
  /// Default constructor
  InterpolatingRebin() : Algorithms::Rebin() {};
  /// Destructor
  virtual ~InterpolatingRebin() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "InterpolatingRebin";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Rebin";}

protected:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  virtual void exec();

  void outputYandEValues(API::MatrixWorkspace_const_sptr inputW, const MantidVecPtr &XValues_new, API::MatrixWorkspace_sptr outputW);
  void cubicInterpolation(const MantidVec &xOld, const MantidVec &yOld, const MantidVec &eOld, const MantidVec& xNew, MantidVec &yNew, MantidVec &eNew) const;
  void noInterpolation(const MantidVec &xOld, const double yOld, const MantidVec &eOld, const MantidVec& xNew, MantidVec &yNew, MantidVec &eNew) const;
  double estimateError(const MantidVec &xsOld, const MantidVec &esOld, const double xNew) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_INTERPOLATINGREBIN_H_*/
