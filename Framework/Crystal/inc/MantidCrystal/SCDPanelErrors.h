#ifndef MANTID_CRYSTAL_SCDPANELERRORS_H_
#define MANTID_CRYSTAL_SCDPANELERRORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidKernel/System.h"
#include "MantidAPI/Workspace_fwd.h"
#include <cmath>

namespace Mantid {
namespace Crystal {
/*
@author Vickie Lynch, SNS
@date 7/25/2016

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SCDPanelErrors : public API::ParamFunction,
                                 public API::IFunction1D {
public:
  /// Constructor
  SCDPanelErrors();

  /// overwrite IFunction base class methods
  std::string name() const override { return "SCDPanelErrors"; }
  const std::string category() const override { return "General"; }
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  ///  function derivatives
  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData) override;

  /// Set a value to attribute attName
  void setAttribute(const std::string &attName,
                    const IFunction::Attribute &value) override;

  /// Move detectors with parameters
  void moveDetector(double x, double y, double z, double rotx, double roty,
                    double rotz, double scalex, double scaley,
                    std::string detname, API::Workspace_sptr inputW) const;

private:
  /// Call the appropriate load function
  void load(const std::string &fname);

  /// Load the points from a Workspace
  void loadWorkspace(const std::string &wsName) const;

  /// Load the points from a Workspace
  void loadWorkspace(boost::shared_ptr<API::Workspace> ws) const;

  /// Clear all data
  void clear() const;

  /// Evaluate the function for a list of arguments and given scaling factor
  void eval(double xshift, double yshift, double zshift, double xrotate,
            double yrotate, double zrotate, double scalex, double scaley,
            double *out, const double *xValues, const size_t nData,
            double tShift) const;

  /// Fill in the workspace and bank names
  void setupData() const;

  /// The default value for the workspace index
  static const int defaultIndexValue;

  /// Temporary workspace holder
  mutable boost::shared_ptr<API::Workspace> m_workspace;

  /// Stores bank
  mutable std::string m_bank;

  /// Flag of completing data setup
  mutable bool m_setupFinished;
};

} // namespace Crystal
} // namespace Mantid

#endif /*MANTID_CRYSTAL_SCDPANELERRORS_H_*/
