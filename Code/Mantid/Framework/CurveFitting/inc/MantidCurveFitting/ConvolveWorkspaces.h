#ifndef MANTID_CURVEFITTING_ConvolveWorkspaces_H_
#define MANTID_CURVEFITTING_ConvolveWorkspaces_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidCurveFitting/CubicSpline.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"

namespace Mantid
{
namespace CurveFitting
{
/** Convolution of two workspaces

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

class DLLExport ConvolveWorkspaces : public API::Algorithm
{
public:
  ConvolveWorkspaces();
  virtual ~ConvolveWorkspaces();
  /// Algorithm's name
  virtual const std::string name() const { return "ConvolveWorkspaces"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Convolution of two workspaces.";}

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Utility\\Workspaces"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  void convolve(MantidVec& xValues, const MantidVec& Y1, const MantidVec& Y2, MantidVec& out)const;
  API::Progress * prog;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_ConvolveWorkspaces_H_*/
