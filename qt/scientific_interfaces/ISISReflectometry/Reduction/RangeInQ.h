/**
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
#ifndef MANTID_CUSTOMINTERFACES_RANGEINQ_H_
#define MANTID_CUSTOMINTERFACES_RANGEINQ_H_
#include "../DllConfig.h"
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL RangeInQ {
public:
  RangeInQ(boost::optional<double> min = boost::none,
           boost::optional<double> step = boost::none,
           boost::optional<double> max = boost::none);

  boost::optional<double> min() const;
  boost::optional<double> max() const;
  boost::optional<double> step() const;

private:
  boost::optional<double> m_min;
  boost::optional<double> m_step;
  boost::optional<double> m_max;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(RangeInQ const &lhs,
                                               RangeInQ const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(RangeInQ const &lhs,
                                               RangeInQ const &rhs);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_RANGEINQ_H_
