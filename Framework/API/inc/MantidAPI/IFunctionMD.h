#ifndef MANTID_API_IFUNCTIONMD_H_
#define MANTID_API_IFUNCTIONMD_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Unit.h"
#ifndef Q_MOC_RUN
#include <boost/scoped_array.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#endif
#include <string>
#include <vector>

namespace Mantid {

namespace API {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class IMDWorkspace;
class IMDIterator;
class Jacobian;
class ParameterTie;
class IConstraint;
class ParameterReference;
class FunctionHandler;
class FunctionDomainMD;

/** This is a specialization of IFunction for functions defined on an
   IMDWorkspace.
    It uses FunctionDomainMD as a domain. Implement functionMD(...) method in a
   concrete
    function.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 12/01/2011

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
class MANTID_API_DLL IFunctionMD : public virtual IFunction {
public:
  /* Overidden methods */

  /// Virtual copy constructor
  boost::shared_ptr<IFunction> clone() const override;
  /// Set the workspace.
  /// @param ws :: Shared pointer to a workspace
  void setWorkspace(boost::shared_ptr<const Workspace> ws) override;

  void function(const FunctionDomain &domain,
                FunctionValues &values) const override;
  void functionDeriv(const FunctionDomain &domain,
                     Jacobian &jacobian) override {
    calNumericalDeriv(domain, jacobian);
  }

protected:
  /// Performs the function evaluations on the MD domain
  void evaluateFunction(const FunctionDomainMD &domain,
                        FunctionValues &values) const;

  virtual void useDimension(const std::string &id);
  /// Do finction initialization after useAllDimensions() was called
  virtual void initDimensions() {}
  /// Does the function evaluation. Must be implemented in derived classes.
  virtual double functionMD(const IMDIterator &r) const = 0;

  /// maps dimension id to its index in m_dimensions
  std::map<std::string, size_t> m_dimensionIndexMap;
  /// dimensions used in this function in the expected order
  std::vector<boost::shared_ptr<const Mantid::Geometry::IMDDimension>>
      m_dimensions;

private:
  /// Use all the dimensions in the workspace
  virtual void
  useAllDimensions(boost::shared_ptr<const IMDWorkspace> workspace);
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IFUNCTIONMD_H_*/
