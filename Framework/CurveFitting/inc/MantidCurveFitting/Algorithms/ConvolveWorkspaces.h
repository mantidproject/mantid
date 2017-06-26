#ifndef MANTID_CURVEFITTING_ConvolveWorkspaces_H_
#define MANTID_CURVEFITTING_ConvolveWorkspaces_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/Functions/CubicSpline.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {
/** Convolution of two workspaces

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class DLLExport ConvolveWorkspaces : public API::Algorithm {
public:
  ConvolveWorkspaces() : API::Algorithm() {}
  ~ConvolveWorkspaces() override {}
  /// Algorithm's name
  const std::string name() const override { return "ConvolveWorkspaces"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Convolution of two workspaces.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Workspaces"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  void convolve(MantidVec &xValues, const MantidVec &Y1, const MantidVec &Y2,
                MantidVec &out) const;
  std::unique_ptr<API::Progress> m_progress = nullptr;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_ConvolveWorkspaces_H_*/
