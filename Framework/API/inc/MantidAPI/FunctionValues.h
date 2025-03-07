// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
*/
class MANTID_API_DLL FunctionValues {
public:
  /// Constructor.
  explicit FunctionValues(size_t n = 0);
  /// Constructor.
  explicit FunctionValues(const FunctionDomain &domain);
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

  /// Return the calculated values as a vector
  const std::vector<double> &toVector() const { return m_calculated; }

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
  /// buffer for calculated values
  std::vector<double> m_calculated;
  /// buffer for fit data
  std::vector<double> m_data;
  /// buffer for fitting weights (reciprocal errors)
  std::vector<double> m_weights;
};

/// typedef for a shared pointer
using FunctionValues_sptr = std::shared_ptr<FunctionValues>;

} // namespace API
} // namespace Mantid
