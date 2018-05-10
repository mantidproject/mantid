#ifndef MANTID_DATAHANDLING_DATABLOCKGENERATOR_H_
#define MANTID_DATAHANDLING_DATABLOCKGENERATOR_H_

#include "MantidDataHandling/DllConfig.h"
#include <boost/optional.hpp>
#include <memory>
#include <vector>

namespace Mantid {
namespace DataHandling {

class DataBlock;

/** DataBlockGenerator: The DataBlockGenerator class provides increasing
    int64_t numbers from a collection of intervals which are being input
    into the generator at construction.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at:
<https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport DataBlockGenerator {
public:
  DataBlockGenerator(const std::vector<std::pair<int64_t, int64_t>> &intervals);
  class DataBlock;
  DataBlockGenerator &operator++();
  DataBlockGenerator operator++(int);
  bool isDone();
  int64_t getValue();
  void next();

public:
  std::vector<std::pair<int64_t, int64_t>> m_intervals;
  int64_t m_currentSpectrum;

  boost::optional<size_t> m_currentIntervalIndex;
};

} // namespace DataHandling
} // namespace Mantid

#endif
