// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Axis.h"

#include <string>
#include <vector>

namespace Mantid {
namespace API {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class MatrixWorkspace;

/** Class to represent a numeric axis of a workspace.

    @author Roman Tolchenov, Tessella plc
    @date 05/07/2010
*/
class MANTID_API_DLL NumericAxis : public Axis {
public:
  NumericAxis(const std::size_t &length);
  NumericAxis(std::vector<double> centres);

  Axis *clone(const MatrixWorkspace *const parentWorkspace) override;
  Axis *clone(const std::size_t length, const MatrixWorkspace *const parentWorkspace) override;
  /// Is the axis numeric - always true for this class
  bool isNumeric() const override { return true; }
  std::size_t length() const override { return m_values.size(); }
  /// Get a value at the specified index
  double operator()(const std::size_t &index, const std::size_t &verticalIndex = 0) const override;
  /// Set the value at a specific index
  void setValue(const std::size_t &index, const double &value) override;
  size_t indexOfValue(const double value) const override;
  bool operator==(const NumericAxis &) const;
  bool operator==(const Axis &) const override;
  virtual bool equalWithinTolerance(const Axis &axis2, const double tolerance) const;
  std::string label(const std::size_t &index) const override;
  /// Create bin boundaries from the point values
  virtual std::vector<double> createBinBoundaries() const;
  /// Return a const reference to the values
  virtual const std::vector<double> &getValues() const;
  /// returns min value defined on axis
  double getMin() const override { return m_values.front(); }
  /// returns max value defined on axis
  double getMax() const override { return m_values.back(); }

protected:
  /// Default constructor
  NumericAxis();

  /// A vector holding the centre values.
  std::vector<double> m_values;

  /// Get number label
  std::string formatLabel(const double value) const;
};

} // namespace API
} // namespace Mantid
