#ifndef MANTID_CURVEFITTING_MULTIBG_H_
#define MANTID_CURVEFITTING_MULTIBG_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/MatrixWorkspace.h"

#ifdef _WIN32
#pragma warning(disable : 4250)
#endif

namespace Mantid {

namespace API {
class WorkspaceGroup;
}

namespace CurveFitting {
/** A composite function.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 19/08/2011

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport MultiBG : public API::CompositeFunction {
public:
  /// Default constructor
  MultiBG() : API::CompositeFunction() {}
  /// Destructor
  virtual ~MultiBG();

  /* Overriden methods */

  using CompositeFunction::setWorkspace;
  virtual void setWorkspace(boost::shared_ptr<const API::Workspace>) {}
  /// Set the workspace
  void setWorkspace(boost::shared_ptr<const API::Workspace> ws, bool copyData);
  void setSlicing(const std::string &slicing);
  virtual boost::shared_ptr<const API::Workspace> getWorkspace() const {
    return m_spectra[0].first;
  }
  /// Returns the function's name
  std::string name() const { return "MultiBG"; }
  /// Returns the function's category
  virtual const std::string category() const { return "Background"; }

  virtual void function(API::FunctionDomain &) const {}

  /// Returns the size of the fitted data (number of double values returned by
  /// the function)
  virtual size_t dataSize() const { return m_data.size(); }
  /// Returns a reference to the fitted data. These data are taken from the
  /// workspace set by setWorkspace() method.
  virtual const double *getData() const { return &m_data[0]; }
  virtual const double *getWeights() const { return &m_weights[0]; }
  /// Function you want to fit to.
  void function(double *out) const;
  /// Derivatives of function with respect to active parameters
  void functionDeriv(API::Jacobian *out);
  void functionDeriv(API::FunctionDomain &domain, API::Jacobian &jacobian) {
    API::IFunction::functionDeriv(domain, jacobian);
  }

protected:
  boost::shared_ptr<API::WorkspaceGroup> createCalculatedWorkspaceGroup(
      const std::vector<double> &sd = std::vector<double>());

  boost::shared_ptr<const API::MatrixWorkspace> m_workspace;

  /// to collect different workspaces found in child functions
  std::vector<std::pair<boost::shared_ptr<const API::MatrixWorkspace>, size_t>>
      m_spectra;
  /// to store function indices to workspaces: m_funIndex[i] gives vector of
  /// indexes of m_spectra for function i
  std::vector<std::vector<size_t>> m_funIndex;
  /// the data vector which is a composition of all fitted spectra
  std::vector<double> m_data;
  /// the vector of fitting weights, one for each value in m_data
  std::vector<double> m_weights;
  /// offsets of particular workspaces in the m_data and m_weights arrays
  std::vector<size_t> m_offset;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_MULTIBG_H_*/
