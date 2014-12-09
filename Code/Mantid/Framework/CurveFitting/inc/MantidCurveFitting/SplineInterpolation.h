#ifndef MANTID_CURVEFITTING_SPLINEINTERPOLATION_H_
#define MANTID_CURVEFITTING_SPLINEINTERPOLATION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidCurveFitting/CubicSpline.h"

namespace Mantid
{
namespace CurveFitting
{

  /** Takes two workspaces as input. One contain a set of points which define a spline,
    and one which contains a number of spectra to be interpolated against spline.

    Produces an output workspace containing the interpolated points

    Optionally the algorithm will also produce a grouped workspace of derivatives of up to order 2
    for each of the interpolated points.

    @author Samuel Jackson, STFC
    @date 25/07/2013
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport SplineInterpolation  : public API::Algorithm
  {
  public:
    SplineInterpolation();
    virtual ~SplineInterpolation();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Interpolates a set of spectra onto a spline defined by a second input workspace. Optionally, this algorithm can also calculate derivatives up to order 2 as a side product";}

  private:
    void init();
    void exec();

    /// CubicSpline member used to perform interpolation
    boost::shared_ptr<CubicSpline> m_cspline;

    /// setup an output workspace using meta data from inws and taking a number of spectra
    API::MatrixWorkspace_sptr setupOutputWorkspace(API::MatrixWorkspace_sptr inws, int size) const;

    /// convert a binned workspace to point data. Uses mean of the bins as point
    API::MatrixWorkspace_sptr convertBinnedData(API::MatrixWorkspace_sptr workspace) const;

    /// set the points that define the spline used for interpolation of a workspace
    void setInterpolationPoints(API::MatrixWorkspace_const_sptr inputWorkspace, const int row) const;

    /// Calculate the interpolation of the input workspace against the spline and store it in outputWorkspace
    void calculateSpline(API::MatrixWorkspace_const_sptr inputWorkspace,
        API::MatrixWorkspace_sptr outputWorkspace, int row) const;

    /// Calculate the derivatives of the input workspace from the spline.
    void calculateDerivatives(API::MatrixWorkspace_const_sptr inputWorkspace,
        API::MatrixWorkspace_sptr outputWorkspace, int order) const;

  };


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_SPLINEINTERPOLATION_H_ */
