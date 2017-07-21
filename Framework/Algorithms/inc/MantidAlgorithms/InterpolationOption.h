#ifndef MANTID_ALGORITHMS_INTERPOLATIONOPTION_H_
#define MANTID_ALGORITHMS_INTERPOLATIONOPTION_H_

#include "MantidAlgorithms/DllConfig.h"
#include <memory>
#include <string>

namespace Mantid {
namespace HistogramData {
class Histogram;
}
namespace Kernel {
class Property;
}
namespace Algorithms {

/**
  Class to provide a consistent interface to an interpolation option on
  algorithms.

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_ALGORITHMS_DLL InterpolationOption {
public:
  // Indices must match the order in static string array
  enum class Value : uint8_t { Linear, CSpline };

  void set(Value kind);
  void set(const std::string &kind);

  std::unique_ptr<Kernel::Property> property() const;
  std::string propertyDoc() const;

  void applyInplace(HistogramData::Histogram &inOut, size_t stepSize) const;
  void applyInPlace(const HistogramData::Histogram &in,
                    HistogramData::Histogram &out) const;

private:
  Value m_value = Value::Linear;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_INTERPOLATIONOPTION_H_ */
