#ifndef MANTID_API_TEXTAXIS_H_
#define MANTID_API_TEXTAXIS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Axis.h"
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/Unit.h"

#ifndef Q_MOC_RUN
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
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
class MANTID_API_DLL TextAxis : public Axis {
public:
  TextAxis(const std::size_t &length);
  Axis *clone(const MatrixWorkspace *const parentWorkspace) override;
  Axis *clone(const std::size_t length,
              const MatrixWorkspace *const parentWorkspace) override;
  std::size_t length() const override { return m_values.size(); }
  /// If this is a TextAxis, always return true for this class
  bool isText() const override { return true; }
  /// Get a value at the specified index
  double operator()(const std::size_t &index,
                    const std::size_t &verticalIndex = 0) const override;
  /// Set the value at the specified index
  void setValue(const std::size_t &index, const double &value) override;
  size_t indexOfValue(const double value) const override;

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
  /// Private, undefined copy assignment operator
  const TextAxis &operator=(const TextAxis &);
  /// A vector holding the axis values for the axis.
  std::vector<std::string> m_values;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_TEXTAXIS_H_ */
