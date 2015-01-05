#ifndef MANTID_API_BINEDGEAXIS_H_
#define MANTID_API_BINEDGEAXIS_H_

#include "MantidAPI/NumericAxis.h"

namespace Mantid {
namespace API {

/**
Stores numeric values that are assumed to be bin edge values.

It overrides indexOfValue to search using the values as bin edges are than
centre points

Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL BinEdgeAxis : public NumericAxis {
public:
  BinEdgeAxis(const std::size_t &length);
  BinEdgeAxis(const std::vector<double> &edges);
  virtual ~BinEdgeAxis() {}

  virtual Axis *clone(const MatrixWorkspace *const parentWorkspace);
  virtual Axis *clone(const std::size_t length,
                      const MatrixWorkspace *const parentWorkspace);

  virtual std::vector<double> createBinBoundaries() const;
  void setValue(const std::size_t &index, const double &value);
  size_t indexOfValue(const double value) const;

private:
  /// Private, undefined copy assignment operator
  const BinEdgeAxis &operator=(const BinEdgeAxis &);
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_BINEDGEAXIS_H_ */
