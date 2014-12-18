#ifndef MANTID_API_ICOSTFUNCTION_H_
#define MANTID_API_ICOSTFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/CostFunctionFactory.h"

namespace Mantid
{
namespace API
{
/** An interface for specifying the cost function to be used with Fit algorithm or minimizers,
    for example, the default being least squares fitting.

    @author Anders Markvardsen, ISIS, RAL
    @date 11/05/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class MANTID_API_DLL ICostFunction 
{
public:
  /// Virtual destructor
  virtual ~ICostFunction() {}

  /// Get name of minimizer
  virtual std::string name() const = 0;

  /// Get short name of minimizer - useful for say labels in guis
  virtual std::string shortName() const {return "Quality";}

  /// Get i-th parameter
  /// @param i :: Index of a parameter
  /// @return :: Value of the parameter
  virtual double getParameter(size_t i)const = 0;
  /// Set i-th parameter
  /// @param i :: Index of a parameter
  /// @param value :: New value of the parameter
  virtual void setParameter(size_t i, const double& value) = 0;
  /// Number of parameters
  virtual size_t nParams()const = 0;

  /// Calculate value of cost function
  virtual double val() const = 0;

  /// Calculate the derivatives of the cost function
  /// @param der :: Container to output the derivatives
  virtual void deriv(std::vector<double>& der) const = 0;

  /// Calculate the value and the derivatives of the cost function
  /// @param der :: Container to output the derivatives
  /// @return :: The value of the function
  virtual double valAndDeriv(std::vector<double>& der) const = 0;
};

/// define a shared pointer to a cost function
typedef boost::shared_ptr<ICostFunction> ICostFunction_sptr;

/**
 * Macro for declaring a new type of cost functions to be used with the CostFunctionFactory
 */
#define DECLARE_COSTFUNCTION(classname,username) \
        namespace { \
	Mantid::Kernel::RegistrationHelper register_costfunction_##classname( \
  ((Mantid::API::CostFunctionFactory::Instance().subscribe<classname>(#username)) \
	, 0)); \
	} 

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_ICOSTFUNCTION_H_*/
