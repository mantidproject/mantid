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
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/Unit.h"

#ifndef Q_MOC_RUN
#include <boost/lexical_cast.hpp>
#include <memory>
#endif

#include <string>
#include <vector>

namespace Mantid {
namespace API {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class MatrixWorkspace;

/** Class to represent a text axis of a workspace.

    @author Roman Tolchenov, Tessella plc
    @date 06/07/2010
*/
class MANTID_API_DLL TextAxis : public Axis {
public:
  TextAxis(const std::size_t &length);
  Axis *clone(const MatrixWorkspace *const parentWorkspace) override;
  Axis *clone(const std::size_t length, const MatrixWorkspace *const parentWorkspace) override;
  std::size_t length() const override { return m_values.size(); }
  /// If this is a TextAxis, always return true for this class
  bool isText() const override { return true; }
  /// Get a value at the specified index
  double operator()(const std::size_t &index, const std::size_t &verticalIndex = 0) const override;
  /// Set the value at the specified index
  void setValue(const std::size_t &index, const double &value) override;
  size_t indexOfValue(const double value) const override;

  bool operator==(const TextAxis &) const;
  bool operator==(const Axis &) const override;
  /// Get the label at the specified index
  std::string label(const std::size_t &index) const override;
  /// Set the label at the given index
  void setLabel(const std::size_t &index, const std::string &lbl);
  /// returns min value defined on axis
  double getMin() const override;
  /// returns max value defined on axis
  double getMax() const override;

private:
  /// A vector holding the axis values for the axis.
  std::vector<std::string> m_values;
  bool compareToTextAxis(const TextAxis &axis2) const;
};

} // namespace API
} // namespace Mantid
