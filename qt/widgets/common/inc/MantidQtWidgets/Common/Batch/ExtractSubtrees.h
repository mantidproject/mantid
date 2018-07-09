/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#ifndef MANTIDQTMANTIDWIDGETS_EXTRACTSUBTREES_H_
#define MANTIDQTMANTIDWIDGETS_EXTRACTSUBTREES_H_
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include "MantidQtWidgets/Common/Batch/Row.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/Subtree.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <boost/optional.hpp>
#include <string>
#include <vector>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON ExtractSubtrees {
public:
  using RandomAccessRowIterator = std::vector<Row>::iterator;
  using RandomAccessConstRowIterator = std::vector<Row>::const_iterator;
  boost::optional<std::vector<Subtree>>
  operator()(std::vector<Row> region) const;

private:
  RandomAccessConstRowIterator
  findEndOfSubtree(RandomAccessConstRowIterator subtreeBegin,
                   RandomAccessConstRowIterator regionEnd,
                   int subtreeRootDepth) const;

  Subtree makeSubtreeFromRows(RowLocation subtreeRootLocation,
                              RandomAccessConstRowIterator subtreeBegin,
                              RandomAccessConstRowIterator subtreeEnd) const;

  std::vector<Subtree> makeSubtreesFromRows(std::vector<Row> const &rows,
                                            int subtreeRootDepth) const;
};

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif
