// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/IDTypes.h"

#include <memory>
#include <string>

namespace Mantid {
namespace Kernel {
class Unit;
}
namespace API {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class MatrixWorkspace;

/** Class to represent the axis of a workspace.

    @author Russell Taylor, Tessella Support Services plc
    @date 16/05/2008
*/
class MANTID_API_DLL Axis {
public:
  Axis();
  virtual ~Axis() = default;

  /// Virtual constructor
  virtual Axis *clone(const MatrixWorkspace *const parentWorkspace) = 0;
  /// Virtual constructor for axis of different length
  virtual Axis *clone(const std::size_t length, const MatrixWorkspace *const parentWorkspace) = 0;

  const std::string &title() const;
  std::string &title();

  const std::shared_ptr<Kernel::Unit> &unit() const;
  std::shared_ptr<Kernel::Unit> &unit();

  /// Set the unit on the Axis
  virtual const std::shared_ptr<Kernel::Unit> &setUnit(const std::string &unitName);

  /// Returns true is the axis is a Spectra axis
  virtual bool isSpectra() const { return false; }
  /// Returns true if the axis is numeric
  virtual bool isNumeric() const { return false; }
  /// Returns true if the axis is Text
  virtual bool isText() const { return false; }

  /// Returns the value at a specified index
  /// @param index :: the index
  /// @param verticalIndex :: The verticalIndex
  virtual double operator()(const std::size_t &index, const std::size_t &verticalIndex = 0) const = 0;
  /// Gets the value at the specified index. Just calls operator() but is easier
  /// to use with Axis pointers
  double getValue(const std::size_t &index, const std::size_t &verticalIndex = 0) const;
  /// returns min value defined on axis
  virtual double getMin() const = 0;
  /// returns max value defined on axis
  virtual double getMax() const = 0;
  /// Sets the value at the specified index
  /// @param index :: The index
  /// @param value :: The new value
  virtual void setValue(const std::size_t &index, const double &value) = 0;
  /// Find the index of the given double value
  virtual size_t indexOfValue(const double value) const = 0;

  /// Get the spectrum number
  virtual specnum_t spectraNo(const std::size_t &index) const;

  /// Get the length of the axis
  virtual std::size_t length() const = 0;

  /// Check whether two axis are the same, i.e same length and same
  /// spectra_values for all elements in the axis
  virtual bool operator==(const Axis &) const = 0;

  /// Returns a text label of for a value
  virtual std::string label(const std::size_t &index) const = 0;

protected:
  Axis(const Axis &) = default;
  Axis &operator=(const Axis &) = default;

private:
  /// The user-defined title for this axis
  std::string m_title;
  /// The unit for this axis
  std::shared_ptr<Kernel::Unit> m_unit;
};

} // namespace API
} // namespace Mantid
