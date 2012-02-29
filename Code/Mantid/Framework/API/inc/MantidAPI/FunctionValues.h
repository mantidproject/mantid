#ifndef MANTID_API_FUNCTIONVALUES_H_
#define MANTID_API_FUNCTIONVALUES_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FunctionDomain.h"

#include <vector>

namespace Mantid
{
namespace API
{
/** Base class that represents the domain of a function.
    A domain is a generalisation of x (argument) and y (value) arrays.
    A domain consists at least of a list of function arguments for which a function should 
    be evaluated and a buffer for the calculated values. If used in fitting also contains
    the fit data and weights.

    @author Roman Tolchenov, Tessella plc
    @date 15/11/2011

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL FunctionValues
{
public:
  /// Default constructor.
  FunctionValues(){}
  /// Constructor.
  FunctionValues(const FunctionDomain& domain);
  /// Copy constructor.
  FunctionValues(const FunctionValues& values);
  /// Reset the values to match a new domain.
  void reset(const FunctionDomain& domain);
  /// Return the number of points, values, etc in the domain
  size_t size() const {return m_calculated.size();}
  /// store i-th calculated value. 0 <= i < size()
  void setCalculated(size_t i,double value) {m_calculated[i] = value;}
  /// get i-th calculated value. 0 <= i < size()
  double getCalculated(size_t i) const {return m_calculated[i];}
  /// Get a pointer to calculated data at index i
  double* getPointerToCalculated(size_t i);
  /// Add other calculated values
  FunctionValues& operator+=(const FunctionValues& values);
  /// Set all calculated values to zero
  void zeroCalculated();

  /// set a fitting data value
  void setFitData(size_t i,double value);
  void setFitData(const std::vector<double>& values);
  /// get a fitting data value
  double getFitData(size_t i) const;
  /// set a fitting weight
  void setFitWeight(size_t i,double value);
  void setFitWeights(const std::vector<double>& values);
  void setFitWeights(const double& value);
 /// get a fitting weight
  double getFitWeight(size_t i) const;
  void setFitDataFromCalculated(const FunctionValues& values);
protected:
  std::vector<double> m_calculated; ///< buffer for calculated values
  std::vector<double> m_data;    ///< buffer for fit data
  std::vector<double> m_weights; ///< buffer for fitting weights (reciprocal errors)
};

/// typedef for a shared pointer
typedef boost::shared_ptr<FunctionValues> FunctionValues_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_FUNCTIONVALUES_H_*/
