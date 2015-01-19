#ifndef MANTID_API_AXIS_H_
#define MANTID_API_AXIS_H_

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/Unit.h"
#include "MantidGeometry/IDTypes.h"
#include <string>
#include <vector>

namespace Mantid {
namespace API {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class MatrixWorkspace;

/** Class to represent the axis of a workspace.

    @author Russell Taylor, Tessella Support Services plc
    @date 16/05/2008

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_API_DLL Axis {
public:
  Axis();
  virtual ~Axis();

  /// Virtual constructor
  virtual Axis *clone(const MatrixWorkspace *const parentWorkspace) = 0;
  /// Virtual constructor for axis of different length
  virtual Axis *clone(const std::size_t length,
                      const MatrixWorkspace *const parentWorkspace) = 0;

  const std::string &title() const;
  std::string &title();

  const Kernel::Unit_sptr &unit() const;
  Kernel::Unit_sptr &unit();

  /// Set the unit on the Axis
  virtual const Kernel::Unit_sptr &setUnit(const std::string &unitName);

  /// Returns true is the axis is a Spectra axis
  virtual bool isSpectra() const { return false; }
  /// Returns true if the axis is numeric
  virtual bool isNumeric() const { return false; }
  /// Returns true if the axis is Text
  virtual bool isText() const { return false; }

  /// Returns the value at a specified index
  /// @param index :: the index
  /// @param verticalIndex :: The verticalIndex
  virtual double operator()(const std::size_t &index,
                            const std::size_t &verticalIndex = 0) const = 0;
  /// Gets the value at the specified index. Just calls operator() but is easier
  /// to use with Axis pointers
  double getValue(const std::size_t &index,
                  const std::size_t &verticalIndex = 0) const;
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

  /// Get the spectrum index
  virtual specid_t spectraNo(const std::size_t &index) const;

  /// Get the length of the axis
  virtual std::size_t length() const = 0;

  /// Check whether two axis are the same, i.e same length and same
  /// spectra_values for all elements in the axis
  virtual bool operator==(const Axis &) const = 0;

  /// Returns a text label of for a value
  virtual std::string label(const std::size_t &index) const = 0;

protected:
  Axis(const Axis &right);

private:
  /// Private, undefined copy assignment operator
  const Axis &operator=(const Axis &);

  /// The user-defined title for this axis
  std::string m_title;
  /// The unit for this axis
  Kernel::Unit_sptr m_unit;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_AXIS_H_*/
