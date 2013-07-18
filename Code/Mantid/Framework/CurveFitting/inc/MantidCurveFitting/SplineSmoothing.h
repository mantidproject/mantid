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
    virtual void initDocs();
    void init();
    void exec();

    API::MatrixWorkspace_sptr setupOutputWorkspace(API::MatrixWorkspace_sptr inws) const;

    API::MatrixWorkspace_sptr convertBinnedData(API::MatrixWorkspace_sptr workspace) const;

    void setSmoothingPoints(CubicSpline_const_sptr cspline,
        API::MatrixWorkspace_const_sptr inputWorkspace, size_t row) const;

    void calculateSmoothing(CubicSpline_const_sptr cspline,
      API::MatrixWorkspace_const_sptr inputWorkspace,
      API::MatrixWorkspace_sptr outputWorkspace, size_t row) const;

    void calculateDerivatives(CubicSpline_const_sptr cspline,
        API::MatrixWorkspace_const_sptr inputWorkspace,
        API::MatrixWorkspace_sptr outputWorkspace, int order, size_t row) const;
  };


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_SPLINESMOOTHING_H_ */
