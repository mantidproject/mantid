// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/NumericAxis.h"

namespace Mantid {
namespace API {
/** A class to represent the axis of a 2D (or more) workspace where the value at
    a given point on the axis varies along the other dimension.
    This class does not hold the axis values itself; they are held by the X
   vectors
    of the workspace itself.
    An axis of this kind is always of numeric type.

    @author Russell Taylor, Tessella Support Services plc
    @date 18/05/2008
*/
class MANTID_API_DLL RefAxis : public NumericAxis {
public:
  RefAxis(const MatrixWorkspace *const parentWorkspace);

  Axis *clone(const MatrixWorkspace *const parentWorkspace) override;
  Axis *clone(const std::size_t length, const MatrixWorkspace *const parentWorkspace) override;
  std::size_t length() const override;
  /// Get a value at the specified index
  double operator()(const std::size_t &index, const std::size_t &verticalIndex) const override;
  void setValue(const std::size_t &index, const double &value) override;
  bool operator==(const Axis &) const override;
  bool equalWithinTolerance(const Axis &axis2, const double tolerance) const override;
  // We must override these to prevent access to NumericAxis::m_values and
  // m_edges, which are unused by RefAxis and thus do not hold sensible values.
  size_t indexOfValue(const double value) const override;
  std::vector<double> createBinBoundaries() const override;
  const std::vector<double> &getValues() const override;
  double getMin() const override;
  double getMax() const override;

private:
  RefAxis(const RefAxis &right, const MatrixWorkspace *const parentWorkspace);
  /// Private, undefined 'regular' copy constructor
  RefAxis(const RefAxis &);
  /// Private, undefined copy assignment operator
  const RefAxis &operator=(const RefAxis &);

  /// A pointer to the workspace holding the axis
  const MatrixWorkspace *const m_parentWS;
};

} // namespace API
} // namespace Mantid
