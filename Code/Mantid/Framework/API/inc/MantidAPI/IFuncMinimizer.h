#ifndef MANTID_API_IFUNCMINIMIZER_H_
#define MANTID_API_IFUNCMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidAPI/ICostFunction.h"

namespace Mantid
{
namespace API
{
// Forward declaration
class IFitFunction;

/** An interface for function minimizers. Minimizers minimize cost functions.

    @author Anders Markvardsen, ISIS, RAL
    @date 11/12/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class MANTID_API_DLL IFuncMinimizer: public Kernel::PropertyManager
{
public:
  /// Virtual destructor
  virtual ~IFuncMinimizer() {}

  /// Initialize minimizer.
  /// @param function :: Function to minimize
  /// @param maxIterations :: Maximum number of iterations.
  virtual void initialize(API::ICostFunction_sptr function, size_t maxIterations = 1000) = 0;

  /// Get name of minimizer
  virtual std::string name() const = 0;

  /// Do one iteration
  /// @param iteration :: Current iteration number. 0 <= iteration < maxIterations
  /// @return :: true if iterations should be continued or false to stop
  virtual bool iterate(size_t iteration) = 0;

  /// Perform iteration with minimizer and return true if successful.
  /// @param maxIterations :: Maximum number of iterations.
  virtual bool minimize(size_t maxIterations = 1000);

  /// Get the error string
  virtual std::string getError() const {return m_errorString;}

  /// Get value of cost function 
  virtual double costFunctionVal() = 0;

protected:
  /// Error string.
  std::string m_errorString;
};

typedef boost::shared_ptr<IFuncMinimizer> IFuncMinimizer_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IFUNCMINIMIZER_H_*/
