#ifndef MANTID_API_NUMERICAXIS_H_
#define MANTID_API_NUMERICAXIS_H_

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
class MANTID_API_DLL NumericAxis : public Axis {
public:
  NumericAxis(const std::size_t &length);
  NumericAxis(const std::vector<double> &centres);

  Axis *clone(const MatrixWorkspace *const parentWorkspace) override;
  Axis *clone(const std::size_t length,
              const MatrixWorkspace *const parentWorkspace) override;
  /// Is the axis numeric - always true for this class
  bool isNumeric() const override { return true; }
  std::size_t length() const override { return m_values.size(); }
  /// Get a value at the specified index
  double operator()(const std::size_t &index,
                    const std::size_t &verticalIndex = 0) const override;
  /// Set the value at a specific index
  void setValue(const std::size_t &index, const double &value) override;
  size_t indexOfValue(const double value) const override;
  bool operator==(const Axis &) const override;
  virtual bool equalWithinTolerance(const Axis &axis2,
                                    const double tolerance) const;
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

private:
  /// Private, undefined copy assignment operator
  const NumericAxis &operator=(const NumericAxis &);
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_NUMERICAXIS_H_ */
