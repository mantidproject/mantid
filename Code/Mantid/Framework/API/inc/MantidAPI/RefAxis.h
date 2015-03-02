#ifndef MANTID_API_REFAXIS_H_
#define MANTID_API_REFAXIS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
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
class MANTID_API_DLL RefAxis : public NumericAxis {
public:
  RefAxis(const std::size_t &length,
          const MatrixWorkspace *const parentWorkspace);
  virtual ~RefAxis();

  Axis *clone(const MatrixWorkspace *const parentWorkspace);
  Axis *clone(const std::size_t length,
              const MatrixWorkspace *const parentWorkspace);
  virtual std::size_t length() const { return m_size; }
  /// Get a value at the specified index
  virtual double operator()(const std::size_t &index,
                            const std::size_t &verticalIndex) const;
  virtual void setValue(const std::size_t &index, const double &value);
  virtual bool operator==(const Axis &) const;
  virtual bool equalWithinTolerance(const Axis &axis2,
                                    const double tolerance = 0.0) const;
  virtual double getMin() const;
  virtual double getMax() const;

private:
  RefAxis(const RefAxis &right, const MatrixWorkspace *const parentWorkspace);
  /// Private, undefined 'regular' copy constructor
  RefAxis(const RefAxis &);
  /// Private, undefined copy assignment operator
  const RefAxis &operator=(const RefAxis &);

  /// A pointer to the workspace holding the axis
  const MatrixWorkspace *const m_parentWS;
  /// Length of the axis
  std::size_t m_size;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_REFAXIS_H_*/
