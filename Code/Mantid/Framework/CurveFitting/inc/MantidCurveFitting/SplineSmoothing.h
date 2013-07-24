#ifndef MANTID_CURVEFITTING_SPLINESMOOTHING_H_
#define MANTID_CURVEFITTING_SPLINESMOOTHING_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/CubicSpline.h"

namespace Mantid
{
namespace CurveFitting
{

  /** SplineSmoothing : TODO: DESCRIPTION
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport SplineSmoothing  : public API::Algorithm
  {
  public:
    SplineSmoothing();
    virtual ~SplineSmoothing();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

  private:

    /// number of smoothing points to start with
    const int M_START_SMOOTH_POINTS;

    boost::shared_ptr<CubicSpline> m_cspline;

    virtual void initDocs();
    void init();
    void exec();

    API::MatrixWorkspace_sptr setupOutputWorkspace(API::MatrixWorkspace_sptr inws, int size) const;

    API::MatrixWorkspace_sptr convertBinnedData(API::MatrixWorkspace_sptr workspace) const;

    void setSmoothingPoint(const int index, const double xpoint, const double ypoint) const;

    void selectSmoothingPoints(std::set<int>& xPoints,
        API::MatrixWorkspace_const_sptr inputWorkspace, size_t row) const;

    void calculateSmoothing(API::MatrixWorkspace_const_sptr inputWorkspace,
      API::MatrixWorkspace_sptr outputWorkspace, size_t row) const;

    void calculateDerivatives(API::MatrixWorkspace_const_sptr inputWorkspace,
        API::MatrixWorkspace_sptr outputWorkspace, int order, size_t row) const;

    void addSmoothingPoints(const std::set<int>& points,
        const double* xs, const double* ys) const;

    bool checkSmoothingAccuracy(const int start, const int end,
        const double* ys, const double* ysmooth) const;
  };


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_SPLINESMOOTHING_H_ */
