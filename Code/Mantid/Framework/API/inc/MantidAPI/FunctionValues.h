#ifndef MANTID_API_FUNCTIONVALUES_H_
#define MANTID_API_FUNCTIONVALUES_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FunctionDomain.h"

#include <vector>

namespace Mantid {
namespace API {
/** A class to store values calculated by a function. It also can contain data
    a function can fit to.

    @author Roman Tolchenov, Tessella plc
    @date 15/11/2011

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
class MANTID_API_DLL FunctionValues {
public:
  /// Default constructor.
  FunctionValues() {}
  /// Constructor.
  FunctionValues(const FunctionDomain &domain);
  /// Copy constructor.
  FunctionValues(const FunctionValues &values);
  /// Destructor
  ~FunctionValues();

  /// Return the number of values
  size_t size() const { return m_calculated.size(); }
  /// Expand values to a new size, preserve stored values.
  void expand(size_t n);

  /// Get a pointer to calculated data at index i
  double *getPointerToCalculated(size_t i);
  /// Set all calculated values to zero
  void zeroCalculated();
  /// set all calculated values to same number
  void setCalculated(double value);

  /// Reset the values to match a new domain.
  void reset(const FunctionDomain &domain);
  /// Store i-th calculated value.
  /// @param i :: Index of the stored value 0 <= i < size()
  /// @param value :: A value to store.
  void setCalculated(size_t i, double value) { m_calculated[i] = value; }
  /// Get i-th calculated value.
  /// @param i :: An index of a value 0 <= i < size()
  double getCalculated(size_t i) const { return m_calculated[i]; }
  /// Get i-th calculated value.
  /// @param i :: An index of a value 0 <= i < size()
  double operator[](size_t i) const { return m_calculated[i]; }
  /// Add a number to a calculated value.
  /// @param i :: An index of a value 0 <= i < size() to update.
  /// @param value :: A value to add
  void addToCalculated(size_t i, double value) { m_calculated[i] += value; }

  /// Add other calculated values
  FunctionValues &operator+=(const FunctionValues &values);
  /// Multiply by other calculated values
  FunctionValues &operator*=(const FunctionValues &values);
  /// Add other calculated values with offset
  void addToCalculated(size_t start, const FunctionValues &values);

  /// Set a fitting data value
  void setFitData(size_t i, double value);
  /// Set all fitting data values
  void setFitData(const std::vector<double> &values);
  /// Get a fitting data value
  double getFitData(size_t i) const;
  /// Set a fitting weight
  void setFitWeight(size_t i, double value);
  /// Set all fitting weights.
  void setFitWeights(const std::vector<double> &values);
  /// Set all fitting weights to a number.
  void setFitWeights(const double &value);
  /// Get a fitting weight
  double getFitWeight(size_t i) const;
  /// Set all calculated values by copying them from another FunctionValues
  /// instance.
  void setFitDataFromCalculated(const FunctionValues &values);

protected:
  /// Copy calculated values to a buffer
  /// @param to :: Pointer to the buffer
  void copyTo(double *to) const;
  /// Add calculated values to values in a buffer and save result to the buffer
  /// @param to :: Pointer to the buffer, it must be large enough
  void add(double *to) const;
  /// Multiply calculated values by values in a buffer and save result to the
  /// buffer
  /// @param to :: Pointer to the buffer, it must be large enough
  void multiply(double *to) const;

  std::vector<double> m_calculated; ///< buffer for calculated values
  std::vector<double> m_data;       ///< buffer for fit data
  std::vector<double>
      m_weights; ///< buffer for fitting weights (reciprocal errors)
};

/// typedef for a shared pointer
typedef boost::shared_ptr<FunctionValues> FunctionValues_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_FUNCTIONVALUES_H_*/
