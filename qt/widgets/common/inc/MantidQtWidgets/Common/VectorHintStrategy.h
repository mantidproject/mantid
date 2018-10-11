#ifndef MANTID_MANTIDWIDGETS_VECTORHINTSTRATEGY_H
#define MANTID_MANTIDWIDGETS_VECTORHINTSTRATEGY_H

#include "MantidQtWidgets/Common/HintStrategy.h"
#include "MantidQtWidgets/Common/Hint.h"

namespace MantidQt {
namespace MantidWidgets {
/** VectorHintStrategy : Produces hints using two vectors.

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
class VectorHintStrategy : public HintStrategy {
public:
  VectorHintStrategy(std::vector<Hint> &hints)
      : m_hint(hints){}

  std::vector<Hint> createHints() override {
    return m_hint;
  }

private:
  std::vector<Hint> m_hint;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGY_H */
